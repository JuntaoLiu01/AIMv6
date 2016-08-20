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

#ifndef _DRIVERS_TIMER_TIMER_A9_HW_H
#define _DRIVERS_TIMER_TIMER_A9_HW_H

/* base address */
#define MPCORE_GTC_BASE	0x0200
#define GTC_PHYSBASE	(MPCORE_PHYSBASE + MPCORE_GTC_BASE)
#define PTC_PHYSBASE	(MPCORE_PHYSBASE + 0x600)
#define SWDT_PHYSBASE	(MPCORE_PHYSBASE + 0x620)

/* register offset */
#define GTC_COUNTER_LO_OFFSET		0x00
#define GTC_COUNTER_HI_OFFSET		0x04
#define GTC_CTRL_OFFSET			0x08
#define GTC_INT_OFFSET			0x0C
#define GTC_COMPARATOR_LO_OFFSET	0x10
#define GTC_COMPARATOR_HI_OFFSET	0x14
#define GTC_INCREMENT_OFFSET		0x18

#endif /* _DRIVERS_TIMER_TIMER_A9_HW_H */

