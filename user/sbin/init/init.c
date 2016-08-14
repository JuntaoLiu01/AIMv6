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
	int pipefd[2];

	/*
	 * Replace it with your own job for now.
	 */
	pipe(pipefd);
	printf("%d %d\n", pipefd[0], pipefd[1]);
	if (fork() == 0) {
		/* Child reads from pipe and writes to terminal */
		close(pipefd[1]);
		dup2(pipefd[0], STDIN_FILENO);
		close(pipefd[0]);
	} else {
		/* Parent reads from terminal and writes to pipe */
		close(pipefd[0]);
		dup2(pipefd[1], STDOUT_FILENO);
		close(pipefd[1]);
	}
	for (;;) {
		gets(buf);
		puts(buf);
	}
}
