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
 * \brief 音视频解码模块
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-08-06: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 5

#include <am_debug.h>
#include <am_vout.h>
#include <am_mem.h>
#include "am_av_internal.h"
#include "../am_adp_internal.h"
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

#define AV_DEV_COUNT      (1)

/****************************************************************************
 * Static data
 ***************************************************************************/

#ifdef EMU_AV
extern const AM_AV_Driver_t emu_av_drv;
#else
extern const AM_AV_Driver_t aml_av_drv;
#endif

static AM_AV_Device_t av_devices[AV_DEV_COUNT] =
{
#ifdef EMU_AV
{
.drv = &emu_av_drv
}
#else
{
.drv = &aml_av_drv
}
#endif
};

/****************************************************************************
 * Static functions
 ***************************************************************************/
 
/**\brief 根据设备号取得设备结构指针*/
static AM_INLINE AM_ErrorCode_t av_get_dev(int dev_no, AM_AV_Device_t **dev)
{
	if ((dev_no < 0) || (dev_no >= AV_DEV_COUNT))
	{
		AM_DEBUG(1, "invalid AV device number %d, must in(%d~%d)", dev_no, 0, AV_DEV_COUNT-1);
		return AM_AV_ERR_INVALID_DEV_NO;
	}

	*dev = &av_devices[dev_no];
	return AM_SUCCESS;
}

/**\brief 根据设备号取得设备结构并检查设备是否已经打开*/
static AM_INLINE AM_ErrorCode_t av_get_openned_dev(int dev_no, AM_AV_Device_t **dev)
{
	AM_TRY(av_get_dev(dev_no, dev));

	if (!(*dev)->openned)
	{
		AM_DEBUG(1, "AV device %d has not been openned", dev_no);
		return AM_AV_ERR_INVALID_DEV_NO;
	}

	return AM_SUCCESS;
}

/**\brief 释放数据参数中的相关资源*/
static void av_free_data_para(AV_DataPlayPara_t *para)
{
	if (para->need_free)
	{
		munmap((uint8_t *)para->data, para->len);
		close(para->fd);
		para->need_free = AM_FALSE;
	}
}

/**\brief 结束播放*/
static AM_ErrorCode_t av_stop(AM_AV_Device_t *dev, AV_PlayMode_t mode)
{
	if (!(dev->mode&mode))
		return AM_SUCCESS;

	if (dev->drv->close_mode)
		dev->drv->close_mode(dev, mode);

	switch(mode)
	{
		case AV_PLAY_VIDEO_ES:
		case AV_DECODE_JPEG:
		case AV_GET_JPEG_INFO:
			av_free_data_para(&dev->vid_player.para);
		break;
		case AV_PLAY_AUDIO_ES:
			av_free_data_para(&dev->aud_player.para);
		break;
		default:
		break;
	}

	dev->mode &= ~mode;
	return AM_SUCCESS;
}

/**\brief 结束所有播放*/
static AM_ErrorCode_t av_stop_all(AM_AV_Device_t *dev, AV_PlayMode_t mode)
{
	if (mode & AV_PLAY_VIDEO_ES)
		av_stop(dev, AV_PLAY_VIDEO_ES);
	if (mode & AV_PLAY_AUDIO_ES)
		av_stop(dev, AV_PLAY_AUDIO_ES);
	if (mode & AV_GET_JPEG_INFO)
		av_stop(dev, AV_GET_JPEG_INFO);
	if (mode & AV_DECODE_JPEG)
		av_stop(dev, AV_DECODE_JPEG);
	if (mode & AV_PLAY_TS)
		av_stop(dev, AV_PLAY_TS);
	if (mode & AV_PLAY_FILE)
		av_stop(dev, AV_PLAY_FILE);
	if (mode & AV_INJECT)
		av_stop(dev, AV_INJECT);
	if (mode & AV_TIMESHIFT)
		av_stop(dev, AV_TIMESHIFT);

	return AM_SUCCESS;
}

/**\brief 开始播放*/
static AM_ErrorCode_t av_start(AM_AV_Device_t *dev, AV_PlayMode_t mode, void *para)
{
	AM_ErrorCode_t ret = AM_SUCCESS;
	AV_PlayMode_t stop_mode;
	AV_DataPlayPara_t *dp_para;

	/*检查需要停止的播放模式*/
	switch (mode)
	{
		case AV_PLAY_VIDEO_ES:
			stop_mode = AV_DECODE_JPEG|AV_GET_JPEG_INFO|AV_PLAY_TS|AV_PLAY_FILE|AV_INJECT|AV_TIMESHIFT;
		break;
		case AV_PLAY_AUDIO_ES:
			stop_mode = AV_PLAY_TS|AV_PLAY_FILE|AV_PLAY_AUDIO_ES|AV_INJECT|AV_TIMESHIFT;
		break;
		case AV_GET_JPEG_INFO:
			stop_mode = AV_MODE_ALL&~(AV_PLAY_AUDIO_ES|AV_PLAY_FILE);
		break;
		case AV_DECODE_JPEG:
			stop_mode = AV_MODE_ALL&~(AV_PLAY_AUDIO_ES|AV_PLAY_FILE);
		break;
		case AV_PLAY_TS:
			dev->ts_player.play_para = *(AV_TSPlayPara_t *)para;
			stop_mode = AV_MODE_ALL;
		break;
		case AV_PLAY_FILE:
			stop_mode = AV_MODE_ALL;
		break;
		case AV_INJECT:
			stop_mode = AV_MODE_ALL;
		break;
		case AV_TIMESHIFT:
			stop_mode = AV_MODE_ALL;
		break;
		default:
			stop_mode = 0;
		break;
	}

	if (stop_mode)
		av_stop_all(dev, stop_mode);

	if (dev->drv->open_mode && !(dev->mode&mode))
	{
		ret = dev->drv->open_mode(dev, mode);
		if (ret != AM_SUCCESS)
			return ret;
	}
	else if (!dev->drv->open_mode)
	{
		AM_DEBUG(1, "do not support the operation");
		return AM_AV_ERR_NOT_SUPPORTED;
	}

	dev->mode |= mode;

	/*记录相关参数数据*/
	switch (mode)
	{
		case AV_PLAY_VIDEO_ES:
		case AV_GET_JPEG_INFO:
		case AV_DECODE_JPEG:
			dp_para = (AV_DataPlayPara_t *)para;
			dev->vid_player.para = *dp_para;
			para = dp_para->para;
		break;
		case AV_PLAY_AUDIO_ES:
			dp_para = (AV_DataPlayPara_t *)para;
			dev->aud_player.para = *dp_para;
			para = dp_para->para;
		break;
		default:
		break;
	}

	/*开始播放*/
	if (dev->drv->start_mode)
	{
		ret = dev->drv->start_mode(dev, mode, para);
		if (ret != AM_SUCCESS)
		{
			switch (mode)
			{
				case AV_PLAY_VIDEO_ES:
				case AV_GET_JPEG_INFO:
				case AV_DECODE_JPEG:
					dev->vid_player.para.need_free = AM_FALSE;
				break;
				case AV_PLAY_AUDIO_ES:
					dev->aud_player.para.need_free = AM_FALSE;
				break;
				default:
				break;
			}
			return ret;
		}
	}

	return AM_SUCCESS;
}

/**\brief 视频输出模式变化事件回调*/
static void av_vout_format_changed(long dev_no, int event_type, void *param, void *data)
{
	AM_AV_Device_t *dev = (AM_AV_Device_t *)data;

	UNUSED(dev_no);

	if (event_type == AM_VOUT_EVT_FORMAT_CHANGED)
	{
		AM_VOUT_Format_t fmt = (AM_VOUT_Format_t)param;
		int w, h;

		w = dev->vout_w;
		h = dev->vout_h;

		switch (fmt)
		{
			case AM_VOUT_FORMAT_576CVBS:
				w = 720;
				h = 576;
			break;
			case AM_VOUT_FORMAT_480CVBS:
				w = 720;
				h = 480;
			break;
			case AM_VOUT_FORMAT_576I:
				w = 720;
				h = 576;
			break;
			case AM_VOUT_FORMAT_576P:
				w = 720;
				h = 576;
			break;
			case AM_VOUT_FORMAT_480I:
				w = 720;
				h = 480;
			break;
			case AM_VOUT_FORMAT_480P:
				w = 720;
				h = 480;
			break;
			case AM_VOUT_FORMAT_720P:
				w = 1280;
				h = 720;
			break;
			case AM_VOUT_FORMAT_1080I:
				w = 1920;
				h = 1080;
			break;
			case AM_VOUT_FORMAT_1080P:
				w = 1920;
				h = 1080;
			break;
			default:
			break;
		}

		if ((w != dev->vout_w) || (h != dev->vout_h))
		{
			int nx, ny, nw, nh;

			if (dev->video_w == 0 || dev->video_h == 0)
			{
				dev->video_w = dev->vout_w;
				dev->video_h = dev->vout_h;
			}

			nx = dev->video_x*w/dev->vout_w;
			ny = dev->video_y*h/dev->vout_h;
			nw = dev->video_w*w/dev->vout_w;
			nh = dev->video_h*h/dev->vout_h;

			AM_AV_SetVideoWindow(dev->dev_no, nx, ny, nw, nh);

			dev->vout_w = w;
			dev->vout_h = h;
		}
	}
}

