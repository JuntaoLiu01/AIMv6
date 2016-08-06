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

#include <sys/types.h>
#include <sys/endian.h>
#include <syscall.h>
#include <limits.h>
#include <fs/vnode.h>
#include <fs/namei.h>
#include <fs/uio.h>
#include <libc/syscalls.h>
#include <libc/string.h>
#include <errno.h>
#include <elfops.h>
#include <ucred.h>
#include <proc.h>
#include <percpu.h>
#include <mm.h>
#include <util.h>
#include <vmm.h>
#include <panic.h>
#include <gfp.h>

static int
__load_elf_hdrs(struct vnode *vnode, struct elfhdr *eh, struct elf_phdr **pharr,
    struct elf_shdr **sharr, int word_len)
{
	struct elf32hdr eh32;
	struct elf32_phdr ph32;
	struct elf32_shdr sh32;
	struct elf64hdr eh64;
	struct elf64_phdr ph64;
	struct elf64_shdr sh64;
	struct elf_phdr *pharray;
	struct elf_shdr *sharray;
	void *tmpeh, *tmpph, *tmpsh;
	size_t szeh, szph, szsh;
	int err, i;
	off_t off;

	tmpeh = (word_len == 64) ? (void *)&eh64 : (void *)&eh32;
	tmpph = (word_len == 64) ? (void *)&ph64 : (void *)&ph32;
	tmpsh = (word_len == 64) ? (void *)&sh64 : (void *)&sh32;
	szeh = (word_len == 64) ? sizeof(eh64) : sizeof(eh32);
	szph = (word_len == 64) ? sizeof(ph64) : sizeof(ph32);
	szsh = (word_len == 64) ? sizeof(sh64) : sizeof(sh32);

	err = vn_read(vnode, 0, szeh, tmpeh, 0, UIO_KERNEL, current_proc, NULL,
	    NOCRED);
	if (err)
		return err;
	if (word_len == 64)
		LOAD_ELFHDR(eh, &eh64);
	else
		LOAD_ELFHDR(eh, &eh32);

	pharray = kcalloc(sizeof(*pharray), eh->phnum, 0);
	off = eh->phoff;
	for (i = 0; i < eh->phnum; ++i) {
		err = vn_read(vnode, off, szph, tmpph, 0, UIO_KERNEL,
		    current_proc, NULL, NOCRED);
		if (err)
			goto rollback_pharray;
		if (word_len == 64)
			LOAD_ELFPHDR(&pharray[i], &ph64);
		else
			LOAD_ELFPHDR(&pharray[i], &ph32);
		off += eh->phentsize;
	}

	sharray = kcalloc(sizeof(*sharray), eh->shnum, 0);
	off = eh->shoff;
	for (i = 0; i < eh->shnum; ++i) {
		err = vn_read(vnode, off, szsh, tmpsh, 0, UIO_KERNEL,
		    current_proc, NULL, NOCRED);
		if (err)
			goto rollback_sharray;
		if (word_len == 64)
			LOAD_ELFSHDR(&sharray[i], &sh64);
		else
			LOAD_ELFSHDR(&sharray[i], &sh32);
		off += eh->shentsize;
	}

	*pharr = pharray;
	*sharr = sharray;
	return 0;

rollback_sharray:
	kfree(sharray);
	*sharr = NULL;
rollback_pharray:
	kfree(pharray);
	*pharr = NULL;
	return err;
}

static int
load_elf_hdrs(struct vnode *vnode, struct elfhdr *eh, struct elf_phdr **pharr,
    struct elf_shdr **sharr)
{
	char ident[EI_NIDENT];
	int err;

	/* TODO: check permissions */

	err = vn_read(vnode, 0, EI_NIDENT, ident, 0, UIO_KERNEL, current_proc,
	    NULL, NOCRED);
	if (err)
		return err;

	if (memcmp(ident, ELFMAG, SELFMAG) != 0)
		return -ENOEXEC;
#ifdef __LP64__
	/* For now 64-bit kernels only support 64-bit binaries */
	if (ident[EI_CLASS] != ELFCLASS64)
		return -ENOEXEC;
#else
	if (ident[EI_CLASS] != ELFCLASS32)
		return -ENOEXEC;
#endif
#ifdef LITTLE_ENDIAN
	if (ident[EI_DATA] != ELFDATA2LSB)
		return -ENOEXEC;
#else
	/* We do not support big endian processors for now. */
	return -ENOEXEC;
#endif

