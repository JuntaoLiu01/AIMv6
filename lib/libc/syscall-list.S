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

/*
 * Add system calls here.
 */

DEFINE_SYSCALL(fork)
DEFINE_SYSCALL(execve)
DEFINE_SYSCALL(waitpid)
DEFINE_SYSCALL(kill)
DEFINE_SYSCALL(sched_yield)
DEFINE_SYSCALL(getpid)
DEFINE_SYSCALL(nanosleep)
DEFINE_SYSCALL(getppid)
DEFINE_SYSCALL(exit)
DEFINE_SYSCALL(open)
DEFINE_SYSCALL(close)
DEFINE_SYSCALL(read)
DEFINE_SYSCALL(write)
DEFINE_SYSCALL(lseek)
DEFINE_SYSCALL(dup)
DEFINE_SYSCALL(dup2)
DEFINE_SYSCALL(link)
DEFINE_SYSCALL(unlink)
DEFINE_SYSCALL(sbrk)
DEFINE_SYSCALL(pipe)
DEFINE_SYSCALL(mkdir)
DEFINE_SYSCALL(chdir)
DEFINE_SYSCALL(rmdir)
DEFINE_SYSCALL(getdents)

/* Will be supported in future or by yourself */
DEFINE_SYSCALL(stat)
DEFINE_SYSCALL(mknod)
DEFINE_SYSCALL(chroot)
DEFINE_SYSCALL(truncate)

/* Only if multi-user is supported */
DEFINE_SYSCALL(chmod)
DEFINE_SYSCALL(chown)
