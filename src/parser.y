/* Parabench - A parallel file system benchmark
 * Copyright (C) 2009-2010  Dennis Runz
 * University of Heidelberg
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

%{
/* Pass the argument to yyparse through to yylex. */
#define YYPARSE_PARAM scanner
#define YYLEX_PARAM   scanner
#define YYERROR_VERBOSE 1

// http://www.gnu.org/software/bison/manual/html_node/Memory-Management.html
#define YYMAXDEPTH 40000

extern int yylineno;

extern int yylex();
void yyerror(const char *str);

#include "common.h"
#include "interpreter.h"
#include "expressions.h"
#include "parameters.h"
#include "statements.h"
#include "groups.h"
#include "patterns.h"
#include "ast.h"
#ifdef HAVE_MPI
  #include <mpi.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <glib.h>
#include <fcntl.h>


inline gboolean reverse_wrapper(GNode *node, gpointer data);
int translate_posix_flags(gchar* str);
void replace_posix_open_flags(ParameterList* paramList);

%}

%locations
%pure_parser

%union {
	glong num;
	gchar *str;
	Expression *expr;
	int type;
	ParameterList *paramList;
	GNode* node;
	GSList* list;
	Group* group;
}

%token TREPEAT TTIME TCTIME TDEFINE TGROUPS TPATTERN TGROUP TMASTER TBARRIER TSLEEP TPARAM
%token TPFOPEN TPFCLOSE TPFWRITE TPFREAD
%token TKBRACEL TKBRACER TEBRACEL TEBRACER TOBRACEL TOBRACER 
%token TEQUAL TADD TSUB TMOD TMUL TDIV TPOW TCOMMA TSEMICOLON TCOLON TTAGS TTAGD

%token <num> TPRINT TWRITE TAPPEND TREAD TLOOKUP TDELETE TMKDIR TRMDIR TCREATE TSTAT TRENAME
%token <num> TFCREAT TFOPEN TFCLOSE TFWRITE TFREAD TFSEEK TFSYNC
%token <num> TPWRITE TPREAD TPDELETE
%token <num> TDIGIT
%token <str> TSTRING TVAR TINVAR

/* ![ModuleHook] parser_token */

%type <node> Block StatementList Statement RepeatStatement CoreTimeStatement Function
%type <node> Command Assign TimeStatement GroupStatement MasterStatement BarrierStatement
%type <num> Number GroupTag SubgroupTag
%type <str> Variable Label
%type <type> CommandIdentifier FunctionIdentifier
%type <paramList> ParameterList
%type <expr> Expression IntExpression StringExpression
%type <list> GroupList
%type <group> Group

/* [http://www-is.informatik.uni-oldenburg.de/~dibo/teaching/java9900/vorlesungen/vorlesung4/sld028.htm] */
/* Associativity / Precedence (ascending) */
%left  TSUB TADD
%left  TMOD
%left  TMUL TDIV
%left  NEG /* negation--unary minus */
%right TPOW

%start Program


%%


Program : /* empty */
        | Defines StatementList {
            ast = $2;
            // Statement list nodes are prepended for performance reasons
            // Thus all node's children need to be reversed
            g_node_traverse(ast, G_PRE_ORDER, G_TRAVERSE_NON_LEAVES, -1, &reverse_wrapper, NULL);
          }
        ;

//====================================================
// DEFINES
//====================================================
Defines : /* empty */
        | Defines DefineGroups
        | Defines DefineParameters
        | Defines DefinePattern
        ;

DefineGroups : TDEFINE TGROUPS TEBRACEL GroupList TEBRACER TSEMICOLON {
               #ifdef HAVE_MPI
				 if(!groupsDefined) {
                   create_groups($4);
                   groupsDefined = TRUE;
                 }
                 else yyerror("Multiple define of groups not allowed!");
               #endif
               }
             ;

Group : TSTRING GroupTag SubgroupTag {
        #ifdef HAVE_MPI
          $$ = group_new($1, $2, $3);
          free($1);
        #endif
        }
      ;

GroupList : Group { $$ = NULL; $$ = g_slist_append($$, $1); }
          | GroupList TCOMMA Group { $1 = g_slist_append($1, $3); }
          ;

GroupTag : /* empty */  { $$ = TAG_NONE; }
         | TCOLON TTAGS { $$ = TAG_SINGLE; }
         | TCOLON TTAGD { $$ = TAG_DISJOINT; }
         ;

