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
#include "iio_mpi.h"

#ifdef HAVE_MPI

gboolean iio_pfopen(const gchar* filename, const gchar* mode, MPI_Comm comm, File** file)
{
	MPI_File fh;
	gint smode = MPI_MODE_RDWR | MPI_MODE_CREATE;

	MPI_ASSERT(MPI_File_open(comm, (gchar*) filename, smode, info, &fh), "PFOpen", FALSE);

	*file = file_new(FILE_MPI, &fh);
	return TRUE;
}

gboolean iio_pfclose(File* file)
{
	g_assert(file);
	g_assert(file->type == FILE_MPI);

	MPI_ASSERT(MPI_File_close(& file->handle.mpifh), "PFClose", FALSE);
	return TRUE;
}

/* Level 0: non-collective, contiguous */
IOStatus iio_pfwrite_level0(const File* file, Pattern* pattern)
{
	g_assert(file);
	g_assert(file->type == FILE_MPI);

	MPI_File fh = file->handle.mpifh;
	MPI_Status status;
	gint i, count = 0;
	gchar* buffer;

	if ((buffer = g_malloc0(pattern->iter * pattern->elem * pattern->type_size)) == NULL) {
		Warning("PFWrite: Couldn't allocate %ld bytes of memory!\n",
				(pattern->iter * pattern->elem * pattern->type_size));
		return iostatus_new(FALSE, 0, 0);
	}

	MPI_ASSERT(MPI_File_set_view(fh, 0, MPI_BYTE, pattern->datatype, "native", info), "PFWrite Level0", FALSE)
	MPI_ASSERT(MPI_File_seek(fh, 0, MPI_SEEK_SET), "PFWrite Level0", FALSE)

	// write data to file
	CORETIME_START();
	for (i = 0; i < pattern->iter; ++i) {
		gint iterCount;
		MPI_File_write(fh, buffer, pattern->elem, MPI_BYTE, &status);

		MPI_Get_count(&status, MPI_BYTE, &iterCount);
		count += iterCount;
	}
	CORETIME_STOP(time);

	g_free(buffer);

	if (count == (pattern->iter * pattern->elem)) {
		Verbose("(PFWrite Level0) Wrote %d bytes", count);
		return iostatus_new(TRUE, time, count);
	}
	else {
		Warning("PFWrite Level0: Error during write! (%d of %d)\n", count, (pattern->iter * pattern->elem));
		return iostatus_new(FALSE, time, count);
	}
}

/* Level 1: collective, contiguous */
IOStatus iio_pfwrite_level1(const File* file, Pattern* pattern)
{
	g_assert(file);
	g_assert(file->type == FILE_MPI);

	MPI_File fh = file->handle.mpifh;
	MPI_Status status;
	gint i, count = 0;
	gchar* buffer;

	if ((buffer = g_malloc0(pattern->iter * pattern->elem * pattern->type_size)) == NULL) {
		Warning("PFWrite: Couldn't allocate %ld bytes of memory!\n",
				(pattern->iter * pattern->elem * pattern->type_size));
		return iostatus_new(FALSE, 0, 0);
	}

	MPI_ASSERT(MPI_File_set_view(fh, 0, MPI_BYTE, pattern->datatype, "native", info), "PFWrite Level1", FALSE)
	MPI_ASSERT(MPI_File_seek(fh, 0, MPI_SEEK_SET), "PFWrite Level1", FALSE)

	// write data to file
	CORETIME_START();
	for (i = 0; i < pattern->iter; ++i) {
		gint iterCount;
		MPI_File_write_all(fh, buffer, pattern->elem, MPI_BYTE, &status);

		MPI_Get_count(&status, MPI_BYTE, &iterCount);
		count += iterCount;
	}
	CORETIME_STOP(time);

	g_free(buffer);

	if (count == (pattern->iter * pattern->elem)) {
		Verbose("(PFWrite Level1) Wrote %d bytes", count);
		return iostatus_new(TRUE, time, count);
	}
	else {
		Warning("PFWrite Level1: Error during write! (%d of %d)\n", count, (pattern->iter * pattern->elem));
		return iostatus_new(FALSE, time, count);
	}
}

