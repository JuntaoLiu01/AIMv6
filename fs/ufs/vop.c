
#include <buf.h>
#include <fs/bio.h>
#include <fs/vnode.h>
#include <fs/specdev.h>
#include <fs/mount.h>
#include <fs/ufs/inode.h>
#include <fs/ufs/ufsmount.h>
#include <panic.h>

/*
 * Execute an I/O request on a UFS vnode.
 *
 * We do it by translating the block I/O request on UFS to another block I/O
 * request on the device the UFS is mounted on.  That is, we only need to
 * translate logical block numbers to physical sector numbers on the
 * partition.
 */
int
ufs_strategy(struct buf *bp)
{
	struct vnode *vp = bp->vnode;
	struct inode *ip = VTOI(vp);
	unsigned long flags;
	int err;

	assert(vp->type != VBLK && vp->type != VCHR);

	/*
	 * NOTE: We only support reading one logical block at a time here.
	 * If we are going to support reading multiple logical blocks, we
	 * will have to deal with physically incontiguous sectors.
	 *
	 * VOP_BMAP has an additional return value, @runp, to indicate how many
	 * physically contiguous sectors are there starting from physical
	 * address of logical block @lblkno.
	 *
	 * So, a VOP_STRATEGY supporting multiple logical block I/O should
	 * repeatedly probe for the longest physically contiguous sectors,
	 * bread() them, and advance to the next one.  But such implementation
	 * would introduce additional complexity to the already-hellish
	 * file system framework, so I didn't do that (it's for teaching
	 * purpose after all).
	 */
	assert(bp->nbytes == (SECTOR_SIZE << VFSTOUFS(vp->mount)->fsbtodb));

	if (bp->blkno == BLKNO_INVALID) {
		err = VOP_BMAP(vp, bp->lblkno, NULL, &(bp->blkno), NULL);
		if (err) {
			bp->errno = err;
			bp->flags |= B_ERROR;
			local_irq_save(flags);
			biodone(bp);
			local_irq_restore(flags);
			return err;
		}
	}
	assert(bp->blkno != BLKNO_INVALID);
	if (bp->blkno == 0) {
		/*
		 * We have a hole here, zero the buffer and return.
		 * NOTE: when writing, be very sure that the logical block #
		 * has a physical sector # mapped.
		 */
		local_irq_save(flags);
		memset(bp->data, 0, bp->nbytes);
		biodone(bp);
		local_irq_restore(flags);
		return 0;
	}
	vp = ip->ufsmount->devvp;
	bp->devno = vp->specinfo->devno;
	/* Here, we are actually calling spec_strategy() */
	return (vp->ops->strategy)(bp);
}

