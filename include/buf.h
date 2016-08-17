
#ifndef _BUF_H
#define _BUF_H

#include <sys/types.h>
#include <list.h>
#include <sys/param.h>
#include <vmm.h>

struct vnode;	/* fs/vnode.h */

struct buf {
	dev_t		devno;
	soff_t		blkno;		/* physical block number on *device* */
#define BLKNO_INVALID	((soff_t)(-1))	/* need bmap() to find the number */
	off_t		lblkno;		/* logical block number in vnode */
	size_t		nbytes;
	uint32_t	flags;
#define B_DIRTY		0x1		/* need to write */
#define B_BUSY		0x2		/* in use */
#define B_INVALID	0x4		/* need to read if clean */
	/* Block I/O status flags set by interrupt callback */
#define B_DONE		0x8		/* done, either succeeded or failed */
#define B_EINTR		0x10		/* interrupted */
#define B_ERROR		0x20		/* failed */
	int		errno;		/* valid if B_ERROR, 0 if unsure */
	void		*data;
	struct vnode	*vnode;		/* associate vnode, NULL means
					 * allocated by bgetempty() */
	struct list_head node;		/* vnode buf cache list node */
	struct list_head ionode;	/* device io list node */
	size_t		nbytesrem;	/* # of bytes not finished */
};

extern struct allocator_cache bufpool;

#endif
