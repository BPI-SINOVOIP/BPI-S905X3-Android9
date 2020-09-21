/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package. *
* Description:
*/
#ifdef _FORTIFY_SOURCE
#undef _FORTIFY_SOURCE
#endif


#include "am_sub.h"
#include "includes.h"
#include "list.h"
#include "dvb_sub.h"
#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "am_misc.h"
#include "pthread.h"
#include "memwatch.h"
#include "semaphore.h"
#include "am_dmx.h"
#include <am_debug.h>


#define LINUX_PES_FILTER

#ifndef LINUX_PES_FILTER
#include "am_pesfilter.h"
#endif

#define DVBSUB_DEBUG                                (1)
#if DVBSUB_DEBUG
#define subtitle_debug                                  printf
#else
#define subtitle_debug(args...) {do{}while(0);   }              
#endif

#define MAX_DMX_COUNT 3
#define VIDEO_PTS_FILE	"/sys/class/tsync/pts_video"

typedef struct pes_buffer
{
	char *pBuffer;
	int length;
}pes_buffer_t;
#ifdef LINUX_PES_FILTER
/**\brief 循环缓冲区*/
typedef struct PesWorkBuffer
{
	char *start_ptr;/**<  循环缓冲区起始地址*/

	int length;     /**<  循环缓冲区大小*/

	char *wr_ptr;   /**<  循环缓冲区写指针*/

	char *rd_ptr;   /**<  循环缓冲区读指针*/

	char *end_ptr;  /**<  循环缓冲区结束地址*/
	char last_rd_wr;/**< 1 rd>wr
			     2 rd<wr
			 */



}PesWorkBuffer_t;

#else
#endif

/* default pes buffer size*/

#define DEFAULT_BUFFER_SIZE	0x2000

/* flags */
#define SUBTITLE_RUN_FLAG                   (0x01)
#define SUBTITLE_ENABLE_FLAG                (0x02)
#define SUBTITLE_SHOW_FLAG                  (0x04)
#define SUBTITLE_PES_GET                    (0x08)
#define SUBTITLE_DECODE_FLAG                    (0x10)





/* state */
#define SUBTITLE_READY                      (0x01)
#define SUBTITLE_DISPLAY                    (0x02)
#define SUBTITLE_REMOVE                     (0x03)


struct sub_display_set
{
    	struct list_head head;
    	INT32U state;
    	INT32U display;
    	dvbsub_picture_t* sub_pic;
    	AM_SUB_Screen_t sub_screen;
};





typedef struct am_mw_subtitle_s
{
	pthread_t get_pes_thread;
	pthread_t subtitle_thread;
	int flags;
	unsigned int decode_handle;
	struct list_head      display_head;
	int dmx_id;
	int composition_id;
	int ancillary_id;
	int pes_pid;
	PesWorkBuffer_t pwb;
	AM_SUB_ShowCb_t show_cb;
	AM_SUB_ClearCb_t clear_cb;
	pthread_mutex_t     lock;
	sem_t subtitle_decode;
	int decode_destroyed;
	int fhandle;
	pthread_mutex_t rd_wr_Mutex;
	
}am_mw_subtitle_t;



am_mw_subtitle_t context_sub;

static void *get_pes_packet_thread(void *para);
static void *subtitle_thread(void *para);

#ifdef DEBUG_SUB
static FILE *fp1=0;
static FILE *fp2=0;
#endif
static void subtitle_remove( struct sub_display_set* sub_display)
{
	INT32U i = 0;
	pthread_mutex_lock(&context_sub.lock);

	list_del(&sub_display->head);
	if (sub_display->sub_screen.region)
	{

		 for (i = 0; i < sub_display->sub_screen.region_num; i++)
		 {
			 sub_display->sub_screen.region[i].palette.colors = NULL;
			 sub_display->sub_screen.region[i].buffer = NULL;
			 sub_display->sub_screen.region[i].text = NULL;
		 }
		 free(sub_display->sub_screen.region);
		 sub_display->sub_screen.region = NULL;
	}
	if (sub_display->sub_pic)
	{
		dvbsub_remove_display_picture(context_sub.decode_handle, sub_display->sub_pic);
	}

	free(sub_display);
	pthread_mutex_unlock(&context_sub.lock);


	return ;
}


