
#include <fs/vnode.h>
#include <fs/ufs/inode.h>
#include <fs/ufs/ext2fs/dinode.h>
#include <fs/ufs/ext2fs/ext2fs.h>
#include <ucred.h>
#include <proc.h>
#include <errno.h>

int
ext2fs_getattr(struct vnode *vp, struct vattr *va, struct ucred *cred,
    struct proc *p)
{
	struct inode *ip = VTOI(vp);

	EXT2FS_ITIMES(ip);

	va->type = vp->type;
	va->mode = EXT2_DINODE(ip)->mode;
	va->size = ext2fs_getsize(ip);

	return 0;
}

int
ext2fs_setattr(struct vnode *vp, struct vattr *va, struct ucred *cred,
    struct proc *p)
{
	int err;

	/* Check for unsettable attributes */
	if (va->type != VNON)
		return -EINVAL;
	if (va->mode != VNOVAL)
		/* TODO: chmod */
		return -ENOTSUP;
	if (va->size != VNOVAL) {
		switch (vp->type) {
		case VDIR:
			return -EISDIR;
		default:
			break;
		}
		err = ext2fs_truncate(VTOI(vp), va->size, cred);
		if (err)
			return err;
	}
	return 0;
}

