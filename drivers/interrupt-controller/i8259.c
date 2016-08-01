
#include <sys/types.h>
#include <aim/device.h>
#include <drivers/interrupt-controller/i8259.h>
#include <platform.h>	/* I8259_IRQ_BASE */
#include <sleep.h>

static struct bus_device *portio;
static bus_read_fp in8;
static bus_write_fp out8;
static lock_t lock = EMPTY_LOCK(lock);

void i8259_eoi(int irq)
{
	unsigned long flags;
	assert(portio != NULL);
	assert(out8 != NULL);

	/*
	 * According to Linux, acknowledging interrupt in Intel 8259 should
	 * mask the IRQ first, then send an EOI, and unmask.
	 */
	spin_lock_irq_save(&lock, flags);
	if (irq >= 8) {
		out8(portio, 0, PIC_SLAVE_IMR, 1 << (irq & 7));
		out8(portio, 0, PIC_SLAVE_CMD, PIC_SPEC_EOI + (irq & 7));
		out8(portio, 0, PIC_MASTER_CMD, PIC_SPEC_EOI + PIC_CASCADE_IR);
		out8(portio, 0, PIC_SLAVE_IMR, 0);
	} else {
		out8(portio, 0, PIC_MASTER_IMR, 1 << irq);
		out8(portio, 0, PIC_MASTER_CMD, PIC_SPEC_EOI + irq);
		out8(portio, 0, PIC_MASTER_IMR, 0);
	}
	spin_unlock_irq_restore(&lock, flags);
}

/*
 * The full initialization routine is described at
 * https://pdos.csail.mit.edu/6.828/2010/readings/hardware/8259A.pdf
 * But you probably don't care how Intel 8259 is initialized anyway.
 */
void i8259_init(bool auto_eoi)
{
	portio = (struct bus_device *)dev_from_name("portio");
	assert(portio != NULL);
	in8 = portio->bus_driver.get_read_fp(portio, 8);
	out8 = portio->bus_driver.get_write_fp(portio, 8);
	kpdebug("initializing Intel 8259\n");

	/* Mask all interrupts */
	out8(portio, 0, PIC_MASTER_IMR, 0xff);
	out8(portio, 0, PIC_SLAVE_IMR, 0xff);

	/* Master ICW1: With ICW4, Edge-triggered, Cascade PICs */
	out8(portio, 0, PIC_MASTER_CMD, 0x11);
	/* Master ICW2: Base IRQ */
	out8(portio, 0, PIC_MASTER_IMR, I8259_IRQ_BASE);
	/* Master ICW3: Bit mask of interrupt lines connected to slaves */
	out8(portio, 0, PIC_MASTER_IMR, 1 << PIC_CASCADE_IR);
	/* Master ICW4: Intel x86 mode, and auto-EOI depend on @auto_eoi */
	out8(portio, 0, PIC_MASTER_IMR, 1 | (auto_eoi ? PIC_ICW4_AEOI : 0));

	/* Slave ICW1: With ICW4, Edge-triggered, Cascade PICs */
	out8(portio, 0, PIC_SLAVE_CMD, 0x11);
	/* Slave ICW2: Base IRQ */
	out8(portio, 0, PIC_SLAVE_IMR, I8259_IRQ_BASE + 8);
	/* Slave ICW3: # of slave's connection to master */
	out8(portio, 0, PIC_SLAVE_IMR, PIC_CASCADE_IR);
	/* Slave ICW4: normal EOI, Intel x86 mode */
	out8(portio, 0, PIC_SLAVE_IMR, 1);

	udelay(100);

	/* Read IRR by default */
	out8(portio, 0, PIC_MASTER_CMD, 0x0a);
	out8(portio, 0, PIC_SLAVE_CMD, 0x0a);

	/* Enable all interrupts */
	out8(portio, 0, PIC_MASTER_IMR, 0x00);
	out8(portio, 0, PIC_SLAVE_IMR, 0x00);
	kpdebug("initialized Intel 8259\n");
}

