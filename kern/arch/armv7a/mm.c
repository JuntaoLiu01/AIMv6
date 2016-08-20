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
#include <aim/early_kmmap.h>
#include <aim/export.h>
#include <aim/kmmap.h>
#include <aim/sync.h>
#include <list.h>
#include <util.h>
#include <aim/console.h>
#include <mm.h>
#include <mmu.h>
#include <pmm.h>
#include <vmm.h>
#include <panic.h>
#include <libc/string.h>

pgindex_t *pgdir_slots[MAX_CPUS];

static struct allocator_cache *pt_l1_cache = NULL, *pt_l2_cache = NULL;

bool early_mapping_valid(struct early_mapping *entry)
{
	if (!IS_ALIGNED(entry->paddr, ARM_SECT_SIZE))
		return false;
	if (!IS_ALIGNED((size_t)entry->vaddr, ARM_SECT_SIZE))
		return false;
	if (!IS_ALIGNED(entry->size, ARM_SECT_SIZE))
		return false;
	return true;
}

addr_t get_mem_physbase()
{
	return RAM_PHYSBASE;
}

addr_t get_mem_size()
{
	/* FIXME */
	extern void *RAM_SIZE;
	return (addr_t)(size_t)&RAM_SIZE;
}

/*
 * by CLEARing a page index, caller assumes it contains no allocated space.
 * Often used to initialize a page index.
 * Two functions for clearing two different levels of the page table.
 * First one used by early boot stages, and both are used as allocator
 * callbacks, so they are named differently and have different signature.
 */
void page_index_clear(pgindex_t * index)
{
	memset(index, 0, ARM_PT_L1_SIZE);
}

static void __pt_l2_clear(void *pt)
{
	memset(pt, 0, ARM_PT_L2_SIZE);
}

/* process general flags into ARM-specific flags */
#define convert_flags(flags, ap, tex, c, b, s, xn) \
	do { \
	uint32_t f = (flags); \
	if (f & VMA_WRITE) (ap) = ARM_PT_AP_USER_BOTH; \
	else if (f & VMA_READ) (ap) = ARM_PT_AP_USER_READ; \
	else (ap) = ARM_PT_AP_USER_NONE; \
	if (f & VMA_EXEC) (xn) = 0; \
	else (xn) = 1; \
	f &= MAP_TYPE_MASK; \
	switch(f) { \
		case MAP_USER_MEM: (tex)=1; (c)=1; (b)=1; (s)=1; break; \
		case MAP_KERN_MEM: (tex)=1; (c)=1; (b)=1; (s)=1; (xn)=0; break; \
		case MAP_PRIV_DEV: (tex)=2; (c)=0; (b)=0; (s)=0; break; \
		case MAP_SHARED_DEV: (tex)=0; (c)=0; (b)=1; (s)=0; break; \
		default: (tex)=1; (c)=1; (b)=1; (s)=1; \
	} \
	} while (0)

/* internal routines does not check for bad parameters */
/* does nothing if not mapped */
static inline void __arm_unmap_l1(arm_pte_l1_t *page_table, void *vaddr)
{
	arm_pte_l1_t entry;

	entry = page_table[(size_t)vaddr >> ARM_SECT_SHIFT];
	if ((entry & ARM_PT_L1_TYPE_MASK) == ARM_PT_L1_TABLE) {
		entry &= ARM_PT_L1_TABLE_BASE_MASK;
		cache_free(pt_l2_cache, (void *)entry);
	}
	page_table[(size_t)vaddr >> ARM_SECT_SHIFT] = 0;
}

static inline void __arm_map_sect(arm_pte_l1_t *page_table, addr_t paddr,
	void *vaddr, uint32_t flags)
{
	uint32_t ap, tex, c, b, s, xn;
	arm_pte_l1_t entry;

//	kpdebug("map_sect(pt=0x%08x, paddr=0x%08x, vaddr=0x%08x, flags=0x%08x)\n",
//		page_table, (size_t)paddr, vaddr, flags);
	/* process flags */
	convert_flags(flags, ap, tex, c, b, s, xn);
	/* cleanup */
	__arm_unmap_l1(page_table, vaddr);
	/* apply map */
	entry = ARM_PT_L1_SECT;
	entry |= paddr;
	entry |= s << 16;
	entry |= tex << 12;
	entry |= ap << 10;
	entry |= xn << 4;
	entry |= c << 3;
	entry |= b << 2;
	page_table[(size_t)vaddr >> ARM_SECT_SHIFT] = entry;
}

/* does nothing if there's a table mapped already. */
static inline void __arm_map_table(arm_pte_l1_t *page_table, void *vaddr)
{
	arm_pte_l1_t entry;

	/* check whether we map at all */
	entry = page_table[(size_t)vaddr >> ARM_SECT_SHIFT];
	if ((entry & ARM_PT_L1_TYPE_MASK) == ARM_PT_L1_TABLE)
		return;
	/* allocate a L2 table */
	void *l2 = cache_alloc(pt_l2_cache);
	assert(l2 != NULL);
	/* apply map */
	entry = ARM_PT_L1_TABLE;
	entry |= kva2pa((uint32_t)l2);
	page_table[(size_t)vaddr >> ARM_SECT_SHIFT] = entry;
}

