
#include <sys/types.h>
#include <fs/vnode.h>
#include <fs/specdev.h>
#include <fs/namei.h>
#include <fs/mount.h>
#include <fs/bio.h>
#include <fs/uio.h>
#include <vmm.h>
#include <errno.h>
#include <libc/string.h>
#include <aim/sync.h>
#include <aim/initcalls.h>
#include <sched.h>
#include <panic.h>
#include <proc.h>
#include <percpu.h>
#include <atomic.h>
#include <ucred.h>
#include <fcntl.h>

struct allocator_cache vnodepool = {
	.size = sizeof(struct vnode),
	.align = 1,
	.flags = 0,
	.create_obj = NULL,
	.destroy_obj = NULL
};

struct vnode *rootvp;
struct vnode *rootvnode;

int
vnodeinit(void)
{
	spinlock_init(&vnodepool.lock);
	assert(cache_create(&vnodepool) == 0);
	return 0;
}
INITCALL_FS(vnodeinit);

/* Allocate a vnode mounted on @mp with operations @ops. */
int
getnewvnode(struct mount *mp, struct vops *ops, struct vnode **vpp)
{
	struct vnode *vp;

	vp = cache_alloc(&vnodepool);
	if (vp == NULL) {
		*vpp = NULL;
		return -ENOMEM;
	}
	memset(vp, 0, sizeof(*vp));

	spinlock_init(&vp->lock);
	vp->ops = ops;
	vp->refs = 1;
	vp->type = VNON;
	list_init(&vp->buf_head);
	list_init(&vp->mount_node);

	insmntque(vp, mp);

	*vpp = vp;
	return 0;
}

/* Lock a vnode for exclusive access */
void
vlock(struct vnode *vp)
{
	kpdebug("vlock %p\n", vp);
	spin_lock(&(vp->lock));
	while (vp->flags & VXLOCK)
		sleep_with_lock(vp, &(vp->lock));
	vp->flags |= VXLOCK;
	spin_unlock(&(vp->lock));
	kpdebug("vlocked %p\n", vp);
}

/* Try locking a vnode */
bool
vtrylock(struct vnode *vp)
{
	kpdebug("vtrylock %p\n", vp);
	spin_lock(&(vp->lock));
	if (!(vp->flags & VXLOCK)) {
		vp->flags |= VXLOCK;
		kpdebug("vtrylocked %p\n", vp);
		return true;
	} else {
		kpdebug("vtrylock fail %p\n", vp);
		return false;
	}
}

/* Unlock a vnode */
void
vunlock(struct vnode *vp)
{
	assert(vp->flags & VXLOCK);

	spin_lock(&(vp->lock));
	vp->flags &= ~VXLOCK;
	wakeup(vp);
	spin_unlock(&(vp->lock));
	kpdebug("vunlocked %p\n", vp);
}

/*
 * Destroy a vnode.
 * Must be inactivated before calling.
 */
void
vgone(struct vnode *vp)
{
	unsigned long flags;

	kpdebug("destroying vnode %p\n", vp);
	/*
	 * vgone() is executed only if refs goes down to 0.  So if vgone()
	 * found that the vnode is locked, there must be another vgone()
	 * going on.
	 * FIXME: do we need to wait for vgone() to finish here?
	 */
	spin_lock_irq_save(&(vp->lock), flags);
	if (vp->flags & VXLOCK) {
		assert(vp->refs == 0);
		spin_unlock_irq_restore(&(vp->lock), flags);
		return;
	}
	vp->flags |= VXLOCK;
	spin_unlock_irq_restore(&(vp->lock), flags);

	/* XXX I think it's OK with NOCRED */
	vinvalbuf(vp, NOCRED, current_proc);

	assert(VOP_RECLAIM(vp) == 0);

	if (vp->mount != NULL)
		insmntque(vp, NULL);
	/* Clean up specinfo */
	if ((vp->type == VBLK || vp->type == VCHR) && vp->specinfo != NULL) {
		list_del(&vp->specinfo->spec_node);
		cache_free(&specinfopool, vp->specinfo);
	}
	/* Free vnode */
	cache_free(&vnodepool, vp);
}

/*
 * vref() - increment reference count
 */
void
vref(struct vnode *vp)
{
	kpdebug("vref %p\n", vp);
	atomic_inc(&(vp->refs));
}

void
vget(struct vnode *vp)
{
	kpdebug("vget %p\n", vp);
	vlock(vp);
	atomic_inc(&(vp->refs));
}

void
vwakeup(struct vnode *vp)
{
	unsigned long flags;

	local_irq_save(flags);
	if (vp != NULL) {
		assert(vp->noutputs > 0);
		if (--(vp->noutputs) == 0)
			wakeup(&vp->noutputs);
	}
	local_irq_restore(flags);
}

/*
 * vput() - unlock and release.  Of course, assumes the vnode to be locked.
 */
void
vput(struct vnode *vp)
{
	kpdebug("vput %p refs %d\n", vp, vp->refs);
	atomic_dec(&(vp->refs));
	if (vp->refs > 0) {
		vunlock(vp);
		return;
	}
	VOP_INACTIVE(vp, current_proc);	/* unlock */
	vgone(vp);
	kpdebug("vput %p done\n", vp);
}

