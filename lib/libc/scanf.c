
#include <stdio.h>
#include <stddef.h>
#include <unistd.h>

int getchar(void)
{
	unsigned char c;
	if (read(STDIN_FILENO, &c, 1) <= 0)
		return EOF;
	return c;
}

/*
 * gets() is extremely dangerous (see gets(3)).  However, it's easier for
 * us to implement since we can provide a solution without dealing with
 * FILE type.  So we just suck it up.
 */
char *gets(char *s)
{
	int c;

	for (c = getchar(); c != EOF && c != '\n'; c = getchar()) {
		switch (c) {
		case '\b':
			*(--s) = '\0';
			break;
		default:
			*(s++) = (char)c;
			break;
		}
	}
	*s = '\0';
	return s;
}

