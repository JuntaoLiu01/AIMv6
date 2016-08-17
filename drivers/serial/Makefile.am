# Copyright (C) 2016 David Gao <davidgao1001@gmail.com>
# Copyright (C) 2016 Gan Quan <coin2028@hotmail.com>
#
# This program is part of AIMv6.
#
# AIMv6 is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# AIMv6 is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

include $(top_srcdir)/env.am

noinst_LTLIBRARIES = libserial-raw.la libserial.la
noinst_DATA =

SRCS = uart.c uart.h
MODULE_SRCS =

if UART_ZYNQ
MODULE_SRCS += uart-zynq.c uart-zynq.h uart-zynq-hw.h
noinst_DATA += uart-zynq.o
uart-zynq.o: uart-zynq.c uart-zynq.h uart-zynq-hw.h
endif

if UART_MSIM
MODULE_SRCS += uart-msim.c uart-msim.h
noinst_DATA += uart-msim.o
uart-msim.o: uart-msim.c uart-msim.h uart-msim-kernel.c
endif

if UART_NS16550
MODULE_SRCS += uart-ns16550.h uart-ns16550.c uart-ns16550-hw.h
noinst_DATA += uart-ns16550.o
uart-ns16550.o: uart-ns16550.c uart-ns16550.h uart-ns16550-hw.h \
	uart-ns16550-kernel.c
endif

libserial_raw_la_SOURCES = $(SRCS) $(MODULE_SRCS)
libserial_raw_la_CPPFLAGS = $(AM_CPPFLAGS_NOPIC)
libserial_raw_la_CFLAGS = $(AM_CFLAGS_NOPIC)
libserial_raw_la_CCASFLAGS = $(AM_CCASFLAGS_NOPIC)
libserial_raw_la_LDFLAGS = $(AM_LDFLAGS_NOPIC)

libserial_la_SOURCES = $(SRCS)

