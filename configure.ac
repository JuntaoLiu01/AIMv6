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

# autoconf requirements
AC_PREREQ([2.69])

AC_INIT([AIMv6], [0.1], [davidgao1001@gmail.com])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

AC_CONFIG_MACRO_DIR([./m4])

# Disable the default -O2 which overrides the -Os for the bootloader
: ${CFLAGS="-g"}

AC_PROG_CC
# TODO: put these into acinclude.m4
AIM_PROG_CC_C11
AS_CASE([$aim_prog_cc_c11],
	[no], [AC_MSG_ERROR([Compiler does not support C11.])]
)
AC_C_CONST
AC_C_RESTRICT
AC_C_VOLATILE
AC_C_INLINE
AC_C_TYPEOF
AIM_GCC_FUNC_ATTRIBUTE([noreturn])
AIM_GCC_FUNC_ATTRIBUTE([weak])
AIM_GCC_FUNC_ATTRIBUTE([noinline])
AIM_GCC_FUNC_ATTRIBUTE([visibility])
AIM_GCC_VAR_ATTRIBUTE([used])
AIM_REQUIRE_COMPILE_FLAG([-fplan9-extensions])

# Builtin functions - not required
AX_GCC_BUILTIN(__builtin_popcount)
AX_GCC_BUILTIN(__builtin_popcountl)
AX_GCC_BUILTIN(__builtin_popcountll)

# Builtin functions - required since we are lazy
AX_GCC_BUILTIN(__builtin_bswap32)
AX_GCC_BUILTIN(__builtin_bswap64)

AC_SUBST([AIM_CFLAGS], [$aim_cflags])

AM_PROG_AS
AM_PROG_AR
AC_CHECK_TOOL([OBJDUMP], [objdump])
AC_CHECK_TOOL([OBJCOPY], [objcopy])

LT_INIT

# target machine
AIM_ARG_VAR([ARCH], [target architecture])
AIM_SUPPORT_ARCH([armv7a i386 mips32 mips64])

AIM_ARG_VAR([MACH], [target machine])
AC_SUBST([PREFIXED_MACH], [mach-$MACH])
AIM_SUPPORT_MACH([msim loongson3a zynq pc generic unknown])

# Default configurations
#
# When specifying whether and how a module is to be built, three values are
# possible:
# - no		The module will not be built. Headers will not be removed.
# - module	The module will be built into a seperate loadable object, and
#         	will not be accessible during early booting stages. Executables
#		outside the kernel, like the firmware and bootloader, will not
#		build nor access the module.
# - yes		The module will be built into the kernel, and initialized
#		according to defined entry(s). It cannot be unloaded from the
#		running kernel.
# If a module is compiled into the kernel and contains some functionality that
# exactly one instance is accessed during early booting stages, a "primary"
# variable can be set to its name. When this is done, the firmware and the
# bootloader will also access it.
# It is usually meaningless to compile a secondary implementation into the
# firmware or the bootloader, so implementations should check corresponding
# macros.

# Debug and test options
AIM_ARG_ENABLE_FEATURE([debug-output], [DEBUG_OUTPUT], [debug output on kernel console], [yes])
AIM_ARG_ENABLE_FEATURE([test-framework], [TEST_FRAMEWORK], [in-kernel test framework], [yes])

# TODO: Loongson 3A needs to change kstacksize or it will crash (because of 16K pages)
AS_VAR_SET([__with_kstacksize], [8192])

AS_CASE([$ARCH],
	[arm*], [
		AS_VAR_SET([__enable_io_mem], [yes])
		AS_VAR_SET([__enable_io_port], [no])
		AS_VAR_SET([__enable_io_mem_root], [yes])
		AS_VAR_SET([__enable_io_port_root], [no])
		AS_VAR_SET([__enable_bus_mapper], [yes])
	],
	[mips32], [
		AC_DEFINE(USE_MIPS32, [], [MIPS32 ABI and instruction set])
		AS_VAR_SET([__enable_io_mem], [yes])
		AS_VAR_SET([__enable_io_port], [no])
		AS_VAR_SET([__enable_io_mem_root], [yes])
		AS_VAR_SET([__enable_io_port_root], [no])
	],
	[mips64], [
		AC_DEFINE(USE_MIPS64, [], [MIPS64r2 ABI and instruction set])
		AS_VAR_SET([__enable_io_mem], [yes])
		AS_VAR_SET([__enable_io_port], [no])
		AS_VAR_SET([__enable_io_mem_root], [yes])
		AS_VAR_SET([__enable_io_port_root], [no])
	],
	[i386], [
		AS_VAR_SET([__enable_io_mem], [yes])
		AS_VAR_SET([__enable_io_port], [yes])
		AS_VAR_SET([__enable_io_mem], [yes])
		AS_VAR_SET([__enable_io_port], [yes])
		AS_VAR_SET([__enable_uart_ns16550], [yes])
		AS_VAR_SET([__with_primary_console], [uart-ns16550])
	]
)

