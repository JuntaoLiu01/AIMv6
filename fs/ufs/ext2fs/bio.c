
#include <fs/vnode.h>
#include <fs/bio.h>
#include <fs/ufs/inode.h>
#include <fs/ufs/ufsmount.h>
#include <fs/ufs/ext2fs/ext2fs.h>
#include <fs/ufs/ext2fs/dinode.h>
#include <ucred.h>
#include <libc/string.h>
#include <panic.h>
#include <errno.h>

/*
 * Allocate a buf for file with inode structure @ip at file logical block
 * number @lblkno, and read the contents inside.  If @lblkno does not
 * correspond to a file system block, allocate a new one (and possibly some
 * indirect blocks).
 *
 * This function NEITHER takes care of NOR check the inode file size.
 *
 * @ip, superblock, and group descriptors (block bitmap in particular) would
 * be modified.
 */
int
ext2fs_buf_alloc(struct inode *ip, off_t lblkno, struct ucred *cred,
    struct buf **bpp)
{
	struct vnode *vp = ITOV(ip);
	struct vnode *devvp = ip->ufsmount->devvp;
	struct m_ext2fs *fs = ip->superblock;
	struct buf *bp, *ibp;
	int level, alloced = 0, used = 0, i, err;
	int offsets[NIADDR];
	off_t fsblks[NIADDR + 1];
	off_t path[NIADDR + 1];
	off_t nb, fsblk;

	kpdebug("ext2fs buf alloc inode %u lblkno %lu\n", ip->ino, lblkno);

	/* Things are easy if we are addressing direct blocks (with @lblkno
	 * < NDADDR). */
	if (lblkno < NDADDR) {
		fsblk = EXT2_DINODE(ip)->blocks[lblkno];
		if (fsblk != 0) {
			/* We already have the block, read it and return */
			err = bread(devvp, fsbtodb(fs, fsblk), fs->bsize, &bp);
			if (err) {
				brelse(bp);
				return err;
			}
			*bpp = bp;
			return 0;
		}
		/* Otherwise, we need to allocate a direct block. */
		err = ext2fs_blkalloc(ip, cred, &fsblk);
		if (err)
			return err;

		/* Retrieve struct buf */
		bp = bget(vp, lblkno, fs->bsize);
		if (bp == NULL) {
			ext2fs_blkfree(ip, fsblk);
			return -ENOMEM;
		}
		memset(bp->data, 0, fs->bsize);

		/* Finish up */
		EXT2_DINODE(ip)->blocks[lblkno] = fsblk;
		ip->flags |= IN_CHANGE | IN_UPDATE;
		*bpp = bp;
		return 0;
	}
	/* Determine the # of levels of indirection */
	level = ext2fs_indirs(ip, lblkno, offsets);
	if (level < 0)
		return level;	/* level = error */
	assert(level != 0);

	/* Fetch or allocate the first indirect block */
	nb = EXT2_DINODE(ip)->blocks[NDADDR + level - 1];
	if (nb == 0) {
		/* need allocation */
		err = ext2fs_blkalloc(ip, cred, &fsblk);
		if (err)
			return err;
		kpdebug("ext2fs bufalloc first indir block at %ld\n", fsblk);
		/* clean the indirect block so that it never contain garbage */
		ibp = bget(devvp, fsbtodb(fs, fsblk), fs->bsize);
		if (ibp == NULL) {
			ext2fs_blkfree(ip, fsblk);
			return -ENOMEM;
		}
		memset(ibp->data, 0, fs->bsize);
		err = bwrite(ibp);
		brelse(ibp);
		if (err != 0) {
			ext2fs_blkfree(ip, fsblk);
			return err;
		}
		/* record the block we allocated for rolling back in case of
		 * something miserable happens */
		EXT2_DINODE(ip)->blocks[NDADDR + level - 1] = fsblk;
		nb = fsblks[alloced++] = fsblk;
	}
	path[used++] = nb;