extern AM_ErrorCode_t av_set_vpath(AM_AV_Device_t *dev, AM_AV_FreeScalePara_t fs, AM_AV_DeinterlacePara_t di, AM_AV_PPMGRPara_t pp)
{
	AM_ErrorCode_t ret = AM_SUCCESS;

	dev->vpath_fs = fs;
	dev->vpath_di = di;
	dev->vpath_ppmgr = pp;

	if (dev->mode & AV_TIMESHIFT) {
		if (dev->drv->timeshift_cmd) {
			ret = dev->drv->timeshift_cmd(dev, AV_PLAY_RESET_VPATH, NULL);
		}
	} else {
		int mode = dev->mode & ~AV_PLAY_AUDIO_ES;
		AM_AV_PlayPara_t para = dev->curr_para;
		void *p = NULL;

		if (mode & AV_PLAY_VIDEO_ES) {
			p = &para.ves;
			dev->vid_player.para.need_free = AM_FALSE;
		} else if(mode & AV_PLAY_TS) {
			p = &para.ts;
		} else if(mode & AV_PLAY_FILE) {
			AM_AV_PlayStatus_t status;

			p = &para.file;
			if (dev->drv->file_status) {
				if (dev->drv->file_status(dev, &status) == AM_SUCCESS) {
					para.pos = status.position;
				}
			}
		} else if (mode & AV_INJECT) {
			p = &para.inject;
		}

		if (mode) {
			av_stop_all(dev, mode);
		}

		if (dev->drv->set_vpath) {
			ret = dev->drv->set_vpath(dev);
		}

		if (mode) {
			av_start(dev, mode, p);
		}

		if (mode & AV_PLAY_VIDEO_ES) {
		} else if (mode & AV_PLAY_TS) {
		} else if (mode & AV_PLAY_FILE) {
			if (para.start && dev->drv->file_cmd) {
				dev->drv->file_cmd(dev, AV_PLAY_SEEK, (void*)(long)para.pos);
				if (para.pause)
					dev->drv->file_cmd(dev, AV_PLAY_PAUSE, NULL);
				else if (para.speed>0)
					dev->drv->file_cmd(dev, AV_PLAY_FF, (void*)(long)para.speed);
				else if (para.speed<0)
					dev->drv->file_cmd(dev, AV_PLAY_FB, (void*)(long)para.speed);
			}
		} else if (mode & AV_INJECT) {
			if (para.pause) {
				if (dev->drv->inject_cmd) {
					dev->drv->inject_cmd(dev, AV_PLAY_PAUSE, NULL);
				}
			}
		}
	}

	return ret;
}

/****************************************************************************
 * API functions
 ***************************************************************************/

/**\brief 打开音视频解码设备
 * \param dev_no 音视频设备号
 * \param[in] para 音视频设备开启参数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_Open(int dev_no, const AM_AV_OpenPara_t *para)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;

	assert(para);

	AM_TRY(av_get_dev(dev_no, &dev));

	pthread_mutex_lock(&am_gAdpLock);

	if (dev->openned)
	{
		AM_DEBUG(1, "AV device %d has already been openned", dev_no);
		ret = AM_AV_ERR_BUSY;
		goto final;
	}

	dev->dev_no  = dev_no;
	pthread_mutex_init(&dev->lock, NULL);
	dev->openned = AM_TRUE;
	dev->mode    = 0;
	dev->video_x = 0;
	dev->video_y = 0;
	dev->video_w = 1280;
	dev->video_h = 720;
	dev->video_contrast   = 0;
	dev->video_saturation = 0;
	dev->video_brightness = 0;
	dev->video_enable     = AM_TRUE;
	dev->video_blackout   = AM_TRUE;
	dev->video_ratio      = AM_AV_VIDEO_ASPECT_AUTO;
	dev->video_match      = AM_AV_VIDEO_ASPECT_MATCH_IGNORE;
	dev->video_mode       = AM_AV_VIDEO_DISPLAY_NORMAL;
	dev->vout_dev_no      = para->vout_dev_no;
	dev->vout_w           = 0;
	dev->vout_h           = 0;
	dev->vpath_fs         = -1;
	dev->vpath_di         = -1;
	dev->vpath_ppmgr      = -1;
	dev->afd_enable       = para->afd_enable;
	dev->crypt_ops        = NULL;

	if (dev->drv->open)
	{
		ret = dev->drv->open(dev, para);
		if(ret!=AM_SUCCESS)
			goto final;
	}

	if (ret == AM_SUCCESS)
	{
		AM_EVT_Subscribe(dev->vout_dev_no, AM_VOUT_EVT_FORMAT_CHANGED, av_vout_format_changed, dev);
	}

final:
	pthread_mutex_unlock(&am_gAdpLock);

	return ret;
}

/**\brief 关闭音视频解码设备
 * \param dev_no 音视频设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_Close(int dev_no)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;

	AM_TRY(av_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&am_gAdpLock);

	/*注销事件*/
	AM_EVT_Unsubscribe(dev->vout_dev_no, AM_VOUT_EVT_FORMAT_CHANGED, av_vout_format_changed, dev);

	/*停止播放*/
	av_stop_all(dev, dev->mode);

	if (dev->drv->close)
		dev->drv->close(dev);

	/*释放资源*/
	pthread_mutex_destroy(&dev->lock);

	dev->openned = AM_FALSE;

	pthread_mutex_unlock(&am_gAdpLock);

	return ret;
}

/**\brief 设定TS流的输入源
 * \param dev_no 音视频设备号
 * \param src TS输入源
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_SetTSSource(int dev_no, AM_AV_TSSource_t src)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;

	AM_TRY(av_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);

	if (dev->drv->ts_source)
		ret = dev->drv->ts_source(dev, src);

	if (ret == AM_SUCCESS)
		dev->ts_player.src = src;

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief 设置FrontEnd的锁定状态
 * \param dev_no 音视频设备号
 * \param value FrontEnd的状态
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */

AM_ErrorCode_t AM_AV_setFEStatus(int dev_no,int value)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	AM_TRY(av_get_openned_dev(dev_no, &dev));
	if (dev->drv->set_fe_status) {
		ret = dev->drv->set_fe_status(dev,value);
	}
	return ret;
}