static  void subtitle_cb(INT32U handle, dvbsub_picture_t *display)
{
	struct sub_display_set* sub_display =0;;
	sub_pic_region_t *pic_reg = NULL, *pic_reg_next = NULL;
	INT32U num = 0;


	if(display)
	{
	        for (pic_reg = display->p_region; pic_reg != NULL; pic_reg = pic_reg_next)
	        {
	            pic_reg_next = pic_reg->p_next;
	            num ++;
	        }

        	sub_display = (struct sub_display_set*)calloc(1, sizeof(struct sub_display_set));
        	if (sub_display)
        	{
			INIT_LIST_HEAD(&sub_display->head);
			sub_display->state = SUBTITLE_READY;
			sub_display->sub_pic = display;
			sub_display->sub_screen.left    = 0;
			sub_display->sub_screen.top     = 0;
			sub_display->sub_screen.width   = 720;
			sub_display->sub_screen.height  = 576;
			sub_display->sub_screen.region_num = num;
			if (num > 0)
			{
                		sub_display->sub_screen.region = (AM_SUB_Region_t*)calloc(num, sizeof(AM_SUB_Region_t));
				
	        	        if (sub_display->sub_screen.region)
	        	        {
					num=0;
	        	            for (pic_reg = sub_display->sub_pic->p_region; pic_reg != NULL; pic_reg = pic_reg_next)
	        	            {
	        	                pic_reg_next = pic_reg->p_next;
	
	        	                sub_display->sub_screen.region[num].left    = pic_reg->left;
	        	                sub_display->sub_screen.region[num].top     = pic_reg->top;
	        	                sub_display->sub_screen.region[num].width   = pic_reg->width;
	        	                sub_display->sub_screen.region[num].height  = pic_reg->height;
	
	        	                sub_display->sub_screen.region[num].palette.color_count = pic_reg->entry;
	        	                sub_display->sub_screen.region[num].palette.colors = pic_reg->clut;
	
	        	                sub_display->sub_screen.region[num].background  = pic_reg->background;
	        	                sub_display->sub_screen.region[num].buffer      = pic_reg->p_buf;
	
	        	                sub_display->sub_screen.region[num].textbg      = pic_reg->bg;
	        	                sub_display->sub_screen.region[num].textfg      = pic_reg->fg;
	        	                sub_display->sub_screen.region[num].length      = pic_reg->length;
	        	                sub_display->sub_screen.region[num].text        = pic_reg->p_text;
	
	        	                num++;
	        	            }
	        	        }
	        	        else
	        	        {
	        	            subtitle_debug("[%s] out of memory !\r\n", __func__);
	        	            free(sub_display);
	        	            dvbsub_remove_display_picture(handle, display);
	        	            return;
	        	        }
			}

			list_add_tail(&sub_display->head, &context_sub.display_head);

		}
		else
		{
			return ;
		}

	}
	return ;
}

AM_ErrorCode_t AM_SUB_Init(int buffer_size)
{

	int ret=AM_SUCCESS;

	INIT_LIST_HEAD(&context_sub.display_head);
	context_sub.dmx_id=-1;
	context_sub.composition_id=-1;
	context_sub.ancillary_id=-1;
	context_sub.pes_pid=0x1fff;
	context_sub.flags = SUBTITLE_ENABLE_FLAG | SUBTITLE_RUN_FLAG | SUBTITLE_SHOW_FLAG|SUBTITLE_PES_GET;

	if(buffer_size<=0)
	{
		//use default size
		buffer_size=DEFAULT_BUFFER_SIZE;
	}

	context_sub.pwb.start_ptr=(char *)malloc(buffer_size);
	context_sub.pwb.length=buffer_size;
	if(!context_sub.pwb.start_ptr)
	{
		ret=AM_SUB_ERR_NO_MEM;
		goto exit;
	}

	sem_init(&context_sub.subtitle_decode,1,0);

	context_sub.decode_destroyed=1;

	pthread_mutex_init(&context_sub.lock, NULL);
	pthread_mutex_init(&context_sub.rd_wr_Mutex,0);

	pthread_create(&context_sub.get_pes_thread,NULL,get_pes_packet_thread,NULL);
	
	pthread_create(&context_sub.subtitle_thread,NULL,subtitle_thread,NULL);
	
	
exit:
	return ret;
}


