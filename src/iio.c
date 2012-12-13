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

#include "iio.h"
#include <string.h>

File* file_new(FileType type, gconstpointer handle)
{
	File* file = g_malloc0(sizeof(File));
	file->type = type;

	switch (type) {
		case FILE_STDIO:
			//file->handle.stdfh = *((FILE**) handle);
			memcpy(& file->handle.stdfh, handle, sizeof(FILE*));
			break;
		case FILE_MPI:
			memcpy(& file->handle.mpifh, handle, sizeof(MPI_File));
			break;
		case FILE_POSIX:
			memcpy(& file->handle.posixfh, handle, sizeof(int));
			break;
		case FILE_WIN32:
			g_assert(FALSE);
			break;
	}

	return file;
}

IOStatus iostatus_new(gboolean success, gdouble time, glong data)
{
	IOStatus status;
	status.success = success;
	status.coreTime = coretime_new(time, data);
	return status;
}
