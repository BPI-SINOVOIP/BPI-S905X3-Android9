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
/**\file am_tv.c
 * \brief TV模块源程序
 *
 * \author Xia Lei Peng <leipeng.xia@amlogic.com>
 * \date 2010-11-11: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 1

#include <assert.h>
#include <am_debug.h>
#include <am_mem.h>
#include <am_av.h>
#include <am_fend.h>
#include <am_db.h>
#include <am_tv.h>
#include "am_tv_internal.h"

#include "am_caman.h"
#include "am_ci.h"

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

#define TV_TRY_FINAL(_cond, _errmsg, _ret)\
	AM_MACRO_BEGIN\
		if (_cond)\
		{\
			AM_DEBUG(1, _errmsg);\
			ret = _ret;\
			goto final;\
		}\
	AM_MACRO_END

/****************************************************************************
 * static functions
 ***************************************************************************/


/**\brief 设置前端参数,返回AM_TRUE为设置了新参数，返回AM_FALSE为未设置 */
static AM_Bool_t am_tv_set_fend(AM_TV_Data_t *tv, struct dvb_frontend_parameters *para)
{
	struct dvb_frontend_parameters fparam;
	fe_status_t status;

	AM_FEND_GetPara(tv->fend_dev, &fparam);
	/*是否需要重新锁频*/
//	if (fparam.frequency == para->frequency)
//		return AM_FALSE;

	AM_DEBUG(1, "TV try lock %u", para->frequency);
	AM_FEND_SetPara(tv->fend_dev, para/*, &status*/);

	tv->status &= ~AM_TV_ST_LOCKED;
	tv->fend_para = *para;

	return AM_TRUE;
}

/**\brief 播放当前节目*/
static AM_ErrorCode_t am_tv_play(AM_TV_Data_t *tv)
{
	char selstr[128];
	AM_ErrorCode_t ret;
	int vid, aid, vfmt, afmt;
	int row = 1;
	char str_apids[256];
	char str_afmts[256];
	int sid;

	AM_DB_HANDLE_PREPARE(tv->hdb);
	
	if (tv->status & AM_TV_ST_PLAYING)
		return AM_SUCCESS;
		
	/*检查即将需要播放的频道是否有效*/
	if (tv->srv_dbid == -1)
	{
		AM_DEBUG(1, "TV: invalid channel play");
		return AM_TV_ERR_NO_CHANNEL;
	}

	/*从数据库获取该频道播放信息*/
	snprintf(selstr, sizeof(selstr), 
		"select vid_pid,aud_pids,vid_fmt,aud_fmts,service_id from srv_table where db_id='%d'", tv->srv_dbid);
	ret = AM_DB_Select(tv->hdb, selstr, &row, "%d,%s:255,%d,%s:255,%d", &vid, str_apids, &vfmt, str_afmts, &sid);
	if (ret != AM_SUCCESS || row == 0)
	{
		AM_DEBUG(1, "TV: cannot get play info for channel %d from dbase!", tv->chan_num);
		return ret;
	}

	sscanf(str_apids, "%d,", &aid);
	sscanf(str_afmts, "%d,", &afmt);
	
	AM_DEBUG(1, "TV Play(C%03d) vid(%d), aid[%s](%d), vfmt(%d), afmt[%s](%d) on av(%d)", 
				tv->chan_num, vid, str_apids, aid, vfmt, str_afmts, afmt ,tv->av_dev);
	AM_TRY(AM_AV_StartTS(tv->av_dev, vid, aid, vfmt, afmt));

	AM_CAMAN_startService(sid, "dummy");

	AM_CAMAN_startService(sid, "ci0");

	tv->status |= AM_TV_ST_PLAYING;
	
	return ret;
}

