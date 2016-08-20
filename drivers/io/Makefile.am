# Copyright (C) 2016 David Gao <davidgao1001@gmail.com>
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

noinst_LTLIBRARIES = libio-raw.la libio.la
noinst_DATA =

SRCS = 

if IO_MEM
SRCS += io-mem.c io-mem.h
endif

if IO_PORT
SRCS += io-port.c io-port.h
endif

if BUS_MAPPER
noinst_DATA += bus-mapper.o
bus-mapper.o: bus-mapper.c
endif

libio_raw_la_SOURCES = $(SRCS)
libio_raw_la_CPPFLAGS = $(AM_CPPFLAGS_NOPIC)
libio_raw_la_CFLAGS = $(AM_CFLAGS_NOPIC)
libio_raw_la_CCASFLAGS = $(AM_CCASFLAGS_NOPIC)
libio_raw_la_LDFLAGS = $(AM_LDFLAGS_NOPIC)

libio_la_SOURCES = $(SRCS)

