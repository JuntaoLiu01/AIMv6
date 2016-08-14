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
#include <proc.h>
#include <sched.h>
#include <mm.h>
#include <errno.h>
#include <libc/string.h>
#include <libc/syscalls.h>
#include <syscall.h>
#include <fs/vnode.h>
#include <mp.h>

void forkret(void)
{
	/*
	 * We are coming from switch_contex() invocation from
	 * schedule(), and since we are inside the scheduler
	 * critical section, we need to exit there.
	 *
	 * This trick comes from xv6.
	 */
	sched_exit_critical();

	proc_trap_return(current_proc);
}

extern void __arch_fork(struct proc *child, struct proc *parent);
pid_t sys_fork(struct trapframe *tf, int *errno)
{
	struct proc *child;
	int err;

	int pid;
	unsigned long flags;

	local_irq_save(flags);

	child = proc_new(current_proc->namespace);

	if (child == NULL) {
		*errno = ENOMEM;
		goto fail;
	}

	if ((child->mm = mm_new()) == NULL) {
		*errno = ENOMEM;
		goto rollback_child;
	}

	if ((err = mm_clone(child->mm, current_proc->mm)) < 0) {
		*errno = err;
		goto rollback_mm;
	}

	__arch_fork(child, current_proc);

	strlcpy(child->name, current_proc->name, PROC_NAME_LEN_MAX);

	/* TODO: duplicate more necessary stuff */
	child->heapsize = current_proc->heapsize;
	child->heapbase = current_proc->heapbase;
	child->cwd = current_proc->cwd;
	child->rootd = current_proc->rootd;
	memcpy(&child->fd, &current_proc->fd, sizeof(child->fd));
	/* increase file vnode ref counts but do not lock them */
	for (int i = 0; i < OPEN_MAX; ++i) {
		if (child->fd[i] != NULL)
			FREF(child->fd[i]);
	}

	pid = child->pid;
	child->state = PS_RUNNABLE;

	child->mainthread = child;
	child->groupleader = current_proc->groupleader;
	child->sessionleader = current_proc->sessionleader;

	child->tty = current_proc->tty;
	child->ttyvnode = current_proc->ttyvnode;

	proctree_add_child(child, current_proc);

	proc_add(child);

	local_irq_restore(flags);

	return pid;

rollback_mm:
	mm_destroy(child->mm);
rollback_child:
	proc_destroy(child);
fail:
	local_irq_restore(flags);
	return -1;
}
ADD_SYSCALL(sys_fork, NRSYS_fork);
