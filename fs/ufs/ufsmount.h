
#ifndef _FS_UFS_UFSMOUNT_H
#define _FS_UFS_UFSMOUNT_H

struct mount;	/* fs/mount.h */
struct vnode;	/* fs/vnode.h */

/*
 * This is the extra fs-specific mount data structure for a struct mount
 * of a UFS-like file system (including: FFS, ext2, etc.)
 */
struct ufsmount {
	struct mount	*mount;
	dev_t		devno;
	struct vnode	*devvp;
	int		type;
#define UM_EXT2		0x1
	/* In-memory super block.  This is NOT identical to the on-disk
	 * counterpart as it may contain more information. */
	void		*superblock;
	unsigned long	nindir;		/* indirect pointers per block */
	unsigned long	bptrtodb;	/* indirect pointer to disk blocks */
	unsigned long	fsbtodb;	/* file system block to disk blocks */
	unsigned long	seqinc;		/* increment between sequential blks */
};

#define VFSTOUFS(mp)	((struct ufsmount *)((mp)->data))

#endif
