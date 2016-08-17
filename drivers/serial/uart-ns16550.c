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

#include <uart-ns16550.h>
#include <uart-ns16550-hw.h>

#include <sys/types.h>
#include <util.h>
#include <io.h>
#include <mm.h>
#include <aim/console.h>
#include <platform.h>
#include <aim/device.h>
#include <aim/console.h>
#include <drivers/io/io-mem.h>
#include <drivers/io/io-port.h>

#ifdef i386
#define NS16550_PORTIO		/* cases where NS16550 is on a port I/O bus */
#endif

/* internal routines */

static struct chr_device __early_uart_ns16550 = {
	.base = UART_BASE,
	.class = DEVCLASS_CHR,
};

static void __uart_ns16550_init(struct chr_device *inst)
{
	struct bus_device *bus = inst->bus;
	bus_write_fp bus_write8 = bus->bus_driver.get_write_fp(bus, 8);

	if (bus_write8 == NULL)
		return;		/* should panic? */

	/* TODO: check if the following configuration works across all
	 * UARTs */
	bus_write8(bus, inst->base, UART_FIFO_CONTROL,
	    UART_FCR_RTB_4 | UART_FCR_RST_TRANSMIT | UART_FCR_RST_RECEIVER |
	    UART_FCR_ENABLE);
	bus_write8(bus, inst->base, UART_LINE_CONTROL, UART_LCR_DLAB);
	bus_write8(bus, inst->base, UART_DIVISOR_LSB,
	    (UART_FREQ / UART_BAUDRATE) & 0xff);
	bus_write8(bus, inst->base, UART_DIVISOR_MSB,
	    ((UART_FREQ / UART_BAUDRATE) >> 8) & 0xff);
	bus_write8(bus, inst->base, UART_LINE_CONTROL,
	    UART_LCR_DATA_8BIT |
	    UART_LCR_STOP_1BIT |
	    UART_LCR_PARITY_NONE);
}

static void __uart_ns16550_enable(struct chr_device *inst)
{
	struct bus_device *bus = inst->bus;
	bus_write_fp bus_write8 = bus->bus_driver.get_write_fp(bus, 8);

	if (bus_write8 == NULL)
		return;		/* should panic? */

	bus_write8(bus, inst->base, UART_MODEM_CONTROL,
	    UART_MCR_RTSC | UART_MCR_DTRC);
}

static void __uart_ns16550_disable(struct chr_device *inst)
{
	struct bus_device *bus = inst->bus;
	bus_write_fp bus_write8 = bus->bus_driver.get_write_fp(bus, 8);

	if (bus_write8 == NULL)
		return;		/* should panic? */

	bus_write8(bus, inst->base, UART_MODEM_CONTROL, 0);
}

static void __uart_ns16550_enable_interrupt(struct chr_device *inst)
{
	struct bus_device *bus = inst->bus;
	bus_write_fp bus_write8 = bus->bus_driver.get_write_fp(bus, 8);

	if (bus_write8 == NULL)
		return;		/* should panic? */

	bus_write8(bus, inst->base, UART_INTR_ENABLE, UART_IER_RBFI);
}

static void __uart_ns16550_disable_interrupt(struct chr_device *inst)
{
	struct bus_device *bus = inst->bus;
	bus_write_fp bus_write8 = bus->bus_driver.get_write_fp(bus, 8);

	if (bus_write8 == NULL)
		return;		/* should panic? */

	bus_write8(bus, inst->base, UART_INTR_ENABLE, 0);
}

static unsigned char __uart_ns16550_getchar(struct chr_device *inst)
{
	struct bus_device *bus = inst->bus;
	bus_read_fp bus_read8 = bus->bus_driver.get_read_fp(bus, 8);
	uint64_t buf;

	if (bus_read8 == NULL)
		return 0;		/* should panic? */
	do {
		bus_read8(bus, inst->base, UART_LINE_STATUS, &buf);
	} while (!(buf & UART_LSR_DATA_READY));

	bus_read8(bus, inst->base, UART_RCV_BUFFER, &buf);
	return (unsigned char)buf;
}

static int __uart_ns16550_putchar(struct chr_device *inst, unsigned char c)
{
	struct bus_device *bus = inst->bus;
	bus_read_fp bus_read8 = bus->bus_driver.get_read_fp(bus, 8);
	bus_write_fp bus_write8 = bus->bus_driver.get_write_fp(bus, 8);
	uint64_t buf;

	if (bus_read8 == NULL || bus_write8 == NULL)
		return EOF;

	do {
		bus_read8(bus, inst->base, UART_LINE_STATUS, &buf);
	} while (!(buf & UART_LSR_THRE));

	bus_write8(bus, inst->base, UART_TRANS_HOLD, c);
	return 0;
}

#ifdef RAW /* baremetal driver */

#else /* not RAW, or kernel driver */

/* Meant to register to kernel, so this interface routine is static */
static int early_console_putchar(unsigned char c)
{
	__uart_ns16550_putchar(&__early_uart_ns16550, c);
	return 0;
}

static void __mmu_handler(void)
{
	/* Currently both Loongson 3A and i386 does not need remapping
	 * since MIPS doesn't require that and i386 is doing port I/O */
	__early_uart_ns16550.base = UART_BASE;
}

static void __early_console_init_bus(void)
{
	/* select bus for NS16550 */
#ifdef NS16550_PORTIO
	__early_uart_ns16550.bus = &early_portio_bus;
#else
	__early_uart_ns16550.bus = &early_memory_bus;
#endif
}

static void __jump_handler(void)
{
	__early_console_init_bus();
	set_console(early_console_putchar, DEFAULT_KPUTS);
}

int early_console_init(void)
{
	__early_console_init_bus();

	__uart_ns16550_init(&__early_uart_ns16550);
	__uart_ns16550_enable(&__early_uart_ns16550);

	set_console(early_console_putchar, DEFAULT_KPUTS);

	if (mmu_handlers_add(__mmu_handler) != 0)
		for (;;) ;	/* panic */
	if (jump_handlers_add((generic_fp)postmap_addr(__jump_handler)) != 0)
		for (;;) ;	/* panic */
	return 0;
}

#include "uart-ns16550-kernel.c"

#endif /* RAW */

