/*
 * Copyright (c) 1997 Manuel Bouyer.
 * Copyright (c) 1982, 1986, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)fs.h	8.10 (Berkeley) 10/27/94
 *  Modified for ext2fs by Manuel Bouyer.
 */

#ifndef _FS_UFS_EXT2FS_EXT2FS_H
#define _FS_UFS_EXT2FS_EXT2FS_H

#include <sys/types.h>
#include <sys/endian.h>
#include <sys/param.h>
#include <fs/ufs/ufs.h>

/*
 * Each disk drive contains some number of file systems.
 * A file system consists of a number of cylinder groups.
 * Each cylinder group has inodes and data.
 *
 * A file system is described by its super-block, which in turn
 * describes the cylinder groups.  The super-block is critical
 * data and is replicated in each cylinder group to protect against
 * catastrophic loss.  This is done at `newfs' time and the critical
 * super-block data does not change, so the copies need not be
 * referenced further unless disaster strikes.
 *
 * The first boot and super blocks are given in absolute disk addresses.
 * The byte-offset forms are preferred, as they don't imply a sector size.
 */
#define BBSIZE		1024
#define SBSIZE		1024
#define	BBOFF		((off_t)(0))
#define	SBOFF		((off_t)(BBOFF + BBSIZE))
#define	BBLOCK		((off_t)(0))
#define	SBLOCK		((off_t)(BBLOCK + BBSIZE / SECTOR_SIZE))

/*
 * Inodes are, like in UFS, 32-bit unsigned integers and therefore ufsino_t.
 * Disk blocks are 32-bit, if the filesystem isn't operating in 64-bit mode
 * (the incompatible ext4 64BIT flag).  More work is needed to properly use
 * daddr_t as the disk block data type on both BE and LE architectures.
 * XXX disk blocks are simply u_int32_t for now.
 */

/*
 * MINBSIZE is the smallest allowable block size.
 * MINBSIZE must be big enough to hold a cylinder group block,
 * thus changes to (struct cg) must keep its size within MINBSIZE.
 * Note that super blocks are always of size SBSIZE,
 * and that both SBSIZE and MAXBSIZE must be >= MINBSIZE.
 * FSIZE means fragment size.
 */
#define LOG_MINBSIZE	10
#define MINBSIZE	(1 << LOG_MINBSIZE)
#define LOG_MINFSIZE	10
#define MINFSIZE	(1 << LOG_MINFSIZE)

/*
 * The path name on which the file system is mounted is maintained
 * in fs_fsmnt. MAXMNTLEN defines the amount of space allocated in
 * the super block for this name.
 */
#define MAXMNTLEN	512

/*
 * Super block for an ext2fs file system.
 * NOTE: not all of them are used in AIMv6.
 */
