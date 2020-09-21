/**
 * f2fs_format.c
 *
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *             http://www.samsung.com/
 *
 * Dual licensed under the GPL or LGPL version 2 licenses.
 */
#define _LARGEFILE64_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#ifndef ANDROID_WINDOWS_HOST
#include <sys/stat.h>
#include <sys/mount.h>
#endif
#include <time.h>
#include <uuid/uuid.h>

#include "f2fs_fs.h"
#include "quota.h"
#include "f2fs_format_utils.h"

extern struct f2fs_configuration c;
struct f2fs_super_block raw_sb;
struct f2fs_super_block *sb = &raw_sb;
struct f2fs_checkpoint *cp;

/* Return first segment number of each area */
#define prev_zone(cur)		(c.cur_seg[cur] - c.segs_per_zone)
#define next_zone(cur)		(c.cur_seg[cur] + c.segs_per_zone)
#define last_zone(cur)		((cur - 1) * c.segs_per_zone)
#define last_section(cur)	(cur + (c.secs_per_zone - 1) * c.segs_per_sec)

static unsigned int quotatype_bits = 0;

const char *media_ext_lists[] = {
	"jpg",
	"gif",
	"png",
	"avi",
	"divx",
	"mp4",
	"mp3",
	"3gp",
	"wmv",
	"wma",
	"mpeg",
	"mkv",
	"mov",
	"asx",
	"asf",
	"wmx",
	"svi",
	"wvx",
	"wm",
	"mpg",
	"mpe",
	"rm",
	"ogg",
	"jpeg",
	"video",
	"apk",	/* for android system */
	"so",	/* for android system */
	NULL
};

static bool is_extension_exist(const char *name)
{
	int i;

	for (i = 0; i < F2FS_MAX_EXTENSION; i++) {
		char *ext = (char *)sb->extension_list[i];
		if (!strcmp(ext, name))
			return 1;
	}

	return 0;
}

static void cure_extension_list(void)
{
	const char **extlist = media_ext_lists;
	char *ext_str = c.extension_list;
	char *ue;
	int name_len;
	int i = 0;

	set_sb(extension_count, 0);
	memset(sb->extension_list, 0, sizeof(sb->extension_list));

	while (*extlist) {
		name_len = strlen(*extlist);
		memcpy(sb->extension_list[i++], *extlist, name_len);
		extlist++;
	}
	set_sb(extension_count, i);

	if (!ext_str)
		return;

	/* add user ext list */
	ue = strtok(ext_str, ", ");
	while (ue != NULL) {
		name_len = strlen(ue);
		if (name_len >= 8) {
			MSG(0, "\tWarn: Extension name (%s) is too long\n", ue);
			goto next;
		}
		if (!is_extension_exist(ue))
			memcpy(sb->extension_list[i++], ue, name_len);
next:
		ue = strtok(NULL, ", ");
		if (i >= F2FS_MAX_EXTENSION)
			break;
	}

	set_sb(extension_count, i);

	free(c.extension_list);
}

static void verify_cur_segs(void)
{
	int i, j;
	int reorder = 0;

	for (i = 0; i < NR_CURSEG_TYPE; i++) {
		for (j = i + 1; j < NR_CURSEG_TYPE; j++) {
			if (c.cur_seg[i] == c.cur_seg[j]) {
				reorder = 1;
				break;
			}
		}
	}

	if (!reorder)
		return;

	c.cur_seg[0] = 0;
	for (i = 1; i < NR_CURSEG_TYPE; i++)
		c.cur_seg[i] = next_zone(i - 1);
}

