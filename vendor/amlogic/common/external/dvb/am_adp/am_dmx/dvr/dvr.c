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
 * \brief DVR demux 驱动
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2012-02-4: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 5

#include <am_debug.h>
#include <am_mem.h>
#include <am_misc.h>
#include <am_dvr.h>
#include "../am_dmx_internal.h"
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <poll.h>
/*add for config define for linux dvb *.h*/
#include <am_config.h>
#include <linux/dvb/dmx.h>


/****************************************************************************
 * Type definitions
 ***************************************************************************/

typedef struct DVR_Channel_s DVR_Channel;
typedef struct DVR_Filter_s  DVR_Filter;

struct DVR_Channel_s
{
	int          used;
	int          pid;
	DVR_Filter  *filters;
	uint8_t      buf[4096];
	int          size;
	int          cc;
	AM_Bool_t    is_sec;
};

struct DVR_Filter_s
{
	AM_Bool_t    used;
	AM_Bool_t    sec_filter;
	AM_DMX_Filter_t *dmx_filter;
	DVR_Channel *chan;
	int          pid;
	uint8_t      filter[DMX_FILTER_SIZE];
	uint8_t      maskandmode[DMX_FILTER_SIZE];
	uint8_t      maskandnotmode[DMX_FILTER_SIZE];
};

#define DVR_CHANNEL_COUNT 32
#define DVR_FILTER_COUNT  32
#define DVR_PID_COUNT     32

typedef struct{
	uint8_t      buf[256*1024];
	size_t       start;
	DVR_Channel  dvr_channels[DVR_CHANNEL_COUNT];
	DVR_Filter   dvr_filters[DVR_FILTER_COUNT];
	int          dvr_fd;
	int          dmx_fds[DVR_PID_COUNT];
	int          dmx_cnt;
}DVR_Demux;

/****************************************************************************
 * Static data definitions
 ***************************************************************************/

static AM_ErrorCode_t dvr_open(AM_DMX_Device_t *dev, const AM_DMX_OpenPara_t *para);
static AM_ErrorCode_t dvr_close(AM_DMX_Device_t *dev);
static AM_ErrorCode_t dvr_alloc_filter(AM_DMX_Device_t *dev, AM_DMX_Filter_t *filter);
static AM_ErrorCode_t dvr_free_filter(AM_DMX_Device_t *dev, AM_DMX_Filter_t *filter);
static AM_ErrorCode_t dvr_set_sec_filter(AM_DMX_Device_t *dev, AM_DMX_Filter_t *filter, const struct dmx_sct_filter_params *params);
static AM_ErrorCode_t dvr_set_pes_filter(AM_DMX_Device_t *dev, AM_DMX_Filter_t *filter, const struct dmx_pes_filter_params *params);
static AM_ErrorCode_t dvr_enable_filter(AM_DMX_Device_t *dev, AM_DMX_Filter_t *filter, AM_Bool_t enable);
static AM_ErrorCode_t dvr_set_buf_size(AM_DMX_Device_t *dev, AM_DMX_Filter_t *filter, int size);
static AM_ErrorCode_t dvr_poll(AM_DMX_Device_t *dev, AM_DMX_FilterMask_t *mask, int timeout);
static AM_ErrorCode_t dvr_read(AM_DMX_Device_t *dev, AM_DMX_Filter_t *filter, uint8_t *buf, int *size);
static AM_ErrorCode_t dvr_set_source(AM_DMX_Device_t *dev, AM_DMX_Source_t src);

const AM_DMX_Driver_t dvr_dmx_drv = {
.open  = dvr_open,
.close = dvr_close,
.alloc_filter = dvr_alloc_filter,
.free_filter  = dvr_free_filter,
.set_sec_filter = dvr_set_sec_filter,
.set_pes_filter = dvr_set_pes_filter,
.enable_filter  = dvr_enable_filter,
.set_buf_size   = dvr_set_buf_size,
.poll           = dvr_poll,
.read           = dvr_read,
.set_source     = dvr_set_source
};


/****************************************************************************
 * Static functions
 ***************************************************************************/

