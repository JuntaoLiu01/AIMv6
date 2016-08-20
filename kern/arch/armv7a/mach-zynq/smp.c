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

#include <aim/console.h>
#include <aim/kmmap.h>
#include <mm.h>

static void *release_addr;

void mach_smp_startup(void)
{
	extern uint32_t slave_entry;
	void *entry;
	release_addr = kmmap(NULL, 0xFFFFF000, PAGE_SIZE, MAP_SHARED_DEV);
	assert(release_addr != NULL);
	release_addr += 0xFF0;
	entry = (void *)&slave_entry;
	kpdebug("bring up using addr=0x%08x, entry=0x%08x\n", release_addr, entry);
	*(void **)release_addr = entry;
}

