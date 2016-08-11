
#include <sys/types.h>
#include <fs/vnode.h>
#include <fs/uio.h>
#include <fs/bio.h>
#include <fs/ufs/inode.h>
#include <fs/ufs/ext2fs/ext2fs.h>
#include <fs/ufs/ext2fs/dinode.h>
#include <buf.h>
#include <ucred.h>
#include <panic.h>
#include <errno.h>

int
ext2fs_read(struct vnode *vp, struct uio *uio, int ioflags, struct ucred *cred)
{
	struct buf *bp = NULL;
	struct m_ext2fs *fs = VTOI(vp)->superblock;
	off_t fsb, offset = uio->offset;
	size_t len = 0, b_off, bytesinfile;
	int i;
	int err;

	/* sanity checks */
	for (i = 0; i < uio->iovcnt; ++i)
		len += uio->iov[i].iov_len;
	assert(len == uio->resid);
	assert(uio->rw == UIO_READ);

	fsb = lblkno(fs, offset);
	b_off = lblkoff(fs, offset);

	while (uio->resid > 0) {
		bytesinfile = ext2fs_getsize(VTOI(vp)) - offset;
		if (bytesinfile <= 0)
			break;
		err = bread(vp, fsb, fs->bsize, &bp);
		if (err) {
			brelse(bp);
			return err;
		}
		len = min3(uio->resid, fs->bsize - b_off, bytesinfile);
		err = uiomove(bp->data + b_off, len, uio);
		if (err) {
			brelse(bp);
			return err;
		}
		b_off = 0;
		offset += len;
		++fsb;
		brelse(bp);
	}

	VTOI(vp)->flags |= IN_ACCESS;
	return 0;
}

int
ext2fs_write(struct vnode *vp, struct uio *uio, int ioflags, struct ucred *cred)
{
	struct inode *ip = VTOI(vp);
	struct m_ext2fs *fs = ip->superblock;
	size_t len = 0;
	size_t resid, osize, xfersize;
	off_t lbn, blkoffset, fileoffset = uio->offset;
	int i, err;
	struct buf *bp;

	/* sanity checks */
	for (i = 0; i < uio->iovcnt; ++i)
		len += uio->iov[i].iov_len;
	assert(len == uio->resid);
	assert(uio->rw == UIO_WRITE);

	/*
	 * If writing 0 bytes, do not change update time or file offset
	 * (standards compliance)
	 */
	if (uio->resid == 0)
		return 0;

	switch (vp->type) {
	case VREG:
		if (ioflags & IO_APPEND)
			fileoffset = ext2fs_getsize(ip);
		/* fallthru */
	case VLNK:
	case VDIR:
		break;
	default:
		panic("%s: bad type %d\n", __func__, vp->type);
	}

	if (fileoffset + uio->resid < uio->resid ||	/* overflowing */
	    fileoffset + uio->resid > fs->maxfilesize)
		return -EFBIG;

	resid = uio->resid;
	osize = ext2fs_getsize(ip);

	for (err = 0; uio->resid > 0; ) {
		lbn = lblkno(fs, fileoffset);
		blkoffset = lblkoff(fs, fileoffset);
		xfersize = min2(uio->resid, fs->bsize - blkoffset);

		err = ext2fs_buf_alloc(ip, lbn, cred, &bp);
		if (err)
			break;
		if (fileoffset + xfersize > ext2fs_getsize(ip)) {
			err = ext2fs_setsize(ip, fileoffset + xfersize);
			if (err) {
				brelse(bp);
				break;
			}
		}

		err = uiomove(bp->data + blkoffset, xfersize, uio);
		if (err) {
			brelse(bp);
			break;
		}

		err = bwrite(bp);
		brelse(bp);
		if (err)
			break;
		ip->flags |= IN_CHANGE | IN_UPDATE;
		fileoffset += xfersize;
	}

	if (err) {
		/* Rollback everything */
		ext2fs_truncate(ip, osize, cred);
		uio->resid = resid;
	} else if (resid > uio->resid) {
		ext2fs_update(ip);
	}

	return err;
}

