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

#ifndef _LIBC_SYSCALLS_H
#define _LIBC_SYSCALLS_H

/*
 * NOTES for system calls:
 * We plan to make these system calls POSIX-compliant, but this doesn't mean
 * that we will fully implement the behaviors specified by POSIX _now_.
 *
 * We will indicate to what extent each system call has been implemented.
 */

/*
 * int fork(void)
 */
#define NRSYS_fork	1

/*
 * int execve(int argc, const char *argv[], const char *envp[])
 * Environment is currently not implemented.
 */
#define NRSYS_execve	2
/*
 * pid_t waitpid(pid_t pid, int *status, int options)
 * We only implement "options == 0" case.
 * We only provide meaningful WIFEXITED() and WEXITSTATUS().
 */
#define NRSYS_waitpid	3
/*
 * int kill(pid_t pid, int sig)
 * Signals and signal handlers are not implemented.
 */
#define NRSYS_kill	4
/*
 * int sched_yield(void)
 */
#define NRSYS_sched_yield	5
/*
 * pid_t getpid(void)
 */
#define NRSYS_getpid	6
/*
 * int nanosleep(const struct timespec *req, struct timespec *rem)
 */
#define NRSYS_nanosleep	7
/*
 * pid_t getppid(void)
 */
#define NRSYS_getppid	8
/*
 * void exit(int status)
 */
#define NRSYS_exit	9
/*
 * int open(const char *pathname, int flags, int mode)
 */
#define NRSYS_open	10
/*
 * void close(int fd)
 */
#define NRSYS_close	11
/*
 * ssize_t read(int fd, void *buf, size_t count)
 */
#define NRSYS_read	12
/*
 * ssize_t write(int fd, const void *buf, size_t count)
 */
#define NRSYS_write	13
/*
 * off_t lseek(int fd, off_t offset, int whence)
 */
#define NRSYS_lseek	14
/*
 * int dup(int oldfd)
 */
#define NRSYS_dup	15
/*
 * int dup2(int oldfd, int newfd)
 */
#define NRSYS_dup2	16
/*
 * int link(const char *old, const char *new)
 */
#define NRSYS_link	17
/*
 * int unlink(const char *path)
 */
#define NRSYS_unlink	18
/*
 * void *sbrk(ssize_t increment)
 */
#define NRSYS_sbrk	19
/*
 * int pipe(int pipefd[2])
 */
#define NRSYS_pipe	20
/*
 * int mkdir(const char *path, int mode)
 */
#define NRSYS_mkdir	21
/*
 * int chdir(const char *path)
 */
#define NRSYS_chdir	22
/*
 * int rmdir(const char *path)
 */
#define NRSYS_rmdir	23
/*
 * int getdents(int fd, struct dirent *dirp, unsigned int count)
 */
#define NRSYS_getdents	24

/*
 * The following system calls would probably be supported in future or left
 * as exercises to students, as they should be easy to implement.
 */

/*
 * int stat(const char *path, struct stat *buf)
 */
#define NRSYS_stat	25
/*
 * int mknod(const char *path, int mode, dev_t dev)
 */
#define NRSYS_mknod	26
/*
 * int chroot(const char *path)
 */
#define NRSYS_chroot	27
/*
 * int truncate(const char *path, off_t length)
 */
#define NRSYS_truncate	28

/*
 * These system calls may be implemented only if multi-user is properly
 * supported.
 */

/*
 * int chmod(const char *path, int mode)
 */
#define NRSYS_chmod	29
/*
 * int chown(const char *path, uid_t owner, gid_t group)
 */
#define NRSYS_chown	30

/*
 * Feel free to expand this list.
 * Don't forget to add the corresponding entry in syscall-list.S!
 */

#define NR_SYSCALLS	50	/* number of system calls */

#endif
