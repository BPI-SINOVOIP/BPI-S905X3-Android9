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
 * \brief 文件模拟解复用设备
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-06-03: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 5

#include <am_debug.h>
#include <am_mem.h>
#include <am_util.h>
#include <am_time.h>
#include <am_time.h>
#include <am_thread.h>
#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include "../am_dmx_internal.h"

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

#define FILTER_BUF_SIZE        (4096+4)
#define PARSER_BUF_SIZE        (512*1024)
 
/****************************************************************************
 * Type definitions
 ***************************************************************************/

/**\brief Channel type*/
typedef enum {
	CHAN_TYPE_SEC,
	CHAN_TYPE_PES
} ChannelType_t;

/**\brief Filter*/
typedef struct Filter Filter_t;
struct Filter {
	union {
		__u16  pid;
		struct dmx_sct_filter_params sct;
		struct dmx_pes_filter_params pes;
	} params;
	uint8_t   value[DMX_FILTER_SIZE+2];
	uint8_t   maskandmode[DMX_FILTER_SIZE+2];
	uint8_t   maskandnotmode[DMX_FILTER_SIZE+2];
	AM_Bool_t neq;
	int       id;
	ChannelType_t type;
	Filter_t *prev;
	Filter_t *next;
	uint8_t  *buf;
	int       buf_size;
	int       begin;
	int       bytes;
	AM_Bool_t enable;
	int       start_time;
};

/**\brief Parse stage*/
typedef enum {
	CHAN_STAGE_START,
	CHAN_STAGE_HEADER,
	CHAN_STAGE_PTS,
	CHAN_STAGE_PTR,
	CHAN_STAGE_DATA,
	CHAN_STAGE_END
} ChannelStage_t;

/**\brief PID channel*/
typedef struct Channel Channel_t;
struct Channel {
	uint16_t   pid;
	Channel_t *prev;
	Channel_t *next;
	uint8_t    buf[FILTER_BUF_SIZE];
	int        bytes;
	uint8_t    cc;
	ChannelStage_t stage;
	ChannelType_t  type;
	uint64_t       pts;
	int            offset;
};

/**\brief TS parser*/
typedef struct {
	Channel_t *channels;
	Filter_t  *filters;
	char       fname[PATH_MAX];
	FILE      *fp;
	uint8_t    buf[PARSER_BUF_SIZE];
	int        bytes;
	int        parsed;
	int        packet_len;
	int        start_time;
	int        rate;
	pthread_t  thread;
	pthread_mutex_t lock;
	pthread_cond_t  cond;
	AM_Bool_t  running;
} TSParser_t;

/****************************************************************************
 * Static data definitions
 ***************************************************************************/

static AM_ErrorCode_t emu_open(AM_DMX_Device_t *dev, const AM_DMX_OpenPara_t *para);
static AM_ErrorCode_t emu_close(AM_DMX_Device_t *dev);
static AM_ErrorCode_t emu_alloc_filter(AM_DMX_Device_t *dev, AM_DMX_Filter_t *filter);
static AM_ErrorCode_t emu_free_filter(AM_DMX_Device_t *dev, AM_DMX_Filter_t *filter);
static AM_ErrorCode_t emu_set_sec_filter(AM_DMX_Device_t *dev, AM_DMX_Filter_t *filter, const struct dmx_sct_filter_params *params);
static AM_ErrorCode_t emu_set_pes_filter(AM_DMX_Device_t *dev, AM_DMX_Filter_t *filter, const struct dmx_pes_filter_params *params);
static AM_ErrorCode_t emu_enable_filter(AM_DMX_Device_t *dev, AM_DMX_Filter_t *filter, AM_Bool_t enable);
static AM_ErrorCode_t emu_set_buf_size(AM_DMX_Device_t *dev, AM_DMX_Filter_t *filter, int size);
static AM_ErrorCode_t emu_poll(AM_DMX_Device_t *dev, AM_DMX_FilterMask_t *mask, int timeout);
static AM_ErrorCode_t emu_read(AM_DMX_Device_t *dev, AM_DMX_Filter_t *filter, uint8_t *buf, int *size);
static AM_ErrorCode_t emu_set_source(AM_DMX_Device_t *dev, AM_DMX_Source_t src);

