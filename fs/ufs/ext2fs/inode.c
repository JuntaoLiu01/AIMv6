
#include <sys/types.h>
#include <fs/bio.h>
#include <fs/ufs/inode.h>
#include <fs/ufs/ufsmount.h>
#include <fs/ufs/ext2fs/dinode.h>
#include <fs/ufs/ext2fs/ext2fs.h>
#include <errno.h>

int
ext2fs_setsize(struct inode *ip, uint64_t size)
{
	struct m_ext2fs *fs = ip->superblock;
	struct ext2fs_dinode *dp = ip->dinode;

	if (size <= fs->maxfilesize) {
		if ((dp->mode & EXT2_IFMT) == EXT2_IFREG || dp->mode == 0)
			dp->size_hi = size >> 32;
		dp->size = size;
		ip->flags |= IN_CHANGE | IN_UPDATE;
		return 0;
	}
	return -EFBIG;
}

uint64_t
ext2fs_getsize(struct inode *ip)
{
	uint64_t size = EXT2_DINODE(ip)->size;
	if ((EXT2_DINODE(ip)->mode & EXT2_IFMT) == EXT2_IFREG)
		size |= ((uint64_t)EXT2_DINODE(ip)->size_hi) << 32;
	return size;
}

/*
 * Update the on-disk inode structure.
 */
int
ext2fs_update(struct inode *ip)
{
	struct m_ext2fs *fs = ip->superblock;
	struct buf *bp;
	int err;
	void *cp;

	kpdebug("ext2fs update %p with inode %d\n", ip, ip->ino);

	EXT2FS_ITIMES(ip);
	if (!(ip->flags & IN_MODIFIED))
		return 0;
	ip->flags &= ~IN_MODIFIED;
	err = bread(ip->ufsmount->devvp, fsbtodb(fs, ino_to_fsba(fs, ip->ino)),
	    fs->bsize, &bp);
	if (err) {
		brelse(bp);
		return err;
	}

	cp = bp->data + ino_to_fsbo(fs, ip->ino) * EXT2_DINODE_SIZE(fs);

	EXT2_DINODE(ip)->uid_low = (uint16_t)ip->uid;
	EXT2_DINODE(ip)->gid_low = (uint16_t)ip->gid;
	EXT2_DINODE(ip)->uid_high = (uint16_t)(ip->uid >> 16);
	EXT2_DINODE(ip)->gid_high = (uint16_t)(ip->gid >> 16);
	EXT2_DINODE(ip)->nblock = (uint32_t)ip->nsect;
	EXT2_DINODE(ip)->nblock_hi = (uint16_t)(ip->nsect >> 32);

	e2fs_isave(fs, EXT2_DINODE(ip), cp);
	err = bwrite(bp);
	brelse(bp);
	return err;
}

static int
ext2fs_extend(struct inode *ip, size_t len, struct ucred *cred)
{
	struct m_ext2fs *fs = ip->superblock;
	off_t lbn = lblkno(fs, len - 1);
	int err;
	struct buf *bp;

	err = ext2fs_buf_alloc(ip, lbn, cred, &bp);
	if (err)
		return err;
	ext2fs_setsize(ip, len);
	brelse(bp);
	ip->flags |= IN_CHANGE | IN_UPDATE;
	return ext2fs_update(ip);
}

static int
ext2fs_shrink(struct inode *ip, size_t len, struct ucred *cred)
{
	struct m_ext2fs *fs = ip->superblock;
	int offset = lblkoff(fs, len);
	off_t lbn, end_lbn;
	size_t osize = ext2fs_getsize(ip);
	int err;
	struct buf *bp;

	assert(osize > len);

	/* Change the size in inode structure first, zeroing out the trailing
	 * bytes in the last-block-to-be in the process */
	if (offset == 0) {
		ext2fs_setsize(ip, len);
	} else {
		lbn = lblkno(fs, len);
		err = ext2fs_buf_alloc(ip, lbn, cred, &bp);
		if (err)
			return err;
		ext2fs_setsize(ip, len);
		memset(bp->data + offset, 0, fs->bsize - offset);
		bwrite(bp);
		brelse(bp);
	}

	/* Now free the unneeded blocks */
	lbn = (len == 0) ? 0 : (lblkno(fs, len - 1) + 1);
	end_lbn = lblkno(fs, osize - 1);	/* osize can't be 0 */
	/*
	 * This is silly and extremely slow, but since it's the simplest
	 * solution came into my mind I just put it here (because we
	 * do not care for efficiency in our OS).
	 *
	 * Also I didn't care about robustness here.
	 */
	for (; lbn < end_lbn; ++lbn) {
		if ((err = ext2fs_lblkfree(ip, lbn, cred)) != 0)
			return err;
	}

	return ext2fs_update(ip);
}

int
ext2fs_truncate(struct inode *ip, size_t len, struct ucred *cred)
{
	struct vnode *vp = ITOV(ip);
	size_t size = ext2fs_getsize(ip);

	kpdebug("ext2fs truncate %ld %ld->%ld\n", ip->ino, size, len);

	if (vp->type != VREG && vp->type != VDIR && vp->type != VLNK)
		return 0;

	if (vp->type == VLNK)
		/* TODO: only allow full truncate (i.e. delete) here */
		panic("%s: symlink truncate NYI\n", __func__);

	/* Refresh time stamps if len == size */
	if (size == len) {
		ip->flags |= IN_CHANGE | IN_UPDATE;
		return ext2fs_update(ip);
	} else if (size < len) {
		return ext2fs_extend(ip, len, cred);
	} else {
		return ext2fs_shrink(ip, len, cred);
	}
}

