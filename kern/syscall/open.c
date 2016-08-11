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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <syscall.h>
#include <libc/syscalls.h>
#include <libc/string.h>
#include <errno.h>
#include <file.h>
#include <limits.h>
#include <percpu.h>
#include <ucred.h>
#include <proc.h>
#include <fcntl.h>
#include <fs/vnode.h>
#include <fs/namei.h>

int
sys_open(struct trapframe *tf, int *errno, char *ufilename, int flags, int mode)
{
	char path[PATH_MAX];
	int i, err;	/* i is the file descriptor to be returned */
	int openflags, ioflags;
	struct nameidata nd;
	struct vattr va;

	for (i = 0; i < OPEN_MAX; ++i) {
		if (current_proc->fd[i].type == FNON)
			goto found;
	}
	/* no idle descriptor found :( */
	*errno = -ENFILE;
	return -1;
found:
	if (strlcpy(path, ufilename, PATH_MAX) == PATH_MAX)
		return -ENAMETOOLONG;

	NDINIT_EMPTY(&nd, NOCRED, current_proc);	/* TODO REPLACE */

	/* Convert fcntl(2) flags to ioflags and flags for vn_open() */
	openflags = flags & ~O_ACCMODE;
	ioflags = 0;
	switch (flags & O_ACCMODE) {
	case O_RDONLY:
		openflags |= FREAD;
		break;
	case O_WRONLY:
		openflags |= FWRITE;
		break;
	case O_RDWR:
		openflags |= FREAD | FWRITE;
		break;
	default:
		*errno = EINVAL;
		return -1;
	}
	if (flags & O_APPEND)
		ioflags |= IO_APPEND;

	err = vn_open(path, openflags, mode, &nd);
	if (err) {
		*errno = -err;
		return -1;
	}

	/* Truncate the file as required */
	if (flags & O_TRUNC) {
		vattr_null(&va);
		va.size = 0;
		err = VOP_SETATTR(nd.vp, &va, nd.cred, nd.proc);
		if (err)
			goto rollback_vp;
	}

	/* Succeeded, put the vnode into file descriptor table */
	current_proc->fd[i].type = FVNODE;
	current_proc->fd[i].vnode = nd.vp;

	vunlock(nd.vp);
	*errno = 0;
	return i;
rollback_vp:
	vput(nd.vp);
	*errno = -err;
	return -1;
}
ADD_SYSCALL(sys_open, NRSYS_open);

