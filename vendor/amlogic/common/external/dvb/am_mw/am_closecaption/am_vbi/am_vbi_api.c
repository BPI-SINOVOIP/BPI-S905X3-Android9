/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file
 * \brief
 *
 * \author  <jianhui.zhou@amlogic.com>
 * \date 2010-05-21: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 1

#include <am_debug.h>
#include "am_vbi_internal.h"
#include "am_vbi.h"
#include <string.h>
#include <assert.h>

/****************************************************************************
 * Macro definitions
 ***************************************************************************/
#if 0
#undef AM_DEBUG
#define AM_DEBUG(level,arg...) printf(arg)
#endif

#define VBI_SYNC

#define VBI_BUF_SIZE       (4096)
#define VBI_POLL_TIMEOUT (500) //  (200*10)

#define VBI_DEV_COUNT      (2)


#define VBI_CHAN_ISSET_FILTER(chan,fid)    ((chan)->filter_mask[(fid)>>3]&(1<<((fid)&3)))
#define VBI_CHAN_SET_FILTER(chan,fid)      ((chan)->filter_mask[(fid)>>3]|=(1<<((fid)&3)))
#define VBI_CHAN_CLR_FILTER(chan,fid)      ((chan)->filter_mask[(fid)>>3]&=~(1<<((fid)&3)))


/****************************************************************************
 * Static data
 ***************************************************************************/


extern const  AM_VBI_Driver_t linux_vbi_drv ;

pthread_mutex_t am_gAdpLock  = PTHREAD_MUTEX_INITIALIZER;

static AM_VBI_Device_t vbi_devices[VBI_DEV_COUNT] =
{

{
.drv = &linux_vbi_drv,
.src = DUAL_FD1,
},
{
.drv = &linux_vbi_drv,
.src = DUAL_FD2,
}
};

/****************************************************************************
 * Static functions
 ***************************************************************************/
 
/**\brief get device pointer depend on dev_no */
static inline AM_ErrorCode_t vbi_get_dev(int dev_no, AM_VBI_Device_t **dev)
{
	if((dev_no<0) || (dev_no>=VBI_DEV_COUNT))
	{
		AM_DEBUG(1,"invalid demux device number %d, must in(%d~%d)", dev_no, 0, VBI_DEV_COUNT-1);
		return AM_VBI_ERR_INVALID_DEV_NO;
	}
	
	*dev = &vbi_devices[dev_no];
	return AM_SUCCESS;
}

/**\brief check whether vbi_device open or not depend dev_no*/
static inline AM_ErrorCode_t vbi_get_openned_dev(int dev_no, AM_VBI_Device_t **dev)
{
	int ret = (vbi_get_dev(dev_no, dev));
	
	if(!(*dev)->openned)
	{
		AM_DEBUG(1,"demux device %d has not been openned", dev_no);
		return AM_VBI_ERR_INVALID_DEV_NO;
	}
	
	return AM_SUCCESS;
}

/**\brief get vbi_dev filter according filter_id as well checking whether that filter is used or not */
static inline AM_ErrorCode_t vbi_get_used_filter(AM_VBI_Device_t *dev, int filter_id, AM_VBI_Filter_t **pf)
{
	AM_VBI_Filter_t *filter;
	
	if((filter_id<0) || (filter_id>=VBI_FILTER_COUNT))
	{
		AM_DEBUG(1,"invalid filter id, must in %d~%d", 0, VBI_FILTER_COUNT-1);
		return AM_VBI_ERR_INVALID_ID;
	}
	
	filter = &dev->filters[filter_id];
	
	if(!filter->used)
	{
		AM_DEBUG(1,"filter %d has not been allocated", filter_id);
		return AM_VBI_ERR_NOT_ALLOCATED;
	}
	
	*pf = filter;
	return AM_SUCCESS;
}

