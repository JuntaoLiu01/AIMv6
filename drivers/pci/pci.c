
/*
 * This is the common code for all PCI buses.
 *
 * Configuration space reading and writing are provided in separate files
 * pci-mem.c and pci-io.c, depending on what bus the PCI bus is attached to,
 * or which platform the PCI bus is on.
 */

#include <aim/device.h>
#include <aim/initcalls.h>
#include <sys/types.h>
#include <sys/param.h>
#include <errno.h>
#include <drivers/pci/pci.h>
#include <libc/string.h>
#include <libc/stdio.h>
#include <panic.h>

#define DEVICE_MODEL	"pci"

static struct bus_driver drv;

static struct bus_device *mem, *io;

static int __rw(struct bus_device *inst, addr_t base, addr_t offset,
    uint64_t *val_read, uint64_t val_write, bool write, int width)
{
	int space = base & PCI_BASE_ADDRESS_SPACE;
	struct bus_device *bus;
	bus_read_fp r;
	bus_write_fp w;

	switch (space) {
	case PCI_BASE_ADDRESS_SPACE_IO:
		bus = io;
		base &= PCI_BASE_ADDRESS_IO_MASK;
		break;
	case PCI_BASE_ADDRESS_SPACE_MEMORY:
		bus = mem;
		base &= PCI_BASE_ADDRESS_MEM_MASK;
		break;
	default:
		break;
	}

	if (write) {
		w = bus->bus_driver.get_write_fp(bus, width);
		w(bus, base, offset, val_write);
	} else {
		r = bus->bus_driver.get_read_fp(bus, width);
		r(bus, base, offset, val_read);
	}

	return 0;
}

static int __write8(struct bus_device *inst, addr_t base, addr_t offset,
    uint64_t val)
{
	return __rw(inst, base, offset, NULL, val, true, 8);
}

static int __write16(struct bus_device *inst, addr_t base, addr_t offset,
    uint64_t val)
{
	return __rw(inst, base, offset, NULL, val, true, 16);
}

static int __write32(struct bus_device *inst, addr_t base, addr_t offset,
    uint64_t val)
{
	return __rw(inst, base, offset, NULL, val, true, 32);
}

static int __read8(struct bus_device *inst, addr_t base, addr_t offset,
    uint64_t *val)
{
	return __rw(inst, base, offset, val, 0, false, 8);
}

static int __read16(struct bus_device *inst, addr_t base, addr_t offset,
    uint64_t *val)
{
	return __rw(inst, base, offset, val, 0, false, 16);
}

static int __read32(struct bus_device *inst, addr_t base, addr_t offset,
    uint64_t *val)
{
	return __rw(inst, base, offset, val, 0, false, 32);
}

static bus_read_fp __get_read_fp(struct bus_device *inst, int width)
{
	switch (width) {
	case 8:
		return __read8;
	case 16:
		return __read16;
	case 32:
		return __read32;
	}
	return NULL;
}

static bus_write_fp __get_write_fp(struct bus_device *inst, int width)
{
	switch (width) {
	case 8:
		return __write8;
	case 16:
		return __write16;
	case 32:
		return __write32;
	}
	return NULL;
}

static int __new(struct devtree_entry *entry)
{
	struct pci_bus_device *dev;
	if (strcmp(entry->model, DEVICE_MODEL) != 0)
		return -ENOTSUP;
	/* PCI bus requires both memory bus and I/O bus to work */
	mem = (struct bus_device *)dev_from_name("memory");
	io = (struct bus_device *)dev_from_name("portio");
	if (mem == NULL || io == NULL)
		return -ENODEV;

	kpdebug("initializing PCI bus\n");
	dev = kmalloc(sizeof(*dev), GFP_ZERO);
	if (dev == NULL)
		return -ENOMEM;
	initdev(dev, DEVCLASS_BUS, entry->name, NODEV, &drv);
	dev->bus = (struct bus_device *)dev_from_name(entry->parent);
	dev->base = entry->regs[0];
	dev->nregs = entry->nregs;

	/* All the __xxxcfg are provided in outer source (e.g. pci-mem.c) */
	dev->r8 = __readcfg8;
	dev->r16 = __readcfg16;
	dev->r32 = __readcfg32;
	dev->w8 = __writecfg8;
	dev->w16 = __writecfg16;
	dev->w32 = __writecfg32;

	dev_add(dev);
	return 0;
}

static struct bus_driver drv = {
	.class = DEVCLASS_BUS,
	.new = __new,
	.probe = pci_probe,
	.get_read_fp = __get_read_fp,
	.get_write_fp = __get_write_fp,
};

static int __driver_init(void)
{
	register_driver(NOMAJOR, &drv);
	return 0;
}
INITCALL_DRIVER(__driver_init);