AS_CASE([$MACH],
	[zynq], [
		AC_SUBST([CORE], [cortexa9])
		AC_SUBST([PREFIXED_CORE], [core-cortexa9])
		AS_VAR_SET([__with_ram_physbase], [0x00100000])
		AS_VAR_SET([__with_kern_start], [0x00100000])

		AS_VAR_SET([__with_fwstack_order], [12])
		AS_VAR_SET([__enable_uart_zynq], [yes])
		AS_VAR_SET([__enable_sd_zynq], [yes])
		AS_VAR_SET([__enable_timer_a9], [yes])

		AS_VAR_SET([__with_primary_console], [UART_ZYNQ])
		AS_VAR_SET([__with_primary_storage], [SD_ZYNQ])
		AS_VAR_SET([__with_primary_timing], [TIMER_A9])
	],
	[msim], [
		AS_VAR_SET([__with_fwstack_order], [12])
		AS_VAR_SET([__enable_uart_msim], [yes])
		AS_VAR_SET([__enable_block_msim], [yes])

		AS_VAR_SET([__with_primary_console], [UART_MSIM])
		AS_VAR_SET([__with_primary_storage], [BLOCK_MSIM])

		AS_VAR_SET([__with_msim_kbd_pa], [0x1f000000])
		AS_VAR_SET([__with_msim_lp_pa], [0x1f000010])
		AS_VAR_SET([__with_msim_disk_pa], [0x1f000020])
		AS_VAR_SET([__with_msim_order_pa], [0x1f000100])
		AS_VAR_SET([__with_msim_mb_base], [0x1f000200])
		AS_VAR_SET([__with_msim_mb_order], [4])
		AS_VAR_SET([__with_msim_rtc_pa], [0x1f000080])
		AS_VAR_SET([__with_msim_firmware_bin], ["firmware.bin"])
		AS_VAR_SET([__with_msim_disk_img], ["disk.img"])
		AS_VAR_SET([__with_cpus], [4])
		AS_VAR_SET([__with_max_cpus], [4])
		AS_VAR_SET([__with_highram_base], [0x80000000])
		AS_VAR_SET([__with_highram_size], [0x00000000])

		AC_DEFINE([CPU_FREQ], [100000000], [CPU frequency])
	], 
	[pc], [
		AS_VAR_SET([__with_fwstack_order], [12])
	],
	[loongson3a], [
		AS_VAR_SET([__with_cpus], [4])
		AS_VAR_SET([__with_max_cpus], [4])
		AS_VAR_SET([__enable_io_port], [yes])
		AS_VAR_SET([__enable_uart_ns16550], [yes])
		AS_VAR_SET([__enable_loongson3a_ram_detection], [yes])
		AS_VAR_SET([__enable_pagesize_16k], [yes])
		AS_VAR_SET([__with_primary_console], [UART_NS16550])
		AC_DEFINE([CPU_FREQ], [750000000], [CPU frequency])
	],
	[generic], [],
	[unknown], [],
	[AC_MSG_ERROR([No machine specified])]
)

# msim
AIM_ARG_WITH([msim-kbd-pa], [MSIM_KBD_PHYSADDR],
	[MSIM keyboard physical address])
AIM_ARG_WITH([msim-lp-pa], [MSIM_LP_PHYSADDR],
	[MSIM printer physical address])
AIM_ARG_WITH([msim-disk-pa], [MSIM_DISK_PHYSADDR],
	[MSIM disk physical address])
AIM_ARG_WITH([msim-order-pa], [MSIM_ORDER_PHYSADDR],
	[MSIM IPC device physical address])
AIM_ARG_WITH([msim-mb-base], [MSIM_ORDER_MAILBOX_BASE],
	[MSIM IPC device mailbox base address])
AIM_ARG_WITH([msim-mb-order], [MSIM_ORDER_MAILBOX_ORDER],
	[order of MSIM IPC device mailbox (size=1<<order)])
AIM_ARG_WITH([msim-rtc-pa], [MSIM_RTC_PHYSADDR],
	[MSIM real time clock physical address])
AIM_ARG_WITH([msim-firmware-bin], [MSIM_FIRMWARE_BIN],
	[MSIM firmware binary file name])
