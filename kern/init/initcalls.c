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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <sys/types.h>
#include <aim/export.h>
#include <aim/initcalls.h>
#include <aim/console.h>
#include <panic.h>

int do_early_initcalls()
{
	extern initcall_t SYMBOL(early_init_start)[];
	extern initcall_t SYMBOL(early_init_end)[];
	/* get pointers, don't compare arrays. */
	initcall_t *start = &SYMBOL(early_init_start)[0];
	initcall_t *end = &SYMBOL(early_init_end)[0];
	initcall_t *this;
	bool failed = false;

	kpdebug("early initcalls 0x%08x to 0x%08x\n", start, end);
	for (this = start; this < end; this += 1) {
		int ret = (*this)();
		if (ret < 0) {
			/* don't panic */
			kprintf("OOPS: Initcall entry 0x%08x failed.\n",
				*this);
			failed = true;
		}
	}
	return (failed == true)? EOF : 0;
}

int do_initcalls()
{
	extern initcall_t SYMBOL(norm_init_start)[];
	extern initcall_t SYMBOL(norm_init_end)[];
	/* get pointers, don't compare arrays. */
	initcall_t *start = &SYMBOL(norm_init_start)[0];
	initcall_t *end = &SYMBOL(norm_init_end)[0];
	initcall_t *this;
	bool failed = false;

	kpdebug("initcalls 0x%08x to 0x%08x\n", start, end);
	for (this = start; this < end; this += 1) {
		kpdebug("executing 0x%p\n", *this);
		int ret = (*this)();
		if (ret < 0) {
			/* don't panic */
			kprintf("OOPS: Initcall entry 0x%08x failed.\n",
				*this);
			failed = true;
		}
	}
	return (failed == true)? EOF : 0;
}

