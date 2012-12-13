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
#include "iio_posix.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/**
 * Opens a file and creates it if doesn't exist.
 */
IOStatus iio_fcreat(const gchar* filename, File** file)
{
	int fd;
	CORETIME_START();
	fd = creat(filename, DEFAULT_OPEN_MODE);
	CORETIME_STOP(time);

	if (fd != -1) {
		*file = file_new(FILE_POSIX, &fd);
		return iostatus_new(TRUE, time, 0);
	}
	else {
		Warning("(FCreat) Couldn't open file \"%s\"", filename);
		return iostatus_new(FALSE, time, 0);
	}
}

IOStatus iio_fopen(const gchar* filename, const gint flags, File** file)
{
	int fd;
	CORETIME_START();
	fd = open(filename, flags, DEFAULT_OPEN_MODE);
	CORETIME_STOP(time);

	if (fd != -1) {
		*file = file_new(FILE_POSIX, &fd);
		return iostatus_new(TRUE, time, 0);
	}
	else {
		Warning("(FOpen) Couldn't open file \"%s\" with flags %d", filename, flags);
		return iostatus_new(FALSE, time, 0);
	}
}

IOStatus iio_fclose(File* file)
{
	g_assert(file);
	g_assert(file->type == FILE_POSIX);

	gint ret;
	CORETIME_START();
	ret = close(file->handle.posixfh);
	CORETIME_STOP(time);

	if (ret == 0)
		return iostatus_new(TRUE, time, 0);
	else {
		Warning("(FClose) Couldn't close handle %d", file->handle.posixfh);
		return iostatus_new(FALSE, time, 0);
	}
}

IOStatus iio_fwrite(const File* file, glong amount, off_t offset)
{
	g_assert(file);
	g_assert(file->type == FILE_POSIX);

	int fd = file->handle.posixfh;

	gchar* buffer;
	glong rSize;

	// set file pointer to offset
	if ((offset >= 0) && (lseek(fd, offset, SEEK_SET) != offset)) {
		Warning("(FWrite) Setting file pointer to offset %ld failed!", offset);
		return iostatus_new(FALSE, 0, 0);
	}
	else if (offset != OFFSET_CUR)
		Verbose("(FWrite) File pointer set to offset %ld", offset);

	// allocate memory for data to write
	if ((buffer = g_malloc0(sizeof(gchar)*amount))) {
		glong i;
		for (i=0; i<amount; i++)
			buffer[i] = '0';

		Verbose("(FWrite) Memory allocated for %ld bytes", amount);
	}
	else {
		Warning("(FWrite) Not enough memory available to allocate %ld bytes!", amount);
		return iostatus_new(FALSE, 0, 0);
	}

	// write the data to file
	CORETIME_START();
	if ((rSize = write(fd, buffer, sizeof(gchar)*amount)) < amount) {
		Warning("(FWrite) Error during write! (%ld of %ld)", rSize, amount);
	}
	CORETIME_STOP(time);

	Verbose("(FWrite) File pointer @ %ld", lseek(fd, 0, SEEK_CUR));

	g_free(buffer);

	if (rSize == amount)
		return iostatus_new(TRUE, time, rSize);
	else
		return iostatus_new(FALSE, time, rSize);
}

IOStatus iio_fread(const File* file, glong amount, off_t offset)
{
	g_assert(file);
	g_assert(file->type == FILE_POSIX);

	int fd = file->handle.posixfh;

	glong lSize, rSize;
	gchar* buffer;

	// if amount is set to READALL the whole file will be read.
	// else we read only amount bytes and leave the file pointer after
	// the end of the read block of data.
	if (amount == READALL) {
		lSize = (glong) lseek(fd, 0, SEEK_END);
		lseek(fd, 0, SEEK_SET);
	}
	else
		lSize = amount;

	// set file pointer to offset if offset >= 0
	if ((offset >= 0) && (lseek(fd, offset, SEEK_SET) != offset)) {
		Warning("(FRead) Setting file pointer to offset %ld failed!", offset);
		return iostatus_new(FALSE, 0, 0);
	}
	else if (offset != OFFSET_CUR)
		Verbose("(FRead) File pointer set to offset %ld", offset);

	// allocate memory
	if (!(buffer = g_malloc0(sizeof(gchar)*lSize))) {
		Warning("(FRead) Not enough memory available to allocate %ld bytes!", amount);
		return iostatus_new(FALSE, 0, 0);
	}

	// copy the data into memory
	CORETIME_START();
	if ((rSize = read(fd, buffer, sizeof(gchar)*lSize)) < lSize) {
		Warning("(FRead) Error during read! (%ld of %ld)", rSize, lSize);
	}
	CORETIME_STOP(time);

	Verbose("(FRead) File pointer @ %ld", lseek(fd, 0, SEEK_CUR));

	g_free(buffer);

	if (rSize == lSize)
		return iostatus_new(TRUE, time, rSize);
	else
		return iostatus_new(FALSE, time, rSize);
}

