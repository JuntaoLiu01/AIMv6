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
#include <timer.h>
#include <mipsregs.h>

uint32_t inc;

void timer_init(void)
{
	uint32_t count, status;

	inc = CPU_FREQ / TIMER_FREQ;
	status = read_c0_status();
	write_c0_status(status | ST_IMx(7));
	count = read_c0_count();
	write_c0_compare(count + inc);
}

void pre_timer_interrupt(void)
{
}

void post_timer_interrupt(void)
{
	write_c0_compare(read_c0_count() + inc);
}

