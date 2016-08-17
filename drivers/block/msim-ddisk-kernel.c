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

/*
 * TODO: merge common code among all disk drivers
 */

/*
 * This is the code for kernel driver.
 * It is separated from msim-ddisk.c for clarity and is included as is in
 * msim-ddisk.c by #include.
 * The internal routines are already provided in msim-ddisk.c
 */

#include <sys/types.h>
#include <proc.h>
#include <buf.h>
#include <fs/vnode.h>
#include <fs/bio.h>
#include <aim/device.h>
#include <mach-conf.h>
#include <aim/initcalls.h>
#include <drivers/io/io-mem.h>
#include <errno.h>
#include <panic.h>
#include <drivers/hd/hd.h>
#include <trap.h>
#include <libc/stdio.h>

#define DEVICE_MODEL	"msim-disk"

static struct blk_driver drv;

static void __init(struct hd_device *hd)
{
	kpdebug("initializing MSIM hard disk\n");
	__msim_dd_init(hd);
	kpdebug("initialization done\n");
}

static int __open(dev_t dev, int mode, struct proc *p)
{
	struct hd_device *hdpart;
	struct hd_device *hd;
	char partname[DEV_NAME_MAX];

	hd = (struct hd_device *)dev_from_id(hdbasedev(dev));
	/* should be initialized by device prober... */
	assert(hd != NULL);
	hdpart = (struct hd_device *)dev_from_id(dev);
	if (hdpart != NULL)
		/* covered the case where partno == 0 */
		return 0;

	kpdebug("__open: %d, %d\n", major(dev), minor(dev));

	/* Detect all partitions if the hard disk has not yet detected its
	 * partitions. */
	if (!(hd->hdflags & HD_LOADED))
		detect_hd_partitions(hd);

	if (hd->part[hdpartno(dev)].len == 0)
		return -ENODEV;

	/* Create a partition device for use */
	hdpart = kmalloc(sizeof(*hdpart), GFP_ZERO);
	snprintf(partname, DEV_NAME_MAX, "%s~%d\n", hd->name, hdpartno(dev));
	initdev(hdpart, DEVCLASS_BLK, partname, dev, &drv);
	dev_add(hdpart);

	return 0;
}

static int __close(dev_t dev, int oflags, struct proc *p)
{
	/*
	 * Since we only have one hard disk (which is root), do we really
	 * want to "close" it? (FIXME)
	 */
	kpdebug("msim-ddisk: closing\n");
	return 0;
}

/*
 * The real place where struct buf's are turned into disk commands.
 * Assumes that the buf queue lock is held.
 * Modifies buf flags, should be called with interrupts disabled.
 */
static void __start(struct hd_device *dev)
{
	struct buf *bp;
	off_t blkno, partoff;
	int partno;
	void *data;

	assert(!list_empty(&dev->bufqueue));
	bp = list_first_entry(&dev->bufqueue, struct buf, ionode);
	assert(bp->nbytesrem != 0);
	assert(IS_ALIGNED(bp->nbytes, SECTOR_SIZE));
	assert(IS_ALIGNED(bp->nbytesrem, SECTOR_SIZE));
	assert(bp->flags & (B_DIRTY | B_INVALID));
	assert(bp->flags & B_BUSY);
	assert(bp->blkno != BLKNO_INVALID);
	partno = hdpartno(bp->devno);
	assert((partno == 0) || (dev->part[partno].len != 0));
	partoff = (partno == 0) ? 0 : dev->part[partno].offset;
	blkno = bp->blkno + (bp->nbytes - bp->nbytesrem) / SECTOR_SIZE + partoff;
	data = bp->data + (bp->nbytes - bp->nbytesrem);
	bp->flags &= ~(B_DONE | B_ERROR | B_EINTR);

	if (bp->flags & B_DIRTY) {
		kpdebug("writing to %d from %p\n", blkno, data);
		__msim_dd_write_sector(dev, blkno, data, false);
	} else if (bp->flags & B_INVALID) {
		kpdebug("reading from %d to %p\n", blkno, data);
		__msim_dd_read_sector(dev, blkno, data, false);
	}
}

static void __startnext(struct hd_device *dev)
{
	if (!list_empty(&dev->bufqueue))
		__start(dev);
}

/*
 * Modifies buf flags, should be called with interrupts disabled.  But since
 * this is an interrupt handler, interrupts must be disabled already.
 */
