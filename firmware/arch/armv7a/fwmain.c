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

/* from kernel */
#include <sys/types.h>
#include <sys/param.h>	/* SECTOR_SIZE */
#include <util.h>
/* from libc */
#include <libc/string.h>
/* from drivers */
#include <drivers/io/io-mem.h>
#include <drivers/serial/uart.h>
#include <drivers/serial/uart-zynq.h>
#include <drivers/timer/timer.h>
#include <drivers/timer/timer-a9.h>
#include <drivers/sd/sd.h>
#include <drivers/sd/sd-zynq.h>

char fw_stack[4096];

void delay(uint32_t s)
{
	uint64_t time, time1;
	time = timer_read();
	time += gt_get_tps() * s;
	do {
		time1 = timer_read();
	} while (time1 < time);
}

void udelay(uint32_t us)
{
	uint64_t time, time1;
	time = timer_read();
	time += gt_get_tpus() * us;
	do {
		time1 = timer_read();
	} while (time1 < time);
}

void fwpanic(const char *msg)
{
	uart_puts(msg);
	while (1);
		/* nothing */;
}

void readdisk(size_t sector, size_t offset, void *buf, size_t len)
{
	volatile unsigned char sector_buf[SECTOR_SIZE];
	size_t l = 0;

	sector += offset / SECTOR_SIZE;
	offset %= SECTOR_SIZE;

	for (; len > 0; len -= l) {
		l = min2(len, SECTOR_SIZE - offset);
		if (sd_read((uint32_t)sector_buf, 1, sector) != 0)
			fwpanic("read disk error\n");
		memcpy(buf, (void *)&sector_buf[offset], l);
		offset = 0;
		buf += l;
		++sector;
	}
}

__attribute__ ((noreturn))
void fw_main(void)
{
	int ret;
	volatile uint8_t *mbr = (void *)0x1FE00000; /* THIS IS NOT A NULL! */
	void (*mbr_entry)() = (void *)mbr;

	io_mem_init(&early_memory_bus);

	/* Wait for UART fifo to flush */
	delay(1);
	
	/* Initialize and enable UART */
	uart_init();
	uart_enable();
	uart_puts("FW: Hello!\n");

	/* Initialize SDHCI interface */
	sd_init();
	uart_puts("FW: SD Controller initialized.\n");

	/* Initialize SD card */
	ret = sd_init_card();
	if (ret == 0)
		uart_puts("FW: SD Card initialized.\n");
	else if (ret == 1)
		uart_puts("FW: SDHC Card initialized.\n");
	else {
		uart_puts("FW: Card initialization failed.\n");
		goto spin;
	}

	/*
	 * We don't turn on SCU now. The kernel should do this.
	 * This CANNOT be done here. DDR in 0x0 to 0xFFFFF is only accessible
	 * to processor cores, not the DMA controller.
	 * See Xilinx UG585, Table 4-1 for details.
	 */

	/* Read MBR */
	ret = sd_read((uint32_t)mbr, 1, 0);
	if (ret == 0) uart_puts("FW: Card read OK.\n");
	else {
		uart_puts("FW: Card read failed.\n");
		goto spin;
	}

	/* Check MBR */
	if (mbr[510] == 0x55 && mbr[511] == 0xAA) {
		uart_puts("FW: MBR valid.\n");
		mbr_entry();
	} else uart_puts("FW: MBR not valid.\n");

spin:
	while (1);
}
