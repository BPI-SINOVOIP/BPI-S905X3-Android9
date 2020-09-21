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
 * \brief
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-06-30: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 5

#include <am_debug.h>
#include <am_time.h>
#include <am_mem.h>
#include <am_misc.h>
#include "am_smc_internal.h"
#include "../am_adp_internal.h"
#include <assert.h>
#include <unistd.h>

/****************************************************************************
 * Macro definitions
 ***************************************************************************/
#define SMC_STATUS_SLEEP_TIME (50)

/****************************************************************************
 * Static data
 ***************************************************************************/

#ifdef EMU_SMC
extern const AM_SMC_Driver_t emu_drv;
#else
extern const AM_SMC_Driver_t aml_smc_drv;
#endif

static AM_SMC_Device_t smc_devices[] =
{
#ifdef EMU_SMC
{
.drv = &emu_drv
}
#else
{
.drv = &aml_smc_drv
}
#endif
};

/**\brief 智能卡设备数*/
#define SMC_DEV_COUNT    AM_ARRAY_SIZE(smc_devices)
#define SMC_BUF_SIZE      (1024)

struct ringbuffer {
	uint8_t         *data;
	int              size;
	int              pread;
	int              pwrite;
	int              error;
};
static uint8_t smc_txbuf[SMC_BUF_SIZE] = {
 0
};
static uint8_t smc_rxbuf[SMC_BUF_SIZE] = {
 0
};
static struct ringbuffer txbuf;
static struct ringbuffer rxbuf;

static void ringbuffer_init(struct ringbuffer *rbuf,uint8_t* buff)
{                                                                                        
	rbuf->pread=rbuf->pwrite=0;                                                             
	rbuf->data=buff;                                                                        
	rbuf->size=SMC_BUF_SIZE;                                                                         
	rbuf->error=0;                                                                          
} 

static void ringbuffer_read(struct ringbuffer *rbuf, uint8_t *buf, int len)
{                                                                                        
	int todo = len;                                                                      
	int split;                                                                           
                                                                                         
	split = (rbuf->pread + len > rbuf->size) ? rbuf->size - rbuf->pread : 0;                
	if (split > 0) {                                                                        
		memcpy(buf, rbuf->data+rbuf->pread, split);                                            
		buf += split;                                                                          
		todo -= split;                                                                         
		rbuf->pread = 0;                                                                       
	}                                                                                       
	memcpy(buf, rbuf->data+rbuf->pread, todo);                                              
                                                                                         
	rbuf->pread = (rbuf->pread + todo) % rbuf->size;                                        
}                                                                                        
                                                                                         
                                                                                         
static int ringbuffer_write(struct ringbuffer *rbuf, const uint8_t *buf, int len)
{                                                                                        
	int todo = len;                                                                      
	int split;                                                                           
                                                                                         
	split = (rbuf->pwrite + len > rbuf->size) ? rbuf->size - rbuf->pwrite : 0;              
                                                                                         
	if (split > 0) {                                                                        
		memcpy(rbuf->data+rbuf->pwrite, buf, split);                                           
		buf += split;                                                                          
		todo -= split;                                                                         
		rbuf->pwrite = 0;                                                                      
	}                                                                                       
	memcpy(rbuf->data+rbuf->pwrite, buf, todo);                                             
	rbuf->pwrite = (rbuf->pwrite + todo) % rbuf->size;                                      
	return len;                                                                             
}
                                                        
int ringbuffer_avail(struct ringbuffer *rbuf)
{
	int avail;

	avail = rbuf->pwrite - rbuf->pread;
	if (avail < 0)
		avail += rbuf->size;
	return avail;
}

void ringbuffer_flush(struct ringbuffer *rbuf)
{
	rbuf->pread = rbuf->pwrite;
	rbuf->error = 0;
}
void ringbuffer_reset(struct ringbuffer *rbuf)
{	
	rbuf->pread = rbuf->pwrite = 0;	
	rbuf->error = 0;
}


/****************************************************************************
 * Static functions
 ***************************************************************************/

