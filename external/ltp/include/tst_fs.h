/*
 * Copyright (c) 2015-2016 Cyril Hrubis <chrubis@suse.cz>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TST_FS_H__
#define TST_FS_H__

/* man 2 statfs or kernel-source/include/linux/magic.h */
#define TST_BTRFS_MAGIC    0x9123683E
#define TST_NFS_MAGIC      0x6969
#define TST_RAMFS_MAGIC    0x858458f6
#define TST_TMPFS_MAGIC    0x01021994
#define TST_V9FS_MAGIC     0x01021997
#define TST_XFS_MAGIC      0x58465342
#define TST_EXT2_OLD_MAGIC 0xEF51
/* ext2, ext3, ext4 have the same magic number */
#define TST_EXT234_MAGIC   0xEF53
#define TST_MINIX_MAGIC    0x137F
#define TST_MINIX_MAGIC2   0x138F
#define TST_MINIX2_MAGIC   0x2468
#define TST_MINIX2_MAGIC2  0x2478
#define TST_MINIX3_MAGIC   0x4D5A
#define TST_UDF_MAGIC      0x15013346
#define TST_SYSV2_MAGIC    0x012FF7B6
#define TST_SYSV4_MAGIC    0x012FF7B5
#define TST_UFS_MAGIC      0x00011954
#define TST_UFS2_MAGIC     0x19540119
#define TST_F2FS_MAGIC     0xF2F52010
#define TST_NILFS_MAGIC    0x3434
#define TST_EXOFS_MAGIC    0x5DF5

enum {
	TST_BYTES = 1,
	TST_KB = 1024,
	TST_MB = 1048576,
	TST_GB = 1073741824,
};

/*
 * @path: path is the pathname of any file within the mounted file system
 * @mult: mult should be TST_KB, TST_MB or TST_GB
 * the required free space is calculated by @size * @mult
 */
int tst_fs_has_free_(void (*cleanup)(void), const char *path,
                     unsigned int size, unsigned int mult);

/*
 * Returns filesystem magick for a given path.
 *
 * The expected usage is:
 *
 *      if (tst_fs_type(cleanup, ".") == TST_NFS_MAGIC) {
 *		tst_brkm(TCONF, cleanup,
 *		         "Test not supported on NFS filesystem");
 *	}
 *
 * Or:
 *
 *	long type;
 *
 *	swtich ((type = tst_fs_type(cleanup, "."))) {
 *	case TST_NFS_MAGIC:
 *	case TST_TMPFS_MAGIC:
 *	case TST_RAMFS_MAGIC:
 *		tst_brkm(TCONF, cleanup, "Test not supported on %s filesystem",
 *		         tst_fs_type_name(type));
 *	break;
 *	}
 */
long tst_fs_type_(void (*cleanup)(void), const char *path);

/*
 * Returns filesystem name given magic.
 */
const char *tst_fs_type_name(long f_type);

/*
 * Try to get maximum number of hard links to a regular file inside the @dir.
 *
 * Note: This number depends on the filesystem @dir is on.
 *
 * The code uses link(2) to create hard links to a single file until it gets
 * EMLINK or creates 65535 links.
 *
 * If limit is hit maximal number of hardlinks is returned and the the @dir is
 * filled with hardlinks in format "testfile%i" where i belongs to [0, limit)
 * interval.
 *
 * If no limit is hit (succed to create 65535 without error) or if link()
 * failed with ENOSPC or EDQUOT zero is returned previously created files are
 * removed.
 */
int tst_fs_fill_hardlinks_(void (*cleanup) (void), const char *dir);

/*
 * Try to get maximum number of subdirectories in directory.
 *
 * Note: This number depends on the filesystem @dir is on.
 *
 * The code uses mkdir(2) to create directories in @dir until it gets EMLINK
 * or creates 65535 directories.
 *
 * If limit is hit the maximal number of subdirectories is returned and the
 * @dir is filled with subdirectories in format "testdir%i" where i belongs to
 * [0, limit - 2) interval (because each newly created dir has two links
 * already the '.' and link from parent dir).
 *
 * If no limit is hit or mkdir() failed with ENOSPC or EDQUOT zero is returned
 * previously created directories are removed.
 *
 */
int tst_fs_fill_subdirs_(void (*cleanup) (void), const char *dir);

/*
 * Checks if a given directory contains any entities,
 * returns 1 if directory is empty, 0 otherwise
 */
int tst_dir_is_empty_(void (*cleanup)(void), const char *name, int verbose);

/*
 * Search $PATH for prog_name and fills buf with absolute path if found.
 *
 * Returns -1 on failure, either command was not found or buffer was too small.
 */
int tst_get_path(const char *prog_name, char *buf, size_t buf_len);

/*
 * Creates/ovewrites a file with specified pattern
 * @path: path to file
 * @pattern: pattern
 * @bs: block size
 * @bcount: blocks amount
 */
int tst_fill_file(const char *path, char pattern, size_t bs, size_t bcount);

/*
 * Returns NULL-terminated array of kernel-supported filesystems.
 */
const char **tst_get_supported_fs_types(void);

/*
 * Creates and writes to files on given path until write fails with ENOSPC
 */
void tst_fill_fs(const char *path, int verbose);

#ifdef TST_TEST_H__
static inline long tst_fs_type(const char *path)
{
	return tst_fs_type_(NULL, path);
}

static inline int tst_fs_has_free(const char *path, unsigned int size,
                                  unsigned int mult)
{
	return tst_fs_has_free_(NULL, path, size, mult);
}

static inline int tst_fs_fill_hardlinks(const char *dir)
{
	return tst_fs_fill_hardlinks_(NULL, dir);
}

static inline int tst_fs_fill_subdirs(const char *dir)
{
	return tst_fs_fill_subdirs_(NULL, dir);
}

static inline int tst_dir_is_empty(const char *name, int verbose)
{
	return tst_dir_is_empty_(NULL, name, verbose);
}
#else
static inline long tst_fs_type(void (*cleanup)(void), const char *path)
{
	return tst_fs_type_(cleanup, path);
}

static inline int tst_fs_has_free(void (*cleanup)(void), const char *path,
                                  unsigned int size, unsigned int mult)
{
	return tst_fs_has_free_(cleanup, path, size, mult);
}

static inline int tst_fs_fill_hardlinks(void (*cleanup)(void), const char *dir)
{
	return tst_fs_fill_hardlinks_(cleanup, dir);
}

static inline int tst_fs_fill_subdirs(void (*cleanup)(void), const char *dir)
{
	return tst_fs_fill_subdirs_(cleanup, dir);
}

static inline int tst_dir_is_empty(void (*cleanup)(void), const char *name, int verbose)
{
	return tst_dir_is_empty_(cleanup, name, verbose);
}
#endif

#endif	/* TST_FS_H__ */
