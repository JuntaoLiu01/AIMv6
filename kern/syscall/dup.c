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

#include <percpu.h>
#include <file.h>
#include <aim/sync.h>
#include <fs/vnode.h>
#include <fs/vfs.h>
#include <errno.h>
#include <syscall.h>
#include <libc/syscalls.h>
#include <limits.h>
#include <ucred.h>

static int do_dup(int oldfd, int newfd)
{
	unsigned long flags;
	struct file *file = current_proc->fd[newfd];
	struct vnode *vnode;
	int err;

	if (current_proc->fd[oldfd] == NULL)
		return -EBADF;
	/* dup2(fd, fd) does nothing */
	if (oldfd == newfd)
		return 0;
	spin_lock_irq_save(&current_proc->fdlock, flags);

	/* Close newfd if it's an opened handle */
	if (file != NULL) {
		/*
		 * The following is a manual close() handler.
		 * TODO: merge common code with close() and dup() while maintaining
		 * atomicity of dup().
		 */
		switch (file->type) {
		case FNON:
			spin_unlock_irq_restore(&current_proc->fdlock, flags);
			return -EBADF;
		case FVNODE:
			vnode = file->vnode;
			vlock(vnode);
			/* TODO REPLACE */
			err = VOP_CLOSE(vnode, file->openflags, NOCRED,
			    current_proc);
			if (err) {
				vunlock(vnode);
				spin_unlock_irq_restore(&current_proc->fdlock,
				    flags);
				return err;
			}

			/* TODO REPLACE */
			VFS_SYNC(vnode->mount, NOCRED, current_proc);
			vput(vnode);
			FRELE(&current_proc->fd[newfd]);
			break;
		default:
			spin_unlock_irq_restore(&current_proc->fdlock, flags);
			return -ENOTSUP;
		}
	}

	/*
	 * Link oldfd to newfd
	 * According to POSIX, if file descriptor flags are considered,
	 * then a file descriptor should contain a record of struct
	 * file and a file descriptor flags, because such flags are
	 * not shared across dup()'ed file descriptors.
	 *
	 * In AIMv6, we do not consider file descriptor flags.
	 *
	 * TODO: consider file descriptor flags such as FD_CLOEXEC,
	 * and refactor the per-process file descriptor table.
	 */
	current_proc->fd[newfd] = current_proc->fd[oldfd];
	FREF(current_proc->fd[newfd]);
	switch (current_proc->fd[oldfd]->type) {
	case FVNODE:
		vref(current_proc->fd[newfd]->vnode);
		break;
	default:
		break;
	}
	spin_unlock_irq_restore(&current_proc->fdlock, flags);
	return 0;

}

int sys_dup(struct trapframe *tf, int *errno, int oldfd)
{
	int i;

	for (i = 0; i < OPEN_MAX; ++i) {
		if (current_proc->fd[i] == NULL)
			goto found;
	}
	*errno = EMFILE;
	return -1;
found:
	*errno = -do_dup(oldfd, i);
	if (*errno == 0)
		return i;
	else
		return -1;
}
ADD_SYSCALL(sys_dup, NRSYS_dup);

int sys_dup2(struct trapframe *tf, int *errno, int oldfd, int newfd)
{
	if (newfd >= OPEN_MAX) {
		*errno = EBADF;
		return -1;
	}
	*errno = -do_dup(oldfd, newfd);
	if (*errno == 0)
		return newfd;
	else
		return -1;
}
ADD_SYSCALL(sys_dup2, NRSYS_dup2);