/**\brief 根据设备号取得设备结构*/
static AM_INLINE AM_ErrorCode_t smc_get_dev(int dev_no, AM_SMC_Device_t **dev)
{
	if((dev_no<0) || (((size_t)dev_no)>=SMC_DEV_COUNT))
	{
		AM_DEBUG(1, "invalid smartcard device number %d, must in(%d~%d)", dev_no, 0, SMC_DEV_COUNT-1);
		return AM_SMC_ERR_INVALID_DEV_NO;
	}
	
	*dev = &smc_devices[dev_no];
	return AM_SUCCESS;
}

/**\brief 根据设备号取得设备结构并检查设备是否已经打开*/
static AM_INLINE AM_ErrorCode_t smc_get_openned_dev(int dev_no, AM_SMC_Device_t **dev)
{
	AM_TRY(smc_get_dev(dev_no, dev));
	
	if(!(*dev)->openned)
	{
		AM_DEBUG(1, "smartcard device %d has not been openned", dev_no);
		return AM_SMC_ERR_NOT_OPENNED;
	}
	
	return AM_SUCCESS;
}

/**\brief 从智能卡读取数据*/
static AM_ErrorCode_t smc_read(AM_SMC_Device_t *dev, uint8_t *buf, int len, int *act_len, int timeout)
{
	uint8_t *ptr = buf;
	int left = len;
	int now, end = 0, diff, cnt = 0;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	if(timeout>=0)
	{
		AM_TIME_GetClock(&now);
		end = now + timeout;
	}
	
	while (left)
	{
		int tlen = left;
		int ms;
		
		if(timeout>=0)
		{
			ms = end-now;
		}
		else
		{
			ms = -1;
		}
		
		ret = dev->drv->read(dev, ptr, &tlen, ms);
		
		if(ret<0)
		{
			break;
		}
		
		ptr  += tlen;
		left -= tlen;
		cnt  += tlen;
		
		if(timeout>=0)
		{
			AM_TIME_GetClock(&now);
			diff = now-end;
			if(left && diff>=0)
			{
				AM_DEBUG(1, "read %d bytes timeout", len);
				ret = AM_SMC_ERR_TIMEOUT;
				break;
			}
		}
	}
	
	if(act_len)
		*act_len = cnt;
     AM_DEBUG(1, "smc_read ret is:%d", ret);
	return ret;
}

/**\brief 向智能卡发送数据*/
static AM_ErrorCode_t smc_write(AM_SMC_Device_t *dev, const uint8_t *buf, int len, int *act_len, int timeout)
{
	const uint8_t *ptr = buf;
	int left = len;
	int now, end = 0, diff, cnt = 0;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	if(timeout>=0)
	{
		AM_TIME_GetClock(&now);
		end = now + timeout;
	}
	
	while (left)
	{
		int tlen = left;
		int ms;
		
		if(timeout>=0)
		{
			ms = end-now;
		}
		else
		{
			ms = -1;
		}
		
		ret = dev->drv->write(dev, ptr, &tlen, ms);
		
		if(ret<0)
		{
			break;
		}
		
		ptr  += tlen;
		left -= tlen;
		cnt  += tlen;
		
		if(timeout>=0)
		{
			AM_TIME_GetClock(&now);
			diff = now-end;
			if(left && diff>=0)
			{
				AM_DEBUG(1, "write %d bytes timeout", len);
				ret = AM_SMC_ERR_TIMEOUT;
				break;
			}
		}
	}
	
	if(act_len)
		*act_len = cnt;

	return ret;
}

