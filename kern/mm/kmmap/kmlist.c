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
#include <aim/kmmap.h>
#include <aim/initcalls.h>
#include <mm.h>
#include <vmm.h>
#include <aim/console.h>
#include <panic.h>
#include <list.h>

/*
 * KMList serves as a kmmap keeper, using a linked list sorted by
 * virual address.
 */

struct kmlist_entry {
	struct list_head node;
	struct kmmap_entry data;
};

static struct list_head head = EMPTY_LIST(head);

static inline bool __overlap(struct kmmap_entry *e1, struct kmmap_entry *e2)
{
	return OVERLAP(
		(size_t)e1->vaddr, e1->size,
		(size_t)e2->vaddr, e2->size
	);
}

static int map(struct kmmap_entry *entry)
{
	struct kmlist_entry *next, *prev, *this;

	if (!IS_ALIGNED(entry->paddr, PAGE_SIZE))
		return EOF;
	if (!IS_ALIGNED((size_t)entry->vaddr, PAGE_SIZE))
		return EOF;
	/* walk the list for the first higher entry */
	for_each_entry(next, &head, node) {
		if (next->data.vaddr > entry->vaddr)
			break;
	}
	/* check for overlap */
	prev = prev_entry(next, node);
	if (!list_is_first(&next->node, &head) && __overlap(&prev->data, entry)) {
		return EOF;
	}
	if (!list_is_last(&prev->node, &head) && __overlap(&next->data, entry)) {
		return EOF;
	}
	/* allocate space and insertion */
	this = kmalloc(sizeof(*this), 0);
	if (this == NULL) return EOF;
	this->data = *entry;
	list_add_before(&this->node, &next->node);

	return 0;
}

static void init(void)
{
	struct early_mapping *old;
	struct kmmap_entry new;

	kprintf("KERN: <kmlist> Initializing.\n");
	/* Go through early kmmap queue */
	for (
		old = early_mapping_next(NULL); old != NULL;
		old = early_mapping_next(old)
	) {
		switch (old->type) {
			case EARLY_MAPPING_MEMORY:
				/* Linear memory */
				new.flags = MAP_KERN_MEM | MAP_LARGE;
				break;
			case EARLY_MAPPING_KMMAP:
				/* Devices */
				new.flags = MAP_SHARED_DEV | MAP_LARGE;
				break;
			case EARLY_MAPPING_TEMP:
				/* Low mappings */
				continue;
			case EARLY_MAPPING_OTHER:
				/* Reserved memory */
				new.flags = MAP_KERN_MEM | MAP_LARGE;
				break;
		}
		new.paddr = old->paddr;
		new.vaddr = old->vaddr;
		new.size = old->size;
		assert(map(&new) == 0);
	}
	kprintf("KERN: <kmlist> Done.\n");
}

static size_t unmap(void *vaddr)
{
	struct kmlist_entry *this;
	size_t size;
	/* walk the list for the first not-lower entry */
	for_each_entry(this, &head, node) {
		if (this->data.vaddr >= vaddr)
			break;
	}
	/* if we get a higher address, the unmap is invalid */
	if (this->data.vaddr > vaddr)
		return 0;
	/* remove from list and free up space */
	size = this->data.size;
	list_del(&this->node);
	kfree(this);
	return size;
}

static int lookup(void *vaddr, addr_t *paddr)
{
	struct kmlist_entry *this;
	/* walk the list for the first not-lower entry */
	for_each_entry(this, &head, node) {
		if (this->data.vaddr >= vaddr)
			break;
	}
	/* if we get a higher address, no result */
	if (this->data.vaddr > vaddr)
		return EOF;
	/* pass paddr if caller asked */
	if (paddr != NULL)
		*paddr = this->data.paddr;
	/* indicate a found mapping */
	return 0;
}

static struct kmmap_entry *next(struct kmmap_entry *base)
{
	struct kmlist_entry *entry;
	if (base == NULL) {
		/* empty or first */
		if (list_empty(&head)) return NULL;
		else entry = list_first_entry(&head, typeof(*entry), node);
	} else {
		entry = container_of(base, typeof(*entry), data);
		if (list_is_last(&entry->node, &head)) return NULL;
		else entry = next_entry(entry, node);
	}
	return &entry->data;
}

static int __init(void)
{
	struct kmmap_keeper this = {
		.init	= init,
		.map	= map,
		.unmap	= unmap,
		.kva2pa	= lookup,
		.next	= next
	};
	set_kmmap_keeper(&this);
	return 0;
}

EARLY_INITCALL(__init)

