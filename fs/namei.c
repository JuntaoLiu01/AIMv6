
#include <sys/types.h>
#include <fs/namei.h>
#include <fs/vnode.h>
#include <proc.h>
#include <limits.h>
#include <errno.h>
#include <vmm.h>
#include <panic.h>
#include <libc/string.h>

int
namei_putparent(struct nameidata *nd)
{
	if (nd->parentvp == nd->vp)
		vrele(nd->parentvp);
	else
		vput(nd->parentvp);
	return 0;
}

bool
namei_trim_slash(char *path)
{
	size_t len = strlen(path);
	char *p;
	bool result = false;

	for (p = path + len - 1; p != path && *p == '/'; --p) {
		result = true;
		*p = '\0';
	}
	return result;
}

/*
 * Translates a path into a vnode.
 *
 * Currently NAMEI_DELETE and NAMEI_LOOKUP are identical as there is no
 * additional job neede for deletion here.
 *
 * Assumes that there are no trailing slashes (except for root directory
 * "/").  If the path name contains trailing slashes, the caller must
 * strip them.
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
	bool firstseg = true;

	if (nd->intent == NAMEI_RENAME) {
		kprintf("KERN: rename and delete namei NYI\n");
		return -ENOTSUP;
	}

	if (pathlen >= PATH_MAX)
		return -ENAMETOOLONG;
	else if (pathlen == 0)
		return -ENOENT;

	kpdebug("namei %s\n", nd->path);

	assert(p != NULL);
	rootdir = p->rootd;
	if (rootdir == NULL)
		rootdir = rootvnode;
	nd->parentvp = (path_lookup[0] == '/') ? rootdir : p->cwd;
	path = kmalloc(PATH_MAX, 0);
	nd->pathbuf = path;
	strlcpy(path, path_lookup, PATH_MAX);
	assert(path[pathlen - 1] != '/' || pathlen == 1);

	/*
	 * Here we are dealing with the simplest case: the file we are
	 * looking up, as well as all the directories we are tracing
	 * down, are all on the same file system.  On Unix-like systems,
	 * things are much more complicated.
	 */
	vget(nd->parentvp);
	nd->vp = rootdir;	/* If we found a '/', return the rootdir */
	err = 0;
	if (strcmp(path, "/") == 0)
		return 0;

	for (segstart = path; *segstart != '\0'; segstart = segend + 1) {
		/* chomp a segment */
		for (segend = segstart; *segend != '/' && *segend != '\0';
		    ++segend)
			/* nothing */;
		pathend = (*segend == '\0');
		*segend = '\0';
		if (segstart == segend)
			continue;

		if (!firstseg) {
			namei_putparent(nd);
			nd->parentvp = nd->vp;
		}

		nd->seg = segstart;

		if (strcmp(nd->seg, "..") == 0 &&
		    (nd->parentvp == rootdir || nd->parentvp == rootvnode)) {
			nd->vp = nd->parentvp;
			vref(nd->vp);
			continue;
		}

		err = VOP_LOOKUP(nd->parentvp, nd->seg, &nd->vp, nd->cred,
		    nd->proc);
		if (err)
			goto fail;

		if (pathend) {
			goto last;
		} else {
			/*
			 * We require that the current segment is a directory.
			 * Since there are no trailing slashes, the directory
			 * must exist.
			 */
			if (nd->vp == NULL) {
				err = -ENOENT;
				goto fail;
			}
			if (nd->vp->type == VLNK) {
				kprintf("namei: symlink NYI\n");
				err = -ENOTSUP;
				goto fail;
			} else if (nd->vp->type != VDIR) {
				err = -EPERM;
				goto fail;
			}
			firstseg = false;
		}
	}
	/* NOTREACHED */
	panic("namei: no ending segment?\n");
last:
	if (!(nd->flags & NAMEI_PARENT)) {
		namei_putparent(nd);
		nd->parentvp = NULL;
	}
	if (nd->vp == NULL) {
		if (nd->intent == NAMEI_CREATE) {
			return 0;
		} else {
			err = -ENOENT;
			goto fail;
		}
	}
	if (nd->vp->type == VLNK && (nd->flags & NAMEI_FOLLOW)) {
		kprintf("namei: symlink NYI\n");
		err = -ENOTSUP;
		goto fail;
	}
	return 0;

fail:
	kfree(path);
	if (nd->parentvp) {
		namei_putparent(nd);
		nd->parentvp = NULL;
	}
	if (nd->vp) {
		vput(nd->vp);
		nd->vp = NULL;
	}
	return err;
}

void
namei_cleanup(struct nameidata *nd)
{
	kfree(nd->pathbuf);
}