	/* Fetch or allocate the data block and rest of indirect blocks */
	for (i = 0; i < level; ++i) {
		err = bread(devvp, fsbtodb(fs, nb), fs->bsize, &bp);
		if (err) {
			brelse(bp);
			goto fail;
		}
		nb = ((uint32_t *)bp->data)[offsets[i]];

		/*
		 * This code block largely resembles the "Fetch or allocate
		 * the first indirect block" code above.
		 */
		if (nb == 0) {
			/* need allocation */
			err = ext2fs_blkalloc(ip, cred, &fsblk);
			if (err) {
				brelse(bp);
				goto fail;
			}
			kpdebug("ext2fs bufalloc new block %ld\n", fsblk);
			/* clean the block */
			ibp = bget(devvp, fsbtodb(fs, fsblk), fs->bsize);
			if (ibp == NULL) {
				err = -ENOMEM;
				ext2fs_blkfree(ip, fsblk);
				brelse(bp);
				goto fail;
			}
			memset(ibp->data, 0, fs->bsize);
			err = bwrite(ibp);
			brelse(ibp);
			if (err != 0) {
				err = -1;
				ext2fs_blkfree(ip, fsblk);
				brelse(bp);
				goto fail;
			}
			/*
			 * Here's a different thing to do: write the file
			 * system block number into the previous-level
			 * indirect block (with buf @bp) at given index.
			 */
			((uint32_t *)bp->data)[offsets[i]] = fsblk;
			if ((err = bwrite(bp)) != 0) {
				ext2fs_blkfree(ip, fsblk);
				brelse(bp);
				goto fail;
			}
			ip->flags |= IN_CHANGE | IN_UPDATE;
			/* record the allocated file system block */
			nb = fsblks[alloced++] = fsblk;
		}
		brelse(bp);
		path[used++] = nb;
	}

	/* Now this is the data block we want. */
	err = bread(devvp, fsbtodb(fs, nb), fs->bsize, &bp);
	if (err) {
		brelse(bp);
		goto fail;
	}
	*bpp = bp;
	return 0;

fail:
	for (--alloced; alloced >= 0; --alloced) {
		ext2fs_blkfree(ip, fsblks[alloced]);
		--used;
		if (used == 0) {
			/* rollback the indirect block in inode structure */
			EXT2_DINODE(ip)->blocks[NDADDR + level - 1] = 0;
		} else {
			assert(i > 0);
			/* rollback intermediate indirect blocks */
			err = bread(devvp, fsbtodb(fs, path[used - 1]),
			    fs->bsize, &bp);
			if (err)
				goto epic_fail;
			((uint32_t *)bp->data)[offsets[--i]] = 0;
			if ((err = bwrite(bp)) != 0)
				goto epic_fail;
			brelse(bp);
		}
	}
	*bpp = NULL;
	return err;
epic_fail:
	panic("%s: can't unwind block %d\n", __func__, fsblks[alloced]);
	/* NOTREACHED */
	return 0;
}

/*
 * Frees a logical block in a file.  Does NOT check whether the block exists
 * or not (in which case the function simply returns).
 */
int
ext2fs_lblkfree(struct inode *ip, off_t lblkno, struct ucred *cred)
{
	struct m_ext2fs *fs = ip->superblock;
	struct vnode *devvp = ip->ufsmount->devvp;
	int level;
	int offsets[NIADDR];
	off_t path[NIADDR + 1];
	struct buf *bp;
	int i, err = 0;

	kpdebug("ext2fs lblkfree %ld %ld\n", ip->ino, lblkno);

	if (lblkno < NDADDR) {
		if (EXT2_DINODE(ip)->blocks[lblkno] == 0)
			return 0;
		ext2fs_blkfree(ip, EXT2_DINODE(ip)->blocks[lblkno]);
		EXT2_DINODE(ip)->blocks[lblkno] = 0;
		goto finalize;
	}

	level = ext2fs_indirs(ip, lblkno, offsets);
	path[0] = EXT2_DINODE(ip)->blocks[NDADDR + level - 1];
	if (path[0] == 0)
		return 0;

	for (i = 0; i < level; ++i) {
		err = bread(devvp, fsbtodb(fs, path[i]), fs->bsize, &bp);
		if (err) {
			brelse(bp);
			goto finalize;
		}
		path[i + 1] = ((uint32_t *)bp->data)[offsets[i]];
		brelse(bp);
		if (path[i + 1] == 0)
			return 0;
	}

	ext2fs_blkfree(ip, path[level]);

	for (i = level - 1; i >= 0; --i) {
		err = bread(devvp, fsbtodb(fs, path[i]), fs->bsize, &bp);
		if (err) {
			brelse(bp);
			goto finalize;
		}
		((uint32_t *)bp->data)[offsets[i]] = 0;
		bwrite(bp);
		for (int j = 0; j < NINDIR(fs); ++j) {
			if (((uint32_t *)bp->data)[j] != 0) {
				brelse(bp);
				goto finalize;
			}
		}
		brelse(bp);
		ext2fs_blkfree(ip, path[i]);
	}

	EXT2_DINODE(ip)->blocks[NDADDR + level - 1] = 0;
finalize:
	ip->flags |= IN_CHANGE | IN_UPDATE;
	return err;
}

