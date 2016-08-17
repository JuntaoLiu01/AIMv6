
#ifndef _FS_UFS_INODE_H
#define _FS_UFS_INODE_H

#include <sys/types.h>
#include <aim/sync.h>
#include <fs/ufs/ufs.h>
#include <list.h>

struct inode {
	/* in-memory inode */
	struct list_head node;		/* UFS hash table entry node */
	struct vnode	*vnode;
	struct ufsmount	*ufsmount;
	uint32_t	flags;
#define IN_ACCESS	0x01		/* refresh access time */
#define IN_CHANGE	0x02		/* refresh change time */
#define IN_UPDATE	0x04		/* refresh update time */
#define IN_MODIFIED	0x08		/* inode modified */
#define IN_RENAMED	0x10		/* inode renamed */
	ufsino_t	ino;
	dev_t		devno;
	int		effnlink;	/* inode nlink when I/O completes */
	void		*superblock;
	lock_t		lock;
	uint32_t	uid;
	uint32_t	gid;
	uint64_t	nsect;		/* number of disk sectors */
	void		*dinode;	/* on-disk inode (dinode) */
};

#define VTOI(vp)	((struct inode *)((vp)->data))
#define ITOV(ip)	((ip)->vnode)

#endif
