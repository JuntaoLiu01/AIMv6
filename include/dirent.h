
#ifndef _DIRENT_H
#define _DIRENT_H

#include <libc/dirent.h>
#include <util.h>

#define DIRENT_RECSIZE(namelen) \
	ALIGN_NEXT(offsetof(struct dirent, d_name) + (namelen), 8)

#endif

