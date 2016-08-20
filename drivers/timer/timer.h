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

#ifndef _DRIVERS_TIMER_TIMER_H
#define _DRIVERS_TIMER_TIMER_H

#ifdef RAW /* baremetal driver */

/* All timers should have 64-bit interface */
uint64_t timer_read(void);
//void timer_write(uint64_t count);

#else /* not RAW, or kernel driver */

#endif /* RAW */

void delay(uint32_t s);
void udelay(uint32_t us);

#endif /* _DRIVERS_TIMER_TIMER_H */

