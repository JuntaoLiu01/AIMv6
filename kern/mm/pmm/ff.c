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

#include <mm.h>
#include <pmm.h>
#include <vmm.h>
#include <panic.h>

struct block {
	struct pages;
	struct list_head node;
};

static struct list_head __head;
static addr_t __free_space;

static int __alloc(struct pages *pages)
{
	struct block *this;

	/* check size alignment */
	if (!IS_ALIGNED(pages->size, PAGE_SIZE))
		return 0;
	if (pages->size > __free_space)
		return 0;

	/* search for a first-fit */
	for_each_entry(this, &__head, node) {
		if (this->size >= pages->size)
			break;
	}
	/* upon failure, @pages remains untouched. */
	if (&this->node == &__head) return EOF;

	/* copy the @pages onto the stack for modification */

	/* cut down the block */
	pages->paddr = this->paddr;
	this->paddr += pages->size;
	this->size -= pages->size;

	/* free the block if empty */
	if (this->size == 0) {
		list_del(&this->node);
		kfree(this);
	}

	/* decrease available memory amount */
	__free_space -= pages->size;

	return 0;
}

static void __free(struct pages *pages)
{
	struct block *this, *prev = NULL, *tmp, *next = NULL;

	if (!IS_ALIGNED(pages->paddr, PAGE_SIZE))
		return;

	if (!IS_ALIGNED(pages->size, PAGE_SIZE))
		return;

	this = kmalloc(sizeof(struct block), 0);
	if (this == NULL)
		panic("Out of memory during free_pages().\n");
	this->paddr = pages->paddr;
	this->size = pages->size;
	this->flags = pages->flags;

	for_each_entry(tmp, &__head, node) {
		if (tmp->paddr >= this->paddr)
			break;
		prev = tmp;
	}

	if (prev != NULL)
		list_add_after(&this->node, &prev->node);
	else
		list_add_after(&this->node, &__head);

	/* merge downwards */
	if (prev != NULL && prev->paddr + prev->size == this->paddr) {
		prev->size += this->size;
		list_del(&this->node);
		kfree(this);
		this = prev;
	}

	/* merge upwards */
	if (!list_is_last(&this->node, &__head))
		next = list_next_entry(this, struct block, node);
	if (next != NULL && this->paddr + this->size == next->paddr) {
		this->size += next->size;
		list_del(&next->node);
		kfree(next);
	}
	__free_space += pages->size;
}

static addr_t __get_free(void)
{
	return __free_space;
}

int page_allocator_init(void)
{
	__free_space = 0;
	list_init(&__head);

	struct page_allocator allocator = {
		.alloc		= __alloc,
		.free		= __free,
		.get_free 	= __get_free
	};
	set_page_allocator(&allocator);
	return 0;
}

int page_allocator_move(struct simple_allocator *old)
{
	struct block *this, *new;
	for_each_entry(this, &__head, node) {
		new = kmalloc(sizeof(struct block), 0);
		if (new == NULL)
			panic("Out of memory during page_allocator_move().\n");
		new->paddr = this->paddr;
		new->size = this->size;
		list_add_after(&new->node, &this->node);
		list_del(&this->node);
		old->free(this);
		this = new;
	}
	return 0;
}