/**\brief data polling thread*/
static void* vbi_data_thread(void *arg)
{
	AM_VBI_Device_t *dev = (AM_VBI_Device_t*)arg;
	static uint8_t sec_buf[128];
	uint8_t *sec;
	int sec_len;
	AM_VBI_FilterMask_t mask;
	AM_ErrorCode_t ret;
	int count = 0;

	while(dev->enable_thread)
	{
		AM_DEBUG(1,"***************!!!!thread count = %d\n",count++);
		
		AM_VBI_FILTER_MASK_CLEAR(&mask);
		int id;
		
		ret = dev->drv->poll(dev, &mask, VBI_POLL_TIMEOUT);
		if(ret==AM_SUCCESS)
		{
			if(AM_VBI_FILTER_MASK_ISEMPTY(&mask))
				goto POLL_END;

#if defined(VBI_WAIT_CB) || defined(VBI_SYNC)
			pthread_mutex_lock(&dev->lock);
			dev->flags |= VBI_FL_RUN_CB;
			pthread_mutex_unlock(&dev->lock);
#endif
				
			for(id=0; id<VBI_FILTER_COUNT; id++)
			{
				AM_VBI_Filter_t *filter=&dev->filters[id];
				AM_VBI_DataCb cb;
				void *data;
				
				if(!AM_VBI_FILTER_MASK_ISSET(&mask, id))
					goto POLL_END;
				
				if(!filter->enable || !filter->used)
					goto POLL_END;
				
				sec_len = sizeof(sec_buf);

#ifndef VBI_WAIT_CB
				pthread_mutex_lock(&dev->lock);
#endif
				if(!filter->enable || !filter->used)
				{
					ret = AM_FAILURE;
				}
				else
				{
					cb   = filter->cb;
					data = filter->user_data;
					ret  = dev->drv->read(dev, filter, sec_buf, &sec_len);
				}
#ifndef VBI_WAIT_CB
				pthread_mutex_unlock(&dev->lock);
#endif
				if(ret==AM_VBI_ERR_TIMEOUT)
				{
					sec = NULL;
					sec_len = 0;
				}
				else if(ret!=AM_SUCCESS)
				{
					goto POLL_END;
				}
				else
				{
					sec = sec_buf;
				}
			        
                               
				if(cb)
				{
					if(sec)
					   AM_DEBUG(1,"\nfilter %d data callback len fd:%d len:%d, %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
						id, (int)filter->drv_data, sec_len,
						sec[0], sec[1], sec[2], sec[3], sec[4],
						sec[5], sec[6], sec[7], sec[8], sec[9]);
					cb(dev->dev_no, id, sec, sec_len, data);
					if(sec)
					   AM_DEBUG(1,"\nfilter %d data callback ok\n", id);
				}
			}
#if defined(VBI_WAIT_CB) || defined(VBI_SYNC)
			pthread_mutex_lock(&dev->lock);
			dev->flags &= ~VBI_FL_RUN_CB;
			pthread_mutex_unlock(&dev->lock);
			pthread_cond_broadcast(&dev->cond);
#endif
		}
		else
	              AM_DEBUG(1,"poll fail \n");
                POLL_END:
                        usleep(1000*50);
	}
	
	return NULL;
}

/**\brief wait for callback completing action*/
static inline AM_ErrorCode_t vbi_wait_cb(AM_VBI_Device_t *dev)
{
#ifdef VBI_WAIT_CB
	if(dev->thread!=pthread_self())
	{
		while(dev->flags&VBI_FL_RUN_CB)
			pthread_cond_wait(&dev->cond, &dev->lock);
	}
#endif
	return AM_SUCCESS;
}

/**\brief stop vbi fileter */
static AM_ErrorCode_t vbi_stop_filter(AM_VBI_Device_t *dev, AM_VBI_Filter_t *filter)
{
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	if(!filter->used || !filter->enable)
	{
		return ret;
	}
	
	if(dev->drv->enable_filter)
	{
		ret = dev->drv->enable_filter(dev, filter, AM_FALSE);
	}
	
	if(ret>=0)
	{
		filter->enable = AM_FALSE;
	}
	
	return ret;
}

/**\brief free vbi device filter*/
static int vbi_free_filter(AM_VBI_Device_t *dev, AM_VBI_Filter_t *filter)
{
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	if(!filter->used)
		return ret;
		
	ret = vbi_stop_filter(dev, filter);
	
	if(ret==AM_SUCCESS)
	{
		if(dev->drv->free_filter)
		{
			ret = dev->drv->free_filter(dev, filter);
		}
	}
	
	if(ret==AM_SUCCESS)
	{
		filter->used=AM_FALSE;
	}
	
	return ret;
}

/****************************************************************************
 * API functions
 ***************************************************************************/

/**\brief open VBI device
 * \param dev_no  VBI device id
 * \param[in] para vbi device parameter
 * \return
 *   - AM_SUCCESS 
 *   - other return value please refer am_vbi.h
 */