static AM_ErrorCode_t dvr_open(AM_DMX_Device_t *dev, const AM_DMX_OpenPara_t *para)
{
	char name[64];
	DVR_Demux *dmx;
	int fd;
	AM_ErrorCode_t ret;

	snprintf(name, sizeof(name), "/dev/dvb0.dvr%d", dev->dev_no);
	fd = open(name, O_RDONLY);
	if(fd == -1){
		AM_DEBUG(1, "cannot open %s %s", name, strerror(errno));
		return AM_DMX_ERR_CANNOT_OPEN_DEV;
	}

	fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK, 0);

	if(para->dvr_buf_size > 0){
		ret = ioctl(fd, DMX_SET_BUFFER_SIZE, para->dvr_buf_size);
		if(ret == -1){
			AM_DEBUG(1, "set dvr buffer size to %d failed", para->dvr_buf_size);
		}
	}

	if(para->dvr_fifo_no >= 0){
		char cmd[32];

		snprintf(name, sizeof(name), "/sys/class/stb/asyncfifo%d_source", para->dvr_fifo_no);
		snprintf(cmd, sizeof(cmd), "dmx%d", dev->dev_no);

		AM_FileEcho(name, cmd);
		AM_DEBUG(1, "%s -> %s", name, cmd);
	}

	dmx = (DVR_Demux*)malloc(sizeof(DVR_Demux));
	if(!dmx){
		AM_DEBUG(1, "not enough memory");
		close(fd);
		return AM_DMX_ERR_NO_MEM;
	}

	memset(dmx, 0, sizeof(DVR_Demux));
	dmx->dvr_fd = fd;
	dev->drv_data = dmx;

	AM_DEBUG(1, "open DVR demux");

	return AM_SUCCESS;
}

static AM_ErrorCode_t dvr_close(AM_DMX_Device_t *dev)
{
	DVR_Demux *dmx = (DVR_Demux*)dev->drv_data;
	int i;

	AM_DEBUG(1, "try to close DVR demux");
	for(i=0; i<DVR_PID_COUNT; i++){
		close(dmx->dmx_fds[i]);
	}

	close(dmx->dvr_fd);
	free(dmx);

	AM_DEBUG(1, "close DVR demux");
	return AM_SUCCESS;
}

static AM_ErrorCode_t dvr_alloc_filter(AM_DMX_Device_t *dev, AM_DMX_Filter_t *filter)
{
	DVR_Demux *dmx = (DVR_Demux*)dev->drv_data;
	DVR_Filter *f;
	int i;

	for(i=0; i<DVR_FILTER_COUNT; i++){
		f = &dmx->dvr_filters[i];
		if(!f->used){
			break;
		}
	}
	
	if(i >= DVR_FILTER_COUNT){
		return AM_DMX_ERR_NO_FREE_FILTER;
	}

	f->used = AM_TRUE;
	f->dmx_filter = filter;
	f->sec_filter = AM_TRUE;
	f->pid  = 0x1FFF;

	filter->drv_data = f;

	return AM_SUCCESS;
}

static AM_ErrorCode_t dvr_free_filter(AM_DMX_Device_t *dev, AM_DMX_Filter_t *filter)
{
	DVR_Demux *dmx = (DVR_Demux*)dev->drv_data;
	DVR_Filter *f = (DVR_Filter*)filter->drv_data;

	dvr_enable_filter(dev, filter, AM_FALSE);
	f->used = AM_FALSE;

	return AM_SUCCESS;
}

static AM_ErrorCode_t dvr_set_sec_filter(AM_DMX_Device_t *dev, AM_DMX_Filter_t *filter, const struct dmx_sct_filter_params *params)
{
	DVR_Demux *dmx = (DVR_Demux*)dev->drv_data;
	DVR_Filter *f = (DVR_Filter*)filter->drv_data;
	AM_Bool_t enable = filter->enable;
	int i;

	if(enable){
		dvr_enable_filter(dev, filter, AM_FALSE);
	}

	f->sec_filter = AM_TRUE;
	f->pid = params->pid;

	for(i=0; i<DMX_FILTER_SIZE; i++){
		uint8_t mode, mask;

		if((i == 1) || (i == 2)){
			f->filter[i] = 0;
			f->maskandmode[i] = 0;
			f->maskandnotmode[i] = 0;
			continue;
		}

		mask = params->filter.mask[i];
		mode = ~params->filter.mode[i];

		f->filter[i] = params->filter.filter[i];
		f->maskandmode[i] = mask & mode;
		f->maskandnotmode[i] = mask & ~mode;
	}

	if(enable){
		dvr_enable_filter(dev, filter, AM_TRUE);
	}

	return AM_SUCCESS;
}

