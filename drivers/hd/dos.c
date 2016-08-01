
#include <bootsect.h>
#include <drivers/hd/hd.h>
#include <fs/vnode.h>
#include <fs/specdev.h>
#include <fs/bio.h>
#include <buf.h>
#include <aim/initcalls.h>
#include <libc/string.h>
#include <panic.h>

/*
 * How to detect if a DOS partition is unused:
 * http://wiki.osdev.org/Partition_Table
 */
static inline bool __valid_dos_partition(struct mbr *mbr, int i)
{
	struct mbr_part_entry *entry = &(mbr->part_entry[i]);
	/* currently we do not detect illegal CHS values */
	return !(entry->type == 0 || entry->sector_count == 0);
}

static int detect_dos_partitions(struct hd_device *dev)
{
	struct mbr mbr;
	int i, j = 0;
	struct vnode *vnode;
	struct buf *buf;
	struct blk_driver *drv;

	kpdebug("detecting DOS partitions\n");

	drv = (struct blk_driver *)devsw[major(dev->devno)];
	buf = bgetempty(SECTOR_SIZE);
	buf->blkno = 0;
	buf->devno = dev->devno;
	drv->strategy(buf);
	biowait(buf);
	assert(!(buf->flags & (B_ERROR | B_EINTR)));

	kpdebug("biowait done\n");

	memcpy(&mbr, buf->data, SECTOR_SIZE);
	if (mbr.signature[0] != 0x55 ||
	    mbr.signature[1] != 0xaa)
		return -1;

	for (i = 1; i <= MAX_PRIMARY_PARTITIONS; ++i) {
		if (__valid_dos_partition(&mbr, i - 1)) {
			dev->part[i].offset =
			    mbr.part_entry[i - 1].first_sector_lba;
			dev->part[i].len =
			    mbr.part_entry[i - 1].sector_count;
		} else {
			dev->part[i].offset = dev->part[i].len = 0;
		}
		kpdebug("Partition detected: %d %d %d\n", i, dev->part[i].offset, dev->part[i].len);
	}

	brelse(buf);

	return 0;
}

static int __dos_partition_table_init(void)
{
	register_partition_table(detect_dos_partitions);
	return 0;
}
INITCALL_DRIVER(__dos_partition_table_init);