SubgroupTag : /* empty */ { $$ = 0; }
            | Number { $$ = $1; }
            | TDIV Number { $$ = $2; }
            ;

DefineParameters : TDEFINE TPARAM TSTRING TVAR TSTRING {
                     param_list_entry * data = malloc(sizeof(param_list_entry));
                     data->param_name = $3;    // TODO: testme  strdup($3);
                     data->var_name = & $4[1];    // TODO: testme  strdup($4 +1);
                     data->default_value = $5; // TODO: testme  strdup($5);
                     paramList = g_slist_prepend(paramList, data);
                   }
                 ;

DefinePattern : TDEFINE TPATTERN TEBRACEL TSTRING TCOMMA Number TCOMMA Number TCOMMA Number TCOMMA Number TEBRACER TSEMICOLON {
                #ifdef HAVE_MPI
                  // NULL group will create data type for world
                  create_pattern(strdup($4), $6, $8, $10, $12, NULL);
                #endif
                }
              | TDEFINE TPATTERN TEBRACEL TSTRING TCOMMA Number TCOMMA Number TCOMMA Number TCOMMA Number TCOMMA TSTRING TEBRACER TSEMICOLON {
                #ifdef HAVE_MPI
                  GroupBlock* group = g_hash_table_lookup(groupMap, $14);
                  if (group) create_pattern(strdup($4), $6, $8, $10, $12, group);
                  else yyerror("Group specified in pattern not defined!");
                  free($14);
                #endif
                }
              ;

//====================================================
// STATEMENTS
//====================================================
StatementList : Statement { 
                  if ($1 != NULL) {
                    $$ = g_node_new(stmt_new(STMT_BLOCK, NULL, NULL, yylineno));
                    g_node_append($$, $1);
                  }
                }
              | StatementList Statement { if ($2 != NULL) g_node_prepend($1, $2); } 
              // left recursion can handle any number of Statements with bounded stack space
              ;

Statement : RepeatStatement { $$ = $1; }
          | TimeStatement { $$ = $1; }
          | CoreTimeStatement { $$ = $1; }
          | GroupStatement { $$ = $1; }
          | MasterStatement { $$ = $1; }
          | BarrierStatement { $$ = $1; }
          | Assign { $$ = $1; }
          | Command { $$ = $1; }
          | Function { $$ = $1; }
          | TSEMICOLON { $$ = NULL; }
          ;

Block : TEBRACEL StatementList TEBRACER { $$ = $2; }
      | TEBRACEL TEBRACER { $$ = NULL; }
      ;

Label : TKBRACEL TSTRING TKBRACER { $$ = $2; };

RepeatStatement : TREPEAT TOBRACEL TVAR TCOMMA IntExpression TOBRACER Block {
                    ParameterList* paramList = param_list_new();
                    Expression* e = expr_constant_string_new(& $3[1]);
                    param_list_append(paramList, e);
                    param_list_append(paramList, $5);
                    
                    // Repeat statement is implicit block, thus we merge
                    GNode* node = $7;
                    g_free(node->data); 
                    node->data = stmt_new(STMT_REPEAT, paramList, NULL, yylineno);
                    
                    $$ = node;
                    free($3);
                  }
                | TREPEAT TOBRACEL TVAR TCOMMA IntExpression TOBRACER Statement {
                    ParameterList* paramList = param_list_new();
                    Expression* e = expr_constant_string_new(& $3[1]);
                    param_list_append(paramList, e);
                    param_list_append(paramList, $5);
                    
                    GNode* node = g_node_new(stmt_new(STMT_REPEAT, paramList, NULL, yylineno));
                    if ($7) g_node_append(node, $7);
                    
                    $$ = node;
                    free($3);
                  }
                | TREPEAT TVAR IntExpression Block {
                    ParameterList* paramList = param_list_new();
                    Expression* e = expr_constant_string_new(& $2[1]);
                    param_list_append(paramList, e);
                    param_list_append(paramList, $3);
                    
                    // Repeat statement is implicit block, thus we merge
                    GNode* node = $4;
                    g_free(node->data); 
                    node->data = stmt_new(STMT_REPEAT, paramList, NULL, yylineno);
                    
                    $$ = node;
                    free($2);
                  }
                | TREPEAT TVAR IntExpression Statement {
                    ParameterList* paramList = param_list_new();
                    Expression* e = expr_constant_string_new(& $2[1]);
                    param_list_append(paramList, e);
                    param_list_append(paramList, $3);
                    
                    GNode* node = g_node_new(stmt_new(STMT_REPEAT, paramList, NULL, yylineno));
                    if ($4) g_node_append(node, $4);
                    
                    $$ = node;
                    free($2);
                  }
                ;

