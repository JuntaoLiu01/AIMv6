
#ifndef _FS_MOUNT_H
#define _FS_MOUNT_H

#include <list.h>
#include <sys/types.h>
#include <aim/sync.h>

struct vnode;	/* fs/vnode.h */
struct vfsops;	/* fs/vfs.h */

struct mount {
	struct list_head vnode_head;
	struct vnode	*covered;	/* vnode we mounted on */
	struct vfsops	*ops;
	/* Mount lock */
	lock_t		lock;
	uint32_t	flags;
#define MOUNT_BUSY	0x1
	void		*data;	/* fs-specific mount data */
	struct list_head node;	/* mountlist node */
};

void insmntque(struct vnode *vp, struct mount *mp);
void addmount(struct mount *mp);
int vfs_rootmountalloc(const char *fsname, struct mount **mpp);
void vfs_mountfree(struct mount **mp);
int vfs_busy(struct mount *mp, bool write, bool sleep);
void vfs_unbusy(struct mount *mp);

void register_mountroot(int (*)(void));
void mountroot(void);

/* FIXME */
#define FS_MAX	5	/* maximum supported # of FS's */

extern struct list_head mountlist;

#endif
