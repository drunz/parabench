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

#include "common.h"
#include "variables.h"
#include "interpreter.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static GHashTable* varmap;


void var_value_destroy_function(gpointer data);

void var_init()
{
	varmap = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, & var_value_destroy_function);
}

void var_free()
{
	g_hash_table_destroy(varmap);
}

VarDesc* var_lookup(const gchar* varname)
{
	return g_hash_table_lookup(varmap, varname);
}

void var_value_destroy_function(gpointer data)
{
    VarDesc* desc = (VarDesc*) data;
    g_free(desc);
}

void var_destroy(const gchar* varname)
{
	g_hash_table_remove(varmap, varname);
}

/**
 * Duplicate data
 * Automatically cleans up old variable if necessary.
 * Memory alignment is as follows:
 * - <StructVarDesc><NameString><Data>
 */
void var_set_value(const gchar* varname, VarType type, gconstpointer data)
{
	int datalength = 0;

	switch(type){
		case VAR_STRING:
			datalength = strlen((gchar*) data) + 1;
			break;
		case VAR_INT:
			//printf(" setVarValue Int: %lu - \n", *((unsigned long*) data));
			datalength = sizeof(glong);
			break;
		case VAR_FILE:
			//g_printf(" setVarValue  File: %p, size = %lu\n", *((FILE**) data), sizeof(FILE*));
			datalength = sizeof(File*);
			break;
		default: g_assert(FALSE);
	}

	int namelength = strlen(varname) + 1;
	int size = sizeof(VarDesc) + namelength + datalength;

	gpointer alloced = g_malloc0(size);
	VarDesc* desc = alloced;

	desc->name  = alloced + sizeof(VarDesc);
	desc->value = alloced + sizeof(VarDesc) + namelength;

	// copy varname
	memcpy(desc->name, varname, namelength);
	memcpy((gpointer) desc->value, data, datalength);
	//g_printf(" setVarValue memcpy  fh = %p\n", *((FILE**) desc->value));

	desc->type = type;

	g_hash_table_replace(varmap,  desc->name,  desc);
}

inline static gboolean is_alpha (gchar c)
{
	return ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'));
}

inline static gboolean is_num (gchar c)
{
	return (c >= '0' && c <= '9');
}

/**
 * Parse a single variable and check end.
 */
static gint var_endpos(const char* var)
{
	gint pos = 0;

	while (is_alpha(var[pos]) || is_num(var[pos])) {
		pos++;
	}

	return pos;
}

gchar* var_replace_substrings(const gchar* what)
{
	if(strchr(what, '$') == NULL){
		return strdup(what);
	}
	GString * resultString = g_string_new("");

	// unprocessed data
	GSList*  toProcess = NULL; // FIXME: memleak?
	toProcess = g_slist_prepend(toProcess, strdup(what));

	// scan repstring for not escaped variables: $[$][A-Za-z0-9]+
	while(toProcess != NULL){
		char * cur_data = toProcess->data;

		// stack
		GSList* old = toProcess;
		toProcess = toProcess->next;
		g_slist_free_1(old);

		//printf("CUR %s\n", cur_data);

		g_assert(cur_data != NULL);

		// at least one $ should be inside
		char * pos = strchr(cur_data, '$');
		if(pos == NULL){
			g_string_append (resultString, cur_data);
		}else{

			// is $ commented out?
			if(cur_data[0] != '$' && pos[-1] == '\\' ){
				// remove slash
				// copy stuff before $ to output string
				pos[-1] = 0;
				g_string_append(resultString, cur_data);


				g_string_append(resultString, "$");

				toProcess = g_slist_prepend(toProcess, strdup(& pos[1]));
			}else {
				// copy stuff before $ to output string
				pos[0] = 0;
				g_string_append(resultString, cur_data);


				// now substitute
				if(pos[1] == '$') { // we have $$
					char * varName = & pos[2];
					gint ipos = var_endpos(varName);


					if( strncmp(varName, "env", 3) == 0){
						// scan for argument()
						gint argLen = 0;
						while( varName[ipos + argLen] != ')' && varName[ipos + argLen] != 0){
							argLen++;
						}
						if(varName[ipos + argLen] != ')' ||varName[ipos] != '(' ){
							printf("Error, $$env requires parameteri.e. $$env(ARG)\n" );
							exit(1);
						}
						// now we can extract the parameter:
						varName[ipos + argLen] = 0;
						varName[ipos] = 0;

						char * envVar = & varName[ipos+1];
						char * envValue = getenv(envVar);
						if(envValue == NULL){
							printf("Error, environment variable %s used but not set\n", envVar);
							exit(1);
						}

						g_string_append(resultString, envValue);

						ipos = ipos + argLen;
					}

					// add post variable data
					toProcess = g_slist_prepend(toProcess, strdup(varName + ipos));
					// variable name:
					varName[ipos] = 0;

					//printf("%s \n", pos + 2);
					if( strstr(varName, "rank") != NULL){
						g_string_append_printf(resultString, "%d", rank);
					}else if( strstr(varName, "rand") != NULL){
						g_string_append_printf(resultString, "%u", g_random_int());
					}else if( strstr(varName, "crand") != NULL){
#ifdef HAVE_MPI
						g_string_append_printf(resultString, "%u", getCollectiveRandomNumber());
#endif
					}else if( strstr(varName, "env") != NULL){
						// already processed
					}else{
						printf("Error invalid global variable $$%s in string \"%s\"\n", varName, what);
						exit(1);
					}
				}else{ // single dollar
					char * varName = & pos[1];
					gint ipos = var_endpos(varName);

					// add post variable data
					toProcess = g_slist_prepend(toProcess, strdup(varName + ipos));

					// variable name:
					varName[ipos] = 0;

					// todo: check for integer values
					VarDesc * var = g_hash_table_lookup(varmap, varName);

					if(var != NULL)
					{
						switch(var->type){
						case VAR_STRING:
							toProcess = g_slist_prepend(toProcess, strdup(var->value));
							break;
						case VAR_INT:
							g_string_append_printf(resultString, "%lu", *((long unsigned *) (var->value)));
							break;
						case VAR_FILE:
							g_assert(FALSE);
							break;
						}
					}else{
						printf("Variable unknown: %s in string \"%s\"\n", varName, what);
						exit(1);
					}
				}
			}
		}
		free(cur_data);
	}


	gchar * retValue = strdup(resultString->str);
	g_string_free(resultString, TRUE);
	return retValue;
}
