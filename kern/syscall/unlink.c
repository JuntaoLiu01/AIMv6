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
#include <panic.h>
#include <limits.h>
#include <fs/namei.h>
#include <fs/vnode.h>

int
sys_unlink(struct trapframe *tf, int *errno, char *upath)
{
	struct nameidata nd;
	char path[PATH_MAX];
	int err;

	if (!is_user(upath)) {
		*errno = EFAULT;
		return -1;
	}
	if (strlcpy(path, upath, PATH_MAX) == PATH_MAX) {
		*errno = ENAMETOOLONG;
		return -1;
	}
	if (namei_trim_slash(path)) {
		*errno = EPERM;
		return -1;
	}

	NDINIT(&nd, path, NAMEI_LOOKUP, NAMEI_FOLLOW | NAMEI_PARENT, NOCRED,
	    current_proc);
	if ((err = namei(&nd)) != 0) {
		*errno = -err;
		return -1;
	}
	assert(nd.parentvp != NULL);
	assert(nd.seg != NULL);
	assert(nd.vp != NULL);

	err = VOP_REMOVE(nd.parentvp, nd.seg, nd.vp, NOCRED, current_proc);

	namei_cleanup(&nd);
	namei_putparent(&nd);
	vput(nd.vp);
	*errno = -err;
	return err ? -1 : 0;
}
ADD_SYSCALL(sys_unlink, NRSYS_unlink);