const AM_DMX_Driver_t emu_dmx_drv = {
.open  = emu_open,
.close = emu_close,
.alloc_filter = emu_alloc_filter,
.free_filter  = emu_free_filter,
.set_sec_filter = emu_set_sec_filter,
.set_pes_filter = emu_set_pes_filter,
.enable_filter  = emu_enable_filter,
.set_buf_size   = emu_set_buf_size,
.poll  = emu_poll,
.read  = emu_read,
.set_source     = emu_set_source
};

/****************************************************************************
 * Static functions
 ***************************************************************************/

/**\brief Reset the channel*/
static void chan_reset(Channel_t *chan)
{
	chan->bytes  = 0;
	chan->cc     = 0xFF;
	chan->offset = 0;
	chan->pts    = 0;
	chan->stage  = CHAN_STAGE_START;
}

/**\brief Reset the TS parser*/
static void parser_reset(TSParser_t *parser)
{
	Channel_t *chan;
	
	parser->packet_len = 0;
	parser->bytes = 0;
	parser->rate = 0;
	
	if(parser->channels)
	{
		for(chan=parser->channels; chan; chan=chan->next)
		{
			chan_reset(chan);
		}
	}
}

/**\brief Get the channel in the parser*/
static Channel_t* parser_get_chan(TSParser_t *parser, uint16_t pid)
{
	Channel_t *chan = NULL;
	
	if(parser->channels)
	{
		for(chan=parser->channels; chan; chan=chan->next)
		{
			if(chan->pid==pid)
				return chan;
		}
	}
	
	chan = (Channel_t*)malloc(sizeof(Channel_t));
	if(!chan)
	{
		AM_DEBUG(1, "not enough memory");
		return NULL;
	}
	
	chan->pid = pid;
	
	chan_reset(chan);
	
	if(parser->channels)
	{
		chan->prev = NULL;
		chan->next = parser->channels;
		parser->channels->prev = chan;
		parser->channels = chan;
	}
	else
	{
		chan->prev = NULL;
		chan->next = NULL;
		parser->channels = chan;
	}
	
	return chan;
}

/**\brief Parse the PTS from the PES header*/
static int parse_pts(uint8_t *buf, uint64_t *pts)
{
	uint8_t sid, flags;
	uint64_t v1, v2, v3;
	
	sid = buf[3];
	if((sid==0xBC) || (sid==0xBE) || (sid==0xBF) || (sid==0xF0) || (sid==0xF1) || (sid==0xFF) ||
			(sid==0xF2) || (sid==0xF8))
		return -1;
	
	flags = buf[7]>>6;
	if(!(flags&2))
		return -1;
	
	v1 = (buf[9]>>1)&7;
	v2 = (buf[10]<<7)|(buf[11]>>1);
	v3 = (buf[12]<<7)|(buf[13]>>1);
	*pts = (v1<<30)|(v2<<15)|v3;
	return 0;
}

