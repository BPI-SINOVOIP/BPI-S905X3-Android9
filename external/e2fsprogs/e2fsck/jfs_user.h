/*
 * Compatibility header file for e2fsck which should be included
 * instead of linux/jfs.h
 *
 * Copyright (C) 2000 Stephen C. Tweedie
 *
 * This file may be redistributed under the terms of the
 * GNU General Public License version 2 or at your discretion
 * any later version.
 */
#ifndef _JFS_USER_H
#define _JFS_USER_H

#ifdef DEBUGFS
#include <stdio.h>
#include <stdlib.h>
#if EXT2_FLAT_INCLUDES
#include "ext2_fs.h"
#include "ext2fs.h"
#include "blkid.h"
#else
#include "ext2fs/ext2_fs.h"
#include "ext2fs/ext2fs.h"
#include "blkid/blkid.h"
#endif
#else
/*
 * Pull in the definition of the e2fsck context structure
 */
#include "e2fsck.h"
#endif

struct buffer_head {
#ifdef DEBUGFS
	ext2_filsys	b_fs;
#else
	e2fsck_t	b_ctx;
#endif
	io_channel	b_io;
	int		b_size;
	int		b_err;
	unsigned int	b_dirty:1;
	unsigned int	b_uptodate:1;
	unsigned long long b_blocknr;
	char		b_data[1024];
};

struct inode {
#ifdef DEBUGFS
	ext2_filsys	i_fs;
#else
	e2fsck_t	i_ctx;
#endif
	ext2_ino_t	i_ino;
	struct ext2_inode i_ext2;
};

struct kdev_s {
#ifdef DEBUGFS
	ext2_filsys	k_fs;
#else
	e2fsck_t	k_ctx;
#endif
	int		k_dev;
};

#define K_DEV_FS	1
#define K_DEV_JOURNAL	2

#define lock_buffer(bh) do {} while (0)
#define unlock_buffer(bh) do {} while (0)
#define buffer_req(bh) 1
#define do_readahead(journal, start) do {} while (0)

typedef struct {
	int	object_length;
} lkmem_cache_t;

#define kmem_cache_alloc(cache, flags) malloc((cache)->object_length)
#define kmem_cache_free(cache, obj) free(obj)
#define kmem_cache_create(name, len, a, b, c) do_cache_create(len)
#define kmem_cache_destroy(cache) do_cache_destroy(cache)
#define kmalloc(len, flags) malloc(len)
#define kfree(p) free(p)

#define cond_resched()	do { } while (0)

#define __init

/*
 * Now pull in the real linux/jfs.h definitions.
 */
#include <ext2fs/kernel-jbd.h>

/*
 * We use the standard libext2fs portability tricks for inline
 * functions.
 */
#ifdef NO_INLINE_FUNCS
extern lkmem_cache_t *do_cache_create(int len);
extern void do_cache_destroy(lkmem_cache_t *cache);
extern size_t journal_tag_bytes(journal_t *journal);
#endif

#if (defined(E2FSCK_INCLUDE_INLINE_FUNCS) || !defined(NO_INLINE_FUNCS))
#ifdef E2FSCK_INCLUDE_INLINE_FUNCS
#if (__STDC_VERSION__ >= 199901L)
#define _INLINE_ extern inline
#else
#define _INLINE_ inline
#endif
#else /* !E2FSCK_INCLUDE_INLINE FUNCS */
#if (__STDC_VERSION__ >= 199901L)
#define _INLINE_ inline
#else /* not C99 */
#ifdef __GNUC__
#define _INLINE_ extern __inline__
#else				/* For Watcom C */
#define _INLINE_ extern inline
#endif /* __GNUC__ */
#endif /* __STDC_VERSION__ >= 199901L */
#endif /* E2FSCK_INCLUDE_INLINE_FUNCS */


_INLINE_ lkmem_cache_t *do_cache_create(int len)
{
	lkmem_cache_t *new_cache;

	new_cache = malloc(sizeof(*new_cache));
	if (new_cache)
		new_cache->object_length = len;
	return new_cache;
}

_INLINE_ void do_cache_destroy(lkmem_cache_t *cache)
{
	free(cache);
}

#undef _INLINE_
#endif

/*
 * Kernel compatibility functions are defined in journal.c
 */
int journal_bmap(journal_t *journal, blk64_t block, unsigned long long *phys);
struct buffer_head *getblk(kdev_t ctx, blk64_t blocknr, int blocksize);
int sync_blockdev(kdev_t kdev);
void ll_rw_block(int rw, int dummy, struct buffer_head *bh[]);
void mark_buffer_dirty(struct buffer_head *bh);
void mark_buffer_uptodate(struct buffer_head *bh, int val);
void brelse(struct buffer_head *bh);
int buffer_uptodate(struct buffer_head *bh);
void wait_on_buffer(struct buffer_head *bh);

/*
 * Define newer 2.5 interfaces
 */
#define __getblk(dev, blocknr, blocksize) getblk(dev, blocknr, blocksize)
#define set_buffer_uptodate(bh) mark_buffer_uptodate(bh, 1)

#ifdef DEBUGFS
#include <assert.h>
#undef J_ASSERT
#define J_ASSERT(x)	assert(x)

#define JSB_HAS_INCOMPAT_FEATURE(jsb, mask)				\
	((jsb)->s_header.h_blocktype == ext2fs_cpu_to_be32(JFS_SUPERBLOCK_V2) &&	\
	 ((jsb)->s_feature_incompat & ext2fs_cpu_to_be32((mask))))
#else  /* !DEBUGFS */

extern e2fsck_t e2fsck_global_ctx;  /* Try your very best not to use this! */

#define J_ASSERT(assert)						\
	do { if (!(assert)) {						\
		printf ("Assertion failure in %s() at %s line %d: "	\
			"\"%s\"\n",					\
			__FUNCTION__, __FILE__, __LINE__, # assert);	\
		fatal_error(e2fsck_global_ctx, 0);			\
	} } while (0)

#endif /* DEBUGFS */

#ifndef EFSBADCRC
#define EFSBADCRC	EXT2_ET_BAD_CRC
#endif
#ifndef EFSCORRUPTED
#define EFSCORRUPTED	EXT2_ET_FILESYSTEM_CORRUPTED
#endif

/* recovery.c */
extern int	journal_recover    (journal_t *journal);
extern int	journal_skip_recovery (journal_t *);

/* revoke.c */
extern int	journal_init_revoke(journal_t *, int);
extern void	journal_destroy_revoke(journal_t *);
extern void	journal_destroy_revoke_caches(void);
extern int	journal_init_revoke_caches(void);

extern int	journal_set_revoke(journal_t *, unsigned long long, tid_t);
extern int	journal_test_revoke(journal_t *, unsigned long long, tid_t);
extern void	journal_clear_revoke(journal_t *);

#endif /* _JFS_USER_H */
