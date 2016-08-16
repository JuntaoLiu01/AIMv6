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

/* from kernel */
#include <sys/types.h>
#include <aim/console.h>
#include <mm.h>
#include <pmm.h>
#include <vmm.h>
#include <trap.h>
#include <panic.h>
#include <init.h>
#include <proc.h>
#include <percpu.h>
#include <sched.h>
#include <mp.h>
#include <timer.h>
#include <aim/initcalls.h>
#include <aim/irq.h>
#include <aim/kmmap.h>

#define BOOTSTRAP_POOL_SIZE	1024

static volatile bool percpu_blocked = true;

static void __noreturn rest_percpu_init(void)
{
	local_irq_enable();

	for (;;)
		/* nothing */;
}

/*
 * Initialization routine after everything before spawning initproc.
 *
 * rest_init() spawns processes and enables scheduling.
 */
static void __noreturn rest_init(void)
{
	spawn_initproc();
	percpu_blocked = false;
	rest_percpu_init();
}

void __noreturn master_init(void)
{
	__attribute__ ((aligned(16)))
	uint8_t bootstrap_pool[BOOTSTRAP_POOL_SIZE];

	jump_handlers_apply();
	kputs("KERN: We are in high address.\n");

	arch_init();

	/*
	 * Page allocator requires arbitrary size allocation to allocate
	 * struct pages, while arbitrary size allocation depends on
	 * page allocator to actually give out memory.
	 *
	 * We break such circular dependency by
	 * (1) bootstrap a small virtual memory allocator which works on the
	 *     stack.
	 * (2) initialize a page allocator which works on the bootstrap
	 *     allocator obtained in (1).
	 * (3) initialize a real virtual memory allocator which depend
	 *     on (2).
	 * (4) make the page allocator depend on (3) instead.
	 *
	 * TODO: move the following piece of code to kern/mm
	 */
	simple_allocator_bootstrap(bootstrap_pool, BOOTSTRAP_POOL_SIZE);
	kputs("KERN: Simple allocator bootstrapping.\n");
	page_allocator_init();
	kputs("KERN: Page allocator initialized.\n");
	add_memory_pages();
	kputs("KERN: Pages added.\n");
	kprintf("KERN: Free memory: 0x%p\n", (size_t)get_free_memory());
	struct simple_allocator old;
	get_simple_allocator(&old);
	simple_allocator_init();
	kputs("KERN: Simple allocator initialized.\n");
	page_allocator_move(&old);
	kputs("KERN: Page allocator moved.\n");

	trap_init();
	kputs("KERN: Traps initialized.\n");

	/* temporary test */
	extern void trap_test(void);
	trap_test();

	kputs("KERN: Traps test passed.\n");

	/* do early initcalls, one by one */
	do_early_initcalls();

	mm_init();
	kputs("KERN: Memory management component initialized.\n");

	extern void mm_test(void);
	mm_test();

	/* init kernel mapping management */
	kmmap_init();

	proc_init();
	sched_init();
	kputs("KERN: Scheduler initialized.\n");

	/* do initcalls, one by one */
	do_initcalls();
	kputs("KERN: Initcalls done.\n");

	/* temporary tests */
	struct allocator_cache cache = {
		.size = 1024,
		.align = 1024,
		.flags = 0,
		.create_obj = NULL,
		.destroy_obj = NULL
	};
	cache_create(&cache);
	void *a, *b, *c;
	a = cache_alloc(&cache);
	kpdebug("a = 0x%08x\n", a);
	b = cache_alloc(&cache);
	kpdebug("b = 0x%08x\n", b);
	c = cache_alloc(&cache);
	kpdebug("c = 0x%08x\n", c);
	cache_free(&cache, a);
	cache_free(&cache, b);
	cache_free(&cache, c);
	a = cache_alloc(&cache);
	kpdebug("a = 0x%08x\n", a);
	cache_free(&cache, a);
	int ret = cache_destroy(&cache);
	kpdebug("cache_destroy returned %d.\n", ret);
	cache_create(&cache);
	a = cache_alloc(&cache);
	kpdebug("a = 0x%08x\n", a);

	probe_devices();
	kprintf("KERN: probe device done\n");

	init_IRQ();

	/* startup smp */
	//smp_startup();

	/* initialize or cleanup namespace */

	idle_init();
	timer_init();

	rest_init();
}

void __noreturn slave_init(void)
{
	kprintf("KERN CPU %d: init\n", cpuid());

	idle_init();
	timer_init();

	while (percpu_blocked)
		/* nothing */;

	rest_percpu_init();
}