/**\brief Parse the payload data*/
static int parse_payload(TSParser_t *parser, Channel_t *chan, uint8_t *ptr, int len, int start)
{
	if(start)
	{
		chan->bytes = 0;
		chan->stage = CHAN_STAGE_HEADER;
	}
	else if(chan->stage==CHAN_STAGE_START)
	{
		return 0;
	}

	memcpy(chan->buf+chan->bytes, ptr, len);
	chan->bytes += len;

	if(chan->bytes<3)
		return 0;
	
	if(chan->stage==CHAN_STAGE_HEADER)
	{
		if((chan->buf[0]==0x00) && (chan->buf[1]==0x00) && (chan->buf[2]==0x01))
		{
			chan->type  = CHAN_TYPE_PES;
			chan->stage = CHAN_STAGE_PTS;
		}
		else
		{
			chan->type  = CHAN_TYPE_SEC;
			chan->stage = CHAN_STAGE_PTR;
		}
	}
	
	if(chan->stage==CHAN_STAGE_PTS)
	{
		uint64_t pts;
		
		if(chan->bytes<14)
			return 0;
		
		if(parse_pts(chan->buf, &pts)>=0)
		{
			int offset = ftell(parser->fp)-parser->bytes+parser->parsed;
			if(chan->pts)
			{
				parser->rate = ((uint64_t)(offset-chan->offset))*1000/((pts-chan->pts)/90);
				//AM_DEBUG(2, "ts bits rate %d", parser->rate*8);
			}
			
			if(!chan->pts)
			{
				chan->pts    = pts;
				chan->offset = offset;
			}
		}
		
		chan->stage = CHAN_STAGE_DATA;
	}
	else if(chan->stage==CHAN_STAGE_PTR)
	{
		int len = chan->buf[0]+1;
		int left = chan->bytes-len;
		
		if(chan->bytes<len)
			return 0;
		
		if(left)
			memmove(chan->buf, chan->buf+len, left);
		chan->bytes = left;
		chan->stage = CHAN_STAGE_DATA;
	}
	
	if(!chan->bytes)
		return 0;
	
retry:
	if(chan->stage==CHAN_STAGE_DATA)
	{
		Filter_t *f;
		
		switch(chan->type)
		{
			case CHAN_TYPE_PES:
				for(f=parser->filters; f; f=f->next)
				{
					if(f->enable && (f->params.pid==chan->pid) && (f->type==chan->type))
						break;
				}
				if(f)
				{
					int left = f->buf_size-f->bytes;
					int len = AM_MIN(left, chan->bytes);
					int pos = (f->begin+f->bytes)%f->buf_size;
					int cnt = f->buf_size-pos;
					
					if(cnt>=len)
					{
						memcpy(f->buf+pos, chan->buf, len);
					}
					else
					{
						int cnt2 = len-cnt;
						memcpy(f->buf+pos, chan->buf, cnt);
						memcpy(f->buf, chan->buf+cnt, cnt2);
					}
					
					pthread_cond_broadcast(&parser->cond);
				}
				chan->bytes = 0;
			break;
			case CHAN_TYPE_SEC:
				if(chan->bytes<1)
					return 0;
				
				if(chan->buf[0]==0xFF)
				{
					chan->stage = CHAN_STAGE_END;
				}
				else
				{
					int sec_len, left;

					if(chan->bytes<3)
						return 0;

					sec_len = ((chan->buf[1]<<8)|chan->buf[2])&0xFFF;
					sec_len += 3;

					if(chan->bytes<sec_len)
						return 0;

					for(f=parser->filters; f; f=f->next)
					{
						AM_Bool_t match = AM_TRUE;
						AM_Bool_t neq = AM_FALSE;
						int i;
						
						if(!f->enable || (f->type!=chan->type) || (f->params.pid!=chan->pid))
							continue;
						
						for(i=0; i<DMX_FILTER_SIZE+2; i++)
						{
							uint8_t xor = chan->buf[i]^f->value[i];
							
							if(xor&f->maskandmode[i])
							{
								match = AM_FALSE;
								break;
							}
							
							if(xor&f->maskandnotmode[i])
								neq = AM_TRUE;
						}
						
						if(match && f->neq && !neq)
							match = AM_FALSE;
						if(!match) continue;
						
						break;
					}
					
					if(f)
					{
						int left = f->buf_size-f->bytes;
						int pos = (f->begin+f->bytes)%f->buf_size;
						int cnt = f->buf_size-pos;

						if(left<sec_len)
						{
							AM_DEBUG(1, "section buffer overflow");
							return 0;
						}
						
						if(cnt>=sec_len)
						{
							memcpy(f->buf+pos, chan->buf, sec_len);
						}
						else
						{
							int cnt2 = sec_len-cnt;
							memcpy(f->buf+pos, chan->buf, cnt);
							memcpy(f->buf, chan->buf+cnt, cnt2);
						}
						
						f->bytes += sec_len;
						pthread_cond_broadcast(&parser->cond);
					}
					
					left = chan->bytes-sec_len;
					if(left)
					{
						memmove(chan->buf, chan->buf+sec_len, left);
					}
					chan->bytes = left;
					if(left)
						goto retry;
				}
			break;
		}
	}
	
	if(chan->stage==CHAN_STAGE_END)
		chan->bytes = 0;
	
	return 0;
}

