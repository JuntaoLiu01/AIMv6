
#include <fs/ufs/inode.h>
#include <fs/ufs/ext2fs/ext2fs.h>
#include <fs/ufs/ext2fs/dir.h>
#include <fs/ufs/ext2fs/dinode.h>
#include <fs/vnode.h>
#include <fs/bio.h>
#include <panic.h>
#include <ucred.h>
#include <lib/libc/string.h>

/*
 * Assumes that @ip->vnode is VGET'd, and @dvp is locked.
 * Does NOT check whether the entry name exists or not.
 */
int
ext2fs_direnter(struct inode *ip, struct vnode *dvp, char *name,
    struct ucred *cred)
{
	struct m_ext2fs *fs = ip->superblock;
	struct buf *bp;
	struct inode *dp = VTOI(dvp);
	int i, err;
	off_t offset;
	struct ext2fs_direct dir, newdir;
	size_t newnamelen = strlen(name), freespace;
	size_t osize = ext2fs_getsize(dp);

	assert(ITOV(ip)->flags & VXLOCK);
	assert(dvp->flags & VXLOCK);

	/*
	 * We will use the simplest algorithm.  For each block, we search
	 * for a suitable space with a first-fit strategy.  If no such space
	 * could be found, we allocate and write into a fresh block.
	 */
	for (i = 0; i < ext2fs_ndatablk(dp); ++i) {
		err = bread(dvp, i, fs->bsize, &bp);
		if (err) {
			brelse(bp);
			return err;
		}
		for (offset = 0; offset < fs->bsize; ) {
			e2fs_load_direct(bp->data + offset, &dir);
			freespace = dir.reclen - EXT2FS_DIRSIZ(dir.namelen);
			if (freespace > EXT2FS_DIRSIZ(newnamelen)) {
				dir.reclen = EXT2FS_DIRSIZ(dir.namelen);
				e2fs_save_direct(&dir, bp->data + offset);
				offset += dir.reclen;
				goto write_entry;
			}
			offset += dir.reclen;
		}
		brelse(bp);
	}
	/* i == ext2fs_ndatablk(vp) */
	err = ext2fs_buf_alloc(dp, i, cred, &bp);
	if (err)
		return err;
	freespace = fs->bsize;
	offset = 0;

write_entry:
	/*
	 * @i and @offset now point to the block and the position we will be
	 * writing our new directory entry, and @bp is the buf for block
	 * @i.
	 */
	newdir.ino = ip->ino;
	newdir.reclen = freespace;
	newdir.namelen = newnamelen;
	newdir.type = E2IF_TO_E2DT(EXT2_DINODE(ip)->mode);
	strlcpy(newdir.name, name, EXT2FS_MAXNAMLEN);

	e2fs_save_direct(&dir, bp->data + offset);
	err = bwrite(bp);
	brelse(bp);

	if (err) {
		/* TODO: replace NOCRED */
		ext2fs_lblkfree(dp, i, cred);
		return err;
	}
	ext2fs_setsize(dp, osize + fs->bsize);
	dp->flags |= IN_CHANGE | IN_UPDATE;

	return ext2fs_update(dp);
}

