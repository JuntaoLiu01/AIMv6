
#ifndef _ELFOPS_H
#define _ELFOPS_H

#include <elf.h>
#include <libc/string.h>	/* memcpy() */

/*
 * These structures have the same members as elf32xxx and elf64xxx, with
 * different types and layouts.
 */

struct elfhdr {
	unsigned char	ident[EI_NIDENT];
	unsigned short	type;
	unsigned short	machine;
	unsigned short	ehsize;
	unsigned short	phentsize;
	unsigned short	phnum;
	unsigned short	shentsize;
	unsigned short	shnum;
	unsigned short	shstrndx;
	unsigned int	version;
	unsigned int	flags;
	unsigned long	entry;
	unsigned long	phoff;
	unsigned long	shoff;
};

struct elf_shdr {
	unsigned int	name;
	unsigned int	type;
	unsigned long	flags;
	unsigned long	addr;
	unsigned long	offset;
	unsigned long	size;
	unsigned int	link;
	unsigned int	info;
	unsigned long	addralign;
	unsigned long	entsize;
};

struct elf_phdr {
	unsigned int	type;
	unsigned int	flags;
	unsigned long	offset;
	unsigned long	vaddr;
	unsigned long	paddr;
	unsigned long	filesz;
	unsigned long	memsz;
	unsigned long	align;
};

/*
 * The following macro can work only if struct elf32hdr and struct elf64hdr
 * have the same member names.
 *
 * The same applies to LOAD_ELFPHDR() and LOAD_ELFSHDR().
 */
#define LOAD_ELFHDR(eh, ehxx) \
	do { \
		memcpy((eh)->ident, (ehxx)->e_ident, EI_NIDENT); \
		(eh)->type = (ehxx)->e_type; \
		(eh)->machine = (ehxx)->e_machine; \
		(eh)->version = (ehxx)->e_version; \
		(eh)->entry = (ehxx)->e_entry; \
		(eh)->phoff = (ehxx)->e_phoff; \
		(eh)->shoff = (ehxx)->e_shoff; \
		(eh)->flags = (ehxx)->e_flags; \
		(eh)->ehsize = (ehxx)->e_ehsize; \
		(eh)->phentsize = (ehxx)->e_phentsize; \
		(eh)->phnum = (ehxx)->e_phnum; \
		(eh)->shentsize = (ehxx)->e_shentsize; \
		(eh)->shnum = (ehxx)->e_shnum; \
		(eh)->shstrndx = (ehxx)->e_shstrndx; \
	} while (0)
#define LOAD_ELFPHDR(ph, phxx) \
	do { \
		(ph)->type = (phxx)->p_type; \
		(ph)->offset = (phxx)->p_offset; \
		(ph)->vaddr = (phxx)->p_vaddr; \
		(ph)->paddr = (phxx)->p_paddr; \
		(ph)->filesz = (phxx)->p_filesz; \
		(ph)->memsz = (phxx)->p_memsz; \
		(ph)->flags = (phxx)->p_flags; \
		(ph)->align = (phxx)->p_align; \
	} while (0)
#define LOAD_ELFSHDR(sh, shxx) \
	do { \
		(sh)->name = (shxx)->sh_name; \
		(sh)->type = (shxx)->sh_type; \
		(sh)->flags = (shxx)->sh_flags; \
		(sh)->addr = (shxx)->sh_addr; \
		(sh)->offset = (shxx)->sh_offset; \
		(sh)->size = (shxx)->sh_size; \
		(sh)->link = (shxx)->sh_link; \
		(sh)->info = (shxx)->sh_info; \
		(sh)->addralign = (shxx)->sh_addralign; \
		(sh)->entsize = (shxx)->sh_entsize; \
	} while (0)

#endif