AIM_ARG_WITH([msim-disk-img], [MSIM_DISK_IMG],
	[MSIM disk image file name])
AIM_ARG_WITH([highram-base], [HIGHRAM_BASE],
	[MIPS high RAM base address])
AIM_ARG_WITH([highram-size], [HIGHRAM_SIZE],
	[MIPS high RAM size])

# Loongson 3A
AIM_ARG_ENABLE_FEATURE([loongson3a-ram-detection], [LOONGSON3A_RAM_DETECTION],
	[RAM space detection on Loongson 3A port (RECOMMENDED)])
AIM_ARG_ENABLE_FEATURE([pagesize-16k], [PAGESIZE_16K],
	[16K pages (for Loongson 3A)])
AIM_ARG_ENABLE_FEATURE([ide-bmdma], [IDE_BMDMA],
	[PCI IDE Bus-mastering DMA])

# target details
AIM_ARG_WITH([ram-physbase], [RAM_PHYSBASE], [physical memory start point],
	[0x00000000])

AIM_ARG_WITH([kern-start], [KERN_START], [kernel loading physical address],
	[0x00000000])

AIM_ARG_WITH([mem-size], [MEM_SIZE], [amount of memory installed], [DETECT])

AIM_ARG_WITH([cpus], [NR_CPUS], [number of cores], [DETECT])
AIM_ARG_WITH([max-cpus], [MAX_CPUS], [maximum number of cores], [NR_CPUS])

# Kernel hyperparameters
# TODO: for addresses, make two default configurations for 32-bit and 64-bit
# respectively.
AIM_ARG_WITH([kern-base], [KERN_BASE], [start of kernel address], [0x80000000])

AIM_ARG_WITH([kmmap-base], [KMMAP_BASE], [start of kernel dynamic mapping],
	[0xf0000000])

AIM_ARG_WITH([reserved-base], [RESERVED_BASE], [start of reserved high address],
	[0xfffff000])

AIM_ARG_WITH([usertop], [USERTOP],
	[top of user program address space], [0x7ff00000])

AIM_ARG_WITH([ustacksize], [USTACKSIZE],
	[size of user stack], [32768])

AIM_ARG_WITH([kstacksize], [KSTACKSIZE],
	[size of kernel stack])

AIM_ARG_WITH([root-partition-id], [ROOT_PARTITION_ID],
	[root partition number], [4])

AIM_ARG_WITH([max-processes], [MAX_PROCESSES], [maximum number of processes],
	[65536])

AIM_ARG_WITH([pipe-size], [PIPE_SIZE], [pipe size], [16384])

AIM_ARG_WITH([fwstack-order], [FWSTACKORDER],
	[order of firmware stack per CPU (size=1<<order)])

AIM_ARG_WITH([timer-freq], [TIMER_FREQ],
	[timer interrupt frequency], [100])

# Drivers
AIM_ARG_ENABLE([io-mem], [IO_MEM], [memory-mapped IO driver])
AIM_ARG_ENABLE([io-port], [IO_PORT], [port IO driver])
AIM_ARG_ENABLE([uart-zynq], [UART_ZYNQ], [zynq UART driver])
AIM_ARG_ENABLE([uart-msim], [UART_MSIM], [MSIM UART driver])
AIM_ARG_ENABLE([uart-ns16550], [UART_NS16550], [NS16550 UART driver])
AIM_ARG_ENABLE([block-msim], [BLOCK_MSIM], [MSIM block device driver])
AIM_ARG_ENABLE([sd-zynq], [SD_ZYNQ], [zynq SD driver])
AIM_ARG_ENABLE([timer-a9], [TIMER_A9], [Cortex-A9 MPCore timer driver])
AIM_ARG_ENABLE([bus-mapper], [BUS_MAPPER], [bus mapper driver])
AIM_ARG_ENABLE_FEATURE([io-mem-root], [IO_MEM_ROOT],
	[memory-mapped IO bus as device tree root])
AIM_ARG_ENABLE_FEATURE([io-port-root], [IO_PORT_ROOT],
	[port IO bus as device tree root])

# Debug
AIM_ARG_ENABLE_FEATURE([debug], [DEBUG], [Debugging (kpdebug)])

AM_CONDITIONAL([IO_MEM], [test x$enable_io_mem = xyes])
AM_CONDITIONAL([IO_PORT], [test x$enable_io_port = xyes])

