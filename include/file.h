/* Copyright (C) 2016 David Gao <davidgao1001@gmail.com>
 *
 * This file is part of AIMv6.
 *
 * AIMv6 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * AIMv6 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _FILE_H
#define _FILE_H

#include <sys/types.h>
#include <fs/vnode.h>
#include <aim/sync.h>
#include <atomic.h>
#include <pipe.h>

enum ftype {
	FNON,
	FVNODE,
	FSOCKET,
	FPIPE,
};

struct file;
struct proc;
struct ucred;
struct uio;

/*
 * A higher level of operations including those to vnodes and pipes.
 */
struct file_ops {
	int (*close)(struct file *, struct proc *);
	int (*read)(struct file *, void *, size_t, loff_t *);
	int (*write)(struct file *, void *, size_t, loff_t *);
};

/*
 * This is the "file" structure used by processes, and serves as the
 * highest-level object where syscalls like read(2) and write(2) takes place.
 */
struct file {
	enum ftype	type;
	union {
		struct vnode *vnode;
		struct pipe *pipe;
#if 0
		struct socket *socket;
#endif
	};
	struct file_ops	*ops;
	loff_t		offset;
	int		ioflags;
	int		openflags;
	atomic_t	refs;
	lock_t		lock;
	struct ucred	*cred;
};

#define FILE_CLOSE(f, p) \
	(((f)->ops->close)((f), (p)))
#define FILE_READ(f, off, uio, cred) \
	(((f)->ops->read)((f), (off), (uio), (cred)))
#define FILE_WRITE(f, off, uio, cred) \
	(((f)->ops->write)((f), (off), (uio), (cred)))

#define FNEW() \
	({ \
		struct file *_f; \
		_f = kmalloc(sizeof(*_f), GFP_ZERO); \
		if (_f) { \
			_f->refs = 1; \
			_f->cred = NOCRED; \
			spinlock_init(&_f->lock); \
		} \
	 	_f; \
	})
#define FREF(f) \
	do { \
		atomic_inc(&(f)->refs); \
	} while (0)
#define FRELE(fp) \
	do { \
		atomic_dec(&(*(fp))->refs); \
		if ((*(fp))->refs == 0) { \
			kfree(*(fp)); \
			*(fp) = NULL; \
		} \
	} while (0)
#define FLOCK(f)	spin_lock(&(f)->lock)
#define FUNLOCK(f)	spin_unlock(&(f)->lock)

extern struct file_ops vnops, pipeops;
#define FINIT_VNODE(fd, vn, off, iof, of) \
	do { \
		(fd)->type = FVNODE; \
		(fd)->vnode = (vn); \
		(fd)->offset = (off); \
		(fd)->ioflags = (iof); \
		(fd)->openflags = (of); \
		(fd)->ops = &vnops; \
	} while (0)
#define FINIT_PIPE(fd, p, of) \
	do { \
		(fd)->type = FPIPE; \
		(fd)->pipe = (p); \
		(fd)->offset = 0; \
		(fd)->ioflags = 0; \
		(fd)->openflags = (of); \
		(fd)->ops = &pipeops; \
	} while (0)

#if 0

/*
 * [OBSOLETE]:
 * The actual file and file system operations are inside fs/ directory.
 */

/* All devices are files, all files accept file_ops */
struct file_ops {
	loff_t (*llseek)(struct file *, loff_t, int);
	ssize_t (*read)(struct file *, char *, size_t, loff_t *);
	ssize_t (*write)(struct file *, const char *, size_t, loff_t *);
	//int (*readdir)(struct file *, void *, filldir_t);
	//unsigned int (*poll)(struct file *, struct poll_table_struct *);
	int (*ioctl)(struct file *, unsigned int, unsigned int, unsigned long);
	//int (*mmap)(struct file *, struct vm_area_struct *);
	int (*open)(struct inode *, struct file *);
	//int (*flush)(struct file *);
	int (*release)(struct inode *, struct file *);
	//int (*fsync)(struct file *, struct dentry *, int datasync);
	//int (*fasync)(int, struct file *, int);
	//int (*lock)(struct file *, int, struct file_lock *);
	//ssize_t (*readv)(struct file *, const struct iovec *, unsigned long,
	//	loff_t *);
	//ssize_t (*writev)(struct file *, const struct iovec *, unsigned long,
	//	loff_t *);
};

/* Only block files accept block_ops */
struct blk_ops {
	ssize_t (*readblk)(struct file *, char *, size_t, loff_t *);
	ssize_t (*writeblk)(struct file *, const char *, size_t, loff_t *);
};

#endif

#endif /* _FILE_H */

