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
 * \brief DVB前端设备
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-06-07: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 5

#include <am_debug.h>
#include <am_mem.h>
#include "am_fend_internal.h"
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <assert.h>
#include <math.h>
#include <errno.h>
#include "../am_adp_internal.h"

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

#define FEND_DEV_COUNT      (2)
#define FEND_WAIT_TIMEOUT   (500)

#define M_BS_START_FREQ				(950)				/*The start RF frequency, 950MHz*/
#define M_BS_STOP_FREQ				(2150)				/*The stop RF frequency, 2150MHz*/
#define M_BS_MAX_SYMB				(45)
#define M_BS_MIN_SYMB				(2)

/****************************************************************************
 * Static data
 ***************************************************************************/

#ifdef LINUX_DVB_FEND
extern const AM_FEND_Driver_t linux_dvb_fend_drv;
#endif
#ifdef EMU_FEND
extern const AM_FEND_Driver_t emu_fend_drv;
#endif
extern const AM_FEND_Driver_t linux_v4l2_fend_drv;

static AM_FEND_Device_t fend_devices[FEND_DEV_COUNT] =
{
#ifdef EMU_FEND
	[0] = {
		.dev_no = 0,
		.drv = &emu_fend_drv,
	},
#if  FEND_DEV_COUNT > 1
	[1] = {
		.dev_no = 1,
		.drv = &emu_fend_drv,
	},
#endif
#elif defined LINUX_DVB_FEND
	[0] = {
		.dev_no = 0,
		.drv = &linux_dvb_fend_drv,
	},
#if  FEND_DEV_COUNT > 1
	[1] = {
		.dev_no = 1,
		.drv = &linux_v4l2_fend_drv,
	},
#endif
#endif
};

/****************************************************************************
 * Static functions
 ***************************************************************************/

/**\brief 根据设备号取得设备结构指针*/
static AM_INLINE AM_ErrorCode_t fend_get_dev(int dev_no, AM_FEND_Device_t **dev)
{
	if((dev_no<0) || (dev_no>=FEND_DEV_COUNT))
	{
		AM_DEBUG(1, "invalid frontend device number %d, must in(%d~%d)", dev_no, 0, FEND_DEV_COUNT-1);
		return AM_FEND_ERR_INVALID_DEV_NO;
	}
	
	*dev = &fend_devices[dev_no];
	return AM_SUCCESS;
}

/**\brief 根据设备号取得设备结构并检查设备是否已经打开*/
static AM_INLINE AM_ErrorCode_t fend_get_openned_dev(int dev_no, AM_FEND_Device_t **dev)
{
	AM_TRY(fend_get_dev(dev_no, dev));
	
	if((*dev)->open_count <= 0)
	{
		AM_DEBUG(1, "frontend device %d has not been openned", dev_no);
		return AM_FEND_ERR_INVALID_DEV_NO;
	}
	
	return AM_SUCCESS;
}

/**\brief 检查两个参数是否相等*/
static AM_Bool_t fend_para_equal(fe_type_t type, const struct dvb_frontend_parameters *p1, const struct dvb_frontend_parameters *p2)
{
	if(p1->frequency!=p2->frequency)
		return AM_FALSE;
	
	switch(type)
	{
		case FE_QPSK:
			if(p1->u.qpsk.symbol_rate!=p2->u.qpsk.symbol_rate)
				return AM_FALSE;
		break;
		case FE_QAM:
			if(p1->u.qam.symbol_rate!=p2->u.qam.symbol_rate)
				return AM_FALSE;
			if(p1->u.qam.modulation!=p2->u.qam.modulation)
				return AM_FALSE;
		break;
		case FE_OFDM:
		case FE_DTMB:
		case FE_ISDBT:
		case FE_ANALOG:
		break;
		case FE_ATSC:
			if(p1->u.vsb.modulation!=p2->u.vsb.modulation)
				return AM_FALSE;
		break;
		default:
			return AM_FALSE;
		break;
	}
	
	return AM_TRUE;
}

/**\brief 前端设备监控线程*/
static void* fend_thread(void *arg)
{
	AM_FEND_Device_t *dev = (AM_FEND_Device_t*)arg;
	struct dvb_frontend_event evt;
	AM_ErrorCode_t ret = AM_FAILURE;

	while(dev->enable_thread)
	{
		/*when blind scan is start, we need stop fend thread read event*/
		if (dev->enable_blindscan_thread) {
			usleep(100 * 1000);
			continue;
		}

		if(dev->drv->wait_event)
		{
			ret = dev->drv->wait_event(dev, &evt, FEND_WAIT_TIMEOUT);
		}
		
		if(dev->enable_thread)
		{
			pthread_mutex_lock(&dev->lock);
			dev->flags |= FEND_FL_RUN_CB;
			pthread_mutex_unlock(&dev->lock);
		
			if(ret==AM_SUCCESS)
			{
				AM_DEBUG(1, "fend_thread wait evt: %x\n", evt.status);
				
				if(dev->cb && dev->enable_cb)
				{
					dev->cb(dev->dev_no, &evt, dev->user_data);
				}

				if(dev->enable_cb)
				{
					AM_EVT_Signal(dev->dev_no, AM_FEND_EVT_STATUS_CHANGED, &evt);
				}
			}
		
			pthread_mutex_lock(&dev->lock);
			dev->flags &= ~FEND_FL_RUN_CB;
			pthread_mutex_unlock(&dev->lock);
			pthread_cond_broadcast(&dev->cond);
		}
	}
	
	return NULL;
}

