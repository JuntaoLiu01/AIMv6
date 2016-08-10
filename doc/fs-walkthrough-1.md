File system walkthrough Part 1
======

This walkthrough contains how the root file system is mounted from scratch,
how the file system interacts with a disk driver, how system calls are
translated into file system operations, etc., **step by step**.

I'm going to refer to symbols rather than source filenames and lines.
Although you can look up definitions and usages by `grep(1)` or `git grep`
command, building a cscope database will make your days easier.
Also, ~~real~~ some editors like
[Emacs](https://www.gnu.org/software/emacs/) and
[Vim](http://www.vim.org/) usually have built-in cscope support and/or
add-ons:

* [Cscope on Emacs](https://www.emacswiki.org/emacs/CScopeAndEmacs)
* [Cscope on Vim](http://cscope.sourceforge.net/cscope_vim_tutorial.html)

If your editor does not have cscope support, ~~throw your stupid editor away and
start learning Vim or Emacs~~ you can always try the interactive cscope
interface by running `cscope(1)` without arguments.

You can build a cscope database by running

```
find . -name '*.c' -or -name '*.h' -or -name '*.S' >cscope.files && cscope -bquk
```

### Prelude: Before bringing up file system

Because disk operations are slow compared to memory loads/stores, we usually want
to let the CPU do other stuff while the disk controller performs I/O.  Doing
this involves

1. Handling interrupts asserted by disk controller
2. Suspending a process (`sleep()` and `sleep_with_lock()`)
3. Resuming a process (`wakeup()`)

All of these imply that starting up a file system requires

* A working scheduler
* At least one running process

We put the job in a kernel process called `initproc`, which is the first
process being created (hence having a PID of 1).  The process itself is
created by function `spawn_initproc()`.

Spawning `initproc` is very easy:

* We allocate a process (`proc_new()`).
* We set it up as a kernel process (`proc_ksetup()`) and tell the kernel
  that the entry is at `initproc_entry()`.  Since the process is executing
  at kernel space for now, we do not need to create any user memory
  mapping.
* We mark the process as runnable.
* We add the process to the scheduler.

After enabling timer interrupt, all processors will try to suspend the
current process and pick the next one from scheduler.  For now, the
scheduler only holds one process `initproc`, so `initproc` will be
started, which means that `initproc_entry()` will be executed in the
context of a running process.

`initproc_entry()` calls `fsinit()`, which brings up the root file
system.

### Bringing up the file system

The overall steps of bringing up a file system includes:

* Locate a disk controller.
* Load disk partitions and determine which is the "root" partition.
* Determine which file system the root partition contains.
* Load the metadata of the file system into memory.

They are finished one-by-one in `mountroot()`.

#### `mountroot()`

`mountroot()` iterates over a list of function entry points
registered by file system providers, calling the function there
and check for the error code.  If the error code is 0, the
mounting is a success and `mountroot()` exits.

The function entry points are registered via `register_mountroot()`
function.  `register_mountroot()` is often called in file system
provider initialization code, which is executed in initcalls
before `fsinit()`.  The ext2 provider initialization routine is
called `ext2fs_register()`, which registers the ext2-specific
`mountroot()` implementation: `ext2fs_mountroot()`.

As we only provide one file system in current AIMv6 implementation,
`ext2fs_mountroot()` is the only one `mountroot()` operation to be
executed.

#### `ext2fs_mountroot()`

`ext2fs_mountroot()` is the function which provides the `mountroot()`
implementation on an ext2 file system, which, along with its
descendants (including ext3 and ext4), is widely used in modern
Unix-like systems such as Linux and BSD.

The technical details of ext2 can be found
[here](http://wiki.osdev.org/Ext2).

`ext2fs_mountroot()` consists of four operations:

1. Obtaining a kernel representation of the device holding the root
  file system via the following call:

  ```C
  bdevvp(rootdev, &rootvp)
  ```

2. Create an object representing the file system as a whole, assigning
  file-system-wide operations to it as methods:

  ```C
  vfs_rootmountalloc("ext2fs", &mp)
  ```
  
3. Load the metadata of the file system into memory:

  ```C
  ext2fs_mountfs(rootvp, mp, current_proc)
  ```
  
4. Add the file system object `mp` into the list of loaded file systems.
  This operation is quite trivial by itself and I will not extend it
  further:

  ```C
  addmount(mp)
  ```
  
### Vnode - the kernel in-memory representation of a file

The `bdevvp()` call takes a device identifier (with type `dev_t`) as
input, and produces a `struct vnode` object as output.  This `struct
vnode` object, or *vnode* for short, represents the device we are going
to read and write on.  In this section, I'll explain what is a vnode,
before moving on to `bdevvp()`, which will be trivial to understand
then.

Vnode is the kernel representation of a file in AIMv6 (and also BSD
and Linux - though Linux uses the term "inode", a UFS-specific term,
across all file systems for this purpose).

#### Files, directories

A file, by [definition](https://en.wikipedia.org/wiki/Computer_file), is
a kind of information storage, available for programs to read and write
as a unit.

A [directory](https://en.wikipedia.org/wiki/Directory_(computing)) is
either a collection of files, or a collection of file references.  In
Unix, directories are treated as files, so they share the same
kernel representation.

##### Device files, device identifier and `rootdev`

Unix (and also Windows!) further extended the concept of files by
treating devices as *special files* (or *device files*, or sometimes *device
nodes*).  In this document these terms may be used interchangeably),
and device accesses and manipulations
as file operations.  For example, outputting text to a serial console
is viewed as `write`'s, while receiving information from serial console
are treated as `read`'s.  The operations which are rather hard to
classify as ordinary file operations (such as changing baud rate of
a serial console) are called `ioctl`'s.

Unix also extended the concept of a device.  A device in Unix can either
be a real peripheral (e.g. serial console, hard disk), be a virtual
concept (e.g. framebuffer), be a mixture of several devices (e.g. terminal,
which involves a keyboard peripheral and a screen peripheral), or some other
stuff (e.g. memory or I/O port bus).

A *device identifier* uniquely identifies a device in a convenient way.
Typically, a device identifier, typed `dev_t`, is represented as an integer.
The integer may be decomposed into two parts:

* *Major number*, identifying the class of a device, or which driver the
  kernel will use.
* *Minor number*, identifying a specific instance of a device in that class.

There is no absolute standard for how a minor number should be exactly used.
It is entirely decided by device drivers.  For example,

* A hard disk driver can designate some of the bits in
  the minor number to represent the hard disk number, and the rest of the bits
  to represent the partition number.

How major numbers and device drivers correspond to each other is also left
open for operating system designers.  For example,

* Unix and other ~~canonical~~ clones such as OpenBSD assigns each driver a
  major number statically.  This requires that all the device files created in
  the root file system should be coupled to the kernel.  Usually, a script
  called `MAKEDEV` does the job during system installation or first boot.
  * In AIMv6, we follow this solution, since it's the simplest one.  `rootdev`
    in our system therefore is the device identifier hardwired in the kernel
    identifying the root hard disk partition.
* More pioneering clones such as FreeBSD, Solaris, and old Linux, create the
  device nodes inside a particular memory region, which is then exposed as a
  file system to outside world.  The file system is usually called `devfs`.
* Most Linux distros today, including Ubuntu and Fedora, pushes the job of
  creating and communicating with device files into `systemd`, which is
  ~~a part of the Linux kernel pretending to be~~ a userspace daemon.

##### Pipes, sockets, and more

Occasionally, inter-process communications, including pipes and sockets,
can also appear in various forms of file operations.

A file does not need to be actually persist in durable storages such
as hard disks; it can be only present in memory, sometimes usable only
by processes requested to open it (e.g. pipes and sockets).

There are more concepts and objects that can be treated as files, some of
which are even beyond imagination:

* Special data sources and sinks (e.g. random number generator, as devices)
* Memory (as a device)
* CPU (as a device)
* Web servers
* Wikipedia
* [~~Your belongings~~](https://en.wikipedia.org/wiki/Dunnet_(video_game))

#### Members of vnode (Part 1)

A vnode has a lot of members, but for now we only need to focus on some
of them:

* `type` - One of the following:

  |Type   |Description (Unix)    |Windows equivalent     |
  |-------|----------------------|-----------------------|
  |`VREG` |Regular file          |Good ol' everyday files|
  |`VDIR` |Directory             |Folders                |
  |`VCHR` |Character device file |                       |
  |`VBLK` |Block device file     |                       |
  |`VLNK` |Symbolic link         |Shortcuts              |
  
  And possibly one of the following, which are not available in AIMv6 but in
  BSD systems:
  
  |Type   |Description (Unix)    |Windows equivalent     |
  |-------|----------------------|-----------------------|
  |`VFIFO`|Named pipes           |                       |
  |`VSOCK`|Sockets               |                       |

* `ops` - The underlying implementation of file operations, usually assigned
  by file system providers upon creation, and depends on the file system and
  file type.  For example, reading from a regular file is certainly different
  from reading from a device.
* `mount` - The file system object the vnode resides on.
* `typedata` - Depending on vnode type, this may point to different structures.
  For now, we only consider member `specinfo`, which points to a `specinfo`
  structure if the vnode corresponds to a device (either `VCHR` or `VBLK`).


#### `getdevvp()`, `bdevvp()` and `cdevvp()`

The job of `getdevvp()` is simple: find or create a vnode corresponding to
the provided device identifier.  A device may or may not have a device identifier.
If a device does have an identifier, then exactly one vnode corresponds to it.
Such vnode is called a *special vnode*, which has type `VCHR` or `VBLK`.

When creating a vnode, `getdevvp()` performs the following tasks:

* Assigning the vnode a set of operations dedicated for special files, or devices.
  They are described in the `spec_vops` structure.
* Allocate a `specinfo` structure, which then keeps the device identifier.
* Associate the vnode and the `specinfo` structure.

`bdevvp()` and `cdevvp()` are merely special cases for creating a block vnode or
character vnode respectively.

### `struct mount` and mounting

The second step of `ext2fs_mountroot` is to create an in-memory representation
of the file system to be mounted.  Such representation is encoded in
`struct mount`.  Also, we call loading a file system into kernel *mounting*,
and unloading the file system *unmounting*.  ~~I'm not sure why they invented
a new word for loading a file system.~~

In AIMv6, `struct mount` does not hold a lot of information like vnodes do.
Normally, a `struct mount` should keep

* A list of associated file-system-wide operations (see below)
* A list of vnodes on this file system.
* File-system specific metadata.  Such metadata is often called a
  *superblock*, and is usually stored on the disk.

#### File system operations

Currently in AIMv6, there are three file system operations, which are all
described in `struct vfsops`:

* `root` - Get vnode of the root directory in the file system.
  * On UFS-like systems including FFS and ext2, `root` returns the vnode for
    directory `"/"`.
* `vget` - Given a `struct mount` and a file identifier, return a vnode
  representing the file, with its metadata (including type, attributes,
  size, etc.) loaded.
* `sync` - Flush the in-memory superblock back to disk.
  * On BSDs, if the system delays some write requests, `sync` flushes them
    all onto disk.  AIMv6 does not do this since it does not support either
    delayed writes or asynchronous writes; every write request is executed
    instantly after the request is made, and the kernel waits for the request
    to finish.

Mounting a file system merely stands for loading the superblock into memory.
The bulk of mounting a file system is written in `ext2fs_mountfs` and consists
of three steps:

1. Prepare the partition device with a call to `VOP_OPEN`.
2. Read the superblock on the disk into memory using `bread`.
3. Compute the auxilliary parameters of the file system (e.g. file system
  block size).

### Vnode operations (Part 1, `open` and `close`)

Preparing and releasing a device involves calling the `open` and `close`
operation, respectively, of a corresponding special vnode.

In AIMv6, before mounting the ext2 file system, special vnodes have their
operations set in the `spec_vops` record, where we can found the concrete
implementation of `open` and `close` operations at `spec_open` and
`spec_close`.

#### Hardware drivers (Part 1, `open` and `close`)

TODO:

* describe the implementation of `open`, `close` for special vnode.
* explain driver table `devsw`
* take MSIM disk driver and/or ATA disk driver as an example, explain
  what its `open` and `close` do.

### `struct buf` - buffered I/O

TODO:

* explain `struct buf`.
* explain `bgetempty`, and `brelse` on standalone buf's.

### Vnode operations (Part 2, `strategy`)

TODO: introduce operation `strategy`.

#### Hardware drivers (Part 2, `strategy`)

TODO:

* explain special vnode's `strategy`.
* explain `strategy` in MSIM disk driver or ATA disk driver.
* explain how interrupts interact with `struct buf`

### End-to-end: Partition detection

TODO:

* example of `strategy`: end-to-end walkthrough of partition detection in
  `VOP_OPEN`.

### `bread` (Part 1, on devices)

TODO:

* explain `bget` and `bread` on block devices, and also `brelse` on associated
  buf's.

### End-to-end: Superblock initialization

TODO:

* demonstrate how superblock is loaded into memory.