/**\brief 开始解码TS流
 * \param dev_no 音视频设备号
 * \param vpid 视频流PID
 * \param apid 音频流PID
 * \param pcrpid PCR PID
 * \param vfmt 视频压缩格式
 * \param afmt 音频压缩格式
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_StartTSWithPCR(int dev_no, uint16_t vpid, uint16_t apid, uint16_t pcrpid, AM_AV_VFormat_t vfmt, AM_AV_AFormat_t afmt)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	AV_TSPlayPara_t para;

	AM_TRY(av_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);

	para.vpid = vpid;
	para.vfmt = vfmt;
	para.apid = apid;
	para.afmt = afmt;
	para.pcrpid = pcrpid;
	para.sub_apid = -1;

	ret = av_start(dev, AV_PLAY_TS, &para);
	if (ret == AM_SUCCESS) {
		dev->curr_para.ts = para;
	}

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief 开始解码TS流
 * \param dev_no 音视频设备号
 * \param vpid 视频流PID
 * \param apid 音频流PID
 * \param vfmt 视频压缩格式
 * \param afmt 音频压缩格式
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_StartTS(int dev_no, uint16_t vpid, uint16_t apid, AM_AV_VFormat_t vfmt, AM_AV_AFormat_t afmt)
{
	return AM_AV_StartTSWithPCR(dev_no, vpid, apid, vpid, vfmt, afmt);
}

/**\brief 停止TS流解码
 * \param dev_no 音视频设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_StopTS(int dev_no)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;

	AM_TRY(av_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);

	ret = av_stop(dev, AV_PLAY_TS);

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief 开始播放文件
 * \param dev_no 音视频设备号
 * \param[in] fname 媒体文件名
 * \param loop 是否循环播放
 * \param pos 播放的起始位置
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_StartFile(int dev_no, const char *fname, AM_Bool_t loop, int pos)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	AV_FilePlayPara_t para;

	assert(fname);

	AM_TRY(av_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);

	para.name = fname;
	para.loop = loop;
	para.pos  = pos;
	para.start= AM_TRUE;

	ret = av_start(dev, AV_PLAY_FILE, &para);
	if (ret == AM_SUCCESS)
	{
		snprintf(dev->file_player.name, sizeof(dev->file_player.name), "%s", fname);
		dev->file_player.speed = 0;
		dev->curr_para.file = para;
		dev->curr_para.start = AM_TRUE;
		dev->curr_para.speed = 0;
		dev->curr_para.pause = AM_FALSE;
		dev->curr_para.file.name = dev->curr_para.file_name;
		snprintf(dev->curr_para.file_name, sizeof(dev->curr_para.file_name), "%s", fname);
	}

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief 切换到文件播放模式但不开始播放
 * \param dev_no 音视频设备号
 * \param[in] fname 媒体文件名
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_AddFile(int dev_no, const char *fname)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	AV_FilePlayPara_t para;

	assert(fname);

	AM_TRY(av_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);

	para.name = fname;
	para.start= AM_FALSE;

	ret = av_start(dev, AV_PLAY_FILE, &para);
	if (ret == AM_SUCCESS)
	{
		snprintf(dev->file_player.name, sizeof(dev->file_player.name), "%s", fname);
		dev->file_player.speed = 0;
		dev->curr_para.file = para;
		dev->curr_para.start = AM_FALSE;
		dev->curr_para.speed = 0;
		dev->curr_para.pause = AM_FALSE;
		dev->curr_para.file.name = dev->curr_para.file_name;
		snprintf(dev->curr_para.file_name, sizeof(dev->curr_para.file_name), "%s", fname);
	}

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief 开始播放已经添加的文件，在AM_AV_AddFile后调用此函数开始播放
 * \param dev_no 音视频设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_StartCurrFile(int dev_no)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;

	AM_TRY(av_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);

	if (!(dev->mode & AV_PLAY_FILE))
	{
		AM_DEBUG(1, "do not in the file play mode");
		ret = AM_AV_ERR_ILLEGAL_OP;
	}

	if (ret == AM_SUCCESS)
	{
		if (dev->drv->file_cmd)
		{
			ret = dev->drv->file_cmd(dev, AV_PLAY_START, 0);
			if (ret == AM_SUCCESS) {
				dev->curr_para.file.start = AM_TRUE;
				dev->curr_para.start = AM_TRUE;
			}
		}
	}

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief 停止播放文件
 * \param dev_no 音视频设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_StopFile(int dev_no)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;

	AM_TRY(av_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);

	ret = av_stop(dev, AV_PLAY_FILE);

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief 暂停文件播放
 * \param dev_no 音视频设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_PauseFile(int dev_no)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;

	AM_TRY(av_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);

	if (!(dev->mode & AV_PLAY_FILE))
	{
		AM_DEBUG(1, "do not in the file play mode");
		ret = AM_AV_ERR_ILLEGAL_OP;
	}

	if (ret == AM_SUCCESS)
	{
		if (dev->drv->file_cmd)
		{
			ret = dev->drv->file_cmd(dev, AV_PLAY_PAUSE, NULL);
			if (ret == AM_SUCCESS) {
				dev->curr_para.pause = AM_TRUE;
			}
		}
	}

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief 恢复文件播放
 * \param dev_no 音视频设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_ResumeFile(int dev_no)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;

	AM_TRY(av_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);

	if (!(dev->mode & AV_PLAY_FILE))
	{
		AM_DEBUG(1, "do not in the file play mode");
		ret = AM_AV_ERR_ILLEGAL_OP;
	}

	if (ret == AM_SUCCESS)
	{
		if (dev->drv->file_cmd)
		{
			ret = dev->drv->file_cmd(dev, AV_PLAY_RESUME, NULL);
			if (ret == AM_SUCCESS) {
				dev->curr_para.pause = AM_FALSE;
			}
		}
	}

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief 暂停数据注入
 * \param dev_no 音视频设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_PauseInject(int dev_no)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;

	AM_TRY(av_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);

	if (!(dev->mode & AV_INJECT))
	{
		AM_DEBUG(1, "do not in the inject mode");
		ret = AM_AV_ERR_ILLEGAL_OP;
	}

	if (ret == AM_SUCCESS)
	{
		if (dev->drv->inject_cmd)
		{
			ret = dev->drv->inject_cmd(dev, AV_PLAY_PAUSE, NULL);
			if (ret == AM_SUCCESS) {
				dev->curr_para.pause = AM_TRUE;
			}
		}
	}

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief 恢复注入
 * \param dev_no 音视频设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_ResumeInject(int dev_no)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;

	AM_TRY(av_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);

	if (!(dev->mode & AV_INJECT))
	{
		AM_DEBUG(1, "do not in the inject mode");
		ret = AM_AV_ERR_ILLEGAL_OP;
	}

	if (ret == AM_SUCCESS)
	{
		if (dev->drv->inject_cmd)
		{
			ret = dev->drv->inject_cmd(dev, AV_PLAY_RESUME, NULL);
			if (ret == AM_SUCCESS) {
				dev->curr_para.pause = AM_FALSE;
			}
		}
	}

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief 设定当前文件播放位置
 * \param dev_no 音视频设备号
 * \param pos 播放位置
 * \param start 是否开始播放
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_SeekFile(int dev_no, int pos, AM_Bool_t start)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	AV_FileSeekPara_t para;

	AM_TRY(av_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);

	if (!(dev->mode & AV_PLAY_FILE))
	{
		AM_DEBUG(1, "do not in the file play mode");
		ret = AM_AV_ERR_ILLEGAL_OP;
	}

	if (ret == AM_SUCCESS)
	{
		if (dev->drv->file_cmd)
		{
			para.pos   = pos;
			para.start = start;
			ret = dev->drv->file_cmd(dev, AV_PLAY_SEEK, &para);
		}
	}

	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief 快速向前播放
 * \param dev_no 音视频设备号
 * \param speed 播放速度
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_FastForwardFile(int dev_no, int speed)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;

	if(speed<0)
	{
		AM_DEBUG(1, "speed must >= 0");
		return AM_AV_ERR_INVAL_ARG;
	}

	AM_TRY(av_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);

	if (!(dev->mode & AV_PLAY_FILE))
	{
		AM_DEBUG(1, "do not in the file play mode");
		ret = AM_AV_ERR_ILLEGAL_OP;
	}

	if (ret == AM_SUCCESS)
	{
		if (dev->drv->file_cmd && (dev->file_player.speed != speed))
		{
			ret = dev->drv->file_cmd(dev, AV_PLAY_FF, (void*)(long)speed);
			if (ret == AM_SUCCESS)
			{
				dev->file_player.speed = speed;
				dev->curr_para.speed = speed;
				AM_EVT_Signal(dev->dev_no, AM_AV_EVT_PLAYER_SPEED_CHANGED, (void*)(long)speed);
			}
		}
	}

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief 快速向后播放
 * \param dev_no 音视频设备号
 * \param speed 播放速度
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_FastBackwardFile(int dev_no, int speed)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;

	if (speed < 0)
	{
		AM_DEBUG(1, "speed must >= 0");
		return AM_AV_ERR_INVAL_ARG;
	}

	AM_TRY(av_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);

	if (!(dev->mode & AV_PLAY_FILE))
	{
		AM_DEBUG(1, "do not in the file play mode");
		ret = AM_AV_ERR_ILLEGAL_OP;
	}

	if (ret == AM_SUCCESS)
	{
		if (dev->drv->file_cmd && (dev->file_player.speed != -speed))
		{
			ret = dev->drv->file_cmd(dev, AV_PLAY_FB, (void*)(long)speed);
			if (ret == AM_SUCCESS)
			{
				dev->file_player.speed = -speed;
				dev->curr_para.speed = -speed;
				AM_EVT_Signal(dev->dev_no, AM_AV_EVT_PLAYER_SPEED_CHANGED, (void*)(long)-speed);
			}
		}
	}

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief 取得当前媒体文件信息
 * \param dev_no 音视频设备号
 * \param[out] info 返回媒体文件信息
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_GetCurrFileInfo(int dev_no, AM_AV_FileInfo_t *info)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;

	assert(info);

	AM_TRY(av_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);

	if (!(dev->mode & AV_PLAY_FILE))
	{
		AM_DEBUG(1, "do not in the file play mode");
		ret = AM_AV_ERR_ILLEGAL_OP;
	}

	if (ret == AM_SUCCESS)
	{
		if (dev->drv->file_info)
		{
			ret = dev->drv->file_info(dev, info);
		}
	}

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief 取得媒体文件播放状态
 * \param dev_no 音视频设备号
 * \param[out] status 返回媒体文件播放状态
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_GetPlayStatus(int dev_no, AM_AV_PlayStatus_t *status)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;

	assert(status);

	AM_TRY(av_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);

	if (!(dev->mode & AV_PLAY_FILE))
	{
		AM_DEBUG(1, "do not in the file play mode");
		ret = AM_AV_ERR_ILLEGAL_OP;
	}

	if (ret == AM_SUCCESS)
	{
		if (!dev->drv->file_status)
		{
			AM_DEBUG(1, "do not support file_status");
			ret = AM_AV_ERR_NOT_SUPPORTED;
		}
	}

	if (ret == AM_SUCCESS)
	{
		ret = dev->drv->file_status(dev, status);
	}
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief 开始进入数据注入模式
 * \param dev_no 音视频设备号
 * \param[in] para 注入参数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_StartInject(int dev_no, const AM_AV_InjectPara_t *para)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;

	assert(para);

	AM_TRY(av_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);

	ret = av_start(dev, AV_INJECT, (void*)para);
	if (ret == AM_SUCCESS) {
		dev->curr_para.inject.para = *para;
		dev->curr_para.start = AM_TRUE;
		dev->curr_para.speed  = 0;
		dev->curr_para.pause  = AM_FALSE;
	}

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief Set DRM mode
 * \param dev_no 音视频设备号
 * \param[in] enable enable or disable DRM mode
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_SetDRMMode(int dev_no, AM_AV_DrmMode_t drm_mode)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_AV_ERR_SYS;

	//assert(para);

	AM_TRY(av_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);

	if (dev->drv->set_drm_mode)
		ret = dev->drv->set_drm_mode(dev, drm_mode);

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief 进入数据注入模式后，向音视频设备注入数据
 * \param dev_no 音视频设备号
 * \param type 注入数据类型
 * \param[in] data 注入数据的缓冲区
 * \param[in,out] size 传入要注入数据的长度，返回实际注入数据的长度
 * \param timeout 等待设备的超时时间，以毫秒为单位，<0表示一直等待
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_InjectData(int dev_no, AM_AV_InjectType_t type, uint8_t *data, int *size, int timeout)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;

	assert(data && size);

	AM_TRY(av_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);

	if (!(dev->mode & AV_INJECT))
	{
		//AM_DEBUG(1, "do not in the file play mode");
		ret = AM_AV_ERR_ILLEGAL_OP;
	}

	if (ret == AM_SUCCESS)
	{
		if (dev->drv->inject)
			ret = dev->drv->inject(dev, type, data, size, timeout);
	}

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief 停止数据注入模式
 * \param dev_no 音视频设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_StopInject(int dev_no)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;

	AM_TRY(av_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);

	ret = av_stop(dev, AV_INJECT);

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief 取得JPEG图片文件属性
 * \param dev_no 音视频设备号
 * \param[in] fname 图片文件名
 * \param[out] info 文件属性
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_GetJPEGInfo(int dev_no, const char *fname, AM_AV_JPEGInfo_t *info)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	uint8_t *data;
	struct stat sbuf;
	int len;
	int fd;
	AV_DataPlayPara_t para;

	assert(fname && info);

	AM_TRY(av_get_openned_dev(dev_no, &dev));

	fd = open(fname, O_RDONLY);
	if (fd == -1)
	{
		AM_DEBUG(1, "cannot open \"%s\"", fname);
		return AM_AV_ERR_CANNOT_OPEN_FILE;
	}

	if (stat(fname, &sbuf) == -1)
	{
		AM_DEBUG(1, "stat failed");
		close(fd);
		return AM_AV_ERR_SYS;
	}

	len = sbuf.st_size;

	data = mmap(NULL, len, PROT_READ, MAP_SHARED, fd, 0);
	if (data == (void *)-1)
	{
		AM_DEBUG(1, "mmap failed");
		close(fd);
		return AM_AV_ERR_SYS;
	}

	para.data  = data;
	para.len   = len;
	para.times = 1;
	para.fd    = fd;
	para.need_free = AM_TRUE;
	para.para  = info;

	pthread_mutex_lock(&dev->lock);

	ret = av_start(dev, AV_GET_JPEG_INFO, &para);

	if (ret == AM_SUCCESS)
		av_stop(dev, AV_GET_JPEG_INFO);

	pthread_mutex_unlock(&dev->lock);

	if (ret != AM_SUCCESS)
		av_free_data_para(&para);

	return ret;
}

/**\brief 取得JPEG图片数据属性
 * \param dev_no 音视频设备号
 * \param[in] data 图片数据缓冲区
 * \param len 数据缓冲区大小
 * \param[out] info 文件属性
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_GetJPEGDataInfo(int dev_no, const uint8_t *data, int len, AM_AV_JPEGInfo_t *info)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	AV_DataPlayPara_t para;

	assert(data && info);

	AM_TRY(av_get_openned_dev(dev_no, &dev));

	para.data  = data;
	para.len   = len;
	para.times = 1;
	para.fd    = -1;
	para.need_free = AM_FALSE;
	para.para  = info;

	pthread_mutex_lock(&dev->lock);

	ret = av_start(dev, AV_GET_JPEG_INFO, info);

	if (ret == AM_SUCCESS)
		av_stop(dev, AV_GET_JPEG_INFO);

	pthread_mutex_unlock(&dev->lock);

	if (ret != AM_SUCCESS)
		av_free_data_para(&para);

	return ret;
}

/**\brief 解码JPEG图片文件
 * \param dev_no 音视频设备号
 * \param[in] fname 图片文件名
 * \param[in] para 输出JPEG的参数
 * \param[out] surf 返回JPEG图片的surface
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_DecodeJPEG(int dev_no, const char *fname, const AM_AV_SurfacePara_t *para, AM_OSD_Surface_t **surf)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	uint8_t *data;
	struct stat sbuf;
	int len;
	int fd;
	AV_DataPlayPara_t d_para;
	AM_AV_SurfacePara_t s_para;
	AV_JPEGDecodePara_t jpeg;

	assert(fname && surf);

	if (!para)
	{
		para = &s_para;
		s_para.width  = 0;
		s_para.height = 0;
		s_para.angle  = 0;
		s_para.option = 0;
	}

	AM_TRY(av_get_openned_dev(dev_no, &dev));

	fd = open(fname, O_RDONLY);
	if (fd == -1)
	{
		AM_DEBUG(1, "cannot open \"%s\"", fname);
		return AM_AV_ERR_CANNOT_OPEN_FILE;
	}

	if (stat(fname, &sbuf) == -1)
	{
		AM_DEBUG(1, "stat failed");
		close(fd);
		return AM_AV_ERR_SYS;
	}

	len = sbuf.st_size;

	data = mmap(NULL, len, PROT_READ, MAP_SHARED, fd, 0);
	if (data == (void *)-1)
	{
		AM_DEBUG(1, "mmap failed");
		close(fd);
		return AM_AV_ERR_SYS;
	}

	d_para.data  = data;
	d_para.len   = len;
	d_para.times = 1;
	d_para.fd    = fd;
	d_para.need_free = AM_TRUE;
	d_para.para  = &jpeg;

	jpeg.surface = NULL;
	jpeg.para    = *para;

	pthread_mutex_lock(&dev->lock);

	ret = av_start(dev, AV_DECODE_JPEG, &d_para);

	if (ret == AM_SUCCESS)
		av_stop(dev, AV_DECODE_JPEG);

	pthread_mutex_unlock(&dev->lock);

	if (ret != AM_SUCCESS)
		av_free_data_para(&d_para);
	else
		*surf = jpeg.surface;

	return ret;
}

/**\brief 解码JPEG图片数据
 * \param dev_no 音视频设备号
 * \param[in] data 图片数据缓冲区
 * \param len 数据缓冲区大小
 * \param[in] para 输出JPEG的参数
 * \param[out] surf 返回JPEG图片的surface
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_DacodeJPEGData(int dev_no, const uint8_t *data, int len, const AM_AV_SurfacePara_t *para, AM_OSD_Surface_t **surf)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	AV_DataPlayPara_t d_para;
	AM_AV_SurfacePara_t s_para;
	AV_JPEGDecodePara_t jpeg;

	assert(data && surf);

	if (!para)
	{
		para = &s_para;
		s_para.width  = 0;
		s_para.height = 0;
		s_para.angle  = 0;
		s_para.option = 0;
	}

	AM_TRY(av_get_openned_dev(dev_no, &dev));

	d_para.data  = data;
	d_para.len   = len;
	d_para.times = 1;
	d_para.fd    = -1;
	d_para.need_free = AM_FALSE;
	d_para.para  = &jpeg;

	jpeg.surface = NULL;
	jpeg.para    = *para;

	pthread_mutex_lock(&dev->lock);

	ret = av_start(dev, AV_DECODE_JPEG, &d_para);
	if (ret == AM_SUCCESS)
		av_stop(dev, AV_DECODE_JPEG);

	pthread_mutex_unlock(&dev->lock);

	if (ret != AM_SUCCESS)
		av_free_data_para(&d_para);
	else
		*surf = jpeg.surface;

	return ret;
}

/**\brief 解码视频基础流文件
 * \param dev_no 音视频设备号
 * \param format 视频压缩格式
 * \param[in] fname 视频文件名
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_StartVideoES(int dev_no, AM_AV_VFormat_t format, const char *fname)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	uint8_t *data;
	struct stat sbuf;
	int len;
	int fd;
	AV_DataPlayPara_t para;

	assert(fname);

	AM_TRY(av_get_openned_dev(dev_no, &dev));

	fd = open(fname, O_RDONLY);
	if (fd == -1)
	{
		AM_DEBUG(1, "cannot open \"%s\"", fname);
		return AM_AV_ERR_CANNOT_OPEN_FILE;
	}

	if (stat(fname, &sbuf) == -1)
	{
		AM_DEBUG(1, "stat failed");
		close(fd);
		return AM_AV_ERR_SYS;
	}

	len = sbuf.st_size;

	data = mmap(NULL, len, PROT_READ, MAP_SHARED, fd, 0);
	if (data == (void *)-1)
	{
		AM_DEBUG(1, "mmap failed");
		close(fd);
		return AM_AV_ERR_SYS;
	}

	para.data  = data;
	para.len   = len;
	para.times = 1;
	para.fd    = fd;
	para.need_free = AM_TRUE;
	para.para  = (void*)format;

	pthread_mutex_lock(&dev->lock);

	ret = av_start(dev, AV_PLAY_VIDEO_ES, &para);
	if (ret == AM_SUCCESS) {
		dev->curr_para.ves = para;
	}

	pthread_mutex_unlock(&dev->lock);

	if (ret != AM_SUCCESS)
		av_free_data_para(&para);

	return ret;
}

/**\brief 解码视频基础流数据
 * \param dev_no 音视频设备号
 * \param format 视频压缩格式
 * \param[in] data 视频数据缓冲区
 * \param len 数据缓冲区大小
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_StartVideoESData(int dev_no, AM_AV_VFormat_t format, const uint8_t *data, int len)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	AV_DataPlayPara_t para;

	assert(data);

	AM_TRY(av_get_openned_dev(dev_no, &dev));

	para.data  = data;
	para.len   = len;
	para.times = 1;
	para.fd    = -1;
	para.need_free = AM_FALSE;
	para.para  = (void*)format;

	pthread_mutex_lock(&dev->lock);

	ret = av_start(dev, AV_PLAY_VIDEO_ES, &para);
	if (ret == AM_SUCCESS) {
		dev->curr_para.ves = para;
	}

	pthread_mutex_unlock(&dev->lock);

	if (ret != AM_SUCCESS)
		av_free_data_para(&para);

	return ret;
}

/**\brief 解码音频基础流文件
 * \param dev_no 音视频设备号
 * \param format 音频压缩格式
 * \param[in] fname 视频文件名
 * \param times 播放次数，<=0表示一直播放
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_StartAudioES(int dev_no, AM_AV_AFormat_t format, const char *fname, int times)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	uint8_t *data;
	struct stat sbuf;
	int len;
	int fd;
	AV_DataPlayPara_t para;

	assert(fname);

	AM_TRY(av_get_openned_dev(dev_no, &dev));

	fd = open(fname, O_RDONLY);
	if (fd == -1)
	{
		AM_DEBUG(1, "cannot open \"%s\"", fname);
		return AM_AV_ERR_CANNOT_OPEN_FILE;
	}

	if (stat(fname, &sbuf) == -1)
	{
		AM_DEBUG(1, "stat failed");
		close(fd);
		return AM_AV_ERR_SYS;
	}

	len = sbuf.st_size;

	data = mmap(NULL, len, PROT_READ, MAP_SHARED, fd, 0);
	if (data == (void *)-1)
	{
		AM_DEBUG(1, "mmap failed");
		close(fd);
		return AM_AV_ERR_SYS;
	}

	para.data  = data;
	para.len   = len;
	para.times = times;
	para.fd    = fd;
	para.need_free = AM_TRUE;
	para.para  = (void*)format;

	pthread_mutex_lock(&dev->lock);

	ret = av_start(dev, AV_PLAY_AUDIO_ES, &para);
	if (ret == AM_SUCCESS) {
		dev->curr_para.aes = para;
	}

	pthread_mutex_unlock(&dev->lock);

	if (ret != AM_SUCCESS)
		av_free_data_para(&para);

	return ret;
}

/**\brief 解码音频基础流数据
 * \param dev_no 音视频设备号
 * \param format 音频压缩格式
 * \param[in] data 音频数据缓冲区
 * \param len 数据缓冲区大小
 * \param times 播放次数，<=0表示一直播放
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_StartAudioESData(int dev_no, AM_AV_AFormat_t format, const uint8_t *data, int len, int times)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	AV_DataPlayPara_t para;

	assert(data);

	AM_TRY(av_get_openned_dev(dev_no, &dev));

	para.data  = data;
	para.len   = len;
	para.times = times;
	para.fd    = -1;
	para.need_free = AM_FALSE;
	para.para  = (void*)format;

	pthread_mutex_lock(&dev->lock);

	ret = av_start(dev, AV_PLAY_AUDIO_ES, &para);
	if (ret == AM_SUCCESS) {
		dev->curr_para.aes = para;
	}

	pthread_mutex_unlock(&dev->lock);

	if (ret != AM_SUCCESS)
		av_free_data_para(&para);

	return ret;
}

/**\brief 停止解码视频基础流
 * \param dev_no 音视频设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_StopVideoES(int dev_no)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;

	AM_TRY(av_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);

	ret = av_stop(dev, AV_PLAY_VIDEO_ES);

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief 停止解码音频基础流
 * \param dev_no 音视频设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_StopAudioES(int dev_no)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;

	AM_TRY(av_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);

	ret = av_stop(dev, AV_PLAY_AUDIO_ES);

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief 设定视频窗口
 * \param dev_no 音视频设备号
 * \param x 窗口左上顶点x坐标
 * \param y 窗口左上顶点y坐标
 * \param w 窗口宽度
 * \param h 窗口高度
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_SetVideoWindow(int dev_no, int x, int y, int w, int h)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	AV_VideoWindow_t win;

	AM_TRY(av_get_openned_dev(dev_no, &dev));

	win.x = x;
	win.y = y;
	win.w = w;
	win.h = h;

	pthread_mutex_lock(&dev->lock);

	//if ((dev->video_x != x) || (dev->video_y != y) || (dev->video_w != w) || (dev->video_h != h))
	{
		if (dev->drv->set_video_para)
		{
			ret = dev->drv->set_video_para(dev, AV_VIDEO_PARA_WINDOW, &win);
		}

		if (ret == AM_SUCCESS)
		{
			AM_AV_VideoWindow_t win;

			dev->video_x = x;
			dev->video_y = y;
			dev->video_w = w;
			dev->video_h = h;

			win.x = x;
			win.y = y;
			win.w = w;
			win.h = h;

			AM_EVT_Signal(dev->dev_no, AM_AV_EVT_VIDEO_WINDOW_CHANGED, &win);
		}
	}

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief 设定视频裁切
 * \param dev_no 音视频设备号
 * \param x 窗口左上顶点x坐标
 * \param y 窗口左上顶点y坐标
 * \param w 窗口宽度
 * \param h 窗口高度
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_SetVideoCropping(int dev_no, int Voffset0, int Hoffset0, int Voffset1, int Hoffset1)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	AV_VideoWindow_t win;

	AM_TRY(av_get_openned_dev(dev_no, &dev));

	win.x = Voffset0;
	win.y = Hoffset0;
	win.w = Voffset1;
	win.h = Hoffset1;

	pthread_mutex_lock(&dev->lock);

	//if ((dev->video_x != x) || (dev->video_y != y) || (dev->video_w != w) || (dev->video_h != h))
	{
		if (dev->drv->set_video_para)
		{
			ret = dev->drv->set_video_para(dev, AV_VIDEO_PARA_CROP, &win);
		}

		if (ret == AM_SUCCESS)
		{
			AM_AV_VideoWindow_t win;

			win.x = Voffset0;
            win.y = Hoffset0;
            win.w = Voffset1;
            win.h = Hoffset1;

			AM_EVT_Signal(dev->dev_no, AM_AV_EVT_VIDEO_CROPPING_CHANGED, &win);
		}
	}

	pthread_mutex_unlock(&dev->lock);

	return ret;
}
/**\brief 返回视频窗口
 * \param dev_no 音视频设备号
 * \param[out] x 返回窗口左上顶点x坐标
 * \param[out] y 返回窗口左上顶点y坐标
 * \param[out] w 返回窗口宽度
 * \param[out] h 返回窗口高度
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_GetVideoWindow(int dev_no, int *x, int *y, int *w, int *h)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;

	AM_TRY(av_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);

	if (x)
		*x = dev->video_x;
	if (y)
		*y = dev->video_y;
	if (w)
		*w = dev->video_w;
	if (h)
		*h = dev->video_h;

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief 设定视频对比度(0~100)
 * \param dev_no 音视频设备号
 * \param val 视频对比度值
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_SetVideoContrast(int dev_no, int val)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;

	AM_TRY(av_get_openned_dev(dev_no, &dev));

	val = AM_MAX(val, AM_AV_VIDEO_CONTRAST_MIN);
	val = AM_MIN(val, AM_AV_VIDEO_CONTRAST_MAX);

	pthread_mutex_lock(&dev->lock);

	if (dev->video_contrast != val)
	{
		if (dev->drv->set_video_para)
		{
			ret = dev->drv->set_video_para(dev, AV_VIDEO_PARA_CONTRAST, (void*)(long)val);
		}

		if (ret == AM_SUCCESS)
		{
			dev->video_contrast = val;
			AM_EVT_Signal(dev->dev_no, AM_AV_EVT_VIDEO_CONTRAST_CHANGED, (void*)(long)val);
		}
	}

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief 取得当前视频对比度
 * \param dev_no 音视频设备号
 * \param[out] val 返回当前视频对比度值
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_GetVideoContrast(int dev_no, int *val)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;

	AM_TRY(av_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);

	if (val)
		*val = dev->video_contrast;

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief 设定视频饱和度(0~100)
 * \param dev_no 音视频设备号
 * \param val 视频饱和度值
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_SetVideoSaturation(int dev_no, int val)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;

	AM_TRY(av_get_openned_dev(dev_no, &dev));

	val = AM_MAX(val, AM_AV_VIDEO_SATURATION_MIN);
	val = AM_MIN(val, AM_AV_VIDEO_SATURATION_MAX);

	pthread_mutex_lock(&dev->lock);

	if (dev->video_saturation != val)
	{
		if (dev->drv->set_video_para)
		{
			ret = dev->drv->set_video_para(dev, AV_VIDEO_PARA_SATURATION, (void*)(long)val);
		}

		if (ret == AM_SUCCESS)
		{
			dev->video_saturation = val;
			AM_EVT_Signal(dev->dev_no, AM_AV_EVT_VIDEO_SATURATION_CHANGED, (void*)(long)val);
		}
	}

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief 取得当前视频饱和度
 * \param dev_no 音视频设备号
 * \param[out] val 返回当前视频饱和度值
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_GetVideoSaturation(int dev_no, int *val)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;

	AM_TRY(av_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);

	if (val)
		*val = dev->video_saturation;

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief 设定视频亮度(0~100)
 * \param dev_no 音视频设备号
 * \param val 视频亮度值
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_SetVideoBrightness(int dev_no, int val)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;

	AM_TRY(av_get_openned_dev(dev_no, &dev));

	val = AM_MAX(val, AM_AV_VIDEO_BRIGHTNESS_MIN);
	val = AM_MIN(val, AM_AV_VIDEO_BRIGHTNESS_MAX);

	pthread_mutex_lock(&dev->lock);

	if (dev->video_brightness != val)
	{
		if (dev->drv->set_video_para)
		{
			ret = dev->drv->set_video_para(dev, AV_VIDEO_PARA_BRIGHTNESS, (void*)(long)val);
		}

		if (ret == AM_SUCCESS)
		{
			dev->video_brightness = val;
			AM_EVT_Signal(dev->dev_no, AM_AV_EVT_VIDEO_BRIGHTNESS_CHANGED, (void*)(long)val);
		}
	}

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief 取得当前视频亮度
 * \param dev_no 音视频设备号
 * \param[out] val 返回当前视频亮度值
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_GetVideoBrightness(int dev_no, int *val)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;

	AM_TRY(av_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);

	if (val)
		*val = dev->video_brightness;

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief 显示视频层
 * \param dev_no 音视频设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_EnableVideo(int dev_no)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;

	AM_TRY(av_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);

	//if (!dev->video_enable)
	{
		if (dev->drv->set_video_para)
		{
			ret = dev->drv->set_video_para(dev, AV_VIDEO_PARA_ENABLE, (void*)(size_t)2);
		}

		if (ret == AM_SUCCESS)
		{
			dev->video_enable = AM_TRUE;
			AM_EVT_Signal(dev->dev_no, AM_AV_EVT_VIDEO_ENABLED, NULL);
		}
	}

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief 隐藏视频层
 * \param dev_no 音视频设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_DisableVideo(int dev_no)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;

	AM_TRY(av_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);

	//if (dev->video_enable)
	{
		if (dev->drv->set_video_para)
		{
			ret = dev->drv->set_video_para(dev, AV_VIDEO_PARA_ENABLE, (void*)AM_FALSE);
		}

		if (ret == AM_SUCCESS)
		{
			dev->video_enable = AM_FALSE;
			AM_EVT_Signal(dev->dev_no, AM_AV_EVT_VIDEO_DISABLED, NULL);
		}
	}

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief 设定视频长宽比
 * \param dev_no 音视频设备号
 * \param ratio 长宽比
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_SetVideoAspectRatio(int dev_no, AM_AV_VideoAspectRatio_t ratio)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;

	AM_TRY(av_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);

	if (dev->drv->set_video_para)
	{
		ret = dev->drv->set_video_para(dev, AV_VIDEO_PARA_RATIO, (void*)ratio);
	}

	if (ret == AM_SUCCESS)
	{
		dev->video_ratio = ratio;
		AM_EVT_Signal(dev->dev_no, AM_AV_EVT_VIDEO_ASPECT_RATIO_CHANGED, (void*)ratio);
	}

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief 取得视频长宽比
 * \param dev_no 音视频设备号
 * \param[out] ratio 长宽比
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_GetVideoAspectRatio(int dev_no, AM_AV_VideoAspectRatio_t *ratio)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;

	AM_TRY(av_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);

	if (ratio)
		*ratio = dev->video_ratio;

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief 设定视频长宽比匹配模式
 * \param dev_no 音视频设备号
 * \param mode 长宽比匹配模式
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_SetVideoAspectMatchMode(int dev_no, AM_AV_VideoAspectMatchMode_t mode)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;

	AM_TRY(av_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);

	if (dev->drv->set_video_para)
	{
		ret = dev->drv->set_video_para(dev, AV_VIDEO_PARA_RATIO_MATCH, (void*)mode);
	}

	if (ret == AM_SUCCESS)
	{
		dev->video_match = mode;
	}

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief 取得视频长宽比匹配模式
 * \param dev_no 音视频设备号
 * \param[out] mode 长宽比匹配模式
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_GetVideoAspectMatchMode(int dev_no, AM_AV_VideoAspectMatchMode_t *mode)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;

	AM_TRY(av_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);

	if (mode)
		*mode = dev->video_match;

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief 设定视频停止时自动隐藏视频层
 * \param dev_no 音视频设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_EnableVideoBlackout(int dev_no)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;

	AM_TRY(av_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);

	if (dev->drv->set_video_para)
	{
		ret = dev->drv->set_video_para(dev, AV_VIDEO_PARA_BLACKOUT, (void*)AM_TRUE);
	}

	if (ret == AM_SUCCESS)
	{
		dev->video_blackout = AM_TRUE;
	}

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief 视频停止时不隐藏视频层
 * \param dev_no 音视频设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_DisableVideoBlackout(int dev_no)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;

	AM_TRY(av_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);

	if (dev->drv->set_video_para)
	{
		ret = dev->drv->set_video_para(dev, AV_VIDEO_PARA_BLACKOUT, (void*)AM_FALSE);
	}

	if (ret == AM_SUCCESS)
	{
		dev->video_blackout = AM_FALSE;
	}

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief 设定视频显示模式
 * \param dev_no 音视频设备号
 * \param mode 视频显示模式
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_SetVideoDisplayMode(int dev_no, AM_AV_VideoDisplayMode_t mode)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;

	AM_TRY(av_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);

	if (dev->video_mode != mode)
	{
		if (dev->drv->set_video_para)
		{
			ret = dev->drv->set_video_para(dev, AV_VIDEO_PARA_MODE, (void*)mode);
		}

		if (ret == AM_SUCCESS)
		{
			dev->video_mode = mode;
			AM_EVT_Signal(dev->dev_no, AM_AV_EVT_VIDEO_DISPLAY_MODE_CHANGED, (void*)mode);
		}
	}

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief 取得当前视频显示模式
 * \param dev_no 音视频设备号
 * \param mode 返回当前视频显示模式
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_GetVideoDisplayMode(int dev_no, AM_AV_VideoDisplayMode_t *mode)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;

	AM_TRY(av_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);

	if (mode)
		*mode = dev->video_mode;

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief 清除视频缓冲区
 * \param dev_no 音视频设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_ClearVideoBuffer(int dev_no)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;

	AM_TRY(av_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);

	if (dev->drv->set_video_para)
	{
		ret = dev->drv->set_video_para(dev, AV_VIDEO_PARA_CLEAR_VBUF, NULL);
	}

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief 取得当前视频显示祯图形(此操作只有视频停止后才可进行)
 * \param dev_no 音视频设备号
 * \param[in] para 创建图像表面的参数
 * \param[out] s 返回图像
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_GetVideoFrame(int dev_no, const AM_AV_SurfacePara_t *para, AM_OSD_Surface_t **s)
{
	AM_AV_Device_t *dev;
	AM_AV_SurfacePara_t real_para;
	AM_ErrorCode_t ret = AM_SUCCESS;

	assert(s);

	if (!para)
	{
		memset(&real_para, 0, sizeof(real_para));
		para = &real_para;
	}

	AM_TRY(av_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);

	if (dev->drv->video_frame)
		ret = dev->drv->video_frame(dev, para, s);
	else
		ret = AM_AV_ERR_NOT_SUPPORTED;

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief 取得当前视频状态
 * \param dev_no 音视频设备号
 * \param[out]status 返回视频状态
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */

