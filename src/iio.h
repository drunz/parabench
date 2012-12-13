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

#ifndef IIO_H_
#define IIO_H_

#include "common.h"
#include "timing.h"
#include "patterns.h"

#include <stdlib.h>
#include <glib.h>
#include <glib/gstdio.h>

MPI_Info info;

// I/O parameter defaults
enum { OFFSET_CUR = -1, READALL = -1 };

typedef union {
	FILE* stdfh;
	MPI_File mpifh;
	int posixfh;
} FileHandle;

typedef enum {
	FILE_STDIO, FILE_MPI,
	FILE_POSIX, FILE_WIN32
} FileType;

typedef struct {
	FileHandle handle;
	FileType type;
} File;

typedef struct {
	gboolean success;
	CoreTime coreTime;
} IOStatus;


File* file_new(FileType type, gconstpointer handle);
IOStatus iostatus_new(gboolean success, gdouble time, glong data);

#endif /* IIO_H_ */