TimeStatement : TTIME Label Block {
                  // Time statement is implicit block, thus we merge
                  GNode* node = $3;
                  g_free(node->data); 
                  node->data = stmt_new(STMT_TIME, NULL, $2, yylineno);
                  
                  $$ = node;
                  free($2);
                }
              | TTIME Label Statement {
                  GNode* node = g_node_new(stmt_new(STMT_TIME, NULL, $2, yylineno));
                  if ($3) g_node_append(node, $3);
                  
                  $$ = node;
                  free($2);
                }
              | TTIME Block {
                  // Time statement is implicit block, thus we merge
                  GNode* node = $2;
                  g_free(node->data); 
                  node->data = stmt_new(STMT_TIME, NULL, "", yylineno);
                  
                  $$ = node;
                }
              | TTIME Statement {
                  GNode* node = g_node_new(stmt_new(STMT_TIME, NULL, "", yylineno));
                  if ($2) g_node_append(node, $2);
                  
                  $$ = node;
                }
              ;

CoreTimeStatement : TCTIME Label Block {
                      // Time statement is implicit block, thus we merge
                      GNode* node = $3;
                      g_free(node->data); 
                      node->data = stmt_new(STMT_CTIME, NULL, $2, yylineno);
                      
                      $$ = node;
                      free($2);
                    }
                  | TCTIME Label Statement {
                      GNode* node = g_node_new(stmt_new(STMT_CTIME, NULL, $2, yylineno));
                      if ($3) g_node_append(node, $3);
                      
                      $$ = node;
                      free($2);
                    }
                  | TCTIME Block {
                      // Time statement is implicit block, thus we merge
                      GNode* node = $2;
                      g_free(node->data); 
                      node->data = stmt_new(STMT_CTIME, NULL, "", yylineno);
                      
                      $$ = node;
                    }
                  | TCTIME Statement {
                      GNode* node = g_node_new(stmt_new(STMT_CTIME, NULL, "", yylineno));
                      if ($2) g_node_append(node, $2);
                      
                      $$ = node;
                    }
                  ;

GroupStatement : TGROUP TOBRACEL TSTRING TOBRACER Block {
                   // Group statement is implicit block, thus we merge
                   GNode* node = $5;
                   g_free(node->data); 
                   node->data = stmt_new(STMT_GROUP, NULL, $3, yylineno);
                   
                   $$ = node;
                   free($3);
                 }
               | TGROUP TSTRING Block {
                   // Group statement is implicit block, thus we merge
                   GNode* node = $3;
                   g_free(node->data); 
                   node->data = stmt_new(STMT_GROUP, NULL, $2, yylineno);
                   
                   $$ = node;
                   free($2);
                 }
               ;

MasterStatement : TMASTER Block {
                    // Master statement is implicit block, thus we merge
                    GNode* node = $2;
                    g_free(node->data); 
                    node->data = stmt_new(STMT_MASTER, NULL, NULL, yylineno);
                    
                    $$ = node;
                  }
                | TMASTER Statement {
                    GNode* node = g_node_new(stmt_new(STMT_MASTER, NULL, NULL, yylineno));
                    if ($2) g_node_append(node, $2);
                    
                    $$ = node;
                  }
                ;

BarrierStatement : TBARRIER TSEMICOLON {
                     ParameterList* paramList = param_list_new();
                     GNode* node = g_node_new(stmt_new(STMT_BARRIER, paramList, NULL, yylineno));
                     $$ = node;
                   }
                 | TBARRIER TOBRACEL TSTRING TOBRACER TSEMICOLON {
                     ParameterList* paramList = param_list_new();
                     Expression* e = expr_constant_string_new($3);
                     param_list_append(paramList, e);
                     
                     GNode* node = g_node_new(stmt_new(STMT_BARRIER, paramList, NULL, yylineno));
                     
                     $$ = node;
                     free($3);
                   }
                 ;

//====================================================
// EXPRESSIONS
//====================================================
Expression : IntExpression { $$ = $1; }
           | StringExpression { $$ = $1; }
           ;

