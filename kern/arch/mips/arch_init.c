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

#include <init.h>
#include <aim/console.h>
#include <drivers/io/io-mem.h>

unsigned long kernelsp[MAX_CPUS];

void abs_jump(void *addr)
{
	asm volatile (
		"move	$25, %0;"
		"jr	%0"
		: /* no output */
		: "r"(addr)
	);
}

void early_arch_init(void)
{
	io_mem_init(&early_memory_bus);
	early_mach_init();
}

void arch_init(void)
{
	mach_init();
}

