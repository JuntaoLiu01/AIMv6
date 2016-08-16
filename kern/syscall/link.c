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
#include <fs/namei.h>
#include <fs/vnode.h>

int
sys_link(struct trapframe *tf, int *errno, char *old, char *new)
{
	char oldpath[PATH_MAX], newpath[PATH_MAX];
	struct nameidata ndold, ndnew;
	int err;

	if (!is_user(old) || !is_user(new)) {
		*errno = EFAULT;
		return -1;
	}

	if (strlcpy(oldpath, old, PATH_MAX) == PATH_MAX) {
		*errno = ENAMETOOLONG;
		return -1;
	}
	if (strlcpy(newpath, new, PATH_MAX) == PATH_MAX) {
		*errno = ENAMETOOLONG;
		return -1;
	}
	if (namei_trim_slash(oldpath) || namei_trim_slash(newpath)) {
		*errno = EPERM;
		return -1;
	}

	NDINIT(&ndold, oldpath, NAMEI_LOOKUP, NAMEI_FOLLOW, NOCRED,
	    current_proc);
	if ((err = namei(&ndold)) != 0) {
		*errno = -err;
		return -1;
	}
	if (ndold.vp->type == VDIR) {
		*errno = EPERM;
		goto rollback_old;
	}

	NDINIT(&ndnew, newpath, NAMEI_CREATE, NAMEI_FOLLOW | NAMEI_PARENT,
	    NOCRED, current_proc);
	if ((err = namei(&ndnew)) != 0) {
		*errno = -err;
		goto rollback_old;
	}
	assert(ndnew.parentvp != NULL);
	assert(ndnew.seg != NULL);
	if (ndnew.vp != NULL) {
		*errno = EEXIST;
		goto rollback_new;
	}

	if ((err = VOP_LINK(ndnew.parentvp, ndnew.seg, ndold.vp, NOCRED,
	    current_proc)) != 0) {
		*errno = -err;
		goto rollback_new;
	}

	*errno = 0;
	return 0;

rollback_new:
	namei_cleanup(&ndnew);
	namei_putparent(&ndnew);
	vput(ndnew.vp);
rollback_old:
	namei_cleanup(&ndold);
	vput(ndold.vp);
	return -1;
}
ADD_SYSCALL(sys_link, NRSYS_link);
