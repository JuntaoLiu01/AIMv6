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

#include <mp.h>
#include <smp.h>

extern void slave_entry(void);

int nr_cpus(void)
{
	return 4;
}

void mach_smp_startup(void)
{
	int i;

	for (i = 1; i < nr_cpus(); ++i) {
		write32(MSIM_ORDER_MAILBOX_BASE +
		    (1 << MSIM_ORDER_MAILBOX_ORDER) * i,
		    (unsigned long)slave_entry);
	}
}