static inline void __arm_map_page(arm_pte_l1_t *page_table, addr_t paddr,
	void *vaddr, uint32_t flags)
{
	uint32_t ap, tex, c, b, s, xn;
	arm_pte_l1_t *t1, e1;
	arm_pte_l2_t *t2, e2;

//	kpdebug("map_page(pt=0x%08x, paddr=0x%08x, vaddr=0x%08x, flags=0x%08x)\n",
//		page_table, (size_t)paddr, vaddr, flags);
	/* process flags */
	convert_flags(flags, ap, tex, c, b, s, xn);
	/* make sure a table is mapped */
	__arm_map_table(page_table, (void *)ALIGN_BELOW((size_t)vaddr, ARM_SECT_SIZE));
	/* get the L2 table */
	t1 = page_table;
	e1 = t1[(size_t)vaddr >> ARM_SECT_SHIFT];
	t2 = (arm_pte_l2_t *)pa2kva(e1 & ARM_PT_L1_TABLE_BASE_MASK);
	/* apply map */
	e2 = ARM_PT_L2_PAGE;
	e2 |= paddr;
	e2 |= s << 10;
	e2 |= tex << 6;
	e2 |= ap << 4;
	e2 |= c << 3;
	e2 |= b << 2;
	e2 |= xn;
	t2[((size_t)vaddr >> ARM_PAGE_SHIFT) & 0xFF] = e2;
}

/*
 * Early map routine does not try to allocate memory.
 */
int page_index_early_map(pgindex_t *index, addr_t paddr, void *vaddr,
	size_t length)
{
	/* alignment check */
	if (IS_ALIGNED(paddr, ARM_SECT_SIZE) &&
	    IS_ALIGNED((size_t)vaddr, ARM_SECT_SIZE) &&
	    IS_ALIGNED(length, ARM_SECT_SIZE) != 1) {
		return EOF;
	}
	/* map each ARM SECT */
	void *vend = vaddr + length;
	while (vaddr < vend) {
		/* Failsafe attributes for early mappings */
		__arm_map_sect(index, paddr, vaddr, MAP_SHARED_DEV | VMA_EXEC);
		paddr += ARM_SECT_SIZE;
		vaddr += ARM_SECT_SIZE;
	}
	return 0;
}

/*
 * mmu_init()
 * initialize the MMU with given page index
 */
int mmu_init(pgindex_t *index)
{
    asm volatile (
        /* Address */
        "mcr     p15, 0, %[index], c2, c0, 0;"
        /* access permission */
        "mov     r0, #0x1;"
        "mcr     p15, 0, r0, c3, c0, 0;"
        /* turn on MMU */
        "mrc     p15, 0, r0, c1, c0, 0;"
        "orr     r0, r0, #0x1;"
        "mcr     p15, 0, r0, c1, c0, 0;"
        ::
        /* For ARM cores, we have low address now, don't translate address. */
        [index] "r" (index)
    );
    return 0;
}

/* add memory chunks to page allocator */
void add_memory_pages(void)
{
	extern uint8_t SYMBOL(kern_end);
	struct pages pages = {
		.paddr = (addr_t)premap_addr((size_t)&SYMBOL(kern_end)),
		.size = get_mem_size() -
			((addr_t)(size_t)(&SYMBOL(kern_end)) - KERN_BASE),
		.flags = GFP_UNSAFE
	};
	free_pages(&pages);
}

/* get_addr_space()
 * determine whether we are running in low address or in high address
 * return values:
 * 0 - low address
 * 1 - high address
 * negative - reserved for errors
 */
int get_addr_space()
{
	uint32_t pc;

	asm volatile (
		"mov	%[pc], pc"
		:[pc] "=r" (pc)
	);
	return (pc > KERN_BASE);
}

void arch_mm_init(void)
{
	/* initialize allocator cache for L1 page tables */
	pt_l1_cache = kmalloc(sizeof(*pt_l1_cache), 0);
	assert(pt_l1_cache != NULL);
	pt_l1_cache->size = ARM_PT_L1_SIZE;
	pt_l1_cache->align = ARM_PT_L1_SIZE;
	pt_l1_cache->flags = 0;
	pt_l1_cache->create_obj = (void *)page_index_clear;
	pt_l1_cache->destroy_obj = (void *)page_index_clear;
	assert(cache_create(pt_l1_cache) == 0);

	/* initialize allocator cache for L2 page tables */
	pt_l2_cache = kmalloc(sizeof(*pt_l2_cache), 0);
	assert(pt_l2_cache != NULL);
	pt_l2_cache->size = ARM_PT_L2_SIZE;
	pt_l2_cache->align = ARM_PT_L2_SIZE;
	pt_l2_cache->flags = 0;
	pt_l2_cache->create_obj = __pt_l2_clear;
	pt_l2_cache->destroy_obj = __pt_l2_clear;
	assert(cache_create(pt_l2_cache) == 0);

	/* SCU, cache and branch predict goes here */
}

