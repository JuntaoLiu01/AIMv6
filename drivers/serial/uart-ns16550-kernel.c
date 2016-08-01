
#include <sys/types.h>
#include <aim/device.h>
#include <aim/initcalls.h>
#include <libc/string.h>
#include <libc/stddef.h>
#include <errno.h>
#include <mach-conf.h>
#include <sched.h>
#include <trap.h>
#include <drivers/console/cons.h>
#include <asm-generic/funcs.h>

#define DEVICE_MODEL	"ns16550"

static struct chr_driver drv;

static int __intr(int irq)
{
	struct chr_device *dev;

	dev = (struct chr_device *)dev_from_id(makedev(UART_MAJOR, 0));
	assert(dev != NULL);
	return cons_intr(irq, dev, __uart_ns16550_getchar);
}

static int __new(struct devtree_entry *entry)
{
	struct chr_device *dev;

	if (strcmp(entry->model, DEVICE_MODEL) != 0)
		return -ENOTSUP;
	kpdebug("initializing UART 16550\n");
	dev = kmalloc(sizeof(*dev), GFP_ZERO);
	if (dev == NULL)
		return -ENOMEM;
	initdev(dev, DEVCLASS_CHR, entry->name, makedev(UART_MAJOR, 0), &drv);
	dev->bus = (struct bus_device *)dev_from_name(entry->parent);
	dev->base = entry->regs[0];
	dev->nregs = entry->nregs;
	dev_add(dev);
	/* Flush FIFO XXX */
	kprintf("\n");
	__uart_ns16550_init(dev);
	__uart_ns16550_enable(dev);
	__uart_ns16550_enable_interrupt(dev);
	add_interrupt_handler(__intr, entry->irq);
	return 0;
}

static int __open(dev_t devno, int mode, struct proc *p)
{
	struct chr_device *dev;
	kpdebug("opening UART 16550\n");
	dev = (struct chr_device *)dev_from_id(devno);
	assert(dev != NULL);
	cons_open(dev, mode, p);
	return 0;
}

static int __close(dev_t devno, int mode, struct proc *p)
{
	/* currently we do nothing */
	return 0;
}

static int __putc(dev_t devno, int c)
{
	struct chr_device *dev;
	dev = (struct chr_device *)dev_from_id(devno);
	assert(dev != NULL);
	return cons_putc(dev, c, __uart_ns16550_putchar);
}

static int __getc(dev_t devno)
{
	struct chr_device *dev;
	dev = (struct chr_device *)dev_from_id(devno);
	assert(dev != NULL);
	return cons_getc(dev);
}

static int __write(dev_t devno, struct uio *uio, int ioflags)
{
	struct chr_device *dev;

	dev = (struct chr_device *)dev_from_id(devno);
	assert(dev == NULL);
	return cons_write(dev, uio, ioflags, __uart_ns16550_putchar);
}

static struct chr_driver drv = {
	.class = DEVCLASS_CHR,
	.new = __new,
	.open = __open,
	.close = __close,
	.read = NOTSUP,	/* NYI */
	.write = __write,
	.getc = __getc,
	.putc = __putc,
};

static int __driver_init(void)
{
	register_driver(UART_MAJOR, &drv);
	return 0;
}
INITCALL_DRIVER(__driver_init);
