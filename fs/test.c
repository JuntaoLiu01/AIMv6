
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <fs/vnode.h>
#include <fs/specdev.h>
#include <panic.h>
#include <mach-conf.h>

extern struct vnode *rootvp;

void
fs_test(void)
{
	ext2fs_mountroot();
	assert(rootvp != NULL);
	assert(rootvp->type == VBLK);
	assert(rootvp->specinfo != NULL);
	assert(major(rootvp->specinfo->devno) == MSIM_DISK_MAJOR);
	assert(minor(rootvp->specinfo->devno) == ROOT_PARTITION_ID);
	assert(rootvp->specinfo->vnode == rootvp);
}
