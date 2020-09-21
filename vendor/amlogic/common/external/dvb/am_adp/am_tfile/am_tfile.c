
#ifdef _FORTIFY_SOURCE
#undef _FORTIFY_SOURCE
#endif
/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file
 * \brief file functions
 *
 ***************************************************************************/

#ifndef ANDROID
#define _LARGEFILE64_SOURCE
#endif
#define AM_DEBUG_LEVEL 5

#include <unistd.h>
//#include <sys/types.h>
#ifdef SUPPORT_CAS
#include <limits.h>
#include <byteswap.h>
#endif
#include <sys/stat.h>
#include <fcntl.h>
#include <am_types.h>
#include <am_debug.h>
#include <am_mem.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
//#include <dlfcn.h>
#include "am_misc.h"
#include "am_evt.h"
#include "am_time.h"
#include "am_tfile.h"
#ifdef SUPPORT_CAS
#include "am_av.h"
#endif

#include "list.h"

/****************************************************************************
 * Macro definitions
 ***************************************************************************/
typedef struct {
	struct list_head head;
	int time;
	loff_t start;
	loff_t end;
} tfragment_t;

typedef struct {
	struct list_head list;
	int wrap;
	int now;
	int wstart;
	int last;
	AM_TFile_t file;
} tfile_timer_t;

#ifdef SUPPORT_CAS
typedef struct
{
	struct list_head list;
	uint64_t start;
	uint64_t end;
	uint16_t info_len;
	uint8_t *info_data;
} tfile_cas_t;

static tfile_cas_t cas_ctx = {0};
static cas_info_data[16] = {0};
static CAS_RecInfo_t cas_rec_info = {
.blk_size = 0,
.len = 0,
.data = cas_info_data
};
#define CAS_MAGIC_NUM 0xBEEFBEEF
#define NODE_IS_REWIND(p) \
	(p->end <= p->start ? 1 : 0)
#endif
/****************************************************************************
 * Static functions
 ***************************************************************************/

static int timer_get_total(void *timer)
{
	if (!timer)
		return 0;
	tfile_timer_t *ptimer = (tfile_timer_t*)timer;
	tfragment_t *pfirst = list_first_entry(&ptimer->list, tfragment_t, head);
	tfragment_t *plast = list_entry(ptimer->list.prev, tfragment_t, head);

	AM_DEBUG(3, "[tfile] total [%d, %d - %d]ms [%lld - %lld]B",
		plast->time - pfirst->time, pfirst->time, plast->time,
		pfirst->start, plast->start);

	return plast->time - pfirst->time;
}

static int timer_try_add_fragment(void *timer, loff_t pre_end, loff_t offset, int force)
{
	tfile_timer_t *ptimer = (tfile_timer_t*)timer;
	tfragment_t *plast = NULL;
	tfragment_t *pnow;
	int now;

	AM_TIME_GetClock(&now);
	ptimer->now = now - ptimer->wstart;

	if (!force && (ptimer->now - ptimer->last  < 1000))//fragment size: 1s
		return 0;

	ptimer->last = ptimer->now;

	AM_EVT_Signal((long )ptimer->file, AM_TFILE_EVT_END_TIME_CHANGED, (void*)(long)ptimer->now);

	if (!list_empty(&ptimer->list)) {
		plast = list_entry(ptimer->list.prev, tfragment_t, head);
	}

	if (plast && (plast->start == offset)) {
		/*write fail? fragment too large? should not get here!!*/
		AM_DEBUG(0, "[tfile] FATAL ERROR");
		return 0;
	}

	if (plast)
		plast->end = pre_end;

	pnow = malloc(sizeof(tfragment_t));
	memset(pnow, 0, sizeof(tfragment_t));

	pnow->time = ptimer->now;
	pnow->start = offset;
	pnow->end = 0;

	list_add_tail(&pnow->head, &ptimer->list);

	AM_DEBUG(3, "[tfile] add fragment %p[time:%d start:%lld] force[%d]",
		pnow, pnow->time, pnow->start, force);

	return 0;
}

static int timer_try_add_fragment_continuous(void *timer, loff_t offset, int force)
{
	return timer_try_add_fragment(timer, offset, offset, force);
}

//remove earliest fragment, return removed size and next fragment's start
static loff_t timer_remove_earliest_fragment(void *timer, loff_t *next)
{
	tfile_timer_t *ptimer = (tfile_timer_t*)timer;
	loff_t size = 0;

	if (!list_empty(&ptimer->list))
	{
		tfragment_t *pfirst, *pnext;

		pfirst = list_first_entry(&ptimer->list, tfragment_t, head);
		list_del(&pfirst->head);
		pnext = list_first_entry(&ptimer->list, tfragment_t, head);

		if (next)
			*next = pnext->start;

		size = (pfirst->end >= pfirst->start) ? (pfirst->end - pfirst->start) : (ptimer->file->size - pfirst->start);

		AM_DEBUG(3, "[tfile] del fragment %p[time:%d start:%lld end:%lld size:%lld]",
			pfirst, pfirst->time, pfirst->start, pfirst->end, size);

		free(pfirst);

		ptimer->wrap = 1;
		AM_EVT_Signal((long)ptimer->file, AM_TFILE_EVT_START_TIME_CHANGED, (void *)(long)pnext->time);

	}
	return size;
}

