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

#include <sys/types.h>

/* This is a ticket lock */
typedef struct {
	union {
		uint32_t lock;
		struct {
			/* Assumes little-endian */
			uint16_t head;
			uint16_t tail;
		};
	};
	int		holder;
} lock_t;

#define EMPTY_LOCK(lk) { .lock = 0, .holder = -1 }

#define smp_mb() \
	asm volatile ("sync" : : : "memory")

#endif

