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

#include <sys/types.h>

#include <uart-zynq.h>
#include <uart-zynq-hw.h>

#include <aim/console.h>
#include <aim/device.h>
#include <aim/early_kmmap.h>
#include <aim/initcalls.h>
#include <aim/kmmap.h>
#include <mm.h>
#include <panic.h>
#include <errno.h>
#include <mach-conf.h>

#include <drivers/io/io-mem.h>

/* we need macro comparision */
#define UART_ZYNQ	1

/* FIXME zedboard uses UART1 only */
#define UART_BASE	UART1_PHYSBASE

/* Should only be used before memory management is initialized */
static struct chr_device __early_uart_zynq;

/* internal routines */
static inline void __uart_zynq_enable(struct chr_device * inst)
{
	struct bus_device * bus = inst->bus;
	bus_write_fp bus_write32 = bus->bus_driver.get_write_fp(bus, 32);

	/* Enable TX and RX */
	bus_write32(bus, inst->base, UART_OFFSET_CR,
		UART_CR_TX_EN | UART_CR_RX_EN);
}

static inline void __uart_zynq_disable(struct chr_device * inst)
{
	struct bus_device * bus = inst->bus;
	bus_write_fp bus_write32 = bus->bus_driver.get_write_fp(bus, 32);

	/* Disable TX and RX */
	bus_write32(bus, inst->base, UART_OFFSET_CR,
		UART_CR_TX_DIS | UART_CR_RX_DIS);
}

static inline void __uart_zynq_init(struct chr_device * inst)
{
	struct bus_device * bus = inst->bus;
	bus_write_fp bus_write32 = bus->bus_driver.get_write_fp(bus, 32);

	/* Disable interrupts */
	bus_write32(bus, inst->base, UART_OFFSET_IDR, UART_IXR_MASK);
	/* Disable TX and RX */
	__uart_zynq_disable(inst);
	/* Reset TX and RX, Clear FIFO */
	bus_write32(bus, inst->base, UART_OFFSET_CR,
		UART_CR_TXRST | UART_CR_RXRST);
	/* Clear Flags */
	bus_write32(bus, inst->base, UART_OFFSET_ISR, UART_IXR_MASK);
	/* Mode Reset to Normal/8-N-1 */
	bus_write32(bus, inst->base, UART_OFFSET_MR,
		UART_MR_CHMODE_NORM | UART_MR_CHARLEN_8_BIT |
		UART_MR_PARITY_NONE | UART_MR_STOPMODE_1_BIT);
	/* Trigger Reset */
	bus_write32(bus, inst->base, UART_OFFSET_RXWM, UART_RXWM_RESET_VAL);
	bus_write32(bus, inst->base, UART_OFFSET_TXWM, UART_TXWM_RESET_VAL);
	/* Disable RX timeout */
	bus_write32(bus, inst->base, UART_OFFSET_RXTOUT, UART_RXTOUT_DISABLE);
	/* Reset baud rate generator and divider to genetate 115200 */
	bus_write32(bus, inst->base, UART_OFFSET_BAUDGEN, 0x3E);
	bus_write32(bus, inst->base, UART_OFFSET_BAUDDIV, 0x06);
	/* Set CR Value */
	bus_write32(bus, inst->base, UART_OFFSET_CR,
		UART_CR_RX_DIS | UART_CR_TX_DIS | UART_CR_STOPBRK);
}

static inline unsigned char __uart_zynq_getchar(struct chr_device * inst)
{
	struct bus_device * bus = inst->bus;
	bus_read_fp bus_read8 = bus->bus_driver.get_read_fp(bus, 8);
	bus_read_fp bus_read32 = bus->bus_driver.get_read_fp(bus, 32);
	uint64_t tmp;

	do {
		bus_read32(bus, inst->base, UART_OFFSET_SR, &tmp);
	} while (tmp & UART_SR_RXEMPTY);
	bus_read8(bus, inst->base, UART_OFFSET_FIFO, &tmp);

	return (unsigned char)tmp;
	/* if anything goes wrong, this routine should return EOF */
}

/* all corresponding interface routines should return an int
 * to indicate success or failure.
 */
static inline int __uart_zynq_putchar(struct chr_device * inst, unsigned char c)
{
	struct bus_device * bus = inst->bus;
	bus_read_fp bus_read32 = bus->bus_driver.get_read_fp(bus, 32);
	bus_write_fp bus_write8 = bus->bus_driver.get_write_fp(bus, 8);
	uint64_t tmp;

	do {
		bus_read32(bus, inst->base, UART_OFFSET_SR, &tmp);
	} while (tmp & UART_SR_TXFULL);
	bus_write8(bus, inst->base, UART_OFFSET_FIFO, c);

	return 0;
}