static int aml_tfile_attach_timer(AM_TFile_t tfile)
{
	tfile_timer_t *timer = malloc(sizeof(tfile_timer_t));
	tfragment_t *frag = malloc(sizeof(tfragment_t));
	int err = 0;

	if (!timer || !frag) {
		AM_DEBUG(0, "[tfile] FATAL: no mem for tfile");
		err = -1;
		goto end;
	}

	memset(timer, 0, sizeof(tfile_timer_t));
	INIT_LIST_HEAD(&timer->list);

	memset(frag, 0, sizeof(tfragment_t));

	pthread_mutex_lock(&tfile->lock);
	if (!tfile->timer) {
		AM_TIME_GetClock(&timer->wstart);
		timer->now = timer->last = 0;

		frag->time = 0;
		frag->end = 0;
		frag->start = tfile->write;
		list_add_tail(&frag->head, &timer->list);

		AM_DEBUG(3, "[tfile] add fragment %p[time:%d start:%lld]",
				frag, frag->time, frag->start);

		timer->file = tfile;
		timer->wrap = 0;

		tfile->timer = timer;
	} else {
		err = 1;
	}
	pthread_mutex_unlock(&tfile->lock);

	AM_EVT_Signal((long)tfile, AM_TFILE_EVT_START_TIME_CHANGED, (void *)(long)0);

end:
	if (err) {
		if (timer)
			free(timer);
		if (frag)
			free(frag);
	}
	return (err < 0) ? err : 0;
}

static int aml_tfile_detach_timer(AM_TFile_t tfile)
{
	tfile_timer_t *timer = NULL;
	struct list_head *pos = NULL, *n = NULL;

	if (!tfile->timer)
		return 0;

	pthread_mutex_lock(&tfile->lock);
	timer = tfile->timer;
	tfile->timer = NULL;
	pthread_mutex_unlock(&tfile->lock);

	list_for_each_safe(pos, n, &timer->list)
	{
		list_del(pos);
		free(list_entry(pos, tfragment_t, head));
	}
	free(timer);
	return 0;
}

static int aml_timeshift_data_write(int fd, uint8_t *buf, int size)
{
	int ret;
	int left = size;
	uint8_t *p = buf;

	while (left > 0)
	{
		ret = write(fd, p, left);
		if (ret == -1)
		{
			if (errno != EINTR)
			{
				AM_DEBUG(0, "[tfile] write data failed: %s", strerror(errno));
				break;
			}
			ret = 0;
		}

		left -= ret;
		p += ret;
	}

	return (size - left);
}

static AM_TFile_Sub_t *aml_timeshift_new_subfile(AM_TFile_t tfile)
{
	int flags;
	struct stat st;
	char fname[1024];
	AM_Bool_t is_timeshift = tfile->is_timeshift;
	AM_TFile_Sub_t *sub_file, *ret = NULL;

	if (is_timeshift)
	{
		/* create new timeshifting sub files */
		flags = O_WRONLY | O_CREAT | O_TRUNC;
	}
	else
	{
		/* gathering all pvr playback sub files */
		flags = O_WRONLY;
	}

	sub_file = (AM_TFile_Sub_t *)malloc(sizeof(AM_TFile_Sub_t));
	if (sub_file == NULL)
	{
		AM_DEBUG(0, "[tfile] Cannot write sub file , no memory!\n");
		return ret;
	}

	sub_file->next = NULL;


	if (tfile->last_sub_index == 0 && !is_timeshift)
	{
		/* To be compatible with the old version */
		sub_file->findex = tfile->last_sub_index++;
		snprintf(fname, sizeof(fname), "%s", tfile->name);
	}
	else
	{
		sub_file->findex = tfile->last_sub_index++;
		snprintf(fname, sizeof(fname), "%s.%d", tfile->name, sub_file->findex);
	}

	if (!is_timeshift && stat(fname, &st) < 0)
	{
		AM_DEBUG(2, "[tfile] sub file '%s': %s", fname, strerror(errno));
		sub_file->wfd = -1;
		sub_file->rfd = -1;
	}
	else
	{
		AM_DEBUG(1, "openning %s\n", fname);
		sub_file->wfd = open(fname, flags, 0666);
		sub_file->rfd = open(fname, O_RDONLY, 0666);
	}

	if (sub_file->wfd < 0 || sub_file->rfd < 0)
	{
		AM_DEBUG(0, "Open file failed: %s", strerror(errno));
		if (is_timeshift)
		{
			AM_DEBUG(0, "Cannot open new sub file: %s\n", strerror(errno));
		}
		else
		{
			/* To be compatible with the old version */
			if (sub_file->findex == 0)
			{
				ret = aml_timeshift_new_subfile(tfile);
			}
			else
			{
				AM_DEBUG(2, "[tfile] sub files end at index %d", sub_file->findex-1);
			}
		}

		if (sub_file->wfd >= 0)
			close(sub_file->wfd);
		if (sub_file->rfd >= 0)
			close(sub_file->rfd);

		free(sub_file);
	}
	else
	{
		ret = sub_file;
	}

	return ret;
}

static ssize_t aml_timeshift_subfile_read(AM_TFile_t tfile, uint8_t *buf, size_t size)
{
	ssize_t ret = 0;

	if (tfile->sub_files == NULL)
	{
		AM_DEBUG(0, "[tfile] No sub files, cannot read\n");
		return -1;
	}

	if (tfile->cur_rsub_file == NULL)
	{
		tfile->cur_rsub_file = tfile->sub_files;
		AM_DEBUG(2, "[tfile] Reading from the start, file index %d", tfile->cur_rsub_file->findex);
	}

	ret = read(tfile->cur_rsub_file->rfd, buf, size);
	if (ret == 0)
	{
		/* reach the end, automatically turn to next sub file */
		if (tfile->cur_rsub_file->next != NULL)
		{
			tfile->cur_rsub_file = tfile->cur_rsub_file->next;
			AM_DEBUG(1, "[tfile] Reading from file index %d ...", tfile->cur_rsub_file->findex);
			lseek64(tfile->cur_rsub_file->rfd, 0, SEEK_SET);
			ret = aml_timeshift_subfile_read(tfile, buf, size);
		}
	}

	return ret;
}

