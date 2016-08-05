
#include <fs/ufs/inode.h>
#include <fs/vnode.h>
#include <panic.h>

int
ext2fs_direnter(struct inode *ip, struct vnode *dvp, char *name)
{
	panic("ext2fs direnter NYI\n");
	return 0;
}

