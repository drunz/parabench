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

#ifndef VARIABLES_H_
#define VARIABLES_H_

#include "config.h"
#include <glib.h>


typedef enum {
	VAR_STRING, VAR_INT, VAR_FILE
} VarType;

typedef struct {
	gchar* name;	// name of the variable
	VarType type;	// type of the variable
	gconstpointer value; //value
} VarDesc;

void     var_init();
void     var_free();
VarDesc* var_lookup(const gchar* varname);
void     var_destroy(const gchar* varname);
void     var_set_value(const gchar* varname, VarType type, gconstpointer data);
gchar*   var_replace_substrings(const gchar* what);

#endif /* VARIABLES_H_ */
