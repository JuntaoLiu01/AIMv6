
#include <sys/types.h>
#include <fs/namei.h>
#include <fs/vnode.h>
#include <proc.h>
#include <limits.h>
#include <errno.h>
#include <vmm.h>
#include <panic.h>
#include <libc/string.h>

/*
 * Translates a path into a vnode.
 * This will be a HUUUUUGE subroutine.
 *
 * Currently NAMEI_DELETE and NAMEI_LOOKUP are identical as there is no
 * additional job neede for deletion here.
 */
int
namei(struct nameidata *nd)
{
	char *path_lookup = nd->path;
	char *path, *segend, *segstart;
	struct vnode *rootdir;
	size_t pathlen = strnlen(path_lookup, PATH_MAX);
	int pathend, err;
	struct proc *p = nd->proc;

	if (nd->intent == NAMEI_RENAME) {
		kprintf("KERN: rename and delete namei NYI\n");
		return -ENOTSUP;
	}

	if (pathlen >= PATH_MAX)
		return -ENAMETOOLONG;
	else if (pathlen == 0)
		return -ENOENT;

	assert(p != NULL);
	rootdir = p->rootd;
	if (rootdir == NULL)
		rootdir = rootvnode;
	nd->parentvp = (path_lookup[0] == '/') ? rootdir : p->cwd;
	path = kmalloc(PATH_MAX, 0);
	nd->pathbuf = path;
	strlcpy(path, path_lookup, PATH_MAX);

	/*
	 * Here we are dealing with the simplest case: the file we are
	 * looking up, as well as all the directories we are tracing
	 * down, are all on the same file system.  On Unix-like systems,
	 * things are much more complicated.
	 */
	vget(nd->parentvp);
	nd->vp = rootdir;		/* If we found a '/', return the rootdir */
	err = 0;
	for (segstart = path; *segstart != '\0'; segstart = segend + 1) {
		/* chomp a segment */
		for (segend = segstart; *segend != '/' && *segend != '\0';
		    ++segend)
			/* nothing */;
		pathend = (*segend == '\0');
		*segend = '\0';
		if (segstart == segend)
			continue;

		/*
		 * Usually, the file system provider should deal with
		 * special segments "." and "..".  There are cases
		 * (e.g. ext2) where the on-disk directories store
		 * the file ID of parent directories.
		 *
		 * However, if we arrive at the root vnode of a process
		 * (due to chroot(2)), or the root vnode of the whole
		 * file system tree, we will have to do a NOP by our own.
		 *
		 * Also, there are cases where we are crossing file systems
		 * if current directory is the root of a file system mounted
		 * on a directory of another file system.  We do not support
		 * mounting multiple file systems right now.
		 */
		if (strcmp(segstart, "..") == 0) {
			if (nd->parentvp == rootdir || nd->parentvp == rootvnode)
				continue;
		}

		nd->seg = segstart;
		/*
		 * Look for that segment in the directory.
		 * There @vp will be set if an entry is found.
		 */
		err = VOP_LOOKUP(nd->parentvp, nd->seg, &nd->vp, nd->cred,
		    nd->proc);
		if (err)
			goto fail;

		/*
		 * If we encountered a symlink and we are not at the
		 * last segment, we should always read in the content
		 * of the symlink and replace the path string to
		 * continue lookup, regardless of the NAMEI_FOLLOW
		 * flag.
		 *
		 * If we found a symlink at the last segment, then
		 * we either return the symlink itself or the
		 * referenced file, depending on the NAMEI_FOLLOW
		 * flag.
		 */
		if (pathend) {
			if (!(nd->flags & NAMEI_PARENT)) {
				vput(nd->parentvp);
				nd->parentvp = NULL;
			}
			if (nd->vp == NULL) {
				if (nd->intent == NAMEI_CREATE)
					/* allow if creating file */
					goto finish;
				err = -ENOENT;
				goto fail;
			}
			if (nd->vp->type == VLNK && (nd->flags & NAMEI_FOLLOW)) {
				/* TODO */
				kprintf("KERN: symlink lookup NYI\n");
				err = -ENOTSUP;
				goto fail;
			}
			goto finish;
		} else {
			/* always put back intermediate parent nodes */
			vput(nd->parentvp);
			nd->parentvp = NULL;
			if (nd->vp == NULL) {
				err = -ENOENT;
				goto fail;
			}
			if (nd->vp->type == VLNK) {
				/* TODO */
				kprintf("KERN: symlink lookup NYI\n");
				err = -ENOTSUP;
				goto fail;
			}
			/* is it a directory?  for further lookups */
			if (nd->vp->type == VDIR) {
				/* Note that vp is already vget'd in VOP_LOOKUP;
				 * we don't need to vget() it again. */
				nd->parentvp = nd->vp;
			} else {
				/* trying to get directory entry inside a file,
				 * put the vnode we just vget'd back */
				assert(nd->vp->type != VNON);
				assert(nd->vp->type != VBAD);
				err = -EPERM;
				goto fail;
			}
		}
	}

fail:
	kfree(path);
	if (nd->parentvp) {
		vput(nd->parentvp);
		nd->parentvp = NULL;
	}
	if (nd->vp) {
		vput(nd->vp);
		nd->vp = NULL;
	}
finish:
	return err;
}

void
namei_cleanup(struct nameidata *nd)
{
	kfree(nd->pathbuf);
}

