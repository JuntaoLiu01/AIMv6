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

#ifndef _DRIVERS_BLOCK_HD_H
#define _DRIVERS_BLOCK_HD_H

#include <sys/types.h>
#include <sys/param.h>
#include <aim/device.h>

#define HD_PART_BITS		4
#define HD_PART_MASK		((1 << HD_PART_BITS) - 1)
#define HD_INSTANCE_BITS	4
#define MAX_PARTITIONS		((1 << HD_PART_BITS) - 1)

#define hdbasedev(dev)	makedev(major(dev), minor(dev) & ~HD_PART_MASK)
#define hdpartno(dev)	(minor(dev) & HD_PART_MASK)
#define make_hdbasedev(maj, inst) \
	makedev(maj, (inst) << HD_INSTANCE_BITS)

struct hdpartition {
	off_t	offset;
	size_t	len;
};

struct hd_device {
	struct blk_device;

	/* Partitions are 1-based */
	struct hdpartition part[MAX_PARTITIONS + 1];

	/* Only valid for whole hd's */
	uint32_t	openmask;
	uint32_t	flags;
#define HD_LOADED	0x1
};

int register_partition_table(int (*register_func)(struct hd_device *));
int detect_hd_partitions(struct hd_device *);

#define MAX_PARTITION_TABLE_TYPES	5

#endif

