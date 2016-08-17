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
 * This page table implementation is rather inefficient, hence not for
 * production purposes.
 */

/*
 * MIPS does not have a page index walker like ARM or i386, so we have
 * the freedom to design our own page table.
 *
 * In 32-bit implementation, the page table is a standard 2-level hierarchical
 * table.
 *
 * The entries for all directories except leaf page tables are kernel virtual
 * addresses pointing to next-level directories:
 * 33222222222211111111110000000000
 * 10987654321098765432109876543210
 * +--4K-aligned virtual address--+
 *
 * The leaf page tables stores PFN and additional information in the
 * following format:
 * 33222222222211111111 110 0 0 0 000000
 * 10987654321098765432 109 8 7 6 543210
 * +-------PFN--------+ CCF D V G 00X000
 *
 * pgindex_t is the type of *virtual* address to the page table structure.
 *
 * In 64-bit implementation we have a 4-level page table, called page global
 * directory (PGD), page upper directory (PUD), page middle directory (PMD),
 * and page table entry (PTE), from top to bottom.
 */

#include <mm.h>
#include <pmm.h>
#include <vmm.h>
#include <mmu.h>
#include <pgtable.h>
#include <libc/string.h>
#include <errno.h>
#include <sys/types.h>
#include <util.h>
#include <mp.h>
#include <tlb.h>
#include <aim/sync.h>

pgindex_t *pgdir_slots[MAX_CPUS];

#ifndef __LP64__	/* 32 bit */

struct pagedesc {
	uint32_t	pdev;	/* page directory physical address */
	uint32_t	ptev;	/* leaf page table physical address */
	int		pdx;
	int		ptx;
};

/*
 * Get a page index descriptor (PDE, PTE in our case) for the page table,
 * possibly creating leaf page tables if needed.
 */
static int 
__getpagedesc(pgindex_t *pgindex,
	      void *addr,
	      bool create,
	      struct pagedesc *pd)
{
	addr_t paddr;
	pde_t *pde = (pde_t *)pgindex;
	pd->pdev = (uint32_t)pgindex;
	pd->pdx = PDX(addr);
	if (pde[pd->pdx] != 0) {
		/* We already have the intermediate directory */
		pd->ptev = pde[pd->pdx];
	} else {
		/* We don't have it, fail or create one */
		if (!create) {
			return -ENOENT;
		}
		if ((paddr = pgalloc()) == -1)
			return -ENOMEM;
		pd->ptev = (uint32_t)pa2kva(paddr);
		memset((void *)pd->ptev, 0, PAGE_SIZE);
		pde[pd->pdx] = pd->ptev;
	}
	pd->ptx = PTX(addr);
	return 0;
}

/*
 * This function assumes that:
 * 1. (Leaf) page table entries for vaddr..vaddr+size are already zero.
 * 2. @vaddr and @size are page-aligned.
 */
static void
__free_intermediate_pgtable(pgindex_t *pgindex, void *vaddr, size_t size)
{
	pde_t *pde = (pde_t *)pgindex;
	int pdx = PDX(vaddr), pdx_end = PDX(vaddr + size - PAGE_SIZE);
	for (; pdx <= pdx_end; ++pdx) {
		pte_t *pte = (pte_t *)pde[pdx];
		for (int i = 0; i < NR_PTENTRIES; ++i) {
			if (pte[i] != 0)
				goto rollback_next_pde;
		}
		pgfree(kva2pa((void *)pde[pdx]));
		pde[pdx] = 0;
rollback_next_pde:
		/* nothing */;
	}
}

#else	/* 64 bit */

/*
 * 64-bit page table implementation is similar to 32-bit in principle.
 * See the 32-bit code for description of struct pagedesc as well as the
 * internal functions (as they are simpler).
 */

struct pagedesc {
	uint64_t	pgdv;
#if PGTABLE_LEVEL == 4
	uint64_t	pudv;
#endif
	uint64_t	pmdv;
	uint64_t	ptev;
	int		pgx;
#if PGTABLE_LEVEL == 4
	int		pux;
#endif
	int		pmx;
	int		ptx;
};

static void *
__addpgdir(uint64_t *vpgdir, int index)
{
	addr_t paddr = pgalloc();
	if (paddr == -1)
		return NULL;
	pmemset(paddr, 0, PAGE_SIZE);
	vpgdir[index] = (uint64_t)pa2kva(paddr);
	return pa2kva(paddr);
}

static void
__delpgdir(uint64_t *vpgdir, int index)
{
	pgfree(kva2pa((void *)(vpgdir[index])));
	vpgdir[index] = 0;
}

