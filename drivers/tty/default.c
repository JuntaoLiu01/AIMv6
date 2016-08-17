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

#include <aim/device.h>
#include <drivers/tty/tty.h>
#include <proc.h>

/*
 * The routine which specify the default terminal composition, i.e. what's
 * the input device, what's the output device, etc.
 */
extern int __mach_setup_default_tty(struct tty_device *, int, struct proc *);
int setup_default_tty(struct tty_device *tty, int mode, struct proc *p)
{
	/* Currently we are hardwiring */
	tty->flags = TTY_ECHO;
	return __mach_setup_default_tty(tty, mode, p);
}

