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

#include <sys/types.h>
#include <file.h>
#include <proc.h>
#include <percpu.h>
#include <syscall.h>
#include <libc/syscalls.h>
#include <errno.h>
#include <fcntl.h>

ssize_t sys_read(struct trapframe *tf, int *errno, int fd, void *buf,
    size_t count)
{
	struct file *file;
	int err;
	size_t len;
	loff_t oldoff;

	if (fd < 0 || fd >= OPEN_MAX) {
		*errno = EBADF;
		return -1;
	}
	file = current_proc->fd[fd];
	if (file == NULL || !(file->openflags & FREAD)) {
		*errno = EBADF;
		return -1;
	}
	if (file->type == FNON) {
		*errno = EBADF;
		return -1;
	}
	if (!is_user(buf)) {
		*errno = EFAULT;
		return -1;
	}
	oldoff = file->offset;
	err = FILE_READ(file, buf, count, &file->offset);
	len = file->offset - oldoff;
	if (err) {
		*errno = -err;
		return -1;
	}
	*errno = 0;
	return len;
}
ADD_SYSCALL(sys_read, NRSYS_read);