/**\brief Initializes the blind scan parameters.*/
static AM_ErrorCode_t AM_FEND_IBlindScanAPI_Initialize(int dev_no)
{
	AM_FEND_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	AM_TRY(fend_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);

	memset(&(dev->bs_setting), 0, sizeof(struct AM_FEND_DVBSx_BlindScanAPI_Setting));

	dev->bs_setting.bsPara.minfrequency = M_BS_START_FREQ * 1000;		/*Default Set Blind scan start frequency*/
	dev->bs_setting.bsPara.maxfrequency = M_BS_STOP_FREQ * 1000;		/*Default Set Blind scan stop frequency*/

	dev->bs_setting.bsPara.maxSymbolRate = M_BS_MAX_SYMB * 1000 * 1000;  /*Set MAX symbol rate*/
	dev->bs_setting.bsPara.minSymbolRate = M_BS_MIN_SYMB * 1000 * 1000;   /*Set MIN symbol rate*/

	dev->bs_setting.bsPara.timeout = FEND_WAIT_TIMEOUT;

	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief Sets the start frequency and stop frequency.*/
static AM_ErrorCode_t  AM_FEND_IBlindScanAPI_SetFreqRange(int dev_no, unsigned int StartFreq_KHz, unsigned int EndFreq_KHz)
{	
	AM_FEND_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	AM_TRY(fend_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);

	dev->bs_setting.bsPara.minfrequency = StartFreq_KHz;		/*Change default start frequency*/
	dev->bs_setting.bsPara.maxfrequency = EndFreq_KHz;			/*Change default end frequency*/

	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief Performs a blind scan operation.*/
static AM_ErrorCode_t  AM_FEND_IBlindScanAPI_Start(int dev_no)
{	
	AM_FEND_Device_t *dev = NULL;
	AM_ErrorCode_t ret = AM_SUCCESS;

	fend_get_openned_dev(dev_no, &dev);
	
	if(!dev->drv->blindscan_scan)
	{
		AM_DEBUG(1, "fronend %d no not support blindscan_scan", dev_no);
		return AM_FEND_ERR_NOT_SUPPORTED;
	}	

	pthread_mutex_lock(&dev->lock);	
	
	struct dvbsx_blindscanpara * pbsPara = &(dev->bs_setting.bsPara);

	/*driver need to set in blindscan mode*/
	ret = dev->drv->blindscan_scan(dev, pbsPara);
	if(ret != AM_SUCCESS)
	{
		ret = AM_FEND_ERR_BLINDSCAN;
		pthread_mutex_unlock(&dev->lock);
		return ret;	
	}
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief Queries the blind scan event.*/
static AM_ErrorCode_t  AM_FEND_IBlindScanAPI_GetScanEvent(int dev_no, struct dvbsx_blindscanevent *pbsevent)
{
	AM_FEND_Device_t *dev = NULL;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	AM_TRY(fend_get_openned_dev(dev_no, &dev));
	
	if(!dev->drv->blindscan_getscanevent)
	{
		AM_DEBUG(1, "fronend %d no not support dvbsx_blindscan_getscanevent", dev_no);
		return AM_FEND_ERR_NOT_SUPPORTED;
	}

	pthread_mutex_lock(&dev->lock);

	struct dvbsx_blindscanevent * pbsEvent = &(dev->bs_setting.bsEvent);	

	/*Query the internal blind scan procedure information.*/
	ret = dev->drv->blindscan_getscanevent(dev, pbsEvent);
	if(ret != AM_SUCCESS)
	{
		ret = AM_FEND_ERR_BLINDSCAN;
		pthread_mutex_unlock(&dev->lock);
		return ret;
	}

	memcpy(pbsevent, pbsEvent, sizeof(struct dvbsx_blindscanevent));

	/*update tp info*/
	if(pbsEvent->status == BLINDSCAN_UPDATERESULTFREQ)
	{
		/*now driver return 1 tp*/
		dev->bs_setting.m_uiChannelCount = dev->bs_setting.m_uiChannelCount + 1;	

		memcpy(&(dev->bs_setting.channels[dev->bs_setting.m_uiChannelCount - 1]), 
				&(pbsEvent->u.parameters), sizeof(struct dvb_frontend_parameters));
	}

	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief Stops blind scan process.*/
static AM_ErrorCode_t  AM_FEND_IBlindScanAPI_Exit(int dev_no)
{
	AM_FEND_Device_t *dev = NULL;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	AM_TRY(fend_get_openned_dev(dev_no, &dev));
	
	if(!dev->drv->blindscan_cancel)
	{
		AM_DEBUG(1, "fronend %d no not support blindscan_cancel", dev_no);
		return AM_FEND_ERR_NOT_SUPPORTED;
	}	

	pthread_mutex_lock(&dev->lock);
	
	/*driver need to set in demod mode*/
	ret = dev->drv->blindscan_cancel(dev);

	usleep(10 * 1000);
	
	pthread_mutex_unlock(&dev->lock);

	return ret;	
}

/**\brief Gets the progress of blind scan process based on current scan step's start frequency.*/
static unsigned short AM_FEND_IBlindscanAPI_GetProgress(int dev_no)
{
	AM_FEND_Device_t *dev = NULL;
	AM_ErrorCode_t ret = AM_SUCCESS;
	unsigned short process = 0;
		
	AM_TRY(fend_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);

	process = dev->bs_setting.bsEvent.u.m_uiprogress;

	pthread_mutex_unlock(&dev->lock);
	
	return process;
}

static AM_ErrorCode_t AM_FEND_BlindDump(int dev_no)
{
	AM_FEND_Device_t *dev = NULL;
	AM_ErrorCode_t ret = AM_SUCCESS;
	int i = 0;

	AM_TRY(fend_get_openned_dev(dev_no, &dev));

	AM_DEBUG(1, "AM_FEND_BlindDump start %d--------------------\n", dev->bs_setting.m_uiChannelCount);

	pthread_mutex_lock(&dev->lock);

	for(i = 0; i < (dev->bs_setting.m_uiChannelCount); i++)
	{
		AM_DEBUG(1, "num:%d freq:%d symb:%d\n", i, dev->bs_setting.channels[i].frequency, dev->bs_setting.channels[i].u.qpsk.symbol_rate);
	}

	pthread_mutex_unlock(&dev->lock);	

	AM_DEBUG(1, "AM_FEND_BlindDump end--------------------\n");

	return ret;
}


/**\brief 前端设备盲扫处理线程*/
static void* fend_blindscan_thread(void *arg)
{
	int dev_no = (long)arg;
	AM_FEND_Device_t *dev = NULL;
	struct dvbsx_blindscanevent cur_bsevent;
	AM_FEND_BlindEvent_t evt;
	AM_ErrorCode_t ret = AM_FAILURE;
	unsigned short index = 0;
	enum AM_FEND_DVBSx_BlindScanAPI_Status BS_Status = DVBSx_BS_Status_Init;

	fend_get_openned_dev(dev_no, &dev);

	while(BS_Status != DVBSx_BS_Status_Exit)
	{
		if(!dev->enable_blindscan_thread)
		{
			BS_Status = DVBSx_BS_Status_Cancel;
		}
		
		switch(BS_Status)
		{
			case DVBSx_BS_Status_Init:			
				{
					BS_Status = DVBSx_BS_Status_Start;
					AM_DEBUG(1, "fend_blindscan_thread %d", DVBSx_BS_Status_Init);					
					break;
				}

			case DVBSx_BS_Status_Start:		
				{			
					ret = AM_FEND_IBlindScanAPI_Start(dev_no);
					AM_DEBUG(1, "fend_blindscan_thread AM_FEND_IBlindScanAPI_Start %d", ret);
					if(ret != AM_SUCCESS)
					{
						BS_Status = DVBSx_BS_Status_Exit;
					}
					else
					{	
						BS_Status = DVBSx_BS_Status_Wait;
					}
					break;
				}

			case DVBSx_BS_Status_Wait: 		
				{
					ret = AM_FEND_IBlindScanAPI_GetScanEvent(dev_no, &cur_bsevent);
					AM_DEBUG(1, "fend_blindscan_thread AM_FEND_IBlindScanAPI_GetScanEvent %d", ret);
					if(ret == AM_SUCCESS)
					{
						BS_Status = DVBSx_BS_Status_User_Process;
					}
					
					if(ret == AM_FEND_ERR_BLINDSCAN)
					{
						BS_Status = DVBSx_BS_Status_Wait;
					}
					
					break;
				}

			case DVBSx_BS_Status_User_Process:	
				{					
					/*
					------------Custom code start-------------------
					customer can add the callback function here such as adding TP information to TP list or lock the TP for parsing PSI
					Add custom code here; Following code is an example
					*/
					AM_DEBUG(1, "fend_blindscan_thread custom cb");
					if(dev->blindscan_cb)
					{
						if(cur_bsevent.status == BLINDSCAN_UPDATESTARTFREQ)
						{
							AM_DEBUG(1, "adp start freq %d\n", cur_bsevent.u.m_uistartfreq_khz);
							evt.freq = cur_bsevent.u.m_uistartfreq_khz;

							evt.status = AM_FEND_BLIND_START;
							dev->blindscan_cb(dev->dev_no, &evt, dev->blindscan_cb_user_data);						
						}
						else if(cur_bsevent.status == BLINDSCAN_UPDATEPROCESS)
						{
							AM_DEBUG(1, "adp process %d\n", cur_bsevent.u.m_uiprogress);
							evt.process = cur_bsevent.u.m_uiprogress;

							evt.status = AM_FEND_BLIND_UPDATEPROCESS;
							dev->blindscan_cb(dev->dev_no, &evt, dev->blindscan_cb_user_data);						
						}
						else if(cur_bsevent.status == BLINDSCAN_UPDATERESULTFREQ)
						{
							AM_DEBUG(1, "adp result freq %d symb %d\n", cur_bsevent.u.parameters.frequency, cur_bsevent.u.parameters.u.qpsk.symbol_rate);

							evt.status = AM_FEND_BLIND_UPDATETP;
							dev->blindscan_cb(dev->dev_no, &evt, dev->blindscan_cb_user_data);
						}
					}

					/*------------Custom code end -------------------*/
					if(cur_bsevent.status == BLINDSCAN_UPDATESTARTFREQ)
					{
						BS_Status = DVBSx_BS_Status_Wait;
					}
					else if(cur_bsevent.status == BLINDSCAN_UPDATEPROCESS)
					{
						if ( (evt.process < 100))
							BS_Status = DVBSx_BS_Status_Wait;
						else											
							BS_Status = DVBSx_BS_Status_WaitExit;					
					}
					else if(cur_bsevent.status == BLINDSCAN_UPDATERESULTFREQ)
					{
						BS_Status = DVBSx_BS_Status_Wait;
					}					
					
					break;
				}

			case DVBSx_BS_Status_WaitExit:
				{
					usleep(50*1000);
					break;
				}

			case DVBSx_BS_Status_Cancel:		
				{ 
					AM_FEND_BlindDump(dev_no);
					
					ret = AM_FEND_IBlindScanAPI_Exit(dev_no);
					BS_Status = DVBSx_BS_Status_Exit;

					AM_DEBUG(1, "AM_FEND_IBlindScanAPI_Exit");
					break;
				}

			default:						    
				{
					BS_Status = DVBSx_BS_Status_Cancel;
					break;
				}
		}
	}
	
	return NULL;
}

static void sighand(int signo) {}

/****************************************************************************
 * API functions
 ***************************************************************************/

/**\brief 打开一个DVB前端设备
 * \param dev_no 前端设备号
 * \param[in] para 设备开启参数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend.h)
 */
AM_ErrorCode_t AM_FEND_Open(int dev_no, const AM_FEND_OpenPara_t *para)
{
	AM_FEND_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	int rc;
	
	AM_TRY(fend_get_dev(dev_no, &dev));
	
	pthread_mutex_lock(&am_gAdpLock);
	
	if(dev->open_count > 0)
	{
		AM_DEBUG(1, "frontend device %d has already been openned", dev_no);
		dev->open_count++;
		ret = AM_SUCCESS;
		goto final;
	}
	
	if(dev->drv->open)
	{
		AM_TRY_FINAL(dev->drv->open(dev, para));
	}
	
	pthread_mutex_init(&dev->lock, NULL);
	pthread_cond_init(&dev->cond, NULL);
	
	dev->dev_no = dev_no;
	dev->open_count = 1;
	dev->enable_thread = AM_TRUE;
	dev->flags = 0;
	dev->enable_cb = AM_TRUE;
	dev->curr_mode = para->mode;
	
	rc = pthread_create(&dev->thread, NULL, fend_thread, dev);
	if(rc)
	{
		AM_DEBUG(1, "%s", strerror(rc));
		
		if(dev->drv->close)
		{
			dev->drv->close(dev);
		}
		pthread_mutex_destroy(&dev->lock);
		pthread_cond_destroy(&dev->cond);
		dev->open_count = 0;
		
		ret = AM_FEND_ERR_CANNOT_CREATE_THREAD;
		goto final;
	}

	{
		struct sigaction actions;
		memset(&actions, 0, sizeof(actions));
		sigemptyset(&actions.sa_mask);
		actions.sa_flags = 0;
		actions.sa_handler = sighand;
		rc = sigaction(SIGALRM, &actions, NULL);
		if (rc != 0)
			AM_DEBUG(1, "sigaction: err=%d", errno);
	}
final:	
	pthread_mutex_unlock(&am_gAdpLock);

	return ret;
}

/**\brief 关闭一个DVB前端设备
 * \param dev_no 前端设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend.h)
 */
AM_ErrorCode_t AM_FEND_CloseEx(int dev_no, AM_Bool_t reset)
{
	AM_FEND_Device_t *dev;
	
	AM_TRY(fend_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&am_gAdpLock);
	
	if (dev->open_count == 1)
	{
		int err = 0;

		dev->enable_cb = AM_FALSE;
		/*Stop the thread*/
		dev->enable_thread = AM_FALSE;
		err = pthread_kill(dev->thread, SIGALRM);
		if (err != 0)
			AM_DEBUG(1, "kill fail, err:%d", err);
		pthread_join(dev->thread, NULL);
		/*Release the device*/
		if(dev->drv->close)
		{
			if (reset && dev->drv->set_mode)
				dev->drv->set_mode(dev, FE_UNKNOWN);

			dev->drv->close(dev);
		}
	
		pthread_mutex_destroy(&dev->lock);
		pthread_cond_destroy(&dev->cond);
	}
	dev->open_count--;
	
	pthread_mutex_unlock(&am_gAdpLock);
	
	return AM_SUCCESS;
}
AM_ErrorCode_t AM_FEND_Close(int dev_no)
{
	return AM_FEND_CloseEx(dev_no, AM_TRUE);
}


/**\brief 关闭一个DVB前端设备
 * \param dev_no 前端设备号
 * \param fe_fd  前端设备文件描述号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend.h)
 */
AM_ErrorCode_t AM_FEND_OpenEx(int dev_no, const AM_FEND_OpenPara_t *para, int *fe_fd)
{
	AM_FEND_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	int rc;

	AM_TRY(fend_get_dev(dev_no, &dev));

	pthread_mutex_lock(&am_gAdpLock);

	if (dev->open_count > 0)
	{
		AM_DEBUG(1, "frontend device %d has already been openned", dev_no);
		dev->open_count++;
		ret = AM_SUCCESS;
		goto final;
	}

	if (dev->drv->open)
	{
		AM_TRY_FINAL(dev->drv->open(dev, para));
	}

	pthread_mutex_init(&dev->lock, NULL);
	pthread_cond_init(&dev->cond, NULL);

	dev->dev_no = dev_no;
	dev->open_count = 1;
	dev->enable_thread = AM_TRUE;
	dev->flags = 0;
	dev->enable_cb = AM_TRUE;
	dev->curr_mode = para->mode;

	rc = pthread_create(&dev->thread, NULL, fend_thread, dev);
	if (rc)
	{
		AM_DEBUG(1, "%s", strerror(rc));

		if (dev->drv->close)
		{
			dev->drv->close(dev);
		}
		pthread_mutex_destroy(&dev->lock);
		pthread_cond_destroy(&dev->cond);
		dev->open_count = 0;

		ret = AM_FEND_ERR_CANNOT_CREATE_THREAD;
		goto final;
	}
	*fe_fd = (long)dev->drv_data;

	{
		struct sigaction actions;
		memset(&actions, 0, sizeof(actions));
		sigemptyset(&actions.sa_mask);
		actions.sa_flags = 0;
		actions.sa_handler = sighand;
		rc = sigaction(SIGALRM, &actions, NULL);
		if (rc != 0)
			AM_DEBUG(1, "sigaction: err=%d", errno);
	}
final:
	pthread_mutex_unlock(&am_gAdpLock);

	return ret;
}


/**\brief 设定前端解调模式
 * \param dev_no 前端设备号
 * \param mode 解调模式
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend.h)
 */
AM_ErrorCode_t AM_FEND_SetMode(int dev_no, int mode)
{
	AM_FEND_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	AM_TRY(fend_get_openned_dev(dev_no, &dev));
	AM_DEBUG(1, "AM_FEND_SetMode %d", mode);
	if(!dev->drv->set_mode)
	{
		AM_DEBUG(1, "fronend %d no not support set_mode", dev_no);
		return AM_FEND_ERR_NOT_SUPPORTED;
	}
	
	pthread_mutex_lock(&dev->lock);
	
	ret = dev->drv->set_mode(dev, mode);
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief 取得一个DVB前端设备的相关信息
 * \param dev_no 前端设备号
 * \param[out] info 返回前端信息数据
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend.h)
 */
AM_ErrorCode_t AM_FEND_GetInfo(int dev_no, struct dvb_frontend_info *info)
{
	AM_FEND_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	assert(info);

	AM_TRY(fend_get_openned_dev(dev_no, &dev));
	
	if(!dev->drv->get_info)
	{
		AM_DEBUG(1, "fronend %d no not support get_info", dev_no);
		return AM_FEND_ERR_NOT_SUPPORTED;
	}
	
	pthread_mutex_lock(&dev->lock);
	
	ret = dev->drv->get_info(dev, info);
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief 取得一个DVB前端设备连接的TS输入源
 * \param dev_no 前端设备号
 * \param[out] src 返回设备对应的TS输入源
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend.h)
 */
AM_ErrorCode_t AM_FEND_GetTSSource(int dev_no, AM_DMX_Source_t *src)
{
	AM_FEND_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	assert(src);

	AM_TRY(fend_get_openned_dev(dev_no, &dev));
	
	if(!dev->drv->get_ts)
	{
		AM_DEBUG(1, "fronend %d no not support get_ts", dev_no);
		return AM_FEND_ERR_NOT_SUPPORTED;
	}
	
	pthread_mutex_lock(&dev->lock);
	
	ret = dev->drv->get_ts(dev, src);
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief 设定前端参数
 * \param dev_no 前端设备号
 * \param[in] para 前端设置参数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend.h)
 */
AM_ErrorCode_t AM_FEND_SetPara(int dev_no, const struct dvb_frontend_parameters *para)
{
	AM_FEND_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	assert(para);

	AM_DEBUG(1, "AM_FEND_SetPara\n");
	
	AM_TRY(fend_get_openned_dev(dev_no, &dev));
	
	if(!dev->drv->set_para)
	{
		AM_DEBUG(1, "fronend %d no not support set_para", dev_no);
		return AM_FEND_ERR_NOT_SUPPORTED;
	}
	
	pthread_mutex_lock(&dev->lock);
	
	ret = dev->drv->set_para(dev, para);
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

AM_ErrorCode_t AM_FEND_SetProp(int dev_no, const struct dtv_properties *prop)
{
	AM_FEND_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	assert(prop);
	
	AM_TRY(fend_get_openned_dev(dev_no, &dev));
	
	if(!dev->drv->set_prop)
	{
		AM_DEBUG(1, "fronend %d no not support set_prop", dev_no);
		return AM_FEND_ERR_NOT_SUPPORTED;
	}
	
	pthread_mutex_lock(&dev->lock);
	
	ret = dev->drv->set_prop(dev, prop);
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief 取得当前端设备设定的参数
 * \param dev_no 前端设备号
 * \param[out] para 前端设置参数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend.h)
 */
AM_ErrorCode_t AM_FEND_GetPara(int dev_no, struct dvb_frontend_parameters *para)
{
	AM_FEND_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	assert(para);
	
	AM_TRY(fend_get_openned_dev(dev_no, &dev));
	
	if(!dev->drv->get_para)
	{
		AM_DEBUG(1, "fronend %d no not support get_para", dev_no);
		return AM_FEND_ERR_NOT_SUPPORTED;
	}
	
	pthread_mutex_lock(&dev->lock);
	
	ret = dev->drv->get_para(dev, para);
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

AM_ErrorCode_t AM_FEND_GetProp(int dev_no, struct dtv_properties *prop)
{
	AM_FEND_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	assert(prop);
	
	AM_TRY(fend_get_openned_dev(dev_no, &dev));
	
	if(!dev->drv->get_prop)
	{
		AM_DEBUG(1, "fronend %d no not support get_prop", dev_no);
		return AM_FEND_ERR_NOT_SUPPORTED;
	}
	
	pthread_mutex_lock(&dev->lock);
	
	ret = dev->drv->get_prop(dev, prop);
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}


/**\brief 取得前端设备当前的锁定状态
 * \param dev_no 前端设备号
 * \param[out] status 返回前端设备的锁定状态
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend.h)
 */
AM_ErrorCode_t AM_FEND_GetStatus(int dev_no, fe_status_t *status)
{
	AM_FEND_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	assert(status);
	
	AM_TRY(fend_get_openned_dev(dev_no, &dev));
	
	if(!dev->drv->get_status)
	{
		AM_DEBUG(1, "fronend %d no not support get_status", dev_no);
		return AM_FEND_ERR_NOT_SUPPORTED;
	}
	
	pthread_mutex_lock(&dev->lock);
	
	ret = dev->drv->get_status(dev, status);
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief 取得前端设备当前的SNR值
 * \param dev_no 前端设备号
 * \param[out] snr 返回SNR值
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend.h)
 */
AM_ErrorCode_t AM_FEND_GetSNR(int dev_no, int *snr)
{
	AM_FEND_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	assert(snr);
	
	AM_TRY(fend_get_openned_dev(dev_no, &dev));
	
	if(!dev->drv->get_snr)
	{
		AM_DEBUG(1, "fronend %d no not support get_snr", dev_no);
		return AM_FEND_ERR_NOT_SUPPORTED;
	}
	
	pthread_mutex_lock(&dev->lock);
	
	ret = dev->drv->get_snr(dev, snr);
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief 取得前端设备当前的BER值
 * \param dev_no 前端设备号
 * \param[out] ber 返回BER值
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend.h)
 */
AM_ErrorCode_t AM_FEND_GetBER(int dev_no, int *ber)
{
	AM_FEND_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	assert(ber);
	
	AM_TRY(fend_get_openned_dev(dev_no, &dev));
	
	if(!dev->drv->get_ber)
	{
		AM_DEBUG(1, "fronend %d no not support get_ber", dev_no);
		return AM_FEND_ERR_NOT_SUPPORTED;
	}
	
	pthread_mutex_lock(&dev->lock);
	
	ret = dev->drv->get_ber(dev, ber);
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief 取得前端设备当前的信号强度值
 * \param dev_no 前端设备号
 * \param[out] strength 返回信号强度值
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend.h)
 */
AM_ErrorCode_t AM_FEND_GetStrength(int dev_no, int *strength)
{
	AM_FEND_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	assert(strength);
	
	AM_TRY(fend_get_openned_dev(dev_no, &dev));
	
	if(!dev->drv->get_strength)
	{
		AM_DEBUG(1, "fronend %d no not support get_strength", dev_no);
		return AM_FEND_ERR_NOT_SUPPORTED;
	}
	
	pthread_mutex_lock(&dev->lock);
	
	ret = dev->drv->get_strength(dev, strength);
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief 取得当前注册的前端状态监控回调函数
 * \param dev_no 前端设备号
 * \param[out] cb 返回注册的状态回调函数
 * \param[out] user_data 返回状态回调函数的参数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend.h)
 */
AM_ErrorCode_t AM_FEND_GetCallback(int dev_no, AM_FEND_Callback_t *cb, void **user_data)
{
	AM_FEND_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	AM_TRY(fend_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	
	if(cb)
	{
		*cb = dev->cb;
	}
	
	if(user_data)
	{
		*user_data = dev->user_data;
	}
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief 注册前端设备状态监控回调函数
 * \param dev_no 前端设备号
 * \param[in] cb 状态回调函数
 * \param[in] user_data 状态回调函数的参数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend.h)
 */
AM_ErrorCode_t AM_FEND_SetCallback(int dev_no, AM_FEND_Callback_t cb, void *user_data)
{
	AM_FEND_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	AM_TRY(fend_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	
	if(cb!=dev->cb || user_data!=dev->user_data)
	{
		if(dev->enable_thread && (dev->thread!=pthread_self()))
		{
			/*等待回调函数执行完*/
			while(dev->flags&FEND_FL_RUN_CB)
			{
				pthread_cond_wait(&dev->cond, &dev->lock);
			}
		}
		
		dev->cb = cb;
		dev->user_data = user_data;
	}
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief 设置前端设备状态监控回调函数活动状态
 * \param dev_no 前端设备号
 * \param[in] enable_cb 允许或者禁止状态回调函数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend.h)
 */
AM_ErrorCode_t AM_FEND_SetActionCallback(int dev_no, AM_Bool_t enable_cb)
{
	AM_FEND_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	AM_TRY(fend_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	
	if(enable_cb != dev->enable_cb)
	{
#if 0
		if(dev->enable_thread && (dev->thread!=pthread_self()))
		{
			/*等待回调函数执行完*/
			while(dev->flags&FEND_FL_RUN_CB)
			{
				pthread_cond_wait(&dev->cond, &dev->lock);
			}
		}
#endif
		dev->enable_cb = enable_cb;
	}
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief AM_FEND_Lock的回调函数参数*/
typedef struct {
	const struct dvb_frontend_parameters *para;
	fe_status_t                          *status;
	AM_FEND_Callback_t                    old_cb;
	void                                 *old_data;
} fend_lock_para_t;

/**\brief AM_FEND_Lock的回调函数*/
static void fend_lock_cb(int dev_no, struct dvb_frontend_event *evt, void *user_data)
{
	AM_FEND_Device_t *dev = NULL;
	fend_lock_para_t *para = (fend_lock_para_t*)user_data;
	
	fend_get_openned_dev(dev_no, &dev);
	
	if(!fend_para_equal(dev->curr_mode, &evt->parameters, para->para))
		return;
	
	if(!evt->status)
		return;
	
	*para->status = evt->status;
	
	pthread_mutex_lock(&dev->lock);
	dev->flags &= ~FEND_FL_LOCK;
	pthread_mutex_unlock(&dev->lock);
	
	if(para->old_cb)
	{
		para->old_cb(dev_no, evt, para->old_data);
	}
	
	pthread_cond_broadcast(&dev->cond);
}

/**\brief 设定前端设备参数，并等待参数设定完成
 * \param dev_no 前端设备号
 * \param[in] para 前端设置参数
 * \param[out] status 返回前端设备状态
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend.h)
 */ 
AM_ErrorCode_t AM_FEND_Lock(int dev_no, const struct dvb_frontend_parameters *para, fe_status_t *status)
{
	AM_FEND_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	fend_lock_para_t lockp;
	
	assert(para && status);

	AM_DEBUG(1, "AM_FEND_Lock\n");
	
	AM_TRY(fend_get_openned_dev(dev_no, &dev));
	
	if(!dev->drv->set_para)
	{
		AM_DEBUG(1, "fronend %d no not support set_para", dev_no);
		return AM_FEND_ERR_NOT_SUPPORTED;
	}
	
	if(dev->thread==pthread_self())
	{
		AM_DEBUG(1, "cannot invoke AM_FEND_Lock in callback");
		return AM_FEND_ERR_INVOKE_IN_CB;
	}
	
	pthread_mutex_lock(&dev->lock);
	
	/*等待回调函数执行完*/
	while(dev->flags&FEND_FL_RUN_CB)
	{
		pthread_cond_wait(&dev->cond, &dev->lock);
	}
	AM_DEBUG(1, "AM_FEND_Lock line:%d\n",__LINE__);
	
	lockp.old_cb   = dev->cb;
	lockp.old_data = dev->user_data;
	lockp.para   = para;
	lockp.status = status;
	
	dev->cb = fend_lock_cb;
	dev->user_data = &lockp;
	dev->flags |= FEND_FL_LOCK;

	
	AM_DEBUG(1, "AM_FEND_Lock line:%d\n",__LINE__);
	ret = dev->drv->set_para(dev, para);
	
	AM_DEBUG(1, "AM_FEND_Lock line:%d,ret:%d\n",__LINE__,ret);
	if(ret==AM_SUCCESS)
	{
		/*等待回调函数执行完*/
		while((dev->flags&FEND_FL_RUN_CB) || (dev->flags&FEND_FL_LOCK))
		{
			pthread_cond_wait(&dev->cond, &dev->lock);
		}
	}
	AM_DEBUG(1, "AM_FEND_Lock line:%d\n",__LINE__);
	
	dev->cb = lockp.old_cb;
	dev->user_data = lockp.old_data;
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief 设定前端管理线程的检测间隔
 * \param dev_no 前端设备号
 * \param delay 间隔时间(单位为毫秒)，0表示没有间隔，<0表示前端管理线程暂停工作
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend.h)
 */
AM_ErrorCode_t AM_FEND_SetThreadDelay(int dev_no, int delay)
{
	AM_FEND_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	AM_TRY(fend_get_openned_dev(dev_no, &dev));
	
	if(!dev->drv->set_para)
	{
		AM_DEBUG(1, "fronend %d no not support set_para", dev_no);
		return AM_FEND_ERR_NOT_SUPPORTED;
	}
	
	if(dev->thread==pthread_self())
	{
		AM_DEBUG(1, "cannot invoke AM_FEND_Lock in callback");
		return AM_FEND_ERR_INVOKE_IN_CB;
	}
	
	pthread_mutex_lock(&dev->lock);
	
	if(dev->drv->set_delay)
		ret = dev->drv->set_delay(dev, delay);
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief 重置数字卫星设备控制
 * \param dev_no 前端设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend.h)
 */
AM_ErrorCode_t AM_FEND_DiseqcResetOverload(int dev_no)
{
	AM_FEND_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	AM_TRY(fend_get_openned_dev(dev_no, &dev));
	
	if(!dev->drv->diseqc_reset_overload)
	{
		AM_DEBUG(1, "fronend %d no not support diseqc_reset_overload", dev_no);
		return AM_FEND_ERR_NOT_SUPPORTED;
	}
	
	if(dev->thread==pthread_self())
	{
		AM_DEBUG(1, "cannot invoke AM_FEND_DiseqcResetOverload in callback");
		return AM_FEND_ERR_INVOKE_IN_CB;
	}
	
	pthread_mutex_lock(&dev->lock);
	
	if(dev->drv->diseqc_reset_overload)
		ret = dev->drv->diseqc_reset_overload(dev);
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief 发送数字卫星设备控制命令
 * \param dev_no 前端设备号 
 * \param[in] cmd 数字卫星设备控制命令
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend.h)
 */
AM_ErrorCode_t AM_FEND_DiseqcSendMasterCmd(int dev_no, struct dvb_diseqc_master_cmd* cmd)
{
	AM_FEND_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;

	assert(cmd);
	
	AM_TRY(fend_get_openned_dev(dev_no, &dev));
	
	if(!dev->drv->diseqc_send_master_cmd)
	{
		AM_DEBUG(1, "fronend %d no not support diseqc_send_master_cmd", dev_no);
		return AM_FEND_ERR_NOT_SUPPORTED;
	}
	
	if(dev->thread==pthread_self())
	{
		AM_DEBUG(1, "cannot invoke AM_FEND_DiseqcSendMasterCmd in callback");
		return AM_FEND_ERR_INVOKE_IN_CB;
	}
	
	pthread_mutex_lock(&dev->lock);
	
	if(dev->drv->diseqc_send_master_cmd)
		ret = dev->drv->diseqc_send_master_cmd(dev, cmd);
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief 接收数字卫星设备控制2.0命令回应
 * \param dev_no 前端设备号 
 * \param[out] reply 数字卫星设备控制回应
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend.h)
 */
AM_ErrorCode_t AM_FEND_DiseqcRecvSlaveReply(int dev_no, struct dvb_diseqc_slave_reply* reply)
{
	AM_FEND_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;

	assert(reply);
	
	AM_TRY(fend_get_openned_dev(dev_no, &dev));
	
	if(!dev->drv->diseqc_recv_slave_reply)
	{
		AM_DEBUG(1, "fronend %d no not support diseqc_recv_slave_reply", dev_no);
		return AM_FEND_ERR_NOT_SUPPORTED;
	}
	
	if(dev->thread==pthread_self())
	{
		AM_DEBUG(1, "cannot invoke AM_FEND_DiseqcRecvSlaveReply in callback");
		return AM_FEND_ERR_INVOKE_IN_CB;
	}
	
	pthread_mutex_lock(&dev->lock);
	
	if(dev->drv->diseqc_recv_slave_reply)
		ret = dev->drv->diseqc_recv_slave_reply(dev, reply);
	
	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief 发送数字卫星设备控制tone burst
 * \param dev_no 前端设备号 
 * \param tone burst控制方式

 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend.h)
 */
AM_ErrorCode_t AM_FEND_DiseqcSendBurst(int dev_no, fe_sec_mini_cmd_t minicmd)
{
	AM_FEND_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
 	
	AM_TRY(fend_get_openned_dev(dev_no, &dev));
	
	if(!dev->drv->diseqc_send_burst)
	{
		AM_DEBUG(1, "fronend %d no not support diseqc_send_burst", dev_no);
		return AM_FEND_ERR_NOT_SUPPORTED;
	}
	
	if(dev->thread==pthread_self())
	{
		AM_DEBUG(1, "cannot invoke AM_FEND_DiseqcSendBurst in callback");
		return AM_FEND_ERR_INVOKE_IN_CB;
	}
	
	pthread_mutex_lock(&dev->lock);
	
	if(dev->drv->diseqc_send_burst)
		ret = dev->drv->diseqc_send_burst(dev, minicmd);
	
	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief 设置卫星设备tone模式
 * \param dev_no 前端设备号 
 * \param tone 卫星设备tone模式
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend.h)
 */
AM_ErrorCode_t AM_FEND_SetTone(int dev_no, fe_sec_tone_mode_t tone)
{
	AM_FEND_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
 	
	AM_TRY(fend_get_openned_dev(dev_no, &dev));
	
	if(!dev->drv->set_tone)
	{
		AM_DEBUG(1, "fronend %d no not support set_tone", dev_no);
		return AM_FEND_ERR_NOT_SUPPORTED;
	}
	
	if(dev->thread==pthread_self())
	{
		AM_DEBUG(1, "cannot invoke AM_FEND_SetTone in callback");
		return AM_FEND_ERR_INVOKE_IN_CB;
	}
	
	pthread_mutex_lock(&dev->lock);
	
	if(dev->drv->set_tone)
		ret = dev->drv->set_tone(dev, tone);
	
	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief 设置卫星设备控制电压
 * \param dev_no 前端设备号 
 * \param voltage 卫星设备控制电压 
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend.h)
 */
AM_ErrorCode_t AM_FEND_SetVoltage(int dev_no, fe_sec_voltage_t voltage)
{
	AM_FEND_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
 	
	AM_TRY(fend_get_openned_dev(dev_no, &dev));
	
	if(!dev->drv->set_voltage)
	{
		AM_DEBUG(1, "fronend %d no not support set_voltage", dev_no);
		return AM_FEND_ERR_NOT_SUPPORTED;
	}
	
	if(dev->thread==pthread_self())
	{
		AM_DEBUG(1, "cannot invoke AM_FEND_SetVoltage in callback");
		return AM_FEND_ERR_INVOKE_IN_CB;
	}
	
	pthread_mutex_lock(&dev->lock);
	
	if(dev->drv->set_voltage)
		ret = dev->drv->set_voltage(dev, voltage);
	
	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief 控制卫星设备LNB高电压
 * \param dev_no 前端设备号 
 * \param arg 0表示禁止，!=0表示允许
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend.h)
 */
AM_ErrorCode_t AM_FEND_EnableHighLnbVoltage(int dev_no, long arg)
{
	AM_FEND_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
 	
	AM_TRY(fend_get_openned_dev(dev_no, &dev));
	
	if(!dev->drv->enable_high_lnb_voltage)
	{
		AM_DEBUG(1, "fronend %d no not support enable_high_lnb_voltage", dev_no);
		return AM_FEND_ERR_NOT_SUPPORTED;
	}
	
	if(dev->thread==pthread_self())
	{
		AM_DEBUG(1, "cannot invoke AM_FEND_EnableHighLnbVoltage in callback");
		return AM_FEND_ERR_INVOKE_IN_CB;
	}
	
	pthread_mutex_lock(&dev->lock);
	
	if(dev->drv->enable_high_lnb_voltage)
		ret = dev->drv->enable_high_lnb_voltage(dev, arg);
	
	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief 卫星盲扫开始  
 * \param dev_no 前端设备号
 * \param[in] cb 盲扫回调函数
 * \param[in] user_data 状态回调函数的参数
 * \param start_freq 开始频点 unit HZ
 * \param stop_freq 结束频点 unit HZ
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend.h)
 */
AM_ErrorCode_t AM_FEND_BlindScan(int dev_no, AM_FEND_BlindCallback_t cb, void *user_data, unsigned int start_freq, unsigned int stop_freq)
{
	AM_FEND_Device_t *dev = NULL;
	AM_ErrorCode_t ret = AM_SUCCESS;
	int rc;

	if(start_freq == stop_freq)
	{
		AM_DEBUG(1, "AM_FEND_BlindScan start_freq equal stop_freq\n");
		ret = AM_FEND_ERR_BLINDSCAN;
		return ret;
	}

	/*this function set the parameters blind scan process needed.*/	
	AM_FEND_IBlindScanAPI_Initialize(dev_no);

	AM_FEND_IBlindScanAPI_SetFreqRange(dev_no, start_freq/1000, stop_freq/1000);

	/*blindscan handle thread*/	
	AM_TRY(fend_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&am_gAdpLock);

	if(cb!=dev->blindscan_cb || user_data!=dev->blindscan_cb_user_data)
	{
		dev->blindscan_cb = cb;
		dev->blindscan_cb_user_data = user_data;
	}

	dev->enable_blindscan_thread = AM_TRUE;
	
	rc = pthread_create(&dev->blindscan_thread, NULL, fend_blindscan_thread, (void *)(long)dev_no);
	if(rc)
	{
		AM_DEBUG(1, "%s", strerror(rc));
		ret = AM_FEND_ERR_BLINDSCAN;
	}

	pthread_mutex_unlock(&am_gAdpLock);
	
	return ret;
}

/**\brief 卫星盲扫结束
 * \param dev_no 前端设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend.h)
 */
AM_ErrorCode_t AM_FEND_BlindExit(int dev_no)
{
	AM_FEND_Device_t *dev = NULL;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	AM_TRY(fend_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&am_gAdpLock);
	
	/*Stop the thread*/
	dev->enable_blindscan_thread = AM_FALSE;
	pthread_join(dev->blindscan_thread, NULL);
	
	pthread_mutex_unlock(&am_gAdpLock);

	return ret;
}


/**\brief 卫星盲扫进度 
 * \param dev_no 前端设备号
 * \param[out] process 盲扫进度0-100
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend.h)
 */
AM_ErrorCode_t AM_FEND_BlindGetProcess(int dev_no, unsigned int *process)
{
	AM_ErrorCode_t ret = AM_SUCCESS;

	assert(process);

	*process = (unsigned int)AM_FEND_IBlindscanAPI_GetProgress(dev_no);

	return ret;
}

/**\brief 卫星盲扫信息
 * \param dev_no 前端设备号
 * \param[in out] para in 盲扫频点信息缓存区大小，out 盲扫频点个数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend.h)
 */
AM_ErrorCode_t AM_FEND_BlindGetTPCount(int dev_no, unsigned int *count)
{
	AM_FEND_Device_t *dev = NULL;
	AM_ErrorCode_t ret = AM_SUCCESS;

	assert(count);

	AM_TRY(fend_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);

    *count = 0;
	if(dev->bs_setting.m_uiChannelCount)
	{
		*count = (unsigned int)(dev->bs_setting.m_uiChannelCount);
	}

	pthread_mutex_unlock(&dev->lock);

	return ret;
}


/**\brief 卫星盲扫信息 
 * \param dev_no 前端设备号
 * \param[out] para 盲扫频点信息缓存区
 * \param[in out] para in 盲扫频点信息缓存区大小，out 盲扫频点个数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend.h)
 */
AM_ErrorCode_t AM_FEND_BlindGetTPInfo(int dev_no, struct dvb_frontend_parameters *para, unsigned int count)
{
	AM_FEND_Device_t *dev = NULL;
	AM_ErrorCode_t ret = AM_SUCCESS;

	assert(para);
		
	AM_TRY(fend_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);

	if(count > dev->bs_setting.m_uiChannelCount)
	{
		count = (unsigned int)(dev->bs_setting.m_uiChannelCount);
	}
	
	memcpy(para, dev->bs_setting.channels, count * sizeof(struct dvb_frontend_parameters));

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief 模拟微调
 *\param dev_no 前端设备号
 *\param freq 频率，单位为Hz
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend.h)
 */
AM_ErrorCode_t AM_FEND_FineTune(int dev_no, unsigned int freq)
{
	AM_FEND_Device_t *dev = NULL;
	AM_ErrorCode_t ret = AM_SUCCESS;

	AM_TRY(fend_get_openned_dev(dev_no, &dev));

	if(!dev->drv->fine_tune)
	{
		AM_DEBUG(1, "fronend %d no not support fine_tune", dev_no);
		return AM_FEND_ERR_NOT_SUPPORTED;
	}

	pthread_mutex_lock(&dev->lock);

	ret = dev->drv->fine_tune(dev, freq);

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief 模拟CVBS AMP OUT
 *\param dev_no 前端设备号
 *\param amp ，单位为int
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend.h)
 */
AM_ErrorCode_t AM_FEND_SetCvbsAmpOut(int dev_no, unsigned int amp)
{
	AM_FEND_Device_t *dev = NULL;
	AM_ErrorCode_t ret = AM_SUCCESS;

	AM_TRY(fend_get_openned_dev(dev_no, &dev));

	if(!dev->drv->set_cvbs_amp_out)
	{
		AM_DEBUG(1, "fronend %d no not support cvbs amp out", dev_no);
		return AM_FEND_ERR_NOT_SUPPORTED;
	}

	pthread_mutex_lock(&dev->lock);
    tuner_param_t tuner_para;
    tuner_para.cmd = TUNER_CMD_SET_CVBS_AMP_OUT ;
    tuner_para.parm = amp;
	ret = dev->drv->set_cvbs_amp_out(dev, &tuner_para);
	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief 模拟get atv status
 *\param dev_no 前端设备号
 *\param atv_status ，单位为atv_status_t
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend.h)
 */
AM_ErrorCode_t AM_FEND_GetAtvStatus(int dev_no,  atv_status_t *atv_status)
{
	AM_FEND_Device_t *dev = NULL;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	AM_TRY(fend_get_openned_dev(dev_no, &dev));
	
	if(!dev->drv->get_atv_status)
	{
		AM_DEBUG(1, "fronend %d no not support get atv status", dev_no);
		return AM_FEND_ERR_NOT_SUPPORTED;
	}
	
	pthread_mutex_lock(&dev->lock);
	ret = dev->drv->get_atv_status(dev, atv_status);
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

AM_ErrorCode_t AM_FEND_SetAfc(int dev_no, unsigned int afc)
{
	AM_FEND_Device_t *dev = NULL;
	AM_ErrorCode_t ret = AM_SUCCESS;

	AM_TRY(fend_get_openned_dev(dev_no, &dev));

	if(!dev->drv->set_afc)
	{
		AM_DEBUG(1, "fronend %d no not support set_afc", dev_no);
		return AM_FEND_ERR_NOT_SUPPORTED;
	}

	pthread_mutex_lock(&dev->lock);

	ret = dev->drv->set_afc(dev, afc);

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

AM_ErrorCode_t AM_FEND_SetSubSystem(int dev_no, unsigned int sub_sys)
{
	AM_ErrorCode_t ret = AM_SUCCESS;
	struct dtv_property p = {.cmd = DTV_DELIVERY_SUB_SYSTEM, .u.data = sub_sys};
	struct dtv_properties props = {.num = 1, .props = &p};
	ret = AM_FEND_SetProp(dev_no, &props);
	return ret;
}

AM_ErrorCode_t AM_FEND_GetSubSystem(int dev_no, unsigned int *sub_sys)
{
	AM_ErrorCode_t ret = AM_SUCCESS;
	struct dtv_property p = {.cmd = DTV_DELIVERY_SUB_SYSTEM, .u.data = 0/*sub_sys*/};
	struct dtv_properties props = {.num = 1, .props = &p};
	ret = AM_FEND_GetProp(dev_no, &props);
	*sub_sys = p.u.data;
	return ret;
}

