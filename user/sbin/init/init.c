/* Copyright (C) 2016 Gan Quan <coin2028@hotmail.com>
 *
 * This file is part of AIMv6.
 *
 * AIMv6 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * AIMv6 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <libc/stdio.h>
#include <libc/string.h>
#include <libc/unistd.h>
#include <libc/fcntl.h>

int main(int argc, char *argv[], char *envp[])
{
	char c;
	char buf[50];
	int fd;

	/*
	 * Replace it with your own job for now.
	 */
	printf("INIT: now init\n");
	memset(buf, 0, sizeof(buf));
	fd = open("/etc/hostname", O_WRONLY, 0);
	if (fd != -1) {
		write(fd, "good\n", 5);
		lseek(fd, 10000, SEEK_SET);
		write(fd, "wow\n", 4);
		close(fd);
	}

	for (;;) {
		/* echo, since terminal echoing is NYI */
		if (read(STDIN_FILENO, &c, 1) != 1)
			break;
		if (write(STDOUT_FILENO, &c, 1) != 1)
			break;
	}
	for (;;)
		;
}
