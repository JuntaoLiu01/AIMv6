/* Copyright (C) 2016 Gan Quan <coin2028@hotmail.com>
 *
 * This file is part of AIMv6.
 *
 * AIMv6 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * AIMv6 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <syscall.h>
#include <libc/syscalls.h>
#include <file.h>
#include <pipe.h>
#include <util.h>
#include <vmm.h>
#include <pmm.h>
#include <aim/sync.h>
#include <percpu.h>
#include <errno.h>
#include <fcntl.h>

int
sys_pipe(struct trapframe *tf, int *errno, int *pipefd)
{
	struct pipe *pipe;
	struct file *reader, *writer;
	int i, j, err;
	unsigned long intr_flags;
	int fds[2];

	if (!is_user(pipefd)) {
		*errno = EFAULT;
		return -1;
	}

	spin_lock_irq_save(&current_proc->fdlock, intr_flags);

	for (i = 0; i < OPEN_MAX; ++i) {
		if (current_proc->fd[i] == NULL)
			goto findj;
	}
	goto fail_nfile;
findj:
	for (j = i + 1; j < OPEN_MAX; ++j) {
		if (current_proc->fd[j] == NULL)
			goto found;
	}
fail_nfile:
	spin_unlock_irq_restore(&current_proc->fdlock, intr_flags);
	*errno = ENFILE;
	return -1;
found:
	if ((pipe = PIPENEW()) == NULL) {
		*errno = ENOMEM;
		goto fail_pipe;
	}
	if ((reader = FNEW()) == NULL) {
		*errno = ENOMEM;
		goto fail_read;
	}
	if ((writer = FNEW()) == NULL) {
		*errno = ENOMEM;
		goto fail_write;
	}
	FINIT_PIPE(reader, pipe, FREAD);
	FINIT_PIPE(writer, pipe, FWRITE);

	current_proc->fd[i] = reader;
	current_proc->fd[j] = writer;

	fds[0] = i;
	fds[1] = j;

	err = copy_to_uvm(current_proc->mm, pipefd, fds, sizeof(fds));
	if (err)
		goto fail_copy;

	spin_unlock_irq_restore(&current_proc->fdlock, intr_flags);
	return 0;

fail_copy:
	FRELE(&writer);
fail_write:
	FRELE(&reader);
fail_read:
	pipe_free(pipe);
fail_pipe:
	spin_unlock_irq_restore(&current_proc->fdlock, intr_flags);
	return -1;
}
ADD_SYSCALL(sys_pipe, NRSYS_pipe);