/* Level 2: non-collective, non-contiguous */
IOStatus iio_pfwrite_level2(const File* file, Pattern* pattern)
{
	g_assert(file);
	g_assert(file->type == FILE_MPI);

	MPI_File fh = file->handle.mpifh;
	MPI_Status status;
	gint count;
	gchar* buffer;

	if ((buffer = g_malloc0(pattern->iter * pattern->elem * pattern->type_size)) == NULL) {
		Warning("PFWrite: Couldn't allocate %ld bytes of memory!\n",
				(pattern->iter * pattern->elem * pattern->type_size));
		return iostatus_new(FALSE, 0, 0);
	}

	MPI_ASSERT(MPI_File_set_view(fh, 0, MPI_BYTE, pattern->datatype, "native", info), "PFWrite Level2", FALSE)
	MPI_ASSERT(MPI_File_seek(fh, 0, MPI_SEEK_SET), "PFWrite Level2", FALSE)

	// write data to file
	CORETIME_START();
	MPI_File_write(fh, buffer, pattern->iter * pattern->elem, MPI_BYTE, &status);
	CORETIME_STOP(time);

	MPI_Get_count(&status, MPI_BYTE, &count);
	g_free(buffer);

	if (count == (pattern->iter * pattern->elem)) {
		Verbose("(PFWrite Level2) Wrote %d bytes", count);
		return iostatus_new(TRUE, time, count);
	}
	else {
		Warning("PFWrite Level2: Error during write! (%d of %d)\n", count, (pattern->iter * pattern->elem));
		return iostatus_new(FALSE, time, count);
	}
}

/* Level 3: collective, non-contiguous */
IOStatus iio_pfwrite_level3(const File* file, Pattern* pattern)
{
	g_assert(file);
	g_assert(file->type == FILE_MPI);

	MPI_File fh = file->handle.mpifh;
	MPI_Status status;
	gint count;
	gchar* buffer;

	if ((buffer = g_malloc0(pattern->iter * pattern->elem * pattern->type_size)) == NULL) {
		Warning("PFWrite: Couldn't allocate %ld bytes of memory!\n",
				(pattern->iter * pattern->elem * pattern->type_size));
		return iostatus_new(FALSE, 0, 0);
	}

	MPI_ASSERT(MPI_File_set_view(fh, 0, MPI_BYTE, pattern->datatype, "native", info), "PFWrite Level3", FALSE)
	MPI_ASSERT(MPI_File_seek(fh, 0, MPI_SEEK_SET), "PFWrite Level3", FALSE)

	// write data to file
	CORETIME_START();
	MPI_File_write_all(fh, buffer, pattern->iter * pattern->elem, MPI_BYTE, &status);
	CORETIME_STOP(time);

	MPI_Get_count(&status, MPI_BYTE, &count);
	g_free(buffer);

	if (count == (pattern->iter * pattern->elem)) {
		Verbose("(PFWrite Level3) Wrote %d bytes", count);
		return iostatus_new(TRUE, time, count);
	}
	else {
		Warning("PFWrite Level3: Error during write! (%d of %d)\n", count, (pattern->iter * pattern->elem));
		return iostatus_new(FALSE, time, count);
	}
}

/* Level 0: non-collective, contiguous */
IOStatus iio_pfread_level0(const File* file, Pattern* pattern)
{
	g_assert(file);
	g_assert(file->type == FILE_MPI);

	MPI_File fh = file->handle.mpifh;
	MPI_Status status;
	gint i, count = 0;
	gchar* buffer;

	if ((buffer = g_malloc0(pattern->iter * pattern->elem * pattern->type_size)) == NULL) {
		Warning("PFRead: Couldn't allocate %ld bytes of memory!\n",
				(pattern->iter * pattern->elem * pattern->type_size));
		return iostatus_new(FALSE, 0, 0);
	}

	MPI_ASSERT(MPI_File_set_view(fh, 0, MPI_BYTE, pattern->datatype, "native", info), "PFRead Level0", FALSE)
	MPI_ASSERT(MPI_File_seek(fh, 0, MPI_SEEK_SET), "PFRead Level0", FALSE)

	// write data to file
	CORETIME_START();
	for (i = 0; i < pattern->iter; ++i) {
		gint iterCount;
		MPI_File_read(fh, buffer, pattern->elem, MPI_BYTE, &status);

		MPI_Get_count(&status, MPI_BYTE, &iterCount);
		count += iterCount;
	}
	CORETIME_STOP(time);

	g_free(buffer);

	if (count == (pattern->iter * pattern->elem)) {
		Verbose("(PFRead Level0) Read %d bytes", count);
		return iostatus_new(TRUE, time, count);
	}
	else {
		Warning("PFRead Level0: Error during read! (%d of %d)\n", count, (pattern->iter * pattern->elem));
		return iostatus_new(FALSE, time, count);
	}
}

