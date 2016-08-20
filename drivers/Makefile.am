# Copyright (C) 2016 David Gao <davidgao1001@gmail.com>
# Copyright (C) 2016 Gan Quan <coin2028@hotmail.com>
#
# This file is part of AIMv6.
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

SUBDIRS = io \
	  interrupt-controller \
	  block \
	  pci \
	  sd \
	  serial \
	  timer \
	  hd \
	  tty \
	  common \
	  console \
	  ata

noinst_LTLIBRARIES = libdrivers-raw.la libdrivers.la

libdrivers_raw_la_SOURCES =
libdrivers_raw_la_LIBADD = \
	$(builddir)/io/libio-raw.la \
	$(builddir)/block/libblock-raw.la \
	$(builddir)/sd/libsd-raw.la \
	$(builddir)/serial/libserial-raw.la \
	$(builddir)/timer/libtimer-raw.la
libdrivers_raw_la_CPPFLAGS = $(AM_CPPFLAGS_NOPIC)
libdrivers_raw_la_CFLAGS = $(AM_CFLAGS_NOPIC)
libdrivers_raw_la_CCASFLAGS = $(AM_CCASFLAGS_NOPIC)
libdrivers_raw_la_LDFLAGS = $(AM_LDFLAGS_NOPIC)

libdrivers_la_SOURCES =
libdrivers_la_LIBADD = \
	$(builddir)/io/libio.la \
	$(builddir)/serial/libserial.la \
	$(builddir)/hd/libhd.la \
	$(builddir)/pci/libpci.la \
	$(builddir)/common/libcommon.la \
	$(builddir)/tty/libtty.la \
	$(builddir)/console/libcons.la \
	$(builddir)/timer/libtimer.la
