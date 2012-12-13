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


#include "../build/default/config.h"
#include "../iio.h"
#include "../iio_posix.h"

#include <glib.h>
#include <glib/gprintf.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>


gboolean parseOnly = FALSE;
gchar* sourceFileName = "";
//int yyparse() {}
//void yyset_in(FILE* file) {}


/**
 * Test suite helper functions
 */
glong get_file_size(const gchar* fname)
{
	FILE* fh = fopen(fname, "r");
	g_assert(fh);
	fseek(fh , 0 , SEEK_END);
	glong size = ftell(fh);
	rewind(fh);
	fclose(fh);
	return size;
}

void create_file(const gchar* fname)
{
	FILE* fh = fopen(fname, "w+");
	g_assert(fh);
	fclose(fh);
}

void delete_file(const gchar* fname)
{
	g_assert(g_remove(fname) == 0);
}

gboolean file_exists(const gchar* fname)
{
	return (g_access(fname, F_OK) == 0);
}

void make_dir(const gchar* dname)
{
	g_assert(g_mkdir_with_parents(dname, 0700) == 0);
}


/**
 * Test cases
 */
void test_io_fopenclose()
{
	GString* fname = g_string_new("testfile_");
	g_string_append_printf(fname, "%d", ABS(g_test_rand_int()));

	create_file(fname->str);

	g_message("File \"%s\" must exist to pass the test!", fname->str);

	File* fh;
	gboolean rc = iio_fopen(fname->str, O_RDONLY, &fh);
	g_assert(fh);
	g_assert(fh->handle.stdfh);
	g_assert(rc);

	rc = iio_fclose(fh);
	g_assert(rc);

	delete_file(fname->str);
}

void test_io_fread_all()
{
	GString* fname = g_string_new("test_fread_sequential_");
	g_string_append_printf(fname, "%d", ABS(g_test_rand_int()));
	gint32 chunkSize = g_test_rand_int_range(32, 1024*1024);
	gint32 offset    = 0;
	const gint FILESIZE = 10*1024*1024;

	g_assert(iio_write(fname->str, FILESIZE, 0).success);


	/* sequential whole file */
	g_message("Reading %d chunks sequential of %d bytes from offset %d of file \"%s\"",
			(FILESIZE/chunkSize), chunkSize, offset, fname->str);
	File* fh;
	g_assert(iio_fopen(fname->str, O_RDONLY, &fh));
	g_assert(fh);
	g_assert(fh->handle.stdfh);

	gint i;
	glong dataRead = 0;
	for (i = 0; i < (FILESIZE/chunkSize)-1; i++) {
		IOStatus status = iio_fread(fh, chunkSize, offset);
		g_assert(status.success);
		dataRead += status.coreTime.data;
	}
	g_assert_cmpint(dataRead, ==, ((FILESIZE/chunkSize)-1)*chunkSize);
	g_assert(iio_fclose(fh));

	/* whole file with one chunk */
	g_message("Reading whole file from offset %d of file \"%s\"", offset, fname->str);
	g_assert(iio_fopen(fname->str, O_RDONLY, &fh));
	g_assert(fh);
	g_assert(fh->handle.stdfh);

	IOStatus status = iio_fread(fh, READALL, offset);
	g_assert(status.success);
	g_assert_cmpint(status.coreTime.data, ==, FILESIZE);
	g_assert(iio_fclose(fh));


	delete_file(fname->str);
	g_string_free(fname, TRUE);
}

