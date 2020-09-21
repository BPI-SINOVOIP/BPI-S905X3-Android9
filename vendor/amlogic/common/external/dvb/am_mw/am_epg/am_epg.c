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
/**\file am_epg.c
 * \brief 表监控模块
 *
 * \author Xia Lei Peng <leipeng.xia@amlogic.com>
 * \date 2010-11-04: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 5
//#define TAG_EXT "-epg"

#include <errno.h>
#include <time.h>
#include <am_debug.h>
#include <assert.h>
#include <am_epg.h>
#include "am_epg_internal.h"
#include <am_time.h>
#include <am_dmx.h>
#include <am_iconv.h>
#include <am_av.h>
#include <am_cond.h>
#ifdef ANDROID
#include <cutils/properties.h>
#include <sys/system_properties.h>
#endif


/****************************************************************************
 * Macro definitions
 ***************************************************************************/

/*是否使用TDT时间*/
#define USE_TDT_TIME

/*子表最大个数*/
#define MAX_EIT4E_SUBTABLE_CNT 500
#define MAX_EIT_SUBTABLE_CNT 1000

/*多子表时接收重复最小间隔*/
#define EIT4E_REPEAT_DISTANCE 3000
#define EIT4F_REPEAT_DISTANCE 5000
#define EIT50_REPEAT_DISTANCE 10000
#define EIT51_REPEAT_DISTANCE 10000
#define EIT60_REPEAT_DISTANCE 20000
#define EIT61_REPEAT_DISTANCE 20000

#define PSIP_EIT_REPEAT_DISTANCE 2000
#define PSIP_ETT_REPEAT_DISTANCE 2000

/*EIT 数据定时通知时间间隔*/
#define NEW_EIT_CHECK_DISTANCE 4000

/*EIT 自动更新间隔*/
#define EITPF_CHECK_DISTANCE (60*1000)
#define EITSCHE_CHECK_DISTANCE (3*3600*1000)

/*TDT 自动更新间隔*/
#define TDT_CHECK_DISTANCE 1000*3600

/*STT 自动更新间隔*/
#define STT_CHECK_DISTANCE 1000*3600

/*预约播放检查间隔*/
#define EPG_SUB_CHECK_TIME (10*1000)
/*预约播放提前通知时间*/
#define EPG_PRE_NOTIFY_TIME (60*1000)

/*并行接收ATSC EIT的个数*/
//#define PARALLEL_PSIP_EIT_CNT 4
/*并行接收ATSC ETT的个数*/
//#define PARALLEL_PSIP_ETT_CNT 4
#define PARALLEL_PSIP_ETT_CNT_PROP "tv.parallel.psip.ett.cnt"
#define PARALLEL_PSIP_EIT_CNT_PROP "tv.parallel.psip.eit.cnt"

 /*位操作*/
#define BIT_MASK(b) (1 << ((b) % 8))
#define BIT_SLOT(b) ((b) / 8)
#define BIT_SET(a, b) ((a)[BIT_SLOT(b)] |= BIT_MASK(b))
#define BIT_CLEAR(a, b) ((a)[BIT_SLOT(b)] &= ~BIT_MASK(b))
#define BIT_TEST(a, b) ((a)[BIT_SLOT(b)] & BIT_MASK(b))
#define BIT_MASK_EX(b) (0xff >> (7 - ((b) % 8)))
#define BIT_CLEAR_EX(a, b) ((a)[BIT_SLOT(b)] &= BIT_MASK_EX(b))

#define dvbpsi_rrt_t rrt_section_info_t

/*清除一个subtable控制数据*/
#define SUBCTL_CLEAR(sc)\
	AM_MACRO_BEGIN\
		memset((sc)->mask, 0, sizeof((sc)->mask));\
		(sc)->ver = 0xff;\
	AM_MACRO_END
	
/*添加数据到列表中*/
#define ADD_TO_LIST(_t, _l)\
	AM_MACRO_BEGIN\
		if ((_l) == NULL){\
			(_l) = (_t);\
		}else{\
			(_t)->p_next = (_l)->p_next;\
			(_l)->p_next = (_t);\
		}\
	AM_MACRO_END
/*释放一个表的所有SI数据*/
#define RELEASE_TABLE_FROM_LIST(_t, _l)\
	AM_MACRO_BEGIN\
		_t *tmp, *next;\
		tmp = (_l);\
		while (tmp){\
			next = tmp->p_next;\
			AM_SI_ReleaseSection(mon->hsi, tmp->i_table_id, (void*)tmp);\
			tmp = next;\
		}\
		(_l) = NULL;\
	AM_MACRO_END

/*解析section并添加到列表*/
#define COLLECT_SECTION(type, list)\
	AM_MACRO_BEGIN\
		type *p_table;\
		if (AM_SI_DecodeSection(mon->hsi, sec_ctrl->pid, (uint8_t*)data, len, (void**)&p_table) == AM_SUCCESS){\
			/* process this section */\
			if (sec_ctrl->proc_sec) {\
				mon->current_fid = sec_ctrl->fid; \
				sec_ctrl->proc_sec(mon, (void*)p_table);\
			}\
			if (sec_ctrl->pid == AM_SI_PID_EIT){\
				/* notify dvb eit */\
				am_epg_tablectl_mark_section_eit(sec_ctrl, &header, data[12]);\
				SIGNAL_EVENT(AM_EPG_EVT_NEW_EIT, (void*)p_table);\
				AM_SI_ReleaseSection(mon->hsi, data[0], (void*)p_table);\
			} else if (sec_ctrl->pid == AM_SI_PID_TOT || data[0] == AM_SI_TID_PSIP_STT) {\
				/* dvb tdt/tot, atsc stt, TDT/TOT/STT has only 1 section */\
				p_table->p_next = NULL;\
				ADD_TO_LIST(p_table, list);\
				TABLE_DONE();\
			} else if (data[0] == AM_SI_TID_PSIP_EIT){\
				/* notify atsc eit */\
				am_epg_tablectl_mark_section(sec_ctrl, &header); \
				SIGNAL_EVENT(AM_EPG_EVT_NEW_PSIP_EIT, (void*)p_table);\
				AM_SI_ReleaseSection(mon->hsi, data[0], (void*)p_table);\
			} else if (data[0] == AM_SI_TID_PSIP_ETT){\
				/* notify atsc ett */\
				am_epg_tablectl_mark_section(sec_ctrl, &header); \
				SIGNAL_EVENT(AM_EPG_EVT_NEW_PSIP_ETT, (void*)p_table);\
				AM_SI_ReleaseSection(mon->hsi, data[0], (void*)p_table);\
			} else {\
				/*For non-eit/tot/stt sections, store to table list*/\
				p_table->p_next = NULL;\
				ADD_TO_LIST(p_table, list); /*添加到搜索结果列表中*/\
				am_epg_tablectl_mark_section(sec_ctrl, &header); /*设置为已接收*/\
			}\
		} else {\
			AM_DEBUG(1, "EPG: Decode %s section failed", sec_ctrl->tname);\
		}\
	AM_MACRO_END