AM_ErrorCode_t AM_SUB_Quit()
{
	//first quit
	context_sub.flags &=~SUBTITLE_PES_GET;
	pthread_join(context_sub.get_pes_thread,NULL);
	//second quit
	
	pthread_join(context_sub.subtitle_thread,NULL);

	if(context_sub.pwb.start_ptr)
	{
		free(context_sub.pwb.start_ptr);
	}
	context_sub.pwb.start_ptr=0;
	context_sub.pwb.length=0;

	pthread_mutex_destroy(&context_sub.lock);
	sem_destroy(&context_sub.subtitle_decode);


	pthread_mutex_destroy(&context_sub.rd_wr_Mutex);

	return 0;
}
#ifdef LINUX_PES_FILTER
static inline int copy_data_to_pes_buffer(char* buffer,int len)
{
	int space=0;
	if(buffer==0)
	{
		return 0;
	}
	//check buffer overflow
	if(context_sub.pwb.wr_ptr>=context_sub.pwb.end_ptr)
	{
		if(context_sub.pwb.rd_ptr!=context_sub.pwb.start_ptr&&context_sub.pwb.rd_ptr!=context_sub.pwb.end_ptr)
		{
			//ring write ptr
			context_sub.pwb.wr_ptr=context_sub.pwb.start_ptr;
			printf("Ring write ptr\n");

		}
		else
		{
			//buffer overflow
			printf("buffer overflow\n"); 
			return 0;
		}
	}

	if(context_sub.pwb.rd_ptr==context_sub.pwb.start_ptr&&context_sub.pwb.wr_ptr==context_sub.pwb.start_ptr)
	{

		//reset buffer
		context_sub.pwb.rd_ptr=context_sub.pwb.end_ptr;
		printf("reset buffer\n");
	}


	if(context_sub.pwb.rd_ptr>context_sub.pwb.wr_ptr)
	{
		space=context_sub.pwb.rd_ptr-context_sub.pwb.wr_ptr;
		if(space>=len)
		{
			memcpy(context_sub.pwb.wr_ptr,buffer,len);
			context_sub.pwb.wr_ptr+=len;
			return len;
		}
		else
		{
			//less space
			memcpy(context_sub.pwb.wr_ptr,buffer,space);
			context_sub.pwb.wr_ptr+=space;
			return space;

		}
	}
	else
	{
		if(context_sub.pwb.rd_ptr==context_sub.pwb.wr_ptr)
		{
			if(context_sub.pwb.last_rd_wr==1)
			{
				return 0;	
			}
	
		}

		int first_space=0;
		int second_space=0;
	
		first_space=context_sub.pwb.end_ptr-context_sub.pwb.wr_ptr;
		second_space=context_sub.pwb.rd_ptr-context_sub.pwb.start_ptr;
		space=first_space+second_space;

		if(len<=space)
		{

			if(len-first_space>0)
			{

				memcpy(context_sub.pwb.wr_ptr,buffer,first_space);
				context_sub.pwb.wr_ptr=context_sub.pwb.start_ptr;
				memcpy(context_sub.pwb.wr_ptr,buffer+first_space,len-first_space);
				context_sub.pwb.wr_ptr+=(len-first_space);
				return len;

			}
			else
			{
				memcpy(context_sub.pwb.wr_ptr,buffer,len);
				if(context_sub.pwb.wr_ptr+len>=context_sub.pwb.end_ptr)
				{
					context_sub.pwb.wr_ptr=context_sub.pwb.start_ptr;
				}
				else
				{
					context_sub.pwb.wr_ptr+=len;
				}
				return len;

			}
		}
		else
		{
			//less space

			if(first_space>0)
			{
				memcpy(context_sub.pwb.wr_ptr,buffer,first_space);
				context_sub.pwb.wr_ptr=context_sub.pwb.start_ptr;
				memcpy(context_sub.pwb.wr_ptr,buffer+first_space,second_space);
				context_sub.pwb.wr_ptr+=second_space;
				return first_space+second_space;
			}
			else
			{
				context_sub.pwb.wr_ptr=context_sub.pwb.start_ptr;
				memcpy(context_sub.pwb.wr_ptr,buffer,second_space);
				context_sub.pwb.wr_ptr+=second_space;
				return second_space;

			}


		}

	}

	return 0;


}


