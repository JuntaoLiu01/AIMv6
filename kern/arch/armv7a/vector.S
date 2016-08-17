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

#include <arch-trap.h>

.arm

.section .text.vector

.globl arm_vector

arm_vector:
	ldr	pc, = idle		//Reset
	ldr	pc, = undef_entry	//Undefined
	ldr	pc, = svc_entry		//SVC
	ldr	pc, = pref_abt_entry	//Prefetch Abort
	ldr	pc, = data_abt_entry	//Data Abort
	ldr	pc, = idle		//Reserved
	ldr	pc, = irq_entry		//IRQ
	ldr	pc, = idle		//FIQ

.text

.globl svc_return_asm

undef_entry:
	/* the instruction causing UNDEF is SKIPPED. */
	push	{r0-r12, lr}
	mov	r1, #ARM_UNDEF
	b	trap_entry

svc_entry:
	/* the instruction causing SVC is DONE. */
	push	{r0-r12, lr}
	mov	r1, #ARM_SVC
	b	trap_entry

pref_abt_entry:
	/* the instruction causing ABT is NOT RAN. */
	sub	lr, lr, #4
	push	{r0-r12, lr}
	mov	r1, #ARM_PREF_ABT
	b	trap_entry

data_abt_entry:
	/* the instruction causing ABT is NOT RAN. */
	sub	lr, lr, #8
	push	{r0-r12, lr}
	mov	r1, #ARM_DATA_ABT
	b	trap_entry

irq_entry:
	/* the instruction is DONE. */
	push	{r0-r12, lr}
	mov	r1, #ARM_IRQ
	b	trap_entry

/*
 * general routine
 * sp contains a stack pointer within a struct regs
 * r1 contains a unsigned integer describing trap type
 */
trap_entry:
	/* push SPSR */
	mrs	r0, spsr
	push	{r0}
	/* pass a pointer using r0 */
	mov	r0, sp
	/* restore stack pointer */
	add	sp, sp, #0x3C
	/*
	 * go to system mode and save banked registers
	 * keep IRQ disabled.
	 */
	msr	cpsr_c, 0xDF
	stmdb	r0!, {sp, lr}
	/* if we came from kernel mode, trust the stack */
	ldr	r2, [r0, #8]
	and	r2, r2, #0x1F
	cmp	r2, #0x1F
	beq	trap_entry_j1
	/* else, grab the kernel stack from the current proc */
	mrc	p15, 0, r2, c13, c0, 4 /* struct percpu *this */
	ldr	r2, [r2] /* struct proc *curr */
	ldmia	r2, {r2, r3} /* kstack and kstack_size */
	add	sp, r2, r3

trap_entry_j1:
	/* leave a space for trapframe */
	sub	sp, sp, #TRAPFRAME_SIZE
	mov	fp, sp
	/* save the trapframe */
	mov	r4, r0	/* old trapframe */
	mov	r5, r1	/* type */
	mov	r0, sp
	mov	r1, r4
	bl	arm_copy_trapframe	/* (dest, src) -> dest */
	/* call the C handler */
	mov	r1, r5
	bl	arm_handle_trap
	/* arm_handle_svc(struct regs *, uint32_t) */

idle:
	b	idle

