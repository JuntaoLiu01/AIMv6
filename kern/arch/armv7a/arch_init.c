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

#include <sys/types.h>
#include <aim/early_kmmap.h>
#include <init.h>
#include <mm.h>
#include <drivers/io/io-mem.h>

unsigned long kernelsp[MAX_CPUS];

void early_arch_init(void)
{
	io_mem_init(&early_memory_bus);
	early_mach_init();

	/* extra low address mapping */
	addr_t mem_base = get_mem_physbase();
	addr_t mem_size = get_mem_size();
	struct early_mapping desc = {
		.paddr = mem_base,
		.vaddr = (void *)(size_t)mem_base,
		.size = (size_t)mem_size,
		.type = EARLY_MAPPING_TEMP
	};
	early_mapping_add(&desc);

	/* [Gan] moved from kern/init/early_init.c */
	early_mapping_add_memory(
		get_mem_physbase(),
		(size_t)get_mem_size());
}

void arch_init(void)
{
	mach_init();
}

/* TODO */
#include <mm.h>
void timer_init(void)
{

}

pgindex_t *get_pgindex(void)
{
	return NULL;
}

int set_pages_perm(pgindex_t *pgindex, void *vaddr, size_t size, uint32_t flags)
{
	return -1;
}

#include <regs.h>
void switch_regs(struct regs *old, struct regs *new)
{

}

void arch_smp_startup(void)
{

}

#include <proc.h>
int __mach_setup_default_tty(struct tty_device *a, int b, struct proc *c)
{
	return -1;
}

void panic_other_cpus(void)
{

}

