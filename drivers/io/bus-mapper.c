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
#include <mm.h>
#include <errno.h>
#include <aim/device.h>
#include <aim/initcalls.h>
#include <aim/kmmap.h>
#include <mach-conf.h>
#include <lib/libc/string.h>

static int read(int width, struct bus_device *inst, addr_t base, addr_t offset,
    uint64_t *ptr)
{
	struct bus_device *bus = inst->bus;
	bus_read_fp read_fp = bus->bus_driver.get_read_fp(bus, width);
	if (read_fp == NULL) return -1;
	return read_fp(bus, inst->base, base + offset, ptr);
}

static int write(int width, struct bus_device *inst, addr_t base, addr_t offset,
    uint64_t val)
{
	struct bus_device *bus = inst->bus;
	bus_write_fp write_fp = bus->bus_driver.get_write_fp(bus, width);
	if (write_fp == NULL) return -1;
	return write_fp(bus, inst->base, base + offset, val);
}

static int read8(struct bus_device *inst, addr_t base, addr_t offset,
    uint64_t *ptr)
{
	return read(8, inst, base, offset, ptr);
}

static int read16(struct bus_device *inst, addr_t base, addr_t offset,
    uint64_t *ptr)
{
	return read(16, inst, base, offset, ptr);
}

static int read32(struct bus_device *inst, addr_t base, addr_t offset,
    uint64_t *ptr)
{
	return read(32, inst, base, offset, ptr);
}

static int write8(struct bus_device *inst, addr_t base, addr_t offset,
    uint64_t val)
{
	return write(8, inst, base, offset, val);
}

static int write16(struct bus_device *inst, addr_t base, addr_t offset,
    uint64_t val)
{
	return write(16, inst, base, offset, val);
}

static int write32(struct bus_device *inst, addr_t base, addr_t offset,
    uint64_t val)
{
	return write(32, inst, base, offset, val);
}

/* forward */
static int new(struct devtree_entry *entry);

static bus_read_fp get_read_fp(struct bus_device *inst, int data_width)
{
	struct bus_device *bus = inst->bus;
	if (bus->bus_driver.get_read_fp(bus, data_width) == NULL)
		return NULL;
	switch (data_width) {
		case 8: return read8;
		case 16: return read16;
		case 32: return read32;
	}
	return NULL;
}

static bus_write_fp get_write_fp(struct bus_device *inst, int data_width)
{
	struct bus_device *bus = inst->bus;
	if (bus->bus_driver.get_write_fp(bus, data_width) == NULL)
		return NULL;
	switch (data_width) {
		case 8: return write8;
		case 16: return write16;
		case 32: return write32;
	}
	return NULL;
}

static struct bus_driver drv = {
	.class = DEVCLASS_BUS,
	.get_read_fp = get_read_fp,
	.get_write_fp = get_write_fp,
	.new = new
};

static int new(struct devtree_entry *entry)
{
	struct bus_device *dev;
	int minor, mapped = 0;
	size_t size;

	if (strcmp(entry->name, "mpcore") == 0) {
		minor = 0;
		size = PAGE_SIZE * 2;
		goto good;
	}

	return -ENOTSUP;

good:
	kpdebug("<bus-mapper> initializing device %s.\n", entry->name);
	/* need to kmmap if parent is memory. */
	void *vaddr;
	if (strcmp(entry->parent, "memory") == 0) {
		mapped = 1;
		vaddr = kmmap(NULL, entry->regs[0], size, MAP_SHARED_DEV);
		if (vaddr == NULL) return -ENOMEM;
	} else {
		vaddr = (void *)ULCAST(entry->regs[0]);
	}
	/* allocate */
	dev = kmalloc(sizeof(*dev), GFP_ZERO);
	if (dev == NULL) {
		if (mapped) kmunmap(vaddr);
		return -ENOMEM;
	}
	initdev(dev, DEVCLASS_BUS, entry->name, makedev(MAPPED_BUS_MAJOR, minor), &drv);
	dev->bus = (struct bus_device *)dev_from_name(entry->parent);
	dev->base = ULCAST(vaddr);
	dev->nregs = entry->nregs;
	dev_add(dev);
	/* tell ARCH when we see mpcore */
	if (strcmp(entry->name, "mpcore") == 0) {
		extern struct bus_device *mpcore;
		mpcore = dev;
	}
	kpdebug("<bus-mapper> device registered.\n");
	return 0;
}

static int __init(void)
{
	kpdebug("<bus-mapper> initializing.\n");
	register_driver(NOMAJOR, &drv);
	kpdebug("<bus-mapper> done.\n");
	return 0;
}
INITCALL_DRIVER(__init);

