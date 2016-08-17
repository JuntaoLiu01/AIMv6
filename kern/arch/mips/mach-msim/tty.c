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
#include <mach-conf.h>
#include <drivers/tty/tty.h>
#include <proc.h>

int __mach_setup_default_tty(struct tty_device *tty, int mode, struct proc *p)
{
	unsigned int min = minor(tty->devno);
	struct chr_driver *indrv = (struct chr_driver *)devsw[MSIM_KBD_MAJOR];
	struct chr_driver *outdrv = (struct chr_driver *)devsw[MSIM_LP_MAJOR];
	dev_t outdevno = makedev(MSIM_LP_MAJOR, min);
	dev_t indevno = makedev(MSIM_KBD_MAJOR, min);
	int err;

	assert(indrv != NULL);
	assert(outdrv != NULL);

	err = indrv->open(indevno, mode, p);
	if (err)
		return err;
	err = outdrv->open(outdevno, mode, p);
	if (err) {
		indrv->close(indevno, mode, p);
		return err;
	}
	tty->indev = (struct chr_device *)dev_from_id(indevno);
	tty->outdev = (struct chr_device *)dev_from_id(outdevno);
	return 0;
}