IntExpression : TDIGIT { $$ = expr_constant_int_new($1); }
              | Variable { $$ = expr_variable_new(& $1[1]); free($1); }
              | IntExpression TADD IntExpression { $$ = expr_rich_int_new(OP_ARITH_ADD, $1, $3); }
              | IntExpression TSUB IntExpression { $$ = expr_rich_int_new(OP_ARITH_SUB, $1, $3); }
              | IntExpression TMOD IntExpression { $$ = expr_rich_int_new(OP_ARITH_MOD, $1, $3); }
              | IntExpression TMUL IntExpression { $$ = expr_rich_int_new(OP_ARITH_MUL, $1, $3); }
              | IntExpression TDIV IntExpression { $$ = expr_rich_int_new(OP_ARITH_DIV, $1, $3); }
              | IntExpression TPOW IntExpression { $$ = expr_rich_int_new(OP_ARITH_POW, $1, $3); }
              | TOBRACEL IntExpression TOBRACER  { $$ = $2; }
              ;

StringExpression : TSTRING { $$ = expr_constant_string_new($1); free($1); }
                 ;

//====================================================
// CONSTANTS
//====================================================
Number : TDIGIT { $$ = $1; }
       | Number TADD Number { $$ = $1 + $3; }
       | Number TSUB Number { $$ = $1 - $3; }
       | Number TMOD Number { $$ = $1 % $3; }
       | Number TMUL Number { $$ = $1 * $3; }
       | Number TDIV Number { if($3 == 0) { yyerror("Division by zero!"); } $$ = (int) ($1 / $3); }
       | TSUB Number %prec NEG { $$ = -$2; }
       | Number TPOW Number { $$ = (int) pow($1, $3); }
       | TOBRACEL Number TOBRACER  { $$ = $2; }
       ;

//====================================================
// ASSIGNS
//====================================================
Assign : TVAR TEQUAL Expression TSEMICOLON { 
           ParameterList* paramList = param_list_new();
           Expression* e = expr_constant_string_new(& $1[1]);
           param_list_append(paramList, e);
           param_list_append(paramList, $3);
           $$ = g_node_new(stmt_new(STMT_ASSIGN, paramList, NULL, yylineno));
           free($1);
         }
       ;

//====================================================
// VARIABLES
//====================================================
Variable : TVAR | TINVAR;

//====================================================
// PARAMETERS
//====================================================
ParameterList : /* empty */ { $$ = param_list_new(); }
              | Expression { $$ = param_list_new(); param_list_append($$, $1); } 
              | ParameterList TCOMMA Expression { param_list_append($1, $3); }
              ;

//====================================================
// COMMANDS
//====================================================
Command : CommandIdentifier TOBRACEL ParameterList TOBRACER TSEMICOLON {
             $$ = g_node_new(stmt_new($1, $3, NULL, yylineno));
          }
		| CommandIdentifier ParameterList TSEMICOLON {
             $$ = g_node_new(stmt_new($1, $2, NULL, yylineno));
          }
        ;


CommandIdentifier : TPRINT   { $$ = STMT_PRINT; }
                  | TSLEEP   { $$ = STMT_SLEEP; }
                  | TFREAD   { $$ = STMT_FREAD; }
                  | TFWRITE  { $$ = STMT_FWRITE; }
                  | TFCLOSE  { $$ = STMT_FCLOSE; }
                  | TFSEEK   { $$ = STMT_FSEEK; }
                  | TFSYNC   { $$ = STMT_FSYNC; }
                  | TWRITE   { $$ = STMT_WRITE; }
                  | TAPPEND  { $$ = STMT_APPEND; }
                  | TREAD    { $$ = STMT_READ; }
                  | TLOOKUP  { $$ = STMT_LOOKUP; }
                  | TDELETE  { $$ = STMT_DELETE; }
                  | TMKDIR   { $$ = STMT_MKDIR; }
                  | TRMDIR   { $$ = STMT_RMDIR; }
                  | TCREATE  { $$ = STMT_CREATE; }
                  | TSTAT    { $$ = STMT_STAT; }
                  | TRENAME  { $$ = STMT_RENAME; }
                  | TPFCLOSE { $$ = STMT_PFCLOSE; }
                  | TPFWRITE { $$ = STMT_PFWRITE; }
                  | TPFREAD  { $$ = STMT_PFREAD; }
                  | TPWRITE  { $$ = STMT_PWRITE; }
                  | TPREAD   { $$ = STMT_PREAD; }
                  | TPDELETE { $$ = STMT_PDELETE; }
                  /* ![ModuleHook] parser_identifier */
                  ;

//====================================================
// FUNCTIONS
//====================================================