void test_io_fread_sequential()
{
	GString* fname = g_string_new("test_fread_sequential_");
	g_string_append_printf(fname, "%d", ABS(g_test_rand_int()));
	gint32 chunkSize = g_test_rand_int_range(32, 1024*1024);
	gint32 offset   = OFFSET_CUR;
	const gint FILESIZE = 10*1024*1024;

	g_assert(iio_write(fname->str, FILESIZE, 0).success);

	g_message("Reading %d bytes from offset %d of file \"%s\"", chunkSize, offset, fname->str);


	File* fh;
	g_assert(iio_fopen(fname->str, O_RDONLY, &fh));
	g_assert(fh);
	g_assert(fh->handle.stdfh);

	gint i;
	glong dataRead = 0;
	for (i = 0; i < (FILESIZE/chunkSize)-1; i++) {
		IOStatus status = iio_fread(fh, chunkSize, offset);
		g_assert(status.success);
		dataRead += status.coreTime.data;
	}

	g_assert_cmpint(dataRead, ==, ((FILESIZE/chunkSize)-1)*chunkSize);

	g_assert(iio_fclose(fh));

	delete_file(fname->str);
	g_string_free(fname, TRUE);
}

void test_io_fread_random()
{
	const gint FILESIZE = 10*1024*1024;
	GString* fname = g_string_new("test_fread_random_");
	g_string_append_printf(fname, "%d", ABS(g_test_rand_int()));
	gint32 chunkSize = g_test_rand_int_range(32, FILESIZE/2);
	gint32 offset    = g_test_rand_int_range(0, FILESIZE/2);

	g_assert(iio_write(fname->str, FILESIZE, 0).success);

	g_message("Reading %d bytes from offset %d of file \"%s\"", chunkSize, offset, fname->str);


	File* fh;
	g_assert(iio_fopen(fname->str, O_RDONLY, &fh));
	g_assert(fh);
	g_assert(fh->handle.stdfh);

	IOStatus status = iio_fread(fh, chunkSize, offset);
	g_assert(status.success);

	g_assert_cmpint(status.coreTime.data, ==, chunkSize);

	g_assert(iio_fclose(fh));

	delete_file(fname->str);
	g_string_free(fname, TRUE);
}

void test_io_fwrite_sequential()
{
	const gint FILESIZE = 10*1024*1024;
	GString* fname = g_string_new("test_fwrite_sequential_");
	g_string_append_printf(fname, "%d", ABS(g_test_rand_int()));
	gint32 chunkSize = g_test_rand_int_range(32, FILESIZE/10);
	gint32 offset    = OFFSET_CUR;

	g_message("Writing %d * %d bytes to offset %d in file \"%s\"", (FILESIZE/chunkSize)-1, chunkSize, offset, fname->str);


	File* fh;
	g_assert(iio_fopen(fname->str, O_WRONLY, &fh));
	g_assert(fh);
	g_assert(fh->handle.stdfh);

	gint i;
	glong dataWrote = 0;
	for (i = 0; i < (FILESIZE/chunkSize)-1; i++) {
		IOStatus status = iio_fwrite(fh, chunkSize, offset);
		g_assert(status.success);
		dataWrote += status.coreTime.data;
	}

	g_assert_cmpint(dataWrote, ==, ((FILESIZE/chunkSize)-1)*chunkSize);
	glong fileSize = get_file_size(fname->str);
	g_assert_cmpint(((FILESIZE/chunkSize)-1)*chunkSize, ==, fileSize);

	g_assert(iio_fclose(fh));

	delete_file(fname->str);
	g_string_free(fname, TRUE);
}

void test_io_fwrite_random()
{
	const gint FILESIZE = 10*1024*1024;
	GString* fname = g_string_new("test_fwrite_random_");
	g_string_append_printf(fname, "%d", ABS(g_test_rand_int()));
	gint32 chunkSize = g_test_rand_int_range(32, FILESIZE/2);
	gint32 offset   = g_test_rand_int_range(0, FILESIZE/2);

	g_message("Writing %d bytes to offset %d in file \"%s\"", chunkSize, offset, fname->str);


	File* fh;
	g_assert(iio_fopen(fname->str, O_WRONLY, &fh));
	g_assert(fh);
	g_assert(fh->handle.stdfh);

	IOStatus status = iio_fwrite(fh, chunkSize, offset);
	g_assert(status.success);

	g_assert_cmpint(status.coreTime.data, ==, chunkSize);
	glong fileSize = get_file_size(fname->str);
	g_assert_cmpint(offset+chunkSize, ==, fileSize);

	g_assert(iio_fclose(fh));

	delete_file(fname->str);
	g_string_free(fname, TRUE);
}