/**\brief status monitor thread*/
static void*
smc_status_thread(void *arg)
{
	AM_SMC_Device_t *dev = (AM_SMC_Device_t*)arg;
	AM_SMC_CardStatus_t old_status = -1; //to enable force run while AC on ignore card inserted or not AM_SMC_CARD_OUT;
	
	while(dev->enable_thread)
	{
		AM_SMC_CardStatus_t status;
		AM_ErrorCode_t ret;
		
		pthread_mutex_lock(&dev->lock);
		
		ret = dev->drv->get_status(dev, &status);
		
		pthread_mutex_unlock(&dev->lock);
		
		if(status!=old_status)
		{
			old_status = status;
			
			pthread_mutex_lock(&dev->lock);
			dev->flags |= SMC_FL_RUN_CB;
			pthread_mutex_unlock(&dev->lock);
			
			if(dev->cb)
			{
				dev->cb(dev->dev_no, status, dev->user_data);
			}
			
			AM_EVT_Signal(dev->dev_no, (status==AM_SMC_CARD_OUT)?AM_SMC_EVT_CARD_OUT:AM_SMC_EVT_CARD_IN, NULL);
			
			pthread_mutex_lock(&dev->lock);
			dev->flags &= ~SMC_FL_RUN_CB;
			pthread_mutex_unlock(&dev->lock);
			pthread_cond_broadcast(&dev->cond);		
		}

		usleep(SMC_STATUS_SLEEP_TIME*1000);
	}
	
	return NULL;
}

/****************************************************************************
 * API functions
 ***************************************************************************/

/**\brief 打开智能卡设备
 * \param dev_no 智能卡设备号
 * \param[in] para 智能卡设备开启参数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_smc.h)
 */
AM_ErrorCode_t AM_SMC_Open(int dev_no, const AM_SMC_OpenPara_t *para)
{
	AM_SMC_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	assert(para);
	
	AM_TRY(smc_get_dev(dev_no, &dev));
	
	pthread_mutex_lock(&am_gAdpLock);
	
	if(dev->openned)
	{
		AM_DEBUG(1, "smartcard device %d has already been openned", dev_no);
		ret = AM_SMC_ERR_BUSY;
		goto final;
	}
	
	dev->dev_no = dev_no;
	
	if(dev->drv->open)
	{
		AM_TRY_FINAL(dev->drv->open(dev, para));
	}
	
	pthread_mutex_init(&dev->lock, NULL);
	pthread_cond_init(&dev->cond, NULL);
	
	dev->enable_thread = para->enable_thread;
	dev->openned = AM_TRUE;
	dev->flags = 0;
	
	if(dev->enable_thread)
	{
		if(pthread_create(&dev->status_thread, NULL, smc_status_thread, dev))
		{
			AM_DEBUG(1, "cannot create the status monitor thread");
			pthread_mutex_destroy(&dev->lock);
			pthread_cond_destroy(&dev->cond);
			if(dev->drv->close)
			{
				dev->drv->close(dev);
			}
			ret = AM_SMC_ERR_CANNOT_CREATE_THREAD;
			goto final;
		}
	}
	else
	{
		dev->status_thread = (pthread_t)-1;
	}
	ringbuffer_init(&rxbuf,smc_rxbuf);
	ringbuffer_init(&txbuf,smc_txbuf);
final:
	pthread_mutex_unlock(&am_gAdpLock);
	
	return ret;
}

/**\brief 关闭智能卡设备
 * \param dev_no 智能卡设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_smc.h)
 */
AM_ErrorCode_t AM_SMC_Close(int dev_no)
{
	AM_SMC_Device_t *dev;
	
	AM_TRY(smc_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&am_gAdpLock);
	
	if(dev->enable_thread)
	{
		dev->enable_thread = AM_FALSE;
		pthread_join(dev->status_thread, NULL);
	}
	
	if(dev->drv->close)
	{
		dev->drv->close(dev);
	}
	
	pthread_mutex_destroy(&dev->lock);
	pthread_cond_destroy(&dev->cond);
	
	dev->openned = AM_FALSE;
	
	pthread_mutex_unlock(&am_gAdpLock);
	
	return AM_SUCCESS;
}

/**\brief 得到当前的智能卡插入状态
 * \param dev_no 智能卡设备号
 * \param[out] status 返回智能卡插入状态
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_smc.h)
 */