Function : TVAR TEQUAL FunctionIdentifier TOBRACEL ParameterList TOBRACER TSEMICOLON {
             // Special case handling
             if ($3 == STMT_FOPEN) {
                 // Posix open requires integer flags
                 // To allow "w" "r+" ... flags, second parameters is replaced
                 replace_posix_open_flags($5);
             }
             
             // For setting file handles, expressions must be modeled as constant strings
             // to prevent evaluation of the string from a variable.
             // File handle parameters must be modeled as variable to allow evaluation to handle.
             Expression* e = expr_constant_string_new(& $1[1]);
             param_list_prepend($5, e);
             $$ = g_node_new(stmt_new($3, $5, NULL, yylineno));
             free($1);
         }

FunctionIdentifier : TFCREAT { $$ = STMT_FCREAT; }
                   | TFOPEN  { $$ = STMT_FOPEN; }
                   | TPFOPEN { $$ = STMT_PFOPEN; }
                   ;


%%

void yyerror(const char *str)
{
	if(rank == MASTER) {
		fprintf(stderr, "Parser error in line %d: %s\n", yylineno, str);
	}
	
	iiFree();
#ifdef HAVE_MPI
	MPI_Finalize();
#endif
	exit(-1);
}

int yywrap()
{
	return 1;
}

inline gboolean reverse_wrapper(GNode *node, gpointer data)
{
	g_node_reverse_children(node);
	return FALSE;
}

int translate_posix_flags(gchar* str)
{
	int flags = 0;
	
	if (strcmp(str, "r") == 0)
		return O_RDONLY;
	if (strcmp(str, "w") == 0)
		return O_WRONLY|O_TRUNC|O_CREAT;
	if (strcmp(str, "a") == 0)
		return O_APPEND|O_CREAT|O_WRONLY;
	if (strcmp(str, "r+") == 0)
		return O_RDWR;
	if (strcmp(str, "w+") == 0)
		return O_RDWR|O_TRUNC|O_CREAT;
	if (strcmp(str, "a+") == 0)
		return O_APPEND|O_CREAT;

	if (strstr(str, "O_RDONLY") != 0)
		flags |= O_RDONLY;
	if (strstr(str, "O_WRONLY") != 0)
		flags |= O_WRONLY;
	if (strstr(str, "O_RDWR") != 0)
		flags |= O_RDWR;
	if (strstr(str, "O_APPEND") != 0)
		flags |= O_APPEND;
	if (strstr(str, "O_ASYNC") != 0)
		flags |= O_ASYNC;
	if (strstr(str, "O_CREAT") != 0)
		flags |= O_CREAT;
	if (strstr(str, "O_DIRECT") != 0)
		flags |= O_DIRECT;
	if (strstr(str, "O_DIRECTORY") != 0)
		flags |= O_DIRECTORY;
	if (strstr(str, "O_EXCL") != 0)
		flags |= O_EXCL;
	if (strstr(str, "O_LARGEFILE") != 0)
		flags |= O_LARGEFILE;
	if ((strstr(str, "O_NOATIME") != 0) || (strstr(str, "0x200000") != 0))
		flags |= O_NOATIME;
	if (strstr(str, "O_NOCTTY") != 0)
		flags |= O_NOCTTY;
	if (strstr(str, "O_NOFOLLOW") != 0)
		flags |= O_NOFOLLOW;
	if (strstr(str, "O_NONBLOCK") != 0)
		flags |= O_NONBLOCK;
	if (strstr(str, "O_NDELAY") != 0)
		flags |= O_NDELAY;
	if (strstr(str, "O_SYNC") != 0)
		flags |= O_SYNC;
	if (strstr(str, "O_TRUNC") != 0)
		flags |= O_TRUNC;

	//printf("RDONLY = %d\n", O_RDONLY);
	//if (flags == 0)
	//	printf("Line: %d\n", yylineno);
	//g_assert(flags != 0);
	return flags;
}

/**
 * Posix open requires integer flags
 * To allow "w" "r+" ... flags, second parameters is replaced
 */
void replace_posix_open_flags(ParameterList* paramList)
{
	g_assert(paramList);
	Expression* e = param_index_get(paramList, 1);
	gchar* str = param_string_get(paramList, 1, NULL);
	gint flags = translate_posix_flags(str);
	
	Expression* eNew = expr_constant_int_new(flags);
	param_list_insert(paramList, 1, eNew);
	
	expr_free(e);
	g_free(str);
}
