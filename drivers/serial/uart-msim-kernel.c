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
#include <proc.h>
#include <vmm.h>
#include <errno.h>
#include <mach-conf.h>
#include <libc/string.h>
#include <trap.h>
#include <asm-generic/funcs.h>
#include <drivers/console/cons.h>

#define LP_DEVICE_MODEL		"msim-lpr"
#define KBD_DEVICE_MODEL	"msim-kbd"

static struct chr_driver kbddrv, lpdrv;

/* Common code for open */
static int __open(dev_t devno, int mode, struct proc *p, bool kbd)
{
	struct chr_device *dev;
	kpdebug("opening %s device\n", kbd ? "keyboard" : "printer");

	dev = (struct chr_device *)dev_from_id(devno);
	/* should be initialized by device prober... */
	assert(dev != NULL);
	cons_open(dev, mode, p);
	return 0;
}

/* Common code for close */
static int __close(dev_t devno, int mode, struct proc *p)
{
	/* Currently we do nothing */
	return 0;
}

static int __lpopen(dev_t devno, int mode, struct proc *p)
{
	return __open(devno, mode, p, false);
}

static int __kbdopen(dev_t devno, int mode, struct proc *p)
{
	return __open(devno, mode, p, true);
}

static int __kbdintr(int irq)
{
	struct chr_device *kbd;

	/* XXX I'm assuming there's only one keyboard */
	kbd = (struct chr_device *)dev_from_id(makedev(MSIM_KBD_MAJOR, 0));
	assert(kbd != NULL);
	return cons_intr(irq, kbd, __uart_msim_getchar);
}

static int __kbdgetc(dev_t devno)
{
	struct chr_device *kbd;
	int c;

	/* XXX I'm assuming there's only one keyboard */
	kbd = (struct chr_device *)dev_from_id(makedev(MSIM_KBD_MAJOR, 0));
	assert(kbd != NULL);
	c = cons_getc(kbd);
	/*
	 * XXX
	 * Ordinarily, the driver SHOULD send raw characters (or character
	 * sequences) to user space, and leave the conversion work (such
	 * as conversion from DEL to BS here) to external user-space
	 * libraries such as terminfo(5).
	 *
	 * Here, we do the job in kernel drivers so that we do not need
	 * to implement a wholly new terminfo(5).  But do remember that
	 * the trick here is usually a *BAD* practice.
	 *
	 * See also:
	 * __getc() in uart-ns16550-kernel.c
	 *
	 * FIXME: maybe someone else could implement a mini terminfo?
	 */
	return (c == 127 ? '\b' : c);
}

static int __lpputc(dev_t devno, int c)
{
	struct chr_device *lp;
	lp = (struct chr_device *)dev_from_id(devno);
	assert(lp != NULL);
	return cons_putc(lp, c, __uart_msim_putchar);
}

static int __lpwrite(dev_t devno, struct uio *uio, int ioflags)
{
	struct chr_device *lp;

	lp = (struct chr_device *)dev_from_id(devno);
	assert(lp != NULL);
	return cons_write(lp, uio, ioflags);
}

static int __new(struct devtree_entry *entry, bool kbd)
{
	struct chr_device *dev;

	if (strcmp(entry->model, kbd ? KBD_DEVICE_MODEL : LP_DEVICE_MODEL) != 0)
		return -ENOTSUP;

	kpdebug("initializing MSIM %s\n", kbd ? "keyboard" : "line printer");
	dev = kmalloc(sizeof(*dev), GFP_ZERO);
	if (dev == NULL)
		return -ENOMEM;
	/* assuming only one keyboard/line-printer */
	kpdebug("name: %s\n", entry->name);
	initdev(dev, DEVCLASS_CHR, entry->name,
	    makedev(kbd ? MSIM_KBD_MAJOR : MSIM_LP_MAJOR, 0),
	    kbd ? &kbddrv : &lpdrv);
	dev->bus = (struct bus_device *)dev_from_name(entry->parent);
	dev->base = entry->regs[0];
	dev->nregs = entry->nregs;
	dev_add(dev);
	__uart_msim_init(kbd ? NULL : dev, kbd ? dev : NULL);
	add_interrupt_handler(__kbdintr, entry->irq);
	return 0;
}

static int __lpnew(struct devtree_entry *entry)
{
	return __new(entry, false);
}

static int __kbdnew(struct devtree_entry *entry)
{
	return __new(entry, true);
}

static struct chr_driver lpdrv = {
	.class = DEVCLASS_CHR,
	.open = __lpopen,
	.close = __close,
	.read = NOTSUP,
	.write = __lpwrite,
	.getc = NOTSUP,
	.putc = __lpputc,
	.new = __lpnew,
};

static struct chr_driver kbddrv = {
	.class = DEVCLASS_CHR,
	.open = __kbdopen,
	.close = __close,
	.read = NOTSUP,		/* NYI */
	.write = NOTSUP,
	.getc = __kbdgetc,	/* used by tty */
	.putc = NOTSUP,
	.new = __kbdnew,
};

static int __driver_init(void)
{
	register_driver(MSIM_LP_MAJOR, &lpdrv);
	register_driver(MSIM_KBD_MAJOR, &kbddrv);
	return 0;
}
INITCALL_DRIVER(__driver_init);

