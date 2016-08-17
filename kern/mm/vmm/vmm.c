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
#include <aim/sync.h>

#include <vmm.h>
#include <mm.h>	/* PAGE_SIZE */
#include <panic.h>

#include <libc/string.h>

#include <mp.h>

/* dummy implementations */
static void *__simple_alloc(size_t size, gfp_t flags) { return NULL; }
static void __simple_free(void *obj) {}
static size_t __simple_size(void *obj) { return 0; }

static struct simple_allocator __simple_allocator = {
	.alloc	= __simple_alloc,
	.free	= __simple_free,
	.size	= __simple_size
};

void *kmalloc(size_t size, gfp_t flags)
{
	unsigned long intr_flags;
	void *result;

	if (size > PAGE_SIZE / 2)
		panic("kmalloc: size %lu too large\n", size);
	//kpdebug("ALLOC: kmalloc request %d bytes\n", size);
	recursive_lock_irq_save(&memlock, intr_flags);
	result = __simple_allocator.alloc(size, flags);
	recursive_unlock_irq_restore(&memlock, intr_flags);
	if (flags & GFP_ZERO)
		memset(result, 0, size);
	//kpdebug("ALLOC: kmalloc returning %p (%d bytes)\n", result, size);
	return result;
}

void kfree(void *obj)
{
	unsigned long flags;
	if (obj != NULL) {
		//kpdebug("ALLOC: kfree requesting %p (%d bytes)\n", obj, ksize(obj));
		recursive_lock_irq_save(&memlock, flags);
		/* Junk filling is in flff.c since we need the gfp flags */
		__simple_allocator.free(obj);
		recursive_unlock_irq_restore(&memlock, flags);
		//kpdebug("ALLOC: kfree freed %p\n", obj);
	}
}

size_t ksize(void *obj)
{
	if (obj != NULL)
		return __simple_allocator.size(obj);
	else
		return 0;
}

void set_simple_allocator(struct simple_allocator *allocator)
{
	if (allocator == NULL)
		return;
	memcpy(&__simple_allocator, allocator, sizeof(*allocator));
}

void get_simple_allocator(struct simple_allocator *allocator)
{
	if (allocator == NULL)
		return;
	memcpy(allocator, &__simple_allocator, sizeof(*allocator));
}

/* dummy implementation again */
static int __caching_create(struct allocator_cache *cache) { return EOF; }
static int __caching_destroy(struct allocator_cache *cache) { return EOF; }
static void *__caching_alloc(struct allocator_cache *cache) { return NULL; }
static int __caching_free(struct allocator_cache *cache, void *obj)
{ return EOF; }
static void __caching_trim(struct allocator_cache *cache) {}

struct caching_allocator __caching_allocator = {
	.create		= __caching_create,
	.destroy	= __caching_destroy,
	.alloc		= __caching_alloc,
	.free		= __caching_free,
	.trim		= __caching_trim
};

void set_caching_allocator(struct caching_allocator *allocator)
{
	if (allocator == NULL)
		return;
	memcpy(&__caching_allocator, allocator, sizeof(*allocator));
}

int cache_create(struct allocator_cache *cache)
{
	if (cache == NULL)
		return EOF;
	assert(cache->size < PAGE_SIZE / 2);
	spinlock_init(&cache->lock);
	//kpdebug("ALLOC: cache_create %p (%d bytes)\n", cache, cache->size);
	return __caching_allocator.create(cache);
}

int cache_destroy(struct allocator_cache *cache)
{
	unsigned long flags;

	if (cache == NULL)
		return EOF;
	//kpdebug("ALLOC: cache_destroy %p (%d bytes)\n", cache, cache->size);
	spin_lock_irq_save(&cache->lock, flags);
	int retval = __caching_allocator.destroy(cache);
	spin_unlock_irq_restore(&cache->lock, flags);
	return retval;
}

void *cache_alloc(struct allocator_cache *cache)
{
	unsigned long flags;
	if (cache == NULL)
		return NULL;
	//kpdebug("ALLOC: cache_alloc requesting %d bytes in %p\n", cache->size, cache);
	spin_lock_irq_save(&cache->lock, flags);
	void *retval = __caching_allocator.alloc(cache);
	spin_unlock_irq_restore(&cache->lock, flags);
	if (cache->flags & GFP_ZERO)
		memset(retval, 0, cache->size);
	//kpdebug("ALLOC: cache_alloc return %p (%d bytes in %p)\n", retval, cache->size, cache);
	return retval;
}

int cache_free(struct allocator_cache *cache, void *obj)
{
	unsigned long flags;
	if (cache == NULL)
		return EOF;
	if (!(cache->flags & GFP_UNSAFE))
		memset(obj, JUNKBYTE, cache->size);
	//kpdebug("ALLOC: cache_free requesting %p (%d bytes in %p)\n", obj, cache->size, cache);
	spin_lock_irq_save(&cache->lock, flags);
	int retval = __caching_allocator.free(cache, obj);
	spin_unlock_irq_restore(&cache->lock, flags);
	//kpdebug("ALLOC: cache_free freed %p\n", obj);
	return retval;
}

void cache_trim(struct allocator_cache *cache)
{
	unsigned long flags;
	if (cache == NULL)
		return;
	spin_lock_irq_save(&cache->lock, flags);
	__caching_allocator.trim(cache);
	spin_unlock_irq_restore(&cache->lock, flags);
}