	switch (ident[EI_CLASS]) {
	case ELFCLASS32:
		err = __load_elf_hdrs(vnode, eh, pharr, sharr, 32);
		if (err)
			return err;
		break;
	case ELFCLASS64:
		err = __load_elf_hdrs(vnode, eh, pharr, sharr, 64);
		if (err)
			return err;
		break;
	}

	/* TODO: how should we check machine type? */
	if (eh->type != ET_EXEC)
		return -ENOEXEC;
	return 0;
}

static int
__load_elf_seg(struct vnode *vnode, struct elf_phdr *ph, struct mm *mm)
{
	int err;
	void *start = (void *)ALIGN_BELOW(ph->vaddr, PAGE_SIZE);
	void *end = (void *)ALIGN_ABOVE(ph->vaddr + ph->memsz, PAGE_SIZE);

	if ((err = create_uvm(mm, start, end - start, ph->flags)) != 0)
		return err;

	err = vn_read(vnode, ph->offset, ph->filesz, start, 0, UIO_USER,
	    current_proc, mm, NOCRED);
	return err;
}

/*
 * Pushes argument strings starting at @argstr_top, from high address to low.
 * Meanwhile, pushes the pointers to the pushed argument strings starting at
 * @argp.
 * After returning, @argp stores the address of the argument string pointer
 * array.
 */
static int
__push_args(struct mm *mm, void *argstr_top, int argc, char *uargs[],
    void **argarrayp, void **argstart)
{
	size_t available = ARG_MAX, len;
	int i, err;
	unsigned long *sp = *argarrayp;
	char *p = NULL;

	--sp;
	if ((err = copy_to_uvm(mm, sp, &p, sizeof(char *))) != 0)
		return err;
	for (i = argc - 1; i >= 0; --i) {
		len = strnlen(uargs[i], ARG_MAX) + 1;
		if (len > available)
			return -E2BIG;
		argstr_top -= len;
		if ((err = copy_to_uvm(mm, argstr_top, uargs[i], len)) != 0)
			return err;
		available -= len;
		--sp;
		err = copy_to_uvm(mm, sp, &argstr_top, sizeof(char *));
		if (err)
			return err;
	}

	*argarrayp = sp;
	if (argstart != NULL)
		*argstart = argstr_top;
	return 0;
}

/* Arch-specific code */
extern void
__prepare_trapframe_and_stack(struct trapframe *tf, void *ustack_top,
    void *entry, int argc, char **argv, char **envp);
/*
 * Prepares arguments, stack, and trap frame.
 */
static int
prepare_trapframe_and_stack(struct trapframe *tf, struct mm *mm, void *entry,
    char *uargv[], char *uenvp[])
{
	void *argstr_base, *args_base, *ustack_base;
	void *argv, *envp;		/* arguments being passed to main() */
	size_t argstr_size, args_size, ustack_size;
	void *envbase;
	int argc, envc;
	int err;

	if (!IS_ALIGNED(USERTOP, PAGE_SIZE))
		panic("kernel configuration error: USERTOP not aligned to pages\n");

	for (argc = 0; uargv[argc] != NULL; ++argc)
		/* nothing */;
	for (envc = 0; uenvp[envc] != NULL; ++envc)
		/* nothing */;

	/*
	 * We need several user memory mappings, from high address to low:
	 * 1. One for storing the argument strings of argv and envp, each
	 *    with maximum total storage of ARG_MAX.
	 * 2. One for storing the content of argv and envp, which is
	 *    an array of pointers, taking argc * sizeof(char *) storage.
	 * 3. One for holding the user stack, and possibly with the
	 *    arguments of main(), as well as other necessary arch-dependent
	 *    stuff for a valid C stack.
	 */
	/* (1) */
	argstr_size = ALIGN_ABOVE(2 * ARG_MAX, PAGE_SIZE);
	argstr_base = (void *)(USERTOP - argstr_size);
	if ((err = create_uvm(mm, argstr_base, argstr_size, VMA_READ)) != 0)
		return err;
	/* (2) */
	args_size = ALIGN_ABOVE((argc + envc) * sizeof(char *), PAGE_SIZE);
	args_base = argstr_base - args_size;
	if ((err = create_uvm(mm, args_base, args_size, VMA_READ)) != 0)
		return err;
	/* (3) */
	ustack_size = ALIGN_ABOVE(USTACKSIZE, PAGE_SIZE);
	ustack_base = args_base - ustack_size;
	err = create_uvm(mm, ustack_base, ustack_size, VMA_READ | VMA_WRITE);
	if (err)
		return err;

	/* Push argv and envp */
	envp = args_base + args_size;
	err = __push_args(mm, (void *)USERTOP, envc, uenvp, &envp, &envbase);
	if (err)
		return err;
	argv = envp;
	if ((err = __push_args(mm, envbase, argc, uargv, &argv, NULL)) != 0)
		return err;

	/* Call the arch-specific code to set up trap frame and argument list of
	 * main() */
	__prepare_trapframe_and_stack(tf, entry, ustack_base + ustack_size,
	    argc, argv, envp);
	return 0;
}

