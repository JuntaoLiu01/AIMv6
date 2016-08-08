
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
struct nameidata;	/* fs/namei.h */
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

struct vattr {
	enum vtype	type;	/* vnode type */
	int		mode;	/* file access mode and type */
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
	 * longer using the vnode (i.e. @refs == 0).
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
	 * Finds a directory entry by name (nd->seg) within nd->parentvp.
	 * The directory (nd->parentvp) should be locked.
	 * The returned vnode is retrieved via vget() hence locked, or NULL if
	 * the file does not exist.
	 * If the vnode is the same as directory, lookup() increases ref count.
	 * Note that lookup() still succeeds even if the file does not exist.
	 */
	int (*lookup)(struct vnode *, char *, struct vnode **, struct ucred *,
	    struct proc *);
	/*
	 * create:
	 * Create a regular file with given file name segment (nd->seg) as its
	 * name within nd->parentvp.
	 * The directory (nd->parentvp) should be locked.
	 * The returned vnode is retrieved via vget() hence locked.
	 * Does NOT check whether the file name already exists.
	 */
	int (*create)(struct vnode *, char *, struct vattr *, struct vnode **,
	    struct ucred *, struct proc *);
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
	/*
	 * link:
	 * Make a hard link.
	 * Assumes that source vnode @srcvp and directory vnode @dvp are
	 * locked.
	 */
	int (*link)(struct vnode *, char *, struct vnode *, struct ucred *,
	    struct proc *);
	/*
	 * remove:
	 * Remove a directory entry.
	 * Assumes that @vp indeed has an entry @name in @dvp.
	 * Assumes that @dvp and @vp are locked.
	 * For a file with @nlink == 0, the actual removal of data blocks and
	 * inode takes place in inactive().
	 */
	int (*remove)(struct vnode *, char *, struct vnode *, struct ucred *,
	    struct proc *);
	/*
	 * access:
	 * Check whether user credential @cred and process @p can indeed
	 * access file vnode @vp with required access permissions @acc.
	 * TODO: feel free to change this interface.
	 * TODO: insert VOP_ACCESS() calls into necessary positions in file
	 * system framework, and maybe in system calls (which are NYI).
	 */
	int (*access)(struct vnode *, int, struct ucred *, struct proc *);
	/*
	 * mkdir:
	 * Create a directory
	 */
	int (*mkdir)(struct vnode *, char *, struct vattr *, struct vnode **,
	    struct ucred *, struct proc *);
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
#define VOP_LOOKUP(dvp, name, vpp, cred, p) \
	((dvp)->ops->lookup((dvp), (name), (vpp), (cred), (p)))
#define VOP_CREATE(dvp, name, va, vpp, cred, p) \
	((dvp)->ops->create((dvp), (name), (va), (vpp), (cred), (p)))
#define VOP_BMAP(vp, lblkno, vpp, blkno, runp) \
	((vp)->ops->bmap((vp), (lblkno), (vpp), (blkno), (runp)))
#define VOP_LINK(dvp, name, vp, cred, p) \
	((dvp)->ops->link((dvp), (name), (vp), (cred), (p)))
#define VOP_REMOVE(dvp, name, vp, cred, p) \
	((dvp)->ops->remove((dvp), (name), (vp), (cred), (p)))
#define VOP_ACCESS(vp, acc, cred, p) \
	((vp)->ops->access((vp), (acc), (cred), (p)))
/* We do not need this because currently all operations are sync. */
#define VOP_MKDIR(dvp, name, va, vpp, cred, p) \
	((dvp)->ops->mkdir((dvp), (name), (va), (vpp), (cred), (p)))
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
    struct proc *, struct mm *, struct ucred *, size_t *);
int vn_write(struct vnode *, off_t, size_t, void *, int, enum uio_seg,
    struct proc *, struct mm *, struct ucred *, size_t *);
/* the core function of open(2), open3(2), openat(2), creat(2), etc. */
int vn_open(char *, int, int, struct nameidata *);

extern dev_t rootdev;	/* initialized in mach_init() or arch_init() */
extern struct vnode *rootvp;	/* root disk device vnode */
extern struct vnode *rootvnode;	/* / */

#endif
