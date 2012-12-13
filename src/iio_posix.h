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

#ifndef IIO_POSIX_H_
#define IIO_POSIX_H_

#include "iio.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define DEFAULT_OPEN_MODE S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH


IOStatus iio_fcreat(const gchar* filename, File** file);
IOStatus iio_fopen(const gchar* filename, const gint flags, File** file);
IOStatus iio_fclose(File* file);
IOStatus iio_fwrite(const File* file, glong amount, off_t offset);
IOStatus iio_fread(const File* file, glong amount, off_t offset);
IOStatus iio_fseek(const File* file, off_t offset, gint whence);
IOStatus iio_fsync(const File* file);
// TODO:
IOStatus iio_fstat(const File* file);
IOStatus iio_fcntl(const File* file, int cmd);

IOStatus iio_write(const gchar* filename, glong amount, glong offset);
IOStatus iio_append(const gchar* filename, glong amount);
IOStatus iio_read(const gchar* filename, glong amount, glong offset);
IOStatus iio_lookup(const gchar* path);
IOStatus iio_delete(const gchar* path);
IOStatus iio_mkdir(const gchar* path);
IOStatus iio_mkdir_all(const gchar* path);
IOStatus iio_rmdir(const gchar* path);
IOStatus iio_create(const gchar* path);
IOStatus iio_stat(const gchar* path);
IOStatus iio_rename(const gchar* oldname, const gchar* newname);
// TODO:
IOStatus iio_link(const gchar* oldpath, const gchar* newpath);
IOStatus iio_unlink(const gchar* path);
IOStatus iio_symlink(const gchar* oldpath, const gchar* newpath);
IOStatus iio_chmod(const gchar* path, mode_t mode);
IOStatus iio_chown(const gchar* path, uid_t owner, gid_t group);


#endif /* IIO_POSIX_H_ */
