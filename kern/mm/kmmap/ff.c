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
#include <list.h>

/*
 * This file implements a first fit algorithm on pages.
 */

#include <aim/console.h>
#include <aim/initcalls.h>
#include <aim/kmmap.h>
#include <mm.h>
#include <vmm.h>
#include <panic.h>

struct block {
	void *addr;
	lsize_t size;
	struct list_head node;
};

static struct list_head __head;

static void *alloc(size_t size)
{
	struct block *this;
	void *ret;

	/* search for a first-fit */
	for_each_entry(this, &__head, node) {
		if (this->size >= size)
			break;
	}
	/* check for failure. there's nothing we can do. */
	if (&this->node == &__head) return NULL;

	/* cut down the block */
	ret = this->addr;
	this->addr += size;
	this->size -= size;

	/* free the block if empty */
	if (this->size == 0) {
		list_del(&this->node);
		kfree(this);
	}

	return ret;
}

static void free(void *pt, size_t size)
{
	struct block *this, *prev = NULL, *tmp, *next = NULL;

	this = kmalloc(sizeof(struct block), 0);
	assert(this != NULL);

	this->addr = pt;
	this->size = size;

	for_each_entry(tmp, &__head, node) {
		if (tmp->addr >= this->addr)
			break;
		prev = tmp;
	}

	if (prev != NULL)
		list_add_after(&this->node, &prev->node);
	else
		list_add_after(&this->node, &__head);

	/* merge downwards */
	if (prev != NULL && prev->addr + prev->size == this->addr) {
		prev->size += this->size;
		list_del(&this->node);
		kfree(this);
		this = prev;
	}

	/* merge upwards */
	if (!list_is_last(&this->node, &__head))
		next = list_next_entry(this, struct block, node);
	if (next != NULL && this->addr + this->size == next->addr) {
		this->size += next->size;
		list_del(&next->node);
		kfree(next);
	}
}

static void init(struct kmmap_keeper *keeper)
{
	struct kmmap_entry *entry;
	struct block *this, *new;

	kprintf("KERN: <kmmap-ff> Initializing.\n");
	/* init list */
	list_init(&__head);
	/* add available address space */
	this = kmalloc(sizeof(*this), 0);
	assert(this != NULL);
	this->addr = (void *)KMMAP_BASE;
	this->size = RESERVED_BASE - KMMAP_BASE;
	list_add_after(&this->node, &__head);
	/* cut off in-use blocks */
	for (
		entry = keeper->next(NULL); entry != NULL; 
		entry = keeper->next(entry)
	) {
		if ((entry->flags & MAP_TYPE_MASK) <= MAP_KERN_MEM)
			continue;
		/* search */
		for_each_entry(this, &__head, node) {
			if (
				this->addr <= entry->vaddr &&
				this->addr + this->size >= entry->vaddr
					+ entry->size
			) break;
		}
		/* 
		 * each in-use block MUST be reserved.
		 * If we can't do that we panic.
		 */
		assert(&this->node != &__head);
		/* lower side */
		if (this->addr < entry->vaddr) {
			new = kmalloc(sizeof(*new), 0);
			new->addr = this->addr;
			new->size = entry->vaddr - this->addr;
			list_add_before(&new->node, &this->node);
		}
		/* upper side */
		if (this->addr + this->size > entry->vaddr + entry->size) {
			new = kmalloc(sizeof(*new), 0);
			new->addr = entry->vaddr + entry->size;
			new->size = this->addr - entry->vaddr + this->size
				- entry->size;
			list_add_after(&new->node, &this->node);
		}
		/* remove the original */
		list_del(&this->node);
		kfree(this);
	}
	kprintf("KERN: <kmmap-ff> Done.\n");
}

static int __init(void)
{

	struct kmmap_allocator allocator = {
		.init	= init,
		.alloc	= alloc,
		.free	= free
	};
	set_kmmap_allocator(&allocator);
	return 0;
}

EARLY_INITCALL(__init)