/**\brief Parse the TS packet*/
static int parse_ts_packet(TSParser_t *parser)
{
	int p = parser->parsed, left=188;
	uint8_t *ptr;
	uint16_t pid;
	uint8_t tei, cc, af_avail, p_avail, ts_start, sc;
	Channel_t *chan;
	
	/*Scan the sync byte*/
	while(parser->buf[p]!=0x47)
	{
		p++;
		if(p>=parser->bytes)
		{
			parser->parsed = parser->bytes;
			return 0;
		}
	}
	
	if(p!=parser->parsed)
		AM_DEBUG(3, "skip %d bytes", p-parser->parsed);
	
	parser->parsed = p;
	
	/*Check the packet length*/
	if(!parser->packet_len)
	{
		if(parser->bytes-p>188)
		{
			if(parser->buf[p+188]==0x47)
			{
				parser->packet_len = 188;
			}
		}
		if(!parser->packet_len && (parser->bytes-p>204))
		{
			if(parser->buf[p+204]==0x47)
			{
				parser->packet_len = 204;
			}
		}
		
		if(!parser->packet_len)
		{
			return 0;
		}
		else
		{
			AM_DEBUG(2, "packet length %d", parser->packet_len);
		}
	}
	
	if(parser->bytes-p<parser->packet_len)
		return 0;
	
	ptr = &parser->buf[p];
	tei = ptr[1]&0x80;
	ts_start = ptr[1]&0x40;
	pid = ((ptr[1]<<8)|ptr[2])&0x1FFF;
	sc  = ptr[3]&0xC0;
	cc  = ptr[3]&0x0F;
	af_avail = ptr[3]&0x20;
	p_avail  = ptr[3]&0x10;

	if(pid==0x1FFF)
		goto end;
	
	if(tei)
	{
		AM_DEBUG(3, "ts error");
		goto end;
	}
	
	chan = parser_get_chan(parser, pid);
	if(!chan)
		goto end;
	
	/*if(chan->cc!=0xFF)
	{
		if(chan->cc!=cc)
			AM_DEBUG(3, "discontinuous");
	}*/
	
	if(p_avail)
		cc = (cc+1)&0x0f;
	chan->cc = cc;
	
	ptr += 4;
	left-= 4;
	
	if(af_avail)
	{
		left-= ptr[0]+1;
		ptr += ptr[0]+1;
	}
	
	if(p_avail && (left>0) && !sc)
	{
		parse_payload(parser, chan, ptr, left, ts_start);
	}
	
end:
	parser->parsed += parser->packet_len;
	return 1;
}