static AM_ErrorCode_t dvr_set_pes_filter(AM_DMX_Device_t *dev, AM_DMX_Filter_t *filter, const struct dmx_pes_filter_params *params)
{
	DVR_Demux *dmx = (DVR_Demux*)dev->drv_data;
	DVR_Filter *f = (DVR_Filter*)filter->drv_data;
	AM_Bool_t enable = filter->enable;
	int i;

	if(enable){
		dvr_enable_filter(dev, filter, AM_FALSE);
	}

	f->sec_filter = AM_FALSE;
	f->pid = params->pid;

	if(enable){
		dvr_enable_filter(dev, filter, AM_TRUE);
	}

	return AM_SUCCESS;
}

static DVR_Channel* dvr_find_channel(DVR_Demux *dmx, int pid, AM_Bool_t is_sec)
{
	DVR_Channel *ch;
	int i;

	for(i=0; i<DVR_CHANNEL_COUNT; i++){
		ch = &dmx->dvr_channels[i];
		if(ch->used && ch->pid==pid){
			if(ch->is_sec != is_sec){
				AM_DEBUG(1, "channel type and filter type mismatch");
				return NULL;
			}
			ch->used++;
			return ch;
		}
	}

	for(i=0; i<DVR_CHANNEL_COUNT; i++){
		ch = &dmx->dvr_channels[i];
		if(!ch->used){
			ch->used = 1;
			ch->is_sec = is_sec;
			ch->pid = pid;
			ch->cc  = -1;
			ch->filters = NULL;
			return ch;
		}
	}

	return NULL;
}

static AM_ErrorCode_t dvr_enable_filter(AM_DMX_Device_t *dev, AM_DMX_Filter_t *filter, AM_Bool_t enable)
{
	DVR_Demux *dmx = (DVR_Demux*)dev->drv_data;
	DVR_Filter *f = (DVR_Filter*)filter->drv_data;
	DVR_Channel *ch;
	AM_Bool_t reset = AM_FALSE;

	if(filter->enable == enable)
		return AM_SUCCESS;

	AM_DEBUG(1, "enable DVR filter %d", enable);
	if(enable){
		ch = dvr_find_channel(dmx, f->pid, f->sec_filter);
		if(!ch){
			return AM_DMX_ERR_NO_FREE_FILTER;
		}

		f->chan = ch;

		if(ch->used == 1)
			reset = AM_TRUE;
	}else{
		ch = f->chan;

		ch->used--;

		if(ch->used == 0)
			reset = AM_TRUE;
	}

	if(reset){
		char name[64];
		struct dmx_pes_filter_params pparam;
		int i;

		for(i=0; i<dmx->dmx_cnt; i++){
			close(dmx->dmx_fds[i]);
		}
		dmx->dmx_cnt = 0;

		snprintf(name, sizeof(name), "/dev/dvb0.demux%d", dev->dev_no);

		for(i=0; i<DVR_CHANNEL_COUNT; i++){
			ch = &dmx->dvr_channels[i];
			if(ch->used){
				int fd;
				int r;

				fd = open(name, O_RDONLY);
				if(fd==-1){
					AM_DEBUG(1, "cannot open %s", name);
					continue;
				}

				memset(&pparam, 0, sizeof(pparam));
				pparam.pid = ch->pid;
				pparam.input = DMX_IN_FRONTEND;
				pparam.output = DMX_OUT_TS_TAP;
				pparam.pes_type = DMX_PES_OTHER;

				r = ioctl(fd, DMX_SET_PES_FILTER, &pparam);
				if(r==-1){
					AM_DEBUG(1, "DMX_SET_PES_FILTER failed");
				}

				r = ioctl(fd, DMX_START);
				if(r==-1){
					AM_DEBUG(1, "DMX_START failed");
				}

				AM_DEBUG(1, "dvr record pid %d", ch->pid);
				dmx->dmx_fds[dmx->dmx_cnt++] = fd;
			}
		}
	}
	
	return AM_SUCCESS;
}