# Primary Driver Selection
AIM_ARG_WITH([primary-console], [PRIMARY_CONSOLE], [primary console])
AIM_ARG_WITH([primary-storage], [PRIMARY_STORAGE], [primary storage])
AIM_ARG_WITH([primary-timing], [PRIMARY_TIMING], [primary timing])

AS_CASE([$with_primary_console],
	[UART_ZYNQ],[
		AC_DEFINE_UNQUOTED([CONSOLE_NEED_CR], [], [Primary console needs CR])
	]
)

# Algorithms
# MUST have unique names.
# Two kinds of algorithm roles exist:
# - Hard-wired: Provides fundamental functionalities. Kernel hard-wire calls
#   to their init functions.
# - Modular: Provides higher-level functionalities. Must be linked within
#   the kernel but initialized via initcalls. Most of these initcalls are of
#   early stage.
# Algorithms register their interface structure when initialization routines
# are called. This routine SHOULD NOT really initialize the algorithm.
# The "with" syntax is used so that at most ONE algorithm is selected for each
# role.
# First fit is named "ff" as page allocator, "flff" as simple allocator.
AIM_ARG_WITH([simple-allocator], [SIMPLE_ALLOCATOR], [non-caching memory object allocator], [FLFF])
AM_CONDITIONAL([ALGO_FLFF], [test x$with_simple_allocator = xFLFF])

AIM_ARG_WITH([page-allocator], [PAGE_ALLOCATOR], [page allocator], [FF])
AM_CONDITIONAL([ALGO_FF], [test x$with_page_allocator = xFF])

AIM_ARG_WITH([caching-allocator], [CACHING_ALLOCATOR], [caching allocator], [SLAB])
AM_CONDITIONAL([ALGO_SLAB], [test x$with_caching_allocator = xSLAB])

AIM_ARG_WITH([dev-index], [DEVICE_INDEX], [device index], [DEVLIST])
AM_CONDITIONAL([ALGO_DEVLIST], [test x$with_dev_index = xDEVLIST])

AIM_ARG_WITH([kmmap-keeper], [KMMAP_KEEPER], [kmmap keeper], [KMLIST])
AM_CONDITIONAL([ALGO_KMLIST], [test x$with_kmmap_keeper = xKMLIST])

# per-target configuration
AS_CASE([$MACH],
	[msim], [
		 AIM_SUBST_MULTILINE([MSIM_CONF_CPULIST],
		 	`for (( i=0; $i<$with_cpus; i=$i+1 )); do \
			echo add dcpu cpu$i; \
			done`
		 )
		 AC_SUBST(MSIM_CONF_FIRMWARE_BIN, $with_msim_firmware_bin)
		 AC_SUBST(MSIM_CONF_KBDPA,
			  `printf 0x%08x $with_msim_kbd_pa`)
		 AC_SUBST(MSIM_CONF_LPPA,
			  `printf 0x%08x $with_msim_lp_pa`)
		 AC_SUBST(MSIM_CONF_DISKPA,
			  `printf 0x%08x $with_msim_disk_pa`)
		 AC_SUBST(MSIM_CONF_DISK_IMAGE, $with_msim_disk_img)
		 AC_SUBST(MSIM_CONF_ORDERPA,
			  `printf 0x%08x $with_msim_order_pa`)
		 AIM_SUBST_MULTILINE(MSIM_CONF_MBLIST,
			  `for (( i=0; $i<$with_cpus; i=$i+1 )); do \
			   pa=$(( $with_msim_mb_base \
				+ ( $i << $with_msim_mb_order ) )); \
			   printf "add rwm mb%d 0x%08x\n" $i $pa; \
			   printf "mb%d generic %d\n" $i \
			   $(( 1 << $with_msim_mb_order )); \
			   done`)
		 AC_SUBST(MSIM_CONF_RTCPA,
			  `printf 0x%08x $with_msim_rtc_pa`)
		 AC_SUBST(MSIM_CONF_HIGHRAM_BASE,
			  `if (( $with_highram_size != 0 )); then \
				  printf "add rwm highram 0x%08x" \
				  $with_highram_base; else \
				  echo "";\
			  fi`)
		 AC_SUBST(MSIM_CONF_HIGHRAM_SIZE,
			  `if (( $with_highram_size != 0 )); then \
				  printf "highram generic 0x%08x" \
				  $with_highram_size; else \
				  echo "";\
			  fi`)
])

# Skip firmware
AM_CONDITIONAL([NO_FIRMWARE],
	[test x$MACH = xloongson3a -o x$ARCH = xi386])
# Skip bootloader
AM_CONDITIONAL([NO_BOOTLOADER],
	[test x$MACH = xloongson3a])

