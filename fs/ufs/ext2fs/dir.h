/*
 * Copyright (c) 1997 Manuel Bouyer.
 * Copyright (c) 1982, 1986, 1989, 1993
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
 *	@(#)dir.h	8.4 (Berkeley) 8/10/94
 * Modified for ext2fs by Manuel Bouyer.
 */

#ifndef _FS_UFS_EXT2FS_DIRECT_H
#define _FS_UFS_EXT2FS_DIRECT_H

#include <sys/types.h>
#include <sys/endian.h>
#include <util.h>
#include <libc/dirent.h>

/*
 * A directory consists of some number of blocks of e2fs_bsize bytes.
 *
 * Each block contains some number of directory entry
 * structures, which are of variable length.  Each directory entry has
 * a struct direct at the front of it, containing its inode number,
 * the length of the entry, and the length of the name contained in
 * the entry.  These are followed by the name padded to a 4 byte boundary
 * with null bytes.  All names are guaranteed null terminated.
 * The maximum length of a name in a directory is EXT2FS_MAXNAMLEN.
 *
 * The macro EXT2FS_DIRSIZ(fmt, dp) gives the amount of space required to
 * represent a directory entry.  Free space in a directory is represented by
 * entries which have dp->e2d_reclen > DIRSIZ(fmt, dp).  All d2fs_bsize bytes
 * in a directory block are claimed by the directory entries.  This
 * usually results in the last entry in a directory having a large
 * dp->e2d_reclen.  When entries are deleted from a directory, the
 * space is returned to the previous entry in the same directory
 * block by increasing its dp->e2d_reclen.  If the first entry of
 * a directory block is free, then its dp->e2d_ino is set to 0.
 * Entries other than the first in a directory do not normally have
 * dp->e2d_ino set to 0.
 * Ext2 rev 0 has a 16 bits e2d_namlen. For Ext2 rev 1 this has been split
 * into a 8 bits e2d_namlen and 8 bits e2d_type (looks like ffs, isnt't it ? :)
 * It's safe to use this for rev 0 as well because all ext2 are little-endian.
 */

#define	EXT2FS_MAXNAMLEN	255

struct ext2fs_dirhdr {
	uint32_t ino;
	uint16_t reclen;
	uint8_t	namelen;
	uint8_t	type;
	/* Ext2 directory file types (not the same as FFS. Sigh. */
#define EXT2_FT_UNKNOWN         0
#define EXT2_FT_REG_FILE        1
#define EXT2_FT_DIR             2
#define EXT2_FT_CHRDEV          3
#define EXT2_FT_BLKDEV          4
#define EXT2_FT_FIFO            5
#define EXT2_FT_SOCK            6
#define EXT2_FT_SYMLINK         7
#define EXT2_FT_MAX             8
};

#define E2IFTODT(mode)	(((mode) & 0170000) >> 12)

/* NOTE: this is different from IFTODT */
static inline uint8_t E2IF_TO_E2DT(uint16_t type)
{
	static uint8_t tbl[] = {
		[DT_FIFO] = EXT2_FT_FIFO,
		[DT_CHR] = EXT2_FT_CHRDEV,
		[DT_DIR] = EXT2_FT_DIR,
		[DT_BLK] = EXT2_FT_BLKDEV,
		[DT_REG] = EXT2_FT_REG_FILE,
		[DT_LNK] = EXT2_FT_SYMLINK,
		[DT_SOCK] = EXT2_FT_SOCK
	};
	return tbl[IFTODT(type)];
}

struct ext2fs_direct {
	struct ext2fs_dirhdr;
	char	name[EXT2FS_MAXNAMLEN];
};

/*
 * The EXT2FS_DIRSIZ macro gives the minimum record length which will hold
 * the directory entryfor a name len "len" (without the terminating null byte).
 * This requires the amount of space in struct direct
 * without the d_name field, plus enough space for the name without a
 * terminating null byte, rounded up to a 4 byte boundary.
 */
#define EXT2FS_DIRSIZ(len)	ALIGN_ABOVE(8 + (len), 4)

#ifdef LITTLE_ENDIAN
#define e2fs_load_dirhdr(old, new) \
	memcpy((new), (old), \
	    EXT2FS_DIRSIZ(((struct ext2fs_dirhdr *)(old))->namelen))
#define e2fs_save_dirhdr(old, new) \
	memcpy((new), (old), \
	    EXT2FS_DIRSIZ(((struct ext2fs_dirhdr *)(old))->namelen))
#else
#endif

#endif
