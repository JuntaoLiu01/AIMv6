
#include <sys/types.h>
#include <fs/mount.h>
#include <fs/vnode.h>
#include <fs/ufs/ext2fs/ext2fs.h>
#include <fs/ufs/ext2fs/dinode.h>
#include <fs/ufs/ufs.h>
#include <fs/ufs/ufsmount.h>
#include <fs/ufs/inode.h>
#include <fs/bio.h>
#include <fs/specdev.h>
#include <panic.h>
#include <vmm.h>
#include <errno.h>

int
ext2fs_vinit(struct mount *mp, struct vops *specvops,
    struct vops *fifovops, struct vnode **vpp)
{
	struct specinfo *si;
	struct vnode *vp = *vpp;
	struct inode *ip = VTOI(vp);
	struct ext2fs_dinode *dp = ip->dinode;

	vp->type = EXT2_IFTOVT(dp->mode);

	switch (vp->type) {
	case VCHR:
	case VBLK:
		kpdebug("ext2fs vinit ino %llu %x %d, %d\n",
		    (ino_t)(ip->ino), dp->rdev, major(dp->rdev), minor(dp->rdev));
		/*
		 * This piece of code relates specinfo, inode and vnode
		 * structures together.
		 */
		si = findspec(dp->rdev);
		if (si != NULL) {
			/*
			 * Discard unneeded vnode but keep the inode.
			 * Note that the lock is carried over in the inode
			 * to the replacement vnode.
			 */
			/* XXX shall we make an assertion here? */
			assert(si->vnode->data == NULL);
			si->vnode->data = vp->data;	/* pass inode */
			vp->data = NULL;
			vp->ops = &spec_vops;
			assert(vp->typedata == NULL);
			vput(vp);	/* get rid of the old vnode */

			/* Update the old vnode associated to found specinfo
			 * with file system information */
			vp = si->vnode;
			ip->vnode = vp;
			insmntque(vp, mp);
			/*
			 * For other special files, both the file's vnode and
			 * the mount device vnode should be ref'd, but if we
			 * are initializing the inode of the mount device
			 * itself, we only ref it once (which is done outside)
			 */
			if (vp != VFSTOUFS(mp)->devvp)
				vref(vp);
		} else {
			si = newspec(dp->rdev);
			si->vnode = vp;
			vp->specinfo = si;
		}
		/* Switch the spec file ops to file system's specvops since we
		 * have an inode and a specinfo to deal with. */
		vp->ops = specvops;
		kpdebug("ext2fs vinit ino %llu done\n", (ino_t)(ip->ino));
		break;
	default:
		break;
	}

	*vpp = vp;
	return 0;
}

/*
 * Retrieve a locked vnode corresponding to inode @ino on mount @mp.
 */
int
ext2fs_vget(struct mount *mp, ino_t ino, struct vnode **vpp)
{
	struct vnode *vp;
	struct inode *ip;
	struct ufsmount *ump;
	struct m_ext2fs *fs;
	struct buf *bp = NULL;
	struct ext2fs_dinode *dp, *ndp;
	dev_t devno;
	int err;

	if (ino > (ufsino_t)(-1))
		panic("ext2fs_vget: alien ino_t %u\n", ino);

	ump = VFSTOUFS(mp);
	devno = ump->devno;

	kpdebug("ext2fs vget %llu on dev %d,%d\n", ino, major(devno), minor(devno));

retry:
	/* Find if inode @ino on device @dev is already loaded */
	if ((*vpp = ufs_ihashget(devno, ino)) != NULL) {
		kpdebug("ext2fs vget found cached %p\n", *vpp);
		return 0;
	}

	/* Not found... allocate a new vnode */
	kpdebug("ext2fs vget allocating new vnode\n");
	if ((err = getnewvnode(mp, &ext2fs_vops, &vp)) != 0) {
		*vpp = NULL;
		return err;
	}

	if ((ip = kmalloc(sizeof(*ip), 0)) == NULL) {
		vrele(vp);
		return -ENOMEM;
	}
	ip->vnode = vp;
	vp->data = ip;
	ip->ino = ino;
	ip->devno = devno;
	ip->ufsmount = ump;
	ip->dinode = NULL;
	fs = ip->superblock = ump->superblock;
	spinlock_init(&ip->lock);
	kpdebug("allocated vnode %p inode %p\n", vp, ip);
	/*
	 * Because vrele()'ing or vput()'ing an ext2 vnode will decrement
	 * ref count of UFS mount device vnode during reclaiming, we increment
	 * the ref count here.  (Is this a hack?)
	 */
	vref(ump->devvp);
	err = ufs_ihashins(ip);
	if (err) {
		kpdebug("ext2fs vget failed to insert inode %p\n", ip);
		vrele(vp);
		if (err == -EEXIST)
			goto retry;
		return err;
	}

	/* Read in the on-disk content of inode */
	kpdebug("ext2fs reading on-disk inode %llu\n", ino);
	vlock(ump->devvp);
	vlock(vp);
	err = bread(ump->devvp, fsbtodb(fs, ino_to_fsba(fs, ino)), fs->bsize,
	    &bp);
	if (err) {
		kpdebug("ext2fs vget failed to read on-disk inode\n");
		brelse(bp);
		vput(vp);
		vunlock(ump->devvp);
		*vpp = NULL;
		return err;
	}
	dp = (struct ext2fs_dinode *)
	    (bp->data + EXT2_DINODE_SIZE(fs) * ino_to_fsbo(fs, ino));
	ndp = kmalloc(sizeof(*ndp), 0);
	e2fs_iload(fs, dp, ndp);
	ip->dinode = ndp;
	brelse(bp);

	ip->effnlink = ndp->nlink;
	ip->uid = ndp->uid_low | (ndp->uid_high << 16);
	ip->gid = ndp->gid_low | (ndp->gid_high << 16);
	/* If the inode was "deleted", reset all fields necessary */
	if (ndp->dtime != 0) {
		ndp->mode = ndp->nblock = 0;
		ext2fs_setsize(ip, 0);
	}
	ip->nsect = ((uint64_t)ndp->nblock_hi << 32) | ndp->nblock;

	kpdebug("ext2fs vget initializing vnode\n");
	err = ext2fs_vinit(mp, &ext2fs_specvops, NULL, &vp);
	if (err) {
		kpdebug("ext2fs vget failed to initialize vnode\n");
		vput(vp);
		vunlock(ump->devvp);
		*vpp = NULL;
		return err;
	}

	/*
	 * If we are getting a device file corresponding to the device
	 * the file system is located on, we do not unlock it.
	 */
	if (vp != ump->devvp)
		vunlock(ump->devvp);

	kpdebug("ext2fs vget %llu vnode %p\n", ino, vp);
	kpdebug("\ttype %d\n", vp->type);
	kpdebug("\tmode %o\n", ndp->mode);

	*vpp = vp;
	return 0;
}

