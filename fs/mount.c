
#include <fs/vnode.h>
#include <fs/mount.h>
#include <fs/vfs.h>
#include <list.h>
#include <vmm.h>
#include <libc/string.h>
#include <aim/sync.h>
#include <aim/initcalls.h>
#include <errno.h>
#include <sched.h>
#include <panic.h>

struct allocator_cache mountpool = {
	.size = sizeof(struct mount),
	.align = 1,
	.flags = GFP_ZERO,
	.create_obj = NULL,
	.destroy_obj = NULL
};

struct list_head mountlist = EMPTY_LIST(mountlist);

int
mountinit(void)
{
	spinlock_init(&mountpool.lock);
	assert(cache_create(&mountpool) == 0);
	return 0;
}
INITCALL_FS(mountinit);

void
insmntque(struct vnode *vp, struct mount *mp)
{
	if (vp->mount != NULL)
		list_del_init(&(vp->mount_node));
	vp->mount = mp;
	if (mp != NULL)
		list_add_tail(&(vp->mount_node), &(mp->vnode_head));
}

void
addmount(struct mount *mp)
{
	list_add_tail(&(mp->node), &mountlist);
}

int
vfs_rootmountalloc(const char *fsname, struct mount **mpp)
{
	struct vfsconf *vfsc;
	struct mount *mp;
       
	vfsc = findvfsconf(fsname);
	if (vfsc == NULL)
		return -ENODEV;

	mp = cache_alloc(&mountpool);
	memset(mp, 0, sizeof(*mp));

	vfs_busy(mp, true, false);
	list_init(&(mp->vnode_head));
	mp->ops = vfsc->ops;

	*mpp = mp;
	return 0;
}

void
vfs_mountfree(struct mount **mp)
{
	vfs_unbusy(*mp);
	cache_free(&mountpool, *mp);
	*mp = NULL;
}

int
vfs_busy(struct mount *mp, bool write, bool sleep)
{
	/* TODO FUTURE: make this a reader-writer lock */
	spin_lock(&(mp->lock));
	while (mp->flags & MOUNT_BUSY) {
		if (sleep) {
			sleep_with_lock(mp, &(mp->lock));
		} else {
			spin_unlock(&(mp->lock));
			return -EAGAIN;
		}
	}
	mp->flags |= MOUNT_BUSY;
	spin_unlock(&(mp->lock));
	return 0;
}

void
vfs_unbusy(struct mount *mp)
{
	/* TODO FUTURE: make this a reader-writer lock */
	spin_lock(&(mp->lock));
	mp->flags &= ~MOUNT_BUSY;
	wakeup(mp);
	spin_unlock(&(mp->lock));
}

int (*mountroot_func[FS_MAX])(void) = {0};
int mountroot_count = 0;

void
register_mountroot(int (*mountroot)(void))
{
	mountroot_func[mountroot_count++] = mountroot;
}

void
mountroot(void)
{
	for (int i = 0; i < mountroot_count; ++i) {
		if (mountroot_func[i] == NULL)
			continue;
		if (mountroot_func[i]() == 0)
			return;
	}
	panic("failed to mount any root fs\n");
}

