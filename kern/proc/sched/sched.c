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
#include <percpu.h>
#include <mp.h>
#include <panic.h>

static lock_t sched_lock;
static unsigned long __sched_intrflags;

void sched_enter_critical(void)
{
	spin_lock_irq_save(&sched_lock, __sched_intrflags);
}

void sched_exit_critical(void)
{
	spin_unlock_irq_restore(&sched_lock, __sched_intrflags);
}

static void __update_proc(struct proc *proc)
{
	proc->state = PS_ONPROC;
	proc->oncpu = cpuid();
}

void schedule(void)
{
	struct proc *oldproc = current_proc, *newproc;

	sched_enter_critical();

	assert(!((oldproc->kpid == 0) ^ (oldproc == cpu_idleproc)));
	if (oldproc->state == PS_ONPROC)
		oldproc->state = PS_RUNNABLE;

	newproc = scheduler->pick();
	if (newproc == NULL)
		newproc = cpu_idleproc;

	__update_proc(newproc);

	switch_context(newproc);

	sched_exit_critical();
}

void proc_add(struct proc *proc)
{
	scheduler->add(proc);
}

void sched_init(void)
{
	spinlock_init(&sched_lock);
}

