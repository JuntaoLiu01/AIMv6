
#include <drivers/pci/pci.h>
#include <sys/types.h>
#include <panic.h>
#include <aim/initcalls.h>

static bool __check(struct pci_bus_device *pci, uint32_t tag,
    struct devtree_entry *entry)
{
	uint64_t misc;
	uint64_t tmp;

	entry->nregs = 6;
	for (int i = 0; i < 6; ++i) {
		pci->r32(pci, tag, PCI_BASE_ADDRESS_0 + 4 * i, &entry->regs[i]);
		kpdebug("BAR%d: %08x\n", i, (unsigned long)entry->regs[i]);
	}
	pci->r8(pci, tag, PCI_INTERRUPT_LINE, &tmp);
	entry->irq = (uint8_t)tmp;
	kpdebug("IRQ: %d\n", entry->irq);

	/* Fix up subclass code and operating mode */
	pci->r32(pci, tag, 0x40, &misc);
	pci->w32(pci, tag, 0x40, misc | 0x1);	/* enable subclass write */
	pci->w8(pci, tag, PCI_CLASS_PROG, 0x8f);
	pci->w8(pci, tag, PCI_SUBCLASS_DEVICE, 0x01);
	pci->r32(pci, tag, 0x40, &misc);
	pci->w32(pci, tag, 0x40, misc & ~0x1);	/* disable subclass write */

	pci->r8(pci, tag, PCI_CLASS_PROG, &tmp);
	kpdebug("Rechecking operating mode %02x\n", (uint8_t)tmp);
	pci->r8(pci, tag, PCI_SUBCLASS_DEVICE, &tmp);
	kpdebug("Rechecking subclass %02x\n", (uint8_t)tmp);

	return true;
}

static int __pci_driver_init(void)
{
	register_pci(0x1002, 0x4390, "ide", __check);
	return 0;
}
INITCALL_DRIVER(__pci_driver_init);