/* Level 1: collective, contiguous */
IOStatus iio_pfread_level1(const File* file, Pattern* pattern)
{
	g_assert(file);
	g_assert(file->type == FILE_MPI);

	MPI_File fh = file->handle.mpifh;
	MPI_Status status;
	gint i, count = 0;
	gchar* buffer;

	if ((buffer = g_malloc0(pattern->iter * pattern->elem * pattern->type_size)) == NULL) {
		Warning("PFRead: Couldn't allocate %ld bytes of memory!\n",
				(pattern->iter * pattern->elem * pattern->type_size));
		return iostatus_new(FALSE, 0, 0);
	}

	MPI_ASSERT(MPI_File_set_view(fh, 0, MPI_BYTE, pattern->datatype, "native", info), "PFRead Level1", FALSE)
	MPI_ASSERT(MPI_File_seek(fh, 0, MPI_SEEK_SET), "PFRead Level1", FALSE)

	// write data to file
	CORETIME_START();
	for (i = 0; i < pattern->iter; ++i) {
		gint iterCount;
		MPI_File_read_all(fh, buffer, pattern->elem, MPI_BYTE, &status);

		MPI_Get_count(&status, MPI_BYTE, &iterCount);
		count += iterCount;
	}
	CORETIME_STOP(time);

	g_free(buffer);

	if (count == (pattern->iter * pattern->elem)) {
		Verbose("(PFRead Level1) Read %d bytes", count);
		return iostatus_new(TRUE, time, count);
	}
	else {
		Warning("PFRead Level1: Error during read! (%d of %d)\n", count, (pattern->iter * pattern->elem));
		return iostatus_new(FALSE, time, count);
	}
}

/* Level 2: non-collective, non-contiguous */
IOStatus iio_pfread_level2(const File* file, Pattern* pattern)
{
	g_assert(file);
	g_assert(file->type == FILE_MPI);

	MPI_File fh = file->handle.mpifh;
	MPI_Status status;
	gint count;
	gchar* buffer;

	if ((buffer = g_malloc0(pattern->iter * pattern->elem * pattern->type_size)) == NULL) {
		Warning("PFRead: Couldn't allocate %ld bytes of memory!\n",
				(pattern->iter * pattern->elem * pattern->type_size));
		return iostatus_new(FALSE, 0, 0);
	}

	MPI_ASSERT(MPI_File_set_view(fh, 0, MPI_BYTE, pattern->datatype, "native", info), "PFRead Level2", FALSE)
	MPI_ASSERT(MPI_File_seek(fh, 0, MPI_SEEK_SET), "PFRead Level2", FALSE)

	// write data to file
	CORETIME_START();
	MPI_File_read(fh, buffer, pattern->iter * pattern->elem, MPI_BYTE, &status);
	CORETIME_STOP(time);

	MPI_Get_count(&status, MPI_BYTE, &count);
	g_free(buffer);

	if (count == (pattern->iter * pattern->elem)) {
		Verbose("(PFRead Level2) Read %d bytes", count);
		return iostatus_new(TRUE, time, count);
	}
	else {
		Warning("PFRead Level2: Error during read! (%d of %d)\n", count, (pattern->iter * pattern->elem));
		return iostatus_new(FALSE, time, count);
	}
}

