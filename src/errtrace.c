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

#include "errtrace.h"
#include "common.h"

#include <stdio.h>

extern gchar* sourceFileName;


void backtrace(Statement* stmt)
{
	Log("Statement \"%s\" failed to execute in line %d:", stmt_get_string(stmt->type), stmt->line);
	print_line(sourceFileName, stmt->line);
}

gchar* print_line(const gchar* filename, guint line)
{
	gchar strbuf[1024];
	guint _line = line;

	FILE* fh = fopen(filename , "r");
	if (fh == NULL) Log("Error opening file!");
	else {
		while (_line-- > 0) fgets(strbuf, 1024, fh);
		Log("%s:%d >> %s", filename, line, strbuf);
		fclose (fh);
	}
	return 0;

}
