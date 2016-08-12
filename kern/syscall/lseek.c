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
#include <libc/unistd.h>
#include <sys/types.h>
#include <file.h>
#include <fs/vnode.h>
#include <ucred.h>
#include <percpu.h>
#include <errno.h>

int
sys_lseek(struct trapframe *tf, int *errno, int fd, off_t offset, int whence)
{
	struct file *file = &current_proc->fd[fd];
	struct vattr va;
	int err;

	if (file->type == FNON) {
		*errno = EBADF;
		return -1;
	}
	if (file->type != FVNODE) {
		*errno = ESPIPE;
		return -1;
	}

	switch (whence) {
	case SEEK_SET:
		file->offset = offset;
		break;
	case SEEK_CUR:
		/* Shall we allow negative offset? */
		file->offset += offset;
		break;
	case SEEK_END:
		/* TODO REPLACE */
		err = VOP_GETATTR(file->vnode, &va, NOCRED, current_proc);
		if (err) {
			/*
			 * Should not get here since the metadata should be
			 * loaded upon opening the file.  But let's just
			 * assume that maybe someone would mess up.
			 */
			*errno = -err;
			return -1;
		}
		file->offset = va.size + offset;
		break;
	default:
		*errno = EINVAL;
		return -1;
	}

	*errno = 0;
	return file->offset;
}
ADD_SYSCALL(sys_lseek, NRSYS_lseek);
