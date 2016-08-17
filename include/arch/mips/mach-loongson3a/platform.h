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

#ifndef _PLATFORM_H
#define _PLATFORM_H

#include <io.h>

/* inb(x) = *(LOONGSON3A_PORTIO_BASE + x) */
#define LOONGSON3A_PORTIO_BASE		0x0efdfc000000
#define LOONGSON3A_UART_BASE		0x1fe001e0

/* for UART */
#define UART_BASE	LOONGSON3A_UART_BASE
#define UART_FREQ	2073600

/*
 * Loongson 3A address space router 
 */
#define LOONGSON3A_CORE_WIN_BASE	0x3ff02000
#define LOONGSON3A_CPU_WIN_BASE		0x3ff00000
#define LOONGSON3A_CPU_WINx_BASE(x)	(LOONGSON3A_CPU_WIN_BASE + (x) * 8)
#define LOONGSON3A_CPU_WINx_MASK(x)	(LOONGSON3A_CPU_WINx_BASE(x) + 0x40)
#define LOONGSON3A_CPU_WINx_MMAP(x)	(LOONGSON3A_CPU_WINx_BASE(x) + 0x80)
#define LOONGSON3A_COREx_WINy_BASE(x, y) \
	(LOONGSON3A_CORE_WIN_BASE + (x) * 0x100 + (y) * 0x8)
#define LOONGSON3A_COREx_WINy_MASK(x, y) \
	(LOONGSON3A_COREx_WINy_BASE(x, y) + 0x40)
#define LOONGSON3A_COREx_WINy_MMAP(x, y) \
	(LOONGSON3A_COREx_WINy_BASE(x, y) + 0x80)
#define LOONGSON3A_CORE_WINS		8
/* Address spaces */
#define LOONGSON3A_NUMA_MASK		0xf00000000000
#define LOONGSON3A_HT0_BASE		0x0c0000000000
#define LOONGSON3A_HT1_BASE		0x0e0000000000
#define LOONGSON3A_HT1_CFG_BASE		0x0efdfb000000
#define LOONGSON3A_HT1_PORTIO_BASE	0x0efdfc000000
#define LOONGSON3A_HT1_PCICFG_BASE	0x0efdfe000000
/* 32-bit compatibility spaces */
#define LOONGSON3A_HTIO32		0x18000000
#define LOONGSON3A_HTMEM32		0x1e000000
/* Address space availability */
#define LOONGSON3A_MMAP_ENABLE		0x80
#define LOONGSON3A_MMAP_DATA		0x20
#define LOONGSON3A_MMAP_IFETCH		0x10
#define LOONGSON3A_MMAP_AVAILABLE \
	(LOONGSON3A_MMAP_ENABLE | \
	 LOONGSON3A_MMAP_DATA | \
	 LOONGSON3A_MMAP_IFETCH)
#define LOONGSON3A_MMAP_AVAILABLE_MASK	0xf0
#define LOONGSON3A_MMAP_PORTMASK	0x7

#define LOONGSON3A_CORE_IPI_BASE	0x3ff01000
#define LOONGSON3A_COREx_IPI_BASE(x)	(LOONGSON3A_CORE_IPI_BASE + ((x) << 8))
#define LOONGSON3A_COREx_IPI_STATUS(x)	(LOONGSON3A_COREx_IPI_BASE(x) + 0x0)
#define LOONGSON3A_COREx_IPI_ENABLE(x)	(LOONGSON3A_COREx_IPI_BASE(x) + 0x4)
#define LOONGSON3A_COREx_IPI_SET(x)	(LOONGSON3A_COREx_IPI_BASE(x) + 0x8)
#define LOONGSON3A_COREx_IPI_CLEAR(x)	(LOONGSON3A_COREx_IPI_BASE(x) + 0xc)
#define LOONGSON3A_COREx_IPI_MB0(x)	(LOONGSON3A_COREx_IPI_BASE(x) + 0x20)
#define LOONGSON3A_COREx_IPI_MB1(x)	(LOONGSON3A_COREx_IPI_BASE(x) + 0x28)
#define LOONGSON3A_COREx_IPI_MB2(x)	(LOONGSON3A_COREx_IPI_BASE(x) + 0x30)
#define LOONGSON3A_COREx_IPI_MB3(x)	(LOONGSON3A_COREx_IPI_BASE(x) + 0x38)

/* Interrupt router registers and values */
#define LOONGSON3A_INTROUTER_BASE	0x3ff01400
#define LOONGSON3A_INTROUTER_LPC	0x0a
#define LOONGSON3A_INTROUTER_HT1(x)	(0x18 + (x))
#define LOONGSON3A_INTROUTER_ISR	0x20
#define LOONGSON3A_INTROUTER_INTEN	0x24
#define LOONGSON3A_INTROUTER_INTENSET	0x28
#define LOONGSON3A_INTROUTER_INTENCLR	0x2c
/* IP 2-5 */
#define LOONGSON3A_INTROUTER_IP(x)	(1 << ((x) + 2))
#define LOONGSON3A_INTROUTER_CORE(x)	(1 << (x))

/*
 * HyperTransport registers
 * TODO: make this into driver framework.
 */
#define LOONGSON3A_HT_ISR0		0x80	/* IRQ 0-31 status */
#define LOONGSON3A_HT_IEN0		0xa0	/* IRQ 0-31 enable */

/* Intel 8259 configurations */
#define I8259_IRQ_BASE			0

#endif
