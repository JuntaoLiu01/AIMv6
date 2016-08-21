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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

/* from kernel */
#include <aim/device.h>
#include <aim/initcalls.h>
#include <aim/kmmap.h>
#include <io.h>
#include <mm.h>
#include <sleep.h>
#include <errno.h>
#include <mach-conf.h>
#include <lib/libc/string.h>
#include <buf.h>
#include <fs/vnode.h>
#include <fs/bio.h>
#include <libc/stdio.h>

#include <drivers/hd/hd.h>
#include <drivers/io/io-mem.h>

/* from sd driver */
#include <sd-zynq.h>
#include <sd-zynq-hw.h>

static int cardtype;

void __sd_zynq_init(struct hd_device *hd)
{
	struct bus_device * bus = hd->bus;
	bus_write_fp w8 = bus->bus_driver.get_write_fp(bus, 8);
	bus_write_fp w16 = bus->bus_driver.get_write_fp(bus, 16);
	bus_read_fp r8 = bus->bus_driver.get_read_fp(bus, 8);
	bus_read_fp r16 = bus->bus_driver.get_read_fp(bus, 16);

	uint64_t tmp;
	uint16_t tmp16;
	uint8_t tmp8;
	/* reset */
	w8(bus, hd->base, SD_SW_RST_OFFSET, SD_SWRST_ALL_MASK);
	do {
		r8(bus, hd->base, SD_SW_RST_OFFSET, &tmp);
	} while (tmp & SD_SWRST_ALL_MASK);

	/* capabilities = r32(bus, hd->base, SD_CAPS_OFFSET, @tgt) */

	/* enable internal clock */
	tmp16 = SD_CC_SDCLK_FREQ_D128 | SD_CC_INT_CLK_EN;
	w16(bus, hd->base, SD_CLK_CTRL_OFFSET, tmp16);
	do {
		r16(bus, hd->base, SD_CLK_CTRL_OFFSET, &tmp);
	}
	while (!(tmp & SD_CC_INT_CLK_STABLE));

	/* enable SD clock */
	r16(bus, hd->base, SD_CLK_CTRL_OFFSET, &tmp);
	tmp |= SD_CC_SD_CLK_EN;
	w16(bus, hd->base, SD_CLK_CTRL_OFFSET, tmp);
	
	/* enable bus power */
	tmp8 = SD_PC_BUS_VSEL_3V3 | SD_PC_BUS_PWR;
	w8(bus, hd->base, SD_POWER_CTRL_OFFSET, tmp8);
	w8(bus, hd->base, SD_HOST_CTRL1_OFFSET, SD_HC_DMA_SDMA);
	/*
	 * Xilinx's driver uses ADMA2 by default, we use single-operation
	 * DMA to avoid putting descriptors in memory.
	 */

	/* enable interrupt status except card */
	tmp16 = SD_NORM_INTR_ALL & (~SD_INTR_CARD);
	w16(bus, hd->base, SD_NORM_INTR_STS_EN_OFFSET, tmp16);
	w16(bus, hd->base, SD_ERR_INTR_STS_EN_OFFSET, SD_ERR_INTR_ALL);

	/* but disable all interrupt signals */
	w16(bus, hd->base, SD_NORM_INTR_SIG_EN_OFFSET, 0x0);
	w16(bus, hd->base, SD_ERR_INTR_SIG_EN_OFFSET, 0x0);

	/* set block size to 512 */
	w16(bus, hd->base, SD_BLK_SIZE_OFFSET, 512);
}

