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

/*
 * This is the code for RAW driver, that is, for firmware.
 * It is separated from msim-ddisk.c for clarity and is included as is in
 * msim-ddisk.c by #include.
 * The internal routines are already provided in msim-ddisk.c
 */

#include <drivers/io/io-mem.h>

static struct blk_device __raw_disk = {
	.base = MSIM_DISK_PHYSADDR
};

void msim_dd_init(unsigned long paddr)
{
	__raw_disk.bus = &early_memory_bus;

	__msim_dd_init(&__raw_disk);
}

size_t msim_dd_get_sector_count(unsigned long paddr)
{
	/*
	 * Dirty hack, as paddr's never change throughout firmware.
	 * The following __raw_disk.paddr statements base on the same rationale.
	 */
	__raw_disk.base = paddr;
	return __msim_dd_get_sector_count(&__raw_disk);
}

int msim_dd_read_sector(unsigned long paddr, off_t off, void *buf, bool poll)
{
	__raw_disk.base = paddr;
	return __msim_dd_read_sector(&__raw_disk, off, buf, poll);
}

int msim_dd_write_sector(unsigned long paddr, off_t off, void *buf, bool poll)
{
	__raw_disk.base = paddr;
	return __msim_dd_write_sector(&__raw_disk, off, buf, poll);
}

int msim_dd_check_interrupt(unsigned long paddr)
{
	__raw_disk.base = paddr;
	return __msim_dd_check_interrupt(&__raw_disk);
}

void msim_dd_ack_interrupt(unsigned long paddr)
{
	__raw_disk.base = paddr;
	return __msim_dd_ack_interrupt(&__raw_disk);
}

