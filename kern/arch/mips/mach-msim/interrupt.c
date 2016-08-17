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

#include <arch-trap.h>
#include <cp0regdef.h>
#include <timer.h>
#include <errno.h>
#include <panic.h>
#include <mp.h>
#include <sys/param.h>
#include <trap.h>
#include <mipsregs.h>

static int (*__disk_dispatch)(int);
static int (*__kbd_dispatch)(int);

void init_IRQ(void)
{
}

static int __kbd_interrupt(struct trapframe *regs)
{
	if (cpuid() != 0)
		return 0;
	if (__kbd_dispatch != NULL)
		__kbd_dispatch(3);	/* IRQ does not matter */
	return 0;
}

static int __disk_interrupt(struct trapframe *regs)
{
	if (cpuid() != 0)
		return 0;
	if (__disk_dispatch != NULL)
		__disk_dispatch(2);	/* IRQ does not matter */
	return 0;
}

static int __ipi_interrupt(struct trapframe *regs)
{
	unsigned int mailbox = read_msim_mailbox(cpuid());
	__ack_ipi();
	if (mailbox == -1) {
		__local_panic();
		/* NOTREACHED */
		return -EINVAL;
	} else {
		/* TODO: convert mailbox value to arch-independent message */
		return handle_ipi_interrupt(mailbox);
	}
}

static int __timer_interrupt(struct trapframe *regs)
{
	handle_timer_interrupt();
	return 0;
}

static int (*__dispatch[])(struct trapframe *) = {
	NULL,			/* Soft interrupt 0 */
	NULL,			/* Soft interrupt 1 */
	__disk_interrupt,	/* Disk interrupt */
	__kbd_interrupt,	/* Keyboard interrupt */
	NULL,			/* Unused */
	NULL,			/* Unused */
	__ipi_interrupt,	/* IPI */
	__timer_interrupt	/* Timer */
};

int handle_interrupt(struct trapframe *regs)
{
	int i;

	for (i = 7; i >= 0; --i) {
		if (regs->cause & CR_IPx(i)) {
			if (__dispatch[i] != NULL)
				return __dispatch[i](regs);
			else
				return -ENOENT;
		}
	}
	/* NOTREACHED */
	return -EINVAL;
}

void add_interrupt_handler(int (*handler)(int), int irq)
{
	switch (irq) {
	case 2:
		__disk_dispatch = handler;
		break;
	case 3:
		__kbd_dispatch = handler;
		break;
	}
}

void panic_other_cpus(void)
{
	uint32_t cpumask;
	int i;

	for (i = 0; i < nr_cpus(); ++i) {
		if (i == cpuid())
			continue;
		write_msim_mailbox(i, -1);
	}

	/* cpumask = 0x1110 if current cpuid is 0. */
	cpumask = ((1 << nr_cpus()) - 1) & ~(1 << cpuid());
	write32(MSIM_ORDER_PHYSADDR + MSIM_ORDER_REG_IPI_SEND, cpumask);
}

