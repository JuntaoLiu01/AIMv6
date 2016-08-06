
#include <fs/vnode.h>
#include <fs/ufs/inode.h>
#include <fs/ufs/ext2fs/dinode.h>
#include <fs/ufs/ext2fs/ext2fs.h>
#include <fs/namei.h>
#include <panic.h>

int
ext2fs_makeinode(int imode, struct nameidata *nd)
{
	int err;
	struct vnode *tvp;
	struct inode *ip;
	struct vnode *dvp = nd->parentvp;
	char *name = nd->seg;

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
	if ((err = ext2fs_direnter(ip, dvp, name, nd->cred)) != 0)
		goto rollback_inode;
	nd->vp = tvp;
	return 0;

rollback_inode:
	EXT2_DINODE(ip)->nlink = 0;
	EXT2_DINODE(ip)->flags |= IN_CHANGE;	/* trigger update */
	tvp->type = VNON;
	vput(tvp);
	return err;
}

int
ext2fs_create(struct nameidata *nd, struct vattr *va)
{
	return ext2fs_makeinode(EXT2_MAKEIMODE(va->type, va->mode), nd);
}

