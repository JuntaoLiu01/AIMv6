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

#include <sys/types.h>
#include <aim/device.h>
#include <aim/initcalls.h>
#include <drivers/tty/tty.h>
#include <fs/uio.h>
#include <proc.h>
#include <vmm.h>
#include <errno.h>
#include <mach-conf.h>	/* TTY_MAJOR */
#include <libc/string.h>

#define DEVICE_NAME	"tty"

static struct chr_driver ttydrv;

/*
 * /dev/tty is the *DEFAULT* terminal device.
 *
 * Each platform has its own "default" terminal device.  However, how to
 * specify the composition of that default terminal, remains to be debated.
 * 1. Hardwire it in kernel codebase.  Changing the device requires
 *    recompiling the kernel.
 * 2. Store the device numbers in a file.  Probably unsafe.
 * 3. ...?
 *
 * A process can change its underlying terminal device via ioctl().
 */

extern int setup_default_tty(struct tty_device *, int, struct proc *);

static int __open(dev_t devno, int mode, struct proc *p)
{
	struct tty_device *tty;
	int err;

	tty = (struct tty_device *)dev_from_id(devno);
	if (tty == NULL) {
		tty = kmalloc(sizeof(*tty), GFP_ZERO);
		if (tty == NULL)
			return -ENOMEM;
		initdev(tty, DEVCLASS_CHR, DEVICE_NAME, devno, &ttydrv);
		err = setup_default_tty(tty, mode, p);
		if (err) {
			kfree(tty);
			return err;
		}
		dev_add(tty);
	}

	/* TODO FUTURE: check and fill sessions */
	return 0;
}

static int __close(dev_t devno, int oflags, struct proc *p)
{
	/* Currently we do nothing */
	return 0;
}

/*
 * TODO FUTURE:
 * Here in this terminal implementation read and write requests go straight
 * to the supporting hardware drivers.
 */
static int __read(dev_t devno, struct uio *uio, int ioflags)
{
	struct tty_device *tty;
	struct chr_device *indev, *outdev;
	struct chr_driver *indrv, *outdrv;
	char c;

	tty = (struct tty_device *)dev_from_id(devno);
	indev = tty->indev;
	outdev = tty->outdev;
	indrv = (struct chr_driver *)devsw[major(indev->devno)];
	outdrv = (struct chr_driver *)devsw[major(outdev->devno)];

	/*
	 * TODO FUTURE:
	 * On BSD, for a background process, one should
	 * 1. Send the whole process group a SIGTTIN signal.
	 * 2. Put the process into sleep on channel ("bed" in AIMv6) lbolt,
	 *    where the processes are woken up once a second.
	 * 3. Re-check whether the process is still a background process.
	 * Not sure how Linux does the job.
	 */
	while (uio->resid > 0) {
		c = indrv->getc(indev->devno);
		ureadc(c, uio);
		if (tty->flags & TTY_ECHO)
			outdrv->putc(outdev->devno, c);
	}
	return 0;
}

static int __write(dev_t devno, struct uio *uio, int ioflags)
{
	struct tty_device *tty;
	struct chr_device *outdev;
	struct chr_driver *outdrv;

	tty = (struct tty_device *)dev_from_id(devno);
	outdev = tty->outdev;
	outdrv = (struct chr_driver *)devsw[major(outdev->devno)];
	return outdrv->write(outdev->devno, uio, ioflags);
}

static struct chr_driver ttydrv = {
	.class = DEVCLASS_CHR,
	.open = __open,
	.close = __close,
	.read = __read,
	.write = __write
};

static int __driver_init(void)
{
	register_driver(TTY_MAJOR, &ttydrv);
	return 0;
}
INITCALL_DRIVER(__driver_init);