AM_ErrorCode_t AM_AV_GetVideoStatus(int dev_no,AM_AV_VideoStatus_t *status)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;

	AM_TRY(av_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);

	if (dev->drv->get_video_status)
		ret = dev->drv->get_video_status(dev,status);
	else
		ret = AM_AV_ERR_NOT_SUPPORTED;

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief 取得当前音频状态
 * \param dev_no 音视频设备号
 * \param[out]status 返回视频状态
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_GetAudioStatus(int dev_no,AM_AV_AudioStatus_t *status)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;

	AM_TRY(av_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);

	if (dev->drv->get_audio_status)
		ret = dev->drv->get_audio_status(dev,status);
	else
		ret = AM_AV_ERR_NOT_SUPPORTED;

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief 开始进入Timeshift模式
 * \param dev_no 音视频设备号
 * \param[in] para Timeshift参数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_StartTimeshift(int dev_no, const AM_AV_TimeshiftPara_t *para)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;

	assert(para);

	AM_TRY(av_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);

	ret = av_start(dev, AV_TIMESHIFT, (void*)para);
	if (ret == AM_SUCCESS) {
		dev->curr_para.time_shift.para = *para;
		snprintf(dev->curr_para.file_name, sizeof(dev->curr_para.file_name), "%s", para->file_path);
		dev->curr_para.start = AM_FALSE;
		dev->curr_para.speed = 0;
		dev->curr_para.pause = AM_FALSE;
	}

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief 进入Timeshift模式后，写数据到timeshift文件
 * \param dev_no 音视频设备号
 * \param[in] data 注入数据的缓冲区
 * \param size 传入要注入数据的长度
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_TimeshiftFillData(int dev_no, uint8_t *data, int size)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;

	assert(data && size);

	AM_TRY(av_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);

	/*if (!(dev->mode & AV_TIMESHIFT))
	{
		AM_DEBUG(1, "do not in the file play mode");
		ret = AM_AV_ERR_ILLEGAL_OP;
	}*/

	if (ret == AM_SUCCESS)
	{
		if (dev->drv->timeshift_fill)
			ret = dev->drv->timeshift_fill(dev, data, size);
	}

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief 停止Timeshift模式
 * \param dev_no 音视频设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_StopTimeshift(int dev_no)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;

	AM_TRY(av_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);

	ret = av_stop(dev, AV_TIMESHIFT);

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief 开始播放Timeshift
 * \param dev_no 音视频设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_PlayTimeshift(int dev_no)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;

	AM_TRY(av_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);

	if (!(dev->mode & AV_TIMESHIFT))
	{
		AM_DEBUG(1, "do not in the timeshift play mode");
		ret = AM_AV_ERR_ILLEGAL_OP;
	}

	if (ret == AM_SUCCESS)
	{
		if (dev->drv->timeshift_cmd)
		{
			ret = dev->drv->timeshift_cmd(dev, AV_PLAY_START, 0);
			if (ret == AM_SUCCESS) {
				dev->curr_para.start = AM_TRUE;
			}
		}
	}

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief 暂停Timeshift
 * \param dev_no 音视频设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_PauseTimeshift(int dev_no)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;

	AM_TRY(av_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);

	if (!(dev->mode & AV_TIMESHIFT))
	{
		AM_DEBUG(1, "do not in the timeshift play mode");
		ret = AM_AV_ERR_ILLEGAL_OP;
	}

	if (ret == AM_SUCCESS)
	{
		if (dev->drv->timeshift_cmd)
		{
			ret = dev->drv->timeshift_cmd(dev, AV_PLAY_PAUSE, NULL);
			if (ret == AM_SUCCESS) {
				dev->curr_para.pause = AM_TRUE;
			}
		}
	}

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief 恢复Timeshift播放
 * \param dev_no 音视频设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_ResumeTimeshift(int dev_no)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;

	AM_TRY(av_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);

	if (!(dev->mode & AV_TIMESHIFT))
	{
		AM_DEBUG(1, "do not in the timeshift play mode");
		ret = AM_AV_ERR_ILLEGAL_OP;
	}

	if (ret == AM_SUCCESS)
	{
		if (dev->drv->timeshift_cmd)
		{
			ret = dev->drv->timeshift_cmd(dev, AV_PLAY_RESUME, NULL);
			if (ret == AM_SUCCESS) {
				dev->curr_para.pause = AM_FALSE;
			}
		}
	}

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief 设定当前Timeshift播放位置
 * \param dev_no 音视频设备号
 * \param pos 播放位置
 * \param start 是否开始播放
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_SeekTimeshift(int dev_no, int pos, AM_Bool_t start)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	AV_FileSeekPara_t para;

	AM_TRY(av_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);

	if (!(dev->mode & AV_TIMESHIFT))
	{
		AM_DEBUG(1, "do not in the Timeshift play mode");
		ret = AM_AV_ERR_ILLEGAL_OP;
	}

	if (ret == AM_SUCCESS)
	{
		if (dev->drv->timeshift_cmd)
		{
			para.pos   = pos;
			para.start = start;
			ret = dev->drv->timeshift_cmd(dev, AV_PLAY_SEEK, &para);
		}
	}

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief 快速向前播放
 * \param dev_no 音视频设备号
 * \param speed 播放速度
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_FastForwardTimeshift(int dev_no, int speed)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;

	if (speed < 0)
	{
		AM_DEBUG(1, "speed must >= 0");
		return AM_AV_ERR_INVAL_ARG;
	}

	AM_TRY(av_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);

	if (!(dev->mode & AV_TIMESHIFT))
	{
		AM_DEBUG(1, "do not in the Timeshift play mode");
		ret = AM_AV_ERR_ILLEGAL_OP;
	}

	if (ret == AM_SUCCESS)
	{
		if (dev->drv->timeshift_cmd)
		{
			ret = dev->drv->timeshift_cmd(dev, AV_PLAY_FF, (void*)(long)speed);
			if (ret == AM_SUCCESS) {
				dev->curr_para.speed = speed;
			}
		}
	}

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief 快速向后播放
 * \param dev_no 音视频设备号
 * \param speed 播放速度
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_FastBackwardTimeshift(int dev_no, int speed)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;

	if (speed < 0)
	{
		AM_DEBUG(1, "speed must >= 0");
		return AM_AV_ERR_INVAL_ARG;
	}

	AM_TRY(av_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);

	if (!(dev->mode & AV_TIMESHIFT))
	{
		AM_DEBUG(1, "do not in the Timeshift play mode");
		ret = AM_AV_ERR_ILLEGAL_OP;
	}

	if (ret == AM_SUCCESS)
	{
		if (dev->drv->timeshift_cmd)
		{
			ret = dev->drv->timeshift_cmd(dev, AV_PLAY_FB, (void*)(long)-speed);
			if (ret == AM_SUCCESS) {
				dev->curr_para.speed = -speed;
			}
		}
	}

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief 切换当前Timeshift播放音频
 * \param dev_no 音视频设备号
 * \param apid the audio pid switched to
 * \param afmt the audio fmt switched to
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_SwitchTimeshiftAudio(int dev_no, int apid, int afmt)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	AV_TSPlayPara_t para;

	AM_TRY(av_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);

	if (!(dev->mode & AV_TIMESHIFT))
	{
		AM_DEBUG(1, "do not in the Timeshift play mode");
		ret = AM_AV_ERR_ILLEGAL_OP;
	}

	if (ret == AM_SUCCESS)
	{
		if (dev->drv->timeshift_cmd)
		{
			para.apid   = apid;
			para.afmt 	= afmt;
			ret = dev->drv->timeshift_cmd(dev, AV_PLAY_SWITCH_AUDIO, &para);
		}
	}

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief 获取当前Timeshift播放信息
 * \param dev_no 音视频设备号
 * \param [out] info 播放信息
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_GetTimeshiftInfo(int dev_no, AM_AV_TimeshiftInfo_t *info)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	AV_TSPlayPara_t para;

	AM_TRY(av_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);

	if (!(dev->mode & AV_TIMESHIFT))
	{
		AM_DEBUG(1, "do not in the Timeshift play mode");
		ret = AM_AV_ERR_ILLEGAL_OP;
	}

	if (ret == AM_SUCCESS)
	{
		if (dev->drv->get_timeshift_info)
		{
			ret = dev->drv->get_timeshift_info(dev, info);
		}
	}

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

