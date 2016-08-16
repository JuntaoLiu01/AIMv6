
#include <sys/types.h>
#include <buf.h>
#include <fs/bio.h>
#include <fs/vnode.h>
#include <fs/specdev.h>
#include <panic.h>
#include <aim/sync.h>
#include <sched.h>
#include <list.h>
#include <vmm.h>
#include <errno.h>
#include <libc/string.h>
#include <aim/initcalls.h>

struct buf *buf_get(struct vnode *vp, off_t lblkno, size_t nbytes);

struct allocator_cache bufpool = {
	.size = sizeof(struct buf),
	.align = 1,
	.flags = GFP_ZERO,
	.create_obj = NULL,
	.destroy_obj = NULL
};

int
binit(void)
{
	spinlock_init(&bufpool.lock);
	assert(cache_create(&bufpool) == 0);
	return 0;
}
INITCALL_FS(binit);

void
ballocdata(struct buf *bp)
{
	struct pages p;
	p.size = ALIGN_ABOVE(bp->nbytes, PAGE_SIZE);
	p.flags = 0;
	alloc_pages(&p);
	bp->data = pa2kva(p.paddr);
}

void
bfreedata(struct buf *bp)
{
	struct pages p;
	p.size = ALIGN_ABOVE(bp->nbytes, PAGE_SIZE);
	p.paddr = kva2pa(bp->data);
	p.flags = 0;
	free_pages(&p);
	bp->data = NULL;
}

/*
 * Find a struct buf in vnode @vp whose starting block is @blkno, spanning
 * @nbytes bytes.  Mark the buf BUSY.
 *
 * bget() does not check for inconsistencies of number of blocks, overlaps
 * of cached buf's etc.
 *
 * The only cause for the return value to be NULL is out of memory.
 */
struct buf *
bget(struct vnode *vp, off_t lblkno, size_t nbytes)
{
	bool lock;
	struct buf *bp;
	unsigned long flags;

	assert(vp != NULL);
	lock = !!(vp->flags & VXLOCK);
	if (!lock)
		vlock(vp);
	local_irq_save(flags);

	kpdebug("bget %p %d %d\n", vp, lblkno, nbytes);

restart:
	for_each_entry (bp, &vp->buf_head, node) {
		assert(bp->vnode == vp);
		if (bp->lblkno == lblkno) {
			assert(bp->nbytes == nbytes);
			if (!(bp->flags & B_BUSY)) {
				bp->flags |= B_BUSY;
				kpdebug("bget found cached %p\n", bp);
				local_irq_restore(flags);
				if (!lock)
					vunlock(vp);
				return bp;
			}
			sleep(bp);	/* brelse() */
			goto restart;
		}
	}

	bp = buf_get(vp, lblkno, nbytes);

	local_irq_restore(flags);
	if (!lock)
		vunlock(vp);

	return bp;
}

struct buf *
bgetempty(size_t nbytes)
{
	kpdebug("bgetempty() getting %d bytes\n", nbytes);
	return buf_get(NULL, 0, nbytes);
}

/*
 * Allocates a buf.
 * Assumes that the lock is held.
 */
struct buf *
buf_get(struct vnode *vp, off_t lblkno, size_t nbytes)
{
	unsigned long flags;
	struct buf *bp;
	assert(vp == NULL || (vp->flags & VXLOCK));

	local_irq_save(flags);

	if (vp == NULL)
		goto create;	/* standalone buf */
	for_each_entry_reverse (bp, &vp->buf_head, node) {
		if (!(bp->flags & (B_BUSY | B_DIRTY))) {
			bfreedata(bp);
			kpdebug("buf_get() recycle %p\n", bp);
			goto initbp;
		}
	}

create:
	bp = cache_alloc(&bufpool);
	if (bp == NULL) {
		local_irq_restore(flags);
		return NULL;
	}
	memset(bp, 0, sizeof(*bp));
	if (vp != NULL)
		list_add_tail(&(bp->node), &(vp->buf_head));
	kpdebug("buf_get() creating new %p\n", bp);
initbp:
	bp->flags = B_BUSY | B_INVALID;
	bp->nbytes = nbytes;
	bp->blkno = BLKNO_INVALID;
	if (vp != NULL) {
		bp->lblkno = lblkno;
		bgetvp(vp, bp);
	}

	ballocdata(bp);

	local_irq_restore(flags);
	return bp;
}