/**\brief 尝试播放当前节目，会比较前端参数*/
static AM_ErrorCode_t am_tv_try_play(AM_TV_Data_t *tv)
{
	char selstr[64];
	AM_ErrorCode_t ret;
	int ts_dbid;
	int freq, symb, mod, bw;
	int row = 1;
	struct dvb_frontend_parameters para;

	AM_DB_HANDLE_PREPARE(tv->hdb);

	if (tv->status & AM_TV_ST_PLAYING)
	{
		AM_DEBUG(1, "Channel %d is already played", tv->chan_num);
		return AM_SUCCESS;
	}
		
	/*检查即将需要播放的频道是否有效*/
	if (tv->srv_dbid == -1)
	{
		AM_DEBUG(1, "TV: invalid channel play");
		return AM_TV_ERR_NO_CHANNEL;
	}

	/*从数据库获取该频道播放信息*/
	snprintf(selstr, sizeof(selstr), "select db_ts_id from srv_table where db_id='%d'", tv->srv_dbid);
	ret = AM_DB_Select(tv->hdb, selstr, &row, "%d", &ts_dbid);
	if (ret != AM_SUCCESS || row == 0)
	{
		AM_DEBUG(1, "TV: cannot get play info for channel %d from dbase!", tv->chan_num);
		return ret;
	}
	
	/*从数据库获取该频道频点信息*/
	row = 1;	
	snprintf(selstr, sizeof(selstr), "select freq,symb,mod,bw from ts_table where db_id='%d'", ts_dbid);
	ret = AM_DB_Select(tv->hdb, selstr, &row, "%d,%d,%d,%d", &freq, &symb, &mod, &bw);
	if (ret != AM_SUCCESS || row == 0)
	{
		AM_DEBUG(1, "TV: cannot get fend param for channel %d from dbase!", tv->chan_num);
		return ret;
	}

#define DVBT 1

	para.frequency = freq;

#if DVBC
	para.u.qam.symbol_rate = symb;
	para.u.qam.modulation = mod;
#else
#if DVBT
	para.u.ofdm.bandwidth = bw;
#endif
#endif
	printf("freq:%d, bw:%d\n", freq, bw);
	/*是否需要更换前端参数*/
	if (! am_tv_set_fend(tv, &para))
	{
		/*直接播放*/
		if (! (tv->status & AM_TV_ST_WAIT_FEND))
			am_tv_play(tv);
	}
	else
	{
		/*等待前端状态后播放*/
		AM_DEBUG(2, "TV wait fend.");
		tv->status |= AM_TV_ST_WAIT_FEND;
	}
	

	return ret;
}

/**\brief 前端回调*/
static void am_tv_fend_callback(long dev_no, int event_type, void *param, void *user_data)
{
	struct dvb_frontend_event *evt = (struct dvb_frontend_event*)param;
	AM_TV_Data_t *tv = (AM_TV_Data_t*)user_data;

	if (!tv || !evt || (evt->status == 0))
		return;

	pthread_mutex_lock(&tv->lock);
	/*前端参数是否匹配*/
	if (evt->parameters.frequency == tv->fend_para.frequency)
	{
		AM_DEBUG(1, "TV:(%u) %s", tv->fend_para.frequency, 
				(evt->status&FE_HAS_LOCK) ? "Locked" : "Unlocked");

		if (evt->status & FE_HAS_LOCK)
			tv->status |= AM_TV_ST_LOCKED;
		else
			tv->status &= ~AM_TV_ST_LOCKED;

		tv->status &= ~AM_TV_ST_WAIT_FEND;
			
		/*播放节目*/
		if (! (tv->status & AM_TV_ST_PLAYING))
			am_tv_play(tv);

		
	}
	pthread_mutex_unlock(&tv->lock);
}


/**\brief 停止播放当前节目*/
static AM_ErrorCode_t am_tv_stop(AM_TV_Data_t *tv)
{
	char selstr[64];
	int row = 1;
	int sid;
	AM_ErrorCode_t ret;
	
	if (!(tv->status & AM_TV_ST_PLAYING))
		return AM_SUCCESS;

	tv->status &= ~AM_TV_ST_PLAYING;

	AM_DB_HANDLE_PREPARE(tv->hdb);

	snprintf(selstr, sizeof(selstr), "select service_id from srv_table where db_id='%d'", tv->srv_dbid);
	ret = AM_DB_Select(tv->hdb, selstr, &row, "%d", &sid);
	if (ret != AM_SUCCESS || row == 0)
	{
		AM_DEBUG(1, "TV: cannot get service_id from dbase!");
		return ret;
	}

	AM_CAMAN_stopService(sid);

	return AM_AV_StopTS(tv->av_dev);
}

