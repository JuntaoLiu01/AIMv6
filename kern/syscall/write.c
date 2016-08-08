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
#include <fs/vnode.h>
#include <fs/uio.h>
#include <ucred.h>
#include <syscall.h>
#include <libc/syscalls.h>
#include <errno.h>

ssize_t sys_write(struct trapframe *tf, int *errno, int fd, void *buf,
    size_t count)
{
	struct file *file = &current_proc->fd[fd];
	int err;
	size_t len;

	if (file->type != FVNODE) {
		/* pipe NYI */
		*errno = -ENOTSUP;
		return -1;
	}

	if (file->vnode == NULL) {
		*errno = EBADF;
		return -1;
	}
	if (file->vnode->type == VDIR) {
		*errno = EISDIR;
		return -1;
	}

	vlock(file->vnode);
	err = vn_write(file->vnode, file->offset, len, buf, file->ioflags,
	    UIO_USER, current_proc, NULL, NOCRED, &len); /* TODO REPLACE */
	if (err) {
		*errno = -err;
		vunlock(file->vnode);
		return -1;
	}
	file->offset += 0;	/* TODO compute new offset */
	vunlock(file->vnode);
	*errno = 0;
	return len;
}
ADD_SYSCALL(sys_write, NRSYS_write);

