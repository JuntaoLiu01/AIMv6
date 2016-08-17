
#ifndef _PIPE_H
#define _PIPE_H

#include <sys/types.h>
#include <sys/param.h>
#include <aim/sync.h>

struct pipe {
	lock_t		lock;
	void		*data;		/* buffer data with size PIPE_SIZE */
	unsigned int	nread;		/* # bytes read */
	unsigned int	nwritten;	/* # bytes written */
	int		nreaders;	/* # pipe readers */
	int		nwriters;	/* # pipe writers */
};

struct pipe *PIPENEW(void);
void pipe_free(struct pipe *pipe);

#endif
