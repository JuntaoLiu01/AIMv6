File System
======

**This note is largely unfinished.**

The file system framework mainly comes from OpenBSD, which in turn comes from
BSD.  To me, OpenBSD code is simpler than Linux and FreeBSD, and works well
across all file systems just as any other Unix-like systems do.  In AIMv6 I
added some more constraints in order to ensure there is no race conditions
and therefore simplifying problems.

The following describes the file system model as well as details for managing
buffers, internal file representations, etc., some of which does not have
counterparts in OpenBSD in order to simplify implementation.

## Support

Currently, we only support ext2 rev. 0.  We may add FAT support in future.

Also, note that Linux "automagically" (sic) upgrade rev. 0 ext2 file systems
to rev. 1, and introduce "extended attributes" feature without user permission,
messing the file system up.  However, most of the time there's no problem,
except that `fsck(8)` may print out unpleasant messages.

Virtual File System (VFS) is ported from OpenBSD to AIMv6 in order to enable
the system to work across multiple file systems smoothly.  We largely
simplified the implementation though.  Even so, the file system framework in
AIMv6 is *awfully* complicated as a result of planning to support multiple
file systems, including UFS-like file systems.

To aid study, I provided an end-to-end walkthrough in the `doc` directory.

Please not that this file system framework implementation is **super slow**.
It does not by any means serve as an implementation in real production
environment *where efficiency matters*.  Nevertheless, we are pretty close.

Also, this file system framework implementation should be pretty buggy for
now, especially in error handling since I never thoroughly tested them because
I don't have the time to do so.  Be sure to make backups.  If you found bugs,
please fix ithem (preferred), or submit patches (preferred), or contact us to
fix them (welcomed, but it will take some time).

### Development constraints

**To readers of walkthrough: if you are not developers improving, enhancing,
or refactoring this file system framework, please skip this section.  Developers
who are unfamiliar with the framework in BSD are also encouraged to read the
walkthrough below first, then jump back here.**

Here are several constraints for reference if you are extending or modifying
the file system framework.  Breaking these rules likely calls for more or less
refactoring.

1. While OpenBSD has implemented reader-writer locks on vnodes, in AIMv6, we
  only have *exclusive* locks.
2. There is at most one vnode corresponding to each file on each file system.
  Moreover, there is at most one vnode corresponding to a device.
3. Any operation on a vnode requires the vnode to be locked, in order to
  prevent any possible race conditions and ensure serialization in a simple but
  brutal way.  This is accomplished via `vlock()` and `vunlock()` routines.
4. Any flag changes on a buf requires the interrupt to be disabled, in order
  to prevent race conditions where the flags are changed in an interrupt
  after reading the flag *AND* before doing anything with it.
  * Note that since the vnode of a buf is locked, other cores cannot do
    anything from outside.
5. The vnode and all of its contents will be sync'ed, cleaned up and reclaimed,
  as soon as its reference count drops to 0.

