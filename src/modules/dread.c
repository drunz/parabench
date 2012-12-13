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

IOStatus dread(const gchar* fname)
{
	int fd;
	if ((fd = open(fname, O_RDONLY|O_DIRECT)) == -1)
		return iostatus_new(FALSE, 0, 0);

	off_t fileSize = lseek(fd, 0, SEEK_END);
	//printf("fileSize = %ld", fileSize);
	lseek(fd, 0, SEEK_SET);

	void* buffer;
	int pageSize = getpagesize();
	if (posix_memalign(&buffer, pageSize, fileSize))
		return iostatus_new(FALSE, 0, 0);

	CORETIME_START();
	size_t bytesRead = read(fd, buffer, (size_t) fileSize);
	CORETIME_STOP(time);

	close(fd);

	free(buffer);

	if (bytesRead == size)
		return iostatus_new(TRUE, time, bytesRead);
	else
		return iostatus_new(FALSE, time, bytesRead);
}