static void pes_filter_callback(int dev_no,int fhandle,const unsigned char *data,int len,void *user_data)
{
	int ret=0;
	int data_remain=len;
	int data_copy=0;
	pthread_mutex_lock(&context_sub.rd_wr_Mutex);

#ifdef DEBUG_SUB

	if(fp2==0)
	{
		fp2=fopen("data_cb_sub.dat","wb+");
	}
	fwrite(data,1,len,fp2);
#endif
	while(data_remain>0&&context_sub.flags&SUBTITLE_DECODE_FLAG)
	{
		data_copy+=copy_data_to_pes_buffer((char*)data+data_copy,data_remain);
		data_remain-=data_copy;
		if(data_remain>0)
		{
			usleep(100);
		}

	}
	pthread_mutex_unlock(&context_sub.rd_wr_Mutex);
}

static int open_demux_pes_filter(int dev_no,int pid,int type)
{
	int fhandle=-1;
	AM_ErrorCode_t ret = AM_SUCCESS;
	struct dmx_pes_filter_params  param;
	if(dev_no>MAX_DMX_COUNT)
	{
		ret=AM_SUB_ERR_OPEN_PES;
		goto exit;
	}
	ret=AM_DMX_AllocateFilter(dev_no,&fhandle);
	if(ret)
	{
		ret=AM_SUB_ERR_OPEN_PES;
		goto exit;

	}
	ret=AM_DMX_SetCallback(dev_no,fhandle,pes_filter_callback,0);
	if(ret)
	{
		ret=AM_SUB_ERR_OPEN_PES;
		goto error;
	}
	ret=AM_DMX_SetBufferSize(dev_no,fhandle,0x40000);
	if(ret)
	{
		ret=AM_SUB_ERR_OPEN_PES;
		goto error;
	}
	param.pid=pid;
	param.output=1;
	param.pes_type=type;
	ret=AM_DMX_SetPesFilter(dev_no,fhandle,&param);

	if(ret)
	{
		ret=AM_SUB_ERR_OPEN_PES;
		goto error;
	}

	ret=AM_DMX_StartFilter(dev_no,fhandle);

	if(ret)
	{
		ret=AM_SUB_ERR_OPEN_PES;
		goto error;
	}
	context_sub.fhandle=fhandle;
	goto exit;
error:
	if(fhandle!=-1)
	{
		AM_DMX_FreeFilter(dev_no,fhandle);

		return ret;
	}
exit:
	return ret;



}

static int close_demux_pes_filter()
{
	if(context_sub.fhandle>=0)
	{
		AM_DMX_FreeFilter(context_sub.dmx_id,context_sub.fhandle);

	}
	return 0;
}

#endif

AM_ErrorCode_t AM_SUB_Start(int dmx_id, int pid, int composition_id, int ancillary_id)
{
	int ret=AM_SUCCESS;

	ret=dvbsub_decoder_create(composition_id,ancillary_id,subtitle_cb,&context_sub.decode_handle);
	if(ret)
	{
		ret=AM_SUB_ERR_CREATE_DECODE;
		goto exit;
	}
	pthread_mutex_lock(&context_sub.lock);

	context_sub.flags|=SUBTITLE_DECODE_FLAG;

	context_sub.decode_destroyed=0;
	pthread_mutex_unlock(&context_sub.lock);

#ifdef LINUX_PES_FILTER
	ret=open_demux_pes_filter(dmx_id,pid,2);
#else
	ret=AM_PesFilter_Open(dmx_id,pid,2);
#endif
	if(ret)
	{
		ret=AM_SUB_ERR_OPEN_PES;
		goto error_open;
	}

#ifdef LINUX_PES_FILTER
	context_sub.pwb.wr_ptr=context_sub.pwb.start_ptr;
	context_sub.pwb.end_ptr=context_sub.pwb.start_ptr+context_sub.pwb.length;
	context_sub.pwb.rd_ptr=context_sub.pwb.end_ptr; 	

#else
	ret=AM_PesFilter_Buffer(dmx_id,&context_sub.pwb);
	if(ret)
	{
		ret=AM_MW_TELETEXT_ERR_SET_BUFFER;
		goto error_set_buffer;
	}
#endif

	context_sub.dmx_id=dmx_id;
	goto exit;

#ifndef LINUX_PES_FILTER
error_set_buffer:
	AM_PesFilter_Close(dmx_id);
#endif
error_open:
	dvbsub_decoder_destroy(context_sub.decode_handle);
//	context_sub.flags &=~SUBTITLE_DECODE_FLAG;
exit:
	return ret;
}


