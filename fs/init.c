
#include <panic.h>
#include <fs/mount.h>
#include <fs/vnode.h>
#include <fs/vfs.h>

void
fsinit(void)
{
	struct mount *rootmp;

	kprintf("KERN: Starting FS...\n");
	mountroot();
	/* get root vnode */
	rootmp = list_first_entry(&mountlist, struct mount, node);
	kpdebug("rootmp at %p\n", rootmp);
	if (VFS_ROOT(rootmp, &rootvnode) != 0)
		panic("root vnode not found\n");
	vunlock(rootvnode);

	kprintf("==============fs test succeeded===============\n");
}

