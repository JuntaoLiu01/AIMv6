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
	char buf[512];
	int fd;

	/*
	 * Replace it with your own job for now.
	 */
	printf("%d\n", chdir("etc"));
	fd = open("hostname", O_WRONLY | O_CREAT | O_TRUNC, 0755);
	printf("%d\n", fd);
	if (fd != -1) {
		write(fd, "localhost\n", 10);
		close(fd);
	}
	if (fork() == 0) {
		fd = open("profile", O_WRONLY | O_CREAT | O_TRUNC, 0755);
		printf("%d %d\n", getpid(), fd);
		if (fd != -1)
			close(fd);
		sync();
		for (;;) ;
	}
	printf("%d\n", chdir(".."));
	fd = open("etc/hostname", O_RDONLY, 0);
	printf("%d\n", fd);
	if (fd != -1) {
		memset(buf, 0, sizeof(buf));
		printf("%d\n", (int)read(fd, buf, 50));
		puts(buf);
		close(fd);
	}
	sync();
	for (;;) {
		gets(buf);
		puts(buf);
	}
}