IOStatus iio_fseek(const File* file, off_t offset, gint whence)
{
	g_assert(file);
	g_assert(file->type == FILE_POSIX);

	int fd = file->handle.posixfh;
	off_t ret;

	Verbose("(FSeek) File pointer @ %ld", lseek(fd, 0, SEEK_CUR));

	// set file pointer to offset
	CORETIME_START();
	ret = lseek(fd, offset, whence);
	CORETIME_STOP(time);

	if (ret == -1) {
		Warning("(FSeek) Setting file pointer to offset %ld failed! (whence = %d)", offset, whence);
		return iostatus_new(FALSE, 0, 0);
	}
	else Verbose("(FSeek) File pointer set to offset %ld (whence = %d), now @ %ld", offset, whence, lseek(fd, 0, SEEK_CUR));

	return iostatus_new(TRUE, time, 0);
}

IOStatus iio_fsync(const File* file)
{
	g_assert(file);
	g_assert(file->type == FILE_POSIX);
	int fd = file->handle.posixfh;

	CORETIME_START();
	int ret = fsync(fd);
	CORETIME_STOP(time);

	if (ret == 0)
		return iostatus_new(TRUE, time, 0);
	else
		return iostatus_new(FALSE, time, 0);
}

IOStatus iio_fstat(const File* file)
{
	g_assert(file);
	g_assert(file->type == FILE_POSIX);
	int fd = file->handle.posixfh;
	struct stat finfo;

	CORETIME_START();
	int ret = fstat(fd, &finfo);
	CORETIME_STOP(time);

	if (ret == 0)
		return iostatus_new(TRUE, time, 0);
	else
		return iostatus_new(FALSE, time, 0);
}

IOStatus iio_fcntl(const File* file, int cmd)
{
	g_assert(file);
	g_assert(file->type == FILE_POSIX);
	int fd = file->handle.posixfh;

	CORETIME_START();
	int ret = fcntl(fd, cmd);
	CORETIME_STOP(time);

	if (ret != -1)
		return iostatus_new(TRUE, time, 0);
	else
		return iostatus_new(FALSE, time, 0);
}

IOStatus iio_write(const gchar* filename, glong amount, glong offset) {
	int fd;
	gchar* buffer;
	glong rSize;

	// open the file for writing
	if ((fd = open(filename, O_WRONLY|O_TRUNC|O_CREAT, DEFAULT_OPEN_MODE)) == -1) {
		Warning("(Write) Couldn't open \"%s\" for writing", filename);
		return iostatus_new(FALSE, 0, 0);
	}

	// set file pointer to offset
	if ((offset >= 0) && (lseek(fd, offset, SEEK_SET) != offset)) {
		Warning("(Write) Setting file pointer to offset %ld failed!", offset);
		close(fd);
		return iostatus_new(FALSE, 0, 0);
	}
	else if (offset != OFFSET_CUR)
		Verbose("(Write) File pointer set to offset %ld", offset);

	// allocate memory for data to write
	if ((buffer = g_malloc0(sizeof(gchar)*amount))) {
		glong i;
		for (i=0; i<amount; i++)
			buffer[i] = '0';

		Verbose("(Write) Memory allocated for %ld bytes", amount);
	}
	else {
		Warning("(Write) Couldn't allocate %ld bytes of memory!", amount);
		close(fd);
		return iostatus_new(FALSE, 0, 0);
	}

	// write the data to file
	CORETIME_START();
	if ((rSize = write(fd, buffer, sizeof(gchar)*amount)) < amount) {
		Warning("(Write) Error during write to file \"%s\"! (%ld of %ld)", filename, rSize, amount);
	}
	CORETIME_STOP(time);

	Verbose("(FWrite) File pointer @ %ld", lseek(fd, 0, SEEK_CUR));

	close(fd);
	g_free(buffer);

	if (rSize == amount)
		return iostatus_new(TRUE, time, rSize);
	else
		return iostatus_new(FALSE, time, rSize);
}

