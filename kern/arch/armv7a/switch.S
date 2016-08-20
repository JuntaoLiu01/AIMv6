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

.globl switch_regs

.arm
.text

/*
 * r0: struct regs *old
 * r1: struct regs *new
 */
switch_regs:
	/* first we save */
	stmia	r0!, {sp, lr}
	mrs	r2, CPSR
	stmia	r0!, {r2}
	stmia	r0!, {r0-r12, pc}
	/* and then we restore */
	ldmia	r1!, {sp, lr}
	ldmia	r1!, {r2}
	msr	CPSR, r2
	ldmia	r1!, {r0-r12, pc}

