
#ifndef _FS_UFS_UFS_H
#define _FS_UFS_UFS_H

#include <sys/types.h>

struct mount;	/* fs/mount.h */
struct vnode;	/* fs/vnode.h */
struct inode;	/* fs/ufs/inode.h */
struct buf;	/* include/buf.h */

int ufs_root(struct mount *mp, struct vnode **vpp);

int ufs_strategy(struct buf *bp);

struct vnode *ufs_ihashget(dev_t devno, ino_t ino);
int ufs_ihashins(struct inode *ip);
int ufs_ihashrem(struct inode *ip);

typedef uint32_t ufsino_t;

#define ROOTINO	2	/* root inode # - always 2 */
#define FIRSTINO 11	/* < 11 are reserved */

#endif
