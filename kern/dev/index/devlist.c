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
#include <aim/device.h>
#include <aim/initcalls.h>
#include <aim/sync.h>
#include <vmm.h>
#include <console.h>

#include <list.h>

struct device_entry {
	struct device *dev;
	struct list_head node;
};

static struct list_head __head = EMPTY_LIST(__head);
static lock_t __lock = UNLOCKED;

/* comparition according to id */
static inline int __cmp(struct device_entry *a, struct device_entry *b)
{
	struct device *dev_a, *dev_b;
	dev_a = a->dev;
	dev_b = b->dev;
	if (dev_a->id_major > dev_b->id_major) return 1;
	if (dev_a->id_major < dev_b->id_major) return -1;
	if (dev_a->id_minor > dev_b->id_minor) return 1;
	if (dev_a->id_minor < dev_b->id_minor) return -1;
	return 0; /* unlikely */
}

static int __add(struct device *dev)
{
	struct device_entry *this, *tmp;

	/* allocate and fill in an entry */
	this = kmalloc(sizeof(struct device_entry), 0);
	if (this == NULL)
		return EOF;
	this->dev = dev;

	/* lock up the list */
	spin_lock(&__lock);

	/* insert */
	for_each_entry(tmp, &__head, node) {
		if (__cmp(tmp, this) > 0) break;
	}
	list_add_before(&this->node, &tmp->node);

	/* unlock the list */
	spin_unlock(&__lock);

	return 0;
}

static int __remove(struct device *dev)
{
	struct device_entry *tmp, *this = NULL;
	int retval = EOF;

	/* lock up the list */
	spin_lock(&__lock);

	/* check for connected devices and the entry to remove */
	for_each_entry(tmp, &__head, node) {
		if ((struct device *)tmp->dev->bus == dev)
			goto ret; /* have connected device */
		else if (tmp->dev == dev)
			this = tmp;
	}

	/* remove */
	if (this != NULL) {
		list_del(&this->node);
		kfree(this);
		retval = 0; /* success */
	}

ret:
	/* unlock the list */
	spin_unlock(&__lock);
	return retval;
}

static struct device *__from_id(devid_t major, devid_t minor)
{
	return NULL;
}

static struct device *__from_name(char *name)
{
	return NULL;
}

static int __init(void)
{
	kprintf("KERN: <devlist> initializing.\n");

	struct device_index this = {
		.add		= __add,
		.remove		= __remove,
		.from_id	= __from_id,
		.from_name	= __from_name
	};
	set_device_index(&this);

	kprintf("KERN: <devlist> Done.\n");
	return 0;
}


INITCALL_CORE(__init)

