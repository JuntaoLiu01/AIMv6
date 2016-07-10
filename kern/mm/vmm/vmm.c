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
#include <aim/sync.h>

#include <vmm.h>

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
static lock_t __kmalloc_lock = EMPTY_LOCK(__kmalloc_lock);

void *kmalloc(size_t size, gfp_t flags)
{
	unsigned long intr_flags;

	if (size > PAGE_SIZE / 2)
		panic("kmalloc: size %ld too big\n", size);

	spin_lock_irq_save(&__kmalloc_lock, intr_flags);
	kprintf("ALLOC: #%d kmalloc request %u bytes\n", cpuid(), size);
	void *ret = __simple_allocator.alloc(size, flags);
	kprintf("ALLOC: #%d kmalloc return %p (%u bytes)\n", cpuid(), ret, size);
	spin_unlock_irq_restore(&__kmalloc_lock, intr_flags);
	return ret;
}

void kfree(void *obj)
{
	unsigned long flags;

	if (obj != NULL) {
		spin_lock_irq_save(&__kmalloc_lock, flags);
		kprintf("ALLOC: #%d kfree request %p\n", cpuid(), obj);
		__simple_allocator.free(obj);
		kprintf("ALLOC: #%d kfree freed %p\n", cpuid(), obj);
		spin_unlock_irq_restore(&__kmalloc_lock, flags);
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
	return __caching_allocator.create(cache);
}

int cache_destroy(struct allocator_cache *cache)
{
	unsigned long flags;

	if (cache == NULL)
		return EOF;
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
	spin_lock_irq_save(&cache->lock, flags);
	kprintf("ALLOC: #%d cache_alloc request %u bytes\n", cpuid(), cache->size);
	void *retval = __caching_allocator.alloc(cache);
	kprintf("ALLOC: #%d cache_alloc return %p (%u bytes)\n", cpuid(), retval, cache->size);
	spin_unlock_irq_restore(&cache->lock, flags);
	return retval;
}

int cache_free(struct allocator_cache *cache, void *obj)
{
	unsigned long flags;

	if (cache == NULL)
		return NULL;
	spin_lock_irq_save(&cache->lock, flags);
	kprintf("ALLOC: #%d cache_free request %p (%u bytes)\n", cpuid(), obj, cache->size);
	int retval = __caching_allocator.free(cache, obj);
	kprintf("ALLOC: #%d cache_free freed %p (%u bytes) with code %d\n", cpuid(), obj, cache->size, retval);
	spin_unlock_irq_restore(&cache->lock, flags);
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