static ssize_t aml_timeshift_subfile_write(AM_TFile_t tfile, uint8_t *buf, size_t size)
{
	ssize_t ret = 0;
	loff_t fsize;

	if (tfile->sub_files == NULL)
	{
		AM_DEBUG(0, "[tfile] No sub files, cannot write\n");
		return -1;
	}

	if (tfile->cur_wsub_file == NULL)
	{
		tfile->cur_wsub_file = tfile->sub_files;
		AM_DEBUG(1, "[tfile] Switching to file index %d for writing...\n",
			tfile->cur_wsub_file->findex);
	}

	fsize = lseek64(tfile->cur_wsub_file->wfd, 0, SEEK_CUR);
	if (fsize >= tfile->sub_file_size)
	{
		AM_Bool_t start_new = AM_FALSE;

		if (tfile->cur_wsub_file->next != NULL)
		{
			tfile->cur_wsub_file = tfile->cur_wsub_file->next;
			start_new = AM_TRUE;
		}
		else
		{
			AM_TFile_Sub_t *sub_file = aml_timeshift_new_subfile(tfile);
			if (sub_file != NULL)
			{
				tfile->cur_wsub_file->next = sub_file;
				tfile->cur_wsub_file = sub_file;
				start_new = AM_TRUE;
			}
		}

		if (start_new)
		{
			AM_DEBUG(1, "[tfile] Switching to file index %d for writing...\n",
				tfile->cur_wsub_file->findex);
			lseek64(tfile->cur_wsub_file->wfd, 0, SEEK_SET);
			ret = aml_timeshift_subfile_write(tfile, buf, size);
		}
	}
	else
	{
		ret = aml_timeshift_data_write(tfile->cur_wsub_file->wfd, buf, size);
	}

	return ret;
}

static int aml_timeshift_subfile_close(AM_TFile_t tfile)
{
	AM_TFile_Sub_t *sub_file, *next;
	char fname[1024];

	sub_file = tfile->sub_files;
	while (sub_file != NULL)
	{
		next = sub_file->next;
		if (sub_file->rfd >= 0)
		{
			close(sub_file->rfd);
		}
		if (sub_file->wfd >= 0)
		{
			close(sub_file->wfd);
		}

		if (tfile->delete_on_close)
		{
			if (sub_file->findex == 0 && !tfile->is_timeshift)
				snprintf(fname, sizeof(fname), "%s", tfile->name);
			else
				snprintf(fname, sizeof(fname), "%s.%d", tfile->name, sub_file->findex);
			AM_DEBUG(1, "[tfile] unlinking file: %s", fname);
			unlink(fname);
		}

		free(sub_file);
		sub_file = next;
	}

	return 0;
}

static int aml_timeshift_subfile_open(AM_TFile_t tfile)
{
	int index = 1, flags;
	char fname[1024];
	AM_TFile_Sub_t *sub_file, *prev_sub_file, *exist_sub_file;
	AM_Bool_t is_timeshift = tfile->is_timeshift;
	loff_t left = is_timeshift ? tfile->size : 1/*just keep the while going*/;

	if (is_timeshift)
	{
		tfile->sub_files = aml_timeshift_new_subfile(tfile);
		if (!tfile->sub_files)
			return -1;
	}
	else
	{
		prev_sub_file = NULL;
		do
		{
			sub_file = aml_timeshift_new_subfile(tfile);
			if (sub_file != NULL)
			{
				off64_t size;

				if (prev_sub_file == NULL)
					tfile->sub_files = sub_file;
				else
					prev_sub_file->next = sub_file;
				prev_sub_file = sub_file;

				size = lseek64(sub_file->rfd, 0, SEEK_END);
				tfile->sub_file_size = AM_MAX(tfile->sub_file_size, size);
				tfile->size += size;
				lseek64(sub_file->rfd, 0, SEEK_SET);
			}
		} while (sub_file != NULL);
	}

	return 0;
}

static loff_t aml_timeshift_subfile_seek(AM_TFile_t tfile, loff_t offset, AM_Bool_t read)
{
	int sub_index = offset/tfile->sub_file_size; /* start from 0 */
	loff_t sub_offset = offset%tfile->sub_file_size;
	AM_TFile_Sub_t *sub_file = tfile->sub_files;

	while (sub_file != NULL)
	{
		if (sub_file->findex == sub_index)
			break;
		sub_file = sub_file->next;
	}

	if (sub_file != NULL)
	{
		AM_DEBUG(1, "[tfile] %c Seek to sub file %d at %lld\n", read?'r':'w', sub_index, sub_offset);
		if (read)
		{
			tfile->cur_rsub_file = sub_file;
			lseek64(sub_file->rfd, sub_offset, SEEK_SET);
		}
		else
		{
			tfile->cur_wsub_file = sub_file;
			lseek64(sub_file->wfd, sub_offset, SEEK_SET);
		}
		return offset;
	}

	return (loff_t)-1;
}