/* Level 3: collective, non-contiguous */
IOStatus iio_pfread_level3(const File* file, Pattern* pattern)
{
	g_assert(file);
	g_assert(file->type == FILE_MPI);

	MPI_File fh = file->handle.mpifh;
	MPI_Status status;
	gint count;
	gchar* buffer;

	if ((buffer = g_malloc0(pattern->iter * pattern->elem * pattern->type_size)) == NULL) {
		Warning("PFRead: Couldn't allocate %ld bytes of memory!\n",
				(pattern->iter * pattern->elem * pattern->type_size));
		return iostatus_new(FALSE, 0, 0);
	}

	MPI_ASSERT(MPI_File_set_view(fh, 0, MPI_BYTE, pattern->datatype, "native", info), "PFRead Level3", FALSE)
	MPI_ASSERT(MPI_File_seek(fh, 0, MPI_SEEK_SET), "PFRead Level3", FALSE)

	// write data to file
	CORETIME_START();
	MPI_File_read_all(fh, buffer, pattern->iter * pattern->elem, MPI_BYTE, &status);
	CORETIME_STOP(time);

	MPI_Get_count(&status, MPI_BYTE, &count);
	g_free(buffer);

	if (count == (pattern->iter * pattern->elem)) {
		Verbose("(PFRead Level3) Read %d bytes", count);
		return iostatus_new(TRUE, time, count);
	}
	else {
		Warning("PFRead Level3: Error during read! (%d of %d)\n", count, (pattern->iter * pattern->elem));
		return iostatus_new(FALSE, time, count);
	}
}


/* Level 0: non-collective, contiguous */
IOStatus iio_pwrite_level0(const gchar* path, Pattern* pattern, MPI_Comm comm) {
	MPI_File fh;
	MPI_Status status;
	gint mode = MPI_MODE_WRONLY | MPI_MODE_CREATE;
	gint i, count = 0;
	gchar* buffer;

	if ((buffer = g_malloc0(pattern->iter * pattern->elem * pattern->type_size)) == NULL) {
		Warning("PWrite: Couldn't allocate %ld bytes of memory!\n",
				(pattern->iter * pattern->elem * pattern->type_size));
		return iostatus_new(FALSE, 0, 0);
	}

	MPI_ASSERT(MPI_File_open(comm, (gchar*)  path, mode, info, &fh), "PWrite Level0", FALSE)
	MPI_ASSERT(MPI_File_set_view(fh, 0, MPI_BYTE, pattern->datatype, "native", info), "PWrite Level0", FALSE)
	MPI_ASSERT(MPI_File_seek(fh, 0, MPI_SEEK_SET), "PWrite Level0", FALSE)

	// write data to file
	CORETIME_START();
	for (i = 0; i < pattern->iter; ++i) {
		gint iterCount;
		MPI_File_write(fh, buffer, pattern->elem, MPI_BYTE, &status);

		MPI_Get_count(&status, MPI_BYTE, &iterCount);
		count += iterCount;
	}
	CORETIME_STOP(time);

	MPI_ASSERT(MPI_File_close(&fh), "PWrite Level0", FALSE)
	g_free(buffer);

	if (count == (pattern->iter * pattern->elem)) {
		Verbose("(PWrite Level0) Wrote %d bytes", count);
		return iostatus_new(TRUE, time, count);
	}
	else {
		Warning("PWrite Level0: Error during write to file %s! (%d of %d)\n", path, count, (pattern->iter * pattern->elem));
		return iostatus_new(FALSE, time, count);
	}
}