static AM_ErrorCode_t dvr_set_buf_size(AM_DMX_Device_t *dev, AM_DMX_Filter_t *filter, int size)
{
	UNUSED(dev);
	UNUSED(filter);
	UNUSED(size);
	return AM_SUCCESS;
}

static void data_cb(AM_DMX_Device_t *dev, AM_DMX_Filter_t *filter, uint8_t *data, int len)
{
	AM_DMX_DataCb cb;
	void *udata;

	cb = filter->cb;
	udata = filter->user_data;

	pthread_mutex_unlock(&dev->lock);
	if(cb){
		cb(dev->dev_no, filter->id, data, len, udata);
	}
	pthread_mutex_lock(&dev->lock);
}

static int check_sec(AM_DMX_Device_t *dev, DVR_Channel *ch, uint8_t *data, int left)
{
	DVR_Demux *dmx = (DVR_Demux*)dev->drv_data;
	int slen;
	int cpy, ret=0;

	if(ch->size + left < 3){
		memcpy(ch->buf+ch->size, data, left);
		return left;
	}

	if(ch->size < 3){
		cpy = 3 - ch->size;
		memcpy(ch->buf+ch->size, data, cpy);
		data += cpy;
		left -= cpy;
		ret += cpy;
		ch->size = 3;
	}

	slen = (((ch->buf[1]<<8)|ch->buf[2])&0xFFF) + 3;
	cpy = AM_MIN(slen - ch->size, left);
	if(cpy){
		memcpy(ch->buf+ch->size, data, cpy);
		ret += cpy;
		ch->size += cpy;
	}

	if(slen==ch->size){
		int fid;
		DVR_Filter *f;

		for(fid=0; fid<DVR_FILTER_COUNT; fid++){
			AM_Bool_t match = AM_TRUE;
			AM_Bool_t check_neq = AM_FALSE, neq = AM_FALSE;
			int i;

			f = &dmx->dvr_filters[fid];

			if(!f->used || f->chan!=ch)
				continue;

			for(i=0; i<DMX_FILTER_SIZE; i++){
				uint8_t xor = ch->buf[i]^f->filter[i];

				if(xor & f->maskandmode[i]){
					match = AM_FALSE;
					break;
				}

				if(f->maskandnotmode[i])
					check_neq = AM_TRUE;

				if(xor & f->maskandnotmode[i])
					neq = AM_TRUE;
			}

			if(match && check_neq && !neq)
				match = AM_FALSE;

			if(match){
				data_cb(dev, f->dmx_filter, ch->buf, ch->size);
				break;
			}
		}

		ch->size = 0;
	}

	return ret;
}

static void parse_sec(AM_DMX_Device_t *dev, DVR_Channel *ch, uint8_t *data, int left, AM_Bool_t p_start)
{
	int first = 1;
	int slen;

	if(p_start){
		int pointer = data[0];

		data++;
		left--;

		if(left>=pointer){
			check_sec(dev, ch, data, pointer);
		}

		if(pointer){
			data += pointer;
			left -= pointer;
		}

		ch->size = 0;
	}

	while((left>0) && (first || data[0]!=0xFF)){
		int cnt;

		cnt = check_sec(dev, ch, data, left);
		data += cnt;
		left -= cnt;
		first = 0;
	}
}

static void parse_pes(AM_DMX_Device_t *dev, DVR_Channel *ch, uint8_t *data, int left)
{
	AM_DMX_Filter_t *filter;

	filter = ch->filters->dmx_filter;
	data_cb(dev, filter, data, left);
}