//duration & size valid in loop mode only now, cause tfile used by timeshifting only, tfile may be used for all recording case in the future
AM_ErrorCode_t AM_TFile_Open(AM_TFile_t *tfile, const char *file_name, AM_Bool_t loop, int max_duration, loff_t max_size)
{
	AM_ErrorCode_t ret = AM_SUCCESS;
	const loff_t SUB_FILE_SIZE = 1024*1024*1024ll;

	AM_TFileData_t *pfile = malloc(sizeof(AM_TFileData_t));
	memset(pfile, 0, sizeof(AM_TFileData_t));

	pfile->name = strdup(file_name);
	pfile->sub_file_size = SUB_FILE_SIZE;
	pfile->is_timeshift = (strstr(pfile->name, "TimeShifting") != NULL);
	pfile->loop = loop;
	pfile->duration = max_duration;
	if (pfile->is_timeshift)
		pfile->delete_on_close = 1;

	if (pfile->loop)
	{
		pfile->size = (max_size <= 0)? SUB_FILE_SIZE : max_size;

		if (aml_timeshift_subfile_open(pfile) < 0)
			return AM_FAILURE;

		pfile->avail = 0;
		pfile->total = 0;
	}
	else
	{
		if (aml_timeshift_subfile_open(pfile) < 0)
			return AM_FAILURE;

		pfile->avail = pfile->size;
		pfile->total = pfile->size;
		AM_DEBUG(1, "[tfile] total subfiles size %lld", pfile->size);
	}

	pfile->start = 0;
	pfile->read = pfile->write = 0;

	pthread_mutex_init(&pfile->lock, NULL);
	pthread_cond_init(&pfile->cond, NULL);

	pfile->opened = 1;

	*tfile = pfile;
	return AM_SUCCESS;
}

AM_ErrorCode_t AM_TFile_Close(AM_TFile_t tfile)
{
	if (! tfile->opened)
	{
		AM_DEBUG(0, "[tfile] has not opened");
		return AM_FAILURE;
	}

	tfile->opened = 0;

	pthread_mutex_lock(&tfile->lock);
	aml_timeshift_subfile_close(tfile);

	/*remove timeshift file*/
	if (tfile->name != NULL)
	{
		free(tfile->name);
		tfile->name = NULL;
	}
	pthread_mutex_unlock(&tfile->lock);

	aml_tfile_detach_timer(tfile);

	pthread_mutex_destroy(&tfile->lock);

	return AM_SUCCESS;
}

#ifdef SUPPORT_CAS
AM_ErrorCode_t AM_TFile_CasOpen(char *path)
{
	int fd, ret;
	uint32_t blk_size;
	uint16_t info_len;
	uint64_t pos;
	tfile_cas_t *pcas = NULL;

	AM_DEBUG(3, "%s in\n", __func__);
	if (!path) {
		INIT_LIST_HEAD(&cas_ctx.list);
		AM_DEBUG(0, "[tfile] cas dat path null");
		return AM_SUCCESS;
	}
	fd = open(path, O_RDONLY);
	if (fd < 0) {
		AM_DEBUG(0, "[tfile] open cas file failed");
		return AM_FAILURE;
	}

	if ((ret = read(fd, &blk_size, 4)) != 4) {
		AM_DEBUG(0, "[tfile] read cas block size failed");
		goto fail;
	}

	if ((ret = read(fd, &info_len, 2)) != 2) {
		AM_DEBUG(0, "[tfile] read cas header info len failed");
		goto fail;
	}

	cas_rec_info.blk_size = bswap_32(blk_size);
	cas_rec_info.len =  bswap_16(info_len);
	if ((ret = read(fd, cas_rec_info.data, cas_rec_info.len)) != cas_rec_info.len) {
		AM_DEBUG(0, "[tfile] read cas header data failed");
		goto fail;
	}

	INIT_LIST_HEAD(&cas_ctx.list);
	AM_DEBUG(0, "[tfile] cas block size:%d, header info len:%d, seek:%d\n",
			bswap_32(blk_size), bswap_16(info_len), ret);
	do {
		if ((ret = read(fd, &pos, 8)) != 8) {
			AM_DEBUG(0, "[tfile] read cas pos failed, ret:%d. maybe end of file", ret);
			close(fd);
			return AM_SUCCESS;
		}
		AM_DEBUG(0, "[tfile] read cas pos:%lld", bswap_64(pos));
		if ((ret = read(fd, &info_len, 2)) != 2) {
			AM_DEBUG(0, "[tfile] read cas info len failed");
			goto fail;
		}

		info_len = bswap_16(info_len);
		AM_DEBUG(0, "[tfile] read cas store info len:%d", info_len);
		if (pos == ULLONG_MAX) {
			break;
		}
		pcas = malloc(sizeof(tfile_cas_t));
		pcas->info_data = malloc(info_len);
		pcas->info_len = info_len;
		pcas->start = bswap_64(pos);
		if ((ret = read(fd, pcas->info_data, pcas->info_len)) != pcas->info_len) {
			AM_DEBUG(0, "[tfile] read cas info len failed");
			goto fail;
		}
		AM_DEBUG(0, "[tfile] read cas store info");
		list_add_tail(&pcas->list, &cas_ctx.list);
		AM_DEBUG(0, "[tfile] add cas list");
	} while (1);

	close(fd);
	return AM_SUCCESS;

fail:
	if (fd > 0) {
		close(fd);
	}
	return AM_FAILURE;
}
AM_ErrorCode_t AM_TFile_CasClose()
{
	struct list_head *pos, *q;
	tfile_cas_t *pcas;

	AM_DEBUG(3, "%s in\n", __func__);
	list_for_each_safe(pos, q, &cas_ctx.list) {
		pcas = list_entry(pos, tfile_cas_t, list);
		list_del(pos);
		free(pcas->info_data);
		free(pcas);
	}
	return AM_SUCCESS;
}

