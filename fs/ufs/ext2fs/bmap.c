
#include <sys/types.h>
#include <fs/ufs/inode.h>
#include <fs/ufs/ufsmount.h>
#include <fs/ufs/ext2fs/dinode.h>
#include <fs/ufs/ext2fs/ext2fs.h>
#include <fs/vnode.h>
#include <fs/bio.h>
#include <buf.h>
#include <panic.h>
#include <errno.h>

/*
 * Determine # of levels of indirection, as well as offsets into each
 * indirect block for tracing down.
 */
int
ext2fs_indirs(struct inode *ip, off_t lblkno, int *offsets)
{
	struct m_ext2fs *fs = ip->superblock;
	int levels = 1, i;
	off_t lim;

	if (lblkno < NDADDR)
		return 0;

	lblkno -= NDADDR;
	for (lim = NINDIR(fs); lblkno >= lim; lim *= NINDIR(fs), ++levels)
		lblkno -= lim;
	if (levels > 3)
		return -EFBIG;
	for (i = 0; i < levels; ++i) {
		lim /= NINDIR(fs);
		*offsets = lblkno / lim;
		lblkno %= lim;
		++offsets;
	}
	return levels;
}

int
ext2fs_bmap(struct vnode *vp, off_t lblkno, struct vnode **vpp,
    soff_t *blkno, int *runp)
{
	struct inode *ip = VTOI(vp);
	struct m_ext2fs *fs = ip->superblock;
	struct vnode *devvp = ip->ufsmount->devvp;
	soff_t fsblkno;
	int offsets[NIADDR];
	int err = 0, level, i;
	struct buf *bp;

	if (vpp != NULL)
		*vpp = devvp;
	if (blkno == NULL)
		return 0;

	/*
	 * Translate the file logical block number to file system
	 * logical block number.
	 *
	 * We deal with the simplest cases first: when the logical
	 * block number is invalid, or we can directly address it.
	 */
	if (lblkno >= ext2fs_ndatablk(ip)) {
		fsblkno = BLKNO_INVALID;
		err = -E2BIG;
		goto finish;
	} else if (lblkno < NDADDR) {
		fsblkno = EXT2_DINODE(ip)->blocks[lblkno];
		goto finish;
	}

	if ((level = ext2fs_indirs(ip, lblkno, offsets)) < 0)
		return level;	/* level = error */
	for (fsblkno = EXT2_DINODE(ip)->blocks[NDADDR + level - 1], i = 0;
	     fsblkno != 0 && level > 0;
	     --level, ++i) {
		err = bread(devvp, fsbtodb(fs, fsblkno), fs->bsize, &bp);
		if (err) {
			brelse(bp);
			return err;
		}
		fsblkno = ((uint32_t *)bp->data)[offsets[i]];
	}
	brelse(bp);

finish:
	/* Translate a file system logical block number to disk sector
	 * number */
	*blkno = (fsblkno == BLKNO_INVALID) ? BLKNO_INVALID :
	    fsbtodb(fs, fsblkno);
	return err;
}

