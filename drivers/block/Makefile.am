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

noinst_LTLIBRARIES = libblock-raw.la libblock.la
noinst_DATA =

libblock_raw_la_SOURCES =
libblock_raw_la_CPPFLAGS = $(AM_CPPFLAGS_NOPIC) -DRAW
libblock_raw_la_CFLAGS = $(AM_CFLAGS_NOPIC)
libblock_raw_la_CCASFLAGS = $(AM_CCASFLAGS_NOPIC)
libblock_raw_la_LDFLAGS = $(AM_LDFLAGS_NOPIC)

libblock_la_SOURCES =

if BLOCK_MSIM
libblock_raw_la_SOURCES += msim-ddisk.c msim-ddisk.h
libblock_raw_la_DEPENDENCIES = $(libblock_raw_la_SOURCES) msim-ddisk-raw.c
noinst_DATA += msim-ddisk.o
msim-ddisk.o: msim-ddisk.c msim-ddisk.h msim-ddisk-kernel.c
endif