AM_ErrorCode_t AM_AV_GetTimeshiftTFile(int dev_no, AM_TFile_t *tfile)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	AV_TSPlayPara_t para;

	AM_TRY(av_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);

	if (ret == AM_SUCCESS)
	{
		if (dev->drv->timeshift_get_tfile)
		{
			ret = dev->drv->timeshift_get_tfile(dev, tfile);
		}
	}

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief 设置视频通道参数
 * \param dev_no 音视频设备号
 * \param fs free scale参数
 * \param di deinterlace参数
 * \param pp PPMGR参数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t
AM_AV_SetVPathPara(int dev_no, AM_AV_FreeScalePara_t fs, AM_AV_DeinterlacePara_t di, AM_AV_PPMGRPara_t pp)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;

	UNUSED(dev_no);
	UNUSED(di);

	AM_TRY(av_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);

	//ret = av_set_vpath(dev, fs, di, pp);
	ret = av_set_vpath(dev, fs, AM_AV_DEINTERLACE_DISABLE, pp);

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief TS播放模式时切换音频，用于多音频切换，需先调用StartTS
 * \param dev_no 音视频设备号
 * \param apid 音频流PID
 * \param afmt 音频压缩格式
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_SwitchTSAudio(int dev_no, uint16_t apid, AM_AV_AFormat_t afmt)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;

	AM_TRY(av_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);

	if (dev->drv->switch_ts_audio)
		ret = dev->drv->switch_ts_audio(dev, apid, afmt);

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

