
#include <fs/vnode.h>
#include <fs/ufs/ext2fs/ext2fs.h>
#include <fs/bio.h>
#include <aim/console.h>
#include <bitmap.h>

/* temporary, will be removed in release */
void
__ext2fs_dump(struct vnode *devvp, struct m_ext2fs *fs)
{
	int i, err, n_zero, n_one;
	struct buf *bp = NULL;
	/*
	 * Please, do check the following dumps with the ones from dumpe2fs(8).
	 */
	kprintf("ext2 inode count: %d\n", fs->e2fs.icount);
	kprintf("ext2 block count: %d\n", fs->e2fs.bcount);
	kprintf("ext2 reserved blocks count: %d\n", fs->e2fs.rbcount);
	kprintf("ext2 free blocks count: %d\n", fs->e2fs.fbcount);
	kprintf("ext2 free inode count: %d\n", fs->e2fs.ficount);
	kprintf("ext2 first data block: %d\n", fs->e2fs.first_dblock);
	kprintf("ext2 blocks per group: %d\n", fs->e2fs.bpg);
	kprintf("ext2 frags per group: %d\n", fs->e2fs.fpg);
	kprintf("ext2 inodes per group: %d\n", fs->e2fs.ipg);
	kprintf("ext2 revision: %d.%d\n", fs->e2fs.rev, fs->e2fs.minrev);
	kprintf("ext2 frag size: %d\n", fs->fsize);
	kprintf("ext2 block size: %d\n", fs->bsize);
	kprintf("ext2 # cylinder groups: %d\n", fs->ncg);
	kprintf("ext2 # group descriptor block: %d\n", fs->ngdb);
	kprintf("ext2 # inodes per block: %d\n", fs->ipb);
	kprintf("ext2 # inode tables per group: %d\n", fs->itpg);
	for (i = 0; i < fs->ncg; ++i) {
		/* XXX twisted: on ext2 with 1KB logical blocks, super blocks
		 * reside on block 1, pushing everything one block away. */
		size_t sb_off = (fs->bsize == 1024) ? 1 : 0;
		size_t block_base = fs->e2fs.bpg * i + sb_off;
		size_t inode_base = fs->e2fs.ipg * i;
		kprintf("ext2 Group #%d\n", i);
		kprintf("\tsuperblock backup at %d\n", block_base);
		kprintf("\tblock group desc at %d\n", block_base + 1);
		kprintf("\tblocks bitmap at %d\n", fs->gd[i].b_bitmap);
		kprintf("\tinode bitmap at %d\n", fs->gd[i].i_bitmap);
		kprintf("\tinode tables at %d-%d\n", fs->gd[i].i_tables,
		    fs->gd[i].i_tables + fs->itpg - 1);
		kprintf("\t# free blocks: %d\n", fs->gd[i].nbfree);
		kprintf("\t# free inodes: %d\n", fs->gd[i].nifree);
		kprintf("\t# directories: %d\n", fs->gd[i].ndirs);

		/* Process ext2fs block bitmap: 0 - available, 1 - used */
		err = bread(devvp, fsbtodb(fs, fs->gd[i].b_bitmap), fs->bsize,
		    &bp);
		if (err != 0) {
			kprintf("\tbread error\n");
			brelse(bp);
			return;
		}
		/* XXX this piece of code is twisted: bitmap_find_first() and
		 * bitmap_find_next() functions are 1-based, and returns 0 when
		 * not found, but the disk block indices are 0-based. */
		n_zero = bitmap_find_first_zero_bit(bp->data, fs->e2fs.bpg);
		do {
			n_one = bitmap_find_next_bit(bp->data, fs->e2fs.bpg, n_zero);
			kprintf("\tfree block set (inclusive): %d-%d\n",
			    n_zero - 1 + block_base,
			    /* - 2 because of right inclusion */
			    n_one ? n_one - 2 + block_base : 
			    min2(block_base + fs->e2fs.bpg - 1, fs->e2fs.bcount - 1));
			n_zero = bitmap_find_next_zero_bit(bp->data, fs->e2fs.bpg, n_one);
		} while (n_zero != 0);
		brelse(bp);
		bp = NULL;

		err = bread(devvp, fsbtodb(fs, fs->gd[i].i_bitmap), fs->bsize,
		    &bp);
		if (err != 0) {
			kprintf("\tbread error\n");
			brelse(bp);
			return;
		}
		/* XXX note that inodes are 1-based */
		n_zero = bitmap_find_first_zero_bit(bp->data, fs->e2fs.ipg);
		do {
			n_one = bitmap_find_next_bit(bp->data, fs->e2fs.ipg, n_zero);
			kprintf("\tfree inode set (inclusive): %d-%d\n",
			    n_zero + inode_base,
			    /* - 1 because of right inclusion */
			    n_one ? n_one - 1 + inode_base :
			    min2(inode_base + fs->e2fs.ipg, fs->e2fs.icount));
			n_zero = bitmap_find_next_zero_bit(bp->data, fs->e2fs.ipg, n_one);
		} while (n_zero != 0);
		brelse(bp);
	}
}