static int f2fs_prepare_super_block(void)
{
	u_int32_t blk_size_bytes;
	u_int32_t log_sectorsize, log_sectors_per_block;
	u_int32_t log_blocksize, log_blks_per_seg;
	u_int32_t segment_size_bytes, zone_size_bytes;
	u_int32_t sit_segments;
	u_int32_t blocks_for_sit, blocks_for_nat, blocks_for_ssa;
	u_int32_t total_valid_blks_available;
	u_int64_t zone_align_start_offset, diff;
	u_int64_t total_meta_zones, total_meta_segments;
	u_int32_t sit_bitmap_size, max_sit_bitmap_size;
	u_int32_t max_nat_bitmap_size, max_nat_segments;
	u_int32_t total_zones;
	u_int32_t next_ino;
	enum quota_type qtype;
	int i;

	set_sb(magic, F2FS_SUPER_MAGIC);
	set_sb(major_ver, F2FS_MAJOR_VERSION);
	set_sb(minor_ver, F2FS_MINOR_VERSION);

	log_sectorsize = log_base_2(c.sector_size);
	log_sectors_per_block = log_base_2(c.sectors_per_blk);
	log_blocksize = log_sectorsize + log_sectors_per_block;
	log_blks_per_seg = log_base_2(c.blks_per_seg);

	set_sb(log_sectorsize, log_sectorsize);
	set_sb(log_sectors_per_block, log_sectors_per_block);

	set_sb(log_blocksize, log_blocksize);
	set_sb(log_blocks_per_seg, log_blks_per_seg);

	set_sb(segs_per_sec, c.segs_per_sec);
	set_sb(secs_per_zone, c.secs_per_zone);

	blk_size_bytes = 1 << log_blocksize;
	segment_size_bytes = blk_size_bytes * c.blks_per_seg;
	zone_size_bytes =
		blk_size_bytes * c.secs_per_zone *
		c.segs_per_sec * c.blks_per_seg;

	set_sb(checksum_offset, 0);

	set_sb(block_count, c.total_sectors >> log_sectors_per_block);

	zone_align_start_offset =
		(c.start_sector * c.sector_size +
		2 * F2FS_BLKSIZE + zone_size_bytes - 1) /
		zone_size_bytes * zone_size_bytes -
		c.start_sector * c.sector_size;

	if (c.start_sector % c.sectors_per_blk) {
		MSG(1, "\t%s: Align start sector number to the page unit\n",
				c.zoned_mode ? "FAIL" : "WARN");
		MSG(1, "\ti.e., start sector: %d, ofs:%d (sects/page: %d)\n",
				c.start_sector,
				c.start_sector % c.sectors_per_blk,
				c.sectors_per_blk);
		if (c.zoned_mode)
			return -1;
	}

	set_sb(segment0_blkaddr, zone_align_start_offset / blk_size_bytes);
	sb->cp_blkaddr = sb->segment0_blkaddr;

	MSG(0, "Info: zone aligned segment0 blkaddr: %u\n",
					get_sb(segment0_blkaddr));

	if (c.zoned_mode && (get_sb(segment0_blkaddr) + c.start_sector /
					c.sectors_per_blk) % c.zone_blocks) {
		MSG(1, "\tError: Unaligned segment0 block address %u\n",
				get_sb(segment0_blkaddr));
		return -1;
	}

	for (i = 0; i < c.ndevs; i++) {
		if (i == 0) {
			c.devices[i].total_segments =
				(c.devices[i].total_sectors *
				c.sector_size - zone_align_start_offset) /
				segment_size_bytes;
			c.devices[i].start_blkaddr = 0;
			c.devices[i].end_blkaddr = c.devices[i].total_segments *
						c.blks_per_seg - 1 +
						sb->segment0_blkaddr;
		} else {
			c.devices[i].total_segments =
				c.devices[i].total_sectors /
				(c.sectors_per_blk * c.blks_per_seg);
			c.devices[i].start_blkaddr =
					c.devices[i - 1].end_blkaddr + 1;
			c.devices[i].end_blkaddr = c.devices[i].start_blkaddr +
					c.devices[i].total_segments *
					c.blks_per_seg - 1;
		}
		if (c.ndevs > 1) {
			memcpy(sb->devs[i].path, c.devices[i].path, MAX_PATH_LEN);
			sb->devs[i].total_segments =
					cpu_to_le32(c.devices[i].total_segments);
		}

		c.total_segments += c.devices[i].total_segments;
	}
	set_sb(segment_count, (c.total_segments / c.segs_per_zone *
						c.segs_per_zone));
	set_sb(segment_count_ckpt, F2FS_NUMBER_OF_CHECKPOINT_PACK);

	set_sb(sit_blkaddr, get_sb(segment0_blkaddr) +
			get_sb(segment_count_ckpt) * c.blks_per_seg);

	blocks_for_sit = SIZE_ALIGN(get_sb(segment_count), SIT_ENTRY_PER_BLOCK);

	sit_segments = SEG_ALIGN(blocks_for_sit);

	set_sb(segment_count_sit, sit_segments * 2);

	set_sb(nat_blkaddr, get_sb(sit_blkaddr) + get_sb(segment_count_sit) *
			c.blks_per_seg);

	total_valid_blks_available = (get_sb(segment_count) -
			(get_sb(segment_count_ckpt) +
			get_sb(segment_count_sit))) * c.blks_per_seg;

	blocks_for_nat = SIZE_ALIGN(total_valid_blks_available,
			NAT_ENTRY_PER_BLOCK);

	set_sb(segment_count_nat, SEG_ALIGN(blocks_for_nat));
	/*
	 * The number of node segments should not be exceeded a "Threshold".
	 * This number resizes NAT bitmap area in a CP page.
	 * So the threshold is determined not to overflow one CP page
	 */
	sit_bitmap_size = ((get_sb(segment_count_sit) / 2) <<
				log_blks_per_seg) / 8;

	if (sit_bitmap_size > MAX_SIT_BITMAP_SIZE)
		max_sit_bitmap_size = MAX_SIT_BITMAP_SIZE;
	else
		max_sit_bitmap_size = sit_bitmap_size;

	/*
	 * It should be reserved minimum 1 segment for nat.
	 * When sit is too large, we should expand cp area. It requires more
	 * pages for cp.
	 */
	if (max_sit_bitmap_size > MAX_SIT_BITMAP_SIZE_IN_CKPT) {
		max_nat_bitmap_size = CHECKSUM_OFFSET -
				sizeof(struct f2fs_checkpoint) + 1;
		set_sb(cp_payload, F2FS_BLK_ALIGN(max_sit_bitmap_size));
	} else {
		max_nat_bitmap_size =
			CHECKSUM_OFFSET - sizeof(struct f2fs_checkpoint) + 1
			- max_sit_bitmap_size;
		set_sb(cp_payload, 0);
	}

	max_nat_segments = (max_nat_bitmap_size * 8) >> log_blks_per_seg;

	if (get_sb(segment_count_nat) > max_nat_segments)
		set_sb(segment_count_nat, max_nat_segments);

	set_sb(segment_count_nat, get_sb(segment_count_nat) * 2);

	set_sb(ssa_blkaddr, get_sb(nat_blkaddr) + get_sb(segment_count_nat) *
			c.blks_per_seg);

	total_valid_blks_available = (get_sb(segment_count) -
			(get_sb(segment_count_ckpt) +
			get_sb(segment_count_sit) +
			get_sb(segment_count_nat))) *
			c.blks_per_seg;

	blocks_for_ssa = total_valid_blks_available /
				c.blks_per_seg + 1;

	set_sb(segment_count_ssa, SEG_ALIGN(blocks_for_ssa));

	total_meta_segments = get_sb(segment_count_ckpt) +
		get_sb(segment_count_sit) +
		get_sb(segment_count_nat) +
		get_sb(segment_count_ssa);
	diff = total_meta_segments % (c.segs_per_zone);
	if (diff)
		set_sb(segment_count_ssa, get_sb(segment_count_ssa) +
			(c.segs_per_zone - diff));

	total_meta_zones = ZONE_ALIGN(total_meta_segments *
						c.blks_per_seg);

	set_sb(main_blkaddr, get_sb(segment0_blkaddr) + total_meta_zones *
				c.segs_per_zone * c.blks_per_seg);

	if (c.zoned_mode) {
		/*
		 * Make sure there is enough randomly writeable
		 * space at the beginning of the disk.
		 */
		unsigned long main_blkzone = get_sb(main_blkaddr) / c.zone_blocks;

		if (c.devices[0].zoned_model == F2FS_ZONED_HM &&
				c.devices[0].nr_rnd_zones < main_blkzone) {
			MSG(0, "\tError: Device does not have enough random "
					"write zones for F2FS volume (%lu needed)\n",
					main_blkzone);
			return -1;
		}
	}

	total_zones = get_sb(segment_count) / (c.segs_per_zone) -
							total_meta_zones;

	set_sb(section_count, total_zones * c.secs_per_zone);

	set_sb(segment_count_main, get_sb(section_count) * c.segs_per_sec);

	/* Let's determine the best reserved and overprovisioned space */
	if (c.overprovision == 0)
		c.overprovision = get_best_overprovision(sb);

	if (c.overprovision == 0 || c.total_segments < F2FS_MIN_SEGMENTS ||
		(c.devices[0].total_sectors *
			c.sector_size < zone_align_start_offset) ||
		(get_sb(segment_count_main) - 2) < c.reserved_segments) {
		MSG(0, "\tError: Device size is not sufficient for F2FS volume\n");
		return -1;
	}

	c.reserved_segments =
			(2 * (100 / c.overprovision + 1) + 6)
			* c.segs_per_sec;

	uuid_generate(sb->uuid);

	/* precompute checksum seed for metadata */
	if (c.feature & cpu_to_le32(F2FS_FEATURE_INODE_CHKSUM))
		c.chksum_seed = f2fs_cal_crc32(~0, sb->uuid, sizeof(sb->uuid));

	utf8_to_utf16(sb->volume_name, (const char *)c.vol_label,
				MAX_VOLUME_NAME, strlen(c.vol_label));
	set_sb(node_ino, 1);
	set_sb(meta_ino, 2);
	set_sb(root_ino, 3);
	next_ino = 4;

	if (c.feature & cpu_to_le32(F2FS_FEATURE_QUOTA_INO)) {
		quotatype_bits = QUOTA_USR_BIT | QUOTA_GRP_BIT;
		if (c.feature & cpu_to_le32(F2FS_FEATURE_PRJQUOTA))
			quotatype_bits |= QUOTA_PRJ_BIT;
	}

	for (qtype = 0; qtype < F2FS_MAX_QUOTAS; qtype++) {
		if (!((1 << qtype) & quotatype_bits))
			continue;
		sb->qf_ino[qtype] = cpu_to_le32(next_ino++);
		MSG(0, "Info: add quota type = %u => %u\n",
					qtype, next_ino - 1);
	}

	if (total_zones <= 6) {
		MSG(1, "\tError: %d zones: Need more zones "
			"by shrinking zone size\n", total_zones);
		return -1;
	}

	if (c.heap) {
		c.cur_seg[CURSEG_HOT_NODE] =
				last_section(last_zone(total_zones));
		c.cur_seg[CURSEG_WARM_NODE] = prev_zone(CURSEG_HOT_NODE);
		c.cur_seg[CURSEG_COLD_NODE] = prev_zone(CURSEG_WARM_NODE);
		c.cur_seg[CURSEG_HOT_DATA] = prev_zone(CURSEG_COLD_NODE);
		c.cur_seg[CURSEG_COLD_DATA] = 0;
		c.cur_seg[CURSEG_WARM_DATA] = next_zone(CURSEG_COLD_DATA);
	} else {
		c.cur_seg[CURSEG_HOT_NODE] = 0;
		c.cur_seg[CURSEG_WARM_NODE] = next_zone(CURSEG_HOT_NODE);
		c.cur_seg[CURSEG_COLD_NODE] = next_zone(CURSEG_WARM_NODE);
		c.cur_seg[CURSEG_HOT_DATA] = next_zone(CURSEG_COLD_NODE);
		c.cur_seg[CURSEG_COLD_DATA] =
				max(last_zone((total_zones >> 2)),
					next_zone(CURSEG_COLD_NODE));
		c.cur_seg[CURSEG_WARM_DATA] =
				max(last_zone((total_zones >> 1)),
					next_zone(CURSEG_COLD_DATA));
	}

	/* if there is redundancy, reassign it */
	verify_cur_segs();

	cure_extension_list();

	/* get kernel version */
	if (c.kd >= 0) {
		dev_read_version(c.version, 0, VERSION_LEN);
		get_kernel_version(c.version);
		MSG(0, "Info: format version with\n  \"%s\"\n", c.version);
	} else {
		get_kernel_uname_version(c.version);
	}

	memcpy(sb->version, c.version, VERSION_LEN);
	memcpy(sb->init_version, c.version, VERSION_LEN);

	sb->feature = c.feature;

	return 0;
}

