Building guide
------

For now, our system is still a bit far from complete.  But you can take
a look at what's going on now in our development.

### Current status

Right now, our system can read and `execve` the `init` program in
user mode, located at `/sbin/init`, on an ext2 file system with revision
number 0.

`init` process forks a child process, then starts echoing user input.

That's it.  :(

### Loongson 3A

Please refer to `doc/toolchain.md` for how to build a **MIPS multilib**
toolchain with Crosstool-NG.  GCC 5.0+ must work, and GCC 4.8+ should work,
although this is not tested.

I recommend using the following configuration to build the whole system (as
other configurations won't work right now):

```
env ARCH=mips64 MACH=loongson3a ./configure \
    --enable-static \
    --disable-shared \
    --without-pic \
    --host=mips64el-multilib-linux-uclibc \
    --with-kern-start=0xffffffff80300000 \
    --with-mem-size=0x10000000 \
    --enable-loongson3a-ram-detection \
    --with-kstacksize=16384 \
    --with-kern-base=0xffffffff80000000 \
    --with-kmmap-base=0xfffffffff0000000 \
    --with-reserved-base=0xffffffffffff0000 \
    --with-root-partition-id=2
```

Among all the options, `--with-mem-size` is merely a filler; it does not mean
that the system should only use 256MB of RAM.

Add `--enable-debug` option to enable debug printing.  Expect a LOT of outputs.

Alternatively, you can just invoke the `loongson3a.configure` script:

```
$ . loongson3a.configure
```

Then, build the kernel and user space programs:

```
$ make
```

#### Preparing root file system (rootfs)

I will try to automate the root file system setup process, but right now
I'm busy completing the file system implementation.

For now, we only support DOS partitions, and we only support primary
partitiions.

Currently, we only support ext2 Revision 0.  Make an ext2 rev. 0 file system
at the **2nd** **primary** partition of the hard disk.  Your command should
look like this (**NOTE: all commands in this section are executed on your
host machine, not on target board**):

```
# mkfs -t ext2 -r 0 /dev/sdb2
```

If you want to use another partition, make sure you change the
`--with-root-partition-id` option accordingly.

I don't recommend large partition because our code may have infinite number
of bugs now, and will often corrupt the file system, which will require
frequent backup & restores, and you wouldn't want to spend too much time
on it.

Mount the partition, and create a TTY character special file there:

```
# mkdir dev
# mknod tty c 0 0
```

Do NOT change the major and minor number, as major numbers are statically
allocated for each device driver, hardwired in the kernel.

Copy the `init` user program into the target file system.  Assuming that your
project is located in directory `~/AIMv6`, the command should probably look
like this:

```
# mkdir sbin
# cp ~/AIMv6/user/sbin/init/init sbin/init
```

Connect the hard disk back to the keyboard, and you can bring up the system.