/*
 * Associate a buf with a vnode.
 */
void
bgetvp(struct vnode *vp, struct buf *bp)
{
	bp->vnode = vp;
	if (vp->type == VCHR || vp->type == VBLK)
		bp->devno = vp->specinfo->devno;
}

void
brelvp(struct buf *bp)
{
	kpdebug("brelvp() releasing %p\n", bp);
	/* currently we do nothing */
}

static struct buf *
bio_doread(struct vnode *vp, off_t blkno, size_t nbytes, uint32_t flags)
{
	struct buf *bp;
	unsigned long intr_flags;

	local_irq_save(intr_flags);
	bp = bget(vp, blkno, nbytes);
	if (bp == NULL)
		return NULL;
	if ((bp->flags & B_INVALID) && !(bp->flags & B_DIRTY))
		VOP_STRATEGY(bp);
	local_irq_restore(intr_flags);

	return bp;
}

/*
 * Construct a buf and read the contents.
 *
 * Should be released by brelse() after use.
 */
int
bread(struct vnode *vp, off_t blkno, size_t nbytes, struct buf **bpp)
{
	struct buf *bp;

	bp = *bpp = bio_doread(vp, blkno, nbytes, 0);
	if (bp == NULL)
		return -ENOMEM;
	return biowait(bp);
}

/*
 * Block write with given buf.
 */
int
bwrite(struct buf *bp)
{
	unsigned long flags;

	local_irq_save(flags);
	bp->vnode->noutputs++;
	bp->flags |= B_DIRTY;
	VOP_STRATEGY(bp);
	local_irq_restore(flags);

	return biowait(bp);
}

int
biowait(struct buf *bp)
{
	unsigned long flags;
	int err = 0;

	local_irq_save(flags);
	while (!(bp->flags & B_DONE))
		sleep(bp);	/* biodone() */
	if (bp->flags & B_EINTR) {
		bp->flags &= ~B_EINTR;
		err = -EINTR;
	}
	if (bp->flags & B_ERROR) {
		err = bp->errno ? bp->errno : -EIO;
	}
	local_irq_restore(flags);
	return err;
}

int
biodone(struct buf *bp)
{
	unsigned long flags;

	local_irq_save(flags);
	assert(!(bp->flags & B_DONE));
	bp->flags |= B_DONE;

	if (bp->flags & B_DIRTY)
		vwakeup(bp->vnode);
	bp->flags &= ~(B_DIRTY | B_INVALID);

	wakeup(bp);	/* biowait() */
	local_irq_restore(flags);
	return 0;
}

/*
 * Release a buffer and make it available in buf cache, or free the buf if
 * it's standalone.
 */
void
brelse(struct buf *bp)
{
	unsigned long flags;
	bool standalone;
	if (bp == NULL)
		return;
	standalone = (bp->vnode == NULL);
	local_irq_save(flags);

	/*
	 * brelse() does *NOT* check whether the I/O is truly finished.  It is
	 * left to the caller to ensure that the I/O is done, either succeeded
	 * or failed (XXX?).
	 * bread() and bwrite() automatically does that for you (by calling
	 * biowait()).  But some drivers may directly call the xxstrategy()
	 * calls, and in that case, biowait() should be called manually.
	 */
	kpdebug("brelse() releasing %snode %p\n", standalone ? "standalone " : "", bp);
	assert(bp->flags & B_BUSY);

	bp->flags &= ~B_BUSY;
	if (standalone)
		bdestroy(bp);
	else
		brelvp(bp);
	wakeup(bp);	/* bget() */

	local_irq_restore(flags);
}

void
bdestroy(struct buf *bp)
{
	kpdebug("bdestroy %p\n", bp);
	bfreedata(bp);
	cache_free(&bufpool, bp);
}