AM_ErrorCode_t AM_AV_ResetAudioDecoder(int dev_no)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;

	AM_TRY(av_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);

	if ((dev->mode & AV_PLAY_TS) && dev->drv->switch_ts_audio) 
		ret = dev->drv->reset_audio_decoder(dev);

	pthread_mutex_unlock(&dev->lock);

	return ret;
}


/**\brief used to set /sys/module/amvdec_h264/parameters/error_recovery_mode to choose display mosaic or not
 * \param dev_no 音视频设备号
 * \param error_recovery_mode : 0 ,skip mosaic and reset vdec,2 skip mosaic ,3 display mosaic
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_SetVdecErrorRecoveryMode(int dev_no, uint8_t error_recovery_mode)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;

	AM_TRY(av_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);

	if (dev->drv->set_video_para)
	{
		ret = dev->drv->set_video_para(dev, AV_VIDEO_PARA_ERROR_RECOVERY_MODE, (void*)(long)error_recovery_mode);
	}

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

AM_ErrorCode_t AM_AV_SetInjectAudio(int dev_no, int aid, AM_AV_AFormat_t afmt)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;

	AM_TRY(av_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);

	if ((dev->mode & AV_INJECT) && dev->drv->set_inject_audio)
		ret = dev->drv->set_inject_audio(dev, aid, afmt);

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

AM_ErrorCode_t AM_AV_SetInjectSubtitle(int dev_no, int sid, int stype)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;

	AM_TRY(av_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);

	if ((dev->mode & AV_INJECT) && dev->drv->set_inject_subtitle)
		ret = dev->drv->set_inject_subtitle(dev, sid, stype);

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

AM_ErrorCode_t AM_AV_SetAudioAd(int dev_no, int enable, uint16_t apid, AM_AV_AFormat_t afmt)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;

	AM_TRY(av_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);

	if (dev->drv->set_audio_ad)
	{
		ret = dev->drv->set_audio_ad(dev, enable, apid, afmt);
	}

	pthread_mutex_unlock(&dev->lock);

	return ret;
}
AM_ErrorCode_t AM_AV_SetAudioCallback(int dev_no,AM_AV_Audio_CB_t cb,void *user_data)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;

	AM_TRY(av_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);

	if (dev->drv->set_audio_cb)
	{
		dev->drv->set_audio_cb(dev, cb, user_data);
	}

	pthread_mutex_unlock(&dev->lock);
	return 0;
}

AM_ErrorCode_t AM_AV_GetVideoPts(int dev_no, uint64_t *pts)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;

	AM_TRY(av_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);

	if (dev->drv->get_pts)
	{
		ret = dev->drv->get_pts(dev, 0, pts);
	}

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

AM_ErrorCode_t AM_AV_GetAudioPts(int dev_no, uint64_t *pts)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;

	AM_TRY(av_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);

	if (dev->drv->get_pts)
	{
		ret = dev->drv->get_pts(dev, 1, pts);
	}

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

AM_ErrorCode_t AM_AV_SetCryptOps(int dev_no, AM_Crypt_Ops_t *ops)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;

	AM_TRY(av_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);

	dev->crypt_ops = ops;

	pthread_mutex_unlock(&dev->lock);

	return ret;
}
