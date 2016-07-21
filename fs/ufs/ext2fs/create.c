
#include <fs/vnode.h>
#include <fs/ufs/inode.h>
#include <fs/ufs/ext2fs/dinode.h>
#include <fs/ufs/ext2fs/ext2fs.h>
#include <panic.h>

int
ext2fs_direnter(struct inode *ip, struct vnode *dvp, char *name)
{
	panic("ext2fs direnter NYI\n");
	return 0;
}

int
ext2fs_makeinode(int imode, struct vnode *dvp, char *name, struct vnode **vpp)
{
	int err;
	struct vnode *tvp;
	struct inode *ip;

	assert((imode & EXT2_IFMT) != 0);

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
	if ((err = ext2fs_direnter(ip, dvp, name)) != 0)
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
ext2fs_create(struct vnode *dvp, char *name, int mode, struct vnode **vpp)
{
	return ext2fs_makeinode(EXT2_MAKEIMODE(VREG, mode), dvp, name, vpp);
}

