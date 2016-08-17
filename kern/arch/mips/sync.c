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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <aim/sync.h>
#include <panic.h>
#include <sys/types.h>
#include <mp.h>

void spinlock_init(lock_t *lock)
{
	lock->lock = 0;
	/* Currently as Loongson 3A automatically handles hazards and
	 * consistency, and MSIM is deterministic, we don't care about
	 * barriers here. */
}

void spin_lock(lock_t *lock)
{
	uint32_t inc = 0x10000;
	uint32_t lock_val, new_lock, my_tail, head;

	/*
	 * Locking a ticket lock:
	 * 1. Atomically increment the tail by one.
	 * 2. Wait until the head becomes equal to initial value of the tail.
	 */
	asm volatile (
		"1:	.set	push;"
		"	.set	noreorder;"
		"	ll	%[lock], %[lock_ptr];"
		"	addu	%[new_lock], %[lock], %[inc];"
		"	sc	%[new_lock], %[lock_ptr];"
		"	beqz	%[new_lock], 1b;"
		"	 srl	%[my_tail], %[lock], 16;"
		"	andi	%[head], %[lock], 0xffff;"
		"	beq	%[head], %[my_tail], 9f;"
		"	 nop;"
		"2:	lhu	%[head], %[lock_head_ptr];"
		"	bne	%[head], %[my_tail], 2b;"
		"	 nop;"
		"9:	.set	pop;"
		: [lock_ptr] "+m" (lock->lock),
		  [lock_head_ptr] "+m" (lock->head),
		  [lock] "=&r" (lock_val),
		  [new_lock] "=&r" (new_lock),
		  [my_tail] "=&r" (my_tail),
		  [head] "=&r" (head)
		: [inc] "r" (inc)
	);
	lock->holder = cpuid();
	smp_mb();
}

void spin_unlock(lock_t *lock)
{
	unsigned int head = lock->head + 1;
	smp_mb();
	lock->holder = -1;
	lock->head = (uint16_t)head;
	smp_mb();
}

