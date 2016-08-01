
/* in ata_ide.c */
static int port;
static int slave;

static bool
__check_error_ack_interrupt(struct hd_device *hd)
{
	return __pollwait(hd);
}

static void
__write_sectors(struct hd_device *hd, off_t blkno, size_t nbytes, void *data)
{
	struct bus_device *bus;
	bus_write_fp w8;
	uint16_t nsects = nbytes / SECTOR_SIZE;

	bus = hd->bus;
	w8 = bus->bus_driver.get_write_fp(bus, 8);

	__prepare_regs(hd, nsects, blkno);
	w8(bus, hd->bases[port], ATA_REG_CMD, ATA_CMD_PIO_WRITE_EXT);

	for (; nbytes; nbytes--, data++)
		w8(bus, hd->bases[port], ATA_REG_DATA, *(char *)data);
}

static void
__read_sectors(struct hd_device *hd, off_t blkno, size_t nbytes, void *data)
{
	struct bus_device *bus;
	bus_write_fp w8;
	uint16_t nsects = nbytes / SECTOR_SIZE;

	bus = hd->bus;
	w8 = bus->bus_driver.get_write_fp(bus, 8);

	__prepare_regs(hd, nsects, blkno);
	w8(bus, hd->bases[port], ATA_REG_CMD, ATA_CMD_PIO_READ_EXT);
}

static void
__fetch(struct hd_device *hd, struct buf *bp)
{
	struct bus_device *bus;
	bus_read_fp r8;
	uint64_t tmp;
	char *data = bp->data + bp->nbytes - bp->nbytesrem;

	bus = hd->bus;
	r8 = bus->bus_driver.get_read_fp(bus, 8);

	for (r8(bus, hd->bases[port], ATA_REG_STATUS, &tmp);
	     tmp & ATA_DRQ;
	     r8(bus, hd->bases[port], ATA_REG_STATUS, &tmp)) {
		r8(bus, hd->bases[port], ATA_REG_DATA, &tmp);
		if (bp->nbytesrem > 0) {
			*(data++) = tmp;
			bp->nbytesrem--;
		}
	}
}

static int
__intr(int irq)
{
	struct hd_device *hd;
	struct buf *bp;
	unsigned long flags;
	bool success;

	hd = (struct hd_device *)dev_from_id(hdbasedev(rootdev));
	kpdebug("ide interrupt\n");

	spin_lock_irq_save(&hd->lock, flags);

	if (list_empty(&hd->bufqueue)) {
		kpdebug("ide spurious interrupt\n");
		__check_error_ack_interrupt(hd);
		spin_unlock_irq_restore(&hd->lock, flags);
		return 0;
	}

	bp = list_first_entry(&hd->bufqueue, struct buf, ionode);
	success = __check_error_ack_interrupt(hd);
	assert(bp->flags & B_BUSY);
	assert(bp->flags & (B_INVALID | B_DIRTY));
	if (!success) {
		bp->errno = -EIO;
		bp->flags |= B_ERROR;
		kpdebug("fail buf %p\n", bp);
		list_del(&(bp->ionode));
		biodone(bp);
		__startnext(hd);
		spin_unlock_irq_restore(&hd->lock, flags);
		return 0;
	}
	if (!(bp->flags & B_DIRTY) && (bp->flags & B_INVALID)) {
		kpdebug("fetching to %p\n", bp->data);
		__fetch(hd, bp);
	}

	kpdebug("buf %p remain %d\n", bp, bp->nbytesrem);
	if (bp->nbytesrem == 0) {
		kpdebug("done buf %p\n", bp);
		list_del(&(bp->ionode));
		biodone(bp);
		/* We start the next request only if the current request is
		 * done or failed */
		__startnext(hd);
	}
	spin_unlock_irq_restore(&hd->lock, flags);
	return 0;
}