static int f2fs_init_sit_area(void)
{
	u_int32_t blk_size, seg_size;
	u_int32_t index = 0;
	u_int64_t sit_seg_addr = 0;
	u_int8_t *zero_buf = NULL;

	blk_size = 1 << get_sb(log_blocksize);
	seg_size = (1 << get_sb(log_blocks_per_seg)) * blk_size;

	zero_buf = calloc(sizeof(u_int8_t), seg_size);
	if(zero_buf == NULL) {
		MSG(1, "\tError: Calloc Failed for sit_zero_buf!!!\n");
		return -1;
	}

	sit_seg_addr = get_sb(sit_blkaddr);
	sit_seg_addr *= blk_size;

	DBG(1, "\tFilling sit area at offset 0x%08"PRIx64"\n", sit_seg_addr);
	for (index = 0; index < (get_sb(segment_count_sit) / 2); index++) {
		if (dev_fill(zero_buf, sit_seg_addr, seg_size)) {
			MSG(1, "\tError: While zeroing out the sit area "
					"on disk!!!\n");
			free(zero_buf);
			return -1;
		}
		sit_seg_addr += seg_size;
	}

	free(zero_buf);
	return 0 ;
}

static int f2fs_init_nat_area(void)
{
	u_int32_t blk_size, seg_size;
	u_int32_t index = 0;
	u_int64_t nat_seg_addr = 0;
	u_int8_t *nat_buf = NULL;

	blk_size = 1 << get_sb(log_blocksize);
	seg_size = (1 << get_sb(log_blocks_per_seg)) * blk_size;

	nat_buf = calloc(sizeof(u_int8_t), seg_size);
	if (nat_buf == NULL) {
		MSG(1, "\tError: Calloc Failed for nat_zero_blk!!!\n");
		return -1;
	}

	nat_seg_addr = get_sb(nat_blkaddr);
	nat_seg_addr *= blk_size;

	DBG(1, "\tFilling nat area at offset 0x%08"PRIx64"\n", nat_seg_addr);
	for (index = 0; index < get_sb(segment_count_nat) / 2; index++) {
		if (dev_fill(nat_buf, nat_seg_addr, seg_size)) {
			MSG(1, "\tError: While zeroing out the nat area "
					"on disk!!!\n");
			free(nat_buf);
			return -1;
		}
		nat_seg_addr = nat_seg_addr + (2 * seg_size);
	}

	free(nat_buf);
	return 0 ;
}