struct ext2fs {
	uint32_t  icount;		/* Inode count */
	uint32_t  bcount;		/* blocks count */
	uint32_t  rbcount;		/* reserved blocks count */
	uint32_t  fbcount;		/* free blocks count */
	uint32_t  ficount;		/* free inodes count */
	uint32_t  first_dblock;		/* first data block */
	uint32_t  log_bsize;		/* block size = 1024*(2^log_bsize) */
	uint32_t  log_fsize;		/* fragment size log2 */
	uint32_t  bpg;			/* blocks per group */
	uint32_t  fpg;			/* frags per group */
	uint32_t  ipg;			/* inodes per group */
	uint32_t  mtime;		/* mount time */
	uint32_t  wtime;		/* write time */
	uint16_t  mnt_count;		/* mount count */
	uint16_t  max_mnt_count;	/* max mount count */
	uint16_t  magic;		/* magic number */
#define	E2FS_MAGIC	0xef53		/* the ext2fs magic number */
	uint16_t  state;		/* file system state */
#define	E2FS_ISCLEAN	0x01
#define	E2FS_ERRORS	0x02
	uint16_t  beh;			/* behavior on errors */
#define E2FS_BEH_CONTINUE	1	/* continue operation */
#define E2FS_BEH_READONLY	2	/* remount fs read only */
#define E2FS_BEH_PANIC		3	/* cause panic */
#define E2FS_BEH_DEFAULT	E2FS_BEH_CONTINUE
	uint16_t  minrev;		/* minor revision level */
	uint32_t  lastfsck;		/* time of last fsck */
	uint32_t  fsckintv;		/* max time between fscks */
	uint32_t  creator;		/* creator OS */
#define E2FS_OS_LINUX 0
#define E2FS_OS_HURD  1
#define E2FS_OS_MASIX 2
	uint32_t  rev;			/* revision level */
#define E2FS_REV0	0		/* revision levels */
#define E2FS_REV1	1		/* revision levels */
	uint16_t  ruid;			/* default uid for reserved blocks */
	uint16_t  rgid;			/* default gid for reserved blocks */
	/* EXT2_DYNAMIC_REV superblocks.  Unused in AIMv6 */
	uint32_t  first_ino;		/* first non-reserved inode */
	uint16_t  inode_size;		/* size of inode structure */
	uint16_t  block_group_nr;	/* block grp number of this sblk*/
	uint32_t  features_compat; 	/*  compatible feature set */
	uint32_t  features_incompat; 	/* incompatible feature set */
	uint32_t  features_rocompat; 	/* RO-compatible feature set */
	uint8_t   uuid[16];		/* 128-bit uuid for volume */
	char      vname[16];		/* volume name */
	char      fsmnt[64]; 		/* name mounted on */
	uint32_t  algo;			/* For compression */
	uint8_t   prealloc;		/* # of blocks to preallocate */
	uint8_t   dir_prealloc;		/* # of blocks to preallocate for dir */
	uint16_t  reserved_ngdb;	/* # of reserved gd blocks for resize */
	/* Ext3 JBD2 journaling.  Unused in AIMv6  */
	uint8_t   journal_uuid[16];
	uint32_t  journal_ino;
	uint32_t  journal_dev;
	uint32_t  last_orphan;		/* start of list of inodes to delete */
	uint32_t  hash_seed[4];		/* htree hash seed */
	uint8_t   def_hash_version;
	uint8_t   journal_backup_type;
	uint16_t  gdesc_size;
	uint32_t  default_mount_opts;
	uint32_t  first_meta_bg;
	uint32_t  mkfs_time;
	uint32_t  journal_backup[17];
	uint32_t  reserved2[76];
};

/* in-memory data for ext2fs */
struct m_ext2fs {
	struct ext2fs e2fs;
	uint8_t	fsmnt[MAXMNTLEN];	/* name mounted on */
	int8_t	ronly;		/* mounted read-only flag */
	int8_t	fmod;		/* super block modified flag */
	int32_t fsize;		/* fragment size */
	int32_t	bsize;		/* block size */
	int32_t bsects;		/* # of sectors per logical block */
	int32_t bshift;		/* ``lblkno'' calc of logical blkno */
	int32_t bmask;		/* ``blkoff'' calc of blk offsets */
	int64_t qbmask;		/* ~fs_bmask - for use with quad size */
	int32_t	fsbtodb;	/* fsbtodb and dbtofsb shift constant */
	int32_t	ncg;		/* number of cylinder groups */
	int32_t	ngdb;		/* number of group descriptor block */
	int32_t	ipb;		/* number of inodes per block */
	int32_t	itpg;		/* number of inode table per group */
	off_t	maxfilesize;	/* depends on LARGE/HUGE flags */
	struct	ext2_gd *gd;	/* group descriptor array */
};

struct ext2_gd {
	uint32_t b_bitmap;	/* blocks bitmap block */
	uint32_t i_bitmap;	/* inodes bitmap block */
	uint32_t i_tables;	/* inodes table block  */
	uint16_t nbfree;	/* number of free blocks */
	uint16_t nifree;	/* number of free inodes */
	uint16_t ndirs;		/* number of directories */
	uint16_t reserved;
	uint32_t reserved2[3];
};

/*
 * fsbtodb, dbtofsb:
 * Convert between file system block number to disk block number.
 */
#define fsbtodb(fs, b)		((b) << (fs)->fsbtodb)
#define dbtofsb(fs, b)		((b) >> (fs)->fsbtodb)

/* inode # to cylinder group # */
#define ino_to_cg(fs, ino)	(((ino) - 1) / (fs)->e2fs.ipg)
/* inode # to file system block # */
#define ino_to_fsba(fs, ino) \
	((fs)->gd[ino_to_cg(fs, ino)].i_tables + \
	 (((ino) - 1) % (fs)->e2fs.ipg) / (fs)->ipb)
/* inode # to file system block offset in that file system block */
#define ino_to_fsbo(fs, ino)	(((ino) - 1) % (fs)->ipb)

