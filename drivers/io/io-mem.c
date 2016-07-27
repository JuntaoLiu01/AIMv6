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

/*
 * This driver is a wrapper to access memory as a bus.
 * It may sound silly, but is actually needed if there's memory-attached
 * devices, especially when there's memory not connected to the processor
 * cores directly.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <sys/types.h>
#include <sys/param.h>
#include <io.h>
#include <mm.h>
#include <aim/device.h>
#include <panic.h>
#include <aim/initcalls.h>
#include <asm-generic/funcs.h>
#include <errno.h>

#include <io-mem.h>

static int __read8(struct bus_device * inst, addr_t addr, uint64_t *ptr)
{
	*ptr = (uint64_t)read8(addr);
	return 0;
}

static int __read16(struct bus_device * inst, addr_t addr, uint64_t *ptr)
{
	*ptr = (uint64_t)read16(addr);
	return 0;
}

static int __read32(struct bus_device * inst, addr_t addr, uint64_t *ptr)
{
	*ptr = (uint64_t)read32(addr);
	return 0;
}

static int __write8(struct bus_device * inst, addr_t addr, uint64_t val)
{
	write8(addr, (uint8_t)val);
	return 0;
}

static int __write16(struct bus_device * inst, addr_t addr, uint64_t val)
{
	write16(addr, (uint16_t)val);
	return 0;
}

static int __write32(struct bus_device * inst, addr_t addr, uint64_t val)
{
	write32(addr, (uint32_t)val);
	return 0;
}

static bus_read_fp __get_read_fp(struct bus_device * inst, int data_width)
{
	switch (data_width) {
		case 8: return __read8;
		case 16: return __read16;
		case 32: return __read32;
	}
	return NULL;
}

static bus_write_fp __get_write_fp(struct bus_device * inst, int data_width)
{
	switch (data_width) {
		case 8: return __write8;
		case 16: return __write16;
		case 32: return __write32;
	}
	return NULL;
}

#ifndef RAW
static void __jump_handler(void)
{
	early_memory_bus.bus_driver.get_read_fp = __get_read_fp;
	early_memory_bus.bus_driver.get_write_fp = __get_write_fp;
}
#endif

/* Should only be used before memory management is initialized */
struct bus_device early_memory_bus = {
	.addr_width = 32,
	.class = DEVCLASS_BUS,
	.name = "memory",
};

void io_mem_init(struct bus_device *memory_bus)
{
	memory_bus->bus_driver.get_read_fp = __get_read_fp;
	memory_bus->bus_driver.get_write_fp = __get_write_fp;
#ifndef RAW
	if (jump_handlers_add((generic_fp)(size_t)postmap_addr(__jump_handler)) != 0)
		panic("Memory IO driver cannot register JUMP handler.\n");
#endif
}

#ifndef RAW

static int __new(struct devtree_entry *entry)
{
	/* default implementation... */
	return -EEXIST;
}

static struct bus_driver drv = {
	.class = DEVCLASS_BUS,
	.get_read_fp = __get_read_fp,
	.get_write_fp = __get_write_fp,
	.probe = NOP,
	.new = __new,
};

static int __driver_init(void)
{
	struct bus_device *memory_bus;
	register_driver(NOMAJOR, &drv);
#ifdef IO_MEM_ROOT
	memory_bus = kmalloc(sizeof(*memory_bus), GFP_ZERO);
	initdev(memory_bus, DEVCLASS_BUS, "memory", NODEV, &drv);
	dev_add(memory_bus);
#endif
	return 0;
}
INITCALL_DRIVER(__driver_init);

#endif