AM_ErrorCode_t AM_NTSC_VBI_Open(int dev_no, const AM_VBI_OpenPara_t *para)
{
	AM_VBI_Device_t *dev = NULL ;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	assert(para);
	
	ret = (vbi_get_dev(dev_no, &dev));
	
	pthread_mutex_lock(&am_gAdpLock);
	
	if(dev->openned)
	{
	        AM_DEBUG(1,"demux device %d has already been openned", dev_no);
		ret = AM_VBI_ERR_BUSY;
		goto final;
	}
	
	dev->dev_no = dev_no;
	
	if(dev->drv->open)
	{
		ret = dev->drv->open(dev, para);
	}
	
	if(ret==AM_SUCCESS)
	{
		pthread_mutex_init(&dev->lock, NULL);
		pthread_cond_init(&dev->cond, NULL);
		dev->enable_thread = AM_TRUE;
		dev->flags = 0;
		
		if(pthread_create(&dev->thread, NULL, vbi_data_thread, dev))
		{
			pthread_mutex_destroy(&dev->lock);
			pthread_cond_destroy(&dev->cond);
			ret = AM_VBI_ERR_CANNOT_CREATE_THREAD;
		}
	}
	
	if(ret==AM_SUCCESS)
	{
		dev->openned = AM_TRUE;
	}
final:
	pthread_mutex_unlock(&am_gAdpLock);
	
	return ret;
}

/**\brief close VBI device
 * \param dev_no VBI device id
 * \return
 *   - AM_SUCCESS 
 *   - other return value please refer am_vbi.h
 */
AM_ErrorCode_t AM_NTSC_VBI_Close(int dev_no)
{
	AM_VBI_Device_t *dev = NULL;
	AM_ErrorCode_t ret = AM_SUCCESS;
	int i;
	
	ret = (vbi_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&am_gAdpLock);

	dev->enable_thread = AM_FALSE;
	pthread_join(dev->thread, NULL);

	for(i=0; i<VBI_FILTER_COUNT; i++)
	{
		vbi_free_filter(dev, &dev->filters[i]);
	}

	if(dev->drv->close)
	{
		dev->drv->close(dev);
	}

	pthread_mutex_destroy(&dev->lock);
	pthread_cond_destroy(&dev->cond);
	
	dev->openned = AM_FALSE;
	
	pthread_mutex_unlock(&am_gAdpLock);
	
	return ret;
}

/**\brief allocate one filter for VBI device
 * \param dev_no VBI device id
 * \param[out] fhandle for filter allocated successfuly
 * \return
 *   - AM_SUCCESS  
 *   - other return value please refer am_vbi.h
 */
