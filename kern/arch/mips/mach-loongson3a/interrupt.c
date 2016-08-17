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

#include <arch-trap.h>
#include <cp0regdef.h>
#include <timer.h>
#include <errno.h>
#include <io.h>
#include <mp.h>
#include <platform.h>

#define PANIC_MASK	0x80000000

static int __discard(struct trapframe *regs)
{
	return 0;
}

static int __timer_interrupt(struct trapframe *regs)
{
	handle_timer_interrupt();
	return 0;
}

static int __ipi_interrupt(struct trapframe *regs)
{
	unsigned int ipi_status;

	ipi_status = read32(LOONGSON3A_COREx_IPI_STATUS(cpuid()));
	write32(LOONGSON3A_COREx_IPI_CLEAR(cpuid()), ipi_status);

	if (ipi_status & PANIC_MASK) {
		__local_panic();
		/* NOTREACHED */
		return -EINVAL;
	} else {
		/* TODO: convert IPI status to arch-independent message */
		return handle_ipi_interrupt(ipi_status);
	}
}

static int (*__dispatch[])(struct trapframe *) = {
	NULL,			/* Soft interrupt 0 */
	NULL,			/* Soft interrupt 1 */
	__discard,		/* LPC/UART interrupt */
	__discard,		/* HT1 interrupt */
	NULL,			/* Unused */
	NULL,			/* Unused */
	__ipi_interrupt,	/* IPI */
	__timer_interrupt	/* Timer */
};

/* TODO: move to mach-specific code */
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

void panic_other_cpus(void)
{
	int i;

	for (i = 0; i < nr_cpus(); ++i) {
		if (i == cpuid())
			continue;
		write32(LOONGSON3A_COREx_IPI_SET(i), PANIC_MASK);
	}
}