static int f2fs_write_check_point_pack(void)
{
	struct f2fs_summary_block *sum = NULL;
	struct f2fs_journal *journal;
	u_int32_t blk_size_bytes;
	u_int32_t nat_bits_bytes, nat_bits_blocks;
	unsigned char *nat_bits = NULL, *empty_nat_bits;
	u_int64_t cp_seg_blk = 0;
	u_int32_t crc = 0, flags;
	unsigned int i;
	char *cp_payload = NULL;
	char *sum_compact, *sum_compact_p;
	struct f2fs_summary *sum_entry;
	enum quota_type qtype;
	u_int32_t quota_inum, quota_dnum;
	int off;
	int ret = -1;

	cp = calloc(F2FS_BLKSIZE, 1);
	if (cp == NULL) {
		MSG(1, "\tError: Calloc Failed for f2fs_checkpoint!!!\n");
		return ret;
	}

	sum = calloc(F2FS_BLKSIZE, 1);
	if (sum == NULL) {
		MSG(1, "\tError: Calloc Failed for summay_node!!!\n");
		goto free_cp;
	}

	sum_compact = calloc(F2FS_BLKSIZE, 1);
	if (sum_compact == NULL) {
		MSG(1, "\tError: Calloc Failed for summay buffer!!!\n");
		goto free_sum;
	}
	sum_compact_p = sum_compact;

	nat_bits_bytes = get_sb(segment_count_nat) << 5;
	nat_bits_blocks = F2FS_BYTES_TO_BLK((nat_bits_bytes << 1) + 8 +
						F2FS_BLKSIZE - 1);
	nat_bits = calloc(F2FS_BLKSIZE, nat_bits_blocks);
	if (nat_bits == NULL) {
		MSG(1, "\tError: Calloc Failed for nat bits buffer!!!\n");
		goto free_sum_compact;
	}

	cp_payload = calloc(F2FS_BLKSIZE, 1);
	if (cp_payload == NULL) {
		MSG(1, "\tError: Calloc Failed for cp_payload!!!\n");
		goto free_nat_bits;
	}

	/* 1. cp page 1 of checkpoint pack 1 */
	srand(time(NULL));
	cp->checkpoint_ver = cpu_to_le64(rand() | 0x1);
	set_cp(cur_node_segno[0], c.cur_seg[CURSEG_HOT_NODE]);
	set_cp(cur_node_segno[1], c.cur_seg[CURSEG_WARM_NODE]);
	set_cp(cur_node_segno[2], c.cur_seg[CURSEG_COLD_NODE]);
	set_cp(cur_data_segno[0], c.cur_seg[CURSEG_HOT_DATA]);
	set_cp(cur_data_segno[1], c.cur_seg[CURSEG_WARM_DATA]);
	set_cp(cur_data_segno[2], c.cur_seg[CURSEG_COLD_DATA]);
	for (i = 3; i < MAX_ACTIVE_NODE_LOGS; i++) {
		set_cp(cur_node_segno[i], 0xffffffff);
		set_cp(cur_data_segno[i], 0xffffffff);
	}

	quota_inum = quota_dnum = 0;
	for (qtype = 0; qtype < F2FS_MAX_QUOTAS; qtype++)
		if (sb->qf_ino[qtype]) {
			quota_inum++;
			quota_dnum += QUOTA_DATA(qtype);
		}

	set_cp(cur_node_blkoff[0], 1 + quota_inum);
	set_cp(cur_data_blkoff[0], 1 + quota_dnum);
	set_cp(valid_block_count, 2 + quota_inum + quota_dnum);
	set_cp(rsvd_segment_count, c.reserved_segments);
	set_cp(overprov_segment_count, (get_sb(segment_count_main) -
			get_cp(rsvd_segment_count)) *
			c.overprovision / 100);
	set_cp(overprov_segment_count, get_cp(overprov_segment_count) +
			get_cp(rsvd_segment_count));

	MSG(0, "Info: Overprovision ratio = %.3lf%%\n", c.overprovision);
	MSG(0, "Info: Overprovision segments = %u (GC reserved = %u)\n",
					get_cp(overprov_segment_count),
					c.reserved_segments);

	/* main segments - reserved segments - (node + data segments) */
	set_cp(free_segment_count, get_sb(segment_count_main) - 6);
	set_cp(user_block_count, ((get_cp(free_segment_count) + 6 -
			get_cp(overprov_segment_count)) * c.blks_per_seg));
	/* cp page (2), data summaries (1), node summaries (3) */
	set_cp(cp_pack_total_block_count, 6 + get_sb(cp_payload));
	flags = CP_UMOUNT_FLAG | CP_COMPACT_SUM_FLAG;
	if (get_cp(cp_pack_total_block_count) <=
			(1 << get_sb(log_blocks_per_seg)) - nat_bits_blocks)
		flags |= CP_NAT_BITS_FLAG;

	if (c.trimmed)
		flags |= CP_TRIMMED_FLAG;

	set_cp(ckpt_flags, flags);
	set_cp(cp_pack_start_sum, 1 + get_sb(cp_payload));
	set_cp(valid_node_count, 1 + quota_inum);
	set_cp(valid_inode_count, 1 + quota_inum);
	set_cp(next_free_nid, get_sb(root_ino) + 1 + quota_inum);
	set_cp(sit_ver_bitmap_bytesize, ((get_sb(segment_count_sit) / 2) <<
			get_sb(log_blocks_per_seg)) / 8);

	set_cp(nat_ver_bitmap_bytesize, ((get_sb(segment_count_nat) / 2) <<
			 get_sb(log_blocks_per_seg)) / 8);

	set_cp(checksum_offset, CHECKSUM_OFFSET);

	crc = f2fs_cal_crc32(F2FS_SUPER_MAGIC, cp, CHECKSUM_OFFSET);
	*((__le32 *)((unsigned char *)cp + CHECKSUM_OFFSET)) =
							cpu_to_le32(crc);

	blk_size_bytes = 1 << get_sb(log_blocksize);

	if (blk_size_bytes != F2FS_BLKSIZE) {
		MSG(1, "\tError: Wrong block size %d / %d!!!\n",
					blk_size_bytes, F2FS_BLKSIZE);
		goto free_cp_payload;
	}

	cp_seg_blk = get_sb(segment0_blkaddr);

	DBG(1, "\tWriting main segments, cp at offset 0x%08"PRIx64"\n",
						cp_seg_blk);
	if (dev_write_block(cp, cp_seg_blk)) {
		MSG(1, "\tError: While writing the cp to disk!!!\n");
		goto free_cp_payload;
	}

	for (i = 0; i < get_sb(cp_payload); i++) {
		cp_seg_blk++;
		if (dev_fill_block(cp_payload, cp_seg_blk)) {
			MSG(1, "\tError: While zeroing out the sit bitmap area "
					"on disk!!!\n");
			goto free_cp_payload;
		}
	}

	/* Prepare and write Segment summary for HOT/WARM/COLD DATA
	 *
	 * The structure of compact summary
	 * +-------------------+
	 * | nat_journal       |
	 * +-------------------+
	 * | sit_journal       |
	 * +-------------------+
	 * | hot data summary  |
	 * +-------------------+
	 * | warm data summary |
	 * +-------------------+
	 * | cold data summary |
	 * +-------------------+
	*/
	memset(sum, 0, sizeof(struct f2fs_summary_block));
	SET_SUM_TYPE((&sum->footer), SUM_TYPE_DATA);

	journal = &sum->journal;
	journal->n_nats = cpu_to_le16(1 + quota_inum);
	journal->nat_j.entries[0].nid = sb->root_ino;
	journal->nat_j.entries[0].ne.version = 0;
	journal->nat_j.entries[0].ne.ino = sb->root_ino;
	journal->nat_j.entries[0].ne.block_addr = cpu_to_le32(
			get_sb(main_blkaddr) +
			get_cp(cur_node_segno[0]) * c.blks_per_seg);

	for (qtype = 0, i = 1; qtype < F2FS_MAX_QUOTAS; qtype++) {
		if (sb->qf_ino[qtype] == 0)
			continue;
		journal->nat_j.entries[i].nid = sb->qf_ino[qtype];
		journal->nat_j.entries[i].ne.version = 0;
		journal->nat_j.entries[i].ne.ino = sb->qf_ino[qtype];
		journal->nat_j.entries[i].ne.block_addr = cpu_to_le32(
				get_sb(main_blkaddr) +
				get_cp(cur_node_segno[0]) *
				c.blks_per_seg + i);
		i++;
	}

	memcpy(sum_compact_p, &journal->n_nats, SUM_JOURNAL_SIZE);
	sum_compact_p += SUM_JOURNAL_SIZE;

	memset(sum, 0, sizeof(struct f2fs_summary_block));
	/* inode sit for root */
	journal->n_sits = cpu_to_le16(6);
	journal->sit_j.entries[0].segno = cp->cur_node_segno[0];
	journal->sit_j.entries[0].se.vblocks =
				cpu_to_le16((CURSEG_HOT_NODE << 10) |
						(1 + quota_inum));
	f2fs_set_bit(0, (char *)journal->sit_j.entries[0].se.valid_map);
	for (i = 1; i <= quota_inum; i++)
		f2fs_set_bit(i, (char *)journal->sit_j.entries[0].se.valid_map);
	journal->sit_j.entries[1].segno = cp->cur_node_segno[1];
	journal->sit_j.entries[1].se.vblocks =
				cpu_to_le16((CURSEG_WARM_NODE << 10));
	journal->sit_j.entries[2].segno = cp->cur_node_segno[2];
	journal->sit_j.entries[2].se.vblocks =
				cpu_to_le16((CURSEG_COLD_NODE << 10));

	/* data sit for root */
	journal->sit_j.entries[3].segno = cp->cur_data_segno[0];
	journal->sit_j.entries[3].se.vblocks =
				cpu_to_le16((CURSEG_HOT_DATA << 10) |
						(1 + quota_dnum));
	f2fs_set_bit(0, (char *)journal->sit_j.entries[3].se.valid_map);
	for (i = 1; i <= quota_dnum; i++)
		f2fs_set_bit(i, (char *)journal->sit_j.entries[3].se.valid_map);

	journal->sit_j.entries[4].segno = cp->cur_data_segno[1];
	journal->sit_j.entries[4].se.vblocks =
				cpu_to_le16((CURSEG_WARM_DATA << 10));
	journal->sit_j.entries[5].segno = cp->cur_data_segno[2];
	journal->sit_j.entries[5].se.vblocks =
				cpu_to_le16((CURSEG_COLD_DATA << 10));

	memcpy(sum_compact_p, &journal->n_sits, SUM_JOURNAL_SIZE);
	sum_compact_p += SUM_JOURNAL_SIZE;

	/* hot data summary */
	sum_entry = (struct f2fs_summary *)sum_compact_p;
	sum_entry->nid = sb->root_ino;
	sum_entry->ofs_in_node = 0;

	off = 1;
	for (qtype = 0; qtype < F2FS_MAX_QUOTAS; qtype++) {
		if (sb->qf_ino[qtype] == 0)
			continue;
		int j;

		for (j = 0; j < QUOTA_DATA(qtype); j++) {
			(sum_entry + off + j)->nid = sb->qf_ino[qtype];
			(sum_entry + off + j)->ofs_in_node = j;
		}
		off += QUOTA_DATA(qtype);
	}

	/* warm data summary, nothing to do */
	/* cold data summary, nothing to do */

	cp_seg_blk++;
	DBG(1, "\tWriting Segment summary for HOT/WARM/COLD_DATA, at offset 0x%08"PRIx64"\n",
			cp_seg_blk);
	if (dev_write_block(sum_compact, cp_seg_blk)) {
		MSG(1, "\tError: While writing the sum_blk to disk!!!\n");
		goto free_cp_payload;
	}

	/* Prepare and write Segment summary for HOT_NODE */
	memset(sum, 0, sizeof(struct f2fs_summary_block));
	SET_SUM_TYPE((&sum->footer), SUM_TYPE_NODE);

	sum->entries[0].nid = sb->root_ino;
	sum->entries[0].ofs_in_node = 0;
	for (qtype = i = 0; qtype < F2FS_MAX_QUOTAS; qtype++) {
		if (sb->qf_ino[qtype] == 0)
			continue;
		sum->entries[1 + i].nid = sb->qf_ino[qtype];
		sum->entries[1 + i].ofs_in_node = 0;
		i++;
	}

	cp_seg_blk++;
	DBG(1, "\tWriting Segment summary for HOT_NODE, at offset 0x%08"PRIx64"\n",
			cp_seg_blk);
	if (dev_write_block(sum, cp_seg_blk)) {
		MSG(1, "\tError: While writing the sum_blk to disk!!!\n");
		goto free_cp_payload;
	}

	/* Fill segment summary for WARM_NODE to zero. */
	memset(sum, 0, sizeof(struct f2fs_summary_block));
	SET_SUM_TYPE((&sum->footer), SUM_TYPE_NODE);

	cp_seg_blk++;
	DBG(1, "\tWriting Segment summary for WARM_NODE, at offset 0x%08"PRIx64"\n",
			cp_seg_blk);
	if (dev_write_block(sum, cp_seg_blk)) {
		MSG(1, "\tError: While writing the sum_blk to disk!!!\n");
		goto free_cp_payload;
	}

	/* Fill segment summary for COLD_NODE to zero. */
	memset(sum, 0, sizeof(struct f2fs_summary_block));
	SET_SUM_TYPE((&sum->footer), SUM_TYPE_NODE);
	cp_seg_blk++;
	DBG(1, "\tWriting Segment summary for COLD_NODE, at offset 0x%08"PRIx64"\n",
			cp_seg_blk);
	if (dev_write_block(sum, cp_seg_blk)) {
		MSG(1, "\tError: While writing the sum_blk to disk!!!\n");
		goto free_cp_payload;
	}

	/* cp page2 */
	cp_seg_blk++;
	DBG(1, "\tWriting cp page2, at offset 0x%08"PRIx64"\n", cp_seg_blk);
	if (dev_write_block(cp, cp_seg_blk)) {
		MSG(1, "\tError: While writing the cp to disk!!!\n");
		goto free_cp_payload;
	}

	/* write NAT bits, if possible */
	if (flags & CP_NAT_BITS_FLAG) {
		uint32_t i;

		*(__le64 *)nat_bits = get_cp_crc(cp);
		empty_nat_bits = nat_bits + 8 + nat_bits_bytes;
		memset(empty_nat_bits, 0xff, nat_bits_bytes);
		test_and_clear_bit_le(0, empty_nat_bits);

		/* write the last blocks in cp pack */
		cp_seg_blk = get_sb(segment0_blkaddr) + (1 <<
				get_sb(log_blocks_per_seg)) - nat_bits_blocks;

		DBG(1, "\tWriting NAT bits pages, at offset 0x%08"PRIx64"\n",
					cp_seg_blk);

		for (i = 0; i < nat_bits_blocks; i++) {
			if (dev_write_block(nat_bits + i *
						F2FS_BLKSIZE, cp_seg_blk + i)) {
				MSG(1, "\tError: write NAT bits to disk!!!\n");
				goto free_cp_payload;
			}
		}
	}

	/* cp page 1 of check point pack 2
	 * Initiatialize other checkpoint pack with version zero
	 */
	cp->checkpoint_ver = 0;

	crc = f2fs_cal_crc32(F2FS_SUPER_MAGIC, cp, CHECKSUM_OFFSET);
	*((__le32 *)((unsigned char *)cp + CHECKSUM_OFFSET)) =
							cpu_to_le32(crc);
	cp_seg_blk = get_sb(segment0_blkaddr) + c.blks_per_seg;
	DBG(1, "\tWriting cp page 1 of checkpoint pack 2, at offset 0x%08"PRIx64"\n",
				cp_seg_blk);
	if (dev_write_block(cp, cp_seg_blk)) {
		MSG(1, "\tError: While writing the cp to disk!!!\n");
		goto free_cp_payload;
	}

	for (i = 0; i < get_sb(cp_payload); i++) {
		cp_seg_blk++;
		if (dev_fill_block(cp_payload, cp_seg_blk)) {
			MSG(1, "\tError: While zeroing out the sit bitmap area "
					"on disk!!!\n");
			goto free_cp_payload;
		}
	}

	/* cp page 2 of check point pack 2 */
	cp_seg_blk += (le32_to_cpu(cp->cp_pack_total_block_count) -
					get_sb(cp_payload) - 1);
	DBG(1, "\tWriting cp page 2 of checkpoint pack 2, at offset 0x%08"PRIx64"\n",
				cp_seg_blk);
	if (dev_write_block(cp, cp_seg_blk)) {
		MSG(1, "\tError: While writing the cp to disk!!!\n");
		goto free_cp_payload;
	}

	ret = 0;

free_cp_payload:
	free(cp_payload);
free_nat_bits:
	free(nat_bits);
free_sum_compact:
	free(sum_compact);
free_sum:
	free(sum);
free_cp:
	free(cp);
	return ret;
}

