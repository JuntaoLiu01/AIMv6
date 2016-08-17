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

#ifndef _ARCH_IRQ_H
#define _ARCH_IRQ_H

#include <sys/types.h>
#include <regs.h>

/*
 * This file contains ASM statements without the VOLATILE modifier.
 * Those ASM statements are carefully written so that they can be
 * optimized by the compiler.
 * We break down larger ASM blocks into small and readable macros
 * at the same time.
 */

#ifndef __ASSEMBLER__

static inline void local_irq_enable()
{
	uint32_t cpsr = arm_read_psr(cpsr);
	/* set to 0 to enable */
	cpsr &= ~ARM_INTERRUPT_MASK;
	arm_write_psr(cpsr_c, cpsr);
}

static inline void local_irq_disable()
{
	uint32_t cpsr = arm_read_psr(cpsr);
	/* set to 1 to disable */
	cpsr |= ARM_INTERRUPT_MASK;
	arm_write_psr(cpsr_c, cpsr);
}

#define local_irq_save(flags) \
	do { \
		uint32_t cpsr = arm_read_psr(cpsr); \
		(flags) = cpsr & ARM_INTERRUPT_MASK; \
		cpsr |= ARM_INTERRUPT_MASK; \
		arm_write_psr(cpsr_c, cpsr); \
	} while (0)

#define local_irq_restore(flags) \
	do { \
		uint32_t cpsr = arm_read_psr(cpsr); \
		cpsr &= ~ARM_INTERRUPT_MASK; \
		cpsr |= flags; \
		arm_write_psr(cpsr_c, cpsr); \
	} while (0)

#endif /* !__ASSEMBLER__ */

#endif /* _ARCH_IRQ_H */

