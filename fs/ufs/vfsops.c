
#include <fs/mount.h>
#include <fs/vnode.h>
#include <fs/vfs.h>
#include <fs/ufs/ufs.h>
#include <panic.h>

int
ufs_root(struct mount *mp, struct vnode **vpp)
{
	struct vnode *vp;
	int err;

	if ((err = VFS_VGET(mp, ROOTINO, &vp)) != 0)
		return err;
	vp->flags |= VROOT;
	*vpp = vp;
	return 0;
}

