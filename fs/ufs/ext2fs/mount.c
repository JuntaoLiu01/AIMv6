
#include <fs/vnode.h>
#include <fs/specdev.h>
#include <fs/mount.h>
#include <fs/vfs.h>
#include <fs/bio.h>
#include <fs/ufs/ufsmount.h>
#include <fs/ufs/ext2fs/ext2fs.h>
#include <fs/ufs/ext2fs/dinode.h>
#include <aim/initcalls.h>
#include <panic.h>
#include <ucred.h>
#include <fcntl.h>
#include <proc.h>
#include <percpu.h>
#include <mach-conf.h>
#include <libc/string.h>
#include <limits.h>
#include <errno.h>
#include <pmm.h>

int ext2fs_mountfs(struct vnode *, struct mount *, struct proc *);

int
ext2fs_mountroot(void)
{
	int err;
	struct mount *mp;

	if (bdevvp(rootdev, &rootvp))
		panic("ext2fs_mountroot: can't setup bdevvp's\n");

	if ((err = vfs_rootmountalloc("ext2fs", &mp)) != 0) {
		kpdebug("rolling back rootvp\n");
		goto rollback_rootvp;
	}

	if ((err = ext2fs_mountfs(rootvp, mp, current_proc)) != 0) {
		kpdebug("rolling back mountalloc\n");
		goto rollback_mountalloc;
	}

	addmount(mp);

	return 0;

rollback_mountalloc:
	vfs_mountfree(&mp);
rollback_rootvp:
	vrele(rootvp);
	return err;
}

/*
 * Check if the on-disk super block is valid.
 * Very ext2-specific, mostly comes from OpenBSD code and is not particularly
 * interesting.
 */
int
ext2fs_sbinit(struct vnode *devvp, struct ext2fs *ondiskfs, struct m_ext2fs *sb)
{
	struct ext2fs *fs = &sb->e2fs;
	int err, i;
	struct buf *bp = NULL;
	struct pages p;	/* for allocating in-memory group descriptors */

	e2fs_sbload(ondiskfs, fs);

	if (fs->magic != E2FS_MAGIC ||
	    fs->log_bsize > 2 ||
	    fs->bpg == 0 ||
	    fs->rev > 1 ||
	    /*
	     * We only support ext2 rev. 0 so we do not check for extended
	     * features.  However, when Linux mounts an ext2 rev. 0 file system,
	     * it changes the revision number to 1 (which is harmless but
	     * annoying).  So we have to ensure that no rev. 1 features are
	     * turned on (which is mostly the case) with additional logic.
	     */
	    fs->features_incompat != 0 ||
	    fs->features_rocompat != 0 ||
	    fs->inode_size != EXT2_REV0_DINODE_SIZE) {
		kprintf("ext2fs invalid: %x %d %d %d %d %d %d\n", fs->magic,
		    fs->log_bsize, fs->bpg, fs->rev, fs->features_incompat,
		    fs->features_rocompat, fs->inode_size);
		return -EINVAL;
	}

	sb->ncg = DIV_ROUND_UP(fs->bcount - fs->first_dblock, fs->bpg);
	sb->fsbtodb = fs->log_bsize + 1;
	sb->bsize = MINBSIZE << fs->log_bsize;
	sb->bsects = sb->bsize / SECTOR_SIZE;
	sb->bshift = LOG_MINBSIZE + fs->log_bsize;
	sb->fsize = MINFSIZE << fs->log_fsize;
	sb->qbmask = sb->bsize - 1;
	sb->bmask = ~sb->qbmask;
	sb->ipb = sb->bsize / EXT2_DINODE_SIZE(sb);
	sb->itpg = fs->ipg / sb->ipb;

	/* Load group descriptors */
	sb->ngdb = DIV_ROUND_UP(sb->ncg, sb->bsize / sizeof(struct ext2_gd));
	p.size = ALIGN_ABOVE(sb->ngdb * sb->bsize, PAGE_SIZE);
	p.flags = 0;
	if (alloc_pages(&p) < 0)
		return -ENOMEM;
	sb->gd = pa2kva(p.paddr);

	for (i = 0; i < sb->ngdb; ++i) {
		unsigned int dblk = ((sb->bsize > 1024) ? 1 : 2) + i;
		size_t gdesc = i * sb->bsize / sizeof(struct ext2_gd);
		struct ext2_gd *gd;

		err = bread(devvp, fsbtodb(sb, dblk), sb->bsize, &bp);
		if (err) {
			free_pages(&p);
			sb->gd = NULL;
			brelse(bp);
			return err;
		}
		gd = bp->data;
		e2fs_cgload(gd, &sb->gd[gdesc], sb->bsize);
		brelse(bp);
		bp = NULL;
	}

	sb->maxfilesize = INT_MAX;	/* Rev. 0 */

	/* temporary, will be removed in release */
	extern void __ext2fs_dump(struct vnode *, struct m_ext2fs *);
	__ext2fs_dump(devvp, sb);

	return 0;
}

int
ext2fs_mountfs(struct vnode *devvp, struct mount *mp, struct proc *p)
{
	int err;
	struct buf *bp = NULL;
	struct ext2fs *fs = NULL;
	struct m_ext2fs *sb = NULL;
	struct ufsmount *ump = NULL;

	/*
	 * TODO:
	 * Disallow multiple mounts on the same device
	 * Disallow mounting a device in use
	 */

	kpdebug("opening rootdev\n");
	err = VOP_OPEN(devvp, FREAD | FWRITE, NOCRED, p);
	if (err != 0)
		return err;

	vlock(devvp);
	kpdebug("reading ext2fs super block\n");
	err = bread(devvp, SBOFF / SECTOR_SIZE, SBSIZE, &bp);
	if (err != 0)
		goto rollback_open;

	sb = kmalloc(sizeof(*sb), 0);
	ump = kmalloc(sizeof(*ump), GFP_ZERO);
	if (sb == NULL || ump == NULL)
		goto rollback_buf;
	fs = bp->data;
	err = ext2fs_sbinit(devvp, fs, sb);
	if (err != 0)
		goto rollback_alloc;

	ump->mount = mp;
	ump->devno = vdev(devvp);
	ump->devvp = devvp;
	ump->type = UM_EXT2;
	ump->nindir = NINDIR(sb);
	ump->fsbtodb = ump->bptrtodb = sb->fsbtodb;
	ump->seqinc = 1;	/* no frags */
	ump->superblock = sb;
	mp->data = ump;
	devvp->specinfo->mountpoint = mp;

	brelse(bp);
	vunlock(devvp);
	return 0;

rollback_alloc:
	kfree(sb);
	kfree(ump);
rollback_buf:
rollback_open:
	brelse(bp);
	VOP_CLOSE(devvp, FREAD | FWRITE, NOCRED, p);
	vunlock(devvp);

	return err;
}

