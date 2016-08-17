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

#ifndef _DRIVERS_TTY_TTY_H
#define _DRIVERS_TTY_TTY_H

#include <aim/device.h>
#include <proc.h>

struct tty_device {
	struct chr_device;

	struct chr_device *indev;
	struct chr_device *outdev;

	uint32_t	flags;
#define TTY_ECHO	0x1	/* echo inputs */
	struct proc	*fg;	/* Leader of foreground process group */
	struct proc	*session; /* Leader of associated session */
};

#endif

