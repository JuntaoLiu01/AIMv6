
#include <sys/types.h>
#include <fs/vnode.h>
#include <fs/ufs/inode.h>
#include <fs/ufs/ufs.h>
#include <list.h>
#include <aim/initcalls.h>
#include <aim/sync.h>
#include <errno.h>

#define IHASHTBL_BITS	8
#define IHASHTBL_SIZE	(1 << IHASHTBL_BITS)

/*
 * Although the name is hash table, I'm not going to use a real hash table.
 * The underlying data structure is a mere list of inode's.
 *
 * Since the developers could simply use interfaces like ufs_ihashget() to
 * query the corresponding vnode, changing the underlying implementation
 * later does not affect code outside.
 */
struct list_head ufs_ihashtbl;
lock_t ufs_ihashlock;

int
ufs_ihashinit(void)
{
	list_init(&ufs_ihashtbl);
	spinlock_init(&ufs_ihashlock);
	return 0;
}
INITCALL_FS(ufs_ihashinit);

/*
 * Use @devno/@ino to query a vnode from hash table.  If found, increment
 * the reference count, and lock it.
 */
struct vnode *
ufs_ihashget(dev_t devno, ino_t ino)
{
	struct inode *ip;
	struct vnode *vp;

	spin_lock(&ufs_ihashlock);

	for_each_entry (ip, &ufs_ihashtbl, node) {
		if (ip->devno == devno && ip->ino == ino) {
			vp = ITOV(ip);
			vget(vp);
			spin_unlock(&ufs_ihashlock);
			return vp;
		}
	}

	spin_unlock(&ufs_ihashlock);
	return NULL;
}

int
ufs_ihashins(struct inode *ip)
{
	struct inode *curip;
	spin_lock(&ufs_ihashlock);
	spin_lock(&ip->lock);

	for_each_entry (curip, &ufs_ihashtbl, node) {
		if (ip->ino == curip->ino && ip->devno == curip->devno) {
			spin_unlock(&ip->lock);
			spin_unlock(&ufs_ihashlock);
			return -EEXIST;
		}
	}

	list_add_tail(&ip->node, &ufs_ihashtbl);
	spin_unlock(&ip->lock);
	spin_unlock(&ufs_ihashlock);
	return 0;
}

int
ufs_ihashrem(struct inode *ip)
{
	spin_lock(&ufs_ihashlock);
	spin_lock(&ip->lock);
	list_del(&(ip->node));
	spin_unlock(&ip->lock);
	spin_unlock(&ufs_ihashlock);
	return 0;
}