/**\brief 获取频道, op <0 chan_num的上一个频道 ==0 chan_num频道; >0 chan_num的下一个频道*/
static AM_ErrorCode_t am_tv_get_channel(AM_TV_Data_t *tv, int op, int chan_num, int *chnum, int *srv_dbid)
{
	char selstr[256];
	int row = 1;
	int srv_type;

	AM_DB_HANDLE_PREPARE(tv->hdb);

	if (tv->status & AM_TV_ST_PLAY_TV)
		srv_type = 0x1;
	else if (tv->status & AM_TV_ST_PLAY_RADIO)
		srv_type = 0x2;
	else
		return AM_TV_ERR_NO_CHANNEL;

	if (op < 0)
	{
		snprintf(selstr, sizeof(selstr), 
		"select chan_num,db_id from srv_table where chan_num<'%d' \
		and service_type='%d' order by chan_num desc limit 1", 
		chan_num, srv_type);
	}
	else if (op > 0)
	{
		snprintf(selstr, sizeof(selstr), 
		"select chan_num,db_id from srv_table where chan_num>'%d' \
		and service_type='%d' order by chan_num limit 1", 
		chan_num, srv_type);
	}
	else
	{
		snprintf(selstr, sizeof(selstr), 
		"select chan_num,db_id from srv_table where chan_num='%d' and service_type='%d'", 
		chan_num, srv_type);
	}
	if (AM_DB_Select(tv->hdb, selstr, &row, "%d,%d", chnum, srv_dbid) != AM_SUCCESS)
	{
		AM_DEBUG(1, "Cannot get the specified channel from dbase");
		return AM_TV_ERR_NO_CHANNEL;
	}

	/*实现循环切台*/
	if (op != 0 && row == 0)
	{
		if (op < 0)
		{
			/*切换至频道号最大的频道上*/
			snprintf(selstr, sizeof(selstr), 
			"select chan_num,db_id from srv_table where chan_num>'%d' \
			and service_type='%d' order by chan_num desc limit 1", 
			chan_num, srv_type);
		}
		else
		{
			/*切换至频道号最小的频道上*/
			snprintf(selstr, sizeof(selstr), 
			"select chan_num,db_id from srv_table where chan_num<'%d' \
			and service_type='%d' order by chan_num limit 1", 
			chan_num, srv_type);
		}
		
		row = 1;

		if (AM_DB_Select(tv->hdb, selstr, &row, "%d,%d", chnum, srv_dbid) != AM_SUCCESS || row == 0)
			return AM_TV_ERR_NO_CHANNEL;
	}
	else if (op == 0 && row == 0)
	{
		AM_DEBUG(1, "Cannot get the specified channel %d from dbase", chan_num);
		return AM_TV_ERR_NO_CHANNEL;
	}

	return AM_SUCCESS;
}

/**\brief 播放频道 op <0 chan_num的上一个频道 ==0 chan_num频道; >0 chan_num的下一个频道*/
static AM_ErrorCode_t am_tv_play_channel(AM_TV_Data_t *tv, int op, int chan_num)
{
	int srv_dbid, cnum;

	/*获取下一个频道*/
	AM_TRY(am_tv_get_channel(tv, op, chan_num, &cnum, &srv_dbid));
	
	if (srv_dbid != tv->srv_dbid)
	{
		/*停止当前播放*/
		am_tv_stop(tv);
		
		/*设置当前频道*/
		tv->srv_dbid = srv_dbid;
		tv->chan_num = cnum;
		if (tv->status & AM_TV_ST_PLAY_TV)
			tv->db_tv = srv_dbid;
		else if (tv->status & AM_TV_ST_PLAY_RADIO)
			tv->db_radio = srv_dbid;
		else
			/*This must not happen*/;
	}

	/*尝试播放*/
	am_tv_try_play(tv);

	return AM_SUCCESS;
}

/****************************************************************************
 * API functions
 ***************************************************************************/

/**\brief 创建一个TV设备
 * \param fend_dev 前端设备
 * \param av_dev 与该TV关联的音视频解码设备
 * \param [in] hdb 数据库操作句柄
 * \param [out] handle 返回TV句柄
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_tv.h)
 */
AM_ErrorCode_t AM_TV_Create(int fend_dev, int av_dev, sqlite3 *hdb, AM_TV_Handle_t *handle)
{
	AM_TV_Data_t *tv;

	assert(handle && hdb);

	*handle = 0;
	/*分配内存*/
	tv = (AM_TV_Data_t*)malloc(sizeof(AM_TV_Data_t));
	if (! tv)
	{
		AM_DEBUG(1, "Cannot create tv, no enough memory");
		return AM_TV_ERR_NO_MEM;
	}

	/*数据初始化*/
	memset(tv, 0, sizeof(AM_TV_Data_t));
	tv->av_dev = av_dev;
	tv->fend_dev = fend_dev;
	tv->hdb = hdb;
	tv->chan_num = -1;
	tv->srv_dbid = -1;
	tv->db_tv = -1;
	tv->db_radio = -1;

	pthread_mutex_init(&tv->lock, NULL);

	/*注册前端事件*/
	AM_EVT_Subscribe(tv->fend_dev, AM_FEND_EVT_STATUS_CHANGED, am_tv_fend_callback, (void*)tv);
		
	*handle = tv;
	AM_DEBUG(1, "Create TV from AV(%d) ok!", av_dev);
	
	return AM_SUCCESS;
}

/**\brief 销毁一个TV设备
 * \param handle TV句柄
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_tv.h)
 */
AM_ErrorCode_t AM_TV_Destroy(AM_TV_Handle_t handle)
{
	AM_TV_Data_t *tv = (AM_TV_Data_t*)handle;

	if (tv)
	{
		pthread_mutex_lock(&tv->lock);
		/*反注册前端事件*/
		AM_EVT_Unsubscribe(tv->fend_dev, AM_FEND_EVT_STATUS_CHANGED, am_tv_fend_callback, (void*)tv);
		pthread_mutex_unlock(&tv->lock);

		pthread_mutex_destroy(&tv->lock);
		free(tv);
	}
		
	return AM_TV_ERR_INVALID_HANDLE;
}

