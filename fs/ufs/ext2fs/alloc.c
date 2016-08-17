
#include <fs/vnode.h>
#include <fs/bio.h>
#include <fs/mount.h>
#include <fs/vfs.h>
#include <fs/ufs/inode.h>
#include <fs/ufs/ufsmount.h>
#include <fs/ufs/ext2fs/ext2fs.h>
#include <fs/ufs/ext2fs/dir.h>
#include <fs/ufs/ext2fs/dinode.h>
#include <bitmap.h>
#include <panic.h>
#include <errno.h>

/*
 * A simple brute-force search algorithm for available inode number.
 * Returns VGET'd vnode.
 * Changes in-memory superblock (including group descriptors) as well
 * as in-memory inode structure.
 * Changes and writes inode bitmap to disk.
 */
int
ext2fs_inode_alloc(struct vnode *dvp, int imode, struct ucred *cred,
    struct vnode **vpp)
{
	struct vnode *devvp = VFSTOUFS(dvp->mount)->devvp;
	struct vnode *vp;	/* the vnode of allocated inode */
	struct inode *pip;	/* inode of @dvp */
	struct inode *ip;	/* the allocated inode */
	struct m_ext2fs *fs;
	struct buf *bp_ibitmap;
	ufsino_t ino = 0, ibase;
	int err, avail, i;

	pip = VTOI(dvp);
	fs = pip->superblock;
	if (fs->e2fs.ficount == 0)
		return -ENOSPC;

	vlock(devvp);
	for (i = 0; i < fs->ncg; ++i) {
		ibase = fs->e2fs.ipg * i;
		err = bread(devvp, fsbtodb(fs, fs->gd[i].i_bitmap), fs->bsize,
		    &bp_ibitmap);
		if (err) {
			brelse(bp_ibitmap);
			vunlock(devvp);
			return err;
		}
		/* Search for available ino in this cylinder group.
		 * Note that bitmap_xxx are 1-based. */
		avail = bitmap_find_first_zero_bit(bp_ibitmap->data,
		    fs->e2fs.ipg);
		while (avail < EXT2_FIRSTINO && avail != 0 && i == 0) {
			/* if ino is below FIRSTINO, discard and continue */
			avail = bitmap_find_next_zero_bit(bp_ibitmap->data,
			    fs->e2fs.ipg, avail);
		}
		if (avail == 0) {
			brelse(bp_ibitmap);
			continue;
		}
		ino = avail + ibase;
		break;
		/* NOTREACHED */
	}
	if (ino == 0) {
		/* No free inodes in bitmap but superblock says there ARE free
		 * inodes.  Here, we don't attempt to fix it. */
		kprintf("%s: inconsistent inode bitmap and superblock\n", __func__);
		vunlock(devvp);
		return -ENOSPC;
	}

	/* We do writes first. */
	atomic_set_bit(avail, bp_ibitmap->data);
	fs->e2fs.ficount--;
	fs->gd[i].nifree--;
	fs->fmod = 1;
	if ((imode & EXT2_IFMT) == EXT2_IFDIR)
		fs->gd[i].ndirs++;
	err = bwrite(bp_ibitmap);
	brelse(bp_ibitmap);
	vunlock(devvp);
	if (err)
		return err;

	/* VGET the inode.  If we can't, unwind the inode bitmap. */
	err = VFS_VGET(dvp->mount, ino, &vp);
	if (err)
		goto rollback_ibitmap;
	ip = VTOI(vp);
	if (EXT2_DINODE(ip)->mode && EXT2_DINODE(ip)->nlink > 0)
		panic("%s: dup alloc mode 0%o, nlinks %u, inum %u\n",
		    __func__, EXT2_DINODE(ip)->mode, EXT2_DINODE(ip)->nlink,
		    ip->ino);
	memset(EXT2_DINODE(ip), 0, sizeof(*EXT2_DINODE(ip)));

	*vpp = vp;
	return 0;
rollback_ibitmap:
	ext2fs_inode_free(VTOI(dvp), ino, imode);
	return err;
}

/*
 * Does the reverse of ext2fs_inode_alloc.
 * Changes in-memory superblock.
 * Does NOT change in-memory inode (do we need to?)
 * Changes and writes inode bitmap to disk
 */
