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

#include <mm.h>
#include <vmm.h>
#include <panic.h>
#include <aim/console.h>
#include <regs.h>
#include <trap.h>

#include <arch-trap.h>

static void arm_init_mode(uint32_t psr_c, char *name)
{
	struct trapframe * tf;

	tf = kmalloc(sizeof(*tf), 0);
	if (tf == NULL)
		panic("Failed to allocate %s context storage.\n", name);
	kprintf("KERN: %s context at 0x%08x.\n", name, tf);
	asm volatile (
		"msr	cpsr_c, %[state1];"
		"mov	sp, %[top];"
		"msr	cpsr_c, %[state2];"
	: /* no output */
	: [top] "r" (tf + 1),
	  [state1] "r" (psr_c),
	  [state2] "r" (0xDF)
	);
}

void trap_init(void)
{
	/* initialize exception vector */
	extern uint32_t arm_vector;
	asm volatile(
		"mcr p15, 0, %[addr], c12, c0, 0;"
		::[addr] "r" (&arm_vector)
	);

	/* IRQ mode */
	arm_init_mode(0xD2, "IRQ");

	/* FIQ(0xD1) mode not used */

	/* SVC mode */
	arm_init_mode(0xD3, "SVC");

	/* ABT mode */
	arm_init_mode(0xD7, "ABT");

	/* UNDEF mode */
	arm_init_mode(0xDB, "UNDEF");

	return;
}

__noreturn
void trap_return(struct trapframe *tf)
{
	/*
	 * You MUST be in SYS mode to call this routine.
	 * tf MUST NOT be on the heap UNLESS it IS exactly the one passed
	 * in arm_handle_trap and IRQ is never enabled. This routine will not
	 * free the pointer.
	 * This routine returns to the execution state in tf and all further
	 * stack use are discarded.
	 * If this current exception interrupts a kernel control flow, the
	 * previous stack state is fully recovered.
	 */
	asm volatile (
		/* disable interrupts */
		"msr	cpsr_c, 0xDF;"
		/* restore banked registers */
		"ldmia	r0!, {sp, lr};"
		/* go to SVC mode */
		"msr	cpsr_c, 0xD3;"
		/* restore other registers */
		"ldmia	r0!, {r1};"
		"msr	spsr, r1;"
		"ldmia	r0, {r0-r12, pc}^;"
		/* the instruction above performs the exception return */
	);
	panic("Control flow went beyond trap_return().");
}

static void dump_regs(struct trapframe *tf)
{
	kprintf("DEBUG: r0 = 0x%08x\n", tf->r0);
	kprintf("DEBUG: r1 = 0x%08x\n", tf->r1);
	kprintf("DEBUG: r2 = 0x%08x\n", tf->r2);
	kprintf("DEBUG: r3 = 0x%08x\n", tf->r3);
	kprintf("DEBUG: r4 = 0x%08x\n", tf->r4);
	kprintf("DEBUG: r5 = 0x%08x\n", tf->r5);
	kprintf("DEBUG: r6 = 0x%08x\n", tf->r6);
	kprintf("DEBUG: r7 = 0x%08x\n", tf->r7);
	kprintf("DEBUG: r8 = 0x%08x\n", tf->r8);
	kprintf("DEBUG: r9 = 0x%08x\n", tf->r9);
	kprintf("DEBUG: r10 = 0x%08x\n", tf->r10);
	kprintf("DEBUG: r11 = 0x%08x\n", tf->r11);
	kprintf("DEBUG: r12 = 0x%08x\n", tf->r12);
	kprintf("DEBUG: pc = 0x%08x\n", tf->pc);
	kprintf("DEBUG: psr = 0x%08x\n", tf->psr);
	kprintf("DEBUG: sp = 0x%08x\n", tf->sp);
	kprintf("DEBUG: lr = 0x%08x\n", tf->lr);
}

struct trapframe *arm_copy_trapframe(struct trapframe *dest, struct trapframe *src)
{
	*dest = *src;
	return dest;
}

void handle_data_abort(tf)
{
	uint32_t status, addr;
	asm volatile (
		"mrc	p15, 0, %[status], c5, c0, 0;"
		"mrc	p15, 0, %[addr], c6, c0, 0;"
	:
		[status] "=r" (status),
		[addr] "=r" (addr)
	);
	panic("ARM Data Abort, DFSR=0x%08x, DFAR=0x%08x.", status, addr);
}

__noreturn
void arm_handle_trap(struct trapframe *tf, uint32_t type)
{
	kprintf("DEBUG: Enter vector slot %d handler!\n", type);
	dump_regs(tf);

	switch (type) {
	case ARM_SVC:
		handle_syscall(tf);
		break;
	case ARM_IRQ:
		handle_interrupt(tf);
		break;
	case ARM_DATA_ABT:
		handle_data_abort(tf);
		break;
	default:
		panic("Unexpected trap.\n");
	}
	trap_return(tf);
}

