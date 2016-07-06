
#include <fs/vfs.h>
#include <fs/mount.h>
#include <fs/vnode.h>
#include <libc/string.h>
#include <errno.h>

static struct vfsconf vfsconfs[MAX_VFSCONFS];
static int vfsconfnum = 0;

void registerfs(const char *name, struct vfsops *ops)
{
	vfsconfs[vfsconfnum].name = name;
	vfsconfs[vfsconfnum].ops = ops;
	++vfsconfnum;
}

struct vfsconf *findvfsconf(const char *name)
{
	for (int i = 0; i < vfsconfnum; ++i)
		if (strcmp(name, vfsconfs[i].name) == 0)
			return &(vfsconfs[i]);
	return NULL;
}

int
VFS_ROOT(struct mount *mp, struct vnode **vpp)
{
	if (mp->ops->root == NULL)
		return -ENOTSUP;
	return mp->ops->root(mp, vpp);
}

int
VFS_VGET(struct mount *mp, ino_t ino, struct vnode **vpp)
{
	if (mp->ops->vget == NULL)
		return -ENOTSUP;
	return mp->ops->vget(mp, ino, vpp);
}

