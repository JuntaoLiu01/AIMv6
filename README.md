# AIMv6

AIMv6 project builds an operating system for teaching purposes.
This system should seperate kernel logic from platform-dependent implementation,
thus should work across platforms.

For now, the system itself is certainly very buggy and vulnerable.

## Supported platforms

* ARMv7A
  - Xilinx Zynq-7000 SoC (2x Cortex A9)
* IA32
  - QEMU
* MIPS32
  - [MSIM](http://d3s.mff.cuni.cz/~holub/sw/msim/)
  - Note that we are only supporting 256MB RAM for all MIPS32 machines.
* MIPS64
  - Loongson 3A

## Building AIMv6

### Prerequisites

AIMv6 builds in UNIX-like environments, and have the following requirements:

* A proper toolchain. Cross-compiling toolchains works for all platforms. For
  IA32 targets, native toolchains are usable as well. See `doc/toolchain.md`
  for details.

  - The C compiler **MUST** support C11 standard.

  - The C compiler **MUST** support plan9 extensions.

* A `sh`-compatible shell.

* `make`

* A full set of GNU autotools if building in maintainer mode (when this source
  tree comes from a source repository instead of a tarball).

  - autoconf-archive must be present. It can be installed from your
    distribution's software repository.

    + Ubuntu/Debian: `sudo apt-get install autoconf-archive`
    + Fedora: `dnf install autoconf-archive` as root.

### Normal mode

If you receive this source tree in the form of a tarball, or if the `configure`
script is present, AIMv6 can be built in normal mode.

Following steps should be taken to build AIMv6.

1. Run `./configure`, with parameters and environment variables according to
   your need.

2. Run `make`

AIMv6 does not come with any test suites right now.

AIMv6 can install built object under given prefix, but this feature is not
finished yet.

### Maintainer mode

If AIMv6 cannot be built in normal mode, a lot of generated scripts are not
present yet. To keep the reporsitory clean, generated files **SHOULD NOT** be
included in commits.

Before you can build the source, run `autoreconf --install --force` to generate
the `configure` scripts as well as other helper files.  Then, follow the normal
mode steps above.

#### Ultimate Maintainer mode

Usually maintainer mode would suffice for all development in this project,
However, if you are willing to try the latest autotools features such as
`AC_PROG_CC_C11`, you can build a set from source as follows:

1. Grab `autoconf` source from [git://git.sv.gnu.org/autoconf.git].

2. Run `autoreconf --install` at the root of the `autoconf` source. Note that
   you are building `autoconf` in maintainer mode as well, so a working set
   of GNU autotools is required. The `autoconf` binary version requirement
   is 2.60.

3. Configure and install `autoconf`. Installation under `$HOME` is highly
   recommended. Do not overwrite the installed version unless you are on LFS
   or **YOU ABSOLUTELY KNOW WHAT YOU ARE DOING**. AIMv6 team is **not**
   responsible for any possible damage to your system. If you are building
   a custom toolchain, you **SHOULD** choose a different prefix for `autoconf`.

4. Add the `bin` folder under your installation prefix **BEFORE** your `$PATH`
   variable. You may want to add this to your `~/.profile`. You should limit
   this effect to the unprivileged user you are currently using, as this
   version of `autoconf` is highly experimental. Also, make sure there's no
   other programs (not included in the `autoconf` install) under the same
   directory, unless you clearly know what you are doing and really mean it.

## Running AIMv6

### MIPS

#### MSIM configuration

A suggested configuration is

```
./configure --host=mips64el-multilib-linux-uclibc \
            --without-pic \
            --enable-static \
            --disable-shared \
            --with-kern-start=0x80300000 \
            --with-mem-size=0x10000000 \
            --enable-io-mem
```

Replace `mips64el-multilib-linux-uclibc` to the prefix of any available toolchain.
For example, if you have a MIPS GCC compiler named
`mips64-unknown-linux-gnu-gcc`, the host argument should be
`mips64-unknown-linux-gnu`.

You can also define smaller or larger memory sizes by replacing
`--with-mem-size` argument with what you need.

##### Preparing disk image

MSIM needs a disk image to simulate hard disks.

###### A sample configuration

**May be changed later**.

Run

```
dd if=/dev/zero of=disk.img bs=1024 count=1000000
```

to create a 1000000K blank disk image.

Then, use `fdisk(8)` to make DOS partitions:

```
fdisk disk.img
```

Copy the bootloader into the first 446 bytes in the first sector:

```
dd if=boot/arch/mips/msim/boot.bin of=disk.img bs=1 count=446 conv=notrunc
```

Use `kpartx(8)` to mount the partitions in disk image (probably as root):

```
kpartx -av disk.img
```

If successful, you may see the following messages:

```
add map loop0p1 (253:3): 0 204800 linear /dev/loop0 2048
add map loop0p2 (253:4): 0 40960 linear /dev/loop0 206848
add map loop0p3 (253:5): 0 1024000 linear /dev/loop0 247808
add map loop0p4 (253:6): 0 776768 linear /dev/loop0 1271808
```

Copy the kernel image to the second partition on disk image (probably as root):

```
dd if=kern/arch/mips/kernel.elf of=/dev/mapper/loop0p2 conv=notrunc
```

Run MSIM to bring up the operating system.

##### Debugging

**IMPORTANT:** MSIM's gdb support is **extremely experimental**, that is,
slow, and sometimes causes errors on gdb.  It is highly recommended to
debug your kernel statically (that is, via the kernel's own output), or
via MSIM's interface and disassembly dumps from
`mips64el-multilib-linux-uclibc-objdump -Ss`.

You need to build a `cross-gdb` to enable debugging MIPS programs.  See
`doc/toolchain.md` for details.

Run

```
msim -g 1234
```

to bring up a server for remote debugging.

Run

```
mips64el-multilib-linux-uclibc-gdb
```

then enter

```
tar rem 127.0.0.1:1234
```

to connect to the gdb server brought up by MSIM.

#### Loongson 3A

You should prepare a SATA-to-USB converter or something to enable you to upload
your kernel image into the hard disk inside Loongson 3A box.

Also, you probably need to use a machine with serial port (which is,
unfortunately, unavailable on most laptops), or a serial-to-USB converter.
Currently we are only using serial consoles for input/output, as VGA and
USB keyboard is too complicated for a teaching operating system.

You need a modem connector program such as "minicom" to work.  **Make sure you
turn off Hardware Flow Control!**

Replace the `boot/vmlinux` file with the kernel image you compiled, and you're
done.

We do **NOT** support debugging on Loongson 3A.  However, technical
contributions are always welcome.

### ARM on qemu

Below is an example on how to test AIMv6: (A lot of work needed here)

```sh
qemu-system-arm -M xilinx-zynq-a9 -m 512 -serial null -serial stdio \
	-kernel firmware/arch/armv7a/firmware.elf -s -S
```

The `-s -S` option will enable gdb support on port 1234. You can then run
`gdb firmware/arch/armv7a/firmware.elf` with needed options and connect to the
emulator via `target remote:1234` in its command line.

**TODO:** please provide a complete, end-to-end QEMU setup, including the
command lines to build disk images (if any), starting up kernels and/or
firmwares, etc.

### i386 on qemu

#### Compiling

A sample compiling configuration:

```
$ env ARCH=i386 MACH=generic ./configure \
> --enable-static --disable-shared --without-pic \
> --with-kern-start=0x100000 --with-mem-size=0x20000000
$ make clean
$ make
```

where `--with-kern-start` specifies the starting address kernel will be
loaded at.

#### Creating a blank disk image

```
$ dd if=/dev/zero of=disk.img bs=1M count=500
```

creates a 500M disk image.

#### Making partitions

```
$ fdisk disk.img
```

to interactively make partitions.

You can also use `sfdisk(8)` to manipulate disk partitions in a
script-oriented manner.

#### Mounting disk image partitions

You would probably have to run as root.

```
# kpartx -av disk.img
```

The partitions are mounted as `/dev/mapper/loop0pX` in Fedora, where
`X` corresponds to partition ID.

**TODO:** add running cases in Ubuntu etc.

#### Installing bootloader

```
$ dd if=boot/arch/i386/boot.bin of=disk.img conv=notrunc
```

Take extra care to ensure that `boot.bin` never exceeds 446 bytes, or
partition entries may be overwritten.

#### Installing kernel onto 2nd partition

On Fedora:

```
# dd if=kern/vmaim.elf of=/dev/mapper/loop0p2
```

**TODO:** add running cases in Ubuntu etc.

#### Running QEMU

```
$ qemu-system-i386 -serial mon:stdio -hda i386.img -smp 4 -m 512
```

#### Running QEMU and debug with GDB

```
$ qemu-system-i386 -serial mon:stdio -hda i386.img -smp 4 -m 512 \
> -S -gdb tcp::1234
```

Then run GDB and enter the following:

```
(gdb) target remote :1234
```

#### Debugging with GDB

If you are going to debug your kernel, you probably want to disable
optimization so that your C code matches the disassembly exactly.
To do so, you will have to reconfigure and rebuild the project with
a new `CFLAGS`:

```
$ env ARCH=i386 MACH=generic CFLAGS='-g -O0' ./configure \
> --enable-static --disable-shared --without-pic \
> --with-kern-start=0x100000 --with-mem-size=0x20000000
$ make clean
$ make
```

Step to next instruction:

```
(gdb) si
```

Switching layout:

```
(gdb) help layout
```

to see usage of `layout` command.  You can view disassembly/sources there.

Set breakpoint at address:

```
(gdb) b *0x7c00
```

to add a breakpoint at bootloader entry.

Loading debugging symbols:

```
(gdb) symbol kern/vmaim.elf
```

to enable C code debugging after QEMU stepped into kernel.

## Contributing to AIMv6

Read `doc/patterns.md` for code styles and design patterns adopted in our
project.  It is highly recommended to follow the guidelines there.

## Roadmap

Feel free to add features here

### Finished

* Device driver subsystem
* Page allocator
* Arbitrary size allocator
* Trap handler
* User memory mapping
* Scheduler
* SMP

### Work in progress

* File system

### Currently skipping

These features may be moved to "work in progress" section as soon as we
begin to work on them.

* `kmmap` subsystem
  - As a consequence, we are assuming that the physical memory can be
    covered by fixed kernel linear mapping.

### Planned

These features may be put into the "work in progress" section, or "currently
skipping" section.

* User-side exceptions as signals
* Multiple-user
* Access control

### Not considering

These features have a fairly low chance to be moved to "planned" section as
they are often quite complicated.

* Implicit dynamic linking
* Explicit dynamic loading
* Signal handler registration
* I/O scheduler

## BUGS