/* add descriptions to a command */
static inline uint16_t sd_frame_cmd(uint16_t cmd)
{
	switch (cmd) {
		case SD_CMD0:
		case SD_CMD4:
			cmd |= SD_RESP_NONE; break;
		case SD_ACMD6:
		case SD_CMD7:
		case SD_CMD10:
		case SD_CMD12:
		case SD_ACMD13:
		case SD_CMD16:
		case SD_ACMD42:
		case SD_CMD52:
		case SD_CMD55:
			cmd |= SD_RESP_R1; break;
		case SD_CMD17:
		case SD_CMD18:
		case SD_CMD23:
		case SD_ACMD23:
		case SD_CMD24:
		case SD_CMD25:
		case SD_ACMD51:
			cmd |= SD_RESP_R1 | SD_DAT_PRESENT; break;
		case SD_CMD5:
			cmd |= SD_RESP_R1B; break;
		case SD_CMD2:
		case SD_CMD9:
			cmd |= SD_RESP_R2; break;
		case SD_CMD1:
		case SD_ACMD41:
			cmd |= SD_RESP_R3; break;
		case SD_CMD3:
			cmd |= SD_RESP_R6; break;
		case SD_CMD6:
			cmd |= SD_RESP_R1 | SD_DAT_PRESENT; break;
			/* for MMC card it is SD_RESP_R1B */
		case SD_CMD8:
			cmd |= SD_RESP_R1; break;
			/* for MMC card it is SD_RESP_R1 | SD_DAT_PRESENT */
	}
	return cmd & 0x3FFF;
}

/*
 * send non-data command
 * data commands are a little different, use read/write instead.
 *
 * 0 = good
 * -1 = command inhibited
 * -2 = command has data but data is inhibited
 * -3 = controller reported error
 */
static int __sd_zynq_send_cmd(struct hd_device *hd, uint16_t cmd, uint16_t count,
	uint32_t arg, int mode)
{
	struct bus_device * bus = hd->bus;
	bus_write_fp w16 = bus->bus_driver.get_write_fp(bus, 16);
	bus_write_fp w32 = bus->bus_driver.get_write_fp(bus, 32);
	bus_read_fp r16 = bus->bus_driver.get_read_fp(bus, 16);
	bus_read_fp r32 = bus->bus_driver.get_read_fp(bus, 32);

	uint64_t tmp;
	uint16_t tmp16;
	/* frame the command */
	cmd = sd_frame_cmd(cmd);
	/* do a state check */
	r32(bus, hd->base, SD_PRES_STATE_OFFSET, &tmp);
	if (tmp & SD_PSR_INHIBIT_CMD) return -1;
	if ((tmp & SD_PSR_INHIBIT_DAT) && (cmd & SD_DAT_PRESENT)) return -2;
	/* write block count */
	w16(bus, hd->base, SD_BLK_CNT_OFFSET, count);
	w16(bus, hd->base, SD_TIMEOUT_CTRL_OFFSET, 0xE);
	/* write argument */
	w32(bus, hd->base, SD_ARGMT_OFFSET, arg);
	w16(bus, hd->base, SD_NORM_INTR_STS_OFFSET, SD_NORM_INTR_ALL);
	w16(bus, hd->base, SD_ERR_INTR_STS_OFFSET, SD_ERR_INTR_ALL);
	/* set transfer mode */
	switch(mode) {
		/* DMA read */
		case 1:
			tmp16 = SD_TM_MUL_SIN_BLK_SEL | SD_TM_DAT_DIR_SEL | \
				SD_TM_AUTO_CMD12_EN | SD_TM_BLK_CNT_EN | \
				SD_TM_DMA_EN;
			break;
		/* DMA write */
		case 2:
			tmp16 = SD_TM_MUL_SIN_BLK_SEL | SD_TM_AUTO_CMD12_EN | \
				SD_TM_BLK_CNT_EN | SD_TM_DMA_EN;
			break;
		/* non-data */
		default:
			tmp16 = SD_TM_DMA_EN;
			break;
	}
	w16(bus, hd->base, SD_XFER_MODE_OFFSET, tmp16);
	/* write command */
	w16(bus, hd->base, SD_CMD_OFFSET, cmd);
	/* wait for result */
	do {
		r16(bus, hd->base, SD_NORM_INTR_STS_OFFSET, &tmp);
		if (tmp & SD_INTR_ERR) return -3;
		/* We don't read error states, and we dont't clear them. */
	} while(!(tmp & SD_INTR_CC));
	/* Clear */
	w16(bus, hd->base, SD_NORM_INTR_STS_OFFSET, SD_INTR_CC);
	return 0;
}

