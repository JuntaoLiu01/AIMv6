
/*
 * TODO: merge common code among all disk drivers
 */

#include <aim/device.h>
#include <aim/initcalls.h>
#include <sys/types.h>
#include <mach-conf.h>
#include <drivers/hd/hd.h>
#include <drivers/ata/ata.h>
#include <vmm.h>
#include <libc/string.h>
#include <buf.h>
#include <fs/bio.h>
#include <fs/vnode.h>
#include <errno.h>
#include <sleep.h>
#include <trap.h>
#include <util.h>
#include <list.h>
#include <libc/stdio.h>

#define DEVICE_MODEL	"ide"

/*
 * Theoretically we can support multiple IDE drives (up to 4), but in our code
 * only one drive is supported.  With a preference over multiple drives for a
 * root drive, it will be easy to extend the code to multiple drives.
 */

static struct blk_driver drv;
static int port;	/* primary/secondary IDE, indicated by 0 or 2 */
static int slave;	/* master/slave IDE */

/* All the following implementations provided in ata_ide_pio.c or
 * ata_ide_bmdma.c (in future) */
static int __intr(int);
static bool __check_error_ack_interrupt(struct hd_device *);
static void __write_sectors(struct hd_device *, off_t, size_t, void *);
static void __read_sectors(struct hd_device *, off_t, size_t, void *);
static void __fetch(struct hd_device *, struct buf *);

static bool __pollwait(struct hd_device *hd)
{
	struct bus_device *bus;
	bus_read_fp r8;
	uint64_t tmp;

	bus = hd->bus;
	r8 = bus->bus_driver.get_read_fp(bus, 8);

	do {
		r8(bus, hd->bases[port], ATA_REG_STATUS, &tmp);
	} while ((tmp & (ATA_BUSY | ATA_DRDY)) != ATA_DRDY);
	if (tmp & (ATA_DF | ATA_ERR))
		return false;
	return true;
}

static int __init(struct devtree_entry *entry)
{
	struct bus_device *bus;
	struct hd_device *hd;
	bus_read_fp r8;
	bus_write_fp w8;
	uint64_t tmp;

	bus = (struct bus_device *)dev_from_name(entry->parent);
	r8 = bus->bus_driver.get_read_fp(bus, 8);
	w8 = bus->bus_driver.get_write_fp(bus, 8);
	for (port = 0; port < 4; port += 2) {
		for (slave = 0; slave <= 1; ++slave) {
			w8(bus, entry->regs[port], ATA_REG_DEVSEL,
			    ATA_LBA | (slave ? ATA_DEV1 : 0));
			udelay(1000);
			r8(bus, entry->regs[port], ATA_REG_STATUS, &tmp);
			if ((tmp & ATA_DRDY) &&
			    !(tmp & (ATA_ERR | ATA_DF))) {
				kpdebug("Selecting %s %s IDE drive\n",
				    port ? "secondary" : "primary",
				    slave ? "slave" : "master");
				goto success;
			}
		}
	}

	panic("no IDE drive found\n");
	/* NOTREACHED */
	return 0;

success:
	/* struct device creation */
	hd = kmalloc(sizeof(*hd), GFP_ZERO);
	if (hd == NULL)
		return -ENOMEM;
	initdev(hd, DEVCLASS_BLK, entry->name,
	    make_hdbasedev(IDE_DISK_MAJOR, 0), &drv);
	hd->bus = bus;
	memcpy(hd->bases, entry->regs, sizeof(addr_t) * entry->nregs);
	hd->nregs = entry->nregs;
	list_init(&hd->bufqueue);
	dev_add(hd);
	/* disable write cache */
	w8(bus, entry->regs[port], ATA_REG_FEATURE, SETFEATURES_WC_OFF);
	w8(bus, entry->regs[port], ATA_REG_CMD, ATA_CMD_SET_FEATURES);
	assert(__pollwait(hd));
	return 0;
}

static int __new(struct devtree_entry *entry)
{
	if (strcmp(entry->model, DEVICE_MODEL) != 0)
		return -ENOTSUP;
	/* assuming only one disk... */
	kpdebug("initializing IDE hard disk...\n");
	__init(entry);
	add_interrupt_handler(__intr, entry->irq);
	return 0;
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
	kpdebug("IDE: closing\n");
	return 0;
}

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
	assert((partno == 0) | (dev->part[partno].len != 0));
	partoff = (partno == 0) ? 0 : dev->part[partno].offset;
	blkno = bp->blkno + partoff;
	data = bp->data + (bp->nbytes - bp->nbytesrem);
	bp->flags &= ~(B_DONE | B_ERROR | B_EINTR);

	if (bp->flags & B_DIRTY) {
		kpdebug("writing to %d from %p\n", blkno, data);
		__write_sectors(dev, blkno, bp->nbytes, data);
	} else if (bp->flags & B_INVALID) {
		kpdebug("reading from %d to %p\n", blkno, data);
		__read_sectors(dev, blkno, bp->nbytes, data);
	}
}

static void
__startnext(struct hd_device *dev)
{
	if (!list_empty(&dev->bufqueue))
		__start(dev);
}

#define NSECT_HI(n)	((uint8_t)((n) >> 8))
#define NSECT_LO(n)	((uint8_t)(n))
#define LBAx(lba, x)	((uint8_t)((lba) >> ((x - 1) * 8)))
static void
__prepare_regs(struct hd_device *hd, uint16_t nsects, off_t blkno)
{
	struct bus_device *bus = hd->bus;
	bus_write_fp w8 = bus->bus_driver.get_write_fp(bus, 8);
	__pollwait(hd);
	w8(bus, hd->bases[port], ATA_REG_NSECT, NSECT_HI(nsects));
	w8(bus, hd->bases[port], ATA_REG_LBAL, LBAx(blkno, 4));
	w8(bus, hd->bases[port], ATA_REG_LBAM, LBAx(blkno, 5));
	w8(bus, hd->bases[port], ATA_REG_LBAH, LBAx(blkno, 6));
	w8(bus, hd->bases[port], ATA_REG_NSECT, NSECT_LO(nsects));
	w8(bus, hd->bases[port], ATA_REG_LBAL, LBAx(blkno, 1));
	w8(bus, hd->bases[port], ATA_REG_LBAM, LBAx(blkno, 2));
	w8(bus, hd->bases[port], ATA_REG_LBAH, LBAx(blkno, 3));
	w8(bus, hd->bases[port], ATA_REG_DEVSEL,
	    ATA_LBA | (slave ? ATA_DEV1 : 0));
	w8(bus, hd->bases[port + 1], 2, 0);	/* device control register */
}

static int
__strategy(struct buf *bp)
{
	struct hd_device *hd;
	unsigned long flags;

	hd = (struct hd_device *)dev_from_id(hdbasedev(bp->devno));
	spin_lock_irq_save(&hd->lock, flags);

	assert(bp->flags & B_BUSY);
	if (!(bp->flags & (B_DIRTY | B_INVALID))) {
		kpdebug("ide: nothing to do\n");
		spin_unlock_irq_restore(&hd->lock, flags);
		return 0;
	}
	kpdebug("ide: queuing buf %p with %d bytes at %p\n",
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

static struct blk_driver drv = {
	.class = DEVCLASS_BLK,
	.new = __new,
	.open = __open,
	.close = __close,
	.strategy = __strategy,
};

static int __driver_init(void)
{
	register_driver(IDE_DISK_MAJOR, &drv);
	return 0;
}
INITCALL_DRIVER(__driver_init);

#ifndef IDE_BMDMA
#include "ata_ide_pio.c"
#else
#include "ata_ide_bmdma.c"
#endif
