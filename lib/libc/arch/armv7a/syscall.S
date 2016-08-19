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

#include <libc/syscalls.h>
#include <util.h>

#ifdef ARM_OABI

/* We don't want to mess with OABI right now. */

#else /* ARM_EABI */

/*
 * The arguments are already organized by C compiler in r0-r3 and on stack.
 * Linux system call convention wants arguments in r0-r6 and stack,
 * and uses r7 as syscall number, so we preserve and then fill r4-r7.
 * We leave r8-r11 untouched and use r12(ip) if needed.
 * As per Linux system call convention, we trigger a swi #0,
 * but write it as svc #0 according to armv7a manuals. SWI and SVC are
 * names for the same instruction.
 *
 * Upon return, r0 will have the return value.
 * Just like linux, AIMv6 does NOT return 64bit values on 32bit systems.
 *
 * We do not create a stack frame here.
 */
#define __empty

#define DEFINE_SYSCALL(name) \
.globl	name; \
name: \
	mov	ip, sp; \
	stmdb	ip!, {r4-r7}; \
	pop	{r4-r6}; \
	ldr	r7, =NRSYS_##name; \
	svc	#0; \
	sub	sp, sp, #12; \
	ldm	ip, {r4-r7}; \
	cmp	r0, #0; \
	bge	1f; \
	push	{lr}; \
	bl	__set_errno; \
	pop	{pc}; \
1:	bx	lr;

#endif

#include <libc/syscall-list.S>