#ifdef RAW /* baremetal driver */

void uart_init(void)
{
	__early_uart_zynq.base = UART_BASE;
	__early_uart_zynq.bus = &early_memory_bus;
	__uart_zynq_init(&__early_uart_zynq);
}

void uart_enable(void)
{
	__uart_zynq_enable(&__early_uart_zynq);
}

void uart_disable(void)
{
	__uart_zynq_disable(&__early_uart_zynq);
}

unsigned char uart_getchar(void)
{
	return __uart_zynq_getchar(&__early_uart_zynq);
}

int uart_putchar(unsigned char c)
{
	__uart_zynq_putchar(&__early_uart_zynq, c);
	return 0;
}

#else /* not RAW, or kernel driver */

#define DEVICE_MODEL	"uart-zynq"

#include <panic.h>

/* forward declarations */
static int new(struct devtree_entry *entry);

static struct chr_driver drv = {
	.class = DEVCLASS_CHR,
	.new = new,
/*	.open = __open,
	.close = __close,
	.read = NOTSUP,
	.write = __write,
	.getc = __getc,
	.putc = __putc,*/
};

/* recognize UART0 and UART1 from physical address. */
static inline int get_minor(struct devtree_entry *entry)
{
	switch (entry->regs[0]) {
		case UART0_PHYSBASE:
			return 0;
		case UART1_PHYSBASE:
			return 1;
		default:
			return 2;
	}
}

static int new(struct devtree_entry *entry)
{
	struct chr_device *dev;
	int minor;

	if (strcmp(entry->model, DEVICE_MODEL) != 0)
		return -ENOTSUP;
	kpdebug("<uart-zynq> initializing device.\n");
	void *vaddr = kmmap(NULL, entry->regs[0], PAGE_SIZE, MAP_SHARED_DEV);
	dev = kmalloc(sizeof(*dev), GFP_ZERO);
	if (dev == NULL)
		return -ENOMEM;
	minor = get_minor(entry);
	initdev(dev, DEVCLASS_CHR, entry->name, makedev(UART_MAJOR, minor), &drv);
	dev->bus = (struct bus_device *)dev_from_name(entry->parent);
	dev->base = vaddr;
	dev->nregs = entry->nregs;
	dev_add(dev);
	/* Flush FIFO XXX */
	__uart_zynq_init(dev);
	__uart_zynq_enable(dev);
//	__uart_zynq_enable_interrupt(dev);
//	add_interrupt_handler(__intr, entry->irq);
	kpdebug("<uart-zynq> device registered.\n");
	return 0;
}
static int __init(void)
{
	kputs("KERN: <uart-zynq> Initializing.\n");
	register_driver(UART_MAJOR, &drv);
	kputs("KERN: <uart-zynq> Ready.\n");
	return 0;
}

INITCALL_DRIVER(__init)

#if PRIMARY_CONSOLE == UART_ZYNQ

static void *__early_mapped_base;

/* Meant to register to kernel, so this interface routine is static */
static int __early_console_putchar(unsigned char c)
{
	__uart_zynq_putchar(&__early_uart_zynq, c);
	return 0;
}

static void __mmu_handler(void)
{
	__early_uart_zynq.base = (size_t)__early_mapped_base;
}

static void __jump_handler(void)
{
	__early_uart_zynq.bus = &early_memory_bus;
	set_console(
		__early_console_putchar,
		DEFAULT_KPUTS
	);
}

int early_console_init(void)
{
	__early_uart_zynq.base = UART_BASE;
	__early_uart_zynq.bus = &early_memory_bus;
	__uart_zynq_init(&__early_uart_zynq);
	__uart_zynq_enable(&__early_uart_zynq);
	set_console(
		__early_console_putchar,
		DEFAULT_KPUTS
	);
	__early_mapped_base = early_mapping_add_kmmap(UART0_PHYSBASE, 1<<20);
	if (__early_mapped_base == 0)
		panic("Zynq UART driver cannot map device.\n");
	__early_mapped_base += UART_BASE - UART0_PHYSBASE;
	if (mmu_handlers_add(__mmu_handler) != 0)
		panic("Zynq UART driver cannot register MMU handler.\n");
	if (jump_handlers_add((generic_fp)(size_t)postmap_addr(__jump_handler)) != 0)
		panic("Zynq UART driver cannot register JUMP handler.\n");
	return 0;
}

#endif /* PRIMARY_CONSOLE == uart_zynq */

#endif /* RAW */

