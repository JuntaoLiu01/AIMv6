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

#include <mp.h>
#include <percpu.h>
#include <panic.h>
#include <aim/console.h>

/* Saves the *physical* address of slave stacks */
unsigned long slave_stacks[MAX_CPUS];
struct percpu cpus[MAX_CPUS];

extern void arch_smp_startup(void);

static void alloc_slave_stacks(void)
{
	int i;
	struct pages p;

	p.size = KSTACKSIZE;
	p.flags = 0;

	/* Allocate kernel stacks for slave CPUs */
	for (i = 1; i < nr_cpus(); ++i) {
		if (alloc_pages(&p) < 0)
			panic("smp_startup: not enough memory for stacks\n");
		slave_stacks[i] = p.paddr;
		kpdebug("allocated stack at %p\n", slave_stacks[i]);
	}
}

void smp_startup(void)
{
	alloc_slave_stacks();
	/* Arch-specific code */
	arch_smp_startup();
}

int handle_ipi_interrupt(unsigned int msg)
{
	/* Currently we discard other IPIs */
	return 0;
}