# change mips32 and mips64 to mips afterwards, because the CFLAGS are
# different, but the source files are in the same directory.
if test x$ARCH = xmips32 -o x$ARCH = xmips64; then
	ARCH=mips
fi

AC_CONFIG_HEADERS([config.h])

AC_CONFIG_FILES([
	msim.conf
	Makefile
	include/Makefile
	include/aim/Makefile
	include/asm-generic/Makefile
	boot/Makefile
	boot/arch/Makefile
	boot/arch/armv7a/Makefile
	boot/arch/i386/Makefile
	boot/arch/mips/Makefile
	boot/arch/mips/mach-msim/Makefile
	doc/Makefile
	drivers/Makefile
	drivers/ata/Makefile
	drivers/block/Makefile
	drivers/common/Makefile
	drivers/console/Makefile
	drivers/hd/Makefile
	drivers/interrupt-controller/Makefile
	drivers/io/Makefile
	drivers/pci/Makefile
	drivers/sd/Makefile
	drivers/serial/Makefile
	drivers/timer/Makefile
	drivers/tty/Makefile
	firmware/Makefile
	firmware/arch/Makefile
	firmware/arch/armv7a/Makefile
	firmware/arch/i386/Makefile
	firmware/arch/mips/Makefile
	firmware/arch/mips/mach-msim/Makefile
	fs/Makefile
	fs/ufs/Makefile
	fs/ufs/ext2fs/Makefile
	kern/Makefile
	kern/arch/Makefile
	kern/arch/armv7a/Makefile
	kern/arch/armv7a/core-cortexa9/Makefile
	kern/arch/armv7a/mach-zynq/Makefile
	kern/arch/i386/Makefile
	kern/arch/mips/Makefile
	kern/arch/mips/mach-generic/Makefile
	kern/arch/mips/mach-msim/Makefile
	kern/arch/mips/mach-loongson3a/Makefile
	kern/init/Makefile
	kern/lib/Makefile
	kern/lib/libbitmap/Makefile
	kern/dev/Makefile
	kern/dev/index/Makefile
	kern/debug/Makefile
	kern/file/Makefile
	kern/mm/Makefile
	kern/mm/kmmap/Makefile
	kern/mm/pmm/Makefile
	kern/mm/vmm/Makefile
	kern/proc/Makefile
	kern/proc/sched/Makefile
	kern/smp/Makefile
	kern/syscall/Makefile
	kern/timer/Makefile
	lib/Makefile
	lib/csu/Makefile
	lib/libc/Makefile
	lib/libc/arch/Makefile
	lib/libc/arch/armv7a/Makefile
	lib/libc/arch/i386/Makefile
	lib/libc/arch/mips/Makefile
	tools/Makefile
	user/Makefile
	user/sbin/Makefile
	user/sbin/init/Makefile
])
AC_OUTPUT

AC_MSG_NOTICE([
=========================
  Configuration Summary
=========================

General
--------
  version:	${VERSION}
  build:	${build}
  host:		${host}
  compiler:	${CC}
  CPPFLAGS:	${CPPFLAGS}
  CFLAGS:	${CFLAGS}
  LDFLAGS:	${LDFLAGS}

* Flags are overridden by architecture and machine-specific flags,
* further overridden by per-target flags,
* and further overridden by flags passed to `make`.

Target
--------
  architecture:	${ARCH}
  machine:	${MACH}
  processors:	${with_cpus}/${with_max_cpus}
  memory:	${with_mem_size}

Kernel
--------
  DEBUG_OUTPUT:	${enable_debug_output}
  KERN_BASE:	${with_kern_base}
  KMMAP_BASE:	${with_kmmap_base}
  RESERVED:	${with_reserved_base}
  USERTOP:	${with_usertop}
  USTACKSIZE:	${with_ustacksize}
  Timer freq:	${with_timer_freq}
  Kernel stack:	${with_kstacksize}

Algorithms
--------
  simple allocator:	${with_simple_allocator}
  page allocator:	${with_page_allocator}
  caching allocator:	${with_caching_allocator}
  device index:		${with_dev_index}

Drivers
--------
  io-mem:	${enable_io_mem}
  io-port:	${enable_io_port}
  uart-zynq:	${enable_uart_zynq}
  uart-msim:	${enable_uart_msim}
  sd-zynq:	${enable_sd_zynq}
  timer-a9:	${enable_timer_a9}

Primary Drivers
--------
  console:	${with_primary_console}
  storage:	${with_primary_storage}
  timing:	${with_primary_timing}

Firmware
--------
  stack order:	${with_fwstack_order}
])

