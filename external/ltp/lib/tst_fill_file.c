/*
 * Copyright (c) 2013 Oracle and/or its affiliates. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it would be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write the Free Software Foundation,
 * Inc.,  51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Author: Stanislav Kholmanskikh <stanislav.kholmanskikh@oracle.com>
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "test.h"

int tst_fill_file(const char *path, char pattern, size_t bs, size_t bcount)
{
	int fd;
	size_t counter;
	char *buf;

	fd = open(path, O_CREAT|O_WRONLY|O_TRUNC, S_IRUSR|S_IWUSR);
	if (fd < 0)
		return -1;

	/* Filling a memory buffer with provided pattern */
	buf = malloc(bs);
	if (buf == NULL) {
		close(fd);

		return -1;
	}

	for (counter = 0; counter < bs; counter++)
		buf[counter] = pattern;

	/* Filling the file */
	for (counter = 0; counter < bcount; counter++) {
		if (write(fd, buf, bs) != (ssize_t)bs) {
			free(buf);
			close(fd);
			unlink(path);

			return -1;
		}
	}

	free(buf);

	if (close(fd) < 0) {
		unlink(path);

		return -1;
	}

	return 0;
}
