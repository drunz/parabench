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

#include "statements.h"
#include <string.h>

Statement* stmt_new(StatementType type, ParameterList* parameters, gchar* label, guint line)
{
	gint labelLength = (label? (strlen(label)+1) : 0);

	//memalign: <struct><label>
	Statement* stmt = g_malloc0(sizeof(Statement) + labelLength);
	stmt->type = type;
	stmt->parameters = parameters;
	stmt->label = (label? ((gpointer) stmt + sizeof(Statement)) : NULL);
	if (label) memcpy((gpointer) stmt->label, label, labelLength);
	stmt->line = line;
	return stmt;
}

void stmt_free(Statement* statement)
{
	if (!statement) return;

	param_list_free(statement->parameters);
	g_free(statement);
}

gchar* stmt_get_string(StatementType type)
{
	switch (type) {
		/* POSIX I/O Statements */
		case STMT_FOPEN:  return "FOpen";
		case STMT_FCLOSE: return "FClose";
		case STMT_FWRITE: return "FWrite";
		case STMT_FREAD:  return "FRead";
		case STMT_WRITE:  return "Write";
		case STMT_APPEND: return "Append";
		case STMT_READ:   return "Read";
		case STMT_LOOKUP: return "Lookup";
		case STMT_DELETE: return "Delete";
		case STMT_MKDIR:  return "Mkdir";
		case STMT_RMDIR:  return "Rmdir";
		case STMT_CREATE: return "Create";
		case STMT_STAT:   return "Stat";
		case STMT_RENAME: return "Rename";
		case STMT_FSEEK:  return "FSeek";
		case STMT_FCREAT: return "FCreat";
		case STMT_FSYNC:  return "FSync";

		/* MPI I/O Statements */
		case STMT_PFOPEN:  return "PFOpen";
		case STMT_PFCLOSE: return "PFClose";
		case STMT_PFWRITE: return "PFWrite";
		case STMT_PFREAD:  return "PFRead";
		case STMT_PWRITE:  return "PWrite";
		case STMT_PREAD:   return "PRead";
		case STMT_PDELETE: return "PDelete";

		/* Auxiliary Statements */
		case STMT_REPEAT:  return "repeat";
		case STMT_TIME:    return "time";
		case STMT_CTIME:   return "ctime";
		case STMT_DEFINE:  return "define";
		case STMT_ASSIGN:  return "assign";
		case STMT_GROUP:   return "group";
		case STMT_MASTER:  return "master";
		case STMT_BARRIER: return "barrier";
		case STMT_SLEEP:   return "sleep";
		case STMT_PRINT:   return "print";
		case STMT_BLOCK:   return "block";

		default: return "unknown";
	}
}