/**\brief TS file parser thread*/
static void* parser_thread(void *arg)
{
	TSParser_t *parser = (TSParser_t*)arg;
	
	parser->fname[0] = 0;
	parser->fp = NULL;
	parser_reset(parser);
	
	while(parser->running)
	{
		char *fname;
		FILE *fp;
		
		/*Reset the input file*/
		fname = getenv("AM_TSFILE");
		if(fname)
		{
			if(!strcmp(fname, parser->fname))
			{
				fp = parser->fp;
			}
			else
			{
				int len = AM_MIN(strlen(fname), sizeof(parser->fname)-1);
				
				strncpy(parser->fname, fname, len);
				parser->fname[len] = 0;
				fp = fopen(fname, "r");
				if(!fp)
					AM_DEBUG(2, "cannot open \"%s\"", fname);
				else
					AM_DEBUG(2, "switch to ts file \"%s\"", fname);
				AM_TIME_GetClock(&parser->start_time);
			}
		}
		else
		{
			fp = NULL;
		}
		
		if(fp!=parser->fp)
		{
			if(parser->fp)
				fclose(parser->fp);
			parser->fp = fp;
			parser_reset(parser);
		}
		
		/*Read the TS file*/
		if(parser->fp)
		{
			int left = sizeof(parser->buf)-parser->bytes;
			int cnt;
			
			if(feof(parser->fp))
			{
				rewind(parser->fp);
				parser_reset(parser);
				AM_TIME_GetClock(&parser->start_time);
				AM_DEBUG(2, "ts file rewind");
			}
			
			cnt = fread(parser->buf+parser->bytes, 1, left, parser->fp);
			if(cnt==0)
			{
				AM_DEBUG(1, "read ts file error");
				sleep(1);
				continue;
			}
			
			parser->bytes += cnt;
		}
		
		/*Parse the TS file*/
		if(parser->bytes)
		{
			int left;
			
			if(parser->bytes<parser->packet_len)
				continue;
			
			parser->parsed = 0;
			
			pthread_mutex_lock(&parser->lock);
			
			while(1)
			{
				if(!parse_ts_packet(parser))
					break;
			}
			
			pthread_mutex_unlock(&parser->lock);
			
			left = parser->bytes-parser->parsed;
			if(left)
			{
				memmove(parser->buf, parser->buf+parser->parsed, left);
			}
			parser->bytes = left;
		}
		
		if(parser->fp)
		{
			/*Sleep according to the bit rate*/
			if(parser->rate)
			{
				int end_clock, end_pos;
				int wait;
				
				end_pos = ftell(parser->fp);
				AM_TIME_GetClock(&end_clock);
				wait = (((uint64_t)end_pos)*1000)/parser->rate-(end_clock-parser->start_time);
				if(wait>0)
				{
					usleep(wait*1000);
				}
			}
		}
		else
		{
			usleep(100000);
		}
	}
	
	if(parser->fp)
	{
		fclose(parser->fp);
	}
	
	return NULL;
}

static AM_ErrorCode_t emu_open(AM_DMX_Device_t *dev, const AM_DMX_OpenPara_t *para)
{
	TSParser_t *parser;
	
	parser = (TSParser_t*)malloc(sizeof(TSParser_t));
	if(!parser)
	{
		AM_DEBUG(1, "not enough memory");
		return AM_DMX_ERR_NO_MEM;
	}
	
	parser->channels = NULL;
	parser->filters  = NULL;
	parser->running  = AM_TRUE;
	
	pthread_mutex_init(&parser->lock, NULL);
	pthread_cond_init(&parser->cond, NULL);
	
	if(pthread_create(&parser->thread, NULL, parser_thread, parser))
	{
		pthread_mutex_destroy(&parser->lock);
		pthread_cond_destroy(&parser->cond);
		free(parser);
		AM_DEBUG(1, "cannot create ts parser thread");
		return AM_DMX_ERR_CANNOT_CREATE_THREAD;
	}
	
	dev->drv_data = parser;
	return AM_SUCCESS;
}

static AM_ErrorCode_t emu_close(AM_DMX_Device_t *dev)
{
	TSParser_t *parser = (TSParser_t*)dev->drv_data;
	Channel_t *chan, *cnext;
	Filter_t *f, *fnext;
	
	parser->running = AM_FALSE;
	pthread_join(parser->thread, NULL);
	pthread_mutex_destroy(&parser->lock);
	pthread_cond_destroy(&parser->cond);
	
	for(f=parser->filters; f; f=fnext)
	{
		fnext = f->next;
		if(f->buf)
			free(f->buf);
		free(f);
	}
	
	for(chan=parser->channels; chan; chan=cnext)
	{
		cnext = chan->next;
		free(chan);
	}

	free(parser);
	
	return AM_SUCCESS;
}