AM_ErrorCode_t AM_SUB_Stop(void)
{

	int ret=AM_SUCCESS;

	context_sub.flags&=~SUBTITLE_DECODE_FLAG;
#ifdef LINUX_PES_FILTER
	close_demux_pes_filter();
#else
	AM_PesFilter_Close(context_sub.dmx_id);
#endif

	//clear display list or memory leak
	sem_wait(&context_sub.subtitle_decode);

	dvbsub_decoder_destroy(context_sub.decode_handle);

	context_sub.decode_destroyed=1;
#ifdef DEBUG_SUB

	if(fp1)
	fclose(fp1);
	if(fp2)
	fclose(fp2);
	fp1=0;
	fp2=0;
#endif
	return ret;
}




static inline int find_pes_start_code(char *buffer)
{

	if(buffer==0)
		return 0;
	if(buffer[0]==0&&buffer[1]==0&&buffer[2]==1&& \
			buffer[3]==0xbd)
	{
		return 1;
	}
	return 0;
}

static inline int find_pes_start_code_2(char *buffer1,int len1,char *buffer2,int len2)
{
	switch(len1)
	{
		case 1:
		if(len2>=5)	
		{
			if(buffer1[0]==0&&buffer2[0]==0&&buffer2[1]==1&&buffer2[0]==0xbd)
				return 1;
			else
				return 0;
		}
		else
		{
			return 0;
		}
		break;
		case 2:
		if(len2>=4)	
		{
			if(buffer1[0]==0&&buffer1[1]==0&&buffer2[0]==1&&buffer2[1]==0xbd)
				return 1;
			else
				return 0;

		}
		else
		{
			return 0;
		}
		break;
		case 3:
		if(len2>=3)	
		{
			if(buffer1[0]==0&&buffer1[1]==0&&buffer1[2]==1&&buffer2[0]==0xbd)
				return 1;
			else
				return 0;		
		}
		else
		{
			return 0;
		}
		break;
	}
	return 0;
}