/* Level 1: collective, contiguous */
IOStatus iio_pwrite_level1(const gchar* path, Pattern* pattern, MPI_Comm comm) {
	MPI_File fh;
	MPI_Status status;
	gint mode = MPI_MODE_WRONLY | MPI_MODE_CREATE;
	gint i, count = 0;
	gchar* buffer;

	if ((buffer = g_malloc0(pattern->iter * pattern->elem * pattern->type_size)) == NULL) {
		Warning("PWrite: Couldn't allocate %ld bytes of memory!\n",
				(pattern->iter * pattern->elem * pattern->type_size));
		return iostatus_new(FALSE, 0, 0);
	}

	MPI_ASSERT(MPI_File_open(comm, (gchar*)  path, mode, info, &fh), "PWrite Level1", FALSE)
	MPI_ASSERT(MPI_File_set_view(fh, 0, MPI_BYTE, pattern->datatype, "native", info), "PWrite Level1", FALSE)
	MPI_ASSERT(MPI_File_seek(fh, 0, MPI_SEEK_SET), "PWrite Level1", FALSE)

	// write data to file
	CORETIME_START();
	for (i = 0; i < pattern->iter; ++i) {
		gint iterCount;
		MPI_File_write_all(fh, buffer, pattern->elem, MPI_BYTE, &status);

		MPI_Get_count(&status, MPI_BYTE, &iterCount);
		count += iterCount;
	}
	CORETIME_STOP(time);

	MPI_ASSERT(MPI_File_close(&fh), "PWrite Level1", FALSE)
	g_free(buffer);

	if (count == (pattern->iter * pattern->elem)) {
		Verbose("(PWrite Level1) Wrote %d bytes", count);
		return iostatus_new(TRUE, time, count);
	}
	else {
		Warning("PWrite Level1: Error during write to file %s! (%d of %d)\n", path, count, (pattern->iter * pattern->elem));
		return iostatus_new(FALSE, time, count);
	}
}

/* Level 2: non-collective, non-contiguous */
IOStatus iio_pwrite_level2(const gchar* path, Pattern* pattern, MPI_Comm comm) {
	MPI_File fh;
	MPI_Status status;
	gint mode = MPI_MODE_WRONLY | MPI_MODE_CREATE;
	gint count;
	gchar* buffer;

	if ((buffer = g_malloc0(pattern->iter * pattern->elem * pattern->type_size)) == NULL) {
		Warning("PWrite: Couldn't allocate %ld bytes of memory!\n",
				(pattern->iter * pattern->elem * pattern->type_size));
		return iostatus_new(FALSE, 0, 0);
	}

	MPI_ASSERT(MPI_File_open(comm, (gchar*)  path, mode, info, &fh), "PWrite Level2", FALSE)
	MPI_ASSERT(MPI_File_set_view(fh, 0, MPI_BYTE, pattern->datatype, "native", info), "PWrite Level2", FALSE)
	MPI_ASSERT(MPI_File_seek(fh, 0, MPI_SEEK_SET), "PWrite Level2", FALSE)

	// write data to file
	CORETIME_START();
	MPI_File_write(fh, buffer, pattern->iter * pattern->elem, MPI_BYTE, &status);
	CORETIME_STOP(time);

	MPI_Get_count(&status, MPI_BYTE, &count);

	MPI_ASSERT(MPI_File_close(&fh), "PWrite Level2", FALSE)
	g_free(buffer);

	if (count == (pattern->iter * pattern->elem)) {
		Verbose("(PWrite Level2) Wrote %d bytes", count);
		return iostatus_new(TRUE, time, count);
	}
	else {
		Warning("PWrite Level2: Error during write to file %s! (%d of %d)\n", path, count, (pattern->iter * pattern->elem));
		return iostatus_new(FALSE, time, count);
	}
}

/* Level 3: collective, non-contiguous */
IOStatus iio_pwrite_level3(const gchar* path, Pattern* pattern, MPI_Comm comm) {
	MPI_File fh;
	MPI_Status status;
	gint mode = MPI_MODE_WRONLY | MPI_MODE_CREATE;
	gint count;
	gchar* buffer;

	if ((buffer = g_malloc0(pattern->iter * pattern->elem * pattern->type_size)) == NULL) {
		Warning("PWrite: Couldn't allocate %ld bytes of memory!\n",
				(pattern->iter * pattern->elem * pattern->type_size));
		return iostatus_new(FALSE, 0, 0);
	}

	MPI_ASSERT(MPI_File_open(comm, (gchar*)  path, mode, info, &fh), "PWrite Level3", FALSE)
	MPI_ASSERT(MPI_File_set_view(fh, 0, MPI_BYTE, pattern->datatype, "native", info), "PWrite Level3", FALSE)
	MPI_ASSERT(MPI_File_seek(fh, 0, MPI_SEEK_SET), "PWrite Level3", FALSE)

	// write data to file
	CORETIME_START();
	MPI_File_write_all(fh, buffer, pattern->iter * pattern->elem, MPI_BYTE, &status);
	CORETIME_STOP(time);

	MPI_Get_count(&status, MPI_BYTE, &count);

	MPI_ASSERT(MPI_File_close(&fh), "PWrite Level3", FALSE)
	g_free(buffer);

	if (count == (pattern->iter * pattern->elem)) {
		Verbose("(PWrite Level3) Wrote %d bytes", count);
		return iostatus_new(TRUE, time, count);
	}
	else {
		Warning("PWrite Level3: Error during write to file %s! (%d of %d)\n", path, count, (pattern->iter * pattern->elem));
		return iostatus_new(FALSE, time, count);
	}
}

