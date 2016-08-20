/* Copyright (C) 2016 David Gao <davidgao1001@gmail.com>
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
#endif /* HAVE_CONFIG_H */

#include <sys/types.h>
#include <arch-trap.h>
#include <proc.h>
#include <percpu.h>

static struct trapframe *__proc_trapframe(struct proc *proc)
{
	struct trapframe *tf;

	tf = (struct trapframe *)(kstacktop(proc) - sizeof(*tf));
	return tf;
}

static void __bootstrap_trapframe(struct trapframe *tf,
				   void *entry,
				   void *stacktop,
				   void *args)
{
	/*
	 * As all registers, no matter caller-saved or callee-saved,
	 * are preserved in trap frames, we can safely assign all
	 * registers with some values.
	 */
	tf->psr = arm_read_cpsr();
	/* Enable interrupts in trap frame */
	tf->psr &= ~ARM_IRQ_MASK;
	tf->pc = (unsigned long)entry;
	tf->sp = (unsigned long)stacktop;
	tf->r0 = (unsigned long)args;
}

extern void forkret(void);
static void __bootstrap_context(struct regs *regs, struct trapframe *tf)
{
	/*
	 * Some of the registers are scratch registers (i.e. caller-saved),
	 * so we should notice that only callee-saved registers can be
	 * played with when bootstrapping contexts, as the values in scratch
	 * registers are not necessarily preserved before and after
	 * executing a routine (hence unreliable).
	 *
	 * On ARM, callee-saved registers are: r4-r11, ip, sp and fp.
	 * pc is not saved, and lr is caller-saved.
	 *
	 * In practice, only callee-saved registers are to be preserved in
	 * per-process context structure, but I reserved spaces for all
	 * registers anyway.
	 * Maybe I should save only callee-saved registers in future...
	 */
	regs->psr = arm_read_cpsr();
	/* Enable interrupt masks in context */
	regs->psr &= ~ARM_IRQ_MASK;
	/* Kernel stack pointer just below trap frame */
	regs->sp = (unsigned long)tf;
	regs->pc = (unsigned long)forkret;
}

static void __bootstrap_user(struct trapframe *tf)
{
	tf->psr &= ~ARM_MODE_MASK;
	tf->psr |= ARM_MODE_USR;
}

void __proc_ksetup(struct proc *proc, void *entry, void *args)
{
	struct trapframe *tf = __proc_trapframe(proc);
	__bootstrap_trapframe(tf, entry, kstacktop(proc), args);
	__bootstrap_context(&(proc->context), tf);
}

void __proc_usetup(struct proc *proc, void *entry, void *stacktop, void *args)
{
	struct trapframe *tf = __proc_trapframe(proc);
	__bootstrap_trapframe(tf, entry, stacktop, args);
	__bootstrap_context(&(proc->context), tf);
	__bootstrap_user(tf);
}

void __prepare_trapframe_and_stack(struct trapframe *tf, void *entry,
    void *ustacktop, int argc, char *argv[], char *envp[])
{
	tf->pc = (unsigned long)entry;
	tf->sp = (unsigned long)(ustacktop - 32);
	tf->r0 = argc;
	tf->r1 = (unsigned long)argv;
	tf->r2 = (unsigned long)envp;
	/* Make sure we are in user mode */
	tf->psr &= ~ARM_MODE_MASK;
	tf->psr |= ARM_MODE_USR;
}

void proc_trap_return(struct proc *proc)
{
	struct trapframe *tf = __proc_trapframe(proc);

	trap_return(tf);
}

void __arch_fork(struct proc *child, struct proc *parent)
{
	struct trapframe *tf_child = __proc_trapframe(child);
	struct trapframe *tf_parent = __proc_trapframe(parent);

	*tf_child = *tf_parent;
	tf_child->r0 = 0;
	tf_child->r1 = 0;

	__bootstrap_context(&(child->context), tf_child);
}

void switch_context(struct proc *proc)
{
	struct proc *current = current_proc;
	current_proc = proc;

	/* Switch page directory */
	switch_pgindex(proc->mm->pgindex);
	/* Switch kernel stack */
	current_kernelsp = (unsigned long)kstacktop(proc);
	/* Switch general registers */
	switch_regs(&(current->context), &(proc->context));
}

