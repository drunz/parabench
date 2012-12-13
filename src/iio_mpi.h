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

#ifndef IIO_MPIIO_H_
#define IIO_MPIIO_H_

#include "iio.h"

#ifdef HAVE_MPI
gboolean iio_pfopen(const gchar* filename, const gchar* mode, MPI_Comm comm, File** file);
gboolean iio_pfclose(File* file);
IOStatus iio_pfwrite_level0(const File* file, Pattern* pattern);
IOStatus iio_pfwrite_level1(const File* file, Pattern* pattern);
IOStatus iio_pfwrite_level2(const File* file, Pattern* pattern);
IOStatus iio_pfwrite_level3(const File* file, Pattern* pattern);
IOStatus iio_pfread_level0(const File* file, Pattern* pattern);
IOStatus iio_pfread_level1(const File* file, Pattern* pattern);
IOStatus iio_pfread_level2(const File* file, Pattern* pattern);
IOStatus iio_pfread_level3(const File* file, Pattern* pattern);

IOStatus iio_pwrite_level0(const gchar* path, Pattern* pattern, MPI_Comm comm);
IOStatus iio_pwrite_level1(const gchar* path, Pattern* pattern, MPI_Comm comm);
IOStatus iio_pwrite_level2(const gchar* path, Pattern* pattern, MPI_Comm comm);
IOStatus iio_pwrite_level3(const gchar* path, Pattern* pattern, MPI_Comm comm);
IOStatus iio_pread_level0(const gchar* path, Pattern* pattern, MPI_Comm comm);
IOStatus iio_pread_level1(const gchar* path, Pattern* pattern, MPI_Comm comm);
IOStatus iio_pread_level2(const gchar* path, Pattern* pattern, MPI_Comm comm);
IOStatus iio_pread_level3(const gchar* path, Pattern* pattern, MPI_Comm comm);

gboolean iio_pdelete(const gchar* path);
#endif

#endif /* IIO_MPIIO_H_ */