void
ext2fs_inode_free(struct inode *ip, ufsino_t ino, int imode)
{
	struct m_ext2fs *fs;
	void *ibp;
	struct buf *bp;
	int err, cg;
	struct vnode *devvp = ip->ufsmount->devvp;

	fs = ip->superblock;
	assert(ino <= fs->e2fs.icount);
	assert(ino >= EXT2_FIRSTINO);
	cg = ino_to_cg(fs, ino);
	err = bread(devvp, fsbtodb(fs, fs->gd[cg].i_bitmap), fs->bsize, &bp);
	if (err) {
		brelse(bp);
		return;
	}
	ibp = bp->data;
	ino = (ino - 1) % fs->e2fs.ipg + 1;
	if (!bitmap_test_bit(ino, ibp))
		panic("%s: freeing free inode %u on %x\n", __func__, ino,
		    ip->devno);
	atomic_clear_bit(ino, ibp);
	fs->e2fs.ficount++;
	fs->gd[cg].nifree++;
	if ((imode & EXT2_IFMT) == EXT2_IFDIR)
		fs->gd[cg].ndirs--;
	fs->fmod = 1;
	bwrite(bp);
	brelse(bp);
}

/*
 * A brute-force searching algorithm for available file system block.
 * Rather uninteresting since the algorithm is largely the same as inode_alloc.
 */
int
ext2fs_blkalloc(struct inode *ip, struct ucred *cred, off_t *fsblkp)
{
	struct vnode *devvp = ip->ufsmount->devvp;
	struct m_ext2fs *fs = ip->superblock;
	struct buf *bp_bbitmap;
	int i, err, avail;
	off_t fsblk = 0, bbase;

	if (fs->e2fs.fbcount == 0)
		return -ENOSPC;

	kpdebug("ext2fs blkalloc for inode %u\n", ip->ino);
	vlock(devvp);
	for (i = 0; i < fs->ncg; ++i) {
		bbase = fs->e2fs.fpg * i + fs->e2fs.first_dblock;
		err = bread(devvp, fsbtodb(fs, fs->gd[i].b_bitmap), fs->bsize,
		    &bp_bbitmap);
		if (err) {
			brelse(bp_bbitmap);
			vunlock(devvp);
			return err;
		}
		/* Search for available block in this cylinder group. */
		avail = bitmap_find_first_zero_bit(bp_bbitmap->data,
		   fs->e2fs.bpg);
		if (avail == -1) {
			brelse(bp_bbitmap);
			continue;
		}
		/* Subtract 1 because block # are zero-based */
		fsblk = avail - 1 + bbase;
		break;
		/* NOTREACHED */
	}
	if (fsblk == 0) {
		kprintf("%s: inconsistent block bitmap and superblock\n",
		    __func__);
		vunlock(devvp);
		return -ENOSPC;
	}

	/* Now we do the writes */
	atomic_set_bit(avail, bp_bbitmap->data);
	err = bwrite(bp_bbitmap);
	brelse(bp_bbitmap);
	vunlock(devvp);
	if (err)
		return err;
	/* Update superblock as well as inode */
	fs->e2fs.fbcount--;
	fs->gd[i].nbfree--;
	fs->fmod = 1;
	ip->nsect += fs->bsects;
	ip->flags |= IN_CHANGE | IN_UPDATE;

	*fsblkp = fsblk;
	kpdebug("ext2fs blkalloc found %lu for inode %u\n", fsblk, ip->ino);
	return 0;
}

void
ext2fs_blkfree(struct inode *ip, off_t fsblk)
{
	struct vnode *devvp = ip->ufsmount->devvp;
	struct m_ext2fs *fs = ip->superblock;
	int cg = dtog(fs, fsblk);
	int err, bno;
	struct buf *bp;

	assert(fsblk < fs->e2fs.bcount);
	kpdebug("ext2fs blkfree freeing %lu for inode %u\n", fsblk, ip->ino);
	err = bread(devvp, fsbtodb(fs, fs->gd[cg].b_bitmap), fs->bsize, &bp);
	if (err) {
		brelse(bp);
		return;
	}
	bno = dtogd(fs, fsblk);
	if (!bitmap_test_bit(bno + 1, bp->data))
		panic("%s: freeing free block %u on %x\n", __func__, fsblk,
		    ip->devno);
	atomic_clear_bit(bno + 1, bp->data);
	fs->e2fs.fbcount++;
	fs->gd[cg].nbfree++;
	fs->fmod = 1;
	ip->nsect -= fs->bsects;
	ip->flags |= IN_CHANGE | IN_UPDATE;
	bwrite(bp);
	brelse(bp);
	kpdebug("ext2fs blkfree freed %lu for inode %u\n", fsblk, ip->ino);
}