int
vrele(struct vnode *vp)
{
	kpdebug("vrele %p refs %d\n", vp, vp->refs);
	atomic_dec(&(vp->refs));
	if (vp->refs > 0)
		return 0;
	vlock(vp);
	VOP_INACTIVE(vp, current_proc);	/* unlock */
	vgone(vp);
	return 0;
}

/* Assume vnode is locked */
int
vwaitforio(struct vnode *vp)
{
	kpdebug("vwaitforio %p\n", vp);
	while (vp->noutputs) {
		sleep(&vp->noutputs);
	}
	kpdebug("vwaitforio %p done\n", vp);
	return 0;
}

/*
 * Invalidate all buf's in vnode @vp:
 * Wait for all I/O's to finish, sync all dirty buf's, clean the vnode buf list
 * and free all elements inside.
 * Assumes vnode is locked.
 */
int
vinvalbuf(struct vnode *vp, struct ucred *cred, struct proc *p)
{
	struct buf *bp, *bnext;
	unsigned long flags;

	kpdebug("vinvalbuf %p\n", vp);

	vwaitforio(vp);
	VOP_FSYNC(vp, cred, p);

	local_irq_save(flags);
loop:
	for_each_entry_safe (bp, bnext, &vp->buf_head, node) {
		if (bp->flags & B_BUSY) {
			sleep(bp);
			goto loop;
		}
		assert(!(bp->flags & B_DIRTY));
		list_del(&bp->node);
		bdestroy(bp);
	}
	local_irq_restore(flags);

	kpdebug("vinvalbuf %p done\n", bp);
	return 0;
}

/*
 * A convenient wrapper of VOP_READ for kernel use, which reads one buffer.
 * Assumes a locked vnode.
 */
int
vn_rw(struct vnode *vp, off_t offset, size_t len, void *buf, int ioflags,
    enum uio_rw rw, enum uio_seg seg, struct proc *p, struct mm *mm,
    struct ucred *cred)
{
	struct uio uio;
	struct iovec iovec;

	iovec.iov_base = buf;
	iovec.iov_len = len;
	uio.iov = &iovec;
	uio.iovcnt = 1;
	uio.offset = offset;
	uio.resid = len;
	uio.rw = rw;
	uio.seg = seg;
	uio.proc = p;
	uio.mm = mm;

	switch (rw) {
	case UIO_READ:
		return VOP_READ(vp, &uio, ioflags, cred);
	case UIO_WRITE:
		return VOP_WRITE(vp, &uio, ioflags, cred);
	}
	/* NOTREACHED */
	return -EINVAL;
}

int
vn_read(struct vnode *vp, off_t offset, size_t len, void *buf, int ioflags,
    enum uio_seg seg, struct proc *p, struct mm *mm, struct ucred *cred)
{
	return vn_rw(vp, offset, len, buf, ioflags, UIO_READ, seg, p, mm, cred);
}

int
vn_write(struct vnode *vp, off_t offset, size_t len, void *buf, int ioflags,
    enum uio_seg seg, struct proc *p, struct mm *mm, struct ucred *cred)
{
	return vn_rw(vp, offset, len, buf, ioflags, UIO_WRITE, seg, p, mm, cred);
}

int
vn_open(char *path, int flags, int mode, struct nameidata *nd)
{
	struct vnode *vp;
	struct vattr va;
	int err;

	if (!(flags & (FREAD | FWRITE)))
		return -EINVAL;		/* u wot m8 */
	nd->path = path;
	nd->flags = 0;
	if (flags & O_CREAT) {
		nd->intent = NAMEI_CREATE;
		nd->flags |= NAMEI_PARENT;
		if (!(flags & (O_NOFOLLOW | O_EXCL)))
			nd->flags |= NAMEI_FOLLOW;
		if ((err = namei(nd)) != 0)
			return err;

		if (nd->vp == NULL) {
			/* truly non-existent, create it */
			va.type = VREG;
			va.mode = mode;
			err = VOP_CREATE(nd, &va);
			if (err) {
				vput(nd->parentvp);
				namei_cleanup(nd);
				return err;
			}
			vput(nd->parentvp);
			vp = nd->vp;
		} else {
			if (nd->parentvp == nd->vp)
				vrele(nd->parentvp);	/* only decrease ref count */
			else
				vput(nd->parentvp);
			nd->parentvp = NULL;
			vp = nd->vp;
			if (flags & O_EXCL) {
				err = -EEXIST;
				goto bad;
			}
		}
	} else {
		nd->intent = NAMEI_LOOKUP;
		if (!(flags & O_NOFOLLOW))
			nd->flags |= NAMEI_FOLLOW;
		if ((err = namei(nd)) != 0)
			return err;
		vp = nd->vp;
	}

	if ((err = VOP_OPEN(vp, flags, nd->cred, nd->proc)) != 0)
		goto bad;

	return 0;
bad:
	vput(vp);
	return err;
}

