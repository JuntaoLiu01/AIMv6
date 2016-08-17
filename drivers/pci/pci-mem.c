
#include <aim/device.h>
#include <sys/types.h>

static int
__readcfg32(struct bus_device *pci, addr_t tag, addr_t reg, uint64_t *val)
{
	struct bus_device *bus = pci->bus;
	bus_read_fp bus_read32 = bus->bus_driver.get_read_fp(bus, 32);

	bus_read32(bus, pci->base, tag + reg, val);
	return 0;
}

static int
__readcfg16(struct bus_device *pci, addr_t tag, addr_t reg, uint64_t *val)
{
	__readcfg32(pci, tag, reg & ~3, val);
	*val = (uint16_t)(*val >> ((reg & 3) * 8));
	return 0;
}

static int
__readcfg8(struct bus_device *pci, addr_t tag, addr_t reg, uint64_t *val)
{
	__readcfg32(pci, tag, reg & ~3, val);
	*val = (uint8_t)(*val >> ((reg & 3) * 8));
	return 0;
}

static int
__writecfg32(struct bus_device *pci, addr_t tag, addr_t reg, uint64_t val)
{
	struct bus_device *bus = pci->bus;
	bus_write_fp bus_write32 = bus->bus_driver.get_write_fp(bus, 32);
	bus_write32(bus, pci->base, tag + reg, val);
	return 0;
}

static int
__writecfg16(struct bus_device *pci, addr_t tag, addr_t reg, uint64_t val)
{
	uint64_t res;
	__readcfg32(pci, tag, reg & ~3, &res);
	res &= ~(0xffff << ((reg & 3) * 8));
	res |= ((uint16_t)val << ((reg & 3) * 8));
	__writecfg32(pci, tag, reg & ~3, res);
	return 0;
}

static int
__writecfg8(struct bus_device *pci, addr_t tag, addr_t reg, uint64_t val)
{
	uint64_t res;
	__readcfg32(pci, tag, reg & ~3, &res);
	res &= ~(0xff << ((reg & 3) * 8));
	res |= (val << ((reg & 3) * 8));
	__writecfg32(pci, tag, reg & ~3, res);
	return 0;
}

#include "drivers/pci/pci.c"

