
#include <fs/vnode.h>
#include <fs/uio.h>
#include <fs/ufs/inode.h>
#include <fs/ufs/ext2fs/dir.h>
#include <fs/ufs/ext2fs/dinode.h>
#include <fs/ufs/ext2fs/ext2fs.h>
#include <fs/namei.h>
#include <sys/stat.h>
#include <panic.h>
#include <errno.h>
#include <limits.h>

int
ext2fs_makeinode(int imode, struct vnode *dvp, char *name, struct vnode **vpp,
    struct ucred *cred, struct proc *p)
{
	int err;
	struct vnode *tvp;
	struct inode *ip;

	assert((imode & EXT2_IFMT) != 0);

	kpdebug("ext2fs makeinode %o %s %u\n", imode, name, VTOI(dvp)->ino);

	err = ext2fs_inode_alloc(dvp, imode, cred, &tvp);
	if (err)
		return err;

	ip = VTOI(tvp);
	/* TODO: set up uid and gid */
	ip->uid = ip->gid = 0;
	ip->flags |= IN_ACCESS | IN_CHANGE | IN_UPDATE;
	EXT2_DINODE(ip)->mode = imode;
	tvp->type = EXT2_IFTOVT(imode);
	EXT2_DINODE(ip)->nlink = 1;
	/* TODO: SGID? */

	/* To rollback ext2fs_inode_alloc we set # links to be 0, and
	 * call vput(tvp).  Since @tvp is just VGET'd in ext2fs_inode_alloc
	 * vput() will call VOP_INACTIVE(), which will delete the inode by
	 * updating dtime to be non-zero.  It will also call ext2fs_inode_free
	 * to erase the bit in inode bitmap. */
	if ((err = ext2fs_update(ip)) != 0)
		goto rollback_inode;
	if ((err = ext2fs_direnter(ip, dvp, name, cred)) != 0)
		goto rollback_inode;
	*vpp = tvp;
	return 0;

rollback_inode:
	EXT2_DINODE(ip)->nlink = 0;
	EXT2_DINODE(ip)->flags |= IN_CHANGE;	/* trigger update */
	tvp->type = VNON;
	vput(tvp);
	return err;
}

int
ext2fs_create(struct vnode *dvp, char *name, struct vattr *va,
    struct vnode **vpp, struct ucred *cred, struct proc *p)
{
	return ext2fs_makeinode(EXT2_MAKEIMODE(va->type, va->mode), dvp, name,
	    vpp, cred, p);
}

int
ext2fs_link(struct vnode *dvp, char *name, struct vnode *srcvp,
    struct ucred *cred, struct proc *p)
{
	struct inode *ip = VTOI(srcvp);
	int err;

	if (srcvp->type == VDIR)
		/*
		 * Allowing hard-linking directories is a BAD IDEA(TM).
		 */
		return -EISDIR;
	if (EXT2_DINODE(ip)->nlink >= LINK_MAX)
		return -EMLINK;

	EXT2_DINODE(ip)->nlink++;
	ip->flags |= IN_CHANGE;
	if ((err = ext2fs_update(ip)) == 0)
		ext2fs_direnter(ip, dvp, name, cred);
	if (err) {
		/* rollback :( */
		EXT2_DINODE(ip)->nlink--;
		ip->flags |= IN_CHANGE;
	}

	return err;
}

int
ext2fs_remove(struct vnode *dvp, char *name, struct vnode *vp,
    struct ucred *cred, struct proc *p)
{
	struct inode *ip = VTOI(vp);
	int err;

	if (vp->type == VDIR)
		/* Please use rmdir(2) instead */
		return -EPERM;

	err = ext2fs_dirremove(dvp, name, cred);
	if (err == 0) {
		EXT2_DINODE(ip)->nlink--;
		ip->flags |= IN_CHANGE;
	}

	return err;
}

int
ext2fs_mkdir(struct vnode *dvp, char *name, struct vattr *va,
    struct vnode **vpp, struct ucred *cred, struct proc *p)
{
	struct inode *dp = VTOI(dvp), *ip;
	struct m_ext2fs *fs = dp->superblock;
	struct ext2fs_direct dir;
	struct vnode *tvp;
	unsigned char dirtemplate[EXT2FS_DIRSIZ(1) + EXT2FS_DIRSIZ(2)];
	int dmode, err;

	if (EXT2_DINODE(dp)->nlink >= LINK_MAX)
		return -EMLINK;

	dmode = va->mode & ACCESSPERMS;
	dmode |= EXT2_IFDIR;

	if ((err = ext2fs_inode_alloc(dvp, dmode, cred, &tvp)) != 0)
		return err;

	/*
	 * We could not directly call ext2fs_makeinode() because we'd like
	 * to get the inode, and write the entry into parent directory ONLY
	 * AFTER initializing the created directory itself (by writing "."
	 * and ".." entries).
	 */

	/* Get inode */
	ip = VTOI(tvp);
	/* TODO: set up UID and GID */
	ip->uid = ip->gid = 0;
	ip->flags |= IN_ACCESS | IN_CHANGE | IN_UPDATE;
	EXT2_DINODE(ip)->mode = dmode;
	tvp->type = VDIR;
	EXT2_DINODE(ip)->nlink = 2;	/* "." and parent directory entry */
	if ((err = ext2fs_update(ip)) != 0)
		goto rollback_inode;

	/*
	 * Increase parent directory ref count because of ".." in created
	 * directory.
	 */
	EXT2_DINODE(dp)->nlink++;
	EXT2_DINODE(dp)->flags |= IN_CHANGE;
	if ((err = ext2fs_update(dp)) != 0)
		goto rollback_inode;

	memset(&dir, 0, sizeof(dir));
	/* first entry is always "." */
	dir.ino = ip->ino;
	dir.namelen = 1;
	dir.type = EXT2_FT_DIR;
	dir.reclen = EXT2FS_DIRSIZ(1);
	dir.name[0] = '.';
	e2fs_save_direct(&dir, dirtemplate);
	/* second entry is always ".." */
	dir.ino = dp->ino;
	dir.namelen = 2;
	dir.type = EXT2_FT_DIR;
	dir.reclen = fs->bsize - EXT2FS_DIRSIZ(1);
	dir.name[0] = dir.name[1] = '.';
	e2fs_save_direct(&dir, dirtemplate + EXT2FS_DIRSIZ(1));

	err = vn_write(tvp, 0, sizeof(dirtemplate), dirtemplate, 0,
	    UIO_KERNEL, p, NULL, cred, NULL);
	if (err)
		goto rollback_dp;

	err = ext2fs_setsize(ip, fs->bsize);
	if (err)
		goto rollback_dp;
	dp->flags |= IN_CHANGE;

	/* insert entry into parent directory */
	err = ext2fs_direnter(ip, dvp, name, cred);
	if (err)
		goto rollback_dp;

	*vpp = tvp;
	return 0;

rollback_dp:
	EXT2_DINODE(dp)->nlink--;
	EXT2_DINODE(dp)->flags |= IN_CHANGE;
rollback_inode:
	EXT2_DINODE(ip)->nlink = 0;
	EXT2_DINODE(ip)->flags |= IN_CHANGE;	/* trigger update */
	tvp->type = VNON;
	vput(tvp);
	return err;
}

