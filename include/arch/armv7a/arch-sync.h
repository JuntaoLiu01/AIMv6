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

#ifndef _ARCH_SYNC_H
#define _ARCH_SYNC_H

/* low level syncronization implementation, always as macros */

#define ARM_STREX_SUCCESS	0
#define ARM_STREX_FAIL		1
#define ARM_STREX_CORRUPT	-1

#define UNLOCKED	0
#define LOCKED		1

#ifndef __ASSEMBLER__

typedef unsigned int lock_t;
#define EMPTY_LOCK(lock)	(UNLOCKED)

#define SMP_DMB() \
	asm volatile ("dmb")

#define SMP_DSB() \
	asm volatile ("dsb")

#define SMP_ISB() \
	asm volatile ("isb")

#endif /* __ASSEMBLER__ */

#endif /* _ARCH_SYNC_H */

