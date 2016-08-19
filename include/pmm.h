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

#ifndef _PMM_H
#define _PMM_H

#include <sys/types.h>
#include <vmm.h>
#include <mmu.h>
#include <gfp.h>

#ifndef __ASSEMBLER__

/* currently ignored */

struct pages {
	addr_t paddr;
	lsize_t size;
	gfp_t flags;
};

struct page_allocator {
	int (*alloc)(struct pages *pages);
	void (*free)(struct pages *pages);
	addr_t (*get_free)(void);
};

struct simple_allocator;	/* FIXME ??? */

int page_allocator_init(void);
int page_allocator_move(struct simple_allocator *old);
void set_page_allocator(struct page_allocator *allocator);
/* The registration above COPIES the struct. */

/* 
 * This interface may look wierd, but it prevents the page allocator from doing
 * any kmalloc-like allocation: it either breaks a block or remove a block upon
 * page allocation.
 * Returns 0 for success and EOF for failure.
 */
int alloc_pages(struct pages *pages);
int alloc_aligned_pages(struct pages *pages, lsize_t align);
void free_pages(struct pages *pages);
addr_t get_free_memory(void);

/* Returns -1 on error */
static inline addr_t pgalloc(void)
{
	struct pages p;
	p.size = PAGE_SIZE;
	p.flags = 0;
	if (alloc_pages(&p) != 0)
		return -1;
	return p.paddr;
}

static inline void pgfree(addr_t paddr)
{
	struct pages p;
	p.paddr = paddr;
	p.size = PAGE_SIZE;
	p.flags = 0;
	free_pages(&p);
}

/*
 * To comply with kmmap() in future, call this function in case of filling
 * contiguous physical pages.
 */
void pmemset(addr_t paddr, unsigned char b, lsize_t size);


/* initialize the page-block structure for remaining free memory */
void add_memory_pages(void);

#endif /* !__ASSEMBLER__ */

#endif /* _PMM_H */

