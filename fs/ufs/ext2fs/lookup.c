
#include <fs/vnode.h>
#include <fs/mount.h>
#include <fs/bio.h>
#include <fs/vfs.h>
#include <fs/uio.h>
#include <fs/namei.h>
#include <fs/ufs/inode.h>
#include <fs/ufs/ext2fs/dinode.h>
#include <fs/ufs/ext2fs/ext2fs.h>
#include <fs/ufs/ext2fs/dir.h>
#include <panic.h>
#include <limits.h>
#include <libc/string.h>
#include <errno.h>
#include <ucred.h>
#include <dirent.h>

/*
 * TODO:
 * The directory entry search algorithm is the same among lookup(),
 * direnter() and dirremove().  Maybe I should merge them, but that would
 * probably make the code more complex.
 */

int
ext2fs_lookup(struct vnode *dvp, char *name, struct vnode **vpp,
    struct ucred *cred, struct proc *p)
{
	unsigned int i;
	struct vnode *vp;
	struct buf *bp = NULL;
	struct inode *ip = VTOI(dvp);
	struct m_ext2fs *fs = ip->superblock;
	struct ext2fs_direct dir;
	off_t offset;
	int err;
	size_t namelen = strnlen(name, PATH_MAX);

	assert(dvp->type == VDIR);
	assert(dvp->flags & VXLOCK);
	kpdebug("ext2fs lookup %p(%d) %s\n", dvp, ip->ino, name);

	/* Do a linear search in the directory content: we read the data
	 * blocks one by one, look up the name, and return the vnode. */
	for (i = 0; i < ext2fs_ndatablk(ip); ++i) {
		err = bread(dvp, i, fs->bsize, &bp);
		if (err) {
			brelse(bp);
			return err;
		}
		/* Iterate over the list of directory entries */
		for (offset = 0; offset < fs->bsize; ) {
			e2fs_load_direct(bp->data + offset, &dir);
			if (dir.reclen == 0) {
				kprintf("ext2fs lookup: reclen 0 at %d:%ld\n",
				    i, offset);
				break;
			}
			if (dir.ino != 0 && dir.namelen == namelen &&
			    memcmp(dir.name, name, dir.namelen) == 0) {
				brelse(bp);
				if (dir.ino == VTOI(dvp)->ino) {
					vref(dvp);
					*vpp = dvp;
				} else {
					err = VFS_VGET(dvp->mount, dir.ino, &vp);
					*vpp = (err == 0) ? vp : NULL;
				}
				kpdebug("ext2fs lookup %p(%d) %s done\n",
				    dvp, ip->ino, name);
				return err;
			}
			offset += dir.reclen;
		}
		brelse(bp);
	}
	*vpp = NULL;
	return 0;
}

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
	size_t newnamelen = strlen(name), freespace, occupied;
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
			if (dir.reclen == 0) {
				kprintf("ext2fs direnter: reclen 0 at %d:%ld\n",
				    i, offset);
				break;
			}
			occupied = dir.ino == 0 ? 0 : EXT2FS_DIRSIZ(dir.namelen);
			freespace = dir.reclen - occupied;
			if (freespace > EXT2FS_DIRSIZ(newnamelen)) {
				if (dir.ino != 0) {
					dir.reclen = occupied;
					e2fs_save_direct(&dir, bp->data + offset);
					offset += dir.reclen;
				} /* else write in place */
				goto write_entry;
			}
			offset += dir.reclen;
		}
		brelse(bp);
	}

	/* i == ext2fs_ndatablk(vp) */
	/* extend the directory data by one file system block */
	if (!IS_ALIGNED(osize, fs->bsize))
		kprintf("ext2fs direnter: unaligned dir size: %ld(%ld)\n",
		    osize, dp->ino);
	err = ext2fs_truncate(dp, ALIGN_ABOVE(osize + fs->bsize, fs->bsize),
	    cred);
	if (err)
		return err;
	bp = bget(dvp, i, fs->bsize);
	if (bp == NULL) {
		err = -ENOMEM;
		goto rollback_truncate;
	}
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

	e2fs_save_direct(&newdir, bp->data + offset);
	err = bwrite(bp);
	brelse(bp);

	if (err)
		goto rollback_truncate;

	return 0;

rollback_truncate:
	ext2fs_truncate(dp, osize, cred);
	return err;
}