static int f2fs_write_super_block(void)
{
	int index;
	u_int8_t *zero_buff;

	zero_buff = calloc(F2FS_BLKSIZE, 1);

	memcpy(zero_buff + F2FS_SUPER_OFFSET, sb, sizeof(*sb));
	DBG(1, "\tWriting super block, at offset 0x%08x\n", 0);
	for (index = 0; index < 2; index++) {
		if (dev_write_block(zero_buff, index)) {
			MSG(1, "\tError: While while writing supe_blk "
					"on disk!!! index : %d\n", index);
			free(zero_buff);
			return -1;
		}
	}

	free(zero_buff);
	return 0;
}

#ifndef WITH_ANDROID
static int discard_obsolete_dnode(struct f2fs_node *raw_node, u_int64_t offset)
{
	u_int64_t next_blkaddr = 0;
	u64 end_blkaddr = (get_sb(segment_count_main) <<
			get_sb(log_blocks_per_seg)) + get_sb(main_blkaddr);
	u_int64_t start_inode_pos = get_sb(main_blkaddr);
	u_int64_t last_inode_pos;
	enum quota_type qtype;
	u_int32_t quota_inum = 0;

	for (qtype = 0; qtype < F2FS_MAX_QUOTAS; qtype++)
		if (sb->qf_ino[qtype]) quota_inum++;

	/* only root inode was written before truncating dnodes */
	last_inode_pos = start_inode_pos +
		c.cur_seg[CURSEG_HOT_NODE] * c.blks_per_seg + quota_inum;

	if (c.zoned_mode)
		return 0;
	do {
		if (offset < get_sb(main_blkaddr) || offset >= end_blkaddr)
			break;

		if (dev_read_block(raw_node, offset)) {
			MSG(1, "\tError: While traversing direct node!!!\n");
			return -1;
		}

		next_blkaddr = le32_to_cpu(raw_node->footer.next_blkaddr);
		memset(raw_node, 0, F2FS_BLKSIZE);

		DBG(1, "\tDiscard dnode, at offset 0x%08"PRIx64"\n", offset);
		if (dev_write_block(raw_node, offset)) {
			MSG(1, "\tError: While discarding direct node!!!\n");
			return -1;
		}
		offset = next_blkaddr;
		/* should avoid recursive chain due to stale data */
		if (offset >= start_inode_pos || offset <= last_inode_pos)
			break;
	} while (1);

	return 0;
}
#endif