/* Computes bytes offset in a logical block (loc % fs->bsize) */
#define lblkoff(fs, loc)	((loc) & (fs)->qbmask)
/* Logical block # to byte offset on the partition */
#define lblktosize(fs, lblkno)	((lblkno) << (fs)->bshift)
/* Computes logical block # given a byte offset */
#define lblkno(fs, loc)		((loc) >> (fs)->bshift)
/* Round up the size to logical blocks */
#define lblkroundup(fs, size)	ALIGN_ABOVE((size), (fs)->bsize)

/* file system block # to cylinder group # */
#define dtog(fs, fsblk) \
	(((fsblk) - (fs)->e2fs.first_dblock) / (fs)->e2fs.fpg)
/* file system block # to cylinder group offset */
#define dtogd(fs, fsblk) \
	(((fsblk) - (fs)->e2fs.first_dblock) % (fs)->e2fs.fpg)

/*
 * Ext2 metadata is stored in little-endian byte order.
 * JBD2 journal used in ext3 and ext4 is big-endian!
 */
#ifdef LITTLE_ENDIAN
#define e2fs_sbload(old, new) memcpy((new), (old), SBSIZE);
#define e2fs_cgload(old, new, size) memcpy((new), (old), (size));
#define e2fs_sbsave(old, new) memcpy((new), (old), SBSIZE);
#define e2fs_cgsave(old, new, size) memcpy((new), (old), (size));
#else
void e2fs_sb_bswap(struct ext2fs *, struct ext2fs *);
void e2fs_cg_bswap(struct ext2_gd *, struct ext2_gd *, int);
#define e2fs_sbload(old, new) e2fs_sb_bswap((old), (new))
#define e2fs_cgload(old, new, size) e2fs_cg_bswap((old), (new), (size));
#define e2fs_sbsave(old, new) e2fs_sb_bswap((old), (new))
#define e2fs_cgsave(old, new, size) e2fs_cg_bswap((old), (new), (size));
#endif

#define NINDIR(fs)	((fs)->bsize / sizeof(uint32_t))

struct proc;	/* include/proc.h */
struct mount;	/* fs/mount.h */
struct vnode;	/* fs/vnode.h */
struct vattr;	/* fs/vnode.h */
struct vfsops;	/* fs/vfs.h */
struct vops;	/* fs/vnode.h */
struct uio;	/* fs/uio.h */
struct ucred;	/* include/ucred.h */
struct inode;	/* fs/ufs/inode.h */

int ext2fs_mountroot(void);

extern struct vfsops ext2fs_vfsops;
extern struct vops ext2fs_vops;
extern struct vops ext2fs_specvops;

/* VFS operations */
int ext2fs_vget(struct mount *mp, ino_t ino, struct vnode **vpp);
int ext2fs_sync(struct mount *mp, struct ucred *cred, struct proc *p);

/* vnode operations */
int ext2fs_read(struct vnode *, struct uio *, int, struct ucred *);
int ext2fs_write(struct vnode *, struct uio *, int, struct ucred *);
int ext2fs_inactive(struct vnode *vp, struct proc *p);
int ext2fs_reclaim(struct vnode *vp);
int ext2fs_bmap(struct vnode *, off_t, struct vnode **, soff_t *, int *);
int ext2fs_lookup(struct vnode *, char *, struct vnode **, struct ucred *,
    struct proc *);
int ext2fs_create(struct vnode *, char *, struct vattr *, struct vnode **,
    struct ucred *, struct proc *);
int ext2fs_link(struct vnode *, char *, struct vnode *, struct ucred *,
    struct proc *);

/* internal operations */
uint64_t ext2fs_getsize(struct inode *);
#define ext2fs_ndatablk(ip) \
	DIV_ROUND_UP(ext2fs_getsize(ip), \
		    ((struct m_ext2fs *)(ip)->superblock)->bsize)

int ext2fs_setsize(struct inode *ip, uint64_t size);
int ext2fs_inode_alloc(struct vnode *, int, struct vnode **);
void ext2fs_inode_free(struct inode *, ufsino_t, int);
int ext2fs_blkalloc(struct inode *, struct ucred *, off_t *);
void ext2fs_blkfree(struct inode *, off_t);
int ext2fs_update(struct inode *);
int ext2fs_indirs(struct inode *, off_t, int *);
int ext2fs_buf_alloc(struct inode *, off_t, struct ucred *, struct buf **);
int ext2fs_lblkfree(struct inode *, off_t, struct ucred *);
int ext2fs_truncate(struct inode *, size_t, struct ucred *);
int ext2fs_direnter(struct inode *, struct vnode *, char *, struct ucred *);

#endif

