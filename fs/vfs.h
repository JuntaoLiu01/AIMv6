
#ifndef _FS_VFS_H
#define _FS_VFS_H

#include <sys/types.h>
#include <fs/mount.h>

struct vnode;	/* fs/vnode.h */
struct ucred;	/* ucred.h */
struct proc;	/* proc.h */

struct vfsconf {
	const char *name;
	struct vfsops *ops;
};

struct vfsops {
	/*
	 * root() - get root vnode.
	 * On UFS-like file systems, get the vnode for '/' directory.
	 * Returns a locked vnode.
	 */
	int (*root)(struct mount *mp, struct vnode **vpp);
	/*
	 * vget() - get vnode according to inode # (or file ID #)
	 * Not every file system has a file ID (e.g. FAT).
	 * Returns a locked vnode.
	 */
	int (*vget)(struct mount *mp, ino_t ino, struct vnode **vpp);
	/*
	 * sync() - Flush out all dirty buffers and modifications onto storage
	 */
	int (*sync)(struct mount *mp, struct ucred *cred, struct proc *p);
};

#define VFS_ROOT(mp, vpp)	((mp)->ops->root((mp), (vpp)))
#define VFS_VGET(mp, ino, vpp)	((mp)->ops->vget((mp), (ino), (vpp)))
#define VFS_SYNC(mp, cred, p)	((mp)->ops->sync((mp), (cred), (p)))

#define MAX_VFSCONFS	5

void registerfs(const char *name, struct vfsops *ops);
struct vfsconf *findvfsconf(const char *name);

void fsinit(void);

#endif