static int f2fs_write_root_inode(void)
{
	struct f2fs_node *raw_node = NULL;
	u_int64_t blk_size_bytes, data_blk_nor;
	u_int64_t main_area_node_seg_blk_offset = 0;

	raw_node = calloc(F2FS_BLKSIZE, 1);
	if (raw_node == NULL) {
		MSG(1, "\tError: Calloc Failed for raw_node!!!\n");
		return -1;
	}

	raw_node->footer.nid = sb->root_ino;
	raw_node->footer.ino = sb->root_ino;
	raw_node->footer.cp_ver = cpu_to_le64(1);
	raw_node->footer.next_blkaddr = cpu_to_le32(
			get_sb(main_blkaddr) +
			c.cur_seg[CURSEG_HOT_NODE] *
			c.blks_per_seg + 1);

	raw_node->i.i_mode = cpu_to_le16(0x41ed);
	raw_node->i.i_links = cpu_to_le32(2);
	raw_node->i.i_uid = cpu_to_le32(getuid());
	raw_node->i.i_gid = cpu_to_le32(getgid());

	blk_size_bytes = 1 << get_sb(log_blocksize);
	raw_node->i.i_size = cpu_to_le64(1 * blk_size_bytes); /* dentry */
	raw_node->i.i_blocks = cpu_to_le64(2);

	raw_node->i.i_atime = cpu_to_le32(time(NULL));
	raw_node->i.i_atime_nsec = 0;
	raw_node->i.i_ctime = cpu_to_le32(time(NULL));
	raw_node->i.i_ctime_nsec = 0;
	raw_node->i.i_mtime = cpu_to_le32(time(NULL));
	raw_node->i.i_mtime_nsec = 0;
	raw_node->i.i_generation = 0;
	raw_node->i.i_xattr_nid = 0;
	raw_node->i.i_flags = 0;
	raw_node->i.i_current_depth = cpu_to_le32(1);
	raw_node->i.i_dir_level = DEF_DIR_LEVEL;

	if (c.feature & cpu_to_le32(F2FS_FEATURE_EXTRA_ATTR)) {
		raw_node->i.i_inline = F2FS_EXTRA_ATTR;
		raw_node->i.i_extra_isize =
				cpu_to_le16(F2FS_TOTAL_EXTRA_ATTR_SIZE);
	}

	if (c.feature & cpu_to_le32(F2FS_FEATURE_PRJQUOTA))
		raw_node->i.i_projid = cpu_to_le32(F2FS_DEF_PROJID);

	if (c.feature & cpu_to_le32(F2FS_FEATURE_INODE_CRTIME)) {
		raw_node->i.i_crtime = cpu_to_le32(time(NULL));
		raw_node->i.i_crtime_nsec = 0;
	}

	data_blk_nor = get_sb(main_blkaddr) +
		c.cur_seg[CURSEG_HOT_DATA] * c.blks_per_seg;
	raw_node->i.i_addr[get_extra_isize(raw_node)] = cpu_to_le32(data_blk_nor);

	raw_node->i.i_ext.fofs = 0;
	raw_node->i.i_ext.blk_addr = 0;
	raw_node->i.i_ext.len = 0;

	if (c.feature & cpu_to_le32(F2FS_FEATURE_INODE_CHKSUM))
		raw_node->i.i_inode_checksum =
			cpu_to_le32(f2fs_inode_chksum(raw_node));

	main_area_node_seg_blk_offset = get_sb(main_blkaddr);
	main_area_node_seg_blk_offset += c.cur_seg[CURSEG_HOT_NODE] *
					c.blks_per_seg;

	DBG(1, "\tWriting root inode (hot node), %x %x %x at offset 0x%08"PRIu64"\n",
			get_sb(main_blkaddr),
			c.cur_seg[CURSEG_HOT_NODE],
			c.blks_per_seg, main_area_node_seg_blk_offset);
	if (dev_write_block(raw_node, main_area_node_seg_blk_offset)) {
		MSG(1, "\tError: While writing the raw_node to disk!!!\n");
		free(raw_node);
		return -1;
	}

	/* avoid power-off-recovery based on roll-forward policy */
	main_area_node_seg_blk_offset = get_sb(main_blkaddr);
	main_area_node_seg_blk_offset += c.cur_seg[CURSEG_WARM_NODE] *
					c.blks_per_seg;

#ifndef WITH_ANDROID
	if (discard_obsolete_dnode(raw_node, main_area_node_seg_blk_offset)) {
		free(raw_node);
		return -1;
	}
#endif
	free(raw_node);
	return 0;
}

