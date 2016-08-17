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

#ifndef _ARCH_SMP_H
#define _ARCH_SMP_H

#ifndef __ASSEMBLER__

#include <sys/types.h>

#define arm_read_mpidr() \
({ \
	register uint32_t ret; \
	asm ( \
		"mrc	p15, 0, %[ret], c0, c0, 5;" \
		: [ret] "=r" (ret) \
	); \
	ret; \
})

static inline unsigned int __cpuid(void)
{
	uint32_t mpidr = arm_read_mpidr();
	/* don't attempt to bring up SMP if there's old format */
	if ((mpidr & 0x80000000) == 0x0) return 0;
	/* uniprocessor */
	if ((mpidr & 0x40000000) == 0x40000000) return 0;
	uint32_t cluster_id = (mpidr & 0xF00) >> 8;
	uint32_t core_id = mpidr & 0x3;
	return (cluster_id << 2) | (core_id);
}

#define cpuid()		__cpuid()

#else	/* __ASSEMBLER__ */
#ifndef __LDSCRIPT__
/*
 * The header is included by an assembly header/source.
 */
	.macro	arm_read_mpidr result
	mrc	p15, 0, \result, c0, c0, 5
	.endm

	.macro	cpuid result, tmp
	mrc	p15, 0, \result, c0, c0, 5
	/* New format? */
	mov	\tmp, #0x80000000
	ands	\tmp, \result, \tmp
	/* 0 for old format */
	beq	1f
	/* MP system? */
	mov	\tmp, #0x40000000
	ands	\tmp, \result, \tmp
	/* 1 for uniprocessor */
	bne	1f
	/* calculate 4-bit cluster number and 2-bit core number */
	mov	\tmp, #0x00000F00
	and	\tmp, \result, \tmp
	mov	\tmp, \tmp, lsr #6
	and	\result, #0x00000003
	or	\result, \result, \tmp
	.exitm
1:
	mov	\result, #0x0
	.endm

#endif /* !__LDSCRIPT__ */
#endif /* !__ASSEMBLER__ */

#ifndef __ASSEMBLER__
#include <mmu.h>
extern unsigned long kernelsp[MAX_CPUS];
#define current_kernelsp	kernelsp[cpuid()]
#define current_pgdir		pgdir_slots[cpuid()]
#endif	/* !__ASSEMBLER__ */

#endif /* _ARCH_SMP_H */

