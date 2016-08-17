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

#include <drivers/tty/tty.h>
#include <proc.h>
#include <panic.h>
#include <mach-conf.h>
#include <sys/types.h>
#include <aim/device.h>

/* TODO: we should probably merge the TTY setup code... */
int __mach_setup_default_tty(struct tty_device *tty, int mode, struct proc *p)
{
	unsigned int min = minor(tty->devno);
	struct chr_driver *drv = (struct chr_driver *)devsw[UART_MAJOR];
	dev_t devno = makedev(UART_MAJOR, min);
	int err;

	assert(drv != NULL);

	err = drv->open(devno, mode, p);
	if (err)
		return err;
	tty->outdev = tty->indev = (struct chr_device *)dev_from_id(devno);
	return 0;
}