/**\brief 播放下一个频道(频道+)
 * \param handle TV句柄
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_tv.h)
 */
AM_ErrorCode_t AM_TV_ChannelUp(AM_TV_Handle_t handle)
{
	AM_TV_Data_t *tv = (AM_TV_Data_t*)handle;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	assert(tv);

	pthread_mutex_lock(&tv->lock);
	/*当前频道是否有效*/
	TV_TRY_FINAL(tv->chan_num == -1, "Channel up error, currently no valid channel played", AM_TV_ERR_NO_CHANNEL);
	
	/*尝试播放下一个频道*/
	AM_TRY_FINAL(am_tv_play_channel(tv, 1, tv->chan_num));
		
final:
	pthread_mutex_unlock(&tv->lock);
	return ret;
}

/**\brief 播放上一个频道(频道-)
 * \param handle TV句柄
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_tv.h)
 */
AM_ErrorCode_t AM_TV_ChannelDown(AM_TV_Handle_t handle)
{
	AM_TV_Data_t *tv = (AM_TV_Data_t*)handle;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	assert(tv);

	pthread_mutex_lock(&tv->lock);
	/*当前频道是否有效*/
	TV_TRY_FINAL(tv->chan_num == -1, "Channel up error, currently no valid channel played", AM_TV_ERR_NO_CHANNEL);
	
	/*尝试播放上一个频道*/
	AM_TRY_FINAL(am_tv_play_channel(tv, -1, tv->chan_num));
		
final:
	pthread_mutex_unlock(&tv->lock);
	return ret;
}

/**\brief 播放当前节目类型的指定频道
 * \param handle TV句柄
 * \param chan_num 频道号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_tv.h)
 */
AM_ErrorCode_t AM_TV_PlayChannel(AM_TV_Handle_t handle, int chan_num)
{
	AM_TV_Data_t *tv = (AM_TV_Data_t*)handle;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	assert(tv);

	pthread_mutex_lock(&tv->lock);
	
	/*尝试播放指定频道*/
	AM_TRY_FINAL(am_tv_play_channel(tv, 0, chan_num));
	
final:
	pthread_mutex_unlock(&tv->lock);
	return ret;
}

/**\brief 播放当前停止的频道,如未播放任何频道，则播放一个频道号最小的频道
 * \param handle TV句柄
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_tv.h)
 */
AM_ErrorCode_t AM_TV_Play(AM_TV_Handle_t handle)
{
	AM_TV_Data_t *tv = (AM_TV_Data_t*)handle;
	AM_ErrorCode_t ret = AM_FAILURE;
	
	assert(tv);

	pthread_mutex_lock(&tv->lock);

	/*播放已停止的节目*/
	ret = am_tv_play_channel(tv, 0, tv->chan_num);
	if (ret != AM_SUCCESS)
	{
		/*尝试播放频道号最小的电视节目*/
		tv->status &= 0xFFFF;
		tv->status |= AM_TV_ST_PLAY_TV;
		if (am_tv_play_channel(tv, 1, 0) != AM_SUCCESS)
		{
			/*尝试播放频道号最小的广播节目*/
			tv->status &= 0xFFFF;
			tv->status |= AM_TV_ST_PLAY_RADIO;
			ret = am_tv_play_channel(tv, 1, 0);
			if (ret != AM_SUCCESS)
				tv->status &= 0xFFFF;
		}
		
	}
	pthread_mutex_unlock(&tv->lock);

	return ret;
}

/**\brief 停止当前播放
 * \param handle TV句柄
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_tv.h)
 */
AM_ErrorCode_t AM_TV_StopPlay(AM_TV_Handle_t handle)
{
	AM_TV_Data_t *tv = (AM_TV_Data_t*)handle;
	AM_ErrorCode_t ret;

	assert(tv);
	
	pthread_mutex_lock(&tv->lock);
	ret = am_tv_stop(tv);
	pthread_mutex_unlock(&tv->lock);

	return ret;
}


/**\brief 获取当前频道
 * \param handle TV句柄
 * \param [out] srv_dbid 当前频道的service在数据库中的唯一索引
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_tv.h)
 */
AM_ErrorCode_t AM_TV_GetCurrentChannel(AM_TV_Handle_t handle, int *srv_dbid)
{
	AM_TV_Data_t *tv = (AM_TV_Data_t*)handle;

	assert(tv);
	
	pthread_mutex_lock(&tv->lock);
	*srv_dbid = tv->srv_dbid;
	pthread_mutex_unlock(&tv->lock);

	return AM_SUCCESS;
}