static int f2fs_write_default_quota(int qtype, unsigned int blkaddr,
						__le32 raw_id)
{
	char *filebuf = calloc(F2FS_BLKSIZE, 2);
	int file_magics[] = INITQMAGICS;
	struct v2_disk_dqheader ddqheader;
	struct v2_disk_dqinfo ddqinfo;
	struct v2r1_disk_dqblk dqblk;

	if (filebuf == NULL) {
		MSG(1, "\tError: Calloc Failed for filebuf!!!\n");
		return -1;
	}

	/* Write basic quota header */
	ddqheader.dqh_magic = cpu_to_le32(file_magics[qtype]);
	/* only support QF_VFSV1 */
	ddqheader.dqh_version = cpu_to_le32(1);

	memcpy(filebuf, &ddqheader, sizeof(ddqheader));

	/* Fill Initial quota file content */
	ddqinfo.dqi_bgrace = cpu_to_le32(MAX_DQ_TIME);
	ddqinfo.dqi_igrace = cpu_to_le32(MAX_IQ_TIME);
	ddqinfo.dqi_flags = cpu_to_le32(0);
	ddqinfo.dqi_blocks = cpu_to_le32(QT_TREEOFF + 5);
	ddqinfo.dqi_free_blk = cpu_to_le32(0);
	ddqinfo.dqi_free_entry = cpu_to_le32(5);

	memcpy(filebuf + V2_DQINFOOFF, &ddqinfo, sizeof(ddqinfo));

	filebuf[1024] = 2;
	filebuf[2048] = 3;
	filebuf[3072] = 4;
	filebuf[4096] = 5;

	filebuf[5120 + 8] = 1;

	dqblk.dqb_id = raw_id;
	dqblk.dqb_pad = cpu_to_le32(0);
	dqblk.dqb_ihardlimit = cpu_to_le64(0);
	dqblk.dqb_isoftlimit = cpu_to_le64(0);
	dqblk.dqb_curinodes = cpu_to_le64(1);
	dqblk.dqb_bhardlimit = cpu_to_le64(0);
	dqblk.dqb_bsoftlimit = cpu_to_le64(0);
	dqblk.dqb_curspace = cpu_to_le64(4096);
	dqblk.dqb_btime = cpu_to_le64(0);
	dqblk.dqb_itime = cpu_to_le64(0);

	memcpy(filebuf + 5136, &dqblk, sizeof(struct v2r1_disk_dqblk));

	/* Write two blocks */
	if (dev_write_block(filebuf, blkaddr) ||
	    dev_write_block(filebuf + F2FS_BLKSIZE, blkaddr + 1)) {
		MSG(1, "\tError: While writing the quota_blk to disk!!!\n");
		free(filebuf);
		return -1;
	}
	DBG(1, "\tWriting quota data, at offset %08x, %08x\n",
					blkaddr, blkaddr + 1);
	free(filebuf);
	return 0;
}

static int f2fs_write_qf_inode(int qtype)
{
	struct f2fs_node *raw_node = NULL;
	u_int64_t data_blk_nor;
	u_int64_t main_area_node_seg_blk_offset = 0;
	__le32 raw_id;
	int i;

	raw_node = calloc(F2FS_BLKSIZE, 1);
	if (raw_node == NULL) {
		MSG(1, "\tError: Calloc Failed for raw_node!!!\n");
		return -1;
	}

	raw_node->footer.nid = sb->qf_ino[qtype];
	raw_node->footer.ino = sb->qf_ino[qtype];
	raw_node->footer.cp_ver = cpu_to_le64(1);
	raw_node->footer.next_blkaddr = cpu_to_le32(
			get_sb(main_blkaddr) +
			c.cur_seg[CURSEG_HOT_NODE] *
			c.blks_per_seg + 1 + qtype + 1);

	raw_node->i.i_mode = cpu_to_le16(0x8180);
	raw_node->i.i_links = cpu_to_le32(1);
	raw_node->i.i_uid = cpu_to_le32(getuid());
	raw_node->i.i_gid = cpu_to_le32(getgid());

	raw_node->i.i_size = cpu_to_le64(1024 * 6); /* Hard coded */
	raw_node->i.i_blocks = cpu_to_le64(1 + QUOTA_DATA(qtype));

	raw_node->i.i_atime = cpu_to_le32(time(NULL));
	raw_node->i.i_atime_nsec = 0;
	raw_node->i.i_ctime = cpu_to_le32(time(NULL));
	raw_node->i.i_ctime_nsec = 0;
	raw_node->i.i_mtime = cpu_to_le32(time(NULL));
	raw_node->i.i_mtime_nsec = 0;
	raw_node->i.i_generation = 0;
	raw_node->i.i_xattr_nid = 0;
	raw_node->i.i_flags = FS_IMMUTABLE_FL;
	raw_node->i.i_current_depth = cpu_to_le32(1);
	raw_node->i.i_dir_level = DEF_DIR_LEVEL;

	if (c.feature & cpu_to_le32(F2FS_FEATURE_EXTRA_ATTR)) {
		raw_node->i.i_inline = F2FS_EXTRA_ATTR;
		raw_node->i.i_extra_isize =
				cpu_to_le16(F2FS_TOTAL_EXTRA_ATTR_SIZE);
	}

	if (c.feature & cpu_to_le32(F2FS_FEATURE_PRJQUOTA))
		raw_node->i.i_projid = cpu_to_le32(F2FS_DEF_PROJID);

	data_blk_nor = get_sb(main_blkaddr) +
		c.cur_seg[CURSEG_HOT_DATA] * c.blks_per_seg + 1;

	for (i = 0; i < qtype; i++)
		if (sb->qf_ino[i])
			data_blk_nor += QUOTA_DATA(i);
	if (qtype == 0)
		raw_id = raw_node->i.i_uid;
	else if (qtype == 1)
		raw_id = raw_node->i.i_gid;
	else if (qtype == 2)
		raw_id = raw_node->i.i_projid;
	else
		ASSERT(0);

	/* write two blocks */
	if (f2fs_write_default_quota(qtype, data_blk_nor, raw_id)) {
		free(raw_node);
		return -1;
	}

	for (i = 0; i < QUOTA_DATA(qtype); i++)
		raw_node->i.i_addr[get_extra_isize(raw_node) + i] =
					cpu_to_le32(data_blk_nor + i);
	raw_node->i.i_ext.fofs = 0;
	raw_node->i.i_ext.blk_addr = 0;
	raw_node->i.i_ext.len = 0;

	if (c.feature & cpu_to_le32(F2FS_FEATURE_INODE_CHKSUM))
		raw_node->i.i_inode_checksum =
			cpu_to_le32(f2fs_inode_chksum(raw_node));

	main_area_node_seg_blk_offset = get_sb(main_blkaddr);
	main_area_node_seg_blk_offset += c.cur_seg[CURSEG_HOT_NODE] *
					c.blks_per_seg + qtype + 1;

	DBG(1, "\tWriting quota inode (hot node), %x %x %x at offset 0x%08"PRIu64"\n",
			get_sb(main_blkaddr),
			c.cur_seg[CURSEG_HOT_NODE],
			c.blks_per_seg, main_area_node_seg_blk_offset);
	if (dev_write_block(raw_node, main_area_node_seg_blk_offset)) {
		MSG(1, "\tError: While writing the raw_node to disk!!!\n");
		free(raw_node);
		return -1;
	}

	free(raw_node);
	return 0;
}

