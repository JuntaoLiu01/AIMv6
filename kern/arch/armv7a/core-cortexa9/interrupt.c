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
#endif /* HAVE_CONFIG_H */

#include <sys/types.h>
#include <aim/device.h>
#include <arch-trap.h>
#include <errno.h>
#include <io.h>
#include <mp.h>

#define NR_INTS	256

#define MPCORE_ICC_BASE	0x0100
#define MPCORE_ICD_BASE	0x1000

#define ICC_ICR_OFFSET	0x00
#define ICC_PMR_OFFSET	0x04

#define ICD_DCR_OFFSET	0x000
#define ICD_ISER_OFFSET	0x100

static int (*__dispatch[NR_INTS])(int);

struct bus_device *mpcore = NULL;

int handle_interrupt(struct trapframe *regs)
{
	/* TODO */
	kpdebug("<<<INTERRUPTED>>>\n");
	/* NOTREACHED */
	return -EINVAL;
}

void add_interrupt_handler(int (*handler)(int), int irq)
{
	__dispatch[irq] = handler;

	/* enable forwarding for this specific IRQ */
	assert(mpcore != NULL);
	bus_write_fp write32 = mpcore->bus_driver.get_write_fp(mpcore, 32);
	write32(mpcore, MPCORE_ICD_BASE, ICD_ISER_OFFSET + (irq >> 5), 1 << (irq & 0x1F));
}

void accept_IRQ_routing(void)
{
	bus_write_fp write32 = mpcore->bus_driver.get_write_fp(mpcore, 32);
	/* accept all priority IRQs */
	write32(mpcore, MPCORE_ICC_BASE, ICC_PMR_OFFSET, 0xFF);
	/* accept routing */
	write32(mpcore, MPCORE_ICC_BASE, ICC_ICR_OFFSET, 0x1);
}

void init_IRQ(void)
{
	mpcore = (struct bus_device *)dev_from_name("mpcore");
	assert(mpcore != NULL);

	bus_write_fp write32 = mpcore->bus_driver.get_write_fp(mpcore, 32);
	/* enable irq routing */
	write32(mpcore, MPCORE_ICD_BASE, ICD_DCR_OFFSET, 0x1);
	/* accept routing on this core */
	accept_IRQ_routing();
}