/*
 * initialize a memory card
 * card inserted into SD slot can be MMC, SDIO, SD(SC/HC/XC)-(memory/combo).
 * we want a SD(SC/HC)-memory here. if we see a combo, we ignore the sdio.
 * 1 = good SDHC
 * 0 = good SD(SC)
 * -1 = no card
 * -2 = error sending CMD0
 * -3 = error sending CMD8
 * -4 = CMD8 response bad
 * -5 = error sending CMD55 & ACMD41
 * -6 = error sending CMD2
 * -7 = error sending CMD3
 * -8 = error sending CMD9
 * -9 = error sending CMD7
 */
int __sd_zynq_init_card(struct hd_device *hd)
{
	struct bus_device * bus = hd->bus;
	bus_read_fp r32 = bus->bus_driver.get_read_fp(bus, 32);

	uint64_t state, resp;
	int ret;
	/* check card */
	r32(bus, hd->base, SD_PRES_STATE_OFFSET, &state);
	if (!(state & SD_PSR_CARD_INSRT)) return -1;
	/* wait 74 clocks (of sd controller). */
	udelay(2000);
	/* CMD0 */
	ret = __sd_zynq_send_cmd(hd, SD_CMD0, 0, 0, 0);
	if (ret) return -2;
	/* CMD8 */
	ret = __sd_zynq_send_cmd(hd, SD_CMD8, 0, SD_CMD8_VOL_PATTERN, 0);
	if (ret) return -3;
	r32(bus, hd->base, SD_RESP0_OFFSET, &resp);
	if (resp != SD_CMD8_VOL_PATTERN) return -4;
	/* CMD55 & ACMD41 */
	do {
		ret = __sd_zynq_send_cmd(hd, SD_CMD55, 0, 0, 0);
		if (ret) return -5;
		ret = __sd_zynq_send_cmd(hd, SD_ACMD41, 0, \
			(SD_ACMD41_HCS | SD_ACMD41_3V3), 0);
		if (ret) return -5;
		r32(bus, hd->base, SD_RESP0_OFFSET, &resp);
	} while (!(resp & SD_RESP_READY));
	/* SD or SDHC? */
	if (resp & SD_ACMD41_HCS) cardtype = 1; /* SDHC */
	else cardtype = 0; /* SD(SC) */
	/* assume S18A(OR) good and go on to CMD2 */
	ret = __sd_zynq_send_cmd(hd, SD_CMD2, 0, 0, 0);
	if (ret) return -6;
	/* response0-3 contains cardID */
	/* CMD3 */
	do {
		ret = __sd_zynq_send_cmd(hd, SD_CMD3, 0, 0, 0);
		if (ret) return -7;
		r32(bus, hd->base, SD_RESP0_OFFSET, &resp);
		resp &= 0xFFFF0000;
	} while (resp == 0);
	/* response0(high 16bit) contains card RCA */
	/* CMD9 for specs, we don't use this now */
	ret = __sd_zynq_send_cmd(hd, SD_CMD9, 0, resp, 0);
	if (ret) return -8;
	/* response0-3 contains cardSpecs */
	/* CMD7 */
	ret = __sd_zynq_send_cmd(hd, SD_CMD7, 0, resp, 0);
	if (ret) return -9;
	return cardtype;
}

/*
 * read block from memory card
 * utilize basic DMA (known as SDMA in documents)
 * processor will spin to wait.
 *
 * pa = physical address
 * count = block count
 * offset = starting offset on card.
 *
 **********************************************
 * WARNING: OFFSET IS IN BYTES ON SD(SC) CARD *
 * BUT IN 512-BYTE BLOCKS ON SDHC CARD!       *
 **********************************************
 *
 * To make cross-platform design easier, we allow 512-byte blocks only.
 *
 * return values:
 * 0 = good
 * -1 = no card
 * -2 = error sending CMD18
 * -3 = error during DMA transfer
 *
 * FIXME add support for cross-page dma
 */
