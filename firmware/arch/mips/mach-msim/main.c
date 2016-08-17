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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <libc/stddef.h>
#include <libc/string.h>
#include <smp.h>
#include <util.h>
#include <drivers/serial/uart.h>
#include <drivers/hd/hd.h>
#include <drivers/block/msim-ddisk.h>
#include <drivers/io/io-mem.h>

#define FWSTACKSIZE	(1 << FWSTACKORDER)

unsigned char fwstack[NR_CPUS][FWSTACKSIZE];

typedef void (*mbr_entry_fp)
    (void (*)(size_t, size_t, void *, size_t), uintptr_t);

void fwpanic(const char *msg)
{
	uart_puts(msg);
	for (;;)
		/* nothing */;
}

void readdisk(size_t sector, size_t offset, void *buf, size_t len)
{
	int i;
	unsigned char sector_buf[SECTOR_SIZE];
	size_t l = 0;

	sector += offset / SECTOR_SIZE;
	offset %= SECTOR_SIZE;

	for (; len > 0; len -= l) {
		l = min2(len, SECTOR_SIZE - offset);
		if (msim_dd_read_sector(MSIM_DISK_PHYSADDR,
		    sector, sector_buf, true) < 0)
			fwpanic("read disk error");
		for (i = 0; i < l; ++i)
			*(unsigned char *)(buf + i) = sector_buf[offset + i];
		offset = 0;
		buf += l;
		++sector;
	}
}

void main(void)
{
	char mbr[SECTOR_SIZE];
	io_mem_init(&early_memory_bus);
	uart_init();
	uart_puts("FW: Hello world!\r\n");
	msim_dd_init(MSIM_DISK_PHYSADDR);
	if (msim_dd_read_sector(MSIM_DISK_PHYSADDR, 0, mbr, true) == 0) {
		/*
		 * DESIGN NOTE:
		 * MIPS instructions are loosely-encoded, so it's usually
		 * not possible to embed the disk driver into MBR, as most
		 * legacy IBM PC-compatible computers (which usually have
		 * x86 cores) do.
		 *
		 * Unfortunately, there's no suitable bootloader standard
		 * for MIPS processors other than UEFI.  The utmost advantage
		 * for UEFI is CPU independence.  However, UEFI is too
		 * complicated as it must support GUID Partition Table (GPT),
		 * and even calls for minimal filesystem support.
		 *
		 * MSIM doesn't ship a firmware; you have to design it
		 * yourself.  And since we're mainly dealing with kernel
		 * design rather than firmware here, we choose an simple
		 * yet ugly design: to couple firmware and MBR together.
		 * The firmware jumps to MBR bootloader entry with an
		 * argument storing the address of the "read disk"
		 * function.  This design sticks firmware, disk hardware,
		 * and MBR together, and is thus a *BAD* design, however
		 * we don't want to waste too much effort here.
		 *
		 * Those interested in UEFI can contribute to our repo.
		 * Especially for MIPS developers, if you do work out a
		 * UEFI firmware, tell Loongson and they will be probably
		 * very grateful.
		 */

		/*
		 * This statement jumps into a function whose address is the
		 * same as buffer variable "mbr", where we just put the first
		 * disk sector.  Since the first 446 bytes of MBR (and hence
		 * the buffer "mbr") contains the bootloader code, jumping
		 * there transfers control to the MBR bootloader.
		 *
		 * For those unfamiliar with function pointers:
		 * This statement does the following:
		 * (1) obtain the buffer address ("mbr")
		 * (2) view it as a function entry with type mbr_entry_fp
		 *     (see the "typedef" statement above for definition)
		 * (3) execute the function there with two arguments: the
		 *     "read disk" function, and the address of MBR, i.e.
		 *     the address of bootloader entry itself.  The reason
		 *     is discussed in boot/arch/mips/msim/bootsect.c.
		 */
		(*(mbr_entry_fp)mbr)(readdisk, mbr);
	}
	for (;;)
		/* nothing */;
}

