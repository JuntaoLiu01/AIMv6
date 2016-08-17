/* Copyright (C) 2016 Gan Quan <coin2028@hotmail.com>
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

#include <addrspace.h>
#include <io.h>
#include <config.h>
#include <libc/string.h>
#include <drivers/hd/hd.h>
#include <drivers/block/msim-ddisk.h>
#include <aim/device.h>

/* NOTE: we are assuming only one disk device.  In reality, we need one
 * DMA buffer *per* device. */
unsigned char msim_dd_dma[SECTOR_SIZE];

static int __msim_dd_check_interrupt(struct blk_device *dev)
{
	uint64_t result;
	struct bus_device *bus = dev->bus;
	bus_read_fp bus_read32 = bus->bus_driver.get_read_fp(bus, 32);
	bus_read32(bus, dev->base, MSIM_DD_STAT, &result);
	return !!(result & STAT_INTR);
}

static int __msim_dd_check_error(struct blk_device *dev)
{
	uint64_t result;
	struct bus_device *bus = dev->bus;
	bus_read_fp bus_read32 = bus->bus_driver.get_read_fp(bus, 32);
	bus_read32(bus, dev->base, MSIM_DD_STAT, &result);
	return !!(result & STAT_ERROR);
}

static void __msim_dd_ack_interrupt(struct blk_device *dev)
{
	struct bus_device *bus = dev->bus;
	bus_write_fp bus_write32 = bus->bus_driver.get_write_fp(bus, 32);
	bus_write32(bus, dev->base, MSIM_DD_COMMAND, CMD_ACK);
}

static void __msim_dd_init(struct blk_device *dev)
{
	struct bus_device *bus = dev->bus;
	bus_write_fp bus_write32 = bus->bus_driver.get_write_fp(bus, 32);
	bus_write32(bus, dev->base, MSIM_DD_DMAADDR, kva2pa(msim_dd_dma));
}

static size_t __msim_dd_get_sector_count(struct blk_device *dev)
{
	struct bus_device *bus = dev->bus;
	bus_read_fp bus_read32 = bus->bus_driver.get_read_fp(bus, 32);
	uint64_t result;
	bus_read32(bus, dev->base, MSIM_DD_SIZE, &result);
	return (size_t)result;
}

/*
 * Read sector, returns 0 if successful.
 */
static int
__msim_dd_read_sector(struct blk_device *dev, size_t sect, void *buf, bool poll)
{
	struct bus_device *bus = dev->bus;
	bus_read_fp bus_read32 = bus->bus_driver.get_read_fp(bus, 32);
	bus_write_fp bus_write32 = bus->bus_driver.get_write_fp(bus, 32);
	uint64_t result;

	bus_write32(bus, dev->base, MSIM_DD_DMAADDR, kva2pa(msim_dd_dma));
	bus_write32(bus, dev->base, MSIM_DD_SECTOR, sect);
	bus_write32(bus, dev->base, MSIM_DD_COMMAND, CMD_READ);
	if (poll) {
		while (!__msim_dd_check_interrupt(dev))
			/* nothing */;
		/* Clear interrupt */
		__msim_dd_ack_interrupt(dev);
		bus_read32(bus, dev->base, MSIM_DD_STAT, &result);
		if (result & STAT_ERROR) {
			return -1;
		} else {
			memcpy(buf, msim_dd_dma, SECTOR_SIZE);
			return 0;
		}
	} else {
		return 0;
	}
}

static int
__msim_dd_write_sector(struct blk_device *dev, size_t sect, void *buf, bool poll)
{
	struct bus_device *bus = dev->bus;
	bus_read_fp bus_read32 = bus->bus_driver.get_read_fp(bus, 32);
	bus_write_fp bus_write32 = bus->bus_driver.get_write_fp(bus, 32);
	uint64_t result;

	memcpy(msim_dd_dma, buf, SECTOR_SIZE);
	bus_write32(bus, dev->base, MSIM_DD_DMAADDR, kva2pa(msim_dd_dma));
	bus_write32(bus, dev->base, MSIM_DD_SECTOR, sect);
	bus_write32(bus, dev->base, MSIM_DD_COMMAND, CMD_WRITE);
	if (poll) {
		while (!__msim_dd_check_interrupt(dev))
			/* nothing */;
		/* Clear interrupt */
		__msim_dd_ack_interrupt(dev);
		bus_read32(bus, dev->base, MSIM_DD_STAT, &result);
		if (result & STAT_ERROR)
			return -1;
		else
			return 0;
	} else {
		return 0;
	}
}

static void __msim_dd_fetch(struct blk_device *dev, void *buf)
{
	memcpy(buf, msim_dd_dma, SECTOR_SIZE);
}

#ifdef RAW
#include "msim-ddisk-raw.c"
#else
#include "msim-ddisk-kernel.c"
#endif