static pes_buffer_t* get_subtitle_pes_packet(PesWorkBuffer_t *pwb)
{
	pes_buffer_t *pPesBuffer=0;
	int data_num=0;
	int first_num=0;
	int second_num=0;
	int data_check=0;
	int ret=-1;
	int start_code_found=0;
	int start_code_found_first=0;
	int start_code_found_12=0;
	int packet_length=0;
	int cross_num=0;
	int first_cpy=0;
	int second_cpy=0;


	if(pwb->rd_ptr>pwb->wr_ptr)
	{
		if(pwb->rd_ptr>=pwb->end_ptr)
		{
			if(pwb->wr_ptr!=pwb->start_ptr)
			{
				pwb->rd_ptr=pwb->start_ptr;
				goto case1;
			}
			else
			{
				//empty
				return 0;
			}

		}
	
		first_num=pwb->end_ptr-pwb->rd_ptr;
		second_num=pwb->wr_ptr-pwb->start_ptr;
		data_num=first_num+second_num;
		if(data_num>=6)
		{
			data_check=0;
			//check start code
			while(data_check<data_num-4)
			{
				if(data_check<=first_num-4)
				{
					ret=find_pes_start_code(pwb->rd_ptr+data_check);
					if(ret)
					{
						start_code_found_first=1;
						break;
					}
				}
				else
				{
					//complex
					if(data_check==first_num-3)
					{
						start_code_found_12=find_pes_start_code_2(pwb->rd_ptr+data_check,3,pwb->start_ptr,second_num);
					}
					else if(data_check==first_num-2)
					{
						start_code_found_12=find_pes_start_code_2(pwb->rd_ptr+data_check,2,pwb->start_ptr,second_num);
					}

					else if(data_check==first_num-1)
					{
						start_code_found_12=find_pes_start_code_2(pwb->rd_ptr+data_check,1,pwb->start_ptr,second_num);
	
					}else
					{
						//rd_ptr point to start_ptr
						pwb->rd_ptr=pwb->start_ptr;
						goto case1;

					}

					if(start_code_found_12)
					{
						cross_num=first_num-data_check;
						break;
					}


				}
				data_check++;

			}
		}

		if(start_code_found_first)
		{
			if(data_check+6<=first_num)
			{
				packet_length=(pwb->rd_ptr[data_check+4]<<8|pwb->rd_ptr[data_check+5])&0xffff;
				if(packet_length<=data_num-data_check-6)
				{
					//bug if packet_lenght is invalid
					pPesBuffer=(pes_buffer_t *)malloc(sizeof(pes_buffer_t ));
					if(!pPesBuffer)
					{
						pwb->rd_ptr+=4;
						return 0;
					}
					pPesBuffer->pBuffer=(char *)malloc(packet_length+6);
					if(!pPesBuffer->pBuffer)
					{
						free(pPesBuffer);	
						pwb->rd_ptr+=4;
						return 0;
					}
					pPesBuffer->length=packet_length+6;
					if(packet_length<=first_num-data_check-6)
					{
						memcpy(pPesBuffer->pBuffer,pwb->rd_ptr+data_check,packet_length+6);

						pwb->rd_ptr+=(data_check+pPesBuffer->length);


					}
					else
					{
						first_cpy=first_num-data_check;
						if(first_cpy>0)
							memcpy(pPesBuffer->pBuffer,pwb->rd_ptr+data_check,first_cpy);
						pwb->rd_ptr=pwb->start_ptr;
						second_cpy=(packet_length-first_cpy+6);
						if(second_cpy>0)
						{
							memcpy(pPesBuffer->pBuffer+first_cpy,pwb->rd_ptr,second_cpy);
							pwb->rd_ptr=pwb->start_ptr+second_cpy;
						}

					}
					return pPesBuffer;

				}
				else
				{
					//wait for more data,bug if error packet length
					if(packet_length>=0xffff)
					{
					}
					pwb->rd_ptr+=data_check;
					return 0;
				}

			}
			else
			{
				//hide bug
				if(second_num>=2)
				{
					packet_length=(pwb->start_ptr[data_check]<<8|pwb->start_ptr[data_check+1])&0xffff;
					if(packet_length<=second_num-2)
					{
						pwb->rd_ptr=pwb->start_ptr+2;
					}
					else
					{
						//wait for more data
						pwb->rd_ptr+=data_check;
						return 0;
					}
				}
				else
				{
					//wait for more data
					pwb->rd_ptr+=data_check;
					return 0;
				}

			}
			return 0;
		}

		if(start_code_found_12)
		{
			printf("crcoss start code\n");

			pwb->rd_ptr=pwb->start_ptr;

			return 0;
		}
		if(!start_code_found_12&&!start_code_found_first)
		{
			pwb->rd_ptr=pwb->start_ptr;
			return 0;

		}
		return 0;

	}
	else
	{
case1:
		if(pwb->wr_ptr==pwb->rd_ptr)
		{
			printf("rd===wr\n");
		}
		data_num=pwb->wr_ptr-pwb->rd_ptr;
		if(data_num>=6)
		{
			data_check=0;
			//check start code
			while(data_check<=data_num-4)
			{
				ret=find_pes_start_code(pwb->rd_ptr+data_check);
				if(ret)
				{
					start_code_found=1;
					break;
				}
				data_check++;
			}
			if(start_code_found)
			{
				//pes packet found
				if(data_num-data_check>=6)
				{
					//data_ok
					packet_length=(pwb->rd_ptr[data_check+4]<<8|pwb->rd_ptr[data_check+5])&0xffff;
					if(data_num-data_check-packet_length-6>=0)
					{
						//new packet 
						pPesBuffer=(pes_buffer_t *)malloc(sizeof(pes_buffer_t ));
						if(!pPesBuffer)
						{
							pwb->rd_ptr+=4;
							return 0;
						}
						pPesBuffer->pBuffer=(char *)malloc(packet_length+6);
						if(!pPesBuffer->pBuffer)
						{
							pwb->rd_ptr+=4;;
							free(pPesBuffer);
							return 0;
						}
						memcpy(pPesBuffer->pBuffer,pwb->rd_ptr+data_check,packet_length+6);
						pPesBuffer->length=packet_length+6;
						pwb->rd_ptr+=(data_check+pPesBuffer->length);
						return pPesBuffer;
					}
					else
					{
						//wait for more data;
						pwb->rd_ptr+=data_check;
						return 0;
					}

				}
				else
				{
					//wait for more data
					pwb->rd_ptr+=data_check;
					return 0;
				}
			}
			else
			{
				//invalid pes data hide bug
				pwb->rd_ptr+=data_check;
				return 0;
			}



		}
		else
		{
			return 0;
		}

	}
	return 0;


}



