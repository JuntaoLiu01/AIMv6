
#ifndef _FS_BIO_H
#define _FS_BIO_H

#include <sys/types.h>

struct vnode;	/* fs/vnode.h */

struct buf *bget(struct vnode *vp, off_t blkno, size_t nbytes);
struct buf *bgetempty(size_t nbytes);
void bgetvp(struct vnode *vp, struct buf *bp);
int bread(struct vnode *vp, off_t lblkno, size_t nbytes, struct buf **bpp);
int bwrite(struct buf *bp);
int biowait(struct buf *bp);
int biodone(struct buf *bp);
void brelse(struct buf *bp);
void bdestroy(struct buf *bp);

#endif
