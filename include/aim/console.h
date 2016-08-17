/* Copyright (C) 2016 David Gao <davidgao1001@gmail.com>
 *
 * This file is part of AIMv6.
 *
 * AIMv6 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * AIMv6 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _CONSOLE_H
#define _CONSOLE_H

typedef int (*putchar_fp)(unsigned char c);
typedef int (*puts_fp)(const char *s);

int early_console_init(void);

/* set_console(putchar, puts)
 * Register two routines to use as kernel console
 * These function pointers need to be in high address space.
 * Registered routines should be compiled as PIC and will work in both
 * address spaces.
 */
void set_console(putchar_fp putchar, puts_fp puts);


/* kernel console output routines
 * these will work in both address spaces.
 */
int kprintf(const char *fmt, ...);
int kputchar(int c);
int kputs(const char *s);	/* Atomic */
int __default_kputs(const char *s);
#define DEFAULT_KPUTS __default_kputs

#ifdef DEBUG_OUTPUT
#define debug_kprintf(fmt, ...) \
	do { \
		kputs("DEBUG: "); \
		kprintf(fmt, ##__VA_ARGS__); \
	} while (0)
#else
#define debug_kprintf(fmt, ...)
#endif /* DEBUG_OUTPUT */

#endif /* _CONSOLE_H */