pgindex_t *init_pgindex(void)
{
	pgindex_t *index = cache_alloc(pt_l1_cache);
	page_index_clear(index);
	return index;
}

void destroy_pgindex(pgindex_t *pgindex)
{
	arm_pte_l1_t *table = pgindex;
	int i;

	/* free L2 tables (if any) */
	for (i = 0; i < ARM_PT_L1_LENGTH; i += 1) {
		uint32_t type = table[i] & ARM_PT_L1_TYPE_MASK;
		if (type == ARM_PT_L1_TABLE) {
			uint32_t l2table = pa2kva(
				table[i] & ARM_PT_L1_TABLE_BASE_MASK);
			cache_free(pt_l2_cache, (void *)l2table);
		}
	}

	/* free the L1 table itself */
	cache_free(pt_l1_cache, pgindex);
}

int map_pages(pgindex_t *pgindex, void *vaddr, addr_t paddr, size_t size,
    uint32_t flags)
{
	if (pgindex == NULL) return EOF;
	/* make sure we are page aligned */
	if (!(
		IS_ALIGNED((size_t)vaddr, ARM_PAGE_SIZE) &&
		IS_ALIGNED(paddr, ARM_PAGE_SIZE) &&
		IS_ALIGNED(size, ARM_PAGE_SIZE)
	)) return EOF;
	/* apply mappings */
	while (size > 0) {
		if (
			IS_ALIGNED((size_t)vaddr, ARM_SECT_SIZE) &&
			IS_ALIGNED(paddr, ARM_SECT_SIZE) &&
			size >= ARM_SECT_SIZE
		) {
			/* allowed and possible, map a section */
			__arm_map_sect(pgindex, paddr, vaddr, flags);
			vaddr += ARM_SECT_SIZE;
			paddr += ARM_SECT_SIZE;
			size -= ARM_SECT_SIZE;
		} else {
			/* allowed and possible, map a page */
			__arm_map_page(pgindex, paddr, vaddr, flags);
			vaddr += ARM_PAGE_SIZE;
			paddr += ARM_PAGE_SIZE;
			size -= ARM_PAGE_SIZE;
		}
	}
	return 0;
}

#define get_mapped(pt, vaddr, paddr, size) \
	do { \
	arm_pte_l1_t e1 = (pt)[(vaddr) >> ARM_SECT_SHIFT]; \
	arm_pte_l2_t *t2, e2; \
	switch (e1 & ARM_PT_L1_TYPE_MASK) { \
		case ARM_PT_L1_TABLE: \
			t2 = (arm_pte_l2_t *)pa2kva(e1 & ARM_PT_L1_TABLE_BASE_MASK); \
			e2 = t2[(vaddr >> ARM_PAGE_SHIFT) & 0xFF]; \
			(paddr) = e2 & ARM_PT_L2_PAGE_BASE_MASK; \
			(size) = ARM_PAGE_SIZE; \
			break; \
		case ARM_PT_L1_SECT: \
			(paddr) = e1 & ARM_PT_L1_SECT_BASE_MASK; \
			(size) = ARM_SECT_SIZE; \
			break; \
		default: (paddr)=0; (size)=0; \
	} \
	} while (0)

ssize_t unmap_pages(pgindex_t *pgindex, void *vaddr, size_t size, addr_t *paddr)
{
	arm_pte_l1_t *table = pgindex;
	addr_t head_paddr, this_paddr;
	size_t block_size, this_size;

	/* find a start point */
	get_mapped(table, (size_t)vaddr, this_paddr, this_size);
	if (this_size == 0) return 0;
	head_paddr = this_paddr;
	block_size = this_size;
	if (paddr != NULL)
		*paddr = head_paddr;
	while (block_size < size) {
		/* unmap */
		/* increment address */
		vaddr += this_size;
		get_mapped(table, (size_t)vaddr, this_paddr, this_size);
		if (this_size == 0 || this_paddr != head_paddr + block_size) {
			return block_size;
		} else {
			block_size += this_size;
		}
	}
	return block_size;
}

int switch_pgindex(pgindex_t *pgindex)
{
	/* ARM errata 754322 */
	SMP_DSB();
	/* normal routine */
	SMP_ISB();
	asm volatile (
		"mcr	p15, 0, %[addr], c8, c3, 0;"
		"mcr	p15, 0, %[addr], c2, c0, 0;"
        ::
        	[addr] "r" (kva2pa(pgindex))
	);
	SMP_ISB();
	/* ARM errata 754322 */
	SMP_DSB();

	return 0;
}

void *uva2kva(pgindex_t *pgindex, void *uaddr)
{
	addr_t paddr;
	size_t size;
	get_mapped(pgindex, ULCAST(uaddr), paddr, size);
	if (size == 0)
		return NULL;
	else
		return (void *)pa2kva(paddr);
}