static int __intr(int irq)
{
	/*
	 * Theoretically we need to enumerate all hard disks to check
	 * if an interrupt is asserted.  Here we assumes that we only
	 * have one device: rootdev.
	 */
	struct hd_device *hd;
	unsigned long flags;
	struct buf *bp;
	void *dst;

	hd = (struct hd_device *)dev_from_id(hdbasedev(rootdev));
	kpdebug("msim disk intr handler\n");
	if (!__msim_dd_check_interrupt(hd)) {
		panic("entering interrupt handler without rootdev interrupt?\n");
		/* return 0; */
	}
	__msim_dd_ack_interrupt(hd);

	assert(!list_empty(&hd->bufqueue));
	bp = list_first_entry(&hd->bufqueue, struct buf, ionode);
	assert(bp->flags & B_BUSY);
	assert(bp->flags & (B_INVALID | B_DIRTY));
	if (__msim_dd_check_error(hd)) {
		bp->errno = -EIO;
		bp->flags |= B_ERROR;
		kpdebug("fail buf %p\n", bp);
		list_del(&(bp->ionode));
		biodone(bp);
		__startnext(hd);
		return 0;
	}

	spin_lock_irq_save(&hd->lock, flags);

	if (list_empty(&hd->bufqueue)) {
		kpdebug("spurious interrupt?\n");
		return 0;
	}
	dst = bp->data + (bp->nbytes - bp->nbytesrem);
	if (!(bp->flags & B_DIRTY) && (bp->flags & B_INVALID)) {
		kpdebug("fetching to %p\n", dst);
		__msim_dd_fetch(hd, dst);
	}
	bp->nbytesrem -= SECTOR_SIZE;
	kpdebug("buf %p remain %d\n", bp, bp->nbytesrem);

	if (bp->nbytesrem == 0) {
		kpdebug("done buf %p\n", bp);
		list_del(&(bp->ionode));
		biodone(bp);
	}

	/* Continue current request if nbytesrem != 0, or next one otherwise */
	__startnext(hd);

	spin_unlock_irq_restore(&hd->lock, flags);
	return 0;
}

static int __strategy(struct buf *bp)
{
	struct hd_device *hd;
	unsigned long flags;

	hd = (struct hd_device *)dev_from_id(hdbasedev(bp->devno));
	spin_lock_irq_save(&hd->lock, flags);

	assert(bp->flags & B_BUSY);
	if (!(bp->flags & (B_DIRTY | B_INVALID))) {
		kpdebug("msim-ddisk: nothing to do\n");
		spin_unlock_irq_restore(&hd->lock, flags);
		return 0;
	}
	kpdebug("msim-ddisk: queuing buf %p with %d bytes at %p\n",
	    bp, bp->nbytes, bp->data);

	bp->nbytesrem = bp->nbytes;

	if (list_empty(&hd->bufqueue)) {
		list_add_tail(&bp->ionode, &hd->bufqueue);
		__start(hd);
	} else {
		list_add_tail(&bp->ionode, &hd->bufqueue);
	}
	spin_unlock_irq_restore(&hd->lock, flags);
	return 0;
}

static int __new(struct devtree_entry *entry)
{
	struct hd_device *hd;

	if (strcmp(entry->model, DEVICE_MODEL) != 0)
		return -ENOTSUP;

	hd = kmalloc(sizeof(*hd), GFP_ZERO);
	if (hd == NULL)
		return -ENOMEM;
	/* assuming only one disk... */
	initdev(hd, DEVCLASS_BLK, entry->name,
	    make_hdbasedev(MSIM_DISK_MAJOR, 0), &drv);
	hd->bus = (struct bus_device *)dev_from_name(entry->parent);
	hd->base = entry->regs[0];
	hd->nregs = entry->nregs;	/* assuming dev tree can't go wrong */
	list_init(&(hd->bufqueue));
	dev_add(hd);
	__init(hd);
	add_interrupt_handler(__intr, entry->irq);
	return 0;
}

static struct blk_driver drv = {
	.class = DEVCLASS_BLK,
	.open = __open,
	.close = __close,
	.strategy = __strategy,
	.new = __new,
};

static int __driver_init(void)
{
	register_driver(MSIM_DISK_MAJOR, &drv);
	return 0;
}
INITCALL_DRIVER(__driver_init);