static int __sd_zynq_read(struct hd_device *hd, uint32_t pa, uint16_t count,
	uint32_t offset, int wait)
{
	struct bus_device * bus = hd->bus;
	bus_read_fp r16 = bus->bus_driver.get_read_fp(bus, 16);
	bus_read_fp r32 = bus->bus_driver.get_read_fp(bus, 32);
	bus_write_fp w16 = bus->bus_driver.get_write_fp(bus, 16);
	bus_write_fp w32 = bus->bus_driver.get_write_fp(bus, 32);

	int ret;
	uint64_t state16;
	uint64_t state32;
	/* check card */
	r32(bus, hd->base, SD_PRES_STATE_OFFSET, &state32);
	if (!(state32 & SD_PSR_CARD_INSRT)) return -1;
	/* block size set to 512 during controller init, skipping check */
	/* SD card uses byte addressing */
	if (cardtype == 0) {
		offset *= 512;
	}
	/* write address */
	w32(bus, hd->base, SD_SDMA_SYS_ADDR_OFFSET, pa);
	/* CMD18 with auto_cmd12 */
	ret = __sd_zynq_send_cmd(hd, SD_CMD18, count, offset, 1);
	if (ret) return -2;
	if (wait) {
		/* wait for transfer complete */
		do {
			r16(bus, hd->base, SD_NORM_INTR_STS_OFFSET, &state16);
			if (state16 & SD_INTR_ERR) {
				w16(bus, hd->base, SD_ERR_INTR_STS_OFFSET, \
					SD_ERR_INTR_ALL);
				return -3;
			}
		} while (!(state16 & SD_INTR_TC));
		/* clean up */
		w16(bus, hd->base, SD_NORM_INTR_STS_OFFSET, SD_INTR_TC);
	}
	return 0;
}

/*
 * write block to memory card
 * utilize basic DMA (known as SDMA in documents)
 * processor will spin to wait.
 *
 * pa = physical address
 * count = block count
 * offset = starting offset on card.
 *
 **********************************************
 * WARNING: OFFSET IS IN BYTES ON SD(SC) CARD *
 * BUT IN 512-BYTE BLOCKS ON SDHC CARD!       *
 **********************************************
 *
 * To make cross-platform design easier, we allow 512-byte blocks only.
 *
 * return values:
 * 0 = good
 * -1 = no card
 * -2 = error sending CMD25
 * -3 = error during DMA transfer
 */
int __sd_zynq_write(struct hd_device *hd, uint32_t pa, uint16_t count,
	uint32_t offset, int wait)
{
	struct bus_device * bus = hd->bus;
	bus_read_fp r16 = bus->bus_driver.get_read_fp(bus, 16);
	bus_read_fp r32 = bus->bus_driver.get_read_fp(bus, 32);
	bus_write_fp w16 = bus->bus_driver.get_write_fp(bus, 16);
	bus_write_fp w32 = bus->bus_driver.get_write_fp(bus, 32);

	int ret;
	uint64_t state16;
	uint64_t state32;
	/* check card */
	r32(bus, hd->base, SD_PRES_STATE_OFFSET, &state32);
	if (!(state32 & SD_PSR_CARD_INSRT)) return -1;
	/* block size set to 512 during controller init, skipping check */
	/* SD card uses byte addressing */
	if (cardtype == 0) {
		offset *= 512;
	}
	/* write address */
	w32(bus, hd->base, SD_SDMA_SYS_ADDR_OFFSET, pa);
	/* CMD25 with auto_cmd12 */
	ret = __sd_zynq_send_cmd(hd, SD_CMD25, count, offset, 1);
	if (ret) return -2;
	if (wait) {
		/* wait for transfer complete */
		do {
			r16(bus, hd->base, SD_NORM_INTR_STS_OFFSET, &state16);
			if (state16 & SD_INTR_ERR) {
				w16(bus, hd->base, SD_ERR_INTR_STS_OFFSET, \
					SD_ERR_INTR_ALL);
				return -3;
			}
		} while (!(state16 & SD_INTR_TC));
		/* clean up */
		w16(bus, hd->base, SD_NORM_INTR_STS_OFFSET, SD_INTR_TC);
	}
	return 0;
}

static void enable_interrupt(struct hd_device *hd)
{
	struct bus_device * bus = hd->bus;
	bus_write_fp w16 = bus->bus_driver.get_write_fp(bus, 16);

	w16(bus, hd->base, SD_NORM_INTR_STS_EN_OFFSET,
		SD_INTR_TC | SD_INTR_CC | SD_INTR_ERR | SD_INTR_DMA);
}

