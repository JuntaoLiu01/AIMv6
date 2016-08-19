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

#ifndef _MACH_H
#define _MACH_H

#define MPCORE_PHYSBASE	0xF8F00000

#define GTC_OFFSET	0x0200
#define GTC_IRQ		27

#define UART0_PHYSBASE	0xE0000000
#define UART0_IRQ	59

#define UART1_PHYSBASE	0xE0001000
#define UART1_IRQ	82

#define SD0_PHYSBASE	0xE0100000
#define SD0_IRQ		56

#define SD1_PHYSBASE	0xE0101000
#define SD1_IRQ		79

#endif /* _MACH_H */

