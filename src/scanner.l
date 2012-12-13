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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "parser.h"

gchar* strstrip(gchar* string);
glong atol_extended(gchar* str);

%}

%option noinput
%option nounput
%option bison-bridge bison-locations

%%

"SEEK_SET"					{ yylval->num = SEEK_SET; return TDIGIT; }
"SEEK_CUR"					{ yylval->num = SEEK_CUR; return TDIGIT; }
"SEEK_END"					{ yylval->num = SEEK_END; return TDIGIT; }

repeat						return TREPEAT;
time						return TTIME;
ctime						return TCTIME;
define						return TDEFINE;
groups						return TGROUPS;
pattern						return TPATTERN;
group						return TGROUP;
master						return TMASTER;
param						return TPARAM;
barrier						return TBARRIER;
sleep						return TSLEEP;
print						return TPRINT;
fcreat						return TFCREAT;
fopen						return TFOPEN;
fclose						return TFCLOSE;
fwrite						return TFWRITE;
fread						return TFREAD;
fseek						return TFSEEK;
fsync						return TFSYNC;
write						return TWRITE;
append						return TAPPEND;
read						return TREAD;
lookup						return TLOOKUP;
delete						return TDELETE;
mkdir						return TMKDIR;
rmdir						return TRMDIR;
create						return TCREATE;
stat						return TSTAT;
rename						return TRENAME;
pfopen						return TPFOPEN;
pfclose						return TPFCLOSE;
pfwrite						return TPFWRITE;
pfread						return TPFREAD;
pwrite						return TPWRITE;
pread						return TPREAD;
pdelete						return TPDELETE;
S							return TTAGS;
D							return TTAGD;
[0-9]+[kmg]?				{ yylval->num = atol_extended(yytext); return TDIGIT; }
\"([^"\n]|\\["\n])*\"		{ yylval->str = strdup(strstrip(yytext)); return TSTRING; }
"$"[a-zA-Z0-9]*  			{ yylval->str = strdup(yytext); return TVAR; }
"$$"[a-zA-Z][a-zA-Z0-9]*  	{ yylval->str = strdup(yytext); return TINVAR; }
\[                      	return TKBRACEL;
\]                      	return TKBRACER;
\{                      	return TEBRACEL;
\}                      	return TEBRACER;
\(							return TOBRACEL;
\)							return TOBRACER;
\=							return TEQUAL;
\+							return TADD;
\-							return TSUB;
\%							return TMOD;
\*							return TMUL;
\/							return TDIV;
\^							return TPOW;
\,                       	return TCOMMA;
\;                       	return TSEMICOLON;
\:                       	return TCOLON;

                            /* ![ModuleHook] scanner_keyword */

"/*""/"*([^*/]|[^*]"/"|"*"[^/])*"*"*"*/"	/* ignore multiline comments */
"#"(.)*										/* ignore single line comments */
\n                      					/* ignore newline */;
[ \t]+                  					/* ignore whitespace */;
[a-zA-Z]+									printf("Scanner Error: invalid keyword \"%s\"\n", yytext);

%%

gchar* strstrip(gchar *string) {
	g_return_val_if_fail (string != NULL, NULL);
	
	// strip front
	guchar *start;

	for(start = (guchar*) string; *start && (((int) *start) == 34); start++);
	
	g_memmove(string, start, strlen ((gchar*) start) + 1);

	// strip back
	gsize len;
	
	len = strlen (string);
	while(len--) {
		if(((int) string[len]) == 34)
			string[len] = '\0';
		else
			break;
	}
	
	return string;
}

glong atol_extended(gchar* str) {
	glong size = 1;
	gint len = strlen(str);
	
	if (str[len-1] == 'k') 
		size = 1024;
	else if (str[len-1] == 'm') 
		size = 1024*1024;
	else if (str[len-1] == 'g') 
		size = 1024*1024*1024;
	
	if (size > 1)
		str[len-1] = '\0';
		
	//g_printf("atol_extended: %s -> %ld\n", str, atol(str)*size);
		
	return (glong) atol(str)*size;
}
