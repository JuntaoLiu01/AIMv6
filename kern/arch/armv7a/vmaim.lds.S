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

#define __LDSCRIPT__

#include <mm.h>

#define KERN_PHYSBASE	RAM_PHYSBASE

#include <asm-generic/vmaim.lds.h>

OUTPUT_ARCH(arm)
ENTRY(master_entry)

SECTIONS
{
	. = KERN_PHYSBASE;

	/DISCARD/ : {}

	/* FIXME, this thing is dirty */
	RAM_SIZE = MEM_SIZE - 1M;

	.text.low : {
		TEXT_LOW
	}

	. += LOAD_OFFSET;

	HIGH_SECTION(.text) {
		TEXT
		. = ALIGN(PAGE_SIZE);
		*(.text.vector)
		. = ALIGN(PAGE_SIZE);
	}

	HIGH_SECTION(.got) {
		*(.got)
	}

	STRUCT_ALIGN();
	HIGH_SECTION(.rodata) {
		RODATA(STRUCT_ALIGNMENT)
		EARLY_INIT(STRUCT_ALIGNMENT)
		/* NORM is added as indication */
		NORM_INIT(STRUCT_ALIGNMENT)
		LATE_INIT(STRUCT_ALIGNMENT)
	}

	STRUCT_ALIGN();
	HIGH_SECTION(.data) {
		DATA(STRUCT_ALIGNMENT)
	}

	STRUCT_ALIGN();
	HIGH_SECTION(.bss) {
		_bss_begin = .;
		BSS(STRUCT_ALIGNMENT)
		. = ALIGN(16K);
		SYMBOL(boot_page_index) = .;
		. += 16K; /* early pgdir */
		. += 4K; /* early stack */
		SYMBOL(boot_stack) = .;
		_bss_end = .;
	}

	SYMBOL(kern_end) = ALIGN(PAGE_SIZE);
}

