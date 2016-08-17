
#include <sys/types.h>
#include <drivers/pci/pci.h>
#include <libc/string.h>
#include <libc/stdio.h>

#define PCI_MODELS_MAX	128
static struct pci_driver_table_entry modeltable[PCI_MODELS_MAX];
static int nentries = 0;

/*
 * In practice this should be a (hash) list keeping all discovered PCI device
 * tags.  Here, we use a single boolean variable since we are dealing with
 * only one PCI device.
 */
static bool probed = false;

void register_pci(uint16_t vendor, uint16_t device, char *model,
    bool (*check)(struct pci_bus_device *, uint32_t, struct devtree_entry *))
{
	modeltable[nentries].vendor = vendor;
	modeltable[nentries].device = device;
	strlcpy(modeltable[nentries].model, model, DEV_NAME_MAX);
	modeltable[nentries].check = check;
	++nentries;
}

int pci_probe(struct bus_device *inst)
{
	/*
	 * In practice, we should enumerate over all combinations of
	 * bus, device and function, check the Vendor-Device pair
	 * inside, and find the appropriate driver.
	 *
	 * Here, we only consider one combination: Bus #0, Device 17,
	 * Function 0, which is SATA controller on AMD SB7xx southbridge.
	 */
	int bus, device, func;
	uint32_t tag;
	uint64_t vendor_id, device_id;
	struct devtree_entry entry;
	struct pci_bus_device *pci = (struct pci_bus_device *)inst;

	/* Should be for-loop(s) in practice */
	bus = 0;
	device = 17;
	func = 0;
	/* In practice we should look up the probed list of PCI tags */
	if (probed)
		return 0;
	kprintf("Detecting PCI Bus %d Dev %d Func %d\n", bus, device, func);

	tag = make_pci_tag(bus, device, func);

	pci->r16(pci, tag, PCI_VENDOR_ID, &vendor_id);
	pci->r16(pci, tag, PCI_DEVICE_ID, &device_id);

	for (int i = 0; i < nentries; ++i) {
		if (vendor_id == modeltable[i].vendor &&
		    device_id == modeltable[i].device) {
			snprintf(entry.name, DEV_NAME_MAX, "pci:%d,%d,%d",
			    bus, device, func);
			strlcpy(entry.model, modeltable[i].model, DEV_NAME_MAX);
			strlcpy(entry.parent, pci->name, DEV_NAME_MAX);
			kpdebug("found device %s\n", entry.name);

			/* Base Address Register and IRQ setups there */
			if (modeltable[i].check(pci, tag, &entry)) {
				probed = true;
				discover_device(&entry);
			}
		}
	}

	return 0;
}

