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

#ifndef _TRAP_H
#define _TRAP_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

struct trapframe;

void trap_init(void);

__noreturn
void trap_return(struct trapframe *regs);

void handle_syscall(struct trapframe *tf);
int handle_interrupt(struct trapframe *tf);

/* mach-specific */
void enable_ipi_interrupt(void);

#endif /* _TRAP_H */

