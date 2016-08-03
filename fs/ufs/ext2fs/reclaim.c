
#include <fs/vnode.h>
#include <fs/ufs/inode.h>
#include <fs/ufs/ufs.h>
#include <fs/ufs/ufsmount.h>
#include <fs/ufs/ext2fs/dinode.h>
#include <atomic.h>
#include <proc.h>
#include <panic.h>
#include <vmm.h>
#include <ucred.h>

int
ext2fs_inactive(struct vnode *vp, struct proc *p)
{
	struct inode *ip = VTOI(vp);
	struct ext2fs_dinode *dp = EXT2_DINODE(ip);
	int err = 0;
	kpdebug("ext2 deactivating vnode %p\n", vp);

	if (dp == NULL || dp->mode == 0 || dp->dtime)
		goto out;

	if (dp->nlink == 0) {
		if (ext2fs_getsize(ip) > 0)
			err = ext2fs_truncate(ip, 0, NOCRED); /* real NOCRED */
		/* TODO: set this to *real* time */
		EXT2_DINODE(ip)->dtime = 1;
		ip->flags |= IN_CHANGE | IN_UPDATE;
		ext2fs_inode_free(ip, ip->ino, EXT2_DINODE(ip)->mode);
	}
	if (ip->flags & (IN_ACCESS | IN_CHANGE | IN_MODIFIED | IN_UPDATE))
		ext2fs_update(ip);
out:
	vunlock(vp);
	return err;
}

int
ext2fs_reclaim(struct vnode *vp)
{
	struct inode *ip;

	kpdebug("ext2 reclaiming vnode %p\n", vp);

	assert(vp->refs == 0);
	ip = VTOI(vp);
	if (ip == NULL)
		return 0;
	ufs_ihashrem(ip);

	if (ip->ufsmount != NULL && ip->ufsmount->devvp != NULL) {
		assert(ip->ufsmount->devvp->refs > 1);
		atomic_dec(&ip->ufsmount->devvp->refs);
	}

	if (ip->dinode != NULL) {
		kfree(ip->dinode);
	}

	kfree(ip);
	vp->data = NULL;

	return 0;
}

