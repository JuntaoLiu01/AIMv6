
#include <asm-generic/funcs.h>
#include <aim/initcalls.h>
#include <fs/vfs.h>
#include <fs/mount.h>
#include <fs/vnode.h>
#include <fs/specdev.h>
#include <fs/ufs/ufs.h>
#include <fs/ufs/inode.h>
#include <fs/ufs/ext2fs/ext2fs.h>
#include <fs/ufs/ext2fs/dinode.h>
#include <panic.h>

int
ext2fs_open(struct vnode *vp, int mode, struct ucred *cred, struct proc *p)
{
	return 0;
}

int
ext2fs_close(struct vnode *vp, int mode, struct ucred *cred, struct proc *p)
{
	struct inode *ip = VTOI(vp);

	if (!(ip->flags & (IN_ACCESS | IN_CHANGE | IN_UPDATE)))
		return 0;
	EXT2FS_ITIMES(ip);
	return ext2fs_update(ip);
}

struct vfsops ext2fs_vfsops = {
	.root = ufs_root,
	.vget = ext2fs_vget,
	.sync = ext2fs_sync,
};

struct vops ext2fs_vops = {
	.open = ext2fs_open,
	.close = ext2fs_close,
	.read = ext2fs_read,
	.write = ext2fs_write,
	.inactive = ext2fs_inactive,
	.reclaim = ext2fs_reclaim,
	.strategy = ufs_strategy,
	.lookup = ext2fs_lookup,
	.create = ext2fs_create,
	.bmap = ext2fs_bmap,
	.link = ext2fs_link,
	.remove = ext2fs_remove,
	.access = NOP,	/* TODO */
	.mkdir = ext2fs_mkdir,
	.readdir = ext2fs_readdir,
	.rmdir = ext2fs_rmdir,
	.getattr = ext2fs_getattr,
	.setattr = ext2fs_setattr,
	.fsync = ext2fs_fsync,
};

/*
 * Special file operations on an ext2 file system.
 * This is a combination of the ordinal spec ops and ext2 ops.
 */
struct vops ext2fs_specvops = {
	.open = spec_open,
	.close = spec_close,
	.read = spec_read,
	.write = spec_write,
	.inactive = ext2fs_inactive,
	.reclaim = ext2fs_reclaim,
	.strategy = spec_strategy,
	.lookup = NOTSUP,
	.create = NOTSUP,
	.bmap = NOTSUP,
	.link = NOTSUP,
	.remove = NOTSUP,
	.access = NOP,	/* TODO */
	.mkdir = NOTSUP,
	.readdir = NOTSUP,
	.rmdir = NOTSUP,
	.getattr = ext2fs_getattr,
	.setattr = ext2fs_setattr,
	.fsync = ext2fs_fsync,
};

int
ext2fs_register(void)
{
	registerfs("ext2fs", &ext2fs_vfsops);
	register_mountroot(ext2fs_mountroot);
	return 0;
}
INITCALL_FS(ext2fs_register);

