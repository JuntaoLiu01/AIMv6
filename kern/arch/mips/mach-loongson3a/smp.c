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
#include <platform.h>
#include <mipsregs.h>

extern void slave_entry(void);

int nr_cpus(void)
{
	return NR_CPUS;
}

void mach_smp_startup(void)
{
	int i;

	for (i = 1; i < nr_cpus(); ++i) {
		write64(LOONGSON3A_COREx_IPI_MB0(i),
			(unsigned long)slave_entry);
	}
}

void enable_ipi_interrupt(void)
{
	uint32_t status = read_c0_status();
	write_c0_status(status | ST_IMx(6));
}