AM_ErrorCode_t AM_NTSC_VBI_AllocateFilter(int dev_no, int *fhandle)
{
	AM_VBI_Device_t *dev = NULL;
	AM_ErrorCode_t ret = AM_SUCCESS;
	int fid;
	
	assert(fhandle);
	
	ret = (vbi_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	
	for(fid=0; fid<VBI_FILTER_COUNT; fid++)
	{
		AM_DEBUG(1,"dev->filters[fid].used = %d\n",dev->filters[fid].used);
		if(!dev->filters[fid].used)
			break;
	}
	
	if(fid>=VBI_FILTER_COUNT)
	{
		AM_DEBUG(1,"no free section filter");
		ret = AM_VBI_ERR_NO_FREE_FILTER;
	}
	
	if(ret==AM_SUCCESS)
	{
		AM_DEBUG(1,"AM_NTSC_VBI_AllocateFilter AM_SUCCESS fid = %d\n",fid);
		vbi_wait_cb(dev);
		
		dev->filters[fid].id   = fid;
		if(dev->drv->alloc_filter)
		{
			ret = dev->drv->alloc_filter(dev, &dev->filters[fid]);
		}
	}
	
	if(ret==AM_SUCCESS)
	{
		dev->filters[fid].used = AM_TRUE;
		*fhandle = fid;
		
		AM_DEBUG(1,"allocate filter %d \n", fid);
	}
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}


/**\brief free filter allocated
 * \param dev_no VBI device id
 * \param fhandle hanlde of VBI filter
 * \return
 *   - AM_SUCCESS  
 *   - other return value please refer am_vbi.h
 */
AM_ErrorCode_t AM_NTSC_VBI_FreeFilter(int dev_no, int fhandle)
{
	AM_VBI_Device_t *dev = NULL;
	AM_VBI_Filter_t *filter;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	ret = (vbi_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	
	ret = vbi_get_used_filter(dev, fhandle, &filter);
	
	if(ret==AM_SUCCESS)
	{
		vbi_wait_cb(dev);
		ret = vbi_free_filter(dev, filter);
		AM_DEBUG(1,"free filter %d", fhandle);
	}
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief  get VBI filter start work
 * \param dev_no VBI device id
 * \param fhandle hanlde of VBI filter
 * \return
 *   - AM_SUCCESS  
 *   - other return value please refer am_vbi.h
 */
AM_ErrorCode_t AM_NTSC_VBI_StartFilter(int dev_no, int fhandle)
{
	AM_DEBUG(1,"AM_NTSC_VBI_StartFilter*************\n");
	AM_VBI_Device_t *dev = NULL;
	AM_VBI_Filter_t *filter = NULL;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	ret = (vbi_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	
	ret = vbi_get_used_filter(dev, fhandle, &filter);
	
	if(!filter->enable)
	{
		if(ret==AM_SUCCESS)
		{
			if(dev->drv->enable_filter)
			{
				ret = dev->drv->enable_filter(dev, filter, AM_TRUE);
			}
		}
		
		if(ret==AM_SUCCESS)
		{
			filter->enable = AM_TRUE;
		}
	}
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief stop appointed VBI filter
 * \param dev_no VBI device id
 * \param fhandle hanlde of VBI filter
 * \return
 *   - AM_SUCCESS  
 *   - other return value please refer am_vbi.h
 */
AM_ErrorCode_t AM_NTSC_VBI_StopFilter(int dev_no, int fhandle)
{
	AM_VBI_Device_t *dev = NULL;
	AM_VBI_Filter_t *filter = NULL;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	ret = (vbi_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	
	ret = vbi_get_used_filter(dev, fhandle, &filter);
	
	if(filter->enable)
	{
		if(ret==AM_SUCCESS)
		{
			vbi_wait_cb(dev);
			ret = vbi_stop_filter(dev, filter);
		}
	}
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief set VBI dev ringbuf size
 * \param dev_no VBI device id
 * \param fhandle hanlde of VBI filter
 * \param size : size of VBI ringbuf
 * \return
 *   - AM_SUCCESS  
 *   -  other return value please refer am_vbi.h
 */
AM_ErrorCode_t AM_NTSC_VBI_SetBufferSize(int dev_no, int fhandle, int size)
{
	AM_VBI_Device_t *dev = NULL;
	AM_VBI_Filter_t *filter;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	ret = (vbi_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	
	if(!dev->drv->set_buf_size)
	{
		AM_DEBUG(1,"do not support set_buf_size");
		ret = AM_VBI_ERR_NOT_SUPPORTED;
	}
	
	if(ret==AM_SUCCESS)
		ret = vbi_get_used_filter(dev, fhandle, &filter);
	
	if(ret==AM_SUCCESS)
		ret = dev->drv->set_buf_size(dev, filter, size);
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief get callback and useparam for filter depend filter handle and device id
 * \param dev_no VBI device id
 * \param fhandle hanlde of VBI filter
 * \param[out] cb callback which filter hold
 * \param[out] data user param to return 
 * \return
 *   - AM_SUCCESS  
 *   - other return value please refer am_vbi.h
 */
AM_ErrorCode_t AM_NTSC_VBI_GetCallback(int dev_no, int fhandle, AM_VBI_DataCb *cb, void **data)
{
	AM_VBI_Device_t *dev = NULL;
	AM_VBI_Filter_t *filter;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	ret = (vbi_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	
	ret = vbi_get_used_filter(dev, fhandle, &filter);
	
	if(ret==AM_SUCCESS)
	{
		if(cb)
			*cb = filter->cb;
	
		if(data)
			*data = filter->user_data;
	}
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief set callback and user param for appointed filter
 * \param dev_no VBI device id
 * \param fhandle hanlde of VBI filter
 * \param[in] cb callback to be set for filter
 * \param[in] data user param for callback
 * \return
 *   - AM_SUCCESS  
 *   - other return value please refer am_vbi.h
 */
AM_ErrorCode_t AM_NTSC_VBI_SetCallback(int dev_no, int fhandle, AM_VBI_DataCb cb, void *data)
{
	AM_DEBUG(1,"AM_NTSC_VBI_SetCallback fhandle = %d\n",fhandle);
	AM_VBI_Device_t *dev = NULL;
	AM_VBI_Filter_t *filter;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	ret = (vbi_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	
	ret = vbi_get_used_filter(dev, fhandle, &filter);
	
	if(ret==AM_SUCCESS)
	{
		AM_DEBUG(1,"AM_NTSC_VBI_SetCallback AM_SUCCESS\n");
		vbi_wait_cb(dev);
	
		filter->cb = cb;
		filter->user_data = data;
	}
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}



/**\brief VBI sync to wait for callback complete data comsume
 * \param dev_no VBI device id
 * \return
 *   - AM_SUCCESS  
 *   - other return value please refer am_vbi.h
 */
AM_ErrorCode_t AM_NTSC_VBI_Sync(int dev_no)
{
	AM_VBI_Device_t *dev = NULL;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	ret = (vbi_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	if(dev->thread!=pthread_self())
	{
		while(dev->flags&VBI_FL_RUN_CB)
			pthread_cond_wait(&dev->cond, &dev->lock);
	}
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