static AM_ErrorCode_t emu_alloc_filter(AM_DMX_Device_t *dev, AM_DMX_Filter_t *filter)
{
	TSParser_t *parser = (TSParser_t*)dev->drv_data;
	Filter_t *f;
	
	f = (Filter_t*)malloc(sizeof(Filter_t));
	if(!f)
	{
		AM_DEBUG(1, "not enough memory");
		return AM_DMX_ERR_NO_MEM;
	}
	
	memset(f, 0, sizeof(Filter_t));
	
	f->id  = filter->id;
	f->buf_size = FILTER_BUF_SIZE;
	f->buf = (uint8_t*)malloc(f->buf_size);
	if(!f->buf)
	{
		free(f);
		AM_DEBUG(1, "not enough memory");
		return AM_DMX_ERR_NO_MEM;
	}
	
	pthread_mutex_lock(&parser->lock);
	
	if(parser->filters)
	{
		f->next = parser->filters;
		parser->filters->prev = f;
		parser->filters = f;
	}
	else
	{
		parser->filters = f;
	}
	
	pthread_mutex_unlock(&parser->lock);
	
	filter->drv_data = f;
	return AM_SUCCESS;
}

static AM_ErrorCode_t emu_free_filter(AM_DMX_Device_t *dev, AM_DMX_Filter_t *filter)
{
	TSParser_t *parser = (TSParser_t*)dev->drv_data;
	Filter_t *f = (Filter_t*)filter->drv_data;
	
	pthread_mutex_lock(&parser->lock);
	
	if(f->prev)
	{
		f->prev->next = f->next;
	}
	else
	{
		parser->filters = f->next;
	}
	
	if(f->next)
		f->next->prev = f->prev;
	
	pthread_mutex_unlock(&parser->lock);
	
	if(f->buf)
		free(f->buf);
	
	free(f);
	
	return AM_SUCCESS;
}

static AM_ErrorCode_t emu_set_sec_filter(AM_DMX_Device_t *dev, AM_DMX_Filter_t *filter, const struct dmx_sct_filter_params *params)
{
	TSParser_t *parser = (TSParser_t*)dev->drv_data;
	Filter_t *f = (Filter_t*)filter->drv_data;
	int i;
	
	pthread_mutex_lock(&parser->lock);
	
	f->type = CHAN_TYPE_SEC;
	f->params.sct = *params;
	f->neq = AM_FALSE;
	
	for(i=0; i<DMX_FILTER_SIZE; i++)
	{
		int pos = i?(i+2):i;
		uint8_t mask = params->filter.mask[i];
		uint8_t mode = params->filter.mode[i];
		
		f->value[pos] = params->filter.filter[i];
		
		mode = ~mode;
		f->maskandmode[pos] = mask&mode;
		f->maskandnotmode[pos] = mask&~mode;
		
		if(f->maskandnotmode[pos])
			f->neq = AM_TRUE;
	}
	
	f->maskandmode[1] = 0;
	f->maskandmode[2] = 0;
	f->maskandnotmode[1] = 0;
	f->maskandnotmode[2] = 0;
	
	pthread_mutex_unlock(&parser->lock);
	
	return AM_SUCCESS;
}

static AM_ErrorCode_t emu_set_pes_filter(AM_DMX_Device_t *dev, AM_DMX_Filter_t *filter, const struct dmx_pes_filter_params *params)
{
	TSParser_t *parser = (TSParser_t*)dev->drv_data;
	Filter_t *f = (Filter_t*)filter->drv_data;
	
	pthread_mutex_lock(&parser->lock);
	
	f->type = CHAN_TYPE_PES;
	f->params.pes = *params;
	
	pthread_mutex_unlock(&parser->lock);
	
	return AM_SUCCESS;
}

static AM_ErrorCode_t emu_enable_filter(AM_DMX_Device_t *dev, AM_DMX_Filter_t *filter, AM_Bool_t enable)
{
	TSParser_t *parser = (TSParser_t*)dev->drv_data;
	Filter_t *f = (Filter_t*)filter->drv_data;
	
	pthread_mutex_lock(&parser->lock);
	
	f->enable = enable;
	f->bytes  = 0;

	if(enable && (f->type==CHAN_TYPE_SEC))
	{
		AM_TIME_GetClock(&f->start_time);
	}
	
	pthread_mutex_unlock(&parser->lock);
	
	return AM_SUCCESS;
}

