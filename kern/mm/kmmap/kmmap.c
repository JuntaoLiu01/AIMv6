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
#include <aim/sync.h>
#include <aim/console.h>
#include <mm.h>
#include <panic.h>

/*
 * The KMMAP interface family has two underlying algorithms instead of one.
 * Allocator manages available address chunks, and keeper records mappings.
 */

/* Dummy implementations */
static void __allocator_init(struct kmmap_keeper *keeper) {}
static void *__alloc(size_t size) { return NULL; }
static void __free(void *pt, size_t size) {}

static void __keeper_init(void) {}
static int __map(struct kmmap_entry *entry) { return EOF; }
static size_t __unmap(void *vaddr) { return 0; }
static int __kva2pa(void *vaddr, addr_t *paddr) { return 0; }
static struct kmmap_entry *__next(struct kmmap_entry *base) { return NULL; }

/* Implementation entrances */
static struct kmmap_allocator __allocator = {
	.init	= __allocator_init,
	.alloc	= __alloc,
	.free	= __free
};

static struct kmmap_keeper __keeper = {
	.init	= __keeper_init,
	.map	= __map,
	.unmap	= __unmap,
	.kva2pa	= __kva2pa,
	.next	= __next
};

/* Registrations */
void set_kmmap_allocator(struct kmmap_allocator *allocator)
{
	__allocator = *allocator;
}

void set_kmmap_keeper(struct kmmap_keeper *keeper)
{
	__keeper = *keeper;
}

/* Subsystem lock */
static lock_t lock = EMPTY_LOCK(lock);

/* Interfaces */
void kmmap_init(void)
{
	__keeper.init();
	__allocator.init(&__keeper);
}

void *kmmap(void *vaddr, addr_t paddr, size_t size, uint32_t flags)
{
	return NULL;
}

size_t kmunmap(void *vaddr)
{
	return 0;
}

int kmmap_apply(pgindex_t *pgindex)
{
	struct kmmap_entry *entry;

	kpdebug("Applying kmmap to: pgindex=0x%08x.\n", pgindex);

	spin_lock(&lock);
	for (
		entry = __keeper.next(NULL); entry != NULL; 
		entry = __keeper.next(entry)
	) {
		int ret = map_pages(pgindex, entry->vaddr, 
			entry->paddr, entry->size, entry->flags);
		if (ret < 0) return EOF;
	}
	return 0;
}