/* Level 0: non-collective, contiguous */
IOStatus iio_pread_level0(const gchar* path, Pattern* pattern, MPI_Comm comm) {
	MPI_File fh;
	MPI_Status status;
	gint mode = MPI_MODE_RDONLY;
	gint i, count = 0;
	gchar* buffer;

	if ((buffer = g_malloc0(pattern->iter * pattern->elem * pattern->type_size)) == NULL) {
		Warning("PRead: Couldn't allocate %ld bytes of memory!\n",
				(pattern->iter * pattern->elem * pattern->type_size));
		return iostatus_new(FALSE, 0, 0);
	}

	MPI_ASSERT(MPI_File_open(comm, (gchar*)  path, mode, info, &fh), "PRead Level0", FALSE)
	MPI_ASSERT(MPI_File_set_view(fh, 0, MPI_BYTE, pattern->datatype, "native", info), "PRead Level0", FALSE)
	MPI_ASSERT(MPI_File_seek(fh, 0, MPI_SEEK_SET), "PRead Level0", FALSE)

	// read data from file
	CORETIME_START();
	for (i = 0; i < pattern->iter; ++i) {
		gint iterCount;
		MPI_File_read(fh, buffer, pattern->elem, MPI_BYTE, &status);

		MPI_Get_count(&status, MPI_BYTE, &iterCount);
		count += iterCount;
	}
	CORETIME_STOP(time);

	MPI_ASSERT(MPI_File_close(&fh), "PRead Level0", FALSE)
	g_free(buffer);

	if (count == (pattern->iter * pattern->elem)) {
		Verbose("(PRead Level0) Read %d bytes", count);
		return iostatus_new(TRUE, time, count);
	}
	else {
		Warning("PRead Level0: Error during read from file %s! (%d of %d)\n", path, count, (pattern->iter * pattern->elem));
		return iostatus_new(FALSE, time, count);
	}
}

/* Level 1: collective, contiguous */
IOStatus iio_pread_level1(const gchar* path, Pattern* pattern, MPI_Comm comm) {
	MPI_File fh;
	MPI_Status status;
	gint mode = MPI_MODE_RDONLY;
	gint i, count = 0;
	gchar* buffer;

	if ((buffer = g_malloc0(pattern->iter * pattern->elem * pattern->type_size)) == NULL) {
		Warning("PRead: Couldn't allocate %ld bytes of memory!\n",
				(pattern->iter * pattern->elem * pattern->type_size));
		return iostatus_new(FALSE, 0, 0);
	}

	MPI_ASSERT(MPI_File_open(comm, (gchar*)  path, mode, info, &fh), "PRead Level1", FALSE)
	MPI_ASSERT(MPI_File_set_view(fh, 0, MPI_BYTE, pattern->datatype, "native", info), "PRead Level1", FALSE)
	MPI_ASSERT(MPI_File_seek(fh, 0, MPI_SEEK_SET), "PRead Level1", FALSE)

	// read data from file
	CORETIME_START();
	for (i = 0; i < pattern->iter; ++i) {
		gint iterCount;
		MPI_File_read_all(fh, buffer, pattern->elem, MPI_BYTE, &status);

		MPI_Get_count(&status, MPI_BYTE, &iterCount);
		count += iterCount;
	}
	CORETIME_STOP(time);

	MPI_ASSERT(MPI_File_close(&fh), "PRead Level1", FALSE)
	g_free(buffer);

	if (count == (pattern->iter * pattern->elem)) {
		Verbose("(PRead Level1) Read %d bytes", count);
		return iostatus_new(TRUE, time, count);
	}
	else {
		Warning("PRead Level1: Error during read from file %s! (%d of %d)\n", path, count, (pattern->iter * pattern->elem));
		return iostatus_new(FALSE, time, count);
	}
}

