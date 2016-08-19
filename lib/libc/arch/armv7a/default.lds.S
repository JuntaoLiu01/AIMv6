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
#endif

#define TARGET	"elf32-littlearm"

#define USER_START	0x8000

OUTPUT_FORMAT(TARGET)

ENTRY(__start)

SECTIONS {
	. = USER_START;
	.text : {
		*(.text*);
	}
	. = ALIGN(32);
	.rodata : {
		*(.rodata*);
	}
	HIDDEN(_gp = ALIGN(16) + 0x7ff0);
	.got : {
		*(.got*);
	}
	.data : {
		*(.data*);
	}
	.bss : {
		*(.bss*);
	}
}

