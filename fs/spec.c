
#include <asm-generic/funcs.h>
#include <list.h>
#include <fs/vnode.h>
#include <fs/specdev.h>
#include <fs/uio.h>
#include <fs/bio.h>
#include <ucred.h>
#include <buf.h>
#include <vmm.h>
#include <errno.h>
#include <aim/device.h>
#include <aim/initcalls.h>
#include <sys/types.h>
#include <sys/param.h>
#include <panic.h>
#include <percpu.h>

static struct list_head specinfo_list = EMPTY_LIST(specinfo_list);

struct allocator_cache specinfopool = {
	.size = sizeof(struct specinfo),
	.align = 1,
	.flags = 0,
	.create_obj = NULL,
	.destroy_obj = NULL
};

int
specinit(void)
{
	spinlock_init(&specinfopool.lock);
	assert(cache_create(&specinfopool) == 0);
	return 0;
}
INITCALL_FS(specinit);

int
spec_inactive(struct vnode *vp, struct proc *p)
{
	/* For a special vnode we just unlock it */
	vunlock(vp);
	return 0;
}

int
spec_open(struct vnode *vp, int mode, struct ucred *cred, struct proc *p)
{
	struct driver *drv;

	switch (vp->type) {
	case VCHR:
		/* TODO, currently fallthru */
	case VBLK:
		drv = devsw[major(vdev(vp))];
		return (drv->open)(vdev(vp), 0, p);
	default:
		return -ENODEV;
	}
	/* NOTREACHED */
}

int
spec_close(struct vnode *vp, int mode, struct ucred *cred, struct proc *p)
{
	bool lock = !!(vp->flags & VXLOCK);
	int err;
	struct driver *drv;

        kpdebug("closing spec vnode %p, lock: %d\n", vp, lock);
	switch (vp->type) {
	case VCHR:
		if (vp->refs > 1 && !lock)
			return 0;
		drv = devsw[major(vdev(vp))];
		err = (drv->close)(vdev(vp), 0, p);
		return err;
	case VBLK:
		/* Invalidate all buffers, which requires that the vnode is
		 * locked. */
		if (!lock)
			vlock(vp);
		err = vinvalbuf(vp, cred, p);
		if (err) {
			if (!lock)
				vunlock(vp);
			return err;
		}
		/* We really close the device only if (1) we are on last close
		 * (the ref count is 1), or (2) we are forcibly closing (the
		 * vnode is locked). */
		if (vp->refs > 1 && !lock) {
			vunlock(vp);
			return 0;
		}
		drv = devsw[major(vdev(vp))];
		err = (drv->close)(vdev(vp), 0, p);
		if (!lock)
			vunlock(vp);
		return err;
	default:
		panic("spec_close: not spec\n");
	}
	/* NOTREACHED */
	return 0;
}

int
spec_read(struct vnode *vp, struct uio *uio, int ioflags, struct ucred *cred)
{
	int err;
	struct driver *drv;
	off_t offset, bn;
	size_t on, n;
	struct buf *bp;

	assert(vp->type == VCHR || vp->type == VBLK);
	assert(uio->rw == UIO_READ);
	if (uio->seg == UIO_USER)
		assert(uio->proc == current_proc);

	vunlock(vp);
	switch (vp->type) {
	case VCHR:
		drv = devsw[major(vdev(vp))];
		err = ((struct chr_driver *)drv)->read(vdev(vp), uio, ioflags);
		vlock(vp);
		return err;
	case VBLK:
		/* Assumes all block devices have their block size 512 */
		if (uio->offset < 0)
			return -EINVAL;
		assert(uio->rw == UIO_READ);
		offset = uio->offset;
		do {
			/* Read in the data sector by sector */
			bn = offset / SECTOR_SIZE;
			on = offset % SECTOR_SIZE;
			n = min2(SECTOR_SIZE - on, uio->resid);

			err = bread(vp, bn, SECTOR_SIZE, &bp);
			if (err) {
				brelse(bp);
				return err;
			}
			err = uiomove(bp->data + on, n, uio);
			brelse(bp);
			offset += n;
		} while (err == 0 && uio->resid > 0 && n != 0);

		vlock(vp);
		return err;
	default:
		panic("spec_read: not spec\n");
	}
	/* NOTREACHED */
	return 0;
}

