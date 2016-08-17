/*
 * Copyright (c) 1997 Manuel Bouyer.
 * Copyright (c) 1982, 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
 *	@(#)dinode.h	8.6 (Berkeley) 9/13/94
 *  Modified for ext2fs by Manuel Bouyer.
 */

#ifndef _FS_UFS_EXT2FS_DINODE_H
#define _FS_UFS_EXT2FS_DINODE_H

#include <sys/types.h>
#include <fs/ufs/dinode.h>	/* for ufsino_t */
#include <fs/ufs/ext2fs/ext2fs.h>
#include <fs/vnode.h>		/* enum vtype */
#include <util.h>
#include <libc/dirent.h>

struct inode;	/* fs/ufs/inode.h */

/*
 * The root inode is the root of the file system.  Inode 0 can't be used for
 * normal purposes and bad blocks are normally linked to inode 1, thus
 * the root inode is 2.
 * Inode 3 to 10 are reserved in ext2fs.
 */
#define	EXT2_ROOTINO ((ufsino_t)2)
#define EXT2_RESIZEINO ((ufsino_t)7)
#define EXT2_FIRSTINO ((ufsino_t)11)

/*
 * A dinode contains all the meta-data associated with a UFS file.
 * This structure defines the on-disk format of a dinode. Since
 * this structure describes an on-disk structure, all its fields
 * are defined by types with precise widths.
 */

#define	NDADDR	12			/* Direct addresses in inode. */
#define	NIADDR	3			/* Indirect addresses in inode. */

#define EXT2_MAXSYMLINKLEN ((NDADDR+NIADDR) * sizeof (uint32_t))

struct ext2fs_dinode {
	uint16_t	mode;		/*   0: IFMT, permissions; see below. */
	uint16_t	uid_low;	/*   2: owner UID, bits 15:0 */
	uint32_t	size;		/*   4: file size (bytes) bits 31:0 */
	uint32_t	atime;		/*   8: Access time */
	uint32_t	ctime;		/*  12: Change time */
	uint32_t	mtime;		/*  16: Modification time */
	uint32_t	dtime;		/*  20: Deletion time */
	uint16_t	gid_low;	/*  24: Owner GID, lowest bits */
	uint16_t	nlink;		/*  26: File link count */
	uint32_t	nblock;		/*  28: blocks count */
	uint32_t	flags;		/*  32: status flags (chflags) */
	uint32_t	version_lo;	/* 36: inode version, bits 31:0 */
	/*
	 * The blocks fields may be overlaid with other information for
	 * file types that do not have associated disk storage. Block
	 * and character devices overlay the first data block with their
	 * dev_t value. Short symbolic links place their path in the
	 * di_db area.
	 */
	union {
		uint32_t blocks[NDADDR+NIADDR]; /* 40: disk blocks */
		uint32_t rdev;
		uint32_t shortlink[NDADDR+NIADDR];
	};
	uint32_t	gen;		/* 100: generation number */
	uint32_t	facl;		/* 104: file ACL, bits 31:0 */
	uint32_t	size_hi;	/* 108: file size (bytes), bits 63:32 */
	uint32_t	faddr;		/* 112: fragment address (obsolete) */
	uint16_t	nblock_hi;	/* 116: blocks count, bits 47:32 */
	uint16_t	facl_hi;	/* 118: file ACL, bits 47:32 */
	uint16_t	uid_high;	/* 120: owner UID, bits 31:16 */
	uint16_t	gid_high;	/* 122: owner GID, bits 31:16 */
	uint16_t	chksum_lo;	/* 124: inode checksum, bits 15:0 */
	uint16_t	_reserved;	/* 126: 	unused */
	uint16_t	isize;		/* 128: size of this inode */
	uint16_t	chksum_hi;	/* 130: inode checksum, bits 31:16 */
	uint32_t	x_ctime;	/* 132: extra Change time */
	uint32_t	x_mtime;	/* 136: extra Modification time */
	uint32_t	x_atime;	/* 140: extra Access time */
	uint32_t	crtime;		/* 144: Creation (birth) time */
	uint32_t	x_crtime;	/* 148: extra Creation (birth) time */
	uint32_t	version_hi;	/* 152: inode version, bits 63:31 */
};

#define EXT2_DINODE(ip)		((struct ext2fs_dinode *)((ip)->dinode))

/* refresh time stamps */
#define EXT2FS_ITIMES(ip) \
	do { \
		if ((ip)->flags & (IN_ACCESS | IN_CHANGE | IN_UPDATE)) { \
			(ip)->flags |= IN_MODIFIED; \
			/* TODO: change time stamps in inode */; \
			(ip)->flags &= ~(IN_ACCESS | IN_CHANGE | IN_UPDATE); \
		} \
	} while (0)

#define	E2MAXSYMLINKLEN	((NDADDR + NIADDR) * sizeof(uint32_t))

