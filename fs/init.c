
#include <panic.h>
#include <fs/mount.h>
#include <fs/vnode.h>
#include <fs/vfs.h>

#include <fs/namei.h>
#include <ucred.h>
#include <percpu.h>
#include <proc.h>
#include <fcntl.h>
#include <fs/uio.h>

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

	struct nameidata nd;
	size_t done;
	char buf[50];
	struct vattr va;
	nd.cred = NOCRED;
	nd.proc = current_proc;
	nd.path = "/etc/rc.d/";
	nd.intent = NAMEI_CREATE;
	nd.flags = NAMEI_FOLLOW | NAMEI_PARENT | NAMEI_STRIP;
	assert(namei(&nd) == 0);
	assert(nd.vp == NULL);
	assert(nd.parentvp != NULL);
	assert(memcmp(nd.seg, "rc.d", 4) == 0);
	va.type = VDIR;	/* does not care */
	va.mode = 0755;	/* rwxr-xr-x */
	assert(VOP_MKDIR(nd.parentvp, nd.seg, &va, &nd.vp, nd.cred, nd.proc) == 0);
	vput(nd.parentvp);
	vput(nd.vp);
	namei_cleanup(&nd);

	kprintf("==============fs test succeeded===============\n");
}