IOStatus iio_append(const gchar* filename, glong amount) {
	int fd;
	gchar* buffer;
	glong rSize;

	// open the file for writing
	if ((fd = open(filename, O_APPEND|O_CREAT|O_WRONLY, DEFAULT_OPEN_MODE)) == -1) {
		Warning("(Append) Couldn't open \"%s\" for writing", filename);
		return iostatus_new(FALSE, 0, 0);
	}

	// allocate memory for data to write
	if ((buffer = g_malloc0(sizeof(gchar)*amount))) {
		glong i;
		for (i=0; i<amount; i++)
			buffer[i] = '0';

		Verbose("(Write) Memory allocated for %ld bytes", amount);
	}
	else {
		Warning("(Append) Couldn't allocate %ld bytes of memory!", amount);
		close(fd);
		return iostatus_new(FALSE, 0, 0);
	}

	// write the data to file
	CORETIME_START();
	if ((rSize = write(fd, buffer, sizeof(gchar)*amount)) < amount) {
		Warning("(Append) Error during write to file \"%s\"! (%ld of %ld)", filename, rSize, amount);
	}
	CORETIME_STOP(time);

	Verbose("(Append) File pointer @ %ld", lseek(fd, 0, SEEK_CUR));

	close(fd);
	g_free(buffer);

	if (rSize == amount)
		return iostatus_new(TRUE, time, rSize);
	else
		return iostatus_new(FALSE, time, rSize);
}

IOStatus iio_read(const gchar* filename, glong amount, glong offset) {
	int fd;
	glong  lSize, rSize;
	gchar* buffer;

	// open the file for reading
	if ((fd = open(filename, O_RDONLY, DEFAULT_OPEN_MODE)) == -1) {
		Warning("(Read) Couldn't open \"%s\" for reading", filename);
		return iostatus_new(FALSE, 0, 0);
	}

	// if amount is set to READALL the whole file will be read.
	// else we read only amount bytes and leave the file pointer after
	// the end of the read block of data.
	if (amount == READALL) {
		lSize = (glong) lseek(fd, 0, SEEK_END);
		lseek(fd, 0, SEEK_SET);
	}
	else
		lSize = amount;

	Verbose("(Read) Reading %ld bytes", lSize);

	// set file pointer to offset
	if ((offset >= 0) && (lseek(fd, offset, SEEK_SET) != offset)) {
		Warning("(Read) Setting file pointer to offset %ld failed!", offset);
		close(fd);
		return iostatus_new(FALSE, 0, 0);
	}
	else if (offset != OFFSET_CUR)
		Verbose("(Read) File pointer set to offset %ld", offset);

	// allocate memory
	if (!(buffer = g_malloc0(sizeof(gchar)*lSize))) {
		Warning("(Read) Couldn't allocate %ld bytes of memory!", lSize);
		close(fd);
		return iostatus_new(FALSE, 0, 0);
	}

	CORETIME_START();
	// copy the data into the memory
	if ((rSize = read(fd, buffer, sizeof(gchar)*lSize)) < lSize) {
		Warning("(Read) Error during read from file \"%s\"! (%ld of %ld)", filename, rSize, lSize);
	}
	CORETIME_STOP(time);

	Verbose("(Read) File pointer @ %ld", lseek(fd, 0, SEEK_CUR));

	close(fd);
	g_free(buffer);

	if (rSize == lSize)
		return iostatus_new(TRUE, time, rSize);
	else
		return iostatus_new(FALSE, time, rSize);
}