/* File permissions, see chmod(1) */
#define	EXT2_OEXEC		0000001		/* Executable. */
#define	EXT2_OWRITE		0000002		/* Writeable. */
#define	EXT2_OREAD		0000004		/* Readable. */
#define	EXT2_GEXEC		0000010		/* Executable. */
#define	EXT2_GWRITE		0000020		/* Writeable. */
#define	EXT2_GREAD		0000040		/* Readable. */
#define	EXT2_UEXEC		0000100		/* Executable. */
#define	EXT2_UWRITE		0000200		/* Writeable. */
#define	EXT2_UREAD		0000400		/* Readable. */
#define	EXT2_ISVTX		0001000		/* Sticky bit. */
#define	EXT2_ISGID		0002000		/* Set-gid. */
#define	EXT2_ISUID		0004000		/* Set-uid. */

/* File types. */
#define	EXT2_IFMT		0170000		/* Mask of file type. */
#define	EXT2_IFIFO		0010000		/* Named pipe (fifo). */
#define	EXT2_IFCHR		0020000		/* Character device. */
#define	EXT2_IFDIR		0040000		/* Directory file. */
#define	EXT2_IFBLK		0060000		/* Block device. */
#define	EXT2_IFREG		0100000		/* Regular file. */
#define	EXT2_IFLNK		0120000		/* Symbolic link. */
#define	EXT2_IFSOCK		0140000		/* UNIX domain socket. */

/* file flags */
#define EXT2_SECRM		0x00000001	/* Secure deletion */
#define EXT2_UNRM		0x00000002	/* Undelete */
#define EXT2_COMPR		0x00000004	/* Compress file */
#define EXT2_SYNC		0x00000008	/* Synchronous updates */
#define EXT2_IMMUTABLE		0x00000010	/* Immutable file */
#define EXT2_APPEND		0x00000020	/* writes to file may only append */
#define EXT2_NODUMP		0x00000040	/* do not dump file */
#define EXT2_NOATIME		0x00000080	/* do not update access time */
#define EXT4_INDEX		0x00001000	/* hash-indexed directory */
#define EXT4_JOURNAL_DATA	0x00004000	/* file data should be journaled */
#define EXT4_DIRSYNC		0x00010000	/* all dirent updates done synchronously */
#define EXT4_TOPDIR		0x00020000	/* top of directory hierarchies */
#define EXT4_HUGE_FILE		0x00040000	/* nblocks unit is fsb, not db */
#define EXT4_EXTENTS		0x00080000	/* inode uses extents */
#define EXT4_EOFBLOCKS		0x00400000	/* blocks allocated beyond EOF */

/* Size of on-disk inode. */
#define EXT2_REV0_DINODE_SIZE	128
#define EXT2_DINODE_SIZE(fs)	((fs)->e2fs.rev > E2FS_REV0 ?  \
				    (fs)->e2fs.inode_size : \
				    EXT2_REV0_DINODE_SIZE)

/* e2fs needs byte swapping on big-endian systems */
#ifdef LITTLE_ENDIAN
#define e2fs_iload(fs, old, new)	\
	memcpy((new),(old), min2(EXT2_DINODE_SIZE(fs), sizeof(struct ext2fs_dinode)))
#define e2fs_isave(fs, old, new) \
	memcpy((new),(old), min2(EXT2_DINODE_SIZE(fs), sizeof(struct ext2fs_dinode)))
#else
struct m_ext2fs;
void e2fs_i_bswap(struct m_ext2fs *, struct ext2fs_dinode *, struct ext2fs_dinode *);
#define e2fs_iload(fs, old, new) e2fs_i_bswap((fs), (old), (new))
#define e2fs_isave(fs, old, new) e2fs_i_bswap((fs), (old), (new))
#endif

static inline enum vtype EXT2_IFTOVT(uint16_t mode)
{
	static enum vtype tbl[] = {
		[DT_FIFO] = VBAD,	/* VFIFO */
		[DT_CHR] = VCHR,
		[DT_DIR] = VDIR,
		[DT_BLK] = VBLK,
		[DT_REG] = VREG,
		[DT_LNK] = VLNK,
		[DT_SOCK] = VBAD,	/* VSOCK */
	};
	return tbl[IFTODT(mode)];
}

static inline int EXT2_VTTOIF(enum vtype type)
{
	static int tbl[] = {
		[VCHR] = EXT2_IFCHR,
		[VBLK] = EXT2_IFBLK,
		[VDIR] = EXT2_IFDIR,
		[VREG] = EXT2_IFREG,
		[VLNK] = EXT2_IFLNK,
	};
	return tbl[type];
}

static inline int EXT2_MAKEIMODE(enum vtype type, int mode)
{
	return EXT2_VTTOIF(type) | mode;
}

#endif
