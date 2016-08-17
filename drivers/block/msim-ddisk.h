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

#ifndef _DRIVERS_HD_MSIM_DDISK_H
#define _DRIVERS_HD_MSIM_DDISK_H

#define MSIM_DD_MAX	256

#define MSIM_DD_DMAADDR	0x0
#define MSIM_DD_SECTOR	0x4
#define MSIM_DD_STAT	0x8
#define MSIM_DD_COMMAND	0x8
#define MSIM_DD_SIZE	0xc

#define STAT_ERROR	0x8
#define STAT_INTR	0x4

#define CMD_ACK		0x4
#define CMD_WRITE	0x2
#define CMD_READ	0x1

#ifdef RAW
void	msim_dd_init(unsigned long paddr);

size_t	msim_dd_get_sector_count(unsigned long);
int	msim_dd_read_sector(unsigned long, off_t, void *, bool);
int	msim_dd_write_sector(unsigned long, off_t, void *, bool);
int	msim_dd_check_interrupt(unsigned long);
void	msim_dd_ack_interrupt(unsigned long);
#endif

#endif
