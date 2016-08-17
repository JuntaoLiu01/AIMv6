/* Copyright (C) 2016 Gan Quan <coin2028@hotmail.com>
 * Copyright (C) 2016 David Gao <davidgao1001@gmail.com>
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

#include <uart.h>
#include <uart-msim.h>

#include <io.h>
#include <aim/console.h>
#include <aim/device.h>
#include <drivers/io/io-mem.h>
#include <mm.h>

/*
 * Design note:
 * Strictly speaking, MSIM line printer (lp) and keyboard (kbd) drivers
 * have to be implemented in separate files.
 */

/* Should only be used before memory management is initialized */
static struct chr_device __early_uart_msim_lp = {
	.base = MSIM_LP_PHYSADDR,
	.class = DEVCLASS_CHR,
};

static struct chr_device __early_uart_msim_kbd = {
	.base = MSIM_KBD_PHYSADDR,
	.class = DEVCLASS_CHR,
};

/*
 * uart-msim is a combined device, so there's two base address
 * @lp and @kbd can be NULL.
 */
static void __uart_msim_init(struct chr_device *lp, struct chr_device *kbd)
{
}

static int __uart_msim_putchar(struct chr_device *lp, unsigned char c)
{
	struct bus_device *bus = lp->bus;
	bus_write_fp bus_write8 = bus->bus_driver.get_write_fp(bus, 8);
	bus_write8(bus, lp->base, 0, c);
	return 0;
}

static unsigned char __uart_msim_getchar(struct chr_device *kbd)
{
	uint64_t b;
	struct bus_device *bus = kbd->bus;

	bus_read_fp bus_read8 = bus->bus_driver.get_read_fp(bus, 8);

	do {
		bus_read8(bus, kbd->base, 0, &b);
	} while (!b);

	return b;
}

#ifdef RAW /* baremetal driver */

void uart_init(void)
{
	/* MSIM line printer and keyboard are memory-mapped devices */
	__early_uart_msim_lp.bus = &early_memory_bus;
	__early_uart_msim_kbd.bus = &early_memory_bus;

	__uart_msim_init(&__early_uart_msim_lp, &__early_uart_msim_kbd);
}

void uart_enable(void)
{
	/* nothing */
}

void uart_disable(void)
{
	/* nothing */
}

unsigned char uart_getchar(void)
{
	return __uart_msim_getchar(&__early_uart_msim_kbd);
}

int uart_putchar(unsigned char c)
{
	return __uart_msim_putchar(&__early_uart_msim_lp, c);
}

#else /* not RAW, or kernel driver */

int early_console_putchar(unsigned char c)
{
	__uart_msim_putchar(&__early_uart_msim_lp, c);
	return 0;
}

static void __mmu_handler(void)
{
	/* nothing since it's always on MIPS */
}

static void __jump_handler(void)
{
	/* ditto */
}

int early_console_init(void)
{
	__early_uart_msim_lp.bus = &early_memory_bus;
	__early_uart_msim_kbd.bus = &early_memory_bus;
	__uart_msim_init(&__early_uart_msim_lp, &__early_uart_msim_kbd);
	set_console(early_console_putchar, DEFAULT_KPUTS);

	if (mmu_handlers_add(__mmu_handler) != 0)
		for (;;) ;	/* panic */
	if (jump_handlers_add(postmap_addr(__jump_handler)) != 0)
		for (;;) ;	/* panic */

	return 0;
}

#include <uart-msim-kernel.c>

#endif /* RAW */