AM_ErrorCode_t AM_TFile_CasUpdateStoreInfo(uint32_t len, uint64_t fsize)
{
	struct list_head *pos, *q;
	tfile_cas_t *pfirst, *plast;

	plast = list_entry(cas_ctx.list.prev, tfile_cas_t, list);
	pfirst = list_first_entry(&cas_ctx.list, tfile_cas_t, list);
	if (plast->end == CAS_MAGIC_NUM) {
		//the last node end pos no value
		plast->end = (plast->start + len) % fsize;
	} else {
		//the last node end pos already had value
		if (plast == pfirst && NODE_IS_REWIND(plast)) {
			//only 1 node and rewind
			plast->start = (plast->start + len) % fsize;
		}
		plast->end = (plast->end + len) % fsize;
	}

	AM_DEBUG(2, "%s, first:%lld, %lld, last:%lld-%lld, arg len:%d, fsize:%lld\n", __func__,
			pfirst->start, pfirst->end, plast->start, plast->end, len, fsize);
	if (plast == pfirst) {
		AM_DEBUG(1, "%s, only 1 node\n", __func__);
		return 0;
	}

	if((plast->start <= pfirst->start && plast->end > pfirst->start) ||
			(NODE_IS_REWIND(plast) && plast->end > pfirst->start)) {
		AM_DEBUG(0, "plast and pfirst has coincide area, pfirst:%lld, plast:%lld\n",
				pfirst->start, plast->end);
		pfirst->start = plast->end;
		if (pfirst->start == pfirst->end) {
			AM_DEBUG(0, "last node overwrite first node, delete first node\n");
			list_del(&pfirst->list);
		}
	}

	return 0;
}

AM_ErrorCode_t AM_TFile_CasSetStoreInfo(CAS_StoreInfo_t info)
{
	tfile_cas_t *pcas, *plast;

	pcas = malloc(sizeof(tfile_cas_t));
	pcas->info_data = malloc(info.len);
	pcas->info_len = info.len;
	pcas->start = info.pos;
	pcas->end = CAS_MAGIC_NUM;
	memcpy(pcas->info_data, info.data, info.len);
	AM_DEBUG(0, "%s set cas store info, len:%d, pos:%lld", __func__, info.len, info.pos);
	list_add_tail(&pcas->list, &cas_ctx.list);

	return AM_SUCCESS;
}

AM_ErrorCode_t AM_TFile_CasGetStoreInfo(uint64_t stream_pos, CAS_StoreInfo_t *info)
{
	struct list_head *pos, *q;
	tfile_cas_t *pcas, *pstart;

	list_for_each_safe(pos, q, &cas_ctx.list) {
		AM_DEBUG(0, "Get store info loop");
		pcas = list_entry(pos, tfile_cas_t, list);
		AM_DEBUG(0, "Get store info %lld->%lld", pcas->start, pcas->end);
		if (NODE_IS_REWIND(pcas)) {
			AM_DEBUG(2, "%s, >> %d, pos:%lld, start-end:%lld-%lld\n",
					__func__, __LINE__, stream_pos, pcas->start, pcas->end);
			if (stream_pos > pcas->start || stream_pos <= pcas->end) {
				//seems like can NOT change info->pos value, otherwise vmx report DVR channel already in used 
				info->pos = 0;//pcas->start;
				info->len = pcas->info_len;
				memcpy(info->data, pcas->info_data, pcas->info_len);
				AM_DEBUG(1, "%s found rewind store info, pos:%lld-%lld, stream_pos:%lld, info_len:%d\n", __func__,
						pcas->start, pcas->end, stream_pos, pcas->info_len);
				return 0;
			}
		} else {
			AM_DEBUG(2, "%s, >> %d, pos:%lld, start-end:%lld-%lld\n",
					__func__, __LINE__, stream_pos, pcas->start, pcas->end);
			if (stream_pos > pcas->start && stream_pos <= pcas->end) {
				//seems like can NOT change info->pos value, otherwise vmx report DVR channel already in used 
				info->pos = 0;//pcas->start;
				info->len = pcas->info_len;
				memcpy(info->data, pcas->info_data, pcas->info_len);
				AM_DEBUG(1, "%s found store info, pos:%lld-%lld, stream_pos:%lld\n", __func__,
						pcas->start, pcas->end, stream_pos);
				return 0;
			}
		}
	}

	AM_DEBUG(0, "Get store info failed");
	return AM_FAILURE;
}

AM_ErrorCode_t AM_TFile_CasGetRecInfo(CAS_RecInfo_t *info)
{
	if (info) {
		info->blk_size = cas_rec_info.blk_size;
		info->len = cas_rec_info.len;
		if (info->data)
			memcpy(info->data, cas_rec_info.data, info->len);
		return AM_SUCCESS;
	}
	return AM_FAILURE;
}
AM_ErrorCode_t AM_TFile_CasDump()
{
	struct list_head *pos, *q;
	tfile_cas_t *pcas;
	list_for_each_safe(pos, q, &cas_ctx.list) {
		pcas = list_entry(pos, tfile_cas_t, list);
		AM_DEBUG(0, "%s, cas info len:%d, pos:%lld", __func__, pcas->info_len, pcas->start);
	}

	return AM_SUCCESS;
}
#endif

