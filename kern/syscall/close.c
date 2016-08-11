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
#include <aim/sync.h>
#include <file.h>
#include <proc.h>
#include <percpu.h>
#include <ucred.h>
#include <errno.h>
#include <fs/vnode.h>
#include <fs/vfs.h>

int
sys_close(struct trapframe *tf, int *errno, int fd)
{
	struct vnode *vnode;
	unsigned long flags;
	int err;
	struct file *file = &current_proc->fd[fd];

	switch (file->type) {
	case FNON:
		*errno = EBADF;
		return -1;
	case FVNODE:
		spin_lock_irq_save(&current_proc->fdlock, flags);
		vnode = file->vnode;
		vlock(vnode);
		/* TODO REPLACE */
		err = VOP_CLOSE(vnode, file->openflags, NOCRED, current_proc);
		if (err) {
			vunlock(vnode);
			spin_unlock_irq_restore(&current_proc->fdlock, flags);
			*errno = -err;
			return -1;
		}

		/* TODO REPLACE */
		VFS_SYNC(file->vnode->mount, NOCRED, current_proc);
		vput(vnode);

		file->vnode = NULL;
		file->type = FNON;
		*errno = 0;
		spin_unlock_irq_restore(&current_proc->fdlock, flags);
		return 0;
	default:
		*errno = ENOTSUP;
		return -1;
	}
}
ADD_SYSCALL(sys_close, NRSYS_close);

