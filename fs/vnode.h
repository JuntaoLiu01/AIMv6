
#ifndef _FS_VNODE_H
#define _FS_VNODE_H

#include <sys/types.h>
#include <aim/sync.h>
#include <list.h>
#include <buf.h>

struct specinfo;	/* fs/specdev.h */
struct mount;		/* fs/mount.h */
struct ucred;		/* include/ucred.h */
struct proc;		/* include/proc.h */
struct mm;		/* include/mm.h */
struct uio;		/* fs/uio.h */
enum uio_seg;		/* fs/uio.h */

enum vtype {
	VNON,	/* no-type */
	VREG,	/* regular file */
	VDIR,	/* directory */
	VBLK,	/* block device */
	VCHR,	/* character device */
	VLNK,	/* symlink */
	VBAD	/* ??? */
};

struct vnode {
	enum vtype	type;
	struct vops	*ops;
	atomic_t	refs;
	lock_t		lock;
	uint32_t	flags;
#define VXLOCK		0x1	/* locked */
#define VROOT		0x2	/* root of a file system */
#define VISTTY		0x4	/* is a terminal */
	/* members for mount */
	struct mount	*mount;
	struct list_head mount_node;

	/*
	 * Buffer queue for the vnode.
	 * To simplify buf management, we ensure that the lock is held
	 * (VXLOCK is set in flags) whenever we are manipulating the
	 * buf queue.
	 */
	struct list_head buf_head;
	int		noutputs;	/* # of writing buf's */

	union {
		/* vnode type-specific data */
		void	*typedata;
		/* if a directory, point to mount structure mounted here */
		struct mount *mountedhere;
		/* if a device, point to device info descriptor */
		struct specinfo *specinfo;
	};
	/* FS-specific data.  For UFS-like filesystems this points to inode. */
	void		*data;
};

struct vops {
	/*
	 * open:
	 */
	int (*open)(struct vnode *, int, struct ucred *, struct proc *);
	/*
	 * close:
	 */
	int (*close)(struct vnode *, int, struct ucred *, struct proc *);
	/*
	 * read:
	 * Read from a vnode as specified by the uio structure.
	 * Assumes that the vnode is locked.
	 */
	int (*read)(struct vnode *, struct uio *, int, struct ucred *);
	/*
	 * write:
	 * Write to a vnode as specified by the uio structure.
	 * Assumes that the vnode is locked.
	 */
	int (*write)(struct vnode *, struct uio *, int, struct ucred *);
	/*
	 * inactive:
	 * Truncate, update, unlock.  Usually called when a kernel is no
	 * longer using the vnode.
	 * xv6 equivalent is iput().
	 * NOTE: this primitive is different from OpenBSD.
	 */
	int (*inactive)(struct vnode *, struct proc *);
	/*
	 * reclaim:
	 * Free up the FS-specific resource.
	 */
	int (*reclaim)(struct vnode *);
	/*
	 * strategy:
	 * Initiate a block I/O.
	 */
	int (*strategy)(struct buf *);
	/*
	 * lookup:
	 * Finds a directory entry by name.
	 * The directory should be locked.
	 * The returned vnode is retrieved via vget() hence locked.
	 * Does NOT check credentials.
	 */
	int (*lookup)(struct vnode *, char *, struct vnode **);
	/*
	 * create:
	 * Create a regular file according to given file name segment.
	 * The directory should be locked.
	 * The returned vnode is retrieved via vget() hence locked.
	 * Does NOT check whether the file name already exists.
	 * Does NOT check credentials.
	 */
	int (*create)(struct vnode *, char *, int, struct vnode **);
	/*
	 * bmap:
	 * Translate a logical block number of a file to a disk sector
	 * number on the partition the file system is mounted on.
	 * Error code:
	 * -E2BIG: the logical block # is not less than # of data blocks
	 * -ENXIO: the logical block # is not less than the maximum # the
	 *         direct + indirect blocks can handle
	 */
	int (*bmap)(struct vnode *, off_t, struct vnode **, soff_t *, int *);
};

#define VOP_OPEN(vp, mode, cred, p)	\
	((vp)->ops->open((vp), (mode), (cred), (p)))
#define VOP_CLOSE(vp, mode, cred, p)	\
	((vp)->ops->close((vp), (mode), (cred), (p)))
#define VOP_READ(vp, uio, ioflags, cred) \
	((vp)->ops->read((vp), (uio), (ioflags), (cred)))
#define VOP_WRITE(vp, uio, ioflags, cred) \
	((vp)->ops->write((vp), (uio), (ioflags), (cred)))
#define VOP_INACTIVE(vp, p)	\
	((vp)->ops->inactive((vp), (p)))
#define VOP_RECLAIM(vp) \
	((vp)->ops->reclaim(vp))
#define VOP_STRATEGY(bp) \
	((bp)->vnode->ops->strategy(bp))
#define VOP_LOOKUP(dvp, name, vpp) \
	((dvp)->ops->lookup((dvp), (name), (vpp)))
#define VOP_CREATE(dvp, name, imode, vpp) \
	((dvp)->ops->create((dvp), (name), (imode), (vpp)))
#define VOP_BMAP(vp, lblkno, vpp, blkno, runp) \
	((vp)->ops->bmap((vp), (lblkno), (vpp), (blkno), (runp)))
/* We do not need this because currently all operations are sync. */
#define VOP_FSYNC(vp, cred, p)

/* ioflags */
#define IO_APPEND	0x02	/* Append to end of file when writing */

int getnewvnode(struct mount *, struct vops *, struct vnode **);
void vlock(struct vnode *);
bool vtrylock(struct vnode *);
void vunlock(struct vnode *);
int vrele(struct vnode *);
void vref(struct vnode *);
void vget(struct vnode *);
void vput(struct vnode *);
void vwakeup(struct vnode *);
int vwaitforio(struct vnode *);
int vinvalbuf(struct vnode *, struct ucred *, struct proc *);

int vn_read(struct vnode *, off_t, size_t, void *, int, enum uio_seg,
    struct proc *, struct mm *, struct ucred *);
int vn_write(struct vnode *, off_t, size_t, void *, int, enum uio_seg,
    struct proc *, struct mm *, struct ucred *);

extern dev_t rootdev;	/* initialized in mach_init() or arch_init() */
extern struct vnode *rootvp;	/* root disk device vnode */
extern struct vnode *rootvnode;	/* / */

#endif