ssize_t AM_TFile_Read(AM_TFile_t tfile, uint8_t *buf, size_t size, int timeout)
{
	ssize_t ret = -1;
	size_t todo, split;
	struct timespec rt;

	if (! tfile->opened)
	{
		AM_DEBUG(0, "[tfile] has not opened");
		return AM_FAILURE;
	}

	if (! tfile->loop)
	{
		ret = aml_timeshift_subfile_read(tfile, buf, size);
		if (ret > 0)
		{
			tfile->avail -= ret;
			tfile->read += ret;
			if (tfile->avail < 0)
				tfile->avail = 0;
		}
		return ret;
	}

	pthread_mutex_lock(&tfile->lock);
	if (tfile->avail <= 0)
	{
		AM_TIME_GetTimeSpecTimeout(timeout, &rt);
		pthread_cond_timedwait(&tfile->cond, &tfile->lock, &rt);
	}
	if (tfile->avail <= 0)
	{
		tfile->avail = 0;
		goto read_done;
	}
	todo = (size < tfile->avail) ? size : tfile->avail;
	size = todo;
	split = ((tfile->read+size) > tfile->size) ? (tfile->size - tfile->read) : 0;
	if (split > 0)
	{
		/*read -> end*/
		ret = aml_timeshift_subfile_read(tfile, buf, split);
		if (ret < 0)
			goto read_done;
		if (ret != (ssize_t)split)
		{
			tfile->read += ret;
			goto read_done;
		}

		tfile->read = 0;
		todo -= ret;
		buf += ret;
	}
	/*rewind the file*/
	if (split > 0)
		aml_timeshift_subfile_seek(tfile, 0, AM_TRUE);
	ret = aml_timeshift_subfile_read(tfile, buf, todo);
	if (ret > 0)
	{
		todo -= ret;
		tfile->read += ret;
		tfile->read %= tfile->size;
		ret = size - todo;
	}
	if (tfile->read == 0) {
		AM_DEBUG(0, "[tfile] reed is set 0 need to seek 0");
		aml_timeshift_subfile_seek(tfile, 0, AM_TRUE);
	}
read_done:
	if (ret > 0)
	{
		tfile->avail -= ret;
		if (tfile->avail < 0)
			tfile->avail = 0;
	}
	pthread_mutex_unlock(&tfile->lock);
	return ret;
}