AM_ErrorCode_t AM_SMC_GetCardStatus(int dev_no, AM_SMC_CardStatus_t *status)
{
	AM_SMC_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	assert(status);
	
	AM_TRY(smc_get_openned_dev(dev_no, &dev));
	
	if(!dev->drv->get_status)
	{
		AM_DEBUG(1, "driver do not support get_status");
		ret = AM_SMC_ERR_NOT_SUPPORTED;
	}
	
	if(ret==0)
	{
		pthread_mutex_lock(&dev->lock);
		ret = dev->drv->get_status(dev, status);
		pthread_mutex_unlock(&dev->lock);
	}
	
	return ret;
}

/**\brief 复位智能卡
 * \param dev_no 智能卡设备号
 * \param[out] atr 返回智能卡的ATR数据
 * \param[in,out] len 输入ATR缓冲区大小，返回实际的ATR长度
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_smc.h)
 */
AM_ErrorCode_t AM_SMC_Reset(int dev_no, uint8_t *atr, int *len)
{
	AM_SMC_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	uint8_t atr_buf[AM_SMC_MAX_ATR_LEN];
	int len_buf;
	
	if(atr && len)
	{
		if(*len<AM_SMC_MAX_ATR_LEN)
		{
			AM_DEBUG(1, "ATR buffer < %d", AM_SMC_MAX_ATR_LEN);
			return AM_SMC_ERR_BUF_TOO_SMALL;
		}
	}
	else
	{
		if(!atr)
			atr = atr_buf;
		if(!len)
			len = &len_buf;
	}
	
	AM_TRY(smc_get_openned_dev(dev_no, &dev));
	
	if(!dev->drv->reset)
	{
		AM_DEBUG(1, "driver do not support reset");
		ret = AM_SMC_ERR_NOT_SUPPORTED;
	}
	
	if(ret==0)
	{
		pthread_mutex_lock(&dev->lock);
		ret = dev->drv->reset(dev, atr, len);
		pthread_mutex_unlock(&dev->lock);
	}
	
	return ret;
}

/**\brief 从智能卡读取数据
 *直接从智能卡读取数据，调用函数的线程会阻塞，直到读取到期望数目的数据，或到达超时时间。
 * \param dev_no 智能卡设备号
 * \param[out] data 数据缓冲区
 * \param[in] len 希望读取的数据长度
 * \param timeout 读取超时时间，以毫秒为单位，<0表示永久等待。
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_smc.h)
 */