static AM_ErrorCode_t emu_set_buf_size(AM_DMX_Device_t *dev, AM_DMX_Filter_t *filter, int size)
{
	TSParser_t *parser = (TSParser_t*)dev->drv_data;
	Filter_t *f = (Filter_t*)filter->drv_data;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	pthread_mutex_lock(&parser->lock);
	
	if(f->buf_size!=size)
	{
		uint8_t *nbuf;
		
		nbuf = (uint8_t*)realloc(f->buf, size);
		if(nbuf)
		{
			f->buf = nbuf;
			f->buf_size = size;
		}
		else
		{
			ret = AM_DMX_ERR_NO_MEM;
		}
	}
	
	pthread_mutex_unlock(&parser->lock);
	
	return ret;
}

static int emu_check_data(TSParser_t *parser, AM_DMX_FilterMask_t *mask)
{
	Filter_t *f;
	
	for(f=parser->filters; f; f=f->next)
	{
		if(f->enable)
		{
			if(f->bytes)
				AM_DMX_FILTER_MASK_SET(mask, f->id);
			else if((f->type==CHAN_TYPE_SEC) && f->params.sct.timeout && f->start_time)
			{
				int now, diff;

				AM_TIME_GetClock(&now);
				diff = f->start_time+f->params.sct.timeout-now;

				if(diff<=0)
				{
					AM_DMX_FILTER_MASK_SET(mask, f->id);
				}
			}
		}
	}
	
	return AM_DMX_FILTER_MASK_ISEMPTY(mask)?AM_FALSE:AM_TRUE;
}

static AM_ErrorCode_t emu_poll(AM_DMX_Device_t *dev, AM_DMX_FilterMask_t *mask, int timeout)
{
	TSParser_t *parser = (TSParser_t*)dev->drv_data;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	pthread_mutex_lock(&parser->lock);
	
	if(timeout<0)
	{
		while(!emu_check_data(parser, mask))
			pthread_cond_wait(&parser->cond, &parser->lock);
	}
	else
	{
		if(!emu_check_data(parser, mask))
		{
			struct timespec ts;
			
			AM_TIME_GetTimeSpecTimeout(timeout, &ts);
			pthread_cond_timedwait(&parser->cond, &parser->lock, &ts);
			
			if(!emu_check_data(parser, mask))
				ret = AM_DMX_ERR_TIMEOUT;
		}
	}
	
	pthread_mutex_unlock(&parser->lock);
	
	return ret;
}

static AM_ErrorCode_t emu_read(AM_DMX_Device_t *dev, AM_DMX_Filter_t *filter, uint8_t *buf, int *size)
{
	TSParser_t *parser = (TSParser_t*)dev->drv_data;
	Filter_t *f = (Filter_t*)filter->drv_data;
	int len = 0, cpy = 0, cnt, cnt2, pos;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	pthread_mutex_lock(&parser->lock);
	
	if(f->bytes)
	{
		switch(f->type)
		{
			case CHAN_TYPE_SEC:
				pos = (f->begin+1)%f->buf_size;
				len = f->buf[pos]<<8;
				pos = (pos+1)%f->buf_size;
				len |= f->buf[pos];
				len &= 0xFFF;
				len += 3;
				cpy = AM_MIN(len, *size);
			break;
			case CHAN_TYPE_PES:
				cpy = AM_MIN(f->bytes, *size);
				len = cpy;
			break;
		}
		
		cnt = f->buf_size-f->begin;
		if(cnt>=cpy)
		{
			memcpy(buf, f->buf+f->begin, cpy);
		}
		else
		{
			cnt2 = cpy-cnt;
			memcpy(buf, f->buf+f->begin, cnt);
			memcpy(buf+cnt, f->buf, cnt2);
		}
		
		*size = cpy;
		f->begin += len;
		f->begin %= f->buf_size;
		f->bytes -= len;
	}
	else
	{
		f->start_time = 0;
		ret = AM_DMX_ERR_TIMEOUT;
	}
	
	pthread_mutex_unlock(&parser->lock);
	
	return ret;
}

static AM_ErrorCode_t emu_set_source(AM_DMX_Device_t *dev, AM_DMX_Source_t src)
{
	AM_DEBUG(1, "set demux source to %d", src);
	return AM_SUCCESS;
}