static int f2fs_update_nat_root(void)
{
	struct f2fs_nat_block *nat_blk = NULL;
	u_int64_t nat_seg_blk_offset = 0;
	enum quota_type qtype;
	int i;

	nat_blk = calloc(F2FS_BLKSIZE, 1);
	if(nat_blk == NULL) {
		MSG(1, "\tError: Calloc Failed for nat_blk!!!\n");
		return -1;
	}

	/* update quota */
	for (qtype = i = 0; qtype < F2FS_MAX_QUOTAS; qtype++) {
		if (sb->qf_ino[qtype] == 0)
			continue;
		nat_blk->entries[sb->qf_ino[qtype]].block_addr =
				cpu_to_le32(get_sb(main_blkaddr) +
				c.cur_seg[CURSEG_HOT_NODE] *
				c.blks_per_seg + i + 1);
		nat_blk->entries[sb->qf_ino[qtype]].ino = sb->qf_ino[qtype];
		i++;
	}

	/* update root */
	nat_blk->entries[get_sb(root_ino)].block_addr = cpu_to_le32(
		get_sb(main_blkaddr) +
		c.cur_seg[CURSEG_HOT_NODE] * c.blks_per_seg);
	nat_blk->entries[get_sb(root_ino)].ino = sb->root_ino;

	/* update node nat */
	nat_blk->entries[get_sb(node_ino)].block_addr = cpu_to_le32(1);
	nat_blk->entries[get_sb(node_ino)].ino = sb->node_ino;

	/* update meta nat */
	nat_blk->entries[get_sb(meta_ino)].block_addr = cpu_to_le32(1);
	nat_blk->entries[get_sb(meta_ino)].ino = sb->meta_ino;

	nat_seg_blk_offset = get_sb(nat_blkaddr);

	DBG(1, "\tWriting nat root, at offset 0x%08"PRIx64"\n",
					nat_seg_blk_offset);
	if (dev_write_block(nat_blk, nat_seg_blk_offset)) {
		MSG(1, "\tError: While writing the nat_blk set0 to disk!\n");
		free(nat_blk);
		return -1;
	}

	free(nat_blk);
	return 0;
}

static int f2fs_add_default_dentry_root(void)
{
	struct f2fs_dentry_block *dent_blk = NULL;
	u_int64_t data_blk_offset = 0;

	dent_blk = calloc(F2FS_BLKSIZE, 1);
	if(dent_blk == NULL) {
		MSG(1, "\tError: Calloc Failed for dent_blk!!!\n");
		return -1;
	}

	dent_blk->dentry[0].hash_code = 0;
	dent_blk->dentry[0].ino = sb->root_ino;
	dent_blk->dentry[0].name_len = cpu_to_le16(1);
	dent_blk->dentry[0].file_type = F2FS_FT_DIR;
	memcpy(dent_blk->filename[0], ".", 1);

	dent_blk->dentry[1].hash_code = 0;
	dent_blk->dentry[1].ino = sb->root_ino;
	dent_blk->dentry[1].name_len = cpu_to_le16(2);
	dent_blk->dentry[1].file_type = F2FS_FT_DIR;
	memcpy(dent_blk->filename[1], "..", 2);

	/* bitmap for . and .. */
	test_and_set_bit_le(0, dent_blk->dentry_bitmap);
	test_and_set_bit_le(1, dent_blk->dentry_bitmap);

	data_blk_offset = get_sb(main_blkaddr);
	data_blk_offset += c.cur_seg[CURSEG_HOT_DATA] *
				c.blks_per_seg;

	DBG(1, "\tWriting default dentry root, at offset 0x%08"PRIx64"\n",
				data_blk_offset);
	if (dev_write_block(dent_blk, data_blk_offset)) {
		MSG(1, "\tError: While writing the dentry_blk to disk!!!\n");
		free(dent_blk);
		return -1;
	}

	free(dent_blk);
	return 0;
}

static int f2fs_create_root_dir(void)
{
	enum quota_type qtype;
	int err = 0;

	err = f2fs_write_root_inode();
	if (err < 0) {
		MSG(1, "\tError: Failed to write root inode!!!\n");
		goto exit;
	}

	for (qtype = 0; qtype < F2FS_MAX_QUOTAS; qtype++)  {
		if (sb->qf_ino[qtype] == 0)
			continue;
		err = f2fs_write_qf_inode(qtype);
		if (err < 0) {
			MSG(1, "\tError: Failed to write quota inode!!!\n");
			goto exit;
		}
	}

	err = f2fs_update_nat_root();
	if (err < 0) {
		MSG(1, "\tError: Failed to update NAT for root!!!\n");
		goto exit;
	}

	err = f2fs_add_default_dentry_root();
	if (err < 0) {
		MSG(1, "\tError: Failed to add default dentries for root!!!\n");
		goto exit;
	}
exit:
	if (err)
		MSG(1, "\tError: Could not create the root directory!!!\n");

	return err;
}

int f2fs_format_device(void)
{
	int err = 0;

	err= f2fs_prepare_super_block();
	if (err < 0) {
		MSG(0, "\tError: Failed to prepare a super block!!!\n");
		goto exit;
	}

	if (c.trim) {
		err = f2fs_trim_devices();
		if (err < 0) {
			MSG(0, "\tError: Failed to trim whole device!!!\n");
			goto exit;
		}
	}

	err = f2fs_init_sit_area();
	if (err < 0) {
		MSG(0, "\tError: Failed to Initialise the SIT AREA!!!\n");
		goto exit;
	}

	err = f2fs_init_nat_area();
	if (err < 0) {
		MSG(0, "\tError: Failed to Initialise the NAT AREA!!!\n");
		goto exit;
	}

	err = f2fs_create_root_dir();
	if (err < 0) {
		MSG(0, "\tError: Failed to create the root directory!!!\n");
		goto exit;
	}

	err = f2fs_write_check_point_pack();
	if (err < 0) {
		MSG(0, "\tError: Failed to write the check point pack!!!\n");
		goto exit;
	}

	err = f2fs_write_super_block();
	if (err < 0) {
		MSG(0, "\tError: Failed to write the Super Block!!!\n");
		goto exit;
	}
exit:
	if (err)
		MSG(0, "\tError: Could not format the device!!!\n");

	return err;
}
