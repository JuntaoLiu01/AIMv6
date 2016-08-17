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

#ifndef _ARCH_REGS_H
#define _ARCH_REGS_H

#define ARM_IRQ_MASK	0x80
#define ARM_FIQ_MASK	0x40
#define ARM_INTERRUPT_MASK	(ARM_IRQ_MASK | ARM_FIQ_MASK)

#define ARM_MODE_MASK	0x1F
#define ARM_MODE_USR	0x10
#define ARM_MODE_FIQ	0x11
#define ARM_MODE_IRQ	0x12
#define ARM_MODE_SVC	0x13
#define ARM_MODE_MON	0x16
#define ARM_MODE_ABT	0x17
#define ARM_MODE_UND	0x1B
#define ARM_MODE_SYS	0x1F

#ifndef __ASSEMBLER__

struct regs {
	/* possible extra registers here */
	uint32_t sp;
	uint32_t lr;
	uint32_t psr;
	uint32_t r0;
	uint32_t r1;
	uint32_t r2;
	uint32_t r3;
	uint32_t r4;
	uint32_t r5;
	uint32_t r6;
	uint32_t r7;
	uint32_t r8;
	uint32_t r9;
	uint32_t r10;
	uint32_t r11;
	uint32_t r12;
	uint32_t pc;
};

#define arm_read_psr(psr) \
({ \
	register uint32_t tmp; \
	asm ( \
		"mrs	%[tmp], " #psr ";" \
		: [tmp] "=r" (tmp) \
	); \
	tmp; \
})

#define arm_write_psr(psr, val) \
	do { \
		register uint32_t tmp = (val); \
		asm ( \
			"msr	" #psr ", %[tmp];" \
			: /* no output */ \
			: [tmp] "r" (tmp) \
			: "cc" \
		); \
	} while (0)

#define arm_read_cpsr() arm_read_psr(cpsr)

#endif	/* !__ASSEMBLER__ */

#endif /* _ARCH_REGS_H */