AM_ErrorCode_t AM_SMC_Read(int dev_no, uint8_t *data, int len, int timeout)
{
	AM_SMC_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	assert(data);
	
	AM_TRY(smc_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	
	ret = smc_read(dev, data, len, NULL, timeout);
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief 向智能卡发送数据
 *直接向智能卡发送数据，调用函数的线程会阻塞，直到全部数据被写入，或到达超时时间。
 * \param dev_no 智能卡设备号
 * \param[in] data 数据缓冲区
 * \param[in] len 希望发送的数据长度
 * \param timeout 读取超时时间，以毫秒为单位，<0表示永久等待。
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_smc.h)
 */
AM_ErrorCode_t AM_SMC_Write(int dev_no, const uint8_t *data, int len, int timeout)
{
	AM_SMC_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	assert(data);
	
	AM_TRY(smc_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	
	ret = smc_write(dev, data, len, NULL, timeout);
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief 从智能卡读取数据
 *直接从智能卡读取数据，调用函数的线程会阻塞，直到读取到期望数目的数据，或到达超时时间。
 * \param dev_no 智能卡设备号
 * \param[out] data 数据缓冲区
 * \param[in] len 希望读取的数据长度
 * \param timeout 读取超时时间，以毫秒为单位，<0表示永久等待。
 * \return
 *   - >=0 实际读取的数据长度
 *   - 其他值 错误代码(见am_smc.h)
 */
AM_ErrorCode_t
AM_SMC_ReadEx(int dev_no, uint8_t *data, int *act_len, int len, int timeout)
{
	AM_SMC_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	//int act_len;
	
	assert(data);
	
	AM_TRY(smc_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	
	ret = smc_read(dev, data, len, act_len, timeout);
	
	pthread_mutex_unlock(&dev->lock);

	//if((ret >= 0) || (ret == AM_SMC_ERR_TIMEOUT))
		//return act_len;
	
	return ret;

}

/**\brief 向智能卡发送数据
 *直接向智能卡发送数据，调用函数的线程会阻塞，直到全部数据被写入，或到达超时时间。
 * \param dev_no 智能卡设备号
 * \param[in] data 数据缓冲区
 * \param[in] len 希望发送的数据长度
 * \param timeout 读取超时时间，以毫秒为单位，<0表示永久等待。
 * \return
 *   - >=0 实际写入的数据长度
 *   - 其他值 错误代码(见am_smc.h)
 */
AM_ErrorCode_t
AM_SMC_WriteEx(int dev_no, const uint8_t *data, int *act_len, int len, int timeout)
{
	AM_SMC_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	//int act_len;
	
	assert(data);
	
	AM_TRY(smc_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	
	ret = smc_write(dev, data, len, act_len, timeout);
	
	pthread_mutex_unlock(&dev->lock);

	//if((ret >= 0) || (ret == AM_SMC_ERR_TIMEOUT))
		//return act_len;
	
	return ret;
}

#define DELAY_MUL 2

/**\brief 按T0协议传输数据
 * \param dev_no 智能卡设备号
 * \param[in] send 发送数据缓冲区
 * \param[in] slen 待发送的数据长度
 * \param[out] recv 接收数据缓冲区
 * \param[out] rlen 返回接收数据的长度
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_smc.h)
 */
AM_ErrorCode_t AM_SMC_TransferT0(int dev_no, const uint8_t *send, int slen, uint8_t *recv, int *rlen)
{
	AM_SMC_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	uint8_t byte;
	uint8_t *dst;
	int left;
	AM_Bool_t sent = AM_FALSE;
	int getsw = 0;
	int next = 0;
	uint8_t buf[255] = {0};
	uint8_t INS_BYTE = send[1];
	uint8_t rxlen = send[4];
	uint8_t regs[100];
	
	if(slen == 5)
	{
		rxlen = send[4];
	}
	else
	{
	  rxlen = 0;
	}
	assert(send && recv && rlen);
	
	dst  = recv;
	ringbuffer_reset(&txbuf);  
	ringbuffer_reset(&rxbuf);
	ringbuffer_write(&txbuf,send,slen);
	//ringbuffer_flush(&rxbuf);
	AM_TRY(smc_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	ringbuffer_read(&txbuf,buf,5);
	AM_TRY_FINAL(smc_write(dev, buf, 5, NULL, 1000*DELAY_MUL));
	while(1)
	{
		AM_TRY_FINAL(smc_read(dev, &byte, 1, NULL, 1000*DELAY_MUL));
		AM_DEBUG(1, "...INS byte is 0x%x, rxlen is:%d.\n", byte, rxlen);
		if(byte ==0x60)
		{
			continue;
		}
		else if((byte & 0xF0) == 0x60)
		{
			ringbuffer_write(&rxbuf,&byte,1);
			AM_TRY_FINAL(smc_read(dev, dst, 1, NULL, 1000*DELAY_MUL));
        		AM_DEBUG(1, "case1--dst is: 0x%x \n", *dst);
			ringbuffer_write(&rxbuf,dst,1);
			goto final;
		}
		else if((byte & 0xF0) == 0x90)
		{
			ringbuffer_write(&rxbuf,&byte,1);
			AM_TRY_FINAL(smc_read(dev, dst, 1, NULL, 1000*DELAY_MUL));
        		AM_DEBUG(1, "case2--dst is: 0x%x \n", *dst);
			ringbuffer_write(&rxbuf,dst,1);
			goto final;
		}
		else if((byte==INS_BYTE)||(byte==(INS_BYTE^0x01))) //0xfe 0xff
		{
			int cnt = ringbuffer_avail(&txbuf);
        		AM_DEBUG(1, "case3--cnt is: 0x%x \n", cnt);
			if(cnt > 0)
			{
			  ringbuffer_read(&txbuf,buf,cnt);
			  AM_TRY_FINAL(smc_write(dev, buf, cnt, NULL, 5000*DELAY_MUL));
			}
			else
			{
				AM_TRY_FINAL(smc_read(dev, buf, rxlen, NULL, 5000*DELAY_MUL));
				ringbuffer_write(&rxbuf,buf,rxlen);
			}
		}
		else if((byte==(INS_BYTE^0xff))||(byte==(INS_BYTE^0xfe))) //0x01 0x00
		{
			AM_TRY_FINAL(smc_read(dev, buf, 1, NULL, 5000*DELAY_MUL));
        		AM_DEBUG(1, "case4--buf is: 0x%x \n", *buf);
			ringbuffer_write(&rxbuf,buf,1);
			rxlen--;		
		}
		else
		{
				AM_DEBUG(1, "%s %d ======================= smc error IO......\n", __FUNCTION__, __LINE__);
				ret = AM_SMC_ERR_IO;
				AM_FileRead("/sys/class/smc-class/smc_reg", (char*)&regs,100);
				AM_DEBUG(1, "line %d smc_regs:  %s\n", __LINE__,regs);
				break;
		}
	}
final:
	*rlen = ringbuffer_avail(&rxbuf);
	ringbuffer_read(&rxbuf,dst,*rlen);
	
	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief 按T1协议传输数据
 * \param dev_no 智能卡设备号
 * \param[in] send 发送数据缓冲区
 * \param[in] slen 待发送的数据长度
 * \param[out] recv 接收数据缓冲区
 * \param[out] rlen 返回接收数据的长度
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_smc.h)
 */
AM_ErrorCode_t AM_SMC_TransferT1(int dev_no, const uint8_t *send, int slen, uint8_t *recv, int *rlen)
{
	AM_SMC_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;

	assert(send && recv && rlen);

	AM_TRY(smc_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);

	AM_TRY_FINAL(smc_write(dev, send, slen, NULL, 1000));

	AM_TRY_FINAL(smc_read(dev, recv, 3, NULL, 1000));

	AM_TRY_FINAL(smc_read(dev, recv + 3, recv[2] + 1, NULL, 2000));

	*rlen = recv[2] + 4;

final:
	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief 按T14协议传输数据
 * \param dev_no 智能卡设备号
 * \param[in] send 发送数据缓冲区
 * \param[in] slen 待发送的数据长度
 * \param[out] recv 接收数据缓冲区
 * \param[out] rlen 返回接收数据的长度
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_smc.h)
 */
AM_ErrorCode_t AM_SMC_TransferT14(int dev_no, const uint8_t *send, int slen, uint8_t *recv, int *rlen)
{
	AM_SMC_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;

	assert(send && recv && rlen);

	AM_TRY(smc_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);

	AM_TRY_FINAL(smc_write(dev, send, slen, NULL, 1000));

	AM_TRY_FINAL(smc_read(dev, recv, 8, NULL, 1000));

	AM_TRY_FINAL(smc_read(dev, recv + 8, recv[7] + 1, NULL, 2000));

	*rlen = recv[7] + 9;

final:
	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief 取得当前的智能卡状态回调函数
 * \param dev_no 智能卡设备号
 * \param[out] cb 返回回调函数指针
 * \param[out] data 返回用户数据
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_smc.h)
 */
AM_ErrorCode_t AM_SMC_GetCallback(int dev_no, AM_SMC_StatusCb_t *cb, void **data)
{
	AM_SMC_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	AM_TRY(smc_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	
	if(cb)
	{
		*cb = dev->cb;
	}
	
	if(data)
	{
		*data = dev->user_data;
	}
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief 设定智能卡状态回调函数
 * \param dev_no 智能卡设备号
 * \param[in] cb 回调函数指针
 * \param[in] data 用户数据
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_smc.h)
 */
AM_ErrorCode_t AM_SMC_SetCallback(int dev_no, AM_SMC_StatusCb_t cb, void *data)
{
	AM_SMC_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	AM_TRY(smc_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	
	while((dev->flags&SMC_FL_RUN_CB) && (pthread_self()!=dev->status_thread))
	{
		pthread_cond_wait(&dev->cond, &dev->lock);
	}
	
	dev->cb = cb;
	dev->user_data = data;
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief 激活智能卡设备
 * \param dev_no 智能卡设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_smc.h)
 */
AM_ErrorCode_t AM_SMC_Active(int dev_no)
{
	AM_SMC_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	AM_TRY(smc_get_openned_dev(dev_no, &dev));
	
	if(!dev->drv->active)
	{
		AM_DEBUG(1, "driver do not support active");
		ret = AM_SMC_ERR_NOT_SUPPORTED;
	}
	
	if(ret==0)
	{
		pthread_mutex_lock(&dev->lock);
		ret = dev->drv->active(dev);
		pthread_mutex_unlock(&dev->lock);
	}
	
	return ret;
}

/**\brief 激活智能卡设备
 * \param dev_no 智能卡设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_smc.h)
 */
AM_ErrorCode_t AM_SMC_PraseATR(int dev_no,unsigned char* atr,int len)
{
	AM_SMC_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	AM_TRY(smc_get_openned_dev(dev_no, &dev));
	
	if(!dev->drv->parse_atr)
	{
		AM_DEBUG(1, "driver do not support active");
		ret = AM_SMC_ERR_NOT_SUPPORTED;
	}
	
	if(ret==0)
	{
		pthread_mutex_lock(&dev->lock);
		ret = dev->drv->parse_atr(dev,atr,len);
		pthread_mutex_unlock(&dev->lock);
	}
	
	return ret;
}



/**\brief 取消激活智能卡设备
 * \param dev_no 智能卡设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_smc.h)
 */
AM_ErrorCode_t AM_SMC_Deactive(int dev_no)
{
	AM_SMC_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	AM_TRY(smc_get_openned_dev(dev_no, &dev));
	
	if(!dev->drv->deactive)
	{
		AM_DEBUG(1, "driver do not support deactive");
		ret = AM_SMC_ERR_NOT_SUPPORTED;
	}
	
	if(ret==0)
	{
		pthread_mutex_lock(&dev->lock);
		ret = dev->drv->deactive(dev);
		pthread_mutex_unlock(&dev->lock);
	}
	
	return ret;
}

/**\brief 获取智能卡参数
 * \param dev_no 智能卡设备号
 * \param[out] para 返回智能卡参数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_smc.h)
 */
AM_ErrorCode_t AM_SMC_GetParam(int dev_no, AM_SMC_Param_t *para)
{
	AM_SMC_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	assert(para);
	
	AM_TRY(smc_get_openned_dev(dev_no, &dev));
	
	if(!dev->drv->get_param)
	{
		AM_DEBUG(1, "driver do not support get_param");
		ret = AM_SMC_ERR_NOT_SUPPORTED;
	}
	
	if(ret==0)
	{
		pthread_mutex_lock(&dev->lock);
		ret = dev->drv->get_param(dev, para);
		pthread_mutex_unlock(&dev->lock);
	}
	
	return ret;
}

/**\brief 设定智能卡参数
 * \param dev_no 智能卡设备号
 * \param[in] para 智能卡参数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_smc.h)
 */
AM_ErrorCode_t AM_SMC_SetParam(int dev_no, const AM_SMC_Param_t *para)
{
	AM_SMC_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	assert(para);
	
	AM_TRY(smc_get_openned_dev(dev_no, &dev));
	
	if(!dev->drv->set_param)
	{
		AM_DEBUG(1, "driver do not support set_param");
		ret = AM_SMC_ERR_NOT_SUPPORTED;
	}
	
	if(ret==0)
	{
		pthread_mutex_lock(&dev->lock);
		ret = dev->drv->set_param(dev, para);
		pthread_mutex_unlock(&dev->lock);
	}
  	
	return ret;
}