/* Level 2: non-collective, non-contiguous */
IOStatus iio_pread_level2(const gchar* path, Pattern* pattern, MPI_Comm comm) {
	MPI_File fh;
	MPI_Status status;
	gint mode = MPI_MODE_RDONLY;
	gint count;
	gchar* buffer;

	if ((buffer = g_malloc0(pattern->iter * pattern->elem * pattern->type_size)) == NULL) {
		Warning("PRead: Couldn't allocate %ld bytes of memory!\n",
				(pattern->iter * pattern->elem * pattern->type_size));
		return iostatus_new(FALSE, 0, 0);
	}

	MPI_ASSERT(MPI_File_open(comm, (gchar*)  path, mode, info, &fh), "PRead Level2", FALSE)
	MPI_ASSERT(MPI_File_set_view(fh, 0, MPI_BYTE, pattern->datatype, "native", info), "PRead Level2", FALSE)
	MPI_ASSERT(MPI_File_seek(fh, 0, MPI_SEEK_SET), "PRead Level2", FALSE)

	// read data from file
	CORETIME_START();
	MPI_File_read(fh, buffer, pattern->iter * pattern->elem, MPI_BYTE, &status);
	CORETIME_STOP(time);

	MPI_Get_count(&status, MPI_BYTE, &count);

	MPI_ASSERT(MPI_File_close(&fh), "PRead Level2", FALSE)
	g_free(buffer);

	if (count == (pattern->iter * pattern->elem)) {
		Verbose("(PRead Level2) Read %d bytes", count);
		return iostatus_new(TRUE, time, count);
	}
	else {
		Warning("PRead Level2: Error during read from file %s! (%d of %d)\n", path, count, (pattern->iter * pattern->elem));
		return iostatus_new(FALSE, time, count);
	}
}

/* Level 3: collective, non-contiguous */
IOStatus iio_pread_level3(const gchar* path, Pattern* pattern, MPI_Comm comm) {
	MPI_File fh;
	MPI_Status status;
	gint mode = MPI_MODE_RDONLY;
	gint count;
	gchar* buffer;

	if ((buffer = g_malloc0(pattern->iter * pattern->elem * pattern->type_size)) == NULL) {
		Warning("PRead: Couldn't allocate %ld bytes of memory!\n",
				(pattern->iter * pattern->elem * pattern->type_size));
		return iostatus_new(FALSE, 0, 0);
	}

	MPI_ASSERT(MPI_File_open(comm, (gchar*)  path, mode, info, &fh), "PRead Level3", FALSE)
	MPI_ASSERT(MPI_File_set_view(fh, 0, MPI_BYTE, pattern->datatype, "native", info), "PRead Level3", FALSE)
	MPI_ASSERT(MPI_File_seek(fh, 0, MPI_SEEK_SET), "PRead Level3", FALSE)

	// read data from file
	CORETIME_START();
	MPI_File_read_all(fh, buffer, pattern->iter * pattern->elem, MPI_BYTE, &status);
	CORETIME_STOP(time);

	MPI_Get_count(&status, MPI_BYTE, &count);

	MPI_ASSERT(MPI_File_close(&fh), "PRead Level3", FALSE)
	g_free(buffer);

	if (count == (pattern->iter * pattern->elem)) {
		Verbose("(PRead Level3) Read %d bytes", count);
		return iostatus_new(TRUE, time, count);
	}
	else {
		Warning("PRead Level3: Error during read from file %s! (%d of %d)\n", path, count, (pattern->iter * pattern->elem));
		return iostatus_new(FALSE, time, count);
	}
}

gboolean iio_pdelete(const gchar* path)
{
	gint rc = -1;
	MPI_ASSERT(rc = MPI_File_delete((gchar*) path, MPI_INFO_NULL), "PDelete", FALSE)

	return (rc == MPI_SUCCESS);
}

#endif