int
spec_write(struct vnode *vp, struct uio *uio, int ioflags, struct ucred *cred)
{
	int err;
	struct driver *drv;
	off_t offset, bn;
	size_t on, n;
	struct buf *bp;

	assert(vp->type == VCHR || vp->type == VBLK);
	assert(uio->rw == UIO_WRITE);
	if (uio->seg == UIO_USER)
		assert(uio->proc == current_proc);

	vunlock(vp);
	switch (vp->type) {
	case VCHR:
		drv = devsw[major(vdev(vp))];
		err = ((struct chr_driver *)drv)->write(vdev(vp), uio, ioflags);
		vlock(vp);
		return err;
	case VBLK:
		if (uio->offset < 0)
			return -EINVAL;
		if (uio->resid == 0)
			return 0;
		assert(uio->rw == UIO_WRITE);
		offset = uio->offset;
		do {
			/* write out the data sector by sector */
			bn = offset / SECTOR_SIZE;
			on = offset % SECTOR_SIZE;
			n = min2(SECTOR_SIZE - on, uio->resid);

			err = bread(vp, bn, SECTOR_SIZE, &bp);
			if (err) {
				brelse(bp);
				return err;
			}
			err = uiomove(bp->data + on, n, uio);
			/* keep writing regardless of errors */
			bwrite(bp);
			brelse(bp);
			offset += n;
		} while (err == 0 && uio->resid > 0 && n != 0);

		vlock(vp);
		return err;
	default:
		panic("spec_write: not spec\n");
	}
	/* NOTREACHED */
	return 0;
}

/*
 * strategy() operation on spec vnodes require that only the following members
 * could be modified, as regular files may invoke this operation to interact
 * with low-level drivers (see ufs_strategy() for one example):
 * 1. nbytesrem
 * 2. blkno (only if bp->vnode is a spec vnode)
 * 3. data
 * 4. flags
 * 5. ionode
 */
int
spec_strategy(struct buf *bp)
{
	struct driver *drv;

	drv = devsw[major(bp->devno)];
	assert(drv != NULL);
	assert(drv->class == DEVCLASS_BLK);
	if (bp->blkno == BLKNO_INVALID) {
		assert(bp->vnode->type == VCHR || bp->vnode->type == VBLK);
		bp->blkno = bp->lblkno;
	}

	return (((struct blk_driver *)drv)->strategy)(bp);
}

struct specinfo *
findspec(dev_t devno)
{
	struct specinfo *si;

	for_each_entry (si, &specinfo_list, spec_node) {
		if (si->devno == devno)
			return si;
	}

	return NULL;
}

struct specinfo *
newspec(dev_t devno)
{
	struct specinfo *si = cache_alloc(&specinfopool);
	si->devno = devno;
	list_add_tail(&(si->spec_node), &specinfo_list);

	return si;
}

/*
 * Get or create the vnode corresponding to the device @devno.
 * Currently we require that each device is matched exactly by one vnode.
 */
int
getdevvp(dev_t devno, struct vnode **vpp, enum vtype type)
{
	struct specinfo *si;
	struct vnode *vp;
	int err;

	si = findspec(devno);
	if (si != NULL) {
		*vpp = si->vnode;
		return 0;
	}

	err = getnewvnode(NULL, &spec_vops, &vp);
	if (err != 0) {
		*vpp = NULL;
		return err;
	}

	vp->type = type;
	/* Allocate and fill in a specinfo structure */
	si = newspec(devno);
	si->vnode = vp;
	vp->specinfo = si;

	*vpp = vp;
	return 0;
}

/* Get or create a block device vnode */
int
bdevvp(dev_t devno, struct vnode **vpp)
{
	return getdevvp(devno, vpp, VBLK);
}

/* Get or create a block device vnode */
int
cdevvp(dev_t devno, struct vnode **vpp)
{
	return getdevvp(devno, vpp, VCHR);
}

dev_t
vdev(struct vnode *vp)
{
	if (vp->type != VCHR && vp->type != VBLK)
		return NODEV;
	else
		return vp->specinfo->devno;
}

struct vops spec_vops = {
	.open = spec_open,
	.close = spec_close,
	.read = spec_read,
	.write = spec_write,
	.inactive = spec_inactive,
	.reclaim = NOP,
	.strategy = spec_strategy,
	.lookup = NOTSUP,
	.create = NOTSUP,
	.bmap = NOTSUP,
	.link = NOTSUP,
	.remove = NOTSUP,
	.access = NOP,
	.mkdir = NOTSUP,
	.readdir = NOTSUP,
	.rmdir = NOTSUP,
	.getattr = NOTSUP,
	.setattr = NOTSUP,
	.fsync = NOP,	/* currently spec ops are always sync */
};

