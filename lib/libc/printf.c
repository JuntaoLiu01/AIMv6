
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>

int puts(const char *s)
{
	char buf[BUFSIZ];
	int result;
	strlcpy(buf, s, BUFSIZ);
	strlcat(buf, "\n", BUFSIZ);

	result = write(STDOUT_FILENO, buf, strlen(buf));
	return result;
}

int printf(const char *fmt, ...)
{
	int result;
	va_list ap;

	va_start(ap, fmt);
	result = vprintf(fmt, ap);
	va_end(ap);

	return result;
}

int vprintf(const char *fmt, va_list ap)
{
	/*
	 * A temporary unbuffered implementation to write to stdout.
	 * In (far) future this should be replaced by vfprintf which operates
	 * on stdio FILE.
	 */
	char buf[BUFSIZ];
	int result, len;

	len = vsnprintf(buf, BUFSIZ, fmt, ap);
	result = write(STDOUT_FILENO, buf, len);

	return result;
}
