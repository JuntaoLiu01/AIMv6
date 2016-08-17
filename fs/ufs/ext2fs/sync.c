
#include <fs/ufs/ufs.h>
#include <fs/ufs/inode.h>
#include <fs/ufs/ufsmount.h>
#include <fs/ufs/ext2fs/ext2fs.h>
#include <fs/bio.h>
#include <fs/mount.h>
#include <proc.h>
#include <ucred.h>
#include <errno.h>

int
ext2fs_sbupdate(struct ufsmount *ump)
{
	struct m_ext2fs *fs = ump->superblock;
	struct buf *bp;
	int err;

	bp = bget(ump->devvp, fsbtodb(fs, (fs->bsize > 1024 ? 0 : 1)),
	    fs->bsize);
	e2fs_sbsave(&fs->e2fs,
	    (struct ext2fs *)(bp->data + (fs->bsize > 1024 ? SBOFF : 0)));
	err = bwrite(bp);
	brelse(bp);
	return err;
}

int
ext2fs_cgupdate(struct ufsmount *ump)
{
	struct m_ext2fs *fs = ump->superblock;
	int i, err, allerr;
	struct buf *bp;

	allerr = ext2fs_sbupdate(ump);

	for (i = 0; i < fs->ngdb; ++i) {
		bp = bget(ump->devvp,
		    fsbtodb(fs, (fs->bsize > 1024 ? 0 : 1) + i + 1),
		    fs->bsize);
		if (bp == NULL)
			return -ENOMEM;
		e2fs_cgsave(&fs->gd[i * fs->bsize / sizeof(struct ext2_gd)],
		    (struct ext2_gd *)(bp->data),
		    fs->bsize);
		if ((err = bwrite(bp)) != 0 && allerr == 0)
			allerr = err;	/* continue writing anyway */
		brelse(bp);
	}

	return allerr;
}

int
ext2fs_sync(struct mount *mp, struct ucred *cred, struct proc *p)
{
	struct ufsmount *ump = VFSTOUFS(mp);
	struct m_ext2fs *fs = ump->superblock;
	struct vnode *vp;
	int err;

	/*
	 * Typically, a file system sync operation involves:
	 * 1. Writing back all modified inodes.
	 * 2. Flush all dirty buf's of the block device onto the storage
	 *    (unnecessary since our system does everything in sync).
	 * 3. Write back the superblock and update the cylinder group
	 *    descriptors.
	 * 4. Copy the superblock and cylinder group descriptors to
	 *    backup locations if you want.
	 */

	for_each_entry (vp, &mp->vnode_head, mount_node)
		VOP_FSYNC(vp, cred, p);

	kpdebug("syncing fs\n");
	if (fs->fmod != 0) {
		fs->fmod = 0;
		/* TODO: refresh write time (@wtime) */
		if ((err = ext2fs_cgupdate(ump)) != 0)
			return err;
	}
	return 0;
}

int
ext2fs_fsync(struct vnode *vp, struct ucred *cred, struct proc *p)
{
	/*
	 * Ordinarily we need to flush out buffers, but in AIMv6 all
	 * file I/O are sync, so we only update inode.
	 */
	kpdebug("syncing inode %d\n", VTOI(vp)->ino);
	return ext2fs_update(VTOI(vp));
}