IOStatus iio_lookup(const gchar* path) {
	CORETIME_START();
	gint rc = g_access(path, F_OK);
	CORETIME_STOP(time);

	if (rc == 0)
		return iostatus_new(TRUE, time, 0);
	else
		return iostatus_new(FALSE, time, 0);
}

IOStatus iio_delete(const gchar* path) {
	CORETIME_START();
	int rc = g_remove(path);
	CORETIME_STOP(time);

	if (rc == 0)
		return iostatus_new(TRUE, time, 0);
	else
		return iostatus_new(FALSE, time, 0);
}

IOStatus iio_mkdir(const gchar* path) {
	CORETIME_START();
	int rc = g_mkdir(path, 0700);
	CORETIME_STOP(time);

	if (rc == 0)
		return iostatus_new(TRUE, time, 0);
	else
		return iostatus_new(FALSE, time, 0);
}

IOStatus iio_mkdir_all(const gchar* path) {
	CORETIME_START();
	int rc = g_mkdir_with_parents(path, 0700);
	CORETIME_STOP(time);

	if (rc == 0)
		return iostatus_new(TRUE, time, 0);
	else
		return iostatus_new(FALSE, time, 0);
}

IOStatus iio_rmdir(const gchar* path) {
	CORETIME_START();
	int rc = g_rmdir(path);
	CORETIME_STOP(time);

	if (rc == 0)
		return iostatus_new(TRUE, time, 0);
	else
		return iostatus_new(FALSE, time, 0);
}

IOStatus iio_create(const gchar* path) {
	int fd;
	CORETIME_START();
	if ((fd = creat(path, 0777))) {
		close(fd);
	}
	CORETIME_STOP(time);

	if (fd != -1)
		return iostatus_new(TRUE, time, 0);
	else
		return iostatus_new(FALSE, time, 0);
}

IOStatus iio_stat(const gchar* path) {
	struct stat finfo;

	CORETIME_START();
	int rc = g_stat(path, &finfo);
	CORETIME_STOP(time);

	if (rc == 0)
		return iostatus_new(TRUE, time, 0);
	else
		return iostatus_new(FALSE, time, 0);
}

IOStatus iio_rename(const gchar* oldname, const gchar* newname) {
	CORETIME_START();
	int rc = g_rename(oldname, newname);
	CORETIME_STOP(time);

	if (rc == 0)
		return iostatus_new(TRUE, time, 0);
	else
		return iostatus_new(FALSE, time, 0);
}

IOStatus iio_link(const gchar* oldpath, const gchar* newpath)
{
	CORETIME_START();
	int rc = link(oldpath, newpath);
	CORETIME_STOP(time);

	if (rc == 0)
		return iostatus_new(TRUE, time, 0);
	else
		return iostatus_new(FALSE, time, 0);
}

IOStatus iio_unlink(const gchar* path)
{
	CORETIME_START();
	int rc = unlink(path);
	CORETIME_STOP(time);

	if (rc == 0)
		return iostatus_new(TRUE, time, 0);
	else
		return iostatus_new(FALSE, time, 0);
}

IOStatus iio_symlink(const gchar* oldpath, const gchar* newpath)
{
	CORETIME_START();
	int rc = symlink(oldpath, newpath);
	CORETIME_STOP(time);

	if (rc == 0)
		return iostatus_new(TRUE, time, 0);
	else
		return iostatus_new(FALSE, time, 0);
}

IOStatus iio_chmod(const gchar* path, mode_t mode)
{
	CORETIME_START();
	int rc = chmod(path, mode);
	CORETIME_STOP(time);

	if (rc == 0)
		return iostatus_new(TRUE, time, 0);
	else
		return iostatus_new(FALSE, time, 0);
}

IOStatus iio_chown(const gchar* path, uid_t owner, gid_t group)
{
	CORETIME_START();
	int rc = chown(path, owner, group);
	CORETIME_STOP(time);

	if (rc == 0)
		return iostatus_new(TRUE, time, 0);
	else
		return iostatus_new(FALSE, time, 0);
}
