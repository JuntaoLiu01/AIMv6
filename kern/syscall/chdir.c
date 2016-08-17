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

#include <syscall.h>
#include <libc/syscalls.h>
#include <libc/string.h>
#include <errno.h>
#include <mm.h>
#include <percpu.h>
#include <proc.h>
#include <ucred.h>
#include <limits.h>
#include <fcntl.h>
#include <fs/namei.h>
#include <fs/vnode.h>

int
sys_chdir(struct trapframe *tf, int *errno, char *upath)
{
	int err;
	char path[PATH_MAX];
	struct nameidata nd;

	if (!is_user(upath)) {
		*errno = EFAULT;
		return -1;
	}
	if (strlcpy(path, upath, PATH_MAX) == PATH_MAX) {
		*errno = ENAMETOOLONG;
		return -1;
	}
	NDINIT_EMPTY(&nd, NOCRED, current_proc);	/* TODO REPLACE */

	/* TODO: proper permission */
	err = vn_open(path, FREAD | FWRITE, 0, &nd);
	if (err) {
		*errno = -err;
		return -1;
	}
	if (nd.vp->type != VDIR) {
		*errno = ENOTDIR;
		vput(nd.vp);
		return -1;
	}

	vrele(current_proc->cwd);
	current_proc->cwd = nd.vp;
	vunlock(current_proc->cwd);
	*errno = 0;
	return 0;
}
ADD_SYSCALL(sys_chdir, NRSYS_chdir);