static int __check_error_ack_interrupt(struct hd_device *hd)
{
	struct bus_device * bus = hd->bus;
	bus_read_fp r16 = bus->bus_driver.get_read_fp(bus, 16);
	bus_write_fp w16 = bus->bus_driver.get_write_fp(bus, 16);
	uint64_t state16;

	r16(bus, hd->base, SD_NORM_INTR_STS_OFFSET, &state16);
	if (state16 & SD_INTR_ERR) {
		w16(bus, hd->base, SD_ERR_INTR_STS_OFFSET, SD_ERR_INTR_ALL);
		return 0;
	} else if (state16 & SD_INTR_DMA) {
		return 2;
	} else {
		w16(bus, hd->base, SD_NORM_INTR_STS_OFFSET, SD_INTR_TC);
		return 1;
	}
}

#ifdef RAW /* baremetal driver */

/* FIXME zedboard uses SD0 only */
#define SD_BASE	SD0_PHYSBASE

static struct hd_device __early_sd_zynq;

void sd_init()
{
	__early_sd_zynq.base = SD_BASE;
	__early_sd_zynq.bus = &early_memory_bus;
	__sd_zynq_init(&__early_sd_zynq);
}

int sd_init_card()
{
	return __sd_zynq_init_card(&__early_sd_zynq);
}

int sd_read(uint32_t pa, uint16_t count, uint32_t offset)
{
	return __sd_zynq_read(&__early_sd_zynq, pa, count, offset, true);
}

int sd_write(uint32_t pa, uint16_t count, uint32_t offset)
{
	return __sd_zynq_write(&__early_sd_zynq, pa, count, offset, true);
}

#else /* not RAW, or kernel driver */

#define DEVICE_MODEL "sd-zynq"

/* forward */
static struct blk_driver drv;

static void init(struct hd_device *hd)
{
	kpdebug("initializing SD controller\n");
	__sd_zynq_init(hd);
	kpdebug("initializing SD(SC/HC) card\n");
	__sd_zynq_init_card(hd);
	kpdebug("initialization done\n");
}

static int open(dev_t dev, int mode, struct proc *p)
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

static int close(dev_t dev, int oflags, struct proc *p)
{
	/*
	 * Since we only have one hard disk (which is root), do we really
	 * want to "close" it? (FIXME)
	 */
	kpdebug("sd-zynq: closing\n");
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
		__sd_zynq_write(dev, kva2pa(ULCAST(data)), bp->nbytesrem/512, blkno, false);
	} else if (bp->flags & B_INVALID) {
		kpdebug("reading from %d to %p\n", blkno, data);
		__sd_zynq_read(dev, kva2pa(ULCAST(data)), bp->nbytesrem/512, blkno, false);
	}
}

