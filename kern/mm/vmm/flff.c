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
#include <sys/param.h>
#include <list.h>

/*
 * This file implements a first fit algorithm on a free list data structure
 * to work as a simple allocator.
 */

#include <mm.h>
#include <pmm.h>
#include <vmm.h>
#include <panic.h>

#include <libc/string.h>

#define ALLOC_ALIGN 16

/* This header directly leads the payload */
struct blockhdr {
	size_t size;	/* size of header AND payload */
	bool free;
	gfp_t flags;
	struct list_head node;
	char __padding[12];/* pad to 16-byte alignment */
};

#define PAYLOAD(bh)		((void *)((struct blockhdr *)(bh) + 1))
#define HEADER(payload)		((struct blockhdr *)(payload) - 1)

static struct list_head __bootstrap_head;
static struct list_head __head;

/*
 * TODO: we probably need to explain the algorithm and data structure here
 */

static inline void *__alloc(struct list_head *head, size_t size, gfp_t flags)
{
	struct blockhdr *this;
	size_t allocsize, newsize;

	/* Make a good size */
	size = ALIGN_ABOVE(size, ALLOC_ALIGN);
	allocsize = size + sizeof(struct blockhdr);

#if 0
	{
		struct blockhdr *this;
		kpdebug("ALLOC: kmalloc: before alloc: free list begin from head %p\n", head);
		for_each_entry (this, head, node) {
			kpdebug("ALLOC: kmalloc: this:%p size:%ld(%ld) free:%d flags:%x next:%p prev:%p\n",
			    this, this->size, this->size - sizeof(struct blockhdr), this->free, this->flags, this->node.next, this->node.prev);
		}
		kpdebug("ALLOC: kmalloc: before alloc: free list end\n");
	}
#endif

	/* Search for a first fit */
	for_each_entry(this, head, node) {
		if (this->size >= allocsize) {
			break;
		}
	}

	/* No fit found, ask for pages */
	if (&this->node == head) {
		struct pages pages = {
			.paddr = 0,
			.size = ALIGN_ABOVE(size, PAGE_SIZE),
			.flags = flags
		};
		struct blockhdr *tmp;
		if (alloc_pages(&pages) == EOF) {
			/* really out of memory now */
			return NULL;
		}

		/* make a block out of the new page */
		this = (struct blockhdr *)(size_t)pa2kva((size_t)pages.paddr);
		this->size = pages.size;
		this->free = true;
		this->flags = flags;

		/* add to free block list */
		for_each_entry(tmp, head, node) {
			if (tmp >= this) { break; }
		}
		list_add_before(&this->node, &tmp->node);
	}

	/* found a fit, so we cut it down */
	newsize = this->size - allocsize;
	if (newsize >= sizeof(struct blockhdr) + ALLOC_ALIGN) {
		struct blockhdr *newblock = ((void *)this) + allocsize;
		newblock->size = newsize;
		newblock->free = true;
		newblock->flags = this->flags;
		this->size = allocsize;
		list_add_after(&newblock->node, &this->node);
	}
	this->free = false;
	list_del(&this->node);
#if 0
	{
		struct blockhdr *this;
		kpdebug("ALLOC: kmalloc: after alloc: free list begin from head %p\n", head);
		for_each_entry (this, head, node) {
			kpdebug("ALLOC: kmalloc: this:%p size:%ld(%ld) free:%d flags:%x next:%p prev:%p\n",
			    this, this->size, this->size - sizeof(struct blockhdr), this->free, this->flags, this->node.next, this->node.prev);
		}
		kpdebug("ALLOC: kmalloc: after alloc: free list end\n");
	}
#endif
	return PAYLOAD(this);
}