//used for get pes packet
static void *get_pes_packet_thread(void *para)
{
	pes_buffer_t *pbt=0;
	int decode_flag=0;

	while(context_sub.flags&SUBTITLE_PES_GET)
	{
		decode_flag=(context_sub.flags & SUBTITLE_DECODE_FLAG) ;
		if(!decode_flag && !context_sub.decode_destroyed)
		{
			usleep(10);
			continue;
		}

		if(context_sub.decode_destroyed)
		{
			usleep(10);
			continue;
		}
		pthread_mutex_lock(&context_sub.rd_wr_Mutex);
		pbt=get_subtitle_pes_packet(&context_sub.pwb);
		pthread_mutex_unlock(&context_sub.rd_wr_Mutex);
		if(context_sub.pwb.rd_ptr>context_sub.pwb.wr_ptr)
			context_sub.pwb.last_rd_wr=1;
		else
		{
			context_sub.pwb.last_rd_wr=2;

		}
#ifdef DEBUG_SUB

		if(fp1==0)
		{
			fp1=fopen("data_get_sub.dat","wb+");
		}
#endif
		if(pbt)
		{
#ifdef DEBUG_SUB

			//fwrite(&context_sub.pwb.rd_ptr,1,4,fp1);
			//fwrite(&context_sub.pwb.wr_ptr,1,4,fp1);
			fwrite(pbt->pBuffer,1,pbt->length,fp1);
#endif
			if(decode_flag)
			{
				pthread_mutex_lock(&context_sub.lock);
				dvbsub_parse_pes_packet(context_sub.decode_handle,(unsigned char*)pbt->pBuffer,pbt->length);	
				pthread_mutex_unlock(&context_sub.lock);
			}

			free(pbt->pBuffer);
			free(pbt);
			continue;
		}
		else
		{
				
			usleep(100);
		}
	}
	context_sub.flags &= ~SUBTITLE_RUN_FLAG ;
	return 0;
}


static unsigned long get_curretn_pts()
{
	char buffer[16];
	int ret=AM_SUCCESS;
	unsigned long pts=0xffffffff;
	ret=AM_FileRead(VIDEO_PTS_FILE,buffer,11);
	if(!ret)
		pts=strtoul(buffer,0,16);
	return pts;
}

static void clean_display_list()
{
	struct list_head* entry = NULL;
	struct list_head* next = NULL;
	struct list_head* prev = NULL;
	struct list_head* temp = NULL;
	struct sub_display_set* sub_display = NULL;

	list_for_each_safe(entry, next, &context_sub.display_head)
	{
		sub_display = list_entry(entry, struct sub_display_set, head);
		if (sub_display->display)
		{
			if (context_sub.clear_cb)
			{
				context_sub.clear_cb(&sub_display->sub_screen);
			}
		}
		sub_display->state = SUBTITLE_REMOVE;
		subtitle_remove(sub_display);
	}

}
//used for display subtile