/*判断并设置某个表的监控*/
#define SET_MODE_EX(mode, table, ctl, f, reset)\
	AM_MACRO_BEGIN\
		if (((mode) & (f)) && (mon->ctl.fid == -1) && !(reset))\
		{/*开启监控*/\
			am_epg_request_section(mon, &mon->ctl);\
			AM_DEBUG(2, "start section -> 0x%x:0x%x [%s]", mon->ctl.pid, mon->ctl.tid, mon->ctl.tname);\
		}\
		else if (!((mode) & (f)))\
		{/*关闭监控*/\
			if (mon->ctl.fid != -1)\
				AM_DEBUG(2, "stop section -> 0x%x:0x%x [%s]", mon->ctl.pid, mon->ctl.tid, mon->ctl.tname);\
			am_epg_free_filter(mon, &mon->ctl.fid);\
			RELEASE_TABLE_FROM_LIST(dvbpsi_##table##_t, mon->table##s);\
			am_epg_tablectl_clear(&mon->ctl);\
		}\
		else if (((mode) & (f)) && (reset))\
		{\
			am_epg_free_filter(mon, &mon->ctl.fid);\
			RELEASE_TABLE_FROM_LIST(dvbpsi_##table##_t, mon->table##s);\
			am_epg_tablectl_clear(&mon->ctl);\
			am_epg_request_section(mon, &mon->ctl);\
			AM_DEBUG(2, "restart section -> 0x%x:0x%x [%s]", mon->ctl.pid, mon->ctl.tid, mon->ctl.tname);\
		}\
	AM_MACRO_END

#define SET_MODE(table, ctl, f, reset) \
	SET_MODE_EX(mon->mode, table, ctl, f, reset)
#define SET_MODE_FORCE(table, ctl, f, reset) \
	SET_MODE_EX(mon->mode|f, table, ctl, f, reset)

/*表收齐操作*/
#define TABLE_DONE()\
	AM_MACRO_BEGIN\
		if (! (mon->evt_flag & sec_ctrl->evt_flag))\
		{\
			AM_DEBUG(2, "table done [%s], subcnt:%d", sec_ctrl->tname, sec_ctrl->subs);\
			mon->evt_flag |= sec_ctrl->evt_flag;\
			if (sec_ctrl->tid == AM_SI_TID_PSIP_EIT) {\
				BIT_SET(mon->psip_eit_done_flag, sec_ctrl - mon->psip_eitctl);\
			} else if(sec_ctrl->tid == AM_SI_TID_PSIP_ETT) {\
				BIT_SET(mon->psip_ett_done_flag, sec_ctrl - mon->psip_ettctl);\
			}\
			pthread_cond_signal(&mon->cond);\
		}\
	AM_MACRO_END

/*通知一个事件*/
#define SIGNAL_EVENT(e, d)\
	AM_MACRO_BEGIN\
		pthread_mutex_unlock(&mon->lock);\
		AM_EVT_Signal((long)mon, e, d);\
		pthread_mutex_lock(&mon->lock);\
	AM_MACRO_END
	
#define STEP_STMT(_stmt, _name, _sql)\
	AM_MACRO_BEGIN\
		int ret1, ret2;\
			ret1 = sqlite3_step(_stmt);\
			ret2 = sqlite3_reset(_stmt);\
			if (ret1 == SQLITE_ERROR && ret2 == SQLITE_SCHEMA){\
				AM_DEBUG(1, "Database schema changed, now re-prepare the stmts...");\
				AM_DB_GetSTMT(&(_stmt), _name, _sql, 1);\
			}\
	AM_MACRO_END


#define HAS_MODE(_ms, _m) (((_ms) & (_m)) == (_m))

/****************************************************************************
 * Static data
 ***************************************************************************/

#ifdef USE_TDT_TIME
/*当前时间管理*/
static AM_EPG_Time_t curr_time = {0, 0, 0};
static long curr_gps_time_offset = 0;

/*当前时间锁*/
static pthread_mutex_t time_lock = PTHREAD_MUTEX_INITIALIZER;
#endif

/****************************************************************************
 * Static functions
 ***************************************************************************/
static int am_epg_get_current_service_id(AM_EPG_Monitor_t *mon);
static AM_EPG_TableCtl_t *am_epg_get_section_ctrl_by_fid(AM_EPG_Monitor_t *mon, int fid);

#if 0
static inline int am_epg_convert_fetype_to_source(int fe_type)
{
	switch(fe_type)
	{
		case FE_QAM:	return AM_FEND_DEMOD_DVBC; break;
		case FE_OFDM:	return AM_FEND_DEMOD_DVBT;  break;
		case FE_QPSK:	return AM_FEND_DEMOD_DVBS;  break;
		case FE_ATSC:	
		default:			break;
	}
	return -1#else
	return fe_type;
}
#endif

static void am_epg_wait_events_done(AM_EPG_Monitor_t *mon)
{
	if (mon->thread == pthread_self())
		return;

	while (mon->evt_flag != 0)
	{
		pthread_cond_wait(&mon->cond, &mon->lock);
	}
}

/**\brief 预约一个EPG事件*/
static AM_ErrorCode_t am_epg_subscribe_event(sqlite3 *hdb, int db_evt_id)
{
	char sql[128];
	char *errmsg;

	if (hdb == NULL)
		return AM_EPG_ERR_INVALID_PARAM;
		
	/*由应用处理时间段冲突*/
	snprintf(sql, sizeof(sql), "update evt_table set sub_flag=1 where db_id=%d", db_evt_id);

	if (sqlite3_exec(hdb, sql, NULL, NULL, &errmsg) != SQLITE_OK)
	{
		AM_DEBUG(0, "Subscribe EPG event for db_id=%d failed, reason: %s", db_evt_id, errmsg ? errmsg : "Unknown");
		if (errmsg)
			sqlite3_free(errmsg);
		return AM_EPG_ERR_SUBSCRIBE_EVENT_FAILED;
	}

	return AM_SUCCESS;
}

/**\brief 取消预约一个EPG事件*/
static AM_ErrorCode_t am_epg_unsubscribe_event(sqlite3 *hdb, int db_evt_id)
{
	char sql[128];
	char *errmsg;

	if (hdb == NULL)
		return AM_EPG_ERR_INVALID_PARAM;
		
	/*由应用处理时间段冲突*/
	snprintf(sql, sizeof(sql), "update evt_table set sub_flag=0,sub_status=0 where db_id=%d", db_evt_id);

	if (sqlite3_exec(hdb, sql, NULL, NULL, &errmsg) != SQLITE_OK)
	{
		AM_DEBUG(0, "Unsubscribe EPG event for db_id=%d failed, reason: %s", db_evt_id, errmsg ? errmsg : "Unknown");
		if (errmsg)
			sqlite3_free(errmsg);
		return AM_EPG_ERR_SUBSCRIBE_EVENT_FAILED;
	}

	return AM_SUCCESS;
}

/**\brief 删除过期的event*/
static AM_ErrorCode_t am_epg_delete_expired_events(sqlite3 *hdb)
{
	int now;
	char sql[128];
	char *errmsg;
	
	if (hdb == NULL)
		return AM_EPG_ERR_INVALID_PARAM;
		
	AM_DEBUG(1, "Deleting expired epg events...");
	AM_EPG_GetUTCTime(&now);
	snprintf(sql, sizeof(sql), "delete from evt_table where end<%d", now);
	if (sqlite3_exec(hdb, sql, NULL, NULL, &errmsg) != SQLITE_OK)
	{
		AM_DEBUG(0, "Delete expired events failed: %s", errmsg ? errmsg : "Unknown");
		if (errmsg)
			sqlite3_free(errmsg);
		return AM_FAILURE;
	}
	AM_DEBUG(1, "Delete expired epg events done!");
	return AM_SUCCESS;
}

/**\brief 释放过滤器，并保证在此之后不会再有无效数据*/
static void am_epg_free_filter(AM_EPG_Monitor_t *mon,  int *fid)
{
	if (*fid == -1)
		return;
	
	AM_DMX_FreeFilter(mon->dmx_dev, *fid);
	*fid = -1;
	pthread_mutex_unlock(&mon->lock);
	/*等待无效数据处理完毕*/
	AM_DMX_Sync(mon->dmx_dev);
	pthread_mutex_lock(&mon->lock);
}

/**\brief 清空一个表控制标志*/
static void am_epg_tablectl_clear(AM_EPG_TableCtl_t * mcl)
{
	mcl->data_arrive_time = 0;
	mcl->check_time = 0;
	if (mcl->subs && mcl->subctl)
	{
		int i;

		memset(mcl->subctl, 0, sizeof(AM_EPG_SubCtl_t) * mcl->subs);
		for (i=0; i<mcl->subs; i++)
		{
			mcl->subctl[i].ver = 0xff;
		}
	}
}

 /**\brief 初始化一个表控制结构*/
static AM_ErrorCode_t am_epg_tablectl_init(AM_EPG_TableCtl_t * mcl, int evt_flag,
						uint16_t pid, uint8_t tid, uint8_t tid_mask,
						const char *name, uint16_t sub_cnt, 
						void (*done)(struct AM_EPG_Monitor_s *), int distance,
						void (*proc_sec)(struct AM_EPG_Monitor_s *, void *))
{
	memset(mcl, 0, sizeof(AM_EPG_TableCtl_t));
	mcl->fid = -1;
	mcl->evt_flag = evt_flag;
	mcl->pid = pid;
	mcl->tid = tid;
	mcl->tid_mask = tid_mask;
	mcl->done = done;
	mcl->repeat_distance = distance;
	mcl->proc_sec = proc_sec;
	strcpy(mcl->tname, name);

	mcl->subs = sub_cnt;
	if (mcl->subs)
	{
		mcl->subctl = (AM_EPG_SubCtl_t*)malloc(sizeof(AM_EPG_SubCtl_t) * mcl->subs);
		if (!mcl->subctl)
		{
			mcl->subs = 0;
			AM_DEBUG(1, "Cannot init tablectl, no enough memory");
			return AM_EPG_ERR_NO_MEM;
		}

		am_epg_tablectl_clear(mcl);
	}

	return AM_SUCCESS;
}

/**\brief 反初始化一个表控制结构*/
static void am_epg_tablectl_deinit(AM_EPG_TableCtl_t * mcl)
{
	if (mcl->subctl)
	{
		free(mcl->subctl);
		mcl->subctl = NULL;
	}
}

/**\brief 判断一个表的所有section是否收齐*/
static AM_Bool_t am_epg_tablectl_test_complete(AM_EPG_TableCtl_t * mcl)
{
	static uint8_t test_array[32] = {0};
	int i;

	for (i=0; i<mcl->subs; i++)
	{
		if ((mcl->data_arrive_time == 0) || 
			((mcl->subctl[i].ver != 0xff) &&
			memcmp(mcl->subctl[i].mask, test_array, sizeof(test_array)))){
			return AM_FALSE;
		}
	}

	return AM_TRUE;
}

/**\brief 判断一个表的指定section是否已经接收*/
static AM_Bool_t am_epg_tablectl_test_recved(AM_EPG_TableCtl_t * mcl, AM_SI_SectionHeader_t *header)
{
	int i;
	
	if (!mcl->subctl)
		return AM_TRUE;
    
	for (i=0; i<mcl->subs; i++)
	{
		if ((mcl->subctl[i].ext == header->extension) && 
			(mcl->subctl[i].ver == header->version) && 
			(mcl->subctl[i].last == header->last_sec_num) && 
			//(mcl->subctl[i].leng == header->length) &&
			!BIT_TEST(mcl->subctl[i].mask, header->sec_num))
		{
			if ((mcl->subs > 1) && (mcl->data_arrive_time == 0))
				AM_TIME_GetClock(&mcl->data_arrive_time);
			
			return AM_TRUE;
		}
	}
	
	return AM_FALSE;
}

/**\brief 在一个表中增加一个EITsection已接收标识*/
static AM_ErrorCode_t am_epg_tablectl_mark_section_eit(AM_EPG_TableCtl_t	 * mcl, 
							AM_SI_SectionHeader_t *header, 
							int seg_last_sec)
{
	int i;
	AM_EPG_SubCtl_t *sub, *fsub;

	if (!mcl->subctl || seg_last_sec > header->last_sec_num)
		return AM_FAILURE;

	sub = fsub = NULL;
	for (i=0; i<mcl->subs; i++)
	{
		if (mcl->subctl[i].ext == header->extension)
		{
			sub = &mcl->subctl[i];
			break;
		}
		/*记录一个空闲的结构*/
		if ((mcl->subctl[i].ver == 0xff) && !fsub)
			fsub = &mcl->subctl[i];
	}
	
	if (!sub && !fsub)
	{
		AM_DEBUG(1, "No more subctl for adding new %s subtable", mcl->tname);
		return AM_FAILURE;
	}
	if (!sub) {
		sub = fsub;
	}
	
	/*发现新版本，重新设置接收控制*/
	if (sub->ver != 0xff && (sub->ver != header->version ||\
		sub->ext != header->extension || sub->last != header->last_sec_num))
		SUBCTL_CLEAR(sub);

	if (sub->ver == 0xff)
	{
		int i;
		
		/*接收到的第一个section*/
		sub->last = header->last_sec_num;
		sub->ver = header->version;	
		sub->ext = header->extension;
		//sub->leng = header->length;
		/*设置未接收标识*/
		for (i=0; i<(sub->last+1); i++)
			BIT_SET(sub->mask, i);
	}

	/*设置已接收标识*/
	BIT_CLEAR(sub->mask, header->sec_num);

	/*设置segment中未使用的section标识为已接收*/
	if (seg_last_sec >= 0)
		BIT_CLEAR_EX(sub->mask, (uint8_t)seg_last_sec);

	if (mcl->data_arrive_time == 0)
		AM_TIME_GetClock(&mcl->data_arrive_time);

	return AM_SUCCESS;
}

/**\brief 在一个表中增加一个section已接收标识*/
static AM_ErrorCode_t am_epg_tablectl_mark_section(AM_EPG_TableCtl_t * mcl, AM_SI_SectionHeader_t *header)
{
	return am_epg_tablectl_mark_section_eit(mcl, header, -1);
}


static int am_epg_get_current_service_id(AM_EPG_Monitor_t *mon)
{
	int row = 1;
	int service_id = 0xffff;
	char sql[256];
	sqlite3 *hdb;

	AM_DB_HANDLE_PREPARE(hdb);

	snprintf(sql, sizeof(sql), "select service_id from srv_table where db_id=%d", mon->mon_service);
	AM_DB_Select(hdb, sql, &row, "%d", &service_id);
	return service_id;
}

static int am_epg_get_current_db_ts_id(AM_EPG_Monitor_t *mon)
{
	int row = 1;
	int db_ts_id = -1;
	char sql[256];
	sqlite3 *hdb;

	AM_DB_HANDLE_PREPARE(hdb);
	
	snprintf(sql, sizeof(sql), "select db_ts_id from srv_table where db_id=%d", mon->mon_service);
	AM_DB_Select(hdb, sql, &row, "%d", &db_ts_id);
	return db_ts_id;
}

static void format_audio_strings(AM_SI_AudioInfo_t *ai, char *pids, char *fmts, char *langs)
{
	int i;
	
	if (ai->audio_count < 0)
		ai->audio_count = 0;
		
	pids[0] = 0;
	fmts[0] = 0;
	langs[0] = 0;
	for (i=0; i<ai->audio_count; i++)
	{
		if (i == 0)
		{
			snprintf(pids, 256, "%d", ai->audios[i].pid);
			snprintf(fmts, 256, "%d", ai->audios[i].fmt);
			sprintf(langs, "%s", ai->audios[i].lang);
		}
		else
		{
			snprintf(pids, 256, "%s %d", pids, ai->audios[i].pid);
			snprintf(fmts, 256, "%s %d", fmts, ai->audios[i].fmt);
			snprintf(langs, 256, "%s %s", langs, ai->audios[i].lang);
		}
	}
}


static AM_Bool_t am_epg_check_program_av(sqlite3 * hdb, int db_srv_id, int vid, int vfmt, AM_SI_AudioInfo_t *aud_info)
{
	int row = 1;
	int prev_vid = 0x1fff, prev_vfmt = -1;
	char sql[1024];
	char str_prev_apids[256];
	char str_prev_afmts[256];
	char str_prev_alangs[256];
	AM_Bool_t ret = AM_FALSE;
	
	/* is video/audio changed ? */
	snprintf(sql, sizeof(sql), "select vid_pid,vid_fmt,aud_pids,aud_fmts,aud_langs \
		from srv_table where db_id=%d", db_srv_id);
	if (AM_DB_Select(hdb, sql, &row, "%d,%d,%s:256,%s:256,%s:256", &prev_vid, 
		&prev_vfmt, str_prev_apids, str_prev_afmts, str_prev_alangs) == AM_SUCCESS && row > 0)
	{
		int i;
		char *str_tok;
		char str_apids[256];
		char str_afmts[256];
		char str_alangs[256];
		AM_SI_AudioInfo_t cur_aud_info;
		
		memset(&cur_aud_info, 0, sizeof(cur_aud_info));
		i = 0;
		AM_TOKEN_PARSE_BEGIN(str_prev_apids, " ", str_tok)
			if (i < (int)AM_ARRAY_SIZE(cur_aud_info.audios))
				cur_aud_info.audios[i].pid = atoi(str_tok);
			else
				break;
			i++;
			cur_aud_info.audio_count++;
		AM_TOKEN_PARSE_END(str_prev_apids, " ", str_tok)
		i = 0;
		AM_TOKEN_PARSE_BEGIN(str_prev_afmts, " ", str_tok)
			if (i < (int)AM_ARRAY_SIZE(cur_aud_info.audios))
				cur_aud_info.audios[i].fmt = atoi(str_tok);
			else
				break;
			i++;
		AM_TOKEN_PARSE_END(str_prev_afmts, " ", str_tok)
		i = 0;
		AM_TOKEN_PARSE_BEGIN(str_prev_alangs, " ", str_tok)
			if (i < (int)AM_ARRAY_SIZE(cur_aud_info.audios))
				memcpy(&cur_aud_info.audios[i].lang, str_tok, 3);
			else
				break;
			i++;
		AM_TOKEN_PARSE_END(str_prev_alangs, " ", str_tok)
		
		if (vid != prev_vid || vfmt != prev_vfmt)
		{
			ret = AM_TRUE;
		}
		else
		{
			int j;
			for (i=0; i<cur_aud_info.audio_count; i++)
			{
				for (j=0; j<aud_info->audio_count; j++)
				{
					if (cur_aud_info.audios[i].pid == aud_info->audios[j].pid &&
						cur_aud_info.audios[i].fmt == aud_info->audios[j].fmt &&
						!strncmp(cur_aud_info.audios[i].lang, aud_info->audios[j].lang, 3))
						break;
				}
				if (j >= aud_info->audio_count)
				{
					ret = AM_TRUE;
					break;
				}
			}
		}
		
		if (ret)
		{
			format_audio_strings(aud_info, str_apids, str_afmts, str_alangs);
			AM_DEBUG(1, "@@ Video/Audio changed @@");
			AM_DEBUG(1, "Video pid/fmt: (%d/%d) -> (%d/%d)", prev_vid, prev_vfmt, vid, vfmt);
			AM_DEBUG(1, "Audio pid/fmt/lang: ('%s'/'%s'/'%s') -> ('%s'/'%s'/'%s')",
				str_prev_apids, str_prev_afmts, str_prev_alangs, str_apids, str_afmts, str_alangs);
			/* update to database */
			snprintf(sql, sizeof(sql), "update srv_table set vid_pid=%d,vid_fmt=%d,\
				aud_pids='%s',aud_fmts='%s',aud_langs='%s', current_aud=-1 where db_id=%d", 
				vid, vfmt, str_apids, str_afmts, str_alangs, db_srv_id);
			sqlite3_exec(hdb, sql, NULL, NULL, NULL);
		}
		else
		{
			AM_DEBUG(2, "@ Video & Audio not changed ! @");
		}
	}
	
	return ret;
}

static void am_epg_proc_tot_section(AM_EPG_Monitor_t *mon, void *tot_section)
{
	dvbpsi_tot_t *p_tot = (dvbpsi_tot_t*)tot_section;
	uint16_t mjd;
	uint8_t hour, min, sec;

	UNUSED(mon);

	/*取UTC时间*/
	mjd = (uint16_t)(p_tot->i_utc_time >> 24);
	hour = (uint8_t)(p_tot->i_utc_time >> 16);
	min = (uint8_t)(p_tot->i_utc_time >> 8);
	sec = (uint8_t)p_tot->i_utc_time;

	pthread_mutex_lock(&time_lock);
	curr_time.tdt_utc_time = AM_EPG_MJD2SEC(mjd) + AM_EPG_BCD2SEC(hour, min, sec);
	/*更新system time，用于时间自动累加*/
	AM_TIME_GetClock(&curr_time.tdt_sys_time);
	pthread_mutex_unlock(&time_lock);
}

static void am_epg_proc_stt_section(AM_EPG_Monitor_t *mon, void *stt_section)
{
	dvbpsi_atsc_stt_t *p_stt = (dvbpsi_atsc_stt_t *)stt_section;
	uint16_t mjd;
	uint8_t hour, min, sec;

	UNUSED(mon);

	AM_DEBUG(2, "STT UTC time is %u gpsoff[%u]",
		p_stt->i_system_time + SECS_BETWEEN_1JAN1970_6JAN1980 - p_stt->i_gps_utc_offset,
		p_stt->i_gps_utc_offset);

	/*取UTC时间*/
	pthread_mutex_lock(&time_lock);
	curr_time.tdt_utc_time = p_stt->i_system_time + SECS_BETWEEN_1JAN1970_6JAN1980 - p_stt->i_gps_utc_offset;
	curr_gps_time_offset = p_stt->i_gps_utc_offset;

	/*更新system time，用于时间自动累加*/
	AM_TIME_GetClock(&curr_time.tdt_sys_time);
	pthread_mutex_unlock(&time_lock);
}

static void am_epg_gen_short_event_text(ShortEventLanguage *all_langs, int lang_cnt, 
	char *evt_name, int name_len, char *short_descr, int descr_len)
{
	const uint8_t split = 0x80;
	int i, cur_name_len, cur_descr_len, tmp_len, n;
	char name[EVT_NAME_LEN+1];
	char desc[EVT_TEXT_LEN+1];
	dvbpsi_short_event_dr_t *pse;

#define COPY_BYTES(_d, _dlen, _s, _slen, _curdlen)\
	AM_MACRO_BEGIN\
		int copy_len = ((_curdlen+_slen)>=_dlen) ? (_dlen-_curdlen) : _slen;\
		if (copy_len > 0){\
			memcpy(_d+_curdlen, _s, copy_len);\
			_curdlen += copy_len;\
		}\
	AM_MACRO_END
	
	cur_name_len = 0;
	cur_descr_len = 0;
	for (i=0; i<lang_cnt; i++)
	{
		pse = all_langs[i].descr;

		name[0] = 0;
		desc[0] = 0;
		
		AM_SI_ConvertDVBTextCode((char*)pse->i_event_name, pse->i_event_name_length,\
								name, EVT_NAME_LEN);
		name[EVT_NAME_LEN] = 0;
		AM_SI_ConvertDVBTextCode((char*)pse->i_text, pse->i_text_length,\
						desc, EVT_TEXT_LEN);
		desc[EVT_TEXT_LEN] = 0;

		if (i != 0)
		{
			/* extra split mark */
			COPY_BYTES(evt_name, name_len, &split, 1, cur_name_len);
		}
		/* 3 bytes language code at first */
		COPY_BYTES(evt_name, name_len, all_langs[i].lang, 3, cur_name_len);
		/* following by its name text */
		tmp_len = strlen(name);
		COPY_BYTES(evt_name, name_len, name, tmp_len, cur_name_len);

		if (i != 0)
		{
			/* extra split mark */
			COPY_BYTES(short_descr, descr_len, &split, 1, cur_descr_len);
		}
		/* 3 bytes language code at first */
		COPY_BYTES(short_descr, descr_len, all_langs[i].lang, 3, cur_descr_len);
		/* following by its descr text */
		tmp_len = strlen(desc);
		COPY_BYTES(short_descr, descr_len, desc, tmp_len, cur_descr_len);

	}

	/* set the ending null byte */
	if (cur_name_len >= name_len)
		evt_name[name_len-1] = 0;
	else
		evt_name[cur_name_len] = 0;

	if (cur_descr_len >= descr_len)
		short_descr[descr_len-1] = 0;
	else
		short_descr[cur_descr_len] = 0;
}

static void am_epg_gen_extended_event_text(ExtendedEventLanguage *all_langs, int lang_cnt, 
	char *item_text, int item_len, char *ext_text, int ext_len)
{
	const uint8_t split = 0x80;
	int i, ii, cur_text_len, cur_item_len, tmp_len;
	char item_descr[ITEM_DESCR_LEN+1];
	char item_char[ITEM_CHAR_LEN + 1];
	char desc[EXT_TEXT_LEN+1];
	int item_cnt, text_len;
	uint8_t text_buffer[64*1024];
	ExtendedEventItem items[MAX_ITEM_CNT];
	dvbpsi_extended_event_dr_t **eedecrs;

#define COPY_BYTES(_d, _dlen, _s, _slen, _curdlen)\
	AM_MACRO_BEGIN\
		int copy_len = ((_curdlen+_slen)>=_dlen) ? (_dlen-_curdlen) : _slen;\
		if (copy_len > 0){\
			memcpy(_d+_curdlen, _s, copy_len);\
			_curdlen += copy_len;\
		}\
	AM_MACRO_END
	
	cur_text_len = 0;
	cur_item_len = 0;
	for (i=0; i<lang_cnt; i++)
	{
		/*Start a new language*/
		eedecrs = all_langs[i].descrs;
		memset(items, 0, sizeof(items));
		item_cnt = 0;
		text_len = 0;

		for (ii=0; ii<16; ii++)
		{
			if (eedecrs[ii] != NULL)
			{
				dvbpsi_extended_event_dr_t *pee = eedecrs[ii];
				int j, k;
				
				AM_DEBUG(2, "extended event descr %d, entry count %d", ii, pee->i_entry_count);
				/*Merge items*/
				for (j=0; j<pee->i_entry_count; j++)
				{
					for (k=0; k<item_cnt; k++)
					{
						if (!memcmp(items[k].item_descr, pee->i_item_description[j], pee->i_item_description_length[j]))
							break;
					}

					if (k >= item_cnt)
					{
						/*Add a new item*/
						if (item_cnt >= MAX_ITEM_CNT)
						{
							AM_DEBUG(1, "Too many items");
							continue;
						}

						items[item_cnt].char_len = pee->i_item_length[j];
						items[item_cnt].descr_len = pee->i_item_description_length[j];
						if (pee->i_item[j] != NULL)
							memcpy(items[item_cnt].item_char, pee->i_item[j], pee->i_item_length[j]);
						if (pee->i_item_description[j] != NULL)
						{
							AM_DEBUG(2, "item_cnt %d, %p, %d", item_cnt, pee->i_item_description[j], pee->i_item_description_length[j]);
							memcpy(items[item_cnt].item_descr, pee->i_item_description[j], pee->i_item_description_length[j]);
						}
						item_cnt++;
					}
					else
					{
						int copy_len;

						if ((items[k].char_len + pee->i_item_length[j]) >= (int)sizeof(items[k].item_char))
							copy_len = sizeof(items[k].item_char) - items[k].char_len;
						else
							copy_len = pee->i_item_length[j];
						
						/*Merge the item_char*/
						if (pee->i_item[j] != NULL && copy_len > 0)
						{
							uint8_t *pdata = pee->i_item[j];

							if (pdata[0] < 0x20)
							{
								/*skip the first byte*/
								copy_len--;
								pdata++;
							}
							memcpy(items[k].item_char+items[k].char_len, pdata, copy_len);
							items[k].char_len += copy_len;
						}
					}
					
				}

				/*Merge the text*/
				if (pee->i_text != NULL && pee->i_text_length > 0)
				{
					int copy_len;
					uint8_t *pdata = pee->i_text;

					if ((text_len + pee->i_text_length) >= (int)sizeof(text_buffer))
						copy_len = sizeof(text_buffer)-text_len;
					else
						copy_len = pee->i_text_length;
					
					if (pdata[0] < 0x20 && text_len > 0)
					{
						/*skip the first byte*/
						copy_len--;
						pdata++;
					}
					
					memcpy(text_buffer+text_len, pdata, copy_len);
					text_len += copy_len;
				}
			}
		}
				
				
		/*Merge all the items in following format:
		 * description1:char1
		 * description2:char2
		  ...
		 * descriptionN:charN
		 */

		if (i != 0)
		{
			/* extra split mark */
			COPY_BYTES(item_text, item_len, &split, 1, cur_item_len);
		}
		
		/* 3 bytes language code at first */
		COPY_BYTES(item_text, item_len, all_langs[i].lang, 3, cur_item_len);

		/* description:char */
		for (ii=0; ii<item_cnt; ii++)
		{
			item_descr[0] = 0;
			item_char[0]  = 0;
			
			AM_SI_ConvertDVBTextCode((char*)items[ii].item_descr, items[ii].descr_len, item_descr, ITEM_DESCR_LEN);
			item_descr[ITEM_DESCR_LEN] = 0;

			tmp_len = strlen(item_descr);
			COPY_BYTES(item_text, item_len, item_descr, tmp_len, cur_item_len);

			COPY_BYTES(item_text, item_len, ":", 1, cur_item_len);

			AM_SI_ConvertDVBTextCode((char*)items[ii].item_char, items[ii].char_len, item_char, ITEM_CHAR_LEN);
			item_char[ITEM_CHAR_LEN] = 0;
			
			tmp_len = strlen(item_char);
			COPY_BYTES(item_text, item_len, item_char, tmp_len, cur_item_len);

			COPY_BYTES(item_text, item_len, "\n", 1, cur_item_len);
		}

		/* text */
		desc[0] = 0;

		if (i != 0)
		{
			/* extra split mark */
			COPY_BYTES(ext_text, ext_len, &split, 1, cur_text_len);
		}
		
		/* 3 bytes language code at first */
		COPY_BYTES(ext_text, ext_len, all_langs[i].lang, 3, cur_text_len);
		AM_SI_ConvertDVBTextCode((char*)text_buffer, text_len, desc, EXT_TEXT_LEN);
		desc[EXT_TEXT_LEN] = 0;
		tmp_len = strlen(desc);
		COPY_BYTES(ext_text, ext_len, desc, tmp_len, cur_text_len);
	}

	/* set the ending null byte */
	if (cur_text_len >= ext_len)
		ext_text[ext_len-1] = 0;
	else
		ext_text[cur_text_len] = 0;
	if (cur_item_len >= item_len)
		item_text[item_len-1] = 0;
	else
		item_text[cur_item_len] = 0;
}

static void am_epg_gen_event_text(atsc_multiple_string_t *multi_string, char *multi_text, int len)
{
	const uint8_t split = 0x80;
	int i, cur_len, tmp_len, n;

#define COPY_BYTES(_d, _dlen, _s, _slen, _curdlen)\
	AM_MACRO_BEGIN\
		int copy_len = ((_curdlen+_slen)>=_dlen) ? (_dlen-_curdlen) : _slen;\
		if (copy_len > 0) {\
			memcpy(_d+_curdlen, _s, copy_len);\
			_curdlen += copy_len;\
		}\
	AM_MACRO_END

	cur_len = 0;
	for (i=0; i<multi_string->i_string_count; i++)
	{
		//already utf8 converted.
		char *text = (char*)multi_string->string[i].string;

		if (i != 0)
		{
			/* extra split mark */
			COPY_BYTES(multi_text, len, &split, 1, cur_len);
		}
		/* 3 bytes language code at first */
		COPY_BYTES(multi_text, len, multi_string->string[i].iso_639_code, 3, cur_len);
		/* following by its text */
		tmp_len = strlen(text);
		COPY_BYTES(multi_text, len, text, tmp_len, cur_len);

	}

	/* set the ending null byte */
	if (cur_len >= len)
		multi_text[len-1] = 0;
	else
		multi_text[cur_len] = 0;
}

static void am_epg_eit_get_event_list(AM_EPG_Monitor_t *mon, void *eit_section, int *evt_cnt, AM_EPG_Event_t **pevt_array)
{
	dvbpsi_eit_t *eit = (dvbpsi_eit_t*)eit_section;
	dvbpsi_eit_event_t *event;
	dvbpsi_descriptor_t *descr;
	int start, end, nibble, now;
	int parental_rating = 0;
	uint16_t mjd;
	uint8_t hour, min, sec;
	ExtendedEventLanguage ext_langs[MAX_LANGUAGE_CNT];
	ShortEventLanguage short_langs[MAX_LANGUAGE_CNT];
	int short_lang_cnt, ext_lang_cnt;
	int events_cnt = 0;
	AM_EPG_Event_t *events = NULL, *pevt;

	AM_DEBUG(4, "process EIT section");

	if ((mon->curr_ts_tsid != -1) && (mon->curr_ts_tsid != eit->i_ts_id)) {
		AM_DEBUG(2, "ignore EIT tsid %d != %d, not matched",
			eit->i_ts_id, mon->curr_ts_tsid);
		*evt_cnt = 0;
		*pevt_array = NULL;
		return;
	}

	AM_SI_LIST_BEGIN(eit->p_first_event, event)
		events_cnt++;
	AM_SI_LIST_END()

	*evt_cnt = events_cnt;

	AM_EPG_Event_t *evts = malloc(sizeof(AM_EPG_Event_t)*events_cnt);
	if(!evts) {
		AM_DEBUG(0, "No memory for eit processing, need %d, evt lost:%d",
			sizeof(AM_EPG_Event_t)*events_cnt, events_cnt);
		*evt_cnt = 0;
		*pevt_array = NULL;
		return;
	}

	events_cnt = 0;
	AM_SI_LIST_BEGIN(eit->p_first_event, event)

		pevt = &evts[events_cnt++];

		/*取UTC时间*/
		mjd = (uint16_t)(event->i_start_time >> 24);
		hour = (uint8_t)(event->i_start_time >> 16);
		min = (uint8_t)(event->i_start_time >> 8);
		sec = (uint8_t)event->i_start_time;
		start = AM_EPG_MJD2SEC(mjd) + AM_EPG_BCD2SEC(hour, min, sec);
		/*取持续事件*/
		hour = (uint8_t)(event->i_duration >> 16);
		min = (uint8_t)(event->i_duration >> 8);
		sec = (uint8_t)event->i_duration;
		end = AM_EPG_BCD2SEC(hour, min, sec);

		end += start;

		/*Donot add the expired event*/
		//AM_EPG_GetUTCTime(&now);
		//if (end < now)
		//	continue;

		pevt->name[0] = 0;
		pevt->desc[0] = 0;
		pevt->ext_descr[0] = 0;

		nibble = 0;
		short_lang_cnt = 0;
		ext_lang_cnt = 0;
		memset(short_langs, 0, sizeof(short_langs));
		memset(ext_langs, 0, sizeof(ext_langs));
		
		AM_SI_LIST_BEGIN(event->p_first_descriptor, descr)
			if (descr->i_tag == AM_SI_DESCR_SHORT_EVENT && descr->p_decoded)
			{
				int ishort;
				dvbpsi_short_event_dr_t *pse = (dvbpsi_short_event_dr_t*)descr->p_decoded;

				/* multil-language support */
				if (short_lang_cnt >= MAX_LANGUAGE_CNT)
					continue;
				for (ishort=0; ishort<short_lang_cnt; ishort++)
				{
					if (!memcmp(short_langs[ishort].lang, pse->i_iso_639_code, 3))
						break;
				}
				/* already found, can this case be true? */
				if (ishort < short_lang_cnt)
					continue;
				/* add a new language */
				memcpy(short_langs[short_lang_cnt].lang, pse->i_iso_639_code, 3);
				short_langs[short_lang_cnt++].descr = pse;
			}
			else if (descr->i_tag == AM_SI_DESCR_EXTENDED_EVENT && descr->p_decoded)
			{
				int iext;
				dvbpsi_extended_event_dr_t *pee = (dvbpsi_extended_event_dr_t*)descr->p_decoded;

				/* multil-language support */
				if (ext_lang_cnt >= MAX_LANGUAGE_CNT)
					continue;
				for (iext=0; iext<ext_lang_cnt; iext++)
				{
					if (!memcmp(ext_langs[iext].lang, pee->i_iso_639_code, 3))
						break;
				}
				/* already found, add this descriptor to its language */
				if (iext < ext_lang_cnt)
				{
					if (pee->i_descriptor_number < 16)
						ext_langs[iext].descrs[pee->i_descriptor_number] = pee;
					continue;
				}
				/* add a new language */
				memcpy(ext_langs[ext_lang_cnt].lang, pee->i_iso_639_code, 3);
				ext_langs[ext_lang_cnt++].descrs[pee->i_descriptor_number] = pee;
			}
			else if (descr->i_tag == AM_SI_DESCR_CONTENT && descr->p_decoded)
			{
				dvbpsi_content_dr_t *pcd = (dvbpsi_content_dr_t*)descr->p_decoded;

				nibble = pcd->i_nibble_level;
				//AM_DEBUG(1, "content_nibble_level is 0x%x", nibble);
			}
			else if (descr->i_tag == AM_SI_DESCR_PARENTAL_RATING && descr->p_decoded)
			{
				dvbpsi_parental_rating_dr_t *prd = (dvbpsi_parental_rating_dr_t*)descr->p_decoded;
				dvbpsi_parental_rating_t *pr = prd->p_parental_rating;
				int i;

				for(i=0; i<prd->i_ratings_number; i++){
					if(pr->i_rating){
						if(!parental_rating || parental_rating<pr->i_rating)
							parental_rating = pr->i_rating;
					}
					pr++;
				}
			}
		AM_SI_LIST_END()

		/* generate multi-language text */
		am_epg_gen_short_event_text(short_langs, short_lang_cnt, pevt->name, 
			sizeof(pevt->name), pevt->desc, sizeof(pevt->desc));
		/* extended text */
		am_epg_gen_extended_event_text(ext_langs, ext_lang_cnt, pevt->ext_item, 
			sizeof(pevt->ext_item), pevt->ext_descr, sizeof(pevt->ext_descr));

		pevt->src = mon->src;
		pevt->srv_id = eit->i_service_id;
		pevt->ts_id = eit->i_ts_id;
		pevt->net_id = eit->i_network_id;
		pevt->evt_id = event->i_event_id;
		pevt->start = start;
		pevt->end = end;
		pevt->nibble = nibble;
		pevt->parental_rating = parental_rating;
		pevt->sub_flag = 0;
		pevt->sub_status = 0;
		pevt->source_id = -1;
		pevt->rrt_ratings[0] = 0;
		AM_DEBUG(4, "evt: sid/net/ts[%d/%d/%d] evt[%d] start[%d] end[%d]", pevt->srv_id, pevt->net_id, pevt->ts_id, pevt->evt_id, pevt->start, pevt->end);
	AM_SI_LIST_END()

	*pevt_array = evts;
}

static void am_epg_psip_eit_get_event_list(AM_EPG_Monitor_t *mon, void *eit_section, int *evt_cnt, AM_EPG_Event_t **pevt_array)
{
	dvbpsi_atsc_eit_t *eit = (dvbpsi_atsc_eit_t*)eit_section;
	dvbpsi_atsc_eit_event_t *event;
	dvbpsi_descriptor_t *descr;
	int start, end, now;
	int events_cnt = 0;
	AM_EPG_Event_t *events = NULL, *pevt;
	AM_EPG_TableCtl_t *tctrl = NULL;

	AM_SI_LIST_BEGIN(eit->p_first_event, event)
		events_cnt++;
	AM_SI_LIST_END()

	*evt_cnt = events_cnt;

	AM_EPG_Event_t *evts = malloc(sizeof(AM_EPG_Event_t)*events_cnt);
	if (!evts) {
		AM_DEBUG(0, "No memory for eit processing, need %d, evt lost:%d",
			sizeof(AM_EPG_Event_t)*events_cnt, events_cnt);
		*evt_cnt = 0;
		*pevt_array = NULL;
		return;
	}

	AM_DEBUG(4, "process psip EIT section sid:0x%x evt_cnt:%d", eit->i_source_id, events_cnt);

	events_cnt = 0;
	AM_SI_LIST_BEGIN(eit->p_first_event, event)
		char values[1024];
		int values_size = sizeof(values);

		pevt = &evts[events_cnt++];

		start = event->i_start_time + SECS_BETWEEN_1JAN1970_6JAN1980 - curr_gps_time_offset;
		end = start + event->i_length_seconds;

		/*Donot add the expired event*/
		//AM_EPG_GetUTCTime(&now);
		//if (end < now)
		//	continue;

		pevt->name[0] = 0;
		pevt->desc[0] = 0;
		pevt->ext_descr[0] = 0;

		/* generate multi-language text */
		{
			atsc_multiple_string_t ms;

			memset(&ms, 0, sizeof(ms));
			atsc_decode_multiple_string_structure(event->i_title, &ms);
			am_epg_gen_event_text(&ms, pevt->name, sizeof(pevt->name));
			//AM_DEBUG(1,"evt: name:%s", pevt->name);
		}

		pevt->rrt_ratings[0] = 0;
		sprintf(pevt->rrt_ratings, "{");

		//get ratings
		values[0] = 0;
		AM_SI_GetRatingStringFromDescriptors(event->p_first_descriptor, values, values_size);
		if (strlen(values))
			snprintf(pevt->rrt_ratings, sizeof(pevt->rrt_ratings), "%sDratings:%s", pevt->rrt_ratings, values);

		//get captions
		values[0] = 0;
		AM_SI_GetATSCCaptionStringFromDescriptors(event->p_first_descriptor, values, values_size);
		if (strlen(values)) {
			if (pevt->rrt_ratings[strlen(pevt->rrt_ratings)-1] != '{')
				snprintf(pevt->rrt_ratings, sizeof(pevt->rrt_ratings), "%s,", pevt->rrt_ratings);
			snprintf(pevt->rrt_ratings, sizeof(pevt->rrt_ratings), "%scaptions:%s", pevt->rrt_ratings, values);
		}

		snprintf(pevt->rrt_ratings, sizeof(pevt->rrt_ratings), "%s}", pevt->rrt_ratings);

		pevt->src = mon->src;
		pevt->srv_id = eit->i_source_id;
		pevt->evt_id = event->i_event_id;
		pevt->start = start;
		pevt->end = end;
		pevt->source_id = eit->i_source_id;
		pevt->ts_id = -1;
		pevt->net_id = -1;
		pevt->nibble = 0;
		pevt->parental_rating = 0;
		{
			AM_EPG_TableCtl_t *table_ctrl = am_epg_get_section_ctrl_by_fid(mon, mon->current_fid);
			int k = -1;
			if (table_ctrl->tid == AM_SI_TID_PSIP_EIT)
				sscanf(table_ctrl->tname, "ATSC EIT%d", &k);
			else if (table_ctrl->tid == AM_SI_TID_PSIP_ETT)
				sscanf(table_ctrl->tname, "ATSC ETT%d", &k);
			pevt->sub_flag = eit->i_version;//reuse for psip eit (ver)
			pevt->sub_status = k;//reuse for psip eit (k)
		}
		AM_DEBUG(4, "evt: sid[%d] evt[%d] start[%d] end[%d] r(%s) v[k:%d-v:%d]",
			pevt->srv_id, pevt->evt_id, pevt->start, pevt->end, pevt->rrt_ratings, pevt->sub_status, pevt->sub_flag);
	AM_SI_LIST_END()

	*pevt_array = evts;
}

static void am_epg_psip_ett_get_event_list(AM_EPG_Monitor_t *mon, void *ett_section, int *evt_cnt, AM_EPG_Event_t **pevt_array)
{
	dvbpsi_atsc_ett_t *ett = (dvbpsi_atsc_ett_t*)ett_section;
	dvbpsi_descriptor_t *descr;
	AM_EPG_Event_t *events = NULL, *pevt;

	AM_DEBUG(4, "process psip ETT section");

	*evt_cnt = 1;

	AM_EPG_Event_t *evts = malloc(sizeof(AM_EPG_Event_t));
	if (!evts) {
		AM_DEBUG(0, "No memory for ett processing, need %d, evt lost:%d",
			sizeof(AM_EPG_Event_t), 1);
		*evt_cnt = 0;
		*pevt_array = NULL;
		return;
	}

	pevt = evts;

	pevt->name[0] = 0;
	pevt->desc[0] = 0;
	pevt->ext_descr[0] = 0;
	pevt->rrt_ratings[0] = 0;

	/* generate multi-language text */
	{
		atsc_multiple_string_t ms;

		atsc_decode_multiple_string_structure(ett->p_etm_data, &ms);
		am_epg_gen_event_text(&ms, pevt->ext_descr, sizeof(pevt->ext_descr));
		//AM_DEBUG(1, "evt: ett:%s", pevt->ext_descr);
	}

	pevt->src = mon->src;
	pevt->srv_id = (ett->i_etm_id >> 16) & 0xffff;
	pevt->evt_id = (ett->i_etm_id & 0xffff) >> 2;
	pevt->start = 0;
	pevt->end = 0;
	pevt->source_id = pevt->srv_id;
	pevt->ts_id = -1;
	pevt->net_id = -1;
	pevt->nibble = 0;
	pevt->parental_rating = 0;
	{
		AM_EPG_TableCtl_t *table_ctrl = am_epg_get_section_ctrl_by_fid(mon, mon->current_fid);
		int k = -1;
		if (table_ctrl->tid == AM_SI_TID_PSIP_ETT)
			sscanf(table_ctrl->tname, "ATSC ETT%d", &k);
		else if (table_ctrl->tid == AM_SI_TID_PSIP_ETT)
			sscanf(table_ctrl->tname, "ATSC ETT%d", &k);
		pevt->sub_flag = ett->i_version;//reuse for psip eit (ver)
		pevt->sub_status = k;//reuse for psip eit (k)
	}
	AM_DEBUG(4, "ett: sid[%d] evt[%d] start[%d] end[%d] v[k:%d-v:%d]",
		pevt->source_id, pevt->evt_id, pevt->start, pevt->end, pevt->sub_status, pevt->sub_flag);

	*pevt_array = evts;
}

static void am_epg_eit_free_event_list(int event_count, AM_EPG_Event_t *pevents)
{
	UNUSED(event_count);

	if(pevents)
		free(pevents);
}

static void am_epg_proc_eit_section_def(AM_EPG_Monitor_t *mon, void *eit_section)
{
	dvbpsi_eit_t *eit = (dvbpsi_eit_t*)eit_section;
	dvbpsi_eit_event_t *event;
	dvbpsi_descriptor_t *descr;
	char name[COMPOSED_NAME_LEN+1];
	char desc[COMPOSED_DESCR_LEN+1];
	char ext_item[COMPOSED_EXT_ITEM_LEN + 1];
	char ext_descr[COMPOSED_EXT_DESCR_LEN + 1];
	char sql[256];
	int row = 1;
	int srv_dbid, net_dbid, ts_dbid, evt_dbid;
	int start, end, nibble, now;
	int parental_rating = 0;
	uint16_t mjd;
	uint8_t hour, min, sec;
	ExtendedEventLanguage ext_langs[MAX_LANGUAGE_CNT];
	ShortEventLanguage short_langs[MAX_LANGUAGE_CNT];
	int short_lang_cnt, ext_lang_cnt;
	sqlite3 *hdb;
	sqlite3_stmt *stmt;
	const char *insert_evt_sql = "insert into evt_table(src,db_net_id, db_ts_id, \
		db_srv_id, event_id, name, start, end, descr, items, ext_descr,nibble_level,\
		sub_flag,sub_status,parental_rating,source_id,rrt_ratings) \
		values(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)";
	const char *insert_evt_sql_name = "insert epg events";
	int evt_cnt;
	AM_EPG_Event_t *pevt_array, *pevt;
	int i;

	AM_DB_HANDLE_PREPARE(hdb);
	if (AM_DB_GetSTMT(&stmt, insert_evt_sql_name, insert_evt_sql, 0) != AM_SUCCESS)
	{
		AM_DEBUG(1, "EPG: prepare insert events stmt failed");
		return;
	}
	if (mon->curr_ts < 0)
	{
		AM_DEBUG(1, "EPG: current ts not set, skip this section");
		return;
	}

	am_epg_eit_get_event_list(mon, eit_section, &evt_cnt, &pevt_array);

	if(!evt_cnt)
		return;

	/*查询service*/
	snprintf(sql, sizeof(sql), "select db_net_id,db_ts_id,db_id from srv_table where db_ts_id=%d \
								and service_id=%d limit 1", mon->curr_ts, pevt_array[0].srv_id);
	if (AM_DB_Select(hdb, sql, &row, "%d,%d,%d", &net_dbid, &ts_dbid, &srv_dbid) != AM_SUCCESS || row == 0)
	{
		/*No such service,but we need free event*/
		AM_DEBUG(1, "EPG: AM_DB_Select error free this event");
		am_epg_eit_free_event_list(evt_cnt, pevt_array);
		return;
	}

	for(i=0; i<evt_cnt; i++) {
		pevt = &pevt_array[i];

		row = 1;
		/*查找该事件是否已经被添加*/
		snprintf(sql, sizeof(sql), "select db_id from evt_table where db_srv_id=%d \
									and start=%d", srv_dbid, pevt->start);
		if (AM_DB_Select(hdb, sql, &row, "%d", &evt_dbid) == AM_SUCCESS && row > 0)
			continue;

		/*添加新事件到evt_table*/
		sqlite3_bind_int(stmt, 1, pevt->src);
		sqlite3_bind_int(stmt, 2, net_dbid);
		sqlite3_bind_int(stmt, 3, ts_dbid);
		sqlite3_bind_int(stmt, 4, srv_dbid);
		sqlite3_bind_int(stmt, 5, pevt->evt_id);
		sqlite3_bind_text(stmt, 6, pevt->name, -1, SQLITE_STATIC);
		sqlite3_bind_int(stmt, 7, pevt->start);
		sqlite3_bind_int(stmt, 8, pevt->end);
		sqlite3_bind_text(stmt, 9, pevt->desc, -1, SQLITE_STATIC);
		sqlite3_bind_text(stmt, 10, pevt->ext_item, -1, SQLITE_STATIC);
		sqlite3_bind_text(stmt, 11, pevt->ext_descr, -1, SQLITE_STATIC);
		sqlite3_bind_int(stmt, 12, pevt->nibble);
		sqlite3_bind_int(stmt, 13, pevt->sub_flag);
		sqlite3_bind_int(stmt, 14, pevt->sub_status);
		sqlite3_bind_int(stmt, 15, pevt->parental_rating);
		sqlite3_bind_int(stmt, 16, pevt->source_id);
		sqlite3_bind_text(stmt, 17, pevt->rrt_ratings, -1, SQLITE_STATIC);
		STEP_STMT(stmt, insert_evt_sql_name, insert_evt_sql);

		/*设置更新通知标志*/
		if (! mon->eit_has_data)
		{
			if (mon->mon_service == srv_dbid)
			{
				AM_DEBUG(1, "Set EPG service(%d) update flag to 1", srv_dbid);
				mon->eit_has_data = AM_TRUE;
			}
		}
	}

	am_epg_eit_free_event_list(evt_cnt, pevt_array);
}

static void am_epg_proc_rrt_section_def(AM_EPG_Monitor_t *mon, void *rrt_section)
{
	rrt_section_info_t *rrt = (rrt_section_info_t *)rrt_section;
	rrt_dimensions_info_t *dimension;
	rrt_rating_value_t *value;
	int i, j = 0;
	int row, version;
	char sql[256];
	sqlite3 *hdb;
	sqlite3_stmt *stmt;
	const char *new_dimension_sql =
        "insert into dimension_table(rating_region,rating_region_name,name,graduated_scale,values_defined,index_j,version,\
        abbrev0,text0,locked0,abbrev1,text1,locked1,abbrev2,text2,locked2,abbrev3,text3,locked3,abbrev4,text4,locked4,\
        abbrev5,text5,locked5,abbrev6,text6,locked6,abbrev7,text7,locked7,abbrev8,text8,locked8,abbrev9,text9,locked9,\
        abbrev10,text10,locked10,abbrev11,text11,locked11,abbrev12,text12,locked12,abbrev13,text13,locked13,\
        abbrev14,text14,locked14,abbrev15,text15,locked15) values(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,\
        ?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)";
	const char *new_dimension_sql_name = "new dimension";

	UNUSED(mon);

	AM_DB_HANDLE_PREPARE(hdb);
	if (AM_DB_GetSTMT(&stmt, new_dimension_sql_name, new_dimension_sql, 0) != AM_SUCCESS)
	{
		AM_DEBUG(1, "EPG: prepare new dimension stmt failed");
		return;
	}
	
	AM_DEBUG(1, "region %d, name(lang0 '%c%c%c') '%s'", rrt->rating_region, 
		rrt->rating_region_name.string[0].iso_639_code[0],
		rrt->rating_region_name.string[0].iso_639_code[1],
		rrt->rating_region_name.string[0].iso_639_code[2],
		rrt->rating_region_name.string[0].string);
		
	if (rrt->rating_region < 0x5)
	{
		AM_DEBUG(1, "Got RRT%d, not downloadable RRT, not used", rrt->rating_region);
		return;
	}
	/*check the RRT version*/
	snprintf(sql, sizeof(sql), "select version from dimension_table where rating_region=%d", rrt->rating_region);
	row = 1;
	if (AM_DB_Select(hdb, sql, &row, "%d", &version) == AM_SUCCESS && row > 0)
	{
		if ((version < rrt->version_number && version != 31) || (version == 31 && rrt->version_number == 0))
		{
			AM_DEBUG(1, "RRT for region %d version increased, %d->%d", rrt->rating_region, version, rrt->version_number);
			/* Delete the previous data */
			snprintf(sql, sizeof(sql), "delete from dimension_table where rating_region=%d", rrt->rating_region);
			sqlite3_exec(hdb, sql, NULL, NULL, NULL);
		}
		else
		{
			AM_DEBUG(1, "RRT for region %d version not changed, version(old/new) = %d/%d", 
				rrt->rating_region, version, rrt->version_number);
			return;
		}
	}
		
	AM_SI_LIST_BEGIN(rrt->dimensions_info, dimension)
		AM_DEBUG(1, "dimension name(lang0 '%c%c%c') '%s'",
			dimension->dimensions_name.string[0].iso_639_code[0],
			dimension->dimensions_name.string[0].iso_639_code[1],
			dimension->dimensions_name.string[0].iso_639_code[2],
			dimension->dimensions_name.string[0].string);
		
		/* Add this new dimension */
		sqlite3_bind_int(stmt, 1, rrt->rating_region);
		sqlite3_bind_text(stmt, 2, (const char*)rrt->rating_region_name.string[0].string, -1, SQLITE_STATIC);
		sqlite3_bind_text(stmt, 3, (const char*)dimension->dimensions_name.string[0].string, -1, SQLITE_STATIC);
		sqlite3_bind_int(stmt, 4, dimension->graduated_scale);
		sqlite3_bind_int(stmt, 5, dimension->values_defined);
		sqlite3_bind_int(stmt, 6, j);
		sqlite3_bind_int(stmt, 7, rrt->version_number);
		for (i=0; i<16; i++)
		{
			AM_DEBUG(1, "value%d (lang0 '%c%c%c') abbrev '%s', text '%s'", i,
				dimension->rating_value[i].abbrev_rating_value_text.string[0].iso_639_code[0],
				dimension->rating_value[i].abbrev_rating_value_text.string[0].iso_639_code[1],
				dimension->rating_value[i].abbrev_rating_value_text.string[0].iso_639_code[2],
				dimension->rating_value[i].abbrev_rating_value_text.string[0].string,
				dimension->rating_value[i].rating_value_text.string[0].string);
			sqlite3_bind_text(stmt, 8+3*i, 
				(i<dimension->values_defined)?(const char*)dimension->rating_value[i].abbrev_rating_value_text.string[0].string:"", 
				-1, SQLITE_STATIC);
			sqlite3_bind_text(stmt, 8+3*i+1, 
				(i<dimension->values_defined)?(const char*)dimension->rating_value[i].rating_value_text.string[0].string:"", 
				-1, SQLITE_STATIC);	
			sqlite3_bind_int(stmt, 8+3*i+2, 0);
		}
		STEP_STMT(stmt, new_dimension_sql_name, new_dimension_sql);
		j++;
	AM_SI_LIST_END()

}

static void am_epg_proc_psip_eit_section_def(AM_EPG_Monitor_t *mon, void *eit_section)
{
	dvbpsi_atsc_eit_t *eit = (dvbpsi_atsc_eit_t*)eit_section;
	dvbpsi_atsc_eit_event_t *event;
	char sql[256];
	char values[1024];
	int srv_dbid, row = 1, starttime, endtime, evt_dbid;
	AM_Bool_t need_update;
	dvbpsi_descriptor_t *descr;
	sqlite3 *hdb;
	sqlite3_stmt *stmt, *update_startend_stmt;
	const char *insert_evt_sql = "insert into evt_table(src,db_net_id, db_ts_id, \
		db_srv_id, event_id, name, start, end, descr, items, ext_descr,nibble_level,\
		sub_flag,sub_status,parental_rating,source_id,rrt_ratings) \
		values(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)";
	const char *insert_evt_sql_name = "insert epg events";
	const char *update_startend_sql = "update evt_table set start=?, end=? where db_id=?";
	const char *update_startend_sql_name = "update startend";
	uint8_t *name;
	int i;
	#define NAMELENGTH (2048+3)*1
	
	if (eit == NULL)
		return;
	name = (uint8_t *)malloc(NAMELENGTH);
    
	if (! name)
	{
		AM_DEBUG(1, "no enough memory");
		return ;
	}
	memset(name,0,NAMELENGTH);
    
	AM_DB_HANDLE_PREPARE(hdb);
	if (AM_DB_GetSTMT(&stmt, insert_evt_sql_name, insert_evt_sql, 0) != AM_SUCCESS)
	{
		AM_DEBUG(1, "EPG: prepare insert events stmt failed");
		free(name);
		name = NULL;
		return;
	}
	if (AM_DB_GetSTMT(&update_startend_stmt, update_startend_sql_name, update_startend_sql, 0) != AM_SUCCESS)
	{
		AM_DEBUG(1, "EPG: prepare insert events stmt failed");
		free(name);
		name = NULL;
		return;
	}
	
	/*查询source_id*/
	snprintf(sql, sizeof(sql), "select db_id from srv_table where db_ts_id=%d and source_id=%d limit 1", mon->curr_ts, eit->i_source_id);
	if (AM_DB_Select(hdb, sql, &row, "%d", &srv_dbid) != AM_SUCCESS || row == 0)
	{
		/*No such source*/
		AM_DEBUG(1, "No such source %d", eit->i_source_id);
		free(name);
		name = NULL;
		return;
	}
	AM_SI_LIST_BEGIN(eit->p_first_event, event)
		int start_time_utc_without_gps_offset = event->i_start_time + SECS_BETWEEN_1JAN1970_6JAN1980;
		row = 1;
		/*查找是否有span EIT time interval的相同event_id的事件*/
		snprintf(sql, sizeof(sql), "select start,end,db_id from evt_table where db_srv_id=%d and source_id=%d \
									and event_id=%d", srv_dbid, eit->i_source_id, event->i_event_id);
		if (AM_DB_Select(hdb, sql, &row, "%d,%d,%d", &starttime, &endtime, &evt_dbid) == AM_SUCCESS && row > 0)
		{
			need_update = AM_FALSE;
			/*合并相同event_id的事件*/
			if (starttime > start_time_utc_without_gps_offset)
			{
				starttime = start_time_utc_without_gps_offset;
				need_update = AM_TRUE;
			}
			if ((int)(start_time_utc_without_gps_offset + event->i_length_seconds) > endtime)
			{
				endtime = start_time_utc_without_gps_offset + event->i_length_seconds;
				need_update = AM_TRUE;
			}
			if (need_update)
			{
				sqlite3_bind_int(update_startend_stmt, 1, starttime);
				sqlite3_bind_int(update_startend_stmt, 2, endtime);
				sqlite3_bind_int(update_startend_stmt, 3, evt_dbid);
				STEP_STMT(update_startend_stmt, update_startend_sql_name, update_startend_sql);
			}
			continue;
		}
		
		values[0] = 0;
		/*取级别控制信息*/
		AM_SI_LIST_BEGIN(event->p_first_descriptor, descr)
			if (descr->p_decoded && descr->i_tag == AM_SI_DESCR_CONTENT_ADVISORY)
			{
				int i, j, dmn_dbid; 
				dvbpsi_atsc_content_advisory_dr_t *pcad = (dvbpsi_atsc_content_advisory_dr_t*)descr->p_decoded;
				
				for (i=0; i<pcad->i_rating_region_count; i++)
				{
					for (j=0; j<pcad->rating_regions[i].i_rated_dimensions; j++)
					{						
						if (values[0] == 0)
							snprintf(values, sizeof(values), "%d %d %d",
								pcad->rating_regions[i].i_rating_region,
								pcad->rating_regions[i].dimensions[j].i_rating_dimension_j,
								pcad->rating_regions[i].dimensions[j].i_rating_value);
						else
							snprintf(values, sizeof(values), "%s,%d %d %d", values,
								pcad->rating_regions[i].i_rating_region,
								pcad->rating_regions[i].dimensions[j].i_rating_dimension_j,
								pcad->rating_regions[i].dimensions[j].i_rating_value);
					}
				}
			}
		AM_SI_LIST_END()
		
		/*添加新事件到evt_table*/
		sqlite3_bind_int(stmt, 1, mon->src);
		sqlite3_bind_int(stmt, 2, -1);
		sqlite3_bind_int(stmt, 3, -1);
		sqlite3_bind_int(stmt, 4, srv_dbid);
		sqlite3_bind_int(stmt, 5, event->i_event_id);
		{
			atsc_multiple_string_t ms;

			memset(&ms, 0, sizeof(ms));
			atsc_decode_multiple_string_structure(event->i_title, &ms);
			/*NOTE：title没有string时，event->title.string[0].string是空字符串*/
			//sqlite3_bind_text(stmt, 6, (const char*)event->title.string[0].string, -1, SQLITE_STATIC);
			//for(i = 0;i < event->title.i_string_count;i++)//maybe in future
			{
				memcpy(name, ms.string[0].iso_639_code,3);
				memcpy(name+3, ms.string[0].string,2048);
			}
			sqlite3_bind_text(stmt, 6, (const char*)name, -1, SQLITE_STATIC);
			sqlite3_bind_int(stmt, 7, start_time_utc_without_gps_offset);
			sqlite3_bind_int(stmt, 8, start_time_utc_without_gps_offset+event->i_length_seconds);
			{
				int ii;

				for (ii=0; ii<ms.i_string_count; ii++)
				{
					AM_DEBUG(3, "lang:'%c%c%c': '%s'",
						ms.string[ii].iso_639_code[0],
						ms.string[ii].iso_639_code[1],
						ms.string[ii].iso_639_code[2],
						ms.string[ii].string);
				}
			}
		}
		AM_DEBUG(3, "event starttime %u, duration %d, source_id %d", start_time_utc_without_gps_offset, event->i_length_seconds, eit->i_source_id);
		sqlite3_bind_text(stmt, 9, "", -1, SQLITE_STATIC);
		sqlite3_bind_text(stmt, 10, "", -1, SQLITE_STATIC);
		sqlite3_bind_text(stmt, 11, "", -1, SQLITE_STATIC);
		sqlite3_bind_int(stmt, 12, 0);
		sqlite3_bind_int(stmt, 13, 0);
		sqlite3_bind_int(stmt, 14, 0);
		sqlite3_bind_int(stmt, 15, 0);
		sqlite3_bind_int(stmt, 16, eit->i_source_id);
		sqlite3_bind_text(stmt, 17, (const char*)values, -1, SQLITE_STATIC);
		STEP_STMT(stmt, insert_evt_sql_name, insert_evt_sql);
	AM_SI_LIST_END()

	free(name);
	name = NULL;
}

static void am_epg_proc_psip_ett_section_def(AM_EPG_Monitor_t *mon, void *ett_section)
{
	dvbpsi_atsc_ett_t *ett = (dvbpsi_atsc_ett_t*)ett_section;
	char sql[256];	 
	int row = 1,evt_dbid; 
	sqlite3 *hdb;
	sqlite3_stmt *update_extdescr_stmt;
	
	const char *update_extdescr_sql = "update evt_table set ext_descr=? where db_id=?";
	const char *update_extdescr_sql_name = "update extdescr";

	uint8_t *text;    
	#define TEXTLENGTH (2048+3)*1

	UNUSED(mon);

	if (ett == NULL)
		return;

	text = (uint8_t *)malloc(TEXTLENGTH);
	if (! text)
	{
		AM_DEBUG(1, "no enough memory");
		return ;
	}
	memset(text,0,TEXTLENGTH);
        
	AM_DB_HANDLE_PREPARE(hdb);
	
	if (AM_DB_GetSTMT(&update_extdescr_stmt, update_extdescr_sql_name, update_extdescr_sql, 0) != AM_SUCCESS)
	{
		AM_DEBUG(1, "EPG: prepare insert events stmt failed");
		return;
	}	
	{
		atsc_multiple_string_t ms;

		memset(&ms, 0, sizeof(ms));
		atsc_decode_multiple_string_structure(ett->p_etm_data, &ms);

		snprintf(sql, sizeof(sql), "select db_id from evt_table where source_id=%d and event_id=%d limit 1",
							(ett->i_etm_id >> 16) & 0xffff, ett->i_etm_id & 0xffff);
		if (AM_DB_Select(hdb, sql, &row, "%d", &evt_dbid) == AM_SUCCESS && row > 0)
		{
			memcpy(text,ms.string[0].iso_639_code,3);
			memcpy(text+3,ms.string[0].string,2048);
			sqlite3_bind_text(update_extdescr_stmt, 1, (const char*)text, -1, SQLITE_STATIC);
			sqlite3_bind_int(update_extdescr_stmt, 2, evt_dbid);
			STEP_STMT(update_extdescr_stmt, update_extdescr_sql_name, update_extdescr_sql);
		}
		else
			AM_DEBUG(1,"%s no this event",__FUNCTION__);
	}
}

static void am_epg_proc_vct_section_def(AM_EPG_Monitor_t *mon, void *vct_section)
{
	dvbpsi_atsc_vct_t *p_vct = (dvbpsi_atsc_vct_t *)vct_section;
	dvbpsi_atsc_vct_channel_t *vcinfo;
	AM_SI_AudioInfo_t aud_info;
	int vid, vfmt, db_ts_id, ts_id, row;
	int major, minor, srv_type, db_srv_id;
	char str_apids[256];
	char str_afmts[256];
	char str_alangs[256];
	char sql[1024];
	sqlite3 *hdb;

	AM_DB_HANDLE_PREPARE(hdb);
	
	/* check if ts_id needs update */
	ts_id = -1;
	db_ts_id = am_epg_get_current_db_ts_id(mon);
	if (db_ts_id >= 0)
	{
		row = 1;
		snprintf(sql, sizeof(sql), "select ts_id from ts_table where db_id=%d", db_ts_id);
		AM_DB_Select(hdb, sql, &row, "%d", &ts_id);
	}
	else
	{
		AM_DEBUG(1, "VCT proc: cannot get current ts!");
		return;
	}
	
	if (p_vct->i_extension != ts_id)
	{
		AM_DEBUG(1, "TS id not match, current(%d), but got(%d)", ts_id, p_vct->i_extension);
		SIGNAL_EVENT(AM_EPG_EVT_UPDATE_TS, (void*)(long)db_ts_id);
		return;
	}
	
	AM_SI_LIST_BEGIN(p_vct->p_first_channel, vcinfo)
		if (vcinfo->i_program_number == 0  || vcinfo->i_program_number == 0xffff)
			continue;
	
		if (vcinfo->i_channel_tsid == p_vct->i_extension)
		{	
			snprintf(sql, sizeof(sql), "select db_id, major_chan_num, minor_chan_num, service_type \
				from srv_table where db_ts_id=%d and service_id=%d", db_ts_id, vcinfo->i_program_number);
			if (AM_DB_Select(hdb, sql, &row, "%d,%d,%d,%d", &db_srv_id, &major, 
				&minor, &srv_type) == AM_SUCCESS && row > 0)
			{
				/* AV changed ? */
				vid = 0x1fff;
				vfmt = -1;
				memset(&aud_info, 0, sizeof(aud_info));
				if (vcinfo->p_first_descriptor != NULL)
					AM_SI_ExtractAVFromVC(vcinfo, &vid, &vfmt, &aud_info);
                AM_DEBUG(1, "%s the vid is:0x%x, vfmt is:%d-----------",__FUNCTION__, vid, vfmt);

				if ((vcinfo->p_first_descriptor != NULL) && (am_epg_check_program_av(hdb, db_srv_id, vid, vfmt, &aud_info)))
				{
					if (db_srv_id == mon->mon_service)
					{
						/*触发通知事件*/
						SIGNAL_EVENT(AM_EPG_EVT_UPDATE_PROGRAM_AV, (void*)(long)mon->mon_service);
					}
				}
				/* channel number changed ? */
				if (major != vcinfo->i_major_number ||
					minor != vcinfo->i_minor_number)
				{
					AM_DEBUG(1, "11 Program(%d) vct update(major-minor): %d-%d -> %d-%d",
						vcinfo->i_program_number, major, minor,
						vcinfo->i_major_number, vcinfo->i_minor_number);
					if (vcinfo->i_major_number < 2 || vcinfo->i_major_number  > 99)
						vcinfo->i_major_number = major;
					if (vcinfo->i_minor_number == 0)
						vcinfo->i_minor_number = minor;
					AM_DEBUG(1, "22 Program(%d) vct update(major-minor): %d-%d -> %d-%d",
						vcinfo->i_program_number, major, minor,
						vcinfo->i_major_number, vcinfo->i_minor_number);
					/* update to database */
					snprintf(sql, sizeof(sql), "update srv_table set major_chan_num=%d,minor_chan_num=%d where db_id=%d", 
						vcinfo->i_major_number,
						vcinfo->i_minor_number,
						db_srv_id);
					sqlite3_exec(hdb, sql, NULL, NULL, NULL);
				}
			}
			else
			{
				AM_DEBUG(1, "Cannot get program %d, need a re-scan for current ts.", vcinfo->i_program_number);
				SIGNAL_EVENT(AM_EPG_EVT_UPDATE_TS, (void*)(long)db_ts_id);
				return;
			}
		}
		else
		{
			AM_DEBUG(1, "Program(%d) of TS(%d) in VCT(%d) found", 
					vcinfo->i_program_number, vcinfo->i_channel_tsid,
					p_vct->i_extension);
			continue;
		}
	AM_SI_LIST_END()
}

/**\brief 根据过滤器号取得相应控制数据*/
static AM_EPG_TableCtl_t *am_epg_get_section_ctrl_by_fid(AM_EPG_Monitor_t *mon, int fid)
{
	AM_EPG_TableCtl_t *scl = NULL;

	if (mon->patctl.fid == fid)
		scl = &mon->patctl;
	else if (mon->catctl.fid == fid)
		scl = &mon->catctl;
	else if (mon->pmtctl.fid == fid)
		scl = &mon->pmtctl;
	else if (mon->sdtctl.fid == fid)
		scl = &mon->sdtctl;
	else if (mon->nitctl.fid == fid)
		scl = &mon->nitctl;
	else if (mon->totctl.fid == fid)
		scl = &mon->totctl;
	else if (mon->eit4ectl.fid == fid)
		scl = &mon->eit4ectl;
	else if (mon->eit4fctl.fid == fid)
		scl = &mon->eit4fctl;
	else if (mon->eit50ctl.fid == fid)
		scl = &mon->eit50ctl;
	else if (mon->eit51ctl.fid == fid)
		scl = &mon->eit51ctl;
	else if (mon->eit60ctl.fid == fid)
		scl = &mon->eit60ctl;
	else if (mon->eit61ctl.fid == fid)
		scl = &mon->eit61ctl;
	else if (mon->sttctl.fid == fid)
		scl = &mon->sttctl;
	else if (mon->mgtctl.fid == fid)
		scl = &mon->mgtctl;
	else if (mon->rrtctl.fid == fid)
		scl = &mon->rrtctl;
	else if (mon->vctctl.fid == fid)
		scl = &mon->vctctl;
	else if (mon->ceactl.fid == fid)
		scl = &mon->ceactl;
	else
	{
		int i;
		
		for (i=0; i<mon->psip_eit_count; i++)
		{
			if (mon->psip_eitctl[i].fid == fid)
			{
				scl = &mon->psip_eitctl[i];
				return scl;
			}
		}

		for(i=0; i<mon->psip_ett_count; i++)
		{
			if (mon->psip_ettctl[i].fid == fid)
			{
				scl = &mon->psip_ettctl[i];
				return scl;
			}
		}
	}

	
	return scl;
}

/**\brief 数据处理函数*/
static void am_epg_section_handler(int dev_no, int fid, const uint8_t *data, int len, void *user_data)
{
	AM_EPG_Monitor_t *mon = (AM_EPG_Monitor_t*)user_data;
	AM_EPG_TableCtl_t * sec_ctrl;
	AM_SI_SectionHeader_t header;

	UNUSED(dev_no);

	if (mon == NULL)
	{
		AM_DEBUG(1, "EPG: Invalid param user_data in dmx callback");
		return;
	}
	if (!data)
		return;

	pthread_mutex_lock(&mon->lock);
	/*获取接收控制数据*/
	sec_ctrl = am_epg_get_section_ctrl_by_fid(mon, fid);
	if (sec_ctrl)
	{
		if (sec_ctrl != &mon->totctl)
		{
			/* for Non-TDT/TOT sections, the section_syntax_indicator bit must be 1 */
			if ((data[1]&0x80) == 0)
			{
				AM_DEBUG(1, "EPG: section_syntax_indicator is 0, skip this section");
				goto handler_done;
			}
			
			if (AM_SI_GetSectionHeader(mon->hsi, (uint8_t*)data, len, &header) != AM_SUCCESS)
			{
				AM_DEBUG(1, "EPG: section header error");
				goto handler_done;
			}

			/*该section是否已经接收过*/
			if (am_epg_tablectl_test_recved(sec_ctrl, &header))
			{
				AM_DEBUG(5,"%s section %d repeat! last_sec %d", sec_ctrl->tname, header.sec_num, header.last_sec_num);
			
				/*当有多个子表时，判断收齐的条件为 收到重复section + 
				 *所有子表收齐 + 重复section间隔时间大于某个值
				 */
				if (sec_ctrl->subs > 1)
				{
					int now;
				
					AM_TIME_GetClock(&now);
					if (am_epg_tablectl_test_complete(sec_ctrl) && 
						((now - sec_ctrl->data_arrive_time) > sec_ctrl->repeat_distance)){
						AM_DEBUG(2, "sec repeat, done.");
						TABLE_DONE();
					}
				}
				goto handler_done;
			}
		}
		else
		{
			/* for TDT/TOT section, the section_syntax_indicator bit must be 0 */
			if ((data[1]&0x80) != 0)
			{
				AM_DEBUG(1, "EPG: TDT/TOT section_syntax_indicator is 1, skip this section");
				goto handler_done;
			}
		}
		/*数据处理*/
		switch (data[0])
		{
			case AM_SI_TID_PAT:
				COLLECT_SECTION(dvbpsi_pat_t, mon->pats);
				break;
			case AM_SI_TID_PMT:
				COLLECT_SECTION(dvbpsi_pmt_t, mon->pmts);
				break;
			case AM_SI_TID_SDT_ACT:
				COLLECT_SECTION(dvbpsi_sdt_t, mon->sdts);
				break;
			case AM_SI_TID_CAT:
				COLLECT_SECTION(dvbpsi_cat_t, mon->cats);
				break;
			case AM_SI_TID_NIT_ACT:
				COLLECT_SECTION(dvbpsi_nit_t, mon->nits);
				break;
			case AM_SI_TID_TOT:
			case AM_SI_TID_TDT:
				COLLECT_SECTION(dvbpsi_tot_t, mon->tots);
				goto handler_done;
				break;
			case AM_SI_TID_EIT_PF_ACT:
			case AM_SI_TID_EIT_PF_OTH:
			case AM_SI_TID_EIT_SCHE_ACT:
			case AM_SI_TID_EIT_SCHE_OTH:
			case (AM_SI_TID_EIT_SCHE_ACT + 1):
			case (AM_SI_TID_EIT_SCHE_OTH + 1):
				COLLECT_SECTION(dvbpsi_eit_t, mon->eits);
				break;
			case AM_SI_TID_PSIP_MGT:
				COLLECT_SECTION(dvbpsi_atsc_mgt_t, mon->atsc_mgts);
				break;
			case AM_SI_TID_PSIP_RRT:
				AM_DEBUG(1, "RRT received(%x/%x)", header.sec_num, header.last_sec_num);
				COLLECT_SECTION(dvbpsi_rrt_t, mon->rrts);
				break;
			case AM_SI_TID_PSIP_EIT:
				AM_DEBUG(3, "%s: tid 0x%x, source_id 0x%x received(%x/%x)", sec_ctrl->tname, header.table_id,
								header.extension, header.sec_num, header.last_sec_num);
				COLLECT_SECTION(dvbpsi_atsc_eit_t, mon->atsc_eits);
				break;
			case AM_SI_TID_PSIP_ETT:
				AM_DEBUG(4, "%s: tid 0x%x, source_id 0x%x received(%x/%x)", sec_ctrl->tname, header.table_id,
								header.extension, header.sec_num, header.last_sec_num);
				COLLECT_SECTION(dvbpsi_atsc_ett_t, mon->atsc_etts);
				break;
			case AM_SI_TID_PSIP_STT:
				COLLECT_SECTION(dvbpsi_atsc_stt_t, mon->atsc_stts);
				goto handler_done;
				break;
			case AM_SI_TID_PSIP_TVCT:
			case AM_SI_TID_PSIP_CVCT:
				COLLECT_SECTION(dvbpsi_atsc_vct_t, mon->atsc_vcts);
				break;
			case AM_SI_TID_PSIP_CEA:
				COLLECT_SECTION(dvbpsi_atsc_cea_t, mon->atsc_ceas);
				break;
			default:
				AM_DEBUG(1, "EPG: Unkown section data, table_id 0x%x", data[0]);
				pthread_mutex_unlock(&mon->lock);
				return;
				break;
		}
			
		/*数据处理完毕，查看该表是否已接收完毕所有section*/
		if (am_epg_tablectl_test_complete(sec_ctrl) && sec_ctrl->subs == 1)
			TABLE_DONE();
	}
	else
	{
		AM_DEBUG(1, "EPG: Unknown filter id %d in dmx callback", fid);
	}

handler_done:	
	pthread_mutex_unlock(&mon->lock);

}

static void am_epg_section_handler_psip_eit_ett(int dev_no, int fid, const uint8_t *data, int len, void *user_data)
{
	AM_EPG_Monitor_t *mon = (AM_EPG_Monitor_t*)user_data;
	AM_EPG_TableCtl_t * sec_ctrl;
	AM_SI_SectionHeader_t header;

	UNUSED(dev_no);

	if (mon == NULL)
	{
		AM_DEBUG(1, "EPG: Invalid param user_data in dmx callback");
		return;
	}
	if (!data)
		return;

	pthread_mutex_lock(&mon->lock);
	/*获取接收控制数据*/
	sec_ctrl = am_epg_get_section_ctrl_by_fid(mon, fid);
	if (sec_ctrl)
	{
		{
			/* for Non-TDT/TOT sections, the section_syntax_indicator bit must be 1 */
			if ((data[1]&0x80) == 0)
			{
				AM_DEBUG(1, "EPG: section_syntax_indicator is 0, skip this section");
				goto handler_done;
			}
			if (AM_SI_GetSectionHeader(mon->hsi, (uint8_t*)data, len, &header) != AM_SUCCESS)
			{
				AM_DEBUG(1, "EPG: section header error");
				goto handler_done;
			}
			if (am_epg_tablectl_test_recved(sec_ctrl, &header))
			{
				if (sec_ctrl->subs > 1)
				{
					int now;
					AM_TIME_GetClock(&now);
					if (((now - sec_ctrl->data_arrive_time) > sec_ctrl->repeat_distance)) {
						AM_DEBUG(3, "tid:0x%02x timeout, done. %x > %x", header.table_id,
							now - sec_ctrl->data_arrive_time, sec_ctrl->repeat_distance);
						TABLE_DONE();
					}
				}
			} else {
				AM_TIME_GetClock(&sec_ctrl->last_repeat_time);
			}
		}
		/*数据处理*/
		switch (data[0])
		{
			case AM_SI_TID_PSIP_EIT:
				AM_DEBUG(4, "%s: tid 0x%x, source_id 0x%x received(%x/%x)", sec_ctrl->tname, header.table_id,
								header.extension, header.sec_num, header.last_sec_num);
				COLLECT_SECTION(dvbpsi_atsc_eit_t, mon->atsc_eits);
				break;
			case AM_SI_TID_PSIP_ETT:
				AM_DEBUG(4, "%s: tid 0x%x, source_id 0x%x received(%x/%x)", sec_ctrl->tname, header.table_id,
								header.extension, header.sec_num, header.last_sec_num);
				COLLECT_SECTION(dvbpsi_atsc_ett_t, mon->atsc_etts);
				break;
			default:
				AM_DEBUG(1, "EPG: Unkown section data, table_id 0x%x", data[0]);
				pthread_mutex_unlock(&mon->lock);
				return;
				break;
		}
		/*数据处理完毕，查看该表是否已接收完毕所有section*/
		if (am_epg_tablectl_test_complete(sec_ctrl) && sec_ctrl->subs == 1)
			TABLE_DONE();
	}
	else
	{
		AM_DEBUG(1, "EPG: Unknown filter id %d in dmx callback", fid);
	}

handler_done:
	pthread_mutex_unlock(&mon->lock);

}


/**\brief 请求一个表的section数据*/
static AM_ErrorCode_t am_epg_request_section(AM_EPG_Monitor_t *mon, AM_EPG_TableCtl_t *mcl)
{
	struct dmx_sct_filter_params param;

	if (! mcl->subctl)
		return AM_FAILURE;
		
	if (mcl->fid != -1)
	{
		am_epg_free_filter(mon, &mcl->fid);
	}

	/*分配过滤器*/
	AM_TRY(AM_DMX_AllocateFilter(mon->dmx_dev, &mcl->fid));
	/*设置处理函数*/
	if ((!strncmp(mcl->tname, "ATSC EIT", 8) && mcl->tid == AM_SI_TID_PSIP_EIT)
			|| (!strncmp(mcl->tname, "ATSC ETT", 8) && mcl->tid == AM_SI_TID_PSIP_ETT))
		AM_TRY(AM_DMX_SetCallback(mon->dmx_dev, mcl->fid, am_epg_section_handler_psip_eit_ett, (void*)mon));
	else
		AM_TRY(AM_DMX_SetCallback(mon->dmx_dev, mcl->fid, am_epg_section_handler, (void*)mon));
	
	/*设置过滤器参数*/
	memset(&param, 0, sizeof(param));
	param.pid = mcl->pid;
	param.filter.filter[0] = mcl->tid;
	param.filter.mask[0] = mcl->tid_mask;

	/*当前设置了需要监控的service,则设置PMT的extension*/
	if (mcl->tid == AM_SI_TID_PMT)
	{
		int srv_id = -1;
		if (mon->mon_service != -1)
			srv_id = am_epg_get_current_service_id(mon);
		else if (mon->mon_service_sid != -1)
			srv_id = mon->mon_service_sid;

		if (srv_id != -1) {
			AM_DEBUG(2, "set filter for service %d", srv_id);
			param.filter.filter[1] = (uint8_t)(srv_id>>8);
			param.filter.filter[2] = (uint8_t)srv_id;
			param.filter.mask[1] = 0xff;
			param.filter.mask[2] = 0xff;
		}
	}

	if (mcl->pid != AM_SI_PID_TDT)
	{
		if (mcl->subctl->ver != 0xff && mcl->subs == 1)
		{
			AM_DEBUG(2, "epg start table %s : (!=%d)", mcl->tname, mcl->subctl->ver);
			/*Current next indicator must be 1*/
			param.filter.filter[3] = (mcl->subctl->ver << 1) | 0x01;
			param.filter.mask[3] = 0x3f;
			param.filter.mode[3] = 0xff;

			SUBCTL_CLEAR(mcl->subctl);
		}
		else
		{
			AM_DEBUG(2, "epg start table %s : no version", mcl->tname);
			/*Current next indicator must be 1*/
			param.filter.filter[3] = 0x01;
			param.filter.mask[3] = 0x01;
		}

		param.flags = DMX_CHECK_CRC;
	}

	AM_TRY(AM_DMX_SetSecFilter(mon->dmx_dev, mcl->fid, &param));
	if ((mcl->pid == AM_SI_PID_EIT && mcl->tid >= 0x50 && mcl->tid <= 0x6F)
			|| ((!strncmp(mcl->tname, "ATSC EIT", 8) && mcl->tid == AM_SI_TID_PSIP_EIT)
				|| (!strncmp(mcl->tname, "ATSC ETT", 8) && mcl->tid == AM_SI_TID_PSIP_ETT)))
		AM_TRY(AM_DMX_SetBufferSize(mon->dmx_dev, mcl->fid, 256*1024));
	else
		AM_TRY(AM_DMX_SetBufferSize(mon->dmx_dev, mcl->fid, 16*1024));
	AM_TRY(AM_DMX_StartFilter(mon->dmx_dev, mcl->fid));

	AM_DEBUG(2, "epg start table %s: %d, tid 0x%x, mask 0x%x",
				mcl->tname, mcl->fid, mcl->tid, mcl->tid_mask);
	return AM_SUCCESS;
}

/**\brief 插入一个Subtitle记录*/
static int insert_subtitle(sqlite3 * hdb, int db_srv_id, int pid, dvbpsi_subtitle_t *psd)
{
	sqlite3_stmt *stmt;
	const char *sql = "insert into subtitle_table(db_srv_id,pid,type,composition_page_id,ancillary_page_id,language) values(?,?,?,?,?,?)";
	if (sqlite3_prepare(hdb, sql, strlen(sql), &stmt, NULL) != SQLITE_OK)
	{
		AM_DEBUG(1, "Prepare sqlite3 failed for insert new subtitle");
		return -1;
	}

	sqlite3_bind_int(stmt, 1, db_srv_id);
	sqlite3_bind_int(stmt, 2, pid);
	sqlite3_bind_int(stmt, 3, psd->i_subtitling_type);
	sqlite3_bind_int(stmt, 4, psd->i_composition_page_id);
	sqlite3_bind_int(stmt, 5, psd->i_ancillary_page_id);
	sqlite3_bind_text(stmt, 6, (const char*)psd->i_iso6392_language_code, 3, SQLITE_STATIC);
	AM_DEBUG(1, "Insert a new subtitle");
	sqlite3_step(stmt);
	sqlite3_reset(stmt);
	sqlite3_finalize(stmt);

	return 0;
}

/**\brief 插入一个Teletext记录*/
static int insert_teletext(sqlite3 * hdb, int db_srv_id, int pid, dvbpsi_teletextpage_t *ptd)
{
	sqlite3_stmt *stmt;
	const char *sql = "insert into teletext_table(db_srv_id,pid,type,magazine_number,page_number,language) values(?,?,?,?,?,?)";
	if (sqlite3_prepare(hdb, sql, strlen(sql), &stmt, NULL) != SQLITE_OK)
	{
		AM_DEBUG(1, "Prepare sqlite3 failed for insert new teletext");
		return -1;
	}
	
	sqlite3_bind_int(stmt, 1, db_srv_id);
	sqlite3_bind_int(stmt, 2, pid);
	if (ptd)
	{
		sqlite3_bind_int(stmt, 3, ptd->i_teletext_type);
		sqlite3_bind_int(stmt, 4, ptd->i_teletext_magazine_number);
		sqlite3_bind_int(stmt, 5, ptd->i_teletext_page_number);
		sqlite3_bind_text(stmt, 6, (const char*)ptd->i_iso6392_language_code, 3, SQLITE_STATIC);
	}
	else
	{
		sqlite3_bind_int(stmt, 3, 0);
		sqlite3_bind_int(stmt, 4, 1);
		sqlite3_bind_int(stmt, 5, 0);
		sqlite3_bind_text(stmt, 6, "", -1, SQLITE_STATIC);
	}
	AM_DEBUG(1, "Insert a new teletext");
	sqlite3_step(stmt);
	sqlite3_reset(stmt);
	sqlite3_finalize(stmt);

	return 0;
}

/**\brief 插入一个网络记录，返回其索引*/
static int insert_net(sqlite3 * hdb, int src, int orig_net_id)
{
	int db_id = -1;
	int row;
	char query_sql[256];
	
	snprintf(query_sql, sizeof(query_sql), 
		"select db_id from net_table where src=%d and network_id=%d",
		src, orig_net_id);

	row = 1;
	/*query wether it exists*/
	if (AM_DB_Select(hdb, query_sql, &row, "%d", &db_id) != AM_SUCCESS || row <= 0)
	{
		db_id = -1;
	}

	/*if not exist , insert a new record*/
	if (db_id == -1)
	{
		char insert_sql[256];

		snprintf(insert_sql, sizeof(insert_sql), 
			"insert into net_table(network_id, src, name) values(%d,%d,'')",
			orig_net_id, src);
		sqlite3_exec(hdb, insert_sql, NULL, NULL, NULL);

		row = 1;
		if (AM_DB_Select(hdb, query_sql, &row, "%d", &db_id) != AM_SUCCESS )
		{
			db_id = -1;
		}
	}

	return db_id;
}

static void am_epg_check_pmt_update_def(AM_EPG_Monitor_t *mon)
{
	dvbpsi_pmt_t *pmt;
	dvbpsi_pmt_es_t *es;
	dvbpsi_descriptor_t *descr;
	int vfmt = -1, prev_vfmt = -1;
	int vid = 0x1fff, prev_vid = 0x1fff;
	int afmt_tmp, vfmt_tmp, db_ts_id;
	AM_SI_AudioInfo_t aud_info;
	char sql[1024];
	char str_apids[256];
	char str_afmts[256];
	char str_alangs[256];
	sqlite3 *hdb;

	AM_DB_HANDLE_PREPARE(hdb);

	/* check if ts_id needs update */
	db_ts_id = am_epg_get_current_db_ts_id(mon);
	if (db_ts_id >= 0 && mon->pats)
	{
		char sql[256];
		int ts_id = 0xffff;
		int row = 1;
		
		snprintf(sql, sizeof(sql), "select ts_id from ts_table where db_id=%d", db_ts_id);
		if (AM_DB_Select(hdb, sql, &row, "%d", &ts_id) == AM_SUCCESS && row > 0)
		{
			if (ts_id == 0xffff)
			{
				/* Currently, we only update this invalid ts_id */
				AM_DEBUG(1, "PAT Update: ts %d's ts_id changed: 0x%x -> 0x%x", db_ts_id, ts_id, mon->pats->i_ts_id);
				snprintf(sql, sizeof(sql), "update ts_table set ts_id=%d where db_id=%d",
					mon->pats->i_ts_id, db_ts_id);
				sqlite3_exec(hdb, sql, NULL, NULL, NULL);
				ts_id = mon->pats->i_ts_id;
			}
		}
		if (ts_id != mon->pats->i_ts_id)
		{
			AM_DEBUG(1, "PAT ts_id not match, expect(%d), but got(%d)", ts_id, mon->pats->i_ts_id);
			return;
		}
	}
	
	aud_info.audio_count = 0;
	/*遍历PMT表*/
	AM_SI_LIST_BEGIN(mon->pmts, pmt)
		/*取ES流信息*/
		AM_SI_LIST_BEGIN(pmt->p_first_es, es)
			AM_SI_ExtractAVFromES(es, &vid, &vfmt, &aud_info);
		AM_SI_LIST_END()
	AM_SI_LIST_END()
	
	if (am_epg_check_program_av(hdb, mon->mon_service, vid, vfmt, &aud_info))
	{
		/* delete the previous teletext & subtitle */
		snprintf(sql, sizeof(sql), "delete from subtitle_table where db_srv_id=%d", mon->mon_service);
		sqlite3_exec(hdb, sql, NULL, NULL, NULL);
		snprintf(sql, sizeof(sql), "delete from teletext_table where db_srv_id=%d", mon->mon_service);
		sqlite3_exec(hdb, sql, NULL, NULL, NULL);
		/*遍历PMT表*/
		AM_SI_LIST_BEGIN(mon->pmts, pmt)
			/*取ES流信息*/
			AM_SI_LIST_BEGIN(pmt->p_first_es, es)
				/*查找Subtilte和Teletext描述符，并添加相关记录*/
				AM_SI_LIST_BEGIN(es->p_first_descriptor, descr)
				if (descr->p_decoded && descr->i_tag == AM_SI_DESCR_SUBTITLING)
				{
					int isub;
					dvbpsi_subtitling_dr_t *psd = (dvbpsi_subtitling_dr_t*)descr->p_decoded;

					AM_DEBUG(0, "Find subtitle descriptor, number:%d",psd->i_subtitles_number);
					for (isub=0; isub<psd->i_subtitles_number; isub++)
					{
						insert_subtitle(hdb, mon->mon_service, es->i_pid, &psd->p_subtitle[isub]);
					}
				}
				else if (descr->i_tag == AM_SI_DESCR_TELETEXT)
				{
					int itel;
					dvbpsi_teletext_dr_t *ptd = (dvbpsi_teletext_dr_t*)descr->p_decoded;

					AM_DEBUG(1, "Find teletext descriptor, ptd %p", ptd);
					if (ptd)
					{
						for (itel=0; itel<ptd->i_pages_number; itel++)
						{
							insert_teletext(hdb, mon->mon_service, es->i_pid, &ptd->p_pages[itel]);
						}
					}
					else
					{
						insert_teletext(hdb, mon->mon_service, es->i_pid, NULL);
					}
				}
				AM_SI_LIST_END()
			AM_SI_LIST_END()
		AM_SI_LIST_END()

		/*触发通知事件*/
		SIGNAL_EVENT(AM_EPG_EVT_UPDATE_PROGRAM_AV, (void*)(long)mon->mon_service);
	}
}

static int am_epg_check_sdt_update_stage_version(AM_EPG_Monitor_t *mon)
{
	char tmpsql[256];
	int prev_sdt_ver, db_ts_id;
	sqlite3 *hdb;
	int row;
	int db_srv_id;

	AM_DB_HANDLE_PREPARE(hdb);

	/* check sdt version for update,
	     the scan proccess will be triggered if sdt 1st received after scanning or version changed.*/

	AM_DEBUG(1, "Checking sdt version for service %d", mon->mon_service);
	row = 1;
	snprintf(tmpsql, sizeof(tmpsql), "select sdt_ver from srv_table where db_id=%d", mon->mon_service);
	AM_DB_Select(hdb, tmpsql, &row, "%d", &prev_sdt_ver);
	if (prev_sdt_ver != 0xff) {
		if (prev_sdt_ver == mon->sdts->i_version) {
			return 0;
		} else {
			AM_DEBUG(1, "sdt version changed, (%d -> %d)", prev_sdt_ver, mon->sdts->i_version);
			return 1;
		}
	}

	db_ts_id = am_epg_get_current_db_ts_id(mon);
	if (db_ts_id < 0)
	{
		AM_DEBUG(1, "SDT update: cannot get current ts(%d)!", db_ts_id);
		return 0;
	}

	/*update sdt_ver.*/
	AM_DEBUG(1, "update sdt version for service in ts %d", db_ts_id);
	do {
		row=1;
		snprintf(tmpsql, sizeof(tmpsql), "select db_id from srv_table where db_ts_id=%d and sdt_ver!=%d",
						db_ts_id, mon->sdts->i_version);
		AM_DB_Select(hdb, tmpsql, &row, "%d", &db_srv_id);

		if(row) {
			snprintf(tmpsql, sizeof(tmpsql), "update srv_table set sdt_ver=%d where db_id=%d",
								mon->sdts->i_version, db_srv_id);
			sqlite3_exec(hdb, tmpsql, NULL, NULL, NULL);
		}
	}while(row);

	return -1;
}

static void am_epg_check_sdt_update_def(AM_EPG_Monitor_t *mon)
{
	const char *sql = "select db_id,name from srv_table where db_ts_id=? \
								and service_id=? limit 1";
	const char *update_sql = "update srv_table set name=? where db_id=?";
	sqlite3_stmt *stmt = NULL;
	sqlite3_stmt *update_stmt = NULL;
	int db_srv_id;
	dvbpsi_sdt_t *sdt;
	AM_Bool_t update = AM_FALSE;
	int db_ts_id;
	sqlite3 *hdb;

	AM_DB_HANDLE_PREPARE(hdb);

	if (mon->sdts)
	{
		dvbpsi_sdt_service_t *srv;
		dvbpsi_descriptor_t *descr;
		char tmpsql[256];
		int db_net_id, prev_db_net_id;
		int prev_ts_id, prev_net_id;
		int row;

		int ret = am_epg_check_sdt_update_stage_version(mon);
		if (ret == 0)
			return;

		db_ts_id = am_epg_get_current_db_ts_id(mon);
		if (db_ts_id < 0)
		{
			AM_DEBUG(1, "SDT update: cannot get current ts(%d)!", db_ts_id);
			return;
		}

		if (ret > 0) {
			AM_DEBUG(1, "SDT version changed");
			SIGNAL_EVENT(AM_EPG_EVT_UPDATE_TS, (void*)(long)db_ts_id);
			return;
		}

		/*no version stored before.*/
		/* check ts_id for update */
		AM_DEBUG(1, "Checking ts_id for ts %d", db_ts_id);
		row = 1;
		snprintf(tmpsql, sizeof(tmpsql), "select ts_id from ts_table where db_id=%d", db_ts_id);
		AM_DB_Select(hdb, tmpsql, &row, "%d", &prev_ts_id);
		if(prev_ts_id != mon->sdts->i_ts_id)
		{
			AM_DEBUG(1, "TS id not match, current(%d), but got(%d)", prev_ts_id, mon->sdts->i_ts_id);
			SIGNAL_EVENT(AM_EPG_EVT_UPDATE_TS, (void*)(long)db_ts_id);
			return;
		}

		/* check if db_net_id needs update */
		AM_DEBUG(1, "Checking for ts %d", db_ts_id);
		row = 1;	
		snprintf(tmpsql, sizeof(tmpsql), "select db_net_id from ts_table where db_id=%d", db_ts_id);
		if (AM_DB_Select(hdb, tmpsql, &row, "%d", &prev_db_net_id) == AM_SUCCESS && row > 0)
		{
			AM_DEBUG(1, "Checking for network %d", prev_db_net_id);
			if (prev_db_net_id < 0)
			{
				/* update this invalid db_net_id */
				AM_DEBUG(1, "SDT Update: insert new network on source %d, orginal_network_id=%d",
							mon->src, mon->sdts->i_network_id);
				db_net_id = insert_net(hdb, mon->src, mon->sdts->i_network_id);
				if (db_net_id >= 0)
				{
					AM_DEBUG(1, "SDT Update: ts %d's db_net_id changed: 0x%x -> 0x%x", 
						db_ts_id, prev_db_net_id, db_net_id);
					snprintf(tmpsql, sizeof(tmpsql), "update ts_table set db_net_id=%d where db_id=%d",
						db_net_id, db_ts_id);
					sqlite3_exec(hdb, tmpsql, NULL, NULL, NULL);
					/* update services */
					snprintf(tmpsql, sizeof(tmpsql), "update srv_table set db_net_id=%d where db_ts_id=%d",
						db_net_id, db_ts_id);
					sqlite3_exec(hdb, tmpsql, NULL, NULL, NULL);

					update = AM_TRUE;
				}
				else
				{
					AM_DEBUG(1, "insert new network failed");
				}
			}
			else
			{
				row = 1;	
				snprintf(tmpsql, sizeof(tmpsql), "select network_id from net_table where db_id=%d", prev_db_net_id);
				if (AM_DB_Select(hdb, tmpsql, &row, "%d", &prev_net_id) == AM_SUCCESS && row > 0)
				{
					if(prev_net_id != mon->sdts->i_network_id)
					{
						AM_DEBUG(1, "Network id not match, current(%d), but got(%d)", prev_net_id, mon->sdts->i_network_id);
						SIGNAL_EVENT(AM_EPG_EVT_UPDATE_TS, (void*)(long)db_ts_id);
						return;
					}
				}
			}
		}
		else
		{
			AM_DEBUG(1, "select db_net_id failed");
		}

		if (sqlite3_prepare(hdb, sql, strlen(sql), &stmt, NULL) != SQLITE_OK)
		{
			AM_DEBUG(1, "Prepare sqlite3 failed for selecting service name");
			return;
		}
		if (sqlite3_prepare(hdb, update_sql, strlen(update_sql), &update_stmt, NULL) != SQLITE_OK)
		{
			AM_DEBUG(1, "Prepare sqlite3 failed for updating service name");
			goto update_end;
		}

		AM_SI_LIST_BEGIN(mon->sdts, sdt)
		AM_SI_LIST_BEGIN(sdt->p_first_service, srv)
			/*从SDT表中查找该service名称*/
			AM_SI_LIST_BEGIN(srv->p_first_descriptor, descr)
			if (descr->p_decoded && descr->i_tag == AM_SI_DESCR_SERVICE)
			{
				dvbpsi_service_dr_t *psd = (dvbpsi_service_dr_t*)descr->p_decoded;
				char name[AM_DB_MAX_SRV_NAME_LEN + 4];
				const unsigned char *old_name;

				/*取节目名称*/
				if (psd->i_service_name_length > 0)
				{
					/*default language code*/
					strcpy(name, "xxx");
					AM_SI_ConvertDVBTextCode((char*)psd->i_service_name, psd->i_service_name_length,\
								name+3, AM_DB_MAX_SRV_NAME_LEN);
					name[AM_DB_MAX_SRV_NAME_LEN+3] = 0;

					sqlite3_bind_int(stmt, 1, db_ts_id);
					sqlite3_bind_int(stmt, 2, srv->i_service_id);
					if (sqlite3_step(stmt) == SQLITE_ROW)
					{
						db_srv_id = sqlite3_column_int(stmt, 0);
						old_name = sqlite3_column_text(stmt, 1);
						if (old_name != NULL && !strcmp((const char*)old_name, "xxxNo Name"))
						{
							AM_DEBUG(1, "SDT Update: Program name changed: %s -> %s", old_name, name);
							sqlite3_bind_text(update_stmt, 1, (const char*)name, -1, SQLITE_STATIC);
							sqlite3_bind_int(update_stmt, 2, db_srv_id);
							sqlite3_step(update_stmt);
							sqlite3_reset(update_stmt);
							AM_DEBUG(1, "Update '%s' done!", name);
							/*触发通知事件*/
							SIGNAL_EVENT(AM_EPG_EVT_UPDATE_PROGRAM_NAME, (void*)(long)db_srv_id);
							if (! update)
								update = AM_TRUE;
						}
						else
						{
							//AM_DEBUG(1, "SDT Update: Program name '%s' not changed !", old_name);
						}
					}
					else
					{
						AM_DEBUG(1, "SDT Update: Cannot find program for db_ts_id=%d  srv=%d",
							db_ts_id,srv->i_service_id);
						SIGNAL_EVENT(AM_EPG_EVT_UPDATE_TS, (void*)(long)db_ts_id);
						goto update_end;
					}
					sqlite3_reset(stmt);
				}
			}
			AM_SI_LIST_END()
		AM_SI_LIST_END()
		AM_SI_LIST_END()
update_end:
		if (stmt != NULL)	
			sqlite3_finalize(stmt);
		if (update_stmt != NULL)
			sqlite3_finalize(update_stmt);
	}
}

static void am_epg_check_pmt_update(AM_EPG_Monitor_t *mon)
{
	int sid = -1;

	if (mon->mon_service != -1)
		sid = am_epg_get_current_service_id(mon);
	else if (mon->mon_service_sid != -1)
		sid = mon->mon_service_sid;

	if (sid == -1 || mon->pmts == NULL)
		return;

	if (sid != mon->pmts->i_program_number)
	{
		AM_DEBUG(1, "PMT update: Program number dismatch!!!");
		return;
	}

	TAB_CB(mon, AM_EPG_TAB_PMT, mon->pmts);

	if (mon->pmt_cb)
		mon->pmt_cb((AM_EPG_Handle_t)mon, mon->pmts);

	if (!mon->disable_def_proc)
		am_epg_check_pmt_update_def(mon);
}

static void am_epg_check_sdt_update(AM_EPG_Monitor_t *mon)
{
	if (!mon->disable_def_proc)
		am_epg_check_sdt_update_def(mon);
}

static void am_epg_proc_eit_section(AM_EPG_Monitor_t *mon, void *eit_section)
{
	TAB_CB(mon, AM_EPG_TAB_EIT, eit_section);

	if (mon->evt_cb) {
		int evt_cnt;
		AM_EPG_Event_t *pevents;
		am_epg_eit_get_event_list(mon, eit_section, &evt_cnt, &pevents);
		mon->evt_cb((AM_EPG_Handle_t)mon, evt_cnt, pevents);
	}

	if (!mon->disable_def_proc)
		am_epg_proc_eit_section_def(mon, eit_section);
}

static void am_epg_proc_rrt_section(AM_EPG_Monitor_t *mon, void *rrt_section)
{
	TAB_CB(mon, AM_EPG_TAB_RRT, rrt_section);

	if (!mon->disable_def_proc)
		am_epg_proc_rrt_section_def(mon, rrt_section);
}

static void am_epg_proc_psip_eit_section(AM_EPG_Monitor_t *mon, void *eit_section)
{
	TAB_CB(mon, AM_EPG_TAB_PSIP_EIT, eit_section);

	if (mon->evt_cb) {
		int evt_cnt;
		AM_EPG_Event_t *pevents;
		am_epg_psip_eit_get_event_list(mon, eit_section, &evt_cnt, &pevents);
		mon->evt_cb((AM_EPG_Handle_t)mon, evt_cnt, pevents);
	}

	if (!mon->disable_def_proc)
		am_epg_proc_psip_eit_section_def(mon, eit_section);
}

static void am_epg_proc_psip_ett_section(AM_EPG_Monitor_t *mon, void *ett_section)
{
	TAB_CB(mon, AM_EPG_TAB_PSIP_ETT, ett_section);

	if (mon->evt_cb) {
		int evt_cnt;
		AM_EPG_Event_t *pevents;
		am_epg_psip_ett_get_event_list(mon, ett_section, &evt_cnt, &pevents);
		mon->evt_cb((AM_EPG_Handle_t)mon, evt_cnt, pevents);
	}

	if (!mon->disable_def_proc)
		am_epg_proc_psip_ett_section_def(mon, ett_section);
}

static void am_epg_proc_vct_section(AM_EPG_Monitor_t *mon, void *vct_section)
{
	if (!mon->disable_def_proc)
		am_epg_proc_vct_section_def(mon, vct_section);
}

static void am_epg_proc_pmt_section(AM_EPG_Monitor_t *mon, void *pmt_section)
{
	dvbpsi_pmt_t *p_pmt = (dvbpsi_pmt_t*)pmt_section;
	int i_version = p_pmt->i_version;
	dvbpsi_descriptor_t *descr;
	int i_program_number = p_pmt->i_program_number;
	AM_EPG_PmtRating_t pmt_rate;
	int values_size = 1024;
	char values[1024];

	memset(values, 0, 1024);
	memset((void *)&pmt_rate, 0, sizeof(AM_EPG_PmtRating_t));

	//AM_DEBUG(1, "ver:%d i_program_number:%d", i_version, i_program_number);
	pmt_rate.rating[0] = 0;
	pmt_rate.i_program_number = i_program_number;
	sprintf(pmt_rate.rating, "{");
	AM_SI_GetRatingStringFromDescriptors(p_pmt->p_first_descriptor, values, values_size);
	if (strlen(values))
			snprintf(pmt_rate.rating, sizeof(pmt_rate.rating), "%sDratings:%s", pmt_rate.rating, values);
	snprintf(pmt_rate.rating, sizeof(pmt_rate.rating), "%s}", pmt_rate.rating);

	//send pmt has rating event
	/*触发通知事件*/
	SIGNAL_EVENT(AM_EPG_EVT_PMT_RATING, (void*)(long)&pmt_rate);
	AM_DEBUG(0, "PMT ver:%d i_program_number:%d rate:[%s]values_size:%d", i_version, i_program_number, pmt_rate.rating, values_size);
}
/**\brief NIT搜索完毕处理*/
static void am_epg_nit_done(AM_EPG_Monitor_t *mon)
{
	am_epg_free_filter(mon, &mon->nitctl.fid);
	/*触发通知事件*/
	if (mon->nits != NULL) {
		SIGNAL_EVENT(AM_EPG_EVT_NEW_NIT, (void*)(long)mon->nits->i_version);
	}
	TAB_CB(mon, AM_EPG_TAB_NIT, mon->nits);

	/* release for updating new tables */
	RELEASE_TABLE_FROM_LIST(dvbpsi_nit_t, mon->nits);
	/*监控下一版本*/
	if (mon->nitctl.subctl)
	{
		//mon->nitctl.subctl->ver++;
		am_epg_request_section(mon, &mon->nitctl);
	}
}

/**\brief PAT搜索完毕处理*/
static void am_epg_pat_done(AM_EPG_Monitor_t *mon)
{
	int db_ts_id;
	
	am_epg_free_filter(mon, &mon->patctl.fid);

	/*触发通知事件*/
	SIGNAL_EVENT(AM_EPG_EVT_NEW_PAT, (void*)mon->pats);

	TAB_CB(mon, AM_EPG_TAB_PAT, mon->pats);

	/*监控下一版本*/
	if (mon->patctl.subctl)
	{
		//mon->patctl.subctl->ver++;
		mon->patctl.subctl->ver = 0xff;
		//am_epg_request_section(mon, &mon->patctl);
	}
	
	/* Search for PMT of mon_service */
	if (mon->mode&AM_EPG_SCAN_PMT)
	{
		int sid = -1;
		if (mon->mon_service != -1)
			sid = am_epg_get_current_service_id(mon);
		else if (mon->mon_service_sid != -1)
			sid = mon->mon_service_sid;
		if (sid != -1) {
			dvbpsi_pat_t *pat;
			dvbpsi_pat_program_t *prog;
			
			AM_SI_LIST_BEGIN(mon->pats, pat)
				AM_SI_LIST_BEGIN(pat->p_first_program, prog)
					if (prog->i_number == sid)
					{
						AM_DEBUG(3, "Got program %d's PMT, pid is 0x%x", prog->i_number, prog->i_pid);
						mon->pmtctl.pid = prog->i_pid;
						SET_MODE(pmt, pmtctl, AM_EPG_SCAN_PMT, AM_TRUE);
						goto release_pats;
					}
				AM_SI_LIST_END()
			AM_SI_LIST_END()
		}
	}

release_pats:
	/* release for updating new tables */
	RELEASE_TABLE_FROM_LIST(dvbpsi_pat_t, mon->pats);		
}

/**\brief PMT搜索完毕处理*/
static void am_epg_pmt_done(AM_EPG_Monitor_t *mon)
{
	am_epg_free_filter(mon, &mon->pmtctl.fid);

	/*触发通知事件*/
	SIGNAL_EVENT(AM_EPG_EVT_NEW_PMT, (void*)mon->pmts);

	am_epg_check_pmt_update(mon);
	/* release for updating new tables */
	RELEASE_TABLE_FROM_LIST(dvbpsi_pmt_t, mon->pmts);

	/*监控下一版本*/
	if (mon->pmtctl.subctl)
	{
		//am_epg_request_section(mon, &mon->pmtctl);
	}
}

/**\brief CAT搜索完毕处理*/
static void am_epg_cat_done(AM_EPG_Monitor_t *mon)
{
	am_epg_free_filter(mon, &mon->catctl.fid);

	/*触发通知事件*/
	SIGNAL_EVENT(AM_EPG_EVT_NEW_CAT, (void*)mon->cats);

	TAB_CB(mon, AM_EPG_TAB_CAT, mon->cats);

	/* release for updating new tables */
	RELEASE_TABLE_FROM_LIST(dvbpsi_cat_t, mon->cats);
	
	/*监控下一版本*/
	if (mon->catctl.subctl)
	{
		am_epg_request_section(mon, &mon->catctl);
	}
}

/**\brief cea搜索完毕处理*/
static void am_epg_cea_done(AM_EPG_Monitor_t *mon)
{
	am_epg_free_filter(mon, &mon->ceactl.fid);

	/*触发通知事件*/
	SIGNAL_EVENT(AM_EPG_EVT_NEW_CEA, (void*)mon->atsc_ceas);

	/* release for updating new tables */
	RELEASE_TABLE_FROM_LIST(dvbpsi_atsc_cea_t, mon->atsc_ceas);
	/*监控下一版本*/
	if (mon->ceactl.subctl)
	{
		am_epg_request_section(mon, &mon->ceactl);
	}
}

/**\brief SDT搜索完毕处理*/
static void am_epg_sdt_done(AM_EPG_Monitor_t *mon)
{
	am_epg_free_filter(mon, &mon->sdtctl.fid);

	/*触发通知事件*/
	SIGNAL_EVENT(AM_EPG_EVT_NEW_SDT, (void*)mon->sdts);

	TAB_CB(mon, AM_EPG_TAB_SDT, mon->sdts);

	am_epg_check_sdt_update(mon);
	/* release for updating new tables */
	RELEASE_TABLE_FROM_LIST(dvbpsi_sdt_t, mon->sdts);
	
	/*监控下一版本*/
	if (mon->sdtctl.subctl)
	{
		am_epg_request_section(mon, &mon->sdtctl);
	}
}

/**\brief TDT搜索完毕处理*/
static void am_epg_tdt_done(AM_EPG_Monitor_t *mon)
{
	am_epg_free_filter(mon, &mon->totctl.fid);
	
	SIGNAL_EVENT(AM_EPG_EVT_NEW_TDT, (void*)mon->tots);

	TAB_CB(mon, AM_EPG_TAB_TDT, mon->tots);

	/* release for updating new tables */
	RELEASE_TABLE_FROM_LIST(dvbpsi_tot_t, mon->tots);

	/*设置完成时间以进行下一次刷新*/
	AM_TIME_GetClock(&mon->totctl.check_time);
}

/**\brief EIT pf actual 搜索完毕处理*/
static void am_epg_eit4e_done(AM_EPG_Monitor_t *mon)
{
	am_epg_free_filter(mon, &mon->eit4ectl.fid);

	/*设置完成时间以进行下一次刷新*/
	AM_TIME_GetClock(&mon->eit4ectl.check_time);
	mon->eit4ectl.data_arrive_time = 0;
}

/**\brief EIT pf other 搜索完毕处理*/
static void am_epg_eit4f_done(AM_EPG_Monitor_t *mon)
{
	am_epg_free_filter(mon, &mon->eit4fctl.fid);

	/*设置完成时间以进行下一次刷新*/
	AM_TIME_GetClock(&mon->eit4fctl.check_time);
	mon->eit4fctl.data_arrive_time = 0;
}

/**\brief EIT schedule actual tableid=0x50 搜索完毕处理*/
static void am_epg_eit50_done(AM_EPG_Monitor_t *mon)
{
	am_epg_free_filter(mon, &mon->eit50ctl.fid);

	/*设置完成时间以进行下一次刷新*/
	AM_TIME_GetClock(&mon->eit50ctl.check_time);
	mon->eit50ctl.data_arrive_time = 0;

	if (!mon->evt_cb) {
		sqlite3 *hdb;
		AM_DB_HANDLE_PREPARE(hdb);
		/*Delete the expired events*/
		am_epg_delete_expired_events(hdb);
	}
}

/**\brief EIT schedule actual tableid=0x51 搜索完毕处理*/
static void am_epg_eit51_done(AM_EPG_Monitor_t *mon)
{
	am_epg_free_filter(mon, &mon->eit51ctl.fid);

	/*设置完成时间以进行下一次刷新*/
	AM_TIME_GetClock(&mon->eit51ctl.check_time);
	mon->eit51ctl.data_arrive_time = 0;
}

/**\brief EIT schedule other tableid=0x60 搜索完毕处理*/
static void am_epg_eit60_done(AM_EPG_Monitor_t *mon)
{
	am_epg_free_filter(mon, &mon->eit60ctl.fid);

	/*设置完成时间以进行下一次刷新*/
	AM_TIME_GetClock(&mon->eit60ctl.check_time);
	mon->eit60ctl.data_arrive_time = 0;

	if (!mon->evt_cb) {
		sqlite3 *hdb;
		AM_DB_HANDLE_PREPARE(hdb);
		/*Delete the expired events*/
		am_epg_delete_expired_events(hdb);
	}
}

/**\brief EIT schedule other tableid=0x61 搜索完毕处理*/
static void am_epg_eit61_done(AM_EPG_Monitor_t *mon)
{
	am_epg_free_filter(mon, &mon->eit61ctl.fid);

	/*设置完成时间以进行下一次刷新*/
	AM_TIME_GetClock(&mon->eit61ctl.check_time);
	mon->eit61ctl.data_arrive_time = 0;
}

static void am_epg_psip_eit_stop(AM_EPG_Monitor_t *mon)
{
	int i;
	for (i = 0; i < mon->psip_eit_count; i++) {
		if (mon->psip_eitctl[i].fid != -1) //stop the filter
			SET_MODE_EX(0, atsc_eit, psip_eitctl[i], 0, 0);
	}
}

static void am_epg_psip_eit_start(AM_EPG_Monitor_t *mon)
{
	int j;
	int first_start = 0;
	/* Start the next pending EIT */
	for (j = 0; j < mon->psip_eit_count; j++)
	{
		if (mon->psip_eitctl[j].pid < 0x1fff
			&& mon->psip_eitctl[j].fid == -1
			&& mon->psip_eitctl[j].data_arrive_time == 0)
		{
			if (j == 0 && first_start == 0)
				first_start = 1;
			if (first_start) {//parallel the first start
				if (j >= mon->parallel_psip_eit_count)
					break;
				am_epg_request_section(mon, &mon->psip_eitctl[j]);
				mon->eit_looped = AM_FALSE;
			} else {
				am_epg_request_section(mon, &mon->psip_eitctl[j]);
				break;
			}
		}
	}
	if (j >= mon->psip_eit_count) {//eit-k collection finished, monitor eit0
		if (mon->psip_eitctl[0].pid < 0x1fff
				&& mon->psip_eitctl[0].fid == -1) {
			SET_MODE_EX(1, atsc_eit, psip_eitctl[0], 1, 1);//restart eit0
			mon->eit_looped = AM_TRUE;
		}
	}
}

static void am_epg_psip_ett_stop(AM_EPG_Monitor_t *mon)
{
	int i;
	for (i = 0; i < mon->psip_ett_count; i++) {
		if (mon->psip_ettctl[i].fid != -1) //stop the filter
			SET_MODE_EX(0, atsc_ett, psip_ettctl[i], 0, 0);
	}
}

static void am_epg_psip_ett_start(AM_EPG_Monitor_t *mon, int parrallel)
{
	int j;
	int count = 0;
	/* Start the next pending ETT */
	for (j = 0; j < mon->psip_ett_count; j++)
	{
		if (mon->psip_ettctl[j].pid < 0x1fff &&
			mon->psip_ettctl[j].fid == -1 &&
			mon->psip_ettctl[j].data_arrive_time == 0)
		{
			am_epg_request_section(mon, &mon->psip_ettctl[j]);
			if (++count >= parrallel)
				break;
		}
	}
	if (j >= mon->psip_ett_count) {//ett-k collection finished, monitor ett0
		if (mon->psip_ettctl[0].pid < 0x1fff
				&& mon->psip_ettctl[0].fid == -1)
		SET_MODE_EX(1, atsc_ett, psip_ettctl[0], 1, 1);//restart ett0
	}
}

/**\brief STT搜索完毕处理*/
static void am_epg_stt_done(AM_EPG_Monitor_t *mon)
{
	am_epg_free_filter(mon, &mon->sttctl.fid);

	/*触发通知事件*/
	SIGNAL_EVENT(AM_EPG_EVT_NEW_STT, (void*)mon->atsc_stts);

	TAB_CB(mon, AM_EPG_TAB_STT, mon->atsc_stts);

	/* release for updating new tables */
	RELEASE_TABLE_FROM_LIST(dvbpsi_atsc_stt_t, mon->atsc_stts);

	/*设置完成时间以进行下一次刷新*/
	AM_TIME_GetClock(&mon->sttctl.check_time);
	if (mon->sttctl.subctl)
	{
		am_epg_request_section(mon, &mon->sttctl);
	}

#if 0
	if (mon->psip_eit_request) {
		mon->psip_eit_request = 0;
		AM_DEBUG(1, "eit requested, start mgt:");
		SET_MODE_FORCE(atsc_mgt, mgtctl, AM_EPG_SCAN_MGT, AM_TRUE);
	}
#endif
}

/**\brief RRT搜索完毕处理*/
static void am_epg_rrt_done(AM_EPG_Monitor_t *mon)
{
	/*触发通知事件*/
	SIGNAL_EVENT(AM_EPG_EVT_NEW_RRT, (void*)mon->rrts);

	TAB_CB(mon, AM_EPG_TAB_RRT, mon->rrts);

	/* release for updating new tables */

	RELEASE_TABLE_FROM_LIST(dvbpsi_rrt_t, mon->rrts);
		/*监控下一版本*/
	if (mon->rrtctl.subctl)
	{
		am_epg_request_section(mon, &mon->rrtctl);
	}
}

/**\brief MGT搜索完毕处理*/
static void am_epg_mgt_done(AM_EPG_Monitor_t *mon)
{
	int eit,ett, parallel_cnt = 0,ett_parallel_cnt = 0;
	dvbpsi_atsc_mgt_t *mgt;
	dvbpsi_atsc_mgt_table_t *table;
	
	am_epg_free_filter(mon, &mon->mgtctl.fid);

	if (HAS_MODE(mon->mode, AM_EPG_SCAN_MGT)) {
		/*触发通知事件*/
		SIGNAL_EVENT(AM_EPG_EVT_NEW_MGT, (void*)mon->atsc_mgts);
	
		TAB_CB(mon, AM_EPG_TAB_MGT, mon->atsc_mgts);
	}

	if (HAS_MODE(mon->mode, AM_EPG_SCAN_VCT)) {
		if (mon->atsc_mgts != NULL)
		{
			int vct_tid = AM_SI_TID_PSIP_TVCT;
			dvbpsi_atsc_mgt_table_t *p_table = mon->atsc_mgts->p_first_table;

			do {
				if (p_table) {
					if (p_table->i_table_type == 0 || p_table->i_table_type == 1)
						vct_tid = AM_SI_TID_PSIP_TVCT;
					else if (p_table->i_table_type == 2 || p_table->i_table_type == 3)
						vct_tid = AM_SI_TID_PSIP_CVCT;
				}
			} while (!!(p_table = p_table->p_next));

			AM_DEBUG(1, "VCT table type: %s", (vct_tid==AM_SI_TID_PSIP_CVCT) ? "CVCT" : "TVCT");
			mon->vctctl.tid = vct_tid;
			SET_MODE(atsc_vct, vctctl, AM_EPG_SCAN_VCT, AM_TRUE);
		}
	}

	/*检查EIT是否需要更新*/
	AM_SI_LIST_BEGIN(mon->atsc_mgts, mgt)
	AM_SI_LIST_BEGIN(mgt->p_first_table, table)
	AM_DEBUG(2, "MGT: table_type 0x%x, pid:0x%x, eit_count:%d, ver:%d",
		table->i_table_type, table->i_table_type_pid, mon->psip_eit_count, table->i_table_type_version);
	if (table->i_table_type >= AM_SI_ATSC_TT_EIT0 &&
		table->i_table_type < (AM_SI_ATSC_TT_EIT0+mon->psip_eit_count))
	{
		if (!HAS_MODE(mon->mode, AM_EPG_SCAN_PSIP_EIT))
			continue;
		if (!curr_time.tdt_utc_time) {
			mon->psip_eit_request = 1;
			continue;
		}

		eit = table->i_table_type - AM_SI_ATSC_TT_EIT0;

		if (mon->psip_eitctl[eit].subctl)
		{
			if (mon->psip_eitctl[eit].pid != table->i_table_type_pid ||
				mon->psip_eitctl[eit].subctl->ver != table->i_table_type_version)
			{
				AM_DEBUG(1, "EIT changed:%d pid/version(0x%x /%d -> 0x%x/%d)", eit,
					mon->psip_eitctl[eit].pid, mon->psip_eitctl[eit].subctl->ver,
					table->i_table_type_pid, table->i_table_type_version);
				am_epg_tablectl_clear(&mon->psip_eitctl[eit]);
				mon->psip_eitctl[eit].pid = table->i_table_type_pid;
				mon->psip_eitctl[eit].subctl->ver = table->i_table_type_version;
				//we only add EIT0~EIT3
				if ((parallel_cnt < mon->parallel_psip_eit_count) && (eit < mon->parallel_psip_eit_count))
				{
					am_epg_request_section(mon, &mon->psip_eitctl[parallel_cnt]);
					parallel_cnt++;
				} else {
					AM_DEBUG(1,"MGT done found more EIT version change but filter not be setted.");
				}
			}
			else
			{
				AM_DEBUG(2, "EIT not changed:%d pid/version(0x%x /%d -> 0x%x/%d)", eit,
					mon->psip_eitctl[eit].pid, mon->psip_eitctl[eit].subctl->ver,
					table->i_table_type_pid, table->i_table_type_version);
			}
		}
	}
	else if (table->i_table_type >= AM_SI_ATSC_TT_ETT0 &&
		table->i_table_type < (AM_SI_ATSC_TT_ETT0+mon->psip_ett_count))
	{
		if (!HAS_MODE(mon->mode, AM_EPG_SCAN_PSIP_ETT))
			continue;
		if (!curr_time.tdt_utc_time) {
			mon->psip_ett_request = 1;
			continue;
		}

		ett = table->i_table_type - AM_SI_ATSC_TT_ETT0;
				
		if (mon->psip_ettctl[ett].subctl)
		{
			if (mon->psip_ettctl[ett].pid != table->i_table_type_pid ||
				mon->psip_ettctl[ett].subctl->ver != table->i_table_type_version)
			{
				AM_DEBUG(1, "ETT changed:%d pid/ver(%d/%d -> %d/%d)", ett,
					mon->psip_ettctl[ett].pid, mon->psip_ettctl[ett].subctl->ver,
						table->i_table_type_pid, table->i_table_type_version);
				am_epg_tablectl_clear(&mon->psip_ettctl[ett]);
				mon->psip_ettctl[ett].pid = table->i_table_type_pid;
				mon->psip_ettctl[ett].subctl->ver = table->i_table_type_version;
				if (ett_parallel_cnt < mon->parallel_psip_ett_count)
				{
					ett_parallel_cnt++;
					//do not start here, start in eit done
					//am_epg_request_section(mon, &mon->psip_ettctl[ett]);
				}
			}
			else
			{
				AM_DEBUG(2, "ETT not changed:%d pid/ver(%d/%d)", ett,
					mon->psip_ettctl[ett].pid, mon->psip_ettctl[ett].subctl->ver);
			}
		}
	}
	AM_SI_LIST_END()
	AM_SI_LIST_END()

	/* release for updating new tables */
	RELEASE_TABLE_FROM_LIST(dvbpsi_atsc_mgt_t, mon->atsc_mgts);
	
	/*监控下一版本*/
	if (mon->mgtctl.subctl)
	{
		am_epg_request_section(mon, &mon->mgtctl);
	}
}

/**\brief VCT搜索完毕处理*/
static void am_epg_vct_done(AM_EPG_Monitor_t *mon)
{
	am_epg_free_filter(mon, &mon->vctctl.fid);
	/*触发通知事件*/
	SIGNAL_EVENT(AM_EPG_EVT_NEW_VCT, (void*)mon->atsc_vcts);

	TAB_CB(mon, AM_EPG_TAB_VCT, mon->atsc_vcts);

	RELEASE_TABLE_FROM_LIST(dvbpsi_atsc_vct_t, mon->atsc_vcts);

	/*监控下一版本*/
	if (mon->vctctl.subctl)
	{
		//am_epg_request_section(mon, &mon->vctctl);
	}
}

/**\brief PSIP EIT搜索完毕处理*/
static void am_epg_psip_eit_done(AM_EPG_Monitor_t *mon)
{
	int i, j;

	for (i=0; i<mon->psip_eit_count; i++)
	{
		if (BIT_TEST(mon->psip_eit_done_flag, i))
		{
			am_epg_free_filter(mon, &mon->psip_eitctl[i].fid);

			BIT_CLEAR(mon->psip_eit_done_flag, i);

			/* Start the next pending EIT */
			am_epg_psip_eit_start(mon);

			//hack here for bad ett update performance$
			if (i < 2)
				am_epg_psip_ett_start(mon, 1);
		}
	}

	/*触发通知事件*/
	SIGNAL_EVENT(AM_EPG_EVT_NEW_PSIP_EIT, (void*)mon->atsc_eits);//evt nothing, has alreay evt per section.

	/* release for updating new tables */
	RELEASE_TABLE_FROM_LIST(dvbpsi_atsc_eit_t, mon->atsc_eits);
}

static void am_epg_psip_ett_done(AM_EPG_Monitor_t *mon)
{
	int i, j;

	for (i=0; i<mon->psip_ett_count; i++)
	{
		if (BIT_TEST(mon->psip_ett_done_flag, i))
		{
			am_epg_free_filter(mon,&mon->psip_ettctl[i].fid);

			BIT_CLEAR(mon->psip_ett_done_flag, i);

			/* Start the next pending EIT */
			if (mon->eit_looped)
				am_epg_psip_ett_start(mon, 1);
		}
	}

	/*触发通知事件*/
	SIGNAL_EVENT(AM_EPG_EVT_NEW_PSIP_ETT, (void*)mon->atsc_etts);//evt nothing, has alreay evt per section.

	/* release for updating new tables */
	RELEASE_TABLE_FROM_LIST(dvbpsi_atsc_ett_t, mon->atsc_etts);
}



/**\brief table control data init*/
static void am_epg_tablectl_data_init(AM_EPG_Monitor_t *mon)
{
	int i;
	char name[32];
	
	am_epg_tablectl_init(&mon->patctl, AM_EPG_EVT_PAT_DONE, AM_SI_PID_PAT, AM_SI_TID_PAT,
							 0xff, "PAT", 1, am_epg_pat_done, 0, NULL);
	am_epg_tablectl_init(&mon->pmtctl, AM_EPG_EVT_PMT_DONE, 0x1fff, 	   AM_SI_TID_PMT,
							 0xff, "PMT", 1, am_epg_pmt_done, 0, am_epg_proc_pmt_section);
	am_epg_tablectl_init(&mon->catctl, AM_EPG_EVT_CAT_DONE, AM_SI_PID_CAT, AM_SI_TID_CAT,
							 0xff, "CAT", 1, am_epg_cat_done, 0, NULL);
	am_epg_tablectl_init(&mon->sdtctl, AM_EPG_EVT_SDT_DONE, AM_SI_PID_SDT, AM_SI_TID_SDT_ACT,
							 0xff, "SDT", 1, am_epg_sdt_done, 0, NULL);
	am_epg_tablectl_init(&mon->nitctl, AM_EPG_EVT_NIT_DONE, AM_SI_PID_NIT, AM_SI_TID_NIT_ACT,
							 0xff, "NIT", 1, am_epg_nit_done, 0, NULL);
	am_epg_tablectl_init(&mon->totctl, AM_EPG_EVT_TDT_DONE, AM_SI_PID_TDT, AM_SI_TID_TOT,
							 0xfc, "TDT/TOT", 1, am_epg_tdt_done, 0, am_epg_proc_tot_section);
	am_epg_tablectl_init(&mon->eit4ectl, AM_EPG_EVT_EIT4E_DONE, AM_SI_PID_EIT, AM_SI_TID_EIT_PF_ACT,
							 0xff, "EIT pf actual", MAX_EIT4E_SUBTABLE_CNT, am_epg_eit4e_done, EIT4E_REPEAT_DISTANCE, am_epg_proc_eit_section);
	am_epg_tablectl_init(&mon->eit4fctl, AM_EPG_EVT_EIT4F_DONE, AM_SI_PID_EIT, AM_SI_TID_EIT_PF_OTH,
							 0xff, "EIT pf other", MAX_EIT_SUBTABLE_CNT, am_epg_eit4f_done, EIT4F_REPEAT_DISTANCE, am_epg_proc_eit_section);
	am_epg_tablectl_init(&mon->eit50ctl, AM_EPG_EVT_EIT50_DONE, AM_SI_PID_EIT, AM_SI_TID_EIT_SCHE_ACT,
							 0xff, "EIT sche act(50)", MAX_EIT_SUBTABLE_CNT, am_epg_eit50_done, EIT50_REPEAT_DISTANCE, am_epg_proc_eit_section);
	am_epg_tablectl_init(&mon->eit51ctl, AM_EPG_EVT_EIT51_DONE, AM_SI_PID_EIT, AM_SI_TID_EIT_SCHE_ACT + 1,
							 0xff, "EIT sche act(51)", MAX_EIT_SUBTABLE_CNT, am_epg_eit51_done, EIT51_REPEAT_DISTANCE, am_epg_proc_eit_section);
	am_epg_tablectl_init(&mon->eit60ctl, AM_EPG_EVT_EIT60_DONE, AM_SI_PID_EIT, AM_SI_TID_EIT_SCHE_OTH,
							 0xff, "EIT sche other(60)", MAX_EIT_SUBTABLE_CNT, am_epg_eit60_done, EIT60_REPEAT_DISTANCE, am_epg_proc_eit_section);
	am_epg_tablectl_init(&mon->eit61ctl, AM_EPG_EVT_EIT61_DONE, AM_SI_PID_EIT, AM_SI_TID_EIT_SCHE_OTH + 1,
							 0xff, "EIT sche other(61)", MAX_EIT_SUBTABLE_CNT, am_epg_eit61_done, EIT61_REPEAT_DISTANCE, am_epg_proc_eit_section);
	am_epg_tablectl_init(&mon->sttctl, AM_EPG_EVT_STT_DONE, AM_SI_ATSC_BASE_PID, AM_SI_TID_PSIP_STT,
							 0xff, "STT", 1, am_epg_stt_done, 0, am_epg_proc_stt_section);
	am_epg_tablectl_init(&mon->mgtctl, AM_EPG_EVT_MGT_DONE, AM_SI_ATSC_BASE_PID, AM_SI_TID_PSIP_MGT,
							 0xff, "MGT", 1, am_epg_mgt_done, 0, NULL);
	am_epg_tablectl_init(&mon->rrtctl, AM_EPG_EVT_RRT_DONE, AM_SI_ATSC_BASE_PID, AM_SI_TID_PSIP_RRT,
							 0xff, "RRT", 24, am_epg_rrt_done, 0, am_epg_proc_rrt_section);
	am_epg_tablectl_init(&mon->vctctl, AM_EPG_EVT_VCT_DONE, AM_SI_ATSC_BASE_PID, AM_SI_TID_PSIP_CVCT,
							 0xff, "VCT", 1, am_epg_vct_done, 0, am_epg_proc_vct_section);
	am_epg_tablectl_init(&mon->ceactl, AM_EPG_EVT_PSIP_CEA_DONE, AM_SI_ATSC_BASE_PID, AM_SI_TID_PSIP_CEA,
							 0xff, "CEA", 1, am_epg_cea_done, 0, NULL);
	for (i=0; i<(int)AM_ARRAY_SIZE(mon->psip_eitctl); i++)
	{
		snprintf(name, sizeof(name), "ATSC EIT%d", i);
		am_epg_tablectl_init(&mon->psip_eitctl[i], AM_EPG_EVT_PSIP_EIT_DONE, 0x1fff, AM_SI_TID_PSIP_EIT,
							 0xff, name, MAX_EIT_SUBTABLE_CNT, am_epg_psip_eit_done, PSIP_EIT_REPEAT_DISTANCE, am_epg_proc_psip_eit_section);
	}
	for (i=0; i<(int)AM_ARRAY_SIZE(mon->psip_ettctl); i++)
	{		
		snprintf(name, sizeof(name), "ATSC ETT%d", i);        
		am_epg_tablectl_init(&mon->psip_ettctl[i], AM_EPG_EVT_PSIP_ETT_DONE, 0x1fff, AM_SI_TID_PSIP_ETT,
							 0xff, name, MAX_EIT_SUBTABLE_CNT, am_epg_psip_ett_done, PSIP_ETT_REPEAT_DISTANCE, am_epg_proc_psip_ett_section);
	}
}

/**\brief 按照当前模式重新设置监控*/
static void am_epg_set_mode(AM_EPG_Monitor_t *mon, AM_Bool_t reset)
{
	int i;
	
	AM_DEBUG(1, "EPG Setmode 0x%x, reset:%d", mon->mode, reset);

	if (mon->mode & AM_EPG_SCAN_PSIP_EIT_VERSION_CHANGE && (!(mon->mode & AM_EPG_SCAN_PSIP_EIT))) {
		for (i=0; i<mon->psip_eit_count; i++)
		{
			if (mon->psip_eitctl[i].fid != -1)
				am_epg_free_filter(mon, &mon->psip_eitctl[i].fid);
		}
		for (i=0; i<mon->psip_ett_count; i++)
		{
			if (mon->psip_ettctl[i].fid != -1)
				am_epg_free_filter(mon, &mon->psip_ettctl[i].fid);
		}
		return;
	}else if (mon->mode & AM_EPG_SCAN_PSIP_EIT_VERSION_CHANGE && mon->mode & AM_EPG_SCAN_PSIP_EIT) {
		for (i=0; i<mon->psip_eit_count; i++)
		{
			if (mon->psip_eitctl[i].fid == -1 && mon->psip_eitctl[i].pid < 0x1fff) {
				am_epg_request_section(mon, &mon->psip_eitctl[i]);
                if (i >= mon->parallel_psip_eit_count)
				     break;
			}
		}
		return;
	}

	SET_MODE(pat, patctl, AM_EPG_SCAN_PAT, reset);
	if (mon->pmtctl.fid != -1)
	{
		SET_MODE(pmt, pmtctl, AM_EPG_SCAN_PMT, reset);
	}
	SET_MODE(tot, totctl, AM_EPG_SCAN_TDT, reset);
	SET_MODE(eit, eit4ectl, AM_EPG_SCAN_EIT_PF_ACT, reset);
	SET_MODE(eit, eit4fctl, AM_EPG_SCAN_EIT_PF_OTH, reset);
	SET_MODE(eit, eit50ctl, AM_EPG_SCAN_EIT_SCHE_ACT, reset);
	SET_MODE(eit, eit51ctl, AM_EPG_SCAN_EIT_SCHE_ACT, reset);
	SET_MODE(eit, eit60ctl, AM_EPG_SCAN_EIT_SCHE_OTH, reset);
	SET_MODE(eit, eit61ctl, AM_EPG_SCAN_EIT_SCHE_OTH, reset);
	SET_MODE(cat, catctl, AM_EPG_SCAN_CAT, reset);
	SET_MODE(sdt, sdtctl, AM_EPG_SCAN_SDT, reset);
	SET_MODE(nit, nitctl, AM_EPG_SCAN_NIT, reset);

	/*For ATSC*/
	SET_MODE(atsc_stt, sttctl, AM_EPG_SCAN_STT, reset);
	SET_MODE(atsc_mgt, mgtctl, AM_EPG_SCAN_MGT, reset);
	SET_MODE(rrt, rrtctl, AM_EPG_SCAN_RRT, reset);
	SET_MODE(atsc_cea, ceactl, AM_EPG_SCAN_PSIP_CEA, reset);
	if (mon->vctctl.fid != -1)
	{
		SET_MODE(atsc_vct, vctctl, AM_EPG_SCAN_VCT, reset);
	}
	for (i=0; i<mon->psip_eit_count; i++)
	{
		int idx = 0;
		if (mon->psip_eitctl[i].subs && mon->psip_eitctl[i].subs > 0 && mon->mode == 0) {
			//AM_DEBUG(1, "--------->>>>>>> psip eit to reset all subctl if any ------>>>>>>>");
			for (idx = 0; idx < mon->psip_eitctl[i].subs; idx++) {
				mon->psip_eitctl[i].subctl[idx].ver = 0xFF;
				//AM_DEBUG(1, "the %d pid:0x%x, fid:0x%x", i, mon->psip_eitctl[i].subctl, mon->psip_eitctl[i].fid);
			}
		}
		if (mon->psip_eitctl[i].fid != -1)
			SET_MODE(atsc_eit, psip_eitctl[i], AM_EPG_SCAN_PSIP_EIT, reset);
		else
			mon->psip_eit_request = 1;
	}
	for (i=0; i<mon->psip_ett_count; i++)
	{
		if (mon->psip_ettctl[i].subs && mon->psip_ettctl[i].subs > 0 && mon->mode == 0)
		{
			//AM_DEBUG(1, "--------->>>>>>> psip ett to reset all subctl if any ------>>>>>>>");
			int idx = 0;
			for (idx = 0; idx < mon->psip_ettctl[i].subs; idx++)
			{
				mon->psip_ettctl[i].subctl[idx].ver = 0xFF;
				//AM_DEBUG(1, "the %d pid:0x%x, fid:0x%x", i, mon->psip_eitctl[i].subctl, mon->psip_eitctl[i].fid);
			}
		}
		if (mon->psip_ettctl[i].fid != -1)
			SET_MODE(atsc_ett, psip_ettctl[i], AM_EPG_SCAN_PSIP_ETT, reset);
		else
			mon->psip_ett_request = 1;
	}
#if 0
	if (HAS_MODE(mon->mode, AM_EPG_SCAN_PSIP_EIT) && mon->psip_eit_request) {
		if (curr_time.tdt_utc_time)//assue stt done
			SET_MODE_FORCE(atsc_mgt, mgtctl, AM_EPG_SCAN_MGT, AM_TRUE);
		else//request stt first for the gps offset, then eit
			SET_MODE_FORCE(atsc_stt, sttctl, AM_EPG_SCAN_STT, AM_TRUE);
	}
#endif
}

/**\brief 按照当前模式重置所有表监控*/
static void am_epg_reset(AM_EPG_Monitor_t *mon)
{
	/*重新打开所有监控*/
	am_epg_set_mode(mon, AM_TRUE);
}

/**\brief 前端回调函数*/
static void am_epg_fend_callback(long dev_no, int event_type, void *param, void *user_data)
{
	struct dvb_frontend_event *evt = (struct dvb_frontend_event*)param;
	AM_EPG_Monitor_t *mon = (AM_EPG_Monitor_t*)user_data;

	UNUSED(dev_no);
	UNUSED(event_type);

	if (!mon || !evt || (evt->status == 0))
		return;

	pthread_mutex_lock(&mon->lock);
	mon->fe_evt = *evt;
	mon->evt_flag |= AM_EPG_EVT_FEND;
	pthread_cond_signal(&mon->cond);
	pthread_mutex_unlock(&mon->lock);
}

/**\brief 处理前端事件*/
static void am_epg_solve_fend_evt(AM_EPG_Monitor_t *mon)
{
	UNUSED(mon);
}

/**\brief 检查EPG更新通知*/
static void am_epg_check_update(AM_EPG_Monitor_t *mon)
{
	/*触发通知事件*/
	if (mon->eit_has_data)
	{
		SIGNAL_EVENT(AM_EPG_EVT_UPDATE_EVENTS, NULL);
		mon->eit_has_data = AM_FALSE;
	}
	AM_TIME_GetClock(&mon->new_eit_check_time);
}

/**\brief 计算下一等待超时时间*/
static void am_epg_get_next_ms(AM_EPG_Monitor_t *mon, int *ms)
{
	int min = 0, now;
	
#define REFRESH_CHECK(t, table, m, d)\
	AM_MACRO_BEGIN\
		if ((mon->mode & (m)) && mon->table##ctl.check_time != 0)\
		{\
			if ((mon->table##ctl.check_time + (d)) - now<= 0)\
			{\
				AM_DEBUG(2, "Refreshing %s...", mon->table##ctl.tname);\
				mon->table##ctl.check_time = 0;\
				/*AM_DMX_StartFilter(mon->dmx_dev, mon->table##ctl.fid);*/\
				SET_MODE(t, table##ctl, m, AM_FALSE);\
			}\
			else\
			{\
				if (min == 0)\
					min = mon->table##ctl.check_time + (d) - now;\
				else\
					min = AM_MIN(min, mon->table##ctl.check_time + (d) - now);\
			}\
		}\
	AM_MACRO_END

#define EVENT_CHECK(check, dis, func)\
	AM_MACRO_BEGIN\
		if (check) {\
			if (now - (check+dis) >= 0){\
				func(mon);\
				if (min == 0) \
					min = dis;\
				else\
					min = AM_MIN(min, dis);\
			} else if (min == 0){\
				min = (check+dis) - now;\
			} else {\
				min = AM_MIN(min, (check+dis) - now);\
			}\
		}\
	AM_MACRO_END

	AM_TIME_GetClock(&now);

	/*自动更新检查*/
	REFRESH_CHECK(tot, tot, AM_EPG_SCAN_TDT, TDT_CHECK_DISTANCE);
	if ((mon->mon_service == -1) && (mon->mon_service_sid == -1))
	{
		REFRESH_CHECK(eit, eit4e, AM_EPG_SCAN_EIT_PF_ACT, mon->eitpf_check_time);
	}
	REFRESH_CHECK(eit, eit4f, AM_EPG_SCAN_EIT_PF_OTH, mon->eitpf_check_time);
	REFRESH_CHECK(eit, eit50, AM_EPG_SCAN_EIT_SCHE_ACT, mon->eitsche_check_time);
	REFRESH_CHECK(eit, eit51, AM_EPG_SCAN_EIT_SCHE_ACT, mon->eitsche_check_time);
	REFRESH_CHECK(eit, eit60, AM_EPG_SCAN_EIT_SCHE_OTH, mon->eitsche_check_time);
	REFRESH_CHECK(eit, eit61, AM_EPG_SCAN_EIT_SCHE_OTH, mon->eitsche_check_time);
	REFRESH_CHECK(atsc_stt, stt, AM_EPG_SCAN_STT, STT_CHECK_DISTANCE);

	if (!mon->evt_cb) {
		/*EIT数据更新通知检查*/
		EVENT_CHECK(mon->new_eit_check_time, NEW_EIT_CHECK_DISTANCE, am_epg_check_update);
	}

	AM_DEBUG(5, "Next timeout is %d ms", min);
	*ms = min;
}

/**\brief MON线程*/
static void *am_epg_thread(void *para)
{
	AM_EPG_Monitor_t *mon = (AM_EPG_Monitor_t*)para;
	AM_Bool_t go = AM_TRUE;
	int distance, ret, evt_flag, i;
	struct timespec rt;
	int dbopen = 0;
	
	/*Reset the TDT time while epg start*/
	pthread_mutex_lock(&time_lock);
	memset(&curr_time, 0, sizeof(curr_time));
	AM_TIME_GetClock(&curr_time.tdt_sys_time);
	AM_DEBUG(1, "Start EPG Monitor thread now, curr_time.tdt_sys_time %d", curr_time.tdt_sys_time);
	pthread_mutex_unlock(&time_lock);

	/*控制数据初始化*/
	am_epg_tablectl_data_init(mon);
	
	/*注册前端事件*/
	AM_EVT_Subscribe(mon->fend_dev, AM_FEND_EVT_STATUS_CHANGED, am_epg_fend_callback, (void*)mon);

	AM_TIME_GetClock(&mon->sub_check_time);
			 			
	pthread_mutex_lock(&mon->lock);
	while (go)
	{
		am_epg_get_next_ms(mon, &distance);
		
		/*等待事件*/
		ret = 0;
		if(mon->evt_flag == 0)
		{
			if (distance == 0)
			{
				ret = pthread_cond_wait(&mon->cond, &mon->lock);
			}
			else
			{
				AM_TIME_GetTimeSpecTimeout(distance, &rt);
				ret = pthread_cond_timedwait(&mon->cond, &mon->lock, &rt);
			}
		}

		if (ret != ETIMEDOUT)
		{
handle_events:
			evt_flag = mon->evt_flag;
			AM_DEBUG(3, "Evt flag 0x%x", mon->evt_flag);

			/*前端事件*/
			if (evt_flag & AM_EPG_EVT_FEND)
				am_epg_solve_fend_evt(mon);
				
			/*PAT表收齐事件*/
			if (evt_flag & AM_EPG_EVT_PAT_DONE)
				mon->patctl.done(mon);
			/*PMT表收齐事件*/
			if (evt_flag & AM_EPG_EVT_PMT_DONE)
				mon->pmtctl.done(mon);
			/*CAT表收齐事件*/
			if (evt_flag & AM_EPG_EVT_CAT_DONE)
				mon->catctl.done(mon);
			/*SDT表收齐事件*/
			if (evt_flag & AM_EPG_EVT_SDT_DONE)
				mon->sdtctl.done(mon);
			/*NIT表收齐事件*/
			if (evt_flag & AM_EPG_EVT_NIT_DONE)
				mon->nitctl.done(mon);
			/*EIT表收齐事件*/
			if (evt_flag & AM_EPG_EVT_EIT4E_DONE)
				mon->eit4ectl.done(mon);
			/*EIT表收齐事件*/
			if (evt_flag & AM_EPG_EVT_EIT4F_DONE)
				mon->eit4fctl.done(mon);
			/*EIT表收齐事件*/
			if (evt_flag & AM_EPG_EVT_EIT50_DONE)
				mon->eit50ctl.done(mon);
			/*EIT表收齐事件*/
			if (evt_flag & AM_EPG_EVT_EIT51_DONE)
				mon->eit51ctl.done(mon);
			/*EIT表收齐事件*/
			if (evt_flag & AM_EPG_EVT_EIT60_DONE)
				mon->eit60ctl.done(mon);
			/*EIT表收齐事件*/
			if (evt_flag & AM_EPG_EVT_EIT61_DONE)
				mon->eit61ctl.done(mon);
			/*TDT表收齐事件*/
			if (evt_flag & AM_EPG_EVT_TDT_DONE)
				mon->totctl.done(mon);
			/*STT表收齐事件*/
			if (evt_flag & AM_EPG_EVT_STT_DONE)
				mon->sttctl.done(mon);
			/*RRT表收齐事件*/
			if (evt_flag & AM_EPG_EVT_RRT_DONE)
				mon->rrtctl.done(mon);
			/*MGT表收齐事件*/
			if (evt_flag & AM_EPG_EVT_MGT_DONE)
				mon->mgtctl.done(mon);
			/*VCT表收齐事件*/
			if (evt_flag & AM_EPG_EVT_VCT_DONE)
				mon->vctctl.done(mon);
			/*cea表收齐事件*/
			if (evt_flag & AM_EPG_EVT_PSIP_CEA_DONE)
				mon->ceactl.done(mon);
			/*PSIP EIT表收齐事件*/
			if (evt_flag & AM_EPG_EVT_PSIP_EIT_DONE)
				mon->psip_eitctl[0].done(mon);
			/*PSIP ETT表收齐事件*/
			if (evt_flag & AM_EPG_EVT_PSIP_ETT_DONE)
				mon->psip_ettctl[0].done(mon);
			/*设置监控模式事件*/
			if (evt_flag & AM_EPG_EVT_SET_MODE)
				am_epg_set_mode(mon, AM_FALSE);
			/*设置EIT PF自动更新间隔*/
			if (evt_flag & AM_EPG_EVT_SET_EITPF_CHECK_TIME)
			{
				/*如果应用将间隔设置为0，则立即启动过滤器，否则等待下次计算超时时启动*/
				if (mon->eitpf_check_time == 0)
				{
					if ((mon->mon_service == -1) && (mon->mon_service_sid == -1))
						SET_MODE(eit, eit4ectl, AM_EPG_SCAN_EIT_PF_ACT, AM_FALSE);
					SET_MODE(eit, eit4fctl, AM_EPG_SCAN_EIT_PF_OTH, AM_FALSE);
				}
			}
			/*设置EIT Schedule自动更新间隔*/
			if (evt_flag & AM_EPG_EVT_SET_EITSCHE_CHECK_TIME)
			{
				/*如果应用将间隔设置为0，则立即启动过滤器，否则等待下次计算超时时启动*/
				if (mon->eitsche_check_time == 0)
				{
					SET_MODE(eit, eit50ctl, AM_EPG_SCAN_EIT_SCHE_ACT, AM_FALSE);
					SET_MODE(eit, eit51ctl, AM_EPG_SCAN_EIT_SCHE_ACT, AM_FALSE);
					SET_MODE(eit, eit60ctl, AM_EPG_SCAN_EIT_SCHE_ACT, AM_FALSE);
					SET_MODE(eit, eit61ctl, AM_EPG_SCAN_EIT_SCHE_ACT, AM_FALSE);
				}
			}

			/*设置当前监控的service*/
			if (evt_flag & AM_EPG_EVT_SET_MON_SRV)
			{
				if (mon->mon_service != -1) {
					int db_ts_id = am_epg_get_current_db_ts_id(mon);
					if (mon->curr_ts != db_ts_id)
					{
						AM_DEBUG(1, "TS changed, %d -> %d", mon->curr_ts, db_ts_id);
						mon->curr_ts = db_ts_id;
					}
				}
			}

			/*退出事件*/
			if (evt_flag & AM_EPG_EVT_QUIT)
			{
				go = AM_FALSE;
				continue;
			}
			
			/*在调用am_epg_free_filter时可能会产生新事件*/
			mon->evt_flag &= ~evt_flag;
			if (mon->evt_flag)
			{
				goto handle_events;
			}
			/* To add the new event */
			pthread_cond_signal(&mon->cond);
		}
	}
	
	/*线程退出，释放资源*/
	mon->mode = 0;
	am_epg_set_mode(mon, AM_FALSE);
	pthread_mutex_unlock(&mon->lock);
	
	/*等待DMX回调执行完毕*/
	AM_DMX_Sync(mon->dmx_dev);

	pthread_mutex_lock(&mon->lock);
	AM_SI_Destroy(mon->hsi);

	am_epg_tablectl_deinit(&mon->patctl);
	am_epg_tablectl_deinit(&mon->pmtctl);
	am_epg_tablectl_deinit(&mon->catctl);
	am_epg_tablectl_deinit(&mon->sdtctl);
	am_epg_tablectl_deinit(&mon->nitctl);
	am_epg_tablectl_deinit(&mon->totctl);
	am_epg_tablectl_deinit(&mon->eit4ectl);
	am_epg_tablectl_deinit(&mon->eit4fctl);
	am_epg_tablectl_deinit(&mon->eit50ctl);
	am_epg_tablectl_deinit(&mon->eit51ctl);
	am_epg_tablectl_deinit(&mon->eit60ctl);
	am_epg_tablectl_deinit(&mon->eit61ctl);
	am_epg_tablectl_deinit(&mon->sttctl);
	am_epg_tablectl_deinit(&mon->mgtctl);
	am_epg_tablectl_deinit(&mon->rrtctl);
	am_epg_tablectl_deinit(&mon->vctctl);
	am_epg_tablectl_deinit(&mon->ceactl);
	for (i=0; i<mon->psip_eit_count; i++)
	{
		am_epg_tablectl_deinit(&mon->psip_eitctl[i]);
	}
	for (i=0; i<mon->psip_ett_count; i++)
	{
		am_epg_tablectl_deinit(&mon->psip_ettctl[i]);
	}
	
	pthread_mutex_unlock(&mon->lock);
	/*取消前端事件*/
	AM_EVT_Unsubscribe(mon->fend_dev, AM_FEND_EVT_STATUS_CHANGED, am_epg_fend_callback, (void*)mon);
	pthread_mutex_destroy(&mon->lock);
	pthread_cond_destroy(&mon->cond);

	return NULL;
}

/****************************************************************************
 * API functions
 ***************************************************************************/

/**\brief 在指定源上创建一个EPG监控
 * \param [in] para 创建参数
 * \param [out] handle 返回句柄
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_epg.h)
 */
AM_ErrorCode_t AM_EPG_Create(AM_EPG_CreatePara_t *para, AM_EPG_Handle_t *handle)
{
	AM_EPG_Monitor_t *mon;
	int rc;
	pthread_mutexattr_t mta;

	assert(handle && para);

	*handle = 0;

	mon = (AM_EPG_Monitor_t*)malloc(sizeof(AM_EPG_Monitor_t));
	if (! mon)
	{
		AM_DEBUG(1, "Create epg error, no enough memory");
		return AM_EPG_ERR_NO_MEM;
	}
	/*数据初始化*/
	memset(mon, 0, sizeof(AM_EPG_Monitor_t));
	mon->src = para->source;
	mon->fend_dev = para->fend_dev;
	mon->dmx_dev = para->dmx_dev;
	snprintf(mon->text_langs, sizeof(mon->text_langs), "%s", para->text_langs);
	mon->eitpf_check_time = EITPF_CHECK_DISTANCE;
	mon->eitsche_check_time = EITSCHE_CHECK_DISTANCE;
	mon->mon_service = -1;
	mon->curr_ts = -1;
	mon->mon_service_sid = -1;
	mon->curr_ts_tsid = -1;
	mon->psip_eit_count = (int)AM_ARRAY_SIZE(mon->psip_eitctl);
	mon->psip_ett_count = (int)AM_ARRAY_SIZE(mon->psip_ettctl);
#ifdef ANDROID
	mon->parallel_psip_eit_count = property_get_int32(PARALLEL_PSIP_EIT_CNT_PROP, 4);
	mon->parallel_psip_ett_count = property_get_int32(PARALLEL_PSIP_ETT_CNT_PROP, 4);
#else
	mon->parallel_psip_eit_count = 4;
	mon->parallel_psip_ett_count = 4;
#endif

	AM_TIME_GetClock(&mon->new_eit_check_time);
	mon->eit_has_data = AM_FALSE;

	if (AM_SI_Create(&mon->hsi) != AM_SUCCESS)
	{
		AM_DEBUG(1, "Create epg error, cannot create si decoder");
		free(mon);
		return AM_EPG_ERR_CANNOT_CREATE_SI;
	}

	pthread_mutexattr_init(&mta);
	pthread_mutexattr_settype(&mta, PTHREAD_MUTEX_RECURSIVE_NP);
	pthread_mutex_init(&mon->lock, &mta);
	pthread_cond_init(&mon->cond, NULL);
	pthread_mutexattr_destroy(&mta);
	/*创建监控线程*/
	rc = pthread_create(&mon->thread, NULL, am_epg_thread, (void*)mon);
	if(rc)
	{
		AM_DEBUG(1, "%s", strerror(rc));
		pthread_mutex_destroy(&mon->lock);
		pthread_cond_destroy(&mon->cond);
		AM_SI_Destroy(mon->hsi);
		free(mon);
		return AM_EPG_ERR_CANNOT_CREATE_THREAD;
	}

	*handle = mon;

	return AM_SUCCESS;
}

/**\brief 销毀一个EPG监控
 * \param handle 句柄
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_epg.h)
 */
AM_ErrorCode_t AM_EPG_Destroy(AM_EPG_Handle_t handle)
{
	AM_EPG_Monitor_t *mon = (AM_EPG_Monitor_t *)handle;
	pthread_t t;
	
	assert(mon);

	pthread_mutex_lock(&mon->lock);
	/*等待搜索线程退出*/
	mon->evt_flag |= AM_EPG_EVT_QUIT;
	t = mon->thread;
	pthread_mutex_unlock(&mon->lock);
	pthread_cond_signal(&mon->cond);

	if (t != pthread_self())
		pthread_join(t, NULL);

	free(mon);

	return AM_SUCCESS;
}

/**\brief 设置EPG监控模式
 * \param handle 句柄
 * \param op	修改操作，见AM_EPG_ModeOp
 * \param mode 监控模式，见AM_EPG_Mode
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_epg.h)
 */
AM_ErrorCode_t AM_EPG_ChangeMode(AM_EPG_Handle_t handle, int op, int mode)
{
	AM_EPG_Monitor_t *mon = (AM_EPG_Monitor_t*)handle;

	assert(mon);

	if (op != AM_EPG_MODE_OP_ADD && op != AM_EPG_MODE_OP_REMOVE && op != AM_EPG_MODE_OP_SET)
	{
		AM_DEBUG(1, "Invalid EPG Mode");
		return AM_EPG_ERR_INVALID_PARAM;
	}
	
	pthread_mutex_lock(&mon->lock);
	am_epg_wait_events_done(mon);
	if ((op == AM_EPG_MODE_OP_ADD && (mon->mode&mode) != mode) ||
		(op == AM_EPG_MODE_OP_REMOVE && (mon->mode&mode) != 0) ||
		(op == AM_EPG_MODE_OP_SET && mon->mode != mode))
	{
		AM_DEBUG(1, "Change EPG mode, 0x%x -> [%d]0x%x", mon->mode, op, mode);
		mon->evt_flag |= AM_EPG_EVT_SET_MODE;
		if (op == AM_EPG_MODE_OP_ADD)
			mon->mode |= mode;
		else if (op == AM_EPG_MODE_OP_REMOVE)
			mon->mode &= ~mode;
		else
			mon->mode = mode;
		
		pthread_cond_signal(&mon->cond);
	}
	else
	{
		AM_DEBUG(1, "No need to change EPG mode, 0x%x -> [%d]0x%x", mon->mode, op, mode);
	}
	
	pthread_mutex_unlock(&mon->lock);

	return AM_SUCCESS;
}

/**\brief 设置当前监控的service，监控其PMT和EIT actual pf
 * \param handle 句柄
 * \param service_id	需要监控的service的数据库索引
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_epg.h)
 */
AM_ErrorCode_t AM_EPG_MonitorService(AM_EPG_Handle_t handle, int db_srv_id)
{
	AM_EPG_Monitor_t *mon = (AM_EPG_Monitor_t*)handle;

	assert(mon);

	pthread_mutex_lock(&mon->lock);
	am_epg_wait_events_done(mon);
	mon->evt_flag |= AM_EPG_EVT_SET_MON_SRV;
	mon->mon_service = db_srv_id;
	pthread_cond_signal(&mon->cond);
	pthread_mutex_unlock(&mon->lock);

	return AM_SUCCESS;
}

/**\brief 设置EPG PF 自动更新间隔
 * \param handle 句柄
 * \param distance 检查间隔,ms单位
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_epg.h)
 */
AM_ErrorCode_t AM_EPG_SetEITPFCheckDistance(AM_EPG_Handle_t handle, int distance)
{
	AM_EPG_Monitor_t *mon = (AM_EPG_Monitor_t*)handle;

	assert(mon);

	if (distance < 0)
	{
		AM_DEBUG(1, "Invalid check distance");
		return AM_EPG_ERR_INVALID_PARAM;
	}	
	
	pthread_mutex_lock(&mon->lock);
	am_epg_wait_events_done(mon);
	mon->evt_flag |= AM_EPG_EVT_SET_EITPF_CHECK_TIME;
	mon->eitpf_check_time = distance;
	pthread_cond_signal(&mon->cond);
	pthread_mutex_unlock(&mon->lock);

	return AM_SUCCESS;
}

/**\brief 设置EPG Schedule 自动更新间隔
 * \param handle 句柄
 * \param distance 检查间隔,ms单位, 为0时将一直更新
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_epg.h)
 */
AM_ErrorCode_t AM_EPG_SetEITScheCheckDistance(AM_EPG_Handle_t handle, int distance)
{
	AM_EPG_Monitor_t *mon = (AM_EPG_Monitor_t*)handle;

	assert(mon);

	if (distance < 0)
	{
		AM_DEBUG(1, "Invalid check distance");
		return AM_EPG_ERR_INVALID_PARAM;
	}	
	
	pthread_mutex_lock(&mon->lock);
	am_epg_wait_events_done(mon);
	mon->evt_flag |= AM_EPG_EVT_SET_EITSCHE_CHECK_TIME;
	mon->eitsche_check_time = distance;
	pthread_cond_signal(&mon->cond);
	pthread_mutex_unlock(&mon->lock);

	return AM_SUCCESS;
}

/**\brief 获得当前UTC时间
 * \param [out] utc_time UTC时间,单位为秒
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_epg.h)
 */
AM_ErrorCode_t AM_EPG_GetUTCTime(int *utc_time)
{
#ifdef USE_TDT_TIME
	int now;
	
	assert(utc_time);

	AM_TIME_GetClock(&now);
	
	pthread_mutex_lock(&time_lock);
	*utc_time = curr_time.tdt_utc_time + (now - curr_time.tdt_sys_time)/1000;
	pthread_mutex_unlock(&time_lock);
#else
	*utc_time = (int)time(NULL);
#endif

	return AM_SUCCESS;
}

/**\brief 计算本地时间
 * \param utc_time UTC时间，单位为秒
 * \param [out] local_time 本地时间,单位为秒
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_epg.h)
 */
AM_ErrorCode_t AM_EPG_UTCToLocal(int utc_time, int *local_time)
{
#ifdef USE_TDT_TIME
	int now;
	
	assert(local_time);

	AM_TIME_GetClock(&now);
	
	pthread_mutex_lock(&time_lock);
	*local_time = utc_time + curr_time.offset;
	pthread_mutex_unlock(&time_lock);
#else
	time_t utc, local;
	struct tm *gm;

	time(&utc);
	gm = gmtime(&utc);
	local = mktime(gm);
	local = utc_time + (utc - local);

	*local_time = (int)local;
#endif
	return AM_SUCCESS;
}

/**\brief 计算UTC时间
 * \param local_time 本地时间,单位为秒
 * \param [out] utc_time UTC时间，单位为秒
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_epg.h)
 */
AM_ErrorCode_t AM_EPG_LocalToUTC(int local_time, int *utc_time)
{
#ifdef USE_TDT_TIME
	int now;
	
	assert(local_time);

	AM_TIME_GetClock(&now);
	
	pthread_mutex_lock(&time_lock);
	*utc_time = local_time - curr_time.offset;
	pthread_mutex_unlock(&time_lock);
#else
	time_t local = (time_t)local_time;
	struct tm *gm;

	gm = gmtime(&local);
	*utc_time = (int)mktime(gm);
#endif
	return AM_SUCCESS;
}

/**\brief 设置时区偏移值
 * \param offset 偏移值,单位为秒
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_epg.h)
 */
AM_ErrorCode_t AM_EPG_SetLocalOffset(int offset)
{
#ifdef USE_TDT_TIME
	pthread_mutex_lock(&time_lock);
	curr_time.offset = offset;
	pthread_mutex_unlock(&time_lock);
#endif
	return AM_SUCCESS;
}

/**\brief 设置用户数据
 * \param handle EPG句柄
 * \param [in] user_data 用户数据
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_epg.h)
 */
AM_ErrorCode_t AM_EPG_SetUserData(AM_EPG_Handle_t handle, void *user_data)
{
	AM_EPG_Monitor_t *mon = (AM_EPG_Monitor_t*)handle;

	if (mon)
	{
		pthread_mutex_lock(&mon->lock);
		mon->user_data = user_data;
		pthread_mutex_unlock(&mon->lock);
	}

	return AM_SUCCESS;
}

/**\brief 取得用户数据
 * \param handle Scan句柄
 * \param [in] user_data 用户数据
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_epg.h)
 */
AM_ErrorCode_t AM_EPG_GetUserData(AM_EPG_Handle_t handle, void **user_data)
{
	AM_EPG_Monitor_t *mon = (AM_EPG_Monitor_t*)handle;

	assert(user_data);
	
	if (mon)
	{
		pthread_mutex_lock(&mon->lock);
		*user_data = mon->user_data;
		pthread_mutex_unlock(&mon->lock);
	}

	return AM_SUCCESS;
}

/**\brief 预约一个EPG事件，用于预约播放
 * \param handle EPG句柄
 * \param db_evt_id 事件的数据库索引
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_epg.h)
 */
AM_ErrorCode_t AM_EPG_SubscribeEvent(AM_EPG_Handle_t handle, int db_evt_id)
{
	AM_EPG_Monitor_t *mon = (AM_EPG_Monitor_t*)handle;
	AM_ErrorCode_t ret = AM_SUCCESS;

#ifdef ANDROID
	assert(mon && mon->hdb);
#endif

	sqlite3 *hdb;
	pthread_mutex_lock(&mon->lock);
	AM_DB_HANDLE_PREPARE(hdb);
	ret = am_epg_subscribe_event(hdb, db_evt_id);
	if (ret == AM_SUCCESS && ! mon->evt_flag)
	{
		/*进行一次EPG预约事件时间检查*/
		AM_TIME_GetClock(&mon->sub_check_time);
		mon->sub_check_time -= EPG_SUB_CHECK_TIME;
		pthread_cond_signal(&mon->cond);
	}
	pthread_mutex_unlock(&mon->lock);

	return ret;
}

/**\brief 取消预约一个EPG事件
 * \param handle EPG句柄
 * \param db_evt_id 事件的数据库索引
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_epg.h)
 */
AM_ErrorCode_t AM_EPG_UnsubscribeEvent(AM_EPG_Handle_t handle, int db_evt_id)
{
	AM_EPG_Monitor_t *mon = (AM_EPG_Monitor_t*)handle;
	AM_ErrorCode_t ret = AM_SUCCESS;

#ifdef ANDROID
	assert(mon && mon->hdb);
#endif

	sqlite3 *hdb;
	pthread_mutex_lock(&mon->lock);
	AM_DB_HANDLE_PREPARE(hdb);
	ret = am_epg_unsubscribe_event(hdb, db_evt_id);
	pthread_mutex_unlock(&mon->lock);

	return ret;
}

AM_ErrorCode_t AM_EPG_MonitorServiceByID(AM_EPG_Handle_t handle, int ts_id, int srv_id, AM_EPG_PMT_UpdateCB_t pmt_cb)
{
	AM_EPG_Monitor_t *mon = (AM_EPG_Monitor_t*)handle;

	assert(mon);

	pthread_mutex_lock(&mon->lock);
	am_epg_wait_events_done(mon);
	mon->evt_flag |= AM_EPG_EVT_SET_MON_SRV;
	mon->mon_service_sid = srv_id;
	mon->curr_ts_tsid = ts_id;
	mon->pmt_cb = pmt_cb;
	pthread_cond_signal(&mon->cond);
	pthread_mutex_unlock(&mon->lock);

	return AM_SUCCESS;
}

AM_ErrorCode_t AM_EPG_SetEventsCallback(AM_EPG_Handle_t handle, AM_EPG_Events_UpdateCB_t cb)
{
	AM_EPG_Monitor_t *mon = (AM_EPG_Monitor_t*)handle;

	assert(mon);

	pthread_mutex_lock(&mon->lock);
	mon->evt_cb = cb;
	pthread_mutex_unlock(&mon->lock);

	return AM_SUCCESS;
}

void AM_EPG_FreeEvents(int event_count, AM_EPG_Event_t *pevents)
{
	am_epg_eit_free_event_list(event_count, pevents);
}


AM_ErrorCode_t AM_EPG_SetTablesCallback(AM_EPG_Handle_t handle, int type, AM_EPG_TAB_UpdateCB_t cb, void * user_data)
{
	AM_EPG_Monitor_t *mon = (AM_EPG_Monitor_t*)handle;

	assert(mon);

	if (type < 0 || type >= AM_EPG_TAB_MAX)
		return AM_EPG_ERR_INVALID_PARAM;

	pthread_mutex_lock(&mon->lock);
	mon->tab_cbs[type].cb = cb;
	mon->tab_cbs[type].user_data = user_data;
	pthread_mutex_unlock(&mon->lock);

	return AM_SUCCESS;
}

AM_ErrorCode_t AM_EPG_DisableDefProc(AM_EPG_Handle_t handle, AM_Bool_t disable)
{
	AM_EPG_Monitor_t *mon = (AM_EPG_Monitor_t*)handle;

	assert(mon);

	pthread_mutex_lock(&mon->lock);
	mon->disable_def_proc = disable;
	pthread_mutex_unlock(&mon->lock);

	return AM_SUCCESS;
}