static int
load_elf_segs(struct vnode *vnode, struct elfhdr *eh, struct elf_phdr *pharr,
    struct elf_shdr *sharr, struct mm *mm, void **progtop)
{
	int err, i;
	unsigned long ptop;

	for (i = 0; i < eh->phnum; ++i) {
		switch (pharr[i].type) {
		case PT_DYNAMIC:
		case PT_INTERP:
			/* We do not support dynamic linking for now */
			return -ENOEXEC;
		case PT_LOAD:
			err = __load_elf_seg(vnode, &pharr[i], mm);
			if (err)
				return err;
			ptop = max2(ptop, pharr[i].vaddr + pharr[i].memsz);
			break;
		default:
			break;
		}
	}

	for (i = 0; i < eh->shnum; ++i) {
		switch (sharr[i].type) {
		case SHT_NOBITS:
			err = fill_uvm(mm, (void *)sharr[i].addr, 0, sharr[i].size);
			if (err)
				return err;
			break;
		default:
			break;
		}
	}

	*progtop = (void *)ptop;
	return 0;
}

int
sys_execve(struct trapframe *tf, int *errno, char *ufilename, char *uargv[],
    char *uenvp[])
{
	struct nameidata nd;
	struct mm *new_mm, *old_mm = current_proc->mm;
	struct elfhdr eh;
	struct elf_shdr *sh;
	struct elf_phdr *ph;
	char *filename;
	void *progtop;
	int err;
	unsigned long flags;

	if (strnlen(ufilename, PATH_MAX) == PATH_MAX) {
		*errno = ENAMETOOLONG;
		return -1;
	}

	if ((filename = kmalloc(PATH_MAX, 0)) == NULL) {
		*errno = ENOMEM;
		return -1;
	}
	strlcpy(filename, ufilename, PATH_MAX);

	if ((new_mm = mm_new()) == NULL) {
		err = -ENOMEM;
		goto rollback_malloc;
	}

	nd.path = filename;
	nd.intent = NAMEI_LOOKUP;
	nd.flags = NAMEI_FOLLOW;
	nd.cred = NOCRED;
	nd.proc = current_proc;

	if ((err = namei(&nd)) != 0)
		goto rollback_mm;

	if ((err = load_elf_hdrs(nd.vp, &eh, &ph, &sh)) != 0)
		goto rollback_vp;
	if ((err = load_elf_segs(nd.vp, &eh, ph, sh, new_mm, &progtop)) != 0)
		goto rollback_hdrs;

	err = prepare_trapframe_and_stack(tf, new_mm, (void *)eh.entry, uargv,
	    uenvp);
	if (err)
		goto rollback_hdrs;

	/* Everything set up, ready to nuke out the old mm.  We probably do
	 * not want the switching process to be interrupted. */
	local_irq_save(flags);
	current_proc->mm = new_mm;
	switch_pgindex(new_mm->pgindex);
	mm_destroy(old_mm);
	current_proc->heapsize = 0;
	current_proc->heapbase = progtop;
	local_irq_restore(flags);

	kfree(ph);
	kfree(sh);
	vput(nd.vp);
	kfree(filename);
	*errno = 0;
	/* The process will begin its new life after returning from trap
	 * handler. */
	return 0;

rollback_hdrs:
	kfree(ph);
	kfree(sh);
rollback_vp:
	vput(nd.vp);
rollback_mm:
	mm_destroy(new_mm);
rollback_malloc:
	kfree(filename);
	*errno = -err;
	return -1;
}
ADD_SYSCALL(sys_execve, NRSYS_execve);

