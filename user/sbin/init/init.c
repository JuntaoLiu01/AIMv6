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

#include <libc/string.h>
#include <libc/unistd.h>

int main(int argc, char *argv[], char *envp[])
{
	char c;
	pid_t pid;

	/*
	 * Replace it with your own job for now.
	 */
	write(STDOUT_FILENO, "INIT: now init\n", 15);
	pid = fork();
	if (pid == 0) {
		getpid();
		for (;;)
			;
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