int
ext2fs_dirremove(struct vnode *dvp, char *name, struct ucred *cred)
{
	struct inode *ip = VTOI(dvp);
	struct m_ext2fs *fs = ip->superblock;
	int err;
	unsigned int i;
	struct buf *bp = NULL;
	off_t offset, prevoff;
	struct ext2fs_direct dir, prevdir;

	assert(dvp->type == VDIR);
	assert(dvp->flags & VXLOCK);

	for (i = 0; i < ext2fs_ndatablk(ip); ++i) {
		prevoff = -1;	/* anything invalid */
		err = bread(dvp, i, fs->bsize, &bp);
		if (err) {
			brelse(bp);
			return err;
		}
		for (offset = 0; offset < fs->bsize; ) {
			e2fs_load_direct(bp->data + offset, &dir);
			if (dir.reclen == 0) {
				kprintf("ext2fs dirremove: reclen 0 at %d:%ld\n",
				    i, offset);
				break;
			}
			if (dir.ino != 0 && dir.namelen == strlen(name) &&
			    memcmp(dir.name, name, dir.namelen) == 0)
				goto remove;
			prevoff = offset;
			offset += dir.reclen;
		}
		brelse(bp);
	}

	/* didn't find the entry name */
	return -ENOENT;

remove:
	if (offset == 0) {
		/*
		 * We're removing the first entry in a block.  Set the
		 * inode to be zero.  But first, let's check if something
		 * weird happens (which should not happen by all means).
		 */
		assert(i != 0);
		dir.ino = 0;
		e2fs_save_direct(&dir, bp->data + offset);
	} else {
		/*
		 * Otherwise, we reclaim the record space by adding it to the
		 * previous entry.
		 */
		assert(prevoff >= 0);
		e2fs_load_direct(bp->data + prevoff, &prevdir);
		prevdir.reclen += dir.reclen;
		e2fs_save_direct(&prevdir, bp->data + prevoff);
	}

	err = bwrite(bp);
	brelse(bp);
	ip->flags |= IN_CHANGE | IN_UPDATE;
	return err;
}

/*
 * Assumes that uio->offset is always at the start of an entry.
 */
int
ext2fs_readdir(struct vnode *dvp, struct uio *uio, struct ucred *cred,
    bool *eofflag)
{
	struct ext2fs_direct dir, dir_disk;
	struct dirent dirent;
	size_t offset = uio->offset, done;
	bool read = false;
	int err;
	*eofflag = false;

	for (;;) {
		/* Read one entry */
		err = vn_read(dvp, offset, sizeof(dir_disk), &dir_disk, 0,
		    UIO_KERNEL, uio->proc, NULL, cred, &done);
		if (err)
			return err;
		if (done == 0) {
			/* EOF */
			*eofflag = true;
			break;
		}
		e2fs_load_direct(&dir_disk, &dir);
		/* Convert ext2fs entry @dir to standard format @dirent */
		memset(&dirent, 0, sizeof(dirent));
		dirent.d_ino = dir.ino;
		dirent.d_off = offset + dir.reclen;
		dirent.d_reclen = DIRENT_RECSIZE(dir.namelen);
		dirent.d_type = E2DT_TO_E2IF(dir.type);
		memcpy(dirent.d_name, dir.name, dir.namelen);
		/* Check if @uio is ready to receive @dirent */
		if (uio->resid < dirent.d_reclen)
			break;
		err = uiomove(&dirent, dirent.d_reclen, uio);
		if (err)
			return err;
		read = true;
		offset += dir.reclen;
	}

	if (!read) {
		assert(done != 0);
		return -EINVAL;
	} else {
		return 0;
	}
}

bool
ext2fs_dirempty(struct inode *ip, ufsino_t parentino, struct ucred *cred,
    struct proc *p)
{
	struct ext2fs_direct dir, dir_disk;
	int err;
	off_t off;
	size_t done, namelen;

	for (off = 0; off < ext2fs_getsize(ip); off += dir.reclen) {
		err = vn_read(ITOV(ip), off, sizeof(dir_disk), &dir_disk, 0,
		    UIO_KERNEL, p, NULL, cred, &done);
		if (err || done == 0)
			return false;
		e2fs_load_direct(&dir_disk, &dir);
		if (dir.reclen == 0)
			/* the directory is likely corrupted */
			return false;
		if (dir.ino == 0)
			/* free entry */
			continue;
		namelen = dir.namelen;
		/* accept only "." and ".." */
		if (namelen > 2)
			return false;
		if (dir.name[0] != '.')
			return false;
		if (namelen == 1)
			continue;
		if (dir.name[1] == '.' && dir.ino == parentino)
			continue;
		return false;
	}
	return true;
}

