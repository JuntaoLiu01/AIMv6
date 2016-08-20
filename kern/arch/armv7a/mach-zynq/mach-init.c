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

#include <aim/device.h>
#include <mach.h>
#include <mach-conf.h>

dev_t rootdev;

void early_mach_init(void)
{

}

void mach_init(void)
{
	rootdev = makedev(SD_MAJOR, ROOT_PARTITION_ID);
}

struct devtree_entry devtree[] = {
	/* memory bus */
	{
		"memory",
		"memory",
		"",
		0,
		{0},
		0
	},
	/* Zynq UART */
	{
		"uart0",
		"uart-zynq",
		"memory",
		1,
		{UART0_PHYSBASE},
		UART0_IRQ
	},
	{
		"uart1",
		"uart-zynq",
		"memory",
		1,
		{UART1_PHYSBASE},
		UART1_IRQ
	},
	{
		"sd0",
		"sd-zynq",
		"memory",
		1,
		{SD0_PHYSBASE},
		SD0_IRQ
	},
	{
		"sd1",
		"sd-zynq",
		"memory",
		1,
		{SD1_PHYSBASE},
		SD1_IRQ
	},
	{
		"mpcore",
		"cortex-a9",
		"memory",
		1,
		{MPCORE_PHYSBASE},
		0
	}
};

int ndevtree_entries = ARRAY_SIZE(devtree);

