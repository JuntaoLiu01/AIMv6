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

#include <proc.h>
#include <sched.h>
#include <panic.h>
#include <libc/unistd.h>
#include <fs/vfs.h>	/* fsinit() */
#include <fs/namei.h>
#include <fs/vnode.h>
#include <fs/specdev.h>
#include <fs/uio.h>
#include <drivers/tty/tty.h>
#include <mach-conf.h>
#include <percpu.h>
#include <fcntl.h>
#include <ucred.h>

static struct proc *initproc;

char *initargv[] = {
	"/sbin/init",
	NULL
};

char *initenvp[] = {
	NULL
};

static void ftty_setup(struct file **fpp, struct vnode *vnode, int openflags)
{
	*fpp = FNEW();
	FINIT_VNODE(*fpp, vnode, 0, 0, openflags);
	vref((*fpp)->vnode);
}

static void ttyinit(void)
{
	struct nameidata nd;
	dev_t ttydevno;
	struct tty_device *tty;

	nd.proc = current_proc;
	nd.cred = NOCRED;	/* TODO REPLACE? */
	if (vn_open("/dev/tty", FREAD | FWRITE, 0, &nd) != 0)
		panic("/dev/tty: not found\n");
	if (nd.vp->type != VCHR)
		panic("/dev/tty: bad file type\n");
	assert(major(vdev(nd.vp)) == TTY_MAJOR);
	nd.vp->flags |= VISTTY;

	/* Set up initproc stdin, stdout, stderr */
	ftty_setup(&current_proc->fstdin, nd.vp, FREAD);
	ftty_setup(&current_proc->fstdout, nd.vp, FWRITE);
	ftty_setup(&current_proc->fstderr, nd.vp, FWRITE);

	/* Set up session data */
	ttydevno = nd.vp->specinfo->devno;
	tty = (struct tty_device *)dev_from_id(ttydevno);
	tty->fg = current_proc;
	tty->session = current_proc;
	current_proc->ttyvnode = nd.vp;
	current_proc->tty = tty;

	vput(nd.vp);
	assert(nd.vp->refs == 3);
}

void initproc_entry(void)
{
	/*
	 * initproc mounts the root filesystem first in kernel space,
	 * then finds the /sbin/init executable, loads it, and performs
	 * an execve(2) system call to jump into user space.
	 *
	 * The reason we mount a file system in a (kernel) process is that
	 * interacting with disks involves sleep() and wakeup(), which
	 * requires a working scheduler and interrupts enabled.
	 */
	kpdebug("initproc running.\n");
	fsinit();
	kpdebug("FS initialized.\n");
	ttyinit();
	kpdebug("TTY initialized.\n");

	current_proc->cwd = rootvnode;
	vref(rootvnode);
	current_proc->rootd = rootvnode;
	vref(rootvnode);

	execve("/sbin/init", initargv, initenvp);

	for (;;)
		/* nothing */;
}

void spawn_initproc(void)
{
	initproc = proc_new(NULL);
	proc_ksetup(initproc, initproc_entry, NULL);
	initproc->state = PS_RUNNABLE;
	initproc->groupleader = initproc;
	initproc->sessionleader = initproc;
	initproc->mainthread = initproc;
	proc_add(initproc);
}

