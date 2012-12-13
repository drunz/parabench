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

//#define _GNU_SOURCE

#include "timing.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

IOStatus dwrite(const gchar* fname, glong size)
{
	void* buffer;
	int pageSize = getpagesize();
	if (posix_memalign(&buffer, pageSize, size))
		return iostatus_new(FALSE, 0, 0);

	int fd;
	if ((fd = open(fname, O_CREAT|O_TRUNC|O_WRONLY|O_DIRECT, S_IRWXU)) == -1)
		return iostatus_new(FALSE, 0, 0);

	CORETIME_START();
	size_t bytesWritten = write(fd, buffer, (size_t) size);
	CORETIME_STOP(time);

	close(fd);

	free(buffer);

	if (bytesWritten == size)
		return iostatus_new(TRUE, time, bytesWritten);
	else
		return iostatus_new(FALSE, time, bytesWritten);
}