static void *subtitle_thread(void *para)
{
	struct list_head* entry = NULL;
	struct list_head* next = NULL;
	struct list_head* prev = NULL;
	struct list_head* temp = NULL;
	struct sub_display_set* sub_display = NULL;
    	INT64U cur_pts = (INT64U)0;
        INT64U disp_pts = (INT64U)0;
	INT64U hide_pts = (INT64U)0;
	INT64U temp_pts = (INT64U)0;
	INT32U time_out = 0;
	INT32U remove_prev = 0;
	struct sub_display_set* tmp_display = NULL;
	AM_SUB_Screen_t *show_screen = NULL, *clear_screen = NULL;
	int decode_flag=0;
	int sem_value=0;;
	int count=0;
	while(context_sub.flags&SUBTITLE_RUN_FLAG)
	{

start:
		decode_flag=(context_sub.flags & SUBTITLE_DECODE_FLAG) ;
		if(!decode_flag && !context_sub.decode_destroyed)
		{
			sem_getvalue(&context_sub.subtitle_decode,&sem_value);
			if(sem_value==0)
			{
				clean_display_list();
				sem_post(&context_sub.subtitle_decode);
			}
			usleep(10);
			continue;
		}

		if(context_sub.decode_destroyed)
		{
			usleep(10);
			continue;
		}
		count=0;

		list_for_each_safe(entry, next, &context_sub.display_head)
		{
			count++;

			if(count>=100)
			{
				printf("too long display list %d\n",count);
				clean_display_list();
				goto start;

			}
			
			remove_prev=0;	
			show_screen=0;
			clear_screen=0;
			sub_display = list_entry(entry,struct sub_display_set, head);	
			
			if(sub_display->sub_pic)
			{
				cur_pts=(INT64U)get_curretn_pts();
				disp_pts = sub_display->sub_pic->pts;
				time_out = sub_display->sub_pic->timeout;
				hide_pts = disp_pts + (INT64U)(time_out * 90000);
				temp_pts = disp_pts;

                		if (disp_pts & (((INT64U)(1 << 16)) << 16))
			      	{
					cur_pts += (((INT64U)(1 << 16)) << 16);
			  	}

				if (sub_display->state == SUBTITLE_READY)
				{

					if (cur_pts >= temp_pts)
					{
						if (cur_pts < hide_pts)
						{
							remove_prev = 1;
							sub_display->state = SUBTITLE_DISPLAY;
							sub_display->display = (context_sub.flags & SUBTITLE_SHOW_FLAG) ? (1) : (0);
							if (sub_display->display)
							{
								show_screen = &sub_display->sub_screen;
							}
						}
						else
						{
							//remove pic
							sub_display->state = SUBTITLE_REMOVE;
						}
					}
				}
				else if (sub_display->state == SUBTITLE_DISPLAY)
				{

					if ((cur_pts >= hide_pts) || !(context_sub.flags & SUBTITLE_SHOW_FLAG))
					{
						if (cur_pts >= hide_pts)
						{
							 sub_display->state = SUBTITLE_REMOVE;

						}
						if (sub_display->display)
						{
							sub_display->display = 0;
							clear_screen = &sub_display->sub_screen;
						}
					}
					else if (context_sub.flags & SUBTITLE_SHOW_FLAG)
					{
						if(cur_pts < disp_pts)
						{
							if(sub_display->display)
							{
								sub_display->display = 0;
								clear_screen = &sub_display->sub_screen;
							}
						}
						else
						{
							sub_display->display = 1;
							if (sub_display->display)
							{
								show_screen = &sub_display->sub_screen;
							}
						}
					}
				}
				//
				if (remove_prev)
				{
					for (temp = sub_display->head.prev; temp != &context_sub.display_head; temp = prev)
					{
						prev = temp->prev;
						tmp_display = list_entry(temp, struct sub_display_set, head);
						tmp_display->state = SUBTITLE_REMOVE;
						if (tmp_display->display)
						{
							tmp_display->display = 0;
							if (context_sub.clear_cb)
							{
								context_sub.clear_cb(&tmp_display->sub_screen);
							}
						}
						subtitle_remove(tmp_display);
					}

				}
				if (clear_screen)
				{
					if (context_sub.clear_cb)
					{
						context_sub.clear_cb(clear_screen);
					}
				}
				if (show_screen)
				{
					if (context_sub.show_cb)
					{
						context_sub.show_cb(show_screen);
					}
				}
				if (sub_display->state == SUBTITLE_REMOVE)
				{
					subtitle_remove(sub_display);	
				}

			}
			else
			{
				subtitle_remove(sub_display);	
			}
		}
		usleep(10);
	}
	count=0;
	//clean list
	list_for_each_safe(entry, next, &context_sub.display_head)
	{
		sub_display = list_entry(entry, struct sub_display_set, head);
		if (sub_display->display)
		{
			if (context_sub.clear_cb)
			{
				context_sub.clear_cb(&sub_display->sub_screen);
			}
		}
		sub_display->state = SUBTITLE_REMOVE;
		subtitle_remove(sub_display);
		count++;
	}


	return 0;
}

AM_ErrorCode_t AM_SUB_RegisterUpdateCbs(AM_SUB_ShowCb_t ssc, AM_SUB_ClearCb_t scc)
{
	context_sub.show_cb=ssc;	
	context_sub.clear_cb=scc;	
	return 0;

}

//show or hide subtitle
AM_ErrorCode_t AM_SUB_Show(AM_Bool_t isShow)
{
	int ret=AM_SUCCESS;

	if(!(context_sub.flags&SUBTITLE_RUN_FLAG))
	{
		ret=AM_SUB_ERR_NOT_RUN;
		goto exit;
	}

	if(isShow)
	{
		context_sub.flags |=SUBTITLE_SHOW_FLAG ;
	}
	else
	{
		context_sub.flags &=~SUBTITLE_SHOW_FLAG ;
	}
exit:
	return ret;
}





