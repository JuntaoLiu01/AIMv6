
#include <syscall.h>
#include <libc/syscalls.h>
#include <percpu.h>
#include <fs/vfs.h>
#include <fs/mount.h>
#include <ucred.h>

void
sys_sync(struct trapframe *tf, int *errno)
{
	/* TODO REPLACE */
	VFS_SYNC(rootvnode->mount, NOCRED, current_proc);
	*errno = 0;
}
ADD_SYSCALL(sys_sync, NRSYS_sync);