ssize_t AM_TFile_Write(AM_TFile_t tfile, uint8_t *buf, size_t size)
{
	ssize_t ret = 0, ret2 = 0;
	size_t len1 = 0, len2 = 0;
	loff_t fsize, wpos;
	int now;
	size_t size_act = 0;
	int wrap = 0;
	int write_fail = 0;

	if (! tfile->opened)
	{
		AM_DEBUG(0, "[tfile] has not opened");
		return AM_FAILURE;
	}

	if (! tfile->loop)
	{
		/* Normal write */
		ret = aml_timeshift_subfile_write(tfile, buf, size);
		if (ret > 0)
		{
			pthread_mutex_lock(&tfile->lock);
			tfile->size += ret;
			tfile->write += ret;
			tfile->total += ret;
			tfile->avail += ret;
			pthread_mutex_unlock(&tfile->lock);
		}

		if (tfile->timer)
			timer_try_add_fragment_continuous(tfile->timer, tfile->write, 0);

		goto write_done;
	}

	pthread_mutex_lock(&tfile->lock);
	fsize = tfile->size;
	wpos = tfile->write;
	pthread_mutex_unlock(&tfile->lock);

	/*is the size exceed the file size?*/
	if (size > fsize)
	{
		size = fsize;
		AM_DEBUG(0, "[tfile] data lost, write size(%zu) > file size(%lld)?", size, fsize);
	}

	len1 = size;
	if ((wpos+len1) >= fsize)
	{
		len1 = (fsize - wpos);
		len2 = size - len1;
		wrap = 1;
		AM_DEBUG(2, "[tfile] wrap here, len[%d]=len1[%d]+len2[%d]", size, len1, len2);
	}
	if (len1 > 0)
	{
		/*write -> end*/
		ret = aml_timeshift_subfile_write(tfile, buf, len1);
		if (ret != (ssize_t)len1)
			write_fail = 1;

		if (tfile->timer)
			timer_try_add_fragment(tfile->timer,
					tfile->write + ret,
					(wrap && !write_fail)? 0 : tfile->write+ret,
					(wrap || (write_fail && ret))? 1 : 0);

		if (write_fail) {
			AM_DEBUG(0, "[tfile] data lost, eof write: try:%d act:%d", len1, ret);
			goto adjust_pos;
		}
	}

	if (len2 > 0)
	{
		/*rewind the file*/
		aml_timeshift_subfile_seek(tfile, 0, AM_FALSE);

		ret2 = aml_timeshift_subfile_write(tfile, buf+len1, len2);
		if (ret2 != (ssize_t)len2)
			write_fail = 1;

		if (tfile->timer)
			timer_try_add_fragment_continuous(tfile->timer, ret2, 1);

		if (write_fail) {
			AM_DEBUG(0, "[tfile] data lost, sof write: try:%d act:%d", len2, ret2);
			goto adjust_pos;
		}
	}

adjust_pos:

	size_act = ret + ret2;

	if (size_act > 0)
	{
		pthread_mutex_lock(&tfile->lock);

		int debug = 0;
		/*now, ret bytes actually written*/
		off_t rleft = tfile->size - tfile->avail;
		off_t sleft = tfile->size - tfile->total;

		tfile->write = (tfile->write + size_act) % tfile->size;
		if (tfile->write == 0) {
			/*rewind the file*/
			AM_DEBUG(0, "[tfile] write == 0, need seek write(0)read(%lld)", tfile->read);
			aml_timeshift_subfile_seek(tfile, 0, AM_FALSE);
		}
		//check the start pointer
		if (size_act > sleft) {
			if (tfile->timer) {
				loff_t start;
				loff_t remove = 0;
				do {
					remove += timer_remove_earliest_fragment(tfile->timer, &start);
					} while(remove < size_act);
				tfile->start = start;//tfile->write;
				tfile->total -= remove;
				//tfile->avail -= remove;
				AM_DEBUG(3, "[tfile] size:%zu w >> s, s:%lld, t:%lld", size_act, tfile->start, tfile->total);
				debug = 1;
			} else {
				tfile->start = tfile->write;
				tfile->total -= size_act;
			}
		}

		//check the read pointer
		if (size_act > rleft) {
			tfile->read = tfile->start;
			tfile->avail -= size_act - rleft;
			//AM_DEBUG(3, "ttt size:%zu w >> r, r:%lld, a:%lld",
			//    size_act, tfile->read, tfile->avail);
			//debug = 1;
		}

		if (tfile->avail < tfile->size)
		{
			tfile->avail += size_act;
			if (tfile->avail > tfile->size)
				tfile->avail = tfile->size;
			pthread_cond_signal(&tfile->cond);
		}
		if (tfile->total < tfile->size)
		{
			tfile->total += size_act;
			if (tfile->total > tfile->size)
				tfile->total = tfile->size;
		}

		if (tfile->avail > tfile->total) {
			tfile->read = tfile->start;
			tfile->avail = tfile->total;
		}

		if (debug) {
			AM_DEBUG(3, "[tfile] size:%zu now -> s:%lld, r:%lld, w:%lld, t:%lld, a:%lld",
				size_act,
				tfile->start, tfile->read, tfile->write, tfile->total, tfile->avail);

			if (tfile->timer)
				timer_get_total(tfile->timer);
		}
		pthread_mutex_unlock(&tfile->lock);
	}

write_done:

#if 1
	//calc rate and notify rate ready
	if (ret > 0 && !tfile->rate)
	{
		AM_TIME_GetClock(&now);
		if (! tfile->wlast) {
			tfile->wtotal = 0;
			tfile->wlast = now;
		} else {
			tfile->wtotal += size;
			if ((now - tfile->wlast) >= 3000) {
				/*Calcaulate the rate*/
				tfile->rate = (tfile->wtotal*1000)/(now - tfile->wlast);
				AM_DEBUG(1, "[tfile] got rate:%dBps", tfile->rate);
				if (tfile->rate) {
					if (tfile->loop) {
						if (tfile->duration >= 0) {
							loff_t size = (loff_t)tfile->rate * (loff_t)tfile->duration;
							if (size < tfile->size) {
								tfile->size = size;
								AM_DEBUG(1, "[tfile] file size -> %lld", tfile->size);
								AM_EVT_Signal((long)tfile, AM_TFILE_EVT_SIZE_CHANGED, (void*)(long)tfile->size);
							}
						} else {
							int duration = tfile->size / tfile->rate;
							tfile->duration = duration;
							AM_DEBUG(1, "[tfile] file duration -> %ds", tfile->duration);
							AM_EVT_Signal((long)tfile, AM_TFILE_EVT_DURATION_CHANGED, (void*)(long)tfile->duration);
						}
					}
					AM_EVT_Signal((long)tfile, AM_TFILE_EVT_RATE_CHANGED, (void*)(long)tfile->rate);
				} else {
					tfile->wlast = 0;
				}
			}
		}
	}
#endif
	return ret + ret2;
}

/**\brief seek到指定的偏移，如越界则返�?*/
int AM_TFile_Seek(AM_TFile_t tfile, loff_t offset)
{
	int ret = 1;
	if (! tfile->opened)
	{
		AM_DEBUG(0, "[tfile] has not opened");
		return AM_FAILURE;
	}

	pthread_mutex_lock(&tfile->lock);
	if (tfile->size <= 0)
	{
		pthread_mutex_unlock(&tfile->lock);
		return ret;
	}
	if (offset > tfile->total)
		offset = tfile->total - 1;
	else if (offset < 0)
		offset = 0;
	else
		ret = 0;

	tfile->read = (tfile->start + offset)%tfile->size;
	tfile->avail = tfile->total - offset;
	if (tfile->avail > 0)
		pthread_cond_signal(&tfile->cond);

	aml_timeshift_subfile_seek(tfile, tfile->read, AM_TRUE);

	AM_DEBUG(3, "[tfile] Seek: start %lld, read %lld, write %lld, avail %lld, total %lld",
				tfile->start, tfile->read, tfile->write, tfile->avail, tfile->total);
	pthread_mutex_unlock(&tfile->lock);
	return ret;
}

loff_t AM_TFile_Tell(AM_TFile_t tfile)
{
	loff_t ret = 0;
	if (! tfile->opened)
	{
		AM_DEBUG(0, "[tfile] has not opened");
		return AM_FAILURE;
	}

	pthread_mutex_lock(&tfile->lock);
	ret = tfile->read;
	pthread_mutex_unlock(&tfile->lock);
	return ret;
}