static int strategy(struct buf *bp)
{
	struct hd_device *hd;
	unsigned long flags;

	hd = (struct hd_device *)dev_from_id(hdbasedev(bp->devno));
	spin_lock_irq_save(&hd->lock, flags);

	assert(bp->flags & B_BUSY);
	if (!(bp->flags & (B_DIRTY | B_INVALID))) {
		kpdebug("sd-zynq: nothing to do\n");
		spin_unlock_irq_restore(&hd->lock, flags);
		return 0;
	}
	kpdebug("sd-zynq: queuing buf %p with %d bytes at %p\n",
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

static int intr(int irq)
{
	struct hd_device *hd;
	struct buf *bp;
	unsigned long flags;
	int state;

	hd = (struct hd_device *)dev_from_id(hdbasedev(rootdev));
	kpdebug("sd interrupt\n");

	spin_lock_irq_save(&hd->lock, flags);

	if (list_empty(&hd->bufqueue)) {
		kpdebug("sd spurious interrupt\n");
		//__check_error_ack_interrupt(hd);
		spin_unlock_irq_restore(&hd->lock, flags);
		return 0;
	}

	bp = list_first_entry(&hd->bufqueue, struct buf, ionode);
	state = __check_error_ack_interrupt(hd);
	assert(bp->flags & B_BUSY);
	assert(bp->flags & (B_INVALID | B_DIRTY));
	if (state == 0) {
		bp->errno = -EIO;
		bp->flags |= B_ERROR;
		kpdebug("fail buf %p\n", bp);
		list_del(&(bp->ionode));
		biodone(bp);
		//__startnext(hd);
		spin_unlock_irq_restore(&hd->lock, flags);
		return 0;
	} else if (state == 2) {
		struct bus_device * bus = hd->bus;
		bus_read_fp r32 = bus->bus_driver.get_read_fp(bus, 32);
		bus_write_fp w32 = bus->bus_driver.get_write_fp(bus, 32);
		uint64_t pa;

		r32(bus, hd->base, SD_SDMA_SYS_ADDR_OFFSET, &pa);
		w32(bus, hd->base, SD_SDMA_SYS_ADDR_OFFSET, pa);
		spin_unlock_irq_restore(&hd->lock, flags);
		return 0;
	}
	if (!(bp->flags & B_DIRTY) && (bp->flags & B_INVALID)) {
		kpdebug("fetching to %p\n",
		    bp->data + bp->nbytes - bp->nbytesrem);
		//__fetch(hd, bp);
	} else if (bp->flags & B_DIRTY) {
		/* Write finished, decrement one sector */
		bp->nbytesrem -= SECTOR_SIZE;
	}

	kpdebug("buf %p remain %d\n", bp, bp->nbytesrem);
	if (bp->nbytesrem == 0) {
		kpdebug("done buf %p\n", bp);
		list_del(&(bp->ionode));
		biodone(bp);
		/* We start the next request only if the current request is
		 * done or failed */
		//__startnext(hd);
	} else if (bp->flags & B_DIRTY) {
		/* Send next sector to PIO if not finished */
		//__send_one_sector(hd, bp->data + bp->nbytes - bp->nbytesrem);
	}
	spin_unlock_irq_restore(&hd->lock, flags);
	return 0;
}

static int new(struct devtree_entry *entry)
{
	struct hd_device *hd;
	int mapped = 0;

	if (strcmp(entry->model, DEVICE_MODEL) != 0)
		return -ENOTSUP;
	if (strcmp(entry->name, "sd1") == 0)
		return -ENOTSUP;
	kpdebug("<sd-zynq> initializing device %s.\n", entry->name);
	/* need to kmmap if parent is memory. */
	void *vaddr;
	if (strcmp(entry->parent, "memory") == 0) {
		mapped = 1;
		vaddr = kmmap(NULL, entry->regs[0], PAGE_SIZE, MAP_SHARED_DEV);
		if (vaddr == NULL) return -ENOMEM;
	} else {
		vaddr = (void *)ULCAST(entry->regs[0]);
	}
	/* allocate */
	hd = kmalloc(sizeof(*hd), GFP_ZERO);
	if (hd == NULL) {
		if (mapped) kmunmap(vaddr);
		return -ENOMEM;
	}
	/* assuming only one card... */
	initdev(hd, DEVCLASS_BLK, entry->name,
	    make_hdbasedev(SD_MAJOR, 0), &drv);
	hd->bus = (struct bus_device *)dev_from_name(entry->parent);
	hd->base = ULCAST(vaddr);
	hd->nregs = entry->nregs;	/* assuming dev tree can't go wrong */
	list_init(&(hd->bufqueue));
	dev_add(hd);
	init(hd);
	enable_interrupt(hd);
	add_interrupt_handler(intr, entry->irq);
	kpdebug("<sd-zynq> device registered.\n");
	return 0;
}

static struct blk_driver drv = {
	.class = DEVCLASS_BLK,
	.open = open,
	.close = close,
	.strategy = strategy,
	.new = new
};

static int __init(void)
{
	kputs("KERN: <sd-zynq> Initializing.\n");
	register_driver(SD_MAJOR, &drv);
	kputs("KERN: <sd-zynq> Ready.\n");
	return 0;
}
INITCALL_DRIVER(__init)

#endif /* RAW */