static int
__del_empty_pgdir(uint64_t *vpgdir, int index)
{
	uint64_t *vsubpgdir = (uint64_t *)vpgdir[index];
	for (int i = 0; i < NR_PTENTRIES; ++i) {
		if (vsubpgdir[i] == 0)
			return 1;
	}
	__delpgdir(vpgdir, index);
	return 0;
}

static int 
__getpagedesc(pgindex_t *pgindex,
	      void *addr,
	      bool create,
	      struct pagedesc *pd)
{
	pgd_t *pgd = (pgd_t *)pgindex;
#if PGTABLE_LEVEL == 4
	pud_t *pud = NULL;
#endif
	pmd_t *pmd = NULL;
	pte_t *pte = NULL;
	pd->pgdv = (uint64_t)pgd;
	pd->pgx = PGX(addr);
#if PGTABLE_LEVEL == 4
	pd->pux = PUX(addr);
#endif
	pd->pmx = PMX(addr);
	pd->ptx = PTX(addr);

	if (!create) {
		if (
#if PGTABLE_LEVEL == 4
		    !(pud = (pud_t *)(pd->pudv = pgd[pd->pgx])) ||
		    !(pmd = (pmd_t *)(pd->pmdv = pud[pd->pux])) ||
#else
		    !(pmd = (pmd_t *)(pd->pmdv = pgd[pd->pgx])) ||
#endif
		    !(pte = (pte_t *)(pd->ptev = pmd[pd->pmx])))
			return -ENOENT;
		else
			return 0;
	}

	if (pgd[pd->pgx] == 0 && __addpgdir(pgd, pd->pgx) == NULL)
		goto nomem;
#if PGTABLE_LEVEL == 4
	pud = (pud_t *)(pd->pudv = pgd[pd->pgx]);
	if (pud[pd->pux] == 0 && __addpgdir(pud, pd->pux) == NULL)
		goto nomem;
#endif
#if PGTABLE_LEVEL == 4
	pmd = (pmd_t *)(pd->pmdv = pud[pd->pux]);
#else
	pmd = (pmd_t *)(pd->pmdv = pgd[pd->pgx]);
#endif
	if (pmd[pd->pmx] == 0 && __addpgdir(pmd, pd->pmx) == NULL)
		goto nomem;
	pte = (pte_t *)(pd->ptev = pmd[pd->pmx]);

	return 0;

nomem:
	assert(pmd != NULL);
#if PGTABLE_LEVEL == 4
	assert(pud != NULL);
#endif
	__del_empty_pgdir(pmd, pd->pmx);
#if PGTABLE_LEVEL == 4
	__del_empty_pgdir(pud, pd->pux);
#endif
	__del_empty_pgdir(pgd, pd->pgx);
	return -ENOMEM;
}

static void
__free_intermediate_pgtable(pgindex_t *pgindex, void *vaddr, size_t size)
{
	/* A very inefficient implementation */
	pgd_t *pgd = (pgd_t *)pgindex;
#if PGTABLE_LEVEL == 4
	pud_t *pud;
#endif
	pmd_t *pmd;
	pte_t *pte;
	struct pagedesc pd;

	for (int sz = 0; sz < size; sz += PAGE_SIZE) {
		__getpagedesc(pgindex, vaddr + sz, false, &pd);
		pte = (pte_t *)pd.ptev;
		pmd = (pmd_t *)pd.pmdv;
#if PGTABLE_LEVEL == 4
		pud = (pud_t *)pd.pudv;
#endif
		if (__del_empty_pgdir(pmd, pd.pmx) ||
#if PGTABLE_LEVEL == 4
		    __del_empty_pgdir(pud, pd.pux) ||
#endif
		    __del_empty_pgdir(pgd, pd.pgx))
			continue;
	}
}

#endif	/* !__LP64__ */

pgindex_t *
init_pgindex(void)
{
	addr_t paddr = pgalloc();
	if (paddr == -1)
		return NULL;

	pmemset(paddr, 0, PAGE_SIZE);

	return pa2kva(paddr);
}

void
destroy_pgindex(pgindex_t *pgindex)
{
	pgfree(kva2pa(pgindex));
}

/* TODO: put this into a header */
extern uint32_t __mach_pgtable_perm(uint32_t vma_flags);

static uint32_t
__pgtable_perm(uint32_t vma_flags)
{
	/*
	 * Some machines (e.g. Loongson 2F) may specify additional flags
	 * (e.g. NOEXEC) to supplement the flags defined by MIPS
	 * standard.
	 *
	 * We will have to rely on machine-specific internal function, so
	 * I prepended the function with double underscores to indicate
	 * that this function should not be called elsewhere.
	 */
	uint32_t flags = PTE_VALID | PTE_GLOBAL | PTE_CACHEABLE |
		((vma_flags & VMA_WRITE) ? PTE_DIRTY : 0) |
		__mach_pgtable_perm(vma_flags);
	return flags;
}