void test_io_read_random()
{
	GString* fname = g_string_new("test_read_random_");
	g_string_append_printf(fname, "%d", ABS(g_test_rand_int()));
	gint32 dataSize = g_test_rand_int_range(1024, 1024*1024);
	gint32 offset   = g_test_rand_int_range(1, 1024);

	g_assert(iio_write(fname->str, 2*1024*1024, 0).success);

	g_message("Reading %d bytes from offset %d of file \"%s\"", dataSize, offset, fname->str);

	IOStatus status = iio_read(fname->str, dataSize, offset);
	g_assert(status.success);

	g_assert_cmpint(dataSize, ==, status.coreTime.data);

	delete_file(fname->str);
	g_string_free(fname, TRUE);
}

void test_io_read_all()
{
	GString* fname = g_string_new("test_read_all_");
	g_string_append_printf(fname, "%d", ABS(g_test_rand_int()));
	gint32 dataSize = READALL;
	gint32 offset   = 0;
	const gint SIZE = 5*1024*1024;

	g_assert(iio_write(fname->str, SIZE, 0).success);

	g_message("Reading whole file \"%s\"", fname->str);

	IOStatus status = iio_read(fname->str, dataSize, offset);
	g_assert(status.success);

	g_assert_cmpint(SIZE, ==, status.coreTime.data);

	delete_file(fname->str);
	g_string_free(fname, TRUE);
}

void test_io_write_random()
{
	GString* fname = g_string_new("test_write_random_");
	g_string_append_printf(fname, "%d", ABS(g_test_rand_int()));
	gint32 dataSize = g_test_rand_int_range(1024, 10*1024*1024);
	gint32 offset   = g_test_rand_int_range(1, 1024);

	g_message("Writing %d bytes to offset %d of file \"%s\"", dataSize, offset, fname->str);

	IOStatus status = iio_write(fname->str, dataSize, offset);
	g_assert(status.success);

	glong fileSize = get_file_size(fname->str);
	g_assert_cmpint(offset+dataSize, ==, fileSize);

	g_assert(g_remove(fname->str) == 0);
	g_string_free(fname, TRUE);
}

void test_io_write_offset0()
{
	GString* fname = g_string_new("test_write_offset0_");
	g_string_append_printf(fname, "%d", g_test_rand_int());
	gint32 dataSize = g_test_rand_int_range(1024, 10*1024*1024);
	gint32 offset   = 0;

	g_message("Writing %d bytes to offset %d of file \"%s\"", dataSize, offset, fname->str);

	IOStatus status = iio_write(fname->str, dataSize, offset);
	g_assert(status.success);

	glong fileSize = get_file_size(fname->str);
	g_assert_cmpint(offset+dataSize, ==, fileSize);

	g_assert(g_remove(fname->str) == 0);
	g_string_free(fname, TRUE);
}

void test_io_append()
{
	GString* fname = g_string_new("test_append_");
	g_string_append_printf(fname, "%d", g_test_rand_int());
	gint32 dataSize = g_test_rand_int_range(1024, 1024*1024);
	const gint reps = 10;

	g_message("Appending %d bytes to file \"%s\" %d times.", dataSize, fname->str, reps);

	gint i;
	for (i=0; i<reps; i++) {
		g_assert(iio_append(fname->str, dataSize).success);
	}

	glong fileSize = get_file_size(fname->str);
	g_assert_cmpint(reps*dataSize, ==, fileSize);

	delete_file(fname->str);
	g_string_free(fname, TRUE);
}

void test_io_lookup()
{
	GString* fname = g_string_new("testfile_");
	g_string_append_printf(fname, "%d", ABS(g_test_rand_int()));

	create_file(fname->str);

	g_message("File \"%s\" must exist to pass the test!", fname->str);
	g_message("File \"doesntexist_43516875646\" must not exist to pass the test!");

	g_assert(iio_lookup(fname->str).success);
	g_assert(!iio_lookup("doesntexist_43516875646").success);

	delete_file(fname->str);
}

