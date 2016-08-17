/*
 * LICENSE NOTES:
 * Since most i386 code comes from MIT xv6, I wonder how we should put the
 * license information...
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <asm.h>
#include <elf.h>
#include <bootsect.h>
#include <sys/types.h>
#include <sys/param.h>
#include <drivers/ata/ata.h>

/* ELF buffer address for storing read headers */
#define ELF_BUFFER	0x10000L

#define IDE_PORTBASE	0x1f0

/* TODO: Should be merged into config.h? */
#define PGSIZE		4096

/* Convenient and readable macros instead of raw inb() and outb() */
#define IDE_PIO_READ(reg)		inb(IDE_PORTBASE + (reg))
#define IDE_PIO_WRITE(reg, data)	outb(IDE_PORTBASE + (reg), data)
#define IDE_PIO_FETCH(dst)	\
	insl(IDE_PORTBASE + ATA_REG_DATA, dst, SECTOR_SIZE / 4)

static inline void
waitdisk(void)
{
	while ((IDE_PIO_READ(ATA_REG_STATUS) & (ATA_BUSY | ATA_DRDY)) !=
	    ATA_DRDY)
		/* nothing */;
}

static void
readsect(void *dst, size_t sector)
{
	/* For bootloaders, the code should be as compact as possible. */
	waitdisk();

	/* A typical ATA command sequence.  Pretty self-explanatory. */
	IDE_PIO_WRITE(ATA_REG_NSECT, 1);
	IDE_PIO_WRITE(ATA_REG_LBAL, sector & 0xff);
	IDE_PIO_WRITE(ATA_REG_LBAM, (sector >> 8) & 0xff);
	IDE_PIO_WRITE(ATA_REG_LBAH, (sector >> 16) & 0xff);
	IDE_PIO_WRITE(ATA_REG_DEVSEL,
	    ((sector >> 24) & 0xff) | (ATA_DEVICE_OBS | ATA_LBA));
	IDE_PIO_WRITE(ATA_REG_CMD, ATA_CMD_PIO_READ);

	waitdisk();

	IDE_PIO_FETCH(dst);
}

/*
 * Read @count bytes from disk at @offset bytes to @pa.
 */
static void
readseg(unsigned char *pa, size_t count, size_t offset)
{
	unsigned char *epa = pa + count;
	unsigned int sector = offset / SECTOR_SIZE;

	pa -= offset % SECTOR_SIZE;

	for (; pa < epa; pa += SECTOR_SIZE, ++sector)
		readsect(pa, sector);
}

void
bootmain(void)
{
	struct elf32hdr *elf = (struct elf32hdr *)ELF_BUFFER;
	struct elf32_phdr *ph, *eph;
	void (*entry)(void);
	unsigned char *pa;
	/* Here 'start' points to the assembler entrance, whose memory
	 * address is 0x7c00. */
	extern unsigned char *start;
	/* By executing this piece of code, MBR is already loaded into
	 * memory. */
	struct mbr *mbr = (struct mbr *)&start;
	size_t part_offset = mbr->part_entry[1].first_sector_lba * SECTOR_SIZE;

	/* Read a reasonably sized block to fetch all headers we need */
	readseg((unsigned char *)elf, PGSIZE, part_offset);

	ph = (struct elf32_phdr *)(ELF_BUFFER + elf->e_phoff);
	eph = ph + elf->e_phnum;

	for (; ph < eph; ++ph) {
		if (ph->p_type == PT_LOAD) {
			pa = (unsigned char *)(ph->p_paddr);
			readseg(pa, ph->p_filesz, part_offset + ph->p_offset);
		}
	}

	/* Jump to ELF entry... */
	entry = (void (*)(void))(elf->e_entry);
	entry();
}