int
map_pages(pgindex_t *pgindex,
	  void *vaddr,
	  addr_t paddr,
	  size_t size,
	  uint32_t flags)
{
	struct pagedesc pd;
	int retcode;
	pte_t *pte;
	addr_t pend = paddr + size;
	addr_t pcur = paddr;
	void *vcur = vaddr;

	if (!IS_ALIGNED(paddr, PAGE_SIZE) ||
	    !IS_ALIGNED(size, PAGE_SIZE) ||
	    !PTR_IS_ALIGNED(vaddr, PAGE_SIZE))
		return -EINVAL;

	/*
	 * 1st pass: allocate leaf page tables if needed, and validate
	 * if there are any conflicts or memory shortage, in either case
	 * we need to rollback.
	 */
	for (; pcur < pend; pcur += PAGE_SIZE, vcur += PAGE_SIZE) {
		retcode = __getpagedesc(pgindex, vcur, true, &pd);
		if (retcode == -ENOMEM)
			goto rollback;

		pte = (pte_t *)pd.ptev;
		if (pte[pd.ptx] != 0) {
			/* we are mapping on the exact same virtual
			 * page which is either valid or invalid (paged
			 * out), fail */
			retcode = -EEXIST;
			goto rollback;
		}
	}

	/* 2nd pass: fill the entries as there shouldn't be any failure */
	pcur = paddr;
	vcur = vaddr;
	for (; pcur < pend; pcur += PAGE_SIZE, vcur += PAGE_SIZE) {
		__getpagedesc(pgindex, vcur, false, &pd);
		pte = (pte_t *)pd.ptev;
		pte[pd.ptx] = pcur | __pgtable_perm(flags);
	}

	return 0;

rollback:
	__free_intermediate_pgtable(pgindex, vaddr, vcur - vaddr);
	return retcode;
}

ssize_t
unmap_pages(pgindex_t *pgindex,
	    void *vaddr,
	    size_t size,
	    addr_t *paddr)
{
	void *vcur = vaddr, *vend = vaddr + size;
	ssize_t unmapped_bytes = 0;
	struct pagedesc pd;
	pte_t *pte;
	addr_t pcur = 0;

	for (; vcur < vend; vcur += PAGE_SIZE,
			    unmapped_bytes += PAGE_SIZE,
			    pcur += PAGE_SIZE) {
		if (__getpagedesc(pgindex, vcur, false, &pd) < 0)
			/* may return -ENOENT? */
			panic("unmap_pages non-existent: %p %p\n",
			    pgindex, vcur);
		pte = (pte_t *)pd.ptev;
		if (unmapped_bytes == 0) {
			/* unmapping the first page: store the physical
			 * address */
			pcur = PTE_PADDR(pte[pd.ptx]);
			if (paddr != NULL)
				*paddr = pcur;
		} else if (pte[pd.ptx] != pcur) {
			break;
		}
		pte[pd.ptx] = 0;
	}

	__free_intermediate_pgtable(pgindex, vaddr, unmapped_bytes);

	return unmapped_bytes;
}

int
set_pages_perm(pgindex_t *pgindex, void *addr, size_t len, uint32_t flags)
{
	size_t i;
	pte_t *pte;
	struct pagedesc pd;

	for (i = 0; i < len; i += PAGE_SIZE, addr += PAGE_SIZE) {
		if (__getpagedesc(pgindex, addr, false, &pd) < 0)
			/* may return -ENOENT? */
			panic("change_pages_perm non-existent: %p %p\n",
			    pgindex, addr);
		pte = (pte_t *)pd.ptev;
		pte[pd.ptx] &= ~PTE_LOWMASK;
		pte[pd.ptx] |= __pgtable_perm(flags);
	}

	return 0;
}

int
switch_pgindex(pgindex_t *pgindex)
{
	unsigned long flags;

	local_irq_save(flags);
	current_pgdir = pgindex;
	tlb_flush();
	local_irq_restore(flags);

	return 0;
}

pgindex_t *
get_pgindex(void)
{
	return current_pgdir;
}

void *
uva2kva(pgindex_t *pgindex, void *uaddr)
{
	struct pagedesc pd;
	pte_t *pte;

	if (__getpagedesc(pgindex, uaddr, false, &pd) < 0)
		return NULL;
	pte = (pte_t *)pd.ptev;
	return pa2kva(PTE_PADDR(pte[pd.ptx]) | PAGE_OFFSET(uaddr));
}