static inline void __free(struct list_head *head, void *obj)
{
	struct blockhdr *this, *tmp;

	this = HEADER(obj);
	/* Fill the object with junk */
	if (!(this->flags & GFP_UNSAFE))
		memset(obj, JUNKBYTE, this->size - sizeof(*this));

	this->free = true;

#if 0
	{
		struct blockhdr *this;
		kpdebug("ALLOC: kfree: before free: free list begin from head %p\n", head);
		for_each_entry (this, head, node) {
			kpdebug("ALLOC: kfree: this:%p size:%ld(%ld) free:%d flags:%x next:%p prev:%p\n",
			    this, this->size, this->size - sizeof(struct blockhdr), this->free, this->flags, this->node.next, this->node.prev);
		}
		kpdebug("ALLOC: kfree: before free: free list end\n");
	}
#endif
	/* insert to list */
	for_each_entry(tmp, head, node) {
		if (tmp >= this) { break; }
	}
	list_add_before(&this->node, &tmp->node);

	/* merge downwards */
	tmp = list_entry(this->node.prev, struct blockhdr, node);
	if (
		&tmp->node != head && /* prev exist */
		(void *)tmp + tmp->size == (void *)this /* direct neighbor */
	) {
		tmp->size += this->size;
		list_del(&this->node);
		this = tmp;
	}

	/* merge upwards */
	tmp = list_entry(this->node.next, struct blockhdr, node);
	if (
		&tmp->node != head && /* next exist */
		(void *)this + this->size == (void *)tmp /* direct neighbor */
	) {
		this->size += tmp->size;
		list_del(&tmp->node);
	}

	/*
	 * return pages:
	 * all blocks across page border are larger than a page, thus remaining
	 * slices after we return pages (if there are any) are capable for
	 * allocation (PAYLOAD >= ALLOC_ALIGN).
	 */
	size_t end = (size_t)this + this->size;
	size_t first_border = ALIGN_ABOVE((size_t)this, PAGE_SIZE);
	size_t last_border = ALIGN_BELOW(end, PAGE_SIZE);
	if (first_border < last_border) {
		struct pages pages = {
			.paddr = (addr_t)kva2pa((void *)first_border),
			.size = (addr_t)(last_border - first_border),
			.flags = this->flags
		};
		/* upper slice */
		if (last_border < end) {
			tmp = (struct blockhdr *)last_border;
			tmp->size = end - last_border;
			tmp->free = true;
			tmp->flags = this->flags;
			list_add_after(&tmp->node, &this->node);
		}
		/* lower slice */
		if (first_border > (size_t)this) {
			this->size = first_border - (size_t)this;
		} else {
			list_del(&this->node);
		}
		/* really free pages */
		free_pages(&pages);
	}
#if 0
	{
		struct blockhdr *this;
		kpdebug("ALLOC: kfree: after free: free list begin from head %p\n", head);
		for_each_entry (this, head, node) {
			kpdebug("ALLOC: kfree: this:%p size:%ld(%ld) free:%d flags:%x next:%p prev:%p\n",
			    this, this->size, this->size - sizeof(struct blockhdr), this->free, this->flags, this->node.next, this->node.prev);
		}
		kpdebug("ALLOC: kfree: after free: free list end\n");
	}
#endif
}

static size_t __size(void *obj)
{
	struct blockhdr *this;

	this = HEADER(obj);

	return this->size - sizeof(struct blockhdr);
}

static void *__bootstrap_alloc(size_t size, gfp_t flags)
{
	return __alloc(&__bootstrap_head, size, flags);
}

static void *__proper_alloc(size_t size, gfp_t flags)
{
	return __alloc(&__head, size, flags);
}

static void __bootstrap_free(void *obj)
{
	__free(&__bootstrap_head, obj);
}

static void __proper_free(void *obj)
{
	__free(&__head, obj);
}

int simple_allocator_bootstrap(void *pt, size_t size)
{
	struct blockhdr *block = pt;
	block->size = size;
	block->free = true;
	block->flags = 0;
	list_init(&__bootstrap_head);
	list_add_after(&block->node, &__bootstrap_head);

	struct simple_allocator allocator = {
		.alloc	= __bootstrap_alloc,
		.free	= __bootstrap_free,
		.size	= __size
	};
	set_simple_allocator(&allocator);
	return 0;
}

int simple_allocator_init(void)
{
	list_init(&__head);

	struct simple_allocator allocator = {
		.alloc	= __proper_alloc,
		.free	= __proper_free,
		.size	= __size
	};
	set_simple_allocator(&allocator);
	return 0;
}