void test_io_delete()
{
	GString* fname = g_string_new("test_delete_");
	g_string_append_printf(fname, "%d", g_test_rand_int());

	create_file(fname->str);

	g_assert(file_exists(fname->str));
	g_assert(iio_delete(fname->str));
	g_assert(!file_exists(fname->str));

	g_string_free(fname, TRUE);
}

void test_io_mkdir()
{
	GString* fname = g_string_new("test_mkdir_");
	g_string_append_printf(fname, "%d", g_test_rand_int());

	g_assert(!file_exists(fname->str));
	g_assert(iio_mkdir(fname->str));
	g_assert(file_exists(fname->str));

	delete_file(fname->str);

	g_string_free(fname, TRUE);
}

void test_io_rmdir()
{
	GString* fname = g_string_new("test_rmdir_");
	g_string_append_printf(fname, "%d", g_test_rand_int());

	make_dir(fname->str);

	g_assert(file_exists(fname->str));
	g_assert(iio_rmdir(fname->str));
	g_assert(!file_exists(fname->str));

	g_string_free(fname, TRUE);
}

void test_io_create()
{
	GString* fname = g_string_new("test_create_");
	g_string_append_printf(fname, "%d", g_test_rand_int());

	g_assert(!file_exists(fname->str));
	g_assert(iio_create(fname->str));
	g_assert(file_exists(fname->str));

	delete_file(fname->str);

	g_string_free(fname, TRUE);
}

void test_io_stat()
{
	GString* fname = g_string_new("test_stat_");
	g_string_append_printf(fname, "%d", g_test_rand_int());

	create_file(fname->str);

	g_assert(iio_stat(fname->str));
	g_assert(!iio_stat("doesntexist"));

	delete_file(fname->str);

	g_string_free(fname, TRUE);
}

void test_io_rename()
{
	GString* oldname = g_string_new("test_rename_");
	GString* newname = g_string_new("test_rename_");
	g_string_append_printf(oldname, "%d", g_test_rand_int());
	g_string_append_printf(newname, "%d", g_test_rand_int());

	create_file(oldname->str);

	g_assert(iio_rename(oldname->str, newname->str));
	g_assert(!file_exists(oldname->str));
	g_assert(file_exists(newname->str));

	g_assert(!iio_rename("doesntexist", "something"));

	delete_file(newname->str);

	g_string_free(oldname, TRUE);
	g_string_free(newname, TRUE);
}

int main(int argc, char *argv[])
{
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/POSIX IO/File open close", test_io_fopenclose);
	g_test_add_func("/POSIX IO/Read whole file (handle)", test_io_fread_all);
	g_test_add_func("/POSIX IO/Sequential read (handle)", test_io_fread_sequential);
	g_test_add_func("/POSIX IO/Random read (handle)", test_io_fread_random);
	g_test_add_func("/POSIX IO/Sequential write (handle)", test_io_fwrite_sequential);
	g_test_add_func("/POSIX IO/Random write (handle)", test_io_fwrite_random);
	g_test_add_func("/POSIX IO/Random read", test_io_read_random);
	g_test_add_func("/POSIX IO/Read whole file", test_io_read_all);
	g_test_add_func("/POSIX IO/Random write", test_io_write_random);
	g_test_add_func("/POSIX IO/Write to offset 0", test_io_write_offset0);
	g_test_add_func("/POSIX IO/Append", test_io_append);
	g_test_add_func("/POSIX IO/Lookup", test_io_lookup);
	g_test_add_func("/POSIX IO/Delete", test_io_delete);
	g_test_add_func("/POSIX IO/Mkdir", test_io_mkdir);
	g_test_add_func("/POSIX IO/Rmdir", test_io_rmdir);
	g_test_add_func("/POSIX IO/Create", test_io_create);
	g_test_add_func("/POSIX IO/Stat", test_io_stat);
	g_test_add_func("/POSIX IO/Rename", test_io_rename);

	return g_test_run();
}
