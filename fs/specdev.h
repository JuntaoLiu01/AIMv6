
#ifndef _FS_SPECDEV_H
#define _FS_SPECDEV_H

#include <sys/types.h>
#include <list.h>
#include <vmm.h>

enum vtype;	/* fs/vnode.h */
struct vnode;	/* fs/vnode.h */
struct vops;	/* fs/vnode.h */
struct ucred;	/* include/ucred.h */
struct proc;	/* include/proc.h */
struct buf;	/* include/buf.h */

struct specinfo {
	dev_t		devno;
	struct vnode	*vnode;
	struct mount	*mountpoint;

	struct list_head spec_node;
};

int bdevvp(dev_t, struct vnode **);
int cdevvp(dev_t, struct vnode **);
int getdevvp(dev_t, struct vnode **, enum vtype);
struct specinfo *findspec(dev_t);
struct specinfo *newspec(dev_t);
dev_t vdev(struct vnode *);

int spec_open(struct vnode *, int, struct ucred *, struct proc *);
int spec_close(struct vnode *, int, struct ucred *, struct proc *);
int spec_read(struct vnode *, struct uio *, int, struct ucred *);
int spec_write(struct vnode *, struct uio *, int, struct ucred *);
int spec_inactive(struct vnode *, struct proc *);
int spec_reclaim(struct vnode *);
int spec_strategy(struct buf *);

extern struct allocator_cache specinfopool;
extern struct vops spec_vops;

#endif
