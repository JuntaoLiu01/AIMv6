
#include <fs/vnode.h>
#include <fs/ufs/inode.h>
#include <fs/ufs/ext2fs/dinode.h>
#include <fs/ufs/ext2fs/ext2fs.h>
#include <fs/namei.h>
#include <panic.h>
#include <errno.h>
#include <limits.h>

int
ext2fs_makeinode(int imode, struct vnode *dvp, char *name, struct vnode **vpp,
    struct ucred *cred, struct proc *p)
{
	int err;
	struct vnode *tvp;
	struct inode *ip;

	assert((imode & EXT2_IFMT) != 0);

	kpdebug("ext2fs makeinode %o %s %u\n", imode, name, VTOI(dvp)->ino);

	err = ext2fs_inode_alloc(dvp, imode, &tvp);
	if (err)
		return err;

	ip = VTOI(tvp);
	/* TODO: set up uid and gid */
	ip->uid = ip->gid = 0;
	ip->flags |= IN_ACCESS | IN_CHANGE | IN_UPDATE;
	EXT2_DINODE(ip)->mode = imode;
	tvp->type = EXT2_IFTOVT(imode);
	EXT2_DINODE(ip)->nlink = 1;
	/* TODO: SGID? */

	/* To rollback ext2fs_inode_alloc we set # links to be 0, and
	 * call vput(tvp).  Since @tvp is just VGET'd in ext2fs_inode_alloc
	 * vput() will call VOP_INACTIVE(), which will delete the inode by
	 * updating dtime to be non-zero.  It will also call ext2fs_inode_free
	 * to erase the bit in inode bitmap. */
	if ((err = ext2fs_update(ip)) != 0)
		goto rollback_inode;
	if ((err = ext2fs_direnter(ip, dvp, name, cred)) != 0)
		goto rollback_inode;
	*vpp = tvp;
	return 0;

rollback_inode:
	EXT2_DINODE(ip)->nlink = 0;
	EXT2_DINODE(ip)->flags |= IN_CHANGE;	/* trigger update */
	tvp->type = VNON;
	vput(tvp);
	return err;
}

int
ext2fs_create(struct vnode *dvp, char *name, struct vattr *va,
    struct vnode **vpp, struct ucred *cred, struct proc *p)
{
	return ext2fs_makeinode(EXT2_MAKEIMODE(va->type, va->mode), dvp, name,
	    vpp, cred, p);
}

int
ext2fs_link(struct vnode *dvp, char *name, struct vnode *srcvp,
    struct ucred *cred, struct proc *p)
{
	struct inode *ip = VTOI(srcvp);
	int err;

	if (srcvp->type == VDIR)
		/*
		 * Allowing hard-linking directories is a BAD IDEA(TM).
		 */
		return -EISDIR;
	if (EXT2_DINODE(ip)->nlink >= LINK_MAX)
		return -EMLINK;

	EXT2_DINODE(ip)->nlink++;
	ip->flags |= IN_CHANGE;
	if ((err = ext2fs_update(ip)) == 0)
		ext2fs_direnter(ip, dvp, name, cred);
	if (err) {
		/* rollback :( */
		EXT2_DINODE(ip)->nlink--;
		ip->flags |= IN_CHANGE;
		ext2fs_update(ip);
	}

	return err;
}

