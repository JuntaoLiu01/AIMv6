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

#ifndef _DRIVERS_IO_PORT_H
#define _DRIVERS_IO_PORT_H

#include <sys/types.h>

/*
 * A port I/O bus is a bus on which devices are accessed via IN and
 * OUT instructions, or its equivalents.
 *
 * Intel 8259A interrupt controller can be regarded as a device
 * connected to a port I/O bus.
 * On x86 systems, kernel can directly communicate to this port I/O bus.
 * On other architectures/platforms such as MIPS or PowerPC, IN and OUT
 * instructions are simulated by memory accesses at some address space,
 * in which case we can view this port I/O bus as being connected to a
 * memory bus.
 *
 * Every machine have at most one port I/O bus.
 */
__attribute__((visibility("hidden")))
extern struct bus_device early_portio_bus;

/* Connect Port I/O bus to another bus. */
void portio_bus_connect(struct bus_device *portio,
			struct bus_device *bus,
			addr_t base);
void portio_bus_init(struct bus_device *portio);

#endif /* _DRIVERS_IO_PORT_H */

