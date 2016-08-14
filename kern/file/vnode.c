
#include <sys/types.h>
#include <file.h>
#include <fs/vnode.h>
#include <fs/vfs.h>
#include <fs/uio.h>
#include <percpu.h>
#include <ucred.h>
#include <errno.h>
#include <panic.h>

int
vn_file_read(struct file *file, void *buf, size_t len, loff_t *off)
{
	size_t done;
	int err;
	assert(file->type == FVNODE);
	if (file->vnode->type == VDIR)
		return -EISDIR;

	FLOCK(file);
	vlock(file->vnode);
	err = vn_read(file->vnode, *off, len, buf, file->ioflags, UIO_USER,
	    current_proc, NULL, file->cred, &done);
	if (err) {
		vunlock(file->vnode);
		/* TODO REPLACE */
		VFS_SYNC(file->vnode->mount, NOCRED, current_proc);
		FUNLOCK(file);
		return err;
	}
	*off += done;
	vunlock(file->vnode);
	/* TODO REPLACE */
	VFS_SYNC(file->vnode->mount, NOCRED, current_proc);
	FUNLOCK(file);

	return 0;
}

int
vn_file_write(struct file *file, void *buf, size_t len, loff_t *off)
{
	size_t done;
	int err;
	assert(file->type == FVNODE);
	if (file->vnode->type == VDIR)
		return -EISDIR;

	FLOCK(file);
	vlock(file->vnode);
	err = vn_write(file->vnode, *off, len, buf, file->ioflags, UIO_USER,
	    current_proc, NULL, file->cred, &done);
	if (err) {
		vunlock(file->vnode);
		/* TODO REPLACE */
		VFS_SYNC(file->vnode->mount, NOCRED, current_proc);
		FUNLOCK(file);
		return err;
	}
	*off += done;
	vunlock(file->vnode);
	/* TODO REPLACE */
	VFS_SYNC(file->vnode->mount, NOCRED, current_proc);
	FUNLOCK(file);

	return 0;
}

int
vn_file_close(struct file *file, struct proc *p)
{
	struct vnode *vnode = file->vnode;
	int err;

	assert(file->type == FVNODE);
	vlock(vnode);
	err = VOP_CLOSE(vnode, file->openflags, file->cred, p);
	if (err) {
		vunlock(vnode);
		return err;
	}

	/* TODO REPLACE */
	VFS_SYNC(vnode->mount, NOCRED, p);
	vput(vnode);
	return 0;
}

void
vn_file_ref(struct file *file)
{
	assert(file->type == FVNODE);
	vref(file->vnode);
}

struct file_ops vnops = {
	.read = vn_file_read,
	.write = vn_file_write,
	.close = vn_file_close,
	.ref = vn_file_ref,
};

