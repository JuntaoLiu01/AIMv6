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

/* from kernel */
#include <sys/types.h>
#include <aim/device.h>
#include <io.h>
#include <trap.h>
#include <mach.h>

/* from timer driver */
#include <timer-a9.h>
#include <timer-a9-hw.h>

// FIXME
#define GTC_TPUS 100l
#define GTC_TPS	(GTC_TPUS * 1000000l)


#ifdef RAW /* baremetal driver */

#define GTC_BASE	GTC_PHYSBASE

uint64_t timer_read(void)
{
	return gt_read();
}

uint64_t gt_read(void)
{
	uint64_t time;
	uint64_t hi, lo, tmp;
	/* HI-LO-HI reading because GTC is 64bit */
	do {
		hi = read32(GTC_BASE + GTC_COUNTER_HI_OFFSET);
		lo = read32(GTC_BASE + GTC_COUNTER_LO_OFFSET);
		tmp = read32(GTC_BASE + GTC_COUNTER_HI_OFFSET);
	} while (hi != tmp);
	time = (uint64_t)hi << 32;
	time |= lo;
	return time;
}

#else /* not RAW, or kernel driver */

#include <sched.h>

extern struct bus_device *mpcore;
void handle_timer_interrupt(void);

static int initialized = false;

static int intr(int irq)
{
	bus_write_fp write32 = mpcore->bus_driver.get_write_fp(mpcore, 32);
	write32(mpcore, MPCORE_GTC_BASE, GTC_INT_OFFSET, 0x1);

	schedule();
	return 0;
}

static inline uint64_t read_counter()
{
	bus_read_fp read32 = mpcore->bus_driver.get_read_fp(mpcore, 32);
	uint64_t time;
	uint64_t hi, lo, tmp;
	/* HI-LO-HI reading because GTC is 64bit */
	do {
		read32(mpcore, MPCORE_GTC_BASE, GTC_COUNTER_HI_OFFSET, &hi);
		read32(mpcore, MPCORE_GTC_BASE, GTC_COUNTER_LO_OFFSET, &lo);
		read32(mpcore, MPCORE_GTC_BASE, GTC_COUNTER_HI_OFFSET, &tmp);
	} while (hi != tmp);
	time = (hi << 32) | (lo & 0xFFFFFFFF);
	return time;
}

void debug_timer(void)
{
	bus_write_fp write32 = mpcore->bus_driver.get_write_fp(mpcore, 32);
	bus_read_fp read32 = mpcore->bus_driver.get_read_fp(mpcore, 32);
	uint64_t tmp;
	write32(mpcore, 0x1000, 0x100, 0xffff0000);
	read32(mpcore, 0x1000, 0x100, &tmp);
	kpdebug("state 0x%08x\n", (uint32_t)tmp);
}

void timer_init(void)
{
	bus_write_fp write32 = mpcore->bus_driver.get_write_fp(mpcore, 32);
	uint64_t time;

	/* read value */
	time = read_counter();
	/* calculate and apply increment */
	uint32_t inc = GTC_TPS / TIMER_FREQ;
	time += inc;
	write32(mpcore, MPCORE_GTC_BASE, GTC_INCREMENT_OFFSET, inc);
	write32(mpcore, MPCORE_GTC_BASE, GTC_COMPARATOR_LO_OFFSET, time & 0xFFFFFFFF);
	write32(mpcore, MPCORE_GTC_BASE, GTC_COMPARATOR_HI_OFFSET, time >> 32);
	/* start: counter, comparator, increment and IRQ */
	write32(mpcore, MPCORE_GTC_BASE, GTC_CTRL_OFFSET, 0xF);
	if (!initialized) {
		/* add handler */
		add_interrupt_handler(intr, 27); // TODO
		initialized = true;
	}
}

void pre_timer_interrupt(void)
{
}

void post_timer_interrupt(void)
{
}

#endif /* RAW */

uint64_t gt_get_tpus(void)
{
	// FIXME
	return GTC_TPUS;
}

uint64_t gt_get_tps(void)
{
	// FIXME
	return GTC_TPS;
}