static int parse_ts_packet(AM_DMX_Device_t *dev, uint8_t *buf, int left)
{
	uint8_t *p = buf;
	int off;
	int pid, error, cc, ap_flags, s_flags, p_start;

	for(off=0; off<left; off++){
		if(p[off] == 0x47)
			break;
	}

	if(off){
		AM_DEBUG(1, "skip %d bytes", off);
		p += off;
		left -= off;
	}

	if(left < 188)
		return off;

	pid     = ((p[1]<<8)|p[2])&0x1FFF;
	error   = p[1]&0x80;
	p_start = p[1]&0x40;
	s_flags = p[3]&0xC0;
	ap_flags= p[3]&0x30;
	cc      = p[3]&0x0F;

	if((pid!=0x1FFF) && !s_flags && !error && (ap_flags&0x10)){
		DVR_Demux *dmx = (DVR_Demux*)dev->drv_data;
		int i;
		DVR_Channel *ch;

		pthread_mutex_lock(&dev->lock);

		for(i=0; i<DVR_CHANNEL_COUNT; i++){
			ch = &dmx->dvr_channels[i];

			if(ch->used && ch->pid==pid)
				break;
		}

		if(i<DVR_CHANNEL_COUNT){
			uint8_t *payload;
			int plen;

			if(ch->cc >= 0){
				int ecc;

				if(ap_flags & 0x10){
					ecc = (ch->cc+1)&0x0F;
				}else{
					ecc = ch->cc;
				}
				if(ecc != cc){
					AM_DEBUG(1, "TS packet discontinue");
				}
			}

			payload = p+4;
			plen = 188-4;

			if(ap_flags & 0x20){
				int alen = payload[0] + 1;

				payload += alen;
				plen -= alen;
			}

			if(plen > 0){
				if(ch->is_sec){
					parse_sec(dev, ch, payload, plen, p_start);
				}else{
					parse_pes(dev, ch, payload, plen);
				}
			}

			ch->cc = cc;
		}

		pthread_mutex_unlock(&dev->lock);
	}

	return off + 188;
}

static AM_ErrorCode_t dvr_poll(AM_DMX_Device_t *dev, AM_DMX_FilterMask_t *mask, int timeout)
{
	DVR_Demux *dmx = (DVR_Demux*)dev->drv_data;
	struct pollfd pf;
	int ret;
	
	UNUSED(mask);

	pf.fd = dmx->dvr_fd;
	pf.events = POLLIN|POLLERR;

	ret = poll(&pf, 1, timeout);
	if(ret <= 0){
		//AM_DEBUG(1, "poll DVR timeout");
		return AM_SUCCESS;
	}

	ret = read(dmx->dvr_fd, dmx->buf+dmx->start, sizeof(dmx->buf)-dmx->start);
	if(ret > 0){
		uint8_t *data = dmx->buf;
		int total = ret + dmx->start;
		int left = total;
		int cnt;

		//AM_DEBUG(1, "read DVR %d bytes", ret);

		do{
			cnt = parse_ts_packet(dev, data, left);
			data += cnt;
			left -= cnt;
		}while(cnt && left);
		
		if(left)
			memmove(dmx->buf, dmx->buf + total - left, left);

		dmx->start = left;
	}

	return AM_SUCCESS;
}

static AM_ErrorCode_t dvr_read(AM_DMX_Device_t *dev, AM_DMX_Filter_t *filter, uint8_t *buf, int *size)
{
	UNUSED(dev);
	UNUSED(filter);
	UNUSED(buf);
	UNUSED(size);

	return AM_SUCCESS;
}

static AM_ErrorCode_t dvr_set_source(AM_DMX_Device_t *dev, AM_DMX_Source_t src)
{
	char buf[32];
	char *cmd;
	
	snprintf(buf, sizeof(buf), "/sys/class/stb/demux%d_source", dev->dev_no);
	
	switch(src)
	{
		case AM_DMX_SRC_TS0:
			cmd = "ts0";
		break;
		case AM_DMX_SRC_TS1:
			cmd = "ts1";
		break;
#if defined(CHIP_8226M) || defined(CHIP_8626X)
		case AM_DMX_SRC_TS2:
			cmd = "ts2";
		break;
#endif

		case AM_DMX_SRC_HIU:
			cmd = "hiu";
		break;
		case AM_DMX_SRC_HIU1:
			cmd = "hiu1";
		break;
		default:
			AM_DEBUG(1, "do not support demux source %d", src);
		return AM_DMX_ERR_NOT_SUPPORTED;
	}
	
	return AM_FileEcho(buf, cmd);
}

