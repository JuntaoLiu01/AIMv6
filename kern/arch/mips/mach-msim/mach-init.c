/* Copyright (C) 2016 Gan Quan <coin2028@hotmail.com>
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
#endif

#include <sys/types.h>
#include <mach-conf.h>
#include <aim/device.h>

dev_t rootdev;

void mach_init(void)
{
	rootdev = makedev(MSIM_DISK_MAJOR, ROOT_PARTITION_ID);
}

struct devtree_entry devtree[] = {
	/* memory bus */
	{
		"memory",
		"memory",
		"",
		0,
		{0},
		0,
	},
	/* MSIM disk */
	{
		"msim-disk",
		"msim-disk",
		"memory",
		1,
		{MSIM_DISK_PHYSADDR},
		2,
	},
	/* MSIM line printer */
	{
		"msim-lpr",
		"msim-lpr",
		"memory",
		1,
		{MSIM_LP_PHYSADDR},
		0,
	},
	/* MSIM keyboard */
	{
		"msim-kbd",
		"msim-kbd",
		"memory",
		1,
		{MSIM_KBD_PHYSADDR},
		3,
	},
};

int ndevtree_entries = ARRAY_SIZE(devtree);
