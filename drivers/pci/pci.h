
#ifndef _DRIVERS_PCI_PCI_H
#define _DRIVERS_PCI_PCI_H

#include <aim/device.h>
#include <drivers/pci/pci_regs.h>
#include <sys/types.h>

#define make_pci_tag(bus, dev, func) \
	(((bus) << 16) | ((dev) << 11) | ((func) << 8))

struct pci_bus_device {
	struct bus_device;

	/* Configuration space read/write, for internal use only */
	bus_read_fp r8, r16, r32;
	bus_write_fp w8, w16, w32;
};

struct pci_driver_table_entry {
	uint16_t	vendor;
	uint16_t	device;
	char		model[DEV_NAME_MAX];
	bool		(*check)(struct pci_bus_device *, uint32_t,
				 struct devtree_entry *);
};

void register_pci(uint16_t, uint16_t, char *,
    bool (*)(struct pci_bus_device *, uint32_t, struct devtree_entry *));
int pci_probe(struct bus_device *);

#endif
