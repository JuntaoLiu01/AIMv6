
#include <fs/vnode.h>
#include <fs/bio.h>
#include <fs/vfs.h>
#include <fs/ufs/inode.h>
#include <fs/ufs/ext2fs/dinode.h>
#include <fs/ufs/ext2fs/ext2fs.h>
#include <fs/ufs/ext2fs/dir.h>
#include <panic.h>
#include <libc/string.h>
#include <errno.h>

int
ext2fs_lookup(struct vnode *dvp, char *name, struct vnode **vpp)
{
	unsigned int i;
	struct vnode *vp;
	struct buf *bp;
	struct inode *ip = VTOI(dvp);
	struct m_ext2fs *fs = ip->superblock;
	struct ext2fs_dirhdr dh;
	void *cur;
	int err;
	char *entry_name;

	assert(dvp->type == VDIR);
	assert(dvp->flags & VXLOCK);
	kpdebug("ext2fs lookup %p(%d) %s\n", dvp, ip->ino, name);

	/*
	 * TODO:
	 * Now I only implement the LOOKUP operation.
	 * Will implement CREATE, RENAME and DELETE operations later.
	 */
	/* Do a linear search in the directory content: we read the data
	 * blocks one by one, look up the name, and return the vnode. */
	for (i = 0; i < ip->ndatablock; ++i) {
		err = bread(dvp, i, fs->bsize, &bp);
		if (err) {
			return err;
		}
		/* Iterate over the list of directory entries */
		cur = bp->data;
		e2fs_load_dirhdr(cur, &dh);
		for (e2fs_load_dirhdr(cur, &dh);
		     (cur < bp->data + fs->bsize) && (dh.reclen != 0);
		     e2fs_load_dirhdr(cur, &dh)) {
			entry_name = cur + sizeof(dh);
			/* Found? */
			if (memcmp(entry_name, name, dh.namelen) == 0) {
				brelse(bp);
				if (dh.ino == VTOI(dvp)->ino) {
					/*
					 * Since @dvp is already vget'd, we
					 * don't want to vget() it again.
					 */
					vp = dvp;
					err = 0;
				} else {
					err = VFS_VGET(dvp->mount, dh.ino, &vp);
				}
				*vpp = (err == 0) ? vp : NULL;
				return err;
			}
			cur += EXT2FS_DIRSIZ(dh.namelen);
		}
		brelse(bp);
	}
	*vpp = NULL;
	return -ENOENT;
}

