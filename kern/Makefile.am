# Copyright (C) 2016 David Gao <davidgao1001@gmail.com>
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
include $(top_srcdir)/modules.am

SUBDIRS = \
	arch \
	init \
	dev \
	debug \
	file \
	lib \
	mm \
	proc \
	smp \
	syscall \
	timer

CLEANFILES = vmaim.s

noinst_PROGRAMS = vmaim.elf
noinst_DATA = vmaim.s

vmaim_elf_SOURCES =
# DO NOT CHANGE THE ORDER OF ARCHIVES HERE!
# Linkers go through input files one by one and will NOT go back.
# If an object has no used symbol, it is discarded.
# Static library order must always solve the refer graph topologically,
# that is, callers go first, and callees go last.
# NO circular reference is allowed.
KERNEL_INTERNAL_LIBS = \
	lib/libbitmap/libbitmap.la \
	lib/libkern.la

KERNEL_LIBS = \
	arch/$(ARCH)/libentry.la \
	init/libinit.la \
	proc/libproc.la \
	arch/$(ARCH)/lib$(ARCH)trap.la \
	timer/libtimer.la \
	proc/sched/libsched.la \
	$(top_builddir)/fs/libfs.la \
	file/libfile.la \
	dev/libdev.la \
	mm/libmm.la \
	smp/libsmp.la \
	arch/$(ARCH)/lib$(ARCH).la \
	syscall/libsyscall.la \
	arch/$(ARCH)/lib$(ARCH)syscall.la \
	$(top_builddir)/drivers/libdrivers.la \
	$(MODULES) \
	$(KERNEL_INTERNAL_LIBS) \
	$(top_builddir)/lib/libc/arch/$(ARCH)/libc-$(ARCH).la \
	$(top_builddir)/lib/libc/libc.la \
	debug/libdebug.la
vmaim_elf_LDADD = $(KERNEL_LIBS)
vmaim_elf_DEPENDENCIES = arch/$(ARCH)/vmaim.lds $(KERNEL_LIBS)
vmaim_elf_LDFLAGS = $(AM_LDFLAGS) -T arch/$(ARCH)/vmaim.lds

