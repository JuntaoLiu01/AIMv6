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

SUBDIRS = $(PREFIXED_MACH) $(PREFIXED_CORE)
DIST_SUBDIRS = \
	core-cortexa9 \
	mach-zynq

noinst_LTLIBRARIES = \
	libentry.la \
	libarmv7a.la \
	libarmv7atrap.la \
	libarmv7asyscall.la
noinst_DATA = vmaim.lds

CLEANFILES = vmaim.lds

libentry_la_SOURCES = \
	entry.S

libarmv7a_la_SOURCES = \
	arch_init.c \
	mm.c \
	jump.c \
	proc.c \
	smp.c \
	sync.c \
	switch.S
libarmv7a_la_LIBADD = \
	$(PREFIXED_MACH)/lib$(MACH).la \
	$(PREFIXED_CORE)/lib$(CORE).la

# libsyscall.la depends on this, containing system call arguments retrieval
# etc.
libarmv7asyscall_la_SOURCES = syscall.c

# depends on libtimer.la (handle_timer_interrupt)
libarmv7atrap_la_SOURCES = \
	trap.c \
	traptest.c \
	vector.S

vmaim.lds: vmaim.lds.S
	$(COMPILE) -E -P $^ -o $@

