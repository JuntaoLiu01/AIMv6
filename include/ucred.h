
#ifndef _UCRED_H
#define _UCRED_H

struct ucred {
	int dummy;
};

#define NOCRED	((struct ucred *)-1)

#endif
