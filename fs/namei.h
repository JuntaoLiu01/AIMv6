
#ifndef _FS_NAMEI_H
#define _FS_NAMEI_H

#include <sys/types.h>

struct vnode;	/* fs/vnode.h */
struct proc;	/* include/proc.h */

struct nameidata {
	/* input fields */
	char		*path;
	int		intent;
#define NAMEI_LOOKUP	0
#define NAMEI_CREATE	1
#define NAMEI_DELETE	2
#define NAMEI_RENAME	3
	uint32_t	flags;
#define NAMEI_FOLLOW	0x1	/* follow symlinks */
#define NAMEI_PARENT	0x100	/* preserve parent */
#define NAMEI_STRIP	0x200	/* strip trailing slashes */
	struct ucred	*cred;
	struct proc	*proc;

	/* output fields */
	struct vnode	*vp;
	struct vnode	*parentvp;

	/* output fields which will be cleaned up by namei_cleanup() */
	char		*seg;	/* current segment name we're looking */

	/* internal fields */
	char		*pathbuf;	/* internal path buffer */
};

int namei(struct nameidata *);
void namei_cleanup(struct nameidata *);	/* each namei() should end with this */

#endif
