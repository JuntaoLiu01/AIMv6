
#include <fs/vnode.h>
#include <fs/mount.h>
#include <fs/bio.h>
#include <fs/vfs.h>
#include <fs/namei.h>
#include <fs/ufs/inode.h>
#include <fs/ufs/ext2fs/dinode.h>
#include <fs/ufs/ext2fs/ext2fs.h>
#include <fs/ufs/ext2fs/dir.h>
#include <panic.h>
#include <limits.h>
#include <libc/string.h>
#include <errno.h>

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
	void *cur;
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
		cur = bp->data;
		e2fs_load_direct(cur, &dir);
		for (e2fs_load_direct(cur, &dir);
		     (cur < bp->data + fs->bsize) && (dir.reclen != 0);
		     e2fs_load_direct(cur, &dir)) {
			/* Found? */
			if (dir.namelen == namelen &&
			    memcmp(dir.name, name, dir.namelen) == 0) {
				brelse(bp);
				if (dir.ino == VTOI(dvp)->ino)
					/*
					 * Since @dvp is already vget'd, we
					 * unlock it to allow re-VGET it so
					 * that we can increase ref count.
					 */
					vunlock(dvp);
				err = VFS_VGET(dvp->mount, dir.ino, &vp);
				*vpp = (err == 0) ? vp : NULL;
				return err;
			}
			cur += EXT2FS_DIRSIZ(dir.namelen);
		}
		brelse(bp);
		bp = NULL;
	}
	*vpp = NULL;
	return 0;
}

