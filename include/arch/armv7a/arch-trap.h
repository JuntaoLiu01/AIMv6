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

#ifndef _ARCH_TRAP_H
#define _ARCH_TRAP_H

#include <regs.h>

#define ARM_RST		0
#define ARM_UNDEF	1
#define ARM_SVC		2
#define ARM_PREF_ABT	3
#define ARM_DATA_ABT	4
/* reserved vector slot */
#define	ARM_IRQ		6
#define ARM_FIQ		7

#define TRAPFRAME_SIZE	0x44

#ifndef __ASSEMBLER__

struct trapframe {
	struct regs;
};

static inline bool from_kernel(struct trapframe *tf)
{
	/* USR mode is user, everything else is kernel. */
	return ((tf->psr & ARM_MODE_MASK) != ARM_MODE_USR);
}

__noreturn
void trap_return(struct trapframe *tf);

#endif	/* !__ASSEMBLER__ */

#endif /* _ARCH_TRAP_H */