loff_t AM_TFile_GetAvailable(AM_TFile_t tfile)
{
	int ret = 0;
	if (! tfile->opened)
	{
		AM_DEBUG(0, "[tfile] has not opened");
		return AM_FAILURE;
	}

	pthread_mutex_lock(&tfile->lock);
	ret = tfile->avail;
	pthread_mutex_unlock(&tfile->lock);
	return ret;
}

int AM_TFile_TimeStart(AM_TFile_t tfile)
{
	if (! tfile->opened)
	{
		AM_DEBUG(0, "[tfile] has not opened");
		return AM_FAILURE;
	}

	return aml_tfile_attach_timer(tfile);
}

//one node for guard time //need more research, fatal exists
//#define get_list_start(_ptimer_) ((_ptimer_)->wrap? (_ptimer_)->list.next : &(_ptimer_)->list)
#define get_list_start(_ptimer_)  (&(_ptimer_)->list)


int AM_TFile_TimeSeek(AM_TFile_t tfile, int offset_ms/*offset from time start*/)
{
	int ret = -1;
	if (! tfile->opened)
	{
		AM_DEBUG(0, "[tfile] has not opened");
		return -1;
	}
	pthread_mutex_lock(&tfile->lock);
	if (tfile->timer) {
		tfile_timer_t *timer = (tfile_timer_t*)tfile->timer;
		tfragment_t *pstart = list_first_entry(get_list_start(timer), tfragment_t, head);
		loff_t from = pstart->start;
		tfragment_t *p;
		list_for_each_entry(p, get_list_start(timer), head)
		{
			if (p->time > offset_ms)
				break;
			from = p->start;
		}
		tfile->read = from;

		aml_timeshift_subfile_seek(tfile, tfile->read, AM_TRUE);

		tfile->avail = (tfile->write >= pstart->start) ? tfile->write - pstart->start : (tfile->size - pstart->start) + tfile->write;
		if (tfile->avail > 0)
			pthread_cond_signal(&tfile->cond);

		AM_DEBUG(3, "[tfile] >>timeseek: pstart: %p[time:%d offset:%d - %d]", pstart, pstart->time, pstart->start, pstart->end);
		AM_DEBUG(3, "[tfile] >>timeseek: got block: next%p[time:%d offset:%d - %d]", p, p->time, p->start, p->end);
		AM_DEBUG(3, "[tfile] >>timeseek: start %lld, read %lld, write %lld, avail %lld, total %lld",
				tfile->start, tfile->read, tfile->write, tfile->avail, tfile->total);
		ret = 0;
	}
	pthread_mutex_unlock(&tfile->lock);
	return ret;
}

int AM_TFile_TimeGetReadNow(AM_TFile_t tfile)
{
	int now = 0;
	if (! tfile->opened)
	{
		AM_DEBUG(0, "[tfile] has not opened");
		return -1;
	}

	pthread_mutex_lock(&tfile->lock);
	if (tfile->timer) {
		loff_t r = tfile->read;
		tfile_timer_t *timer = (tfile_timer_t*)tfile->timer;
		tfragment_t *pstart = list_first_entry(get_list_start(timer), tfragment_t, head);
		tfragment_t *pend = list_entry(timer->list.prev, tfragment_t, head);
		if (r > pstart->start) {
			/*
				| s--i--e |
				or
				|       s---i--|
				|--e           |
			*/
			now = pstart->time;
			tfragment_t *p;
			list_for_each_entry(p, get_list_start(timer), head)
			{
				if (p->start > r)
					break;
				now = p->time;
			}
		} else if (r < pstart->start) {
			/*
				|           s---|
				|--i-e          |
			*/
			now = pend->time;
			struct list_head *pos;
			list_for_each_prev(pos, &timer->list)
			{
				tfragment_t *p = list_entry(pos, tfragment_t, head);
				if (p->start <= r) {
					now = p->time;
					break;
				}
			}
		} else {
			now = pstart->time;
		}
		AM_DEBUG(3, "[tfile] >>read:\ttime[%d]\toffset[%lld]", now, tfile->read);
	}
	pthread_mutex_unlock(&tfile->lock);
	return now;
}

int AM_TFile_TimeGetStart(AM_TFile_t tfile)
{
	int time = 0;
	if (!tfile->opened)
	{
		AM_DEBUG(0, "[tfile] has not opened");
		return -1;
	}

	pthread_mutex_lock(&tfile->lock);
	if (tfile->timer) {
		tfile_timer_t *timer = (tfile_timer_t*)tfile->timer;
		tfragment_t *pstart = list_first_entry(get_list_start(timer), tfragment_t, head);
		time = pstart->time;
		AM_DEBUG(3, "[tfile] >>start:\ttime[%d]\toffset[%lld]", pstart->time, pstart->start);
	}
	pthread_mutex_unlock(&tfile->lock);
	return time;
}

int AM_TFile_TimeGetEnd(AM_TFile_t tfile)
{
	int time = 0;
	if (!tfile->opened)
	{
		AM_DEBUG(0, "[tfile] has not opened");
		return -1;
	}

	pthread_mutex_lock(&tfile->lock);
	if (tfile->timer) {
		tfile_timer_t *timer = (tfile_timer_t*)tfile->timer;
		tfragment_t *pend = list_entry(timer->list.prev, tfragment_t, head);
		time = pend->time;
		AM_DEBUG(3, "[tfile] >>end:\ttime[%d]\toffset[%lld]", pend->time, pend->start);
	}
	pthread_mutex_unlock(&tfile->lock);
	return time;
}

