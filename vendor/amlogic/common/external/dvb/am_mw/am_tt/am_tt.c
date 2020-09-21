/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: 
 */
#ifdef _FORTIFY_SOURCE
#undef _FORTIFY_SOURCE
#endif

#include "includes.h"


#include "VTCommon.h"
#include "VTDecoder.h"
#include "VTTeletext.h"
#include "VTDrawer.h"
#include "VTMosaicGraphics.h"

#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include "am_debug.h"

#include "am_tt.h"
#include "am_dmx.h"

#define LINUX_PES_FILTER

#ifndef LINUX_PES_FILTER
#include "am_pesfilter.h"
#endif

#define MAX_DMX_COUNT 3

#define logp(...) __android_log_print(ANDROID_LOG_INFO, "XXXX", __VA_ARGS__)

enum
{
    TELTTEXT_UNLIZED,
    TELETEXT_INILIZED,
    TELETEXT_STARTED,
    TELETEXT_STOPED,
};

enum
{
    VT_PAGENOTFIND = 0,
    VT_INVALIDPAGENUM,
    VT_NOTELETEXT
};

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

typedef struct pes_buffer
{
	char *pBuffer;
	int length;
}pes_buffer_t;

#endif



#define DEFAULT_BUFFER_SIZE	0x20000
#define TELETEXT_MAX_PAGENUM               	(300)
#define TELETEXT_BUFFER_SIZE 0x8000
#define TELETEXT_MAX_PAGEHISTORY            (64)

#define TELETEXT_DRAW(ctx, doit)\
	AM_MACRO_BEGIN\
	if((ctx)->draw_begin) (ctx)->draw_begin();\
	doit;\
	if((ctx)->update_display) (ctx)->update_display();\
	AM_MACRO_END


typedef struct am_mw_teletext_context_s
{
	int dmx_id;
	int pid;
	int status;
	int flags;
	PesWorkBuffer_t pwb;
	pthread_t teletext_thread;
    	INT32U cb_parameter;
    	unsigned short  teletext_status;
    	unsigned short  wPageIndex;
    	unsigned short  wPageSubCode;
    	struct  TVTPage sCurrPage;
	AM_TT_FillRectCb_t fr;
	AM_TT_DrawTextCb_t dt;
	AM_TT_ConvertColorCb_t    cc;
	AM_TT_GetFontHeightCb_t   gfh;
	AM_TT_GetFontMaxWidthCb_t gfw;
	AM_TT_DrawBeginCb_t       draw_begin;
	AM_TT_DisplayUpdateCb_t   update_display;
	AM_TT_ClearDisplayCb_t       co;
	int fhandle;
	pthread_mutex_t rd_wr_Mutex;
	pthread_cond_t  rd_wr_Cond;
	char auto_page_status ;
	char TtxCodepage;
	AM_TT_EventCb_t ecb;
	
}am_mw_teletext_context_t;

#define TELTE_RUNNING_FLAG	0x1

am_mw_teletext_context_t context_tele;

static unsigned char VTDoubleProfile[25];
static INT16U  VTPageHistoryHead;

static INT16U  VTPageHistory[TELETEXT_MAX_PAGEHISTORY];


static INT32S teletext_fill_rectangle(INT32S left, INT32S top, INT32U width, INT32U height, INT32U color);
static INT32S teletext_draw_text(INT32S x, INT32S y, INT32U width, INT32U height, INT16U *text, INT32S len, INT32U color, INT32U w_scale, INT32U h_scale);
static INT32U teletext_convert_color(INT32U index, INT32U type);
static INT32U teletext_get_font_height(void);
static INT32U teletext_get_font_max_width(void);
static unsigned int teletext_mosaic_convert_color(unsigned char alpha, unsigned char red,  unsigned char green,  unsigned char blue);
static BOOLEAN teletext_set_codepage(int Codepage);
static void teletext_clean_osd();

#ifdef DEBUG_TELE
static FILE *fp1=0;
static FILE *fp2=0;
#endif



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
						return 0;
						//goto case1;

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


static inline int read_teletext_data(unsigned char *buffer,int len)
{
	int data_read=0;
	PesWorkBuffer_t *pwb=&context_tele.pwb;
	int num=0;

	if(pwb==0)
	{
		goto exit;
	}
	if(pwb->rd_ptr==pwb->wr_ptr)
	{
		if(pwb->wr_ptr!=pwb->start_ptr)
			pwb->rd_ptr=pwb->start_ptr;
		else
		{
			pwb->rd_ptr=pwb->end_ptr;

		}
		data_read=0;
		goto exit;

	}


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
				goto exit;
			}

		}
		num=pwb->end_ptr-pwb->rd_ptr;

		if(len>=num)
		{
			data_read=num;
		}
		else
		{
			data_read=len;
		}

		if(data_read>0)
		{
			memcpy(buffer,pwb->rd_ptr,data_read);
			if(num>=len)
			{
				if(pwb->wr_ptr!=pwb->start_ptr)
					pwb->rd_ptr=pwb->start_ptr;
				else
				{
					pwb->rd_ptr=pwb->end_ptr;

				}
			}
			else
			{
				pwb->rd_ptr+=data_read;
			}
			goto exit;
		}
		else
		{
			data_read=0;
			goto exit;
		}


	}
	else if(pwb->rd_ptr<pwb->wr_ptr)
	{
case1:
		num=pwb->wr_ptr-pwb->rd_ptr-1;
		if(len>=num)
		{
			data_read=num;
		}
		else
		{
			data_read=len;
		}

		if(data_read>0)
		{
			memcpy(buffer,pwb->rd_ptr,data_read);
			pwb->rd_ptr+=data_read;
			goto exit;

		}
		else
		{
			data_read=0;
			goto exit;
		}


	}


exit:
	return data_read; 
}

static int teletext_parse_data(unsigned char *pData,int nDatalength)
{
	unsigned char  data_unit_length = 0;
	unsigned char  data_unit_id = 0;
	
	unsigned char framing_code;
	int processLength = 0;


	if (pData == 0)
	{
		 return -1;
	}

	while (processLength < nDatalength)
	{
		data_unit_id = *pData++;
		data_unit_length = *pData++;
		if (data_unit_id != 0x02 && data_unit_id != 0x03)
		{
			
		   	pData += data_unit_length;
		   	processLength += data_unit_length + 2;
		   	continue;
		}
		pData++;
		framing_code = *pData++;
		if (framing_code != 0xe4)
		{
		   	pData += data_unit_length - 2;
		   	processLength += data_unit_length + 2;
			printf("data error 1\n");
		   	continue;
		}

		VTDecodeLine(pData);
		
		pData += 42;
		processLength += 46;
	}

	
	return AM_SUCCESS;


}

static void* teletext_thread(void *para)
{
	int buffer_space=TELETEXT_BUFFER_SIZE;
	int buffer_off=0;
	unsigned char *buffer ;
	pes_buffer_t *pbt=0;
	int data_read=0;
	int packet_head=0;
	buffer=(unsigned char *)malloc(TELETEXT_BUFFER_SIZE);
#ifdef DEBUG_TELE

	if(fp1==0)
	{
		fp1=fopen("data_get.dat","wb+");
	}
#endif

	while(context_tele.flags&TELTE_RUNNING_FLAG)
	{
		pthread_mutex_lock(&context_tele.rd_wr_Mutex);
		while((context_tele.flags&TELTE_RUNNING_FLAG) && (context_tele.pwb.rd_ptr==context_tele.pwb.wr_ptr)){
			pthread_cond_wait(&context_tele.rd_wr_Cond, &context_tele.rd_wr_Mutex);
		}
		pbt=get_subtitle_pes_packet(&context_tele.pwb);
		pthread_mutex_unlock(&context_tele.rd_wr_Mutex);

		if(context_tele.pwb.rd_ptr>context_tele.pwb.wr_ptr)
			context_tele.pwb.last_rd_wr=1;
		else
		{
			context_tele.pwb.last_rd_wr=2;

		}
		if(pbt)
		{
			
#ifdef DEBUG_TELE

			fwrite(pbt->pBuffer,1,pbt->length,fp1);
#endif
			packet_head=pbt->pBuffer[8]+9;
			//AM_DEBUG(1, "@@@1 %d\n",*(pbt->pBuffer+packet_head),*(pbt->pBuffer+packet_head+1),*(pbt->pBuffer+packet_head+2));
			teletext_parse_data((unsigned char*)pbt->pBuffer+packet_head+1,pbt->length-packet_head-1);
			//AM_DEBUG(1,"@@@2 %x %x",pbt->pBuffer,pbt);
			free(pbt->pBuffer);
			free(pbt);

			continue;
		}
	}
	if(buffer)
	{
		free(buffer);
	}
	return 0;
}

static void teletext_draw_display_message(INT8U uType)
{
    RECT DrawRect ;

    DrawRect.left = 100;
    DrawRect.top = 200;
    DrawRect.width = 500;
    DrawRect.height = 30;

    char VTDisplayMessage[46];

    context_tele.fr(DrawRect.left, DrawRect.top, DrawRect.width, DrawRect.height, context_tele.cc(1, AM_TT_COLOR_TEXT_BG));

    memset(VTDisplayMessage, 0x20, 45);

    switch (uType)
    {
        case VT_PAGENOTFIND:
            //strcpy(VTDisplayMessage, "Please wait...");
            VTDisplayMessage[0]=0x0;VTDisplayMessage[1]='P';	
            VTDisplayMessage[2]=0x0;VTDisplayMessage[3]='l';
            VTDisplayMessage[4]=0x0;VTDisplayMessage[5]='e';
            VTDisplayMessage[6]=0x0;VTDisplayMessage[7]='a';
            VTDisplayMessage[8]=0x0;VTDisplayMessage[9]='s';
            VTDisplayMessage[10]=0x0;VTDisplayMessage[11]='e';
            VTDisplayMessage[12]=0x0;VTDisplayMessage[13]=' ';	
            VTDisplayMessage[14]=0x0;VTDisplayMessage[15]='w';
            VTDisplayMessage[16]=0x0;VTDisplayMessage[17]='a';
            VTDisplayMessage[18]=0x0;VTDisplayMessage[19]='i';
            VTDisplayMessage[20]=0x0;VTDisplayMessage[21]='t';
            VTDisplayMessage[22]=0x0;VTDisplayMessage[23]='.';
            VTDisplayMessage[24]=0x0;VTDisplayMessage[25]='.';
            VTDisplayMessage[26]=0x0;VTDisplayMessage[27]='.';
 
            context_tele.dt( DrawRect.left, DrawRect.top-5, DrawRect.width, DrawRect.height, (unsigned short*)VTDisplayMessage, 28, context_tele.cc(3, AM_TT_COLOR_TEXT_FG), 1, 1);
            break;

        case VT_INVALIDPAGENUM:
            //strcpy(VTDisplayMessage, "check range 0x100-0x899");
            VTDisplayMessage[0]=0x0;VTDisplayMessage[1]='c';	
            VTDisplayMessage[2]=0x0;VTDisplayMessage[3]='h';
            VTDisplayMessage[4]=0x0;VTDisplayMessage[5]='e';
            VTDisplayMessage[6]=0x0;VTDisplayMessage[7]='c';
            VTDisplayMessage[8]=0x0;VTDisplayMessage[9]='k';
            VTDisplayMessage[10]=0x0;VTDisplayMessage[11]=' ';
            VTDisplayMessage[12]=0x0;VTDisplayMessage[13]='r';	
            VTDisplayMessage[14]=0x0;VTDisplayMessage[15]='a';
            VTDisplayMessage[16]=0x0;VTDisplayMessage[17]='n';
            VTDisplayMessage[18]=0x0;VTDisplayMessage[19]='g';
            VTDisplayMessage[20]=0x0;VTDisplayMessage[21]='e';
            VTDisplayMessage[22]=0x0;VTDisplayMessage[23]=' ';
            VTDisplayMessage[24]=0x0;VTDisplayMessage[25]='0';
            VTDisplayMessage[26]=0x0;VTDisplayMessage[27]='x';
            VTDisplayMessage[28]=0x0;VTDisplayMessage[29]='1';
            VTDisplayMessage[30]=0x0;VTDisplayMessage[31]='0';
            VTDisplayMessage[32]=0x0;VTDisplayMessage[33]='0';
            VTDisplayMessage[34]=0x0;VTDisplayMessage[35]='~';
            VTDisplayMessage[36]=0x0;VTDisplayMessage[37]='0';
            VTDisplayMessage[38]=0x0;VTDisplayMessage[39]='x';
            VTDisplayMessage[40]=0x0;VTDisplayMessage[41]='8';
            VTDisplayMessage[42]=0x0;VTDisplayMessage[43]='9';
            VTDisplayMessage[44]=0x0;VTDisplayMessage[45]='9';

            context_tele.dt( DrawRect.left, DrawRect.top-5, DrawRect.width, DrawRect.height, (INT16U *)VTDisplayMessage, 46, context_tele.cc(3, AM_TT_COLOR_TEXT_FG), 1, 1);
            break;
        default:
            break;
    }

    return;
}

static void teletext_history_reset(void)
{
    INT32S error_code = AM_SUCCESS;

    VTPageHistoryHead = 0;
    memset(VTPageHistory, 0, TELETEXT_MAX_PAGEHISTORY);

    return;
}

static void teletext_history_push_page(INT16U wPageHex)
{
    INT32S error_code = AM_SUCCESS;

    if (VTPageHistory[VTPageHistoryHead] != wPageHex)
    {
        VTPageHistoryHead = (VTPageHistoryHead + 1) % TELETEXT_MAX_PAGEHISTORY;
        VTPageHistory[VTPageHistoryHead] = wPageHex;
    }

    return;
}

static INT16U teletext_history_pop_lastpage(INT16U wCurrentPageHex)
{
    INT16U wPageHex;
    INT16U wLastHistoryHead;

    if (VTPageHistory[VTPageHistoryHead] == wCurrentPageHex)
    {
        wLastHistoryHead = (TELETEXT_MAX_PAGEHISTORY + VTPageHistoryHead - 1) % TELETEXT_MAX_PAGEHISTORY;

        if (VTPageHistory[wLastHistoryHead] == 0)
        {
            return 0;
        }

        VTPageHistory[VTPageHistoryHead] = 0;
        VTPageHistoryHead = wLastHistoryHead;
    }

    wPageHex = VTPageHistory[VTPageHistoryHead];

    if (VTPageHistory[VTPageHistoryHead] != 0)
    {
        VTPageHistory[VTPageHistoryHead] = 0;
        VTPageHistoryHead = (TELETEXT_MAX_PAGEHISTORY + VTPageHistoryHead - 1) % TELETEXT_MAX_PAGEHISTORY;
    }

    return wPageHex;
}







static int teletext_header_update(unsigned int dwPageCode)
{
	char szOldClock[8];
    	if (context_tele.status != TELETEXT_STARTED)
    	{
        	return AM_TT_ERR_NOT_START;
    	}

    	if (LOWWORD(dwPageCode) == context_tele.wPageIndex)
    	{

        	GetDisplayHeader(&context_tele.sCurrPage, FALSE);
		if (context_tele.auto_page_status == TRUE)
		{
		    teletext_set_codepage(VTCODEPAGE_NONE);
		    if (context_tele.TtxCodepage >= VTCODEPAGE_ENGLISH && context_tele.TtxCodepage < VTCODEPAGE_LASTONE)
		    {
		        SetCodepage(context_tele.TtxCodepage);
		    }
		}
		TELETEXT_DRAW(&context_tele, ({
        	DrawPage(&context_tele.sCurrPage, 0/*VTDF_HEADERONLY*/, VTDoubleProfile);
		}));
    	}
    	else
    	{
        	memcpy(szOldClock, &context_tele.sCurrPage.Frame[16][32], 8);
        	GetDisplayHeader(&context_tele.sCurrPage, TRUE);


    	}

    	return AM_SUCCESS;

}

static int teletext_page_update(INT32U dwPageCode)
{
    	int i;

    	if (context_tele.status != TELETEXT_STARTED)
    	{
        	return AM_TT_ERR_NOT_START;
    	}

    	if (LOWWORD(dwPageCode) == context_tele.wPageIndex)
    	{
        	if (context_tele.wPageSubCode == 0xFFFF || HIGHWORD(dwPageCode) == context_tele.wPageSubCode)
        	{
            		dwPageCode = GetDisplayPage(dwPageCode, &context_tele.sCurrPage);
            		GetDisplayHeader(&context_tele.sCurrPage, TRUE);
			if (context_tele.auto_page_status == TRUE)
			{
			    teletext_set_codepage(VTCODEPAGE_NONE);
			    if (context_tele.TtxCodepage >= VTCODEPAGE_ENGLISH && context_tele.TtxCodepage < VTCODEPAGE_LASTONE)
			    {
			        SetCodepage(context_tele.TtxCodepage);
			    }
			}
            		for (i = 0; i < 23; i++)
            		{
               		 // If an updated line was previously drawn as
                	// double height, we need to force an update
                	// on the line proceeding incase the new line
                	// is no longer double height.

                		if ((context_tele.sCurrPage.LineState[i] & CACHESTATE_UPDATED) != 0)
                		{
                    			if (VTDoubleProfile[i] != FALSE)
                    			{
                        			context_tele.sCurrPage.LineState[i + 1] |= CACHESTATE_UPDATED;
                    			}
                		}
            		}
			TELETEXT_DRAW(&context_tele, ({
            		DrawPage(&context_tele.sCurrPage, 0/*VTDF_UPDATEDONLY*/, VTDoubleProfile);
			}));
       		}
       		return AM_SUCCESS;

    	}

    	return AM_FAILURE;

}

static int teletext_page_refresh(INT32U dwPageCode)
{
    	INT32U LoadedPageCode;
    	if (context_tele.status != TELETEXT_STARTED)
    	{
        	return AM_TT_ERR_NOT_START;
    	}

    	if (LOWWORD(dwPageCode) == context_tele.wPageIndex)
    	{
        	if (context_tele.wPageSubCode == 0xFFFF && HIGHWORD(dwPageCode) != context_tele.wPageSubCode)
        	{
            		LoadedPageCode = GetDisplayPage(dwPageCode, &context_tele.sCurrPage);
            		GetDisplayHeader(&context_tele.sCurrPage, TRUE);
			if (context_tele.auto_page_status == TRUE)
			{
			    teletext_set_codepage(VTCODEPAGE_NONE);
			    if (context_tele.TtxCodepage >= VTCODEPAGE_ENGLISH && context_tele.TtxCodepage < VTCODEPAGE_LASTONE)
			    {
			        SetCodepage(context_tele.TtxCodepage);
			    }
			}

			TELETEXT_DRAW(&context_tele, ({
            		DrawPage(&context_tele.sCurrPage, 0, VTDoubleProfile);
			}));

            		return AM_SUCCESS;
        	}
    	}

    	return AM_FAILURE;

}

static BOOLEAN teletext_set_codepage(int Codepage)
{
    INT8U SubsetCode;
    INT8U nRegion;

    if (Codepage == VTCODEPAGE_NONE)
    {
        SubsetCode =context_tele.sCurrPage.uCharacterSubset;
        nRegion =context_tele.sCurrPage.uCharacterRegion;
	printf("%d %d %d %x\n",nRegion, SubsetCode,context_tele.wPageIndex,context_tele.wPageIndex);
        if (nRegion < VTREGION_LASTONE)
        {
            //context_tele.TtxRegion = nRegion;
        }
        Codepage = GetRegionCodepage(nRegion, SubsetCode, TRUE);
    }
    else
    {
        //TtxUserCodepage = Codepage;
    }

    if (Codepage != context_tele.TtxCodepage)
    {
        context_tele.TtxCodepage = Codepage;

        return TRUE;
    }

    return FALSE;
}




AM_ErrorCode_t AM_TT_SetCurrentPageCode(unsigned short wPageCode, unsigned short wSubPageCode)
{
    	INT32U dwPageCode;
    	INT32S nRetCode = 0;
    	INT8U  nTemp = 0;
    	char auto_page_status;

    	if (context_tele.status != TELETEXT_STARTED)
    	{
        	return AM_TT_ERR_NOT_START;
    	}
    	if (PageHex2ArrayIndex(wPageCode) == 0xFFFF)
    	{
		if(context_tele.ecb!=0)
		{
			context_tele.ecb(AM_TT_EVT_PAGE_ERROR);
        		return AM_FAILURE;
		}
    	}

    	dwPageCode = MAKELONG(wPageCode, wSubPageCode);
    	nRetCode = GetDisplayPage(dwPageCode, &context_tele.sCurrPage);

    	if (nRetCode == 0)
    	{
        	context_tele.wPageIndex =  wPageCode;
        	SetWaitingPage(wPageCode, TRUE);
		TELETEXT_DRAW(&context_tele, ({
        	teletext_draw_display_message(VT_PAGENOTFIND);
		}));

        	return AM_TT_ERR_PAGE_NOT_EXSIT;
    	}
	    if (context_tele.auto_page_status == TRUE)
	    {
	        teletext_set_codepage(VTCODEPAGE_NONE);
	        if (context_tele.TtxCodepage >= VTCODEPAGE_ENGLISH && context_tele.TtxCodepage < VTCODEPAGE_LASTONE)
	        {
	            SetCodepage(context_tele.TtxCodepage);
	        }
	    }



    	GetDisplayHeader(&context_tele.sCurrPage, TRUE);

	TELETEXT_DRAW(&context_tele, ({
    	DrawPage(&context_tele.sCurrPage, 0, VTDoubleProfile);
	}));

    	context_tele.wPageIndex =  wPageCode ;
    	context_tele.wPageSubCode = wSubPageCode ;
    	SetTeletextCurrPageIndex(context_tele.wPageIndex);
    	teletext_history_push_page(context_tele.wPageIndex);

    return AM_SUCCESS;

}


static void teletext_page_process_callback(INT32U event, INT32U index)
{

	context_tele.cb_parameter = index;	
	switch (event)
	{
		case  DECODEREVENT_HEADERUPDATE:
		teletext_header_update(context_tele.cb_parameter);
		break;
		case DECODEREVENT_PAGEUPDATE:
		teletext_page_update(context_tele.cb_parameter);
		break;
		case DECODEREVENT_PAGEREFRESH:
		teletext_page_refresh(context_tele.cb_parameter);
		break;
		case DECODEREVENT_GETWAITINGPAGE:
		AM_TT_SetCurrentPageCode(LOWWORD(context_tele.cb_parameter), HIGHWORD(context_tele.cb_parameter));
		break;
		default:
		break;
	}

	if(context_tele.ecb)
	{
		context_tele.ecb(AM_TT_EVT_PAGE_UPDATE);
	}
	return;
}


unsigned int AM_TT_GetCurrentPageCode()
{
    	if (context_tele.status != TELETEXT_STARTED)
    	{
        	return 0;
    	}

    	return MAKELONG(context_tele.wPageIndex, context_tele.wPageSubCode);

}

AM_ErrorCode_t AM_TT_Init(int buffer_size)
{
	int ret=AM_SUCCESS;
	if(buffer_size<=0)
	{
		buffer_size=DEFAULT_BUFFER_SIZE;
		
	}

	context_tele.pwb.start_ptr=(char *)malloc(buffer_size);
	context_tele.pwb.length=buffer_size;
	if(!context_tele.pwb.start_ptr)
	{
		ret=AM_TT_ERR_NO_MEM;
		goto exit;
	}
	Vtdrawer_init();
	Mosaic_init();
	
	Draw_RegisterCallback(teletext_fill_rectangle,teletext_draw_text,teletext_convert_color,teletext_get_font_height,teletext_get_font_max_width,teletext_clean_osd);
	Mosaic_RegisterCallback(teletext_fill_rectangle,teletext_convert_color);

	pthread_mutex_init(&context_tele.rd_wr_Mutex,0);
	pthread_cond_init(&context_tele.rd_wr_Cond,0);

	context_tele.flags=0;
	context_tele.status=TELETEXT_INILIZED;
	context_tele.fr=NULL;
	context_tele.dt=NULL;
	context_tele.cc=NULL;
	context_tele.gfh=NULL;
	context_tele.gfw=NULL;
	context_tele.co=NULL;
	context_tele.draw_begin=NULL;
	context_tele.update_display=NULL;

	context_tele.auto_page_status=TRUE;

	
	context_tele.ecb=0;
	return ret;
exit:
	Vtdrawer_deinit();
	Mosaic_deinit();
		
	return ret;
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
	if(context_tele.pwb.wr_ptr>=context_tele.pwb.end_ptr)
	{
		if(context_tele.pwb.rd_ptr!=context_tele.pwb.start_ptr&&context_tele.pwb.rd_ptr!=context_tele.pwb.end_ptr)
		{
			//ring write ptr
			context_tele.pwb.wr_ptr=context_tele.pwb.start_ptr;
			printf("Ring write ptr\n");

		}
		else
		{
			//buffer overflow
			printf("buffer overflow\n");
			return 0;
		}
	}

	if(context_tele.pwb.rd_ptr==context_tele.pwb.start_ptr&&context_tele.pwb.wr_ptr==context_tele.pwb.start_ptr)
	{

		//reset buffer
		context_tele.pwb.rd_ptr=context_tele.pwb.end_ptr;
		printf("reset buffer\n");
	}


	if(context_tele.pwb.rd_ptr>context_tele.pwb.wr_ptr)
	{
		space=context_tele.pwb.rd_ptr-context_tele.pwb.wr_ptr;
		if(space>=len)
		{
			memcpy(context_tele.pwb.wr_ptr,buffer,len);
			context_tele.pwb.wr_ptr+=len;
			return len;
		}
		else
		{
			//less space
			memcpy(context_tele.pwb.wr_ptr,buffer,space);
			context_tele.pwb.wr_ptr+=space;
			return space;

		}
	}
	if(context_tele.pwb.rd_ptr<=context_tele.pwb.wr_ptr)
	{
		if(context_tele.pwb.rd_ptr==context_tele.pwb.wr_ptr)
		{
			if(context_tele.pwb.last_rd_wr==1)
			{
				return 0;	
			}
	
		}
		int first_space=0;
		int second_space=0;
	
		first_space=context_tele.pwb.end_ptr-context_tele.pwb.wr_ptr;
		second_space=context_tele.pwb.rd_ptr-context_tele.pwb.start_ptr;
		space=first_space+second_space;

		if(len<=space)
		{

			if(len-first_space>0)
			{

				memcpy(context_tele.pwb.wr_ptr,buffer,first_space);
				context_tele.pwb.wr_ptr=context_tele.pwb.start_ptr;
				memcpy(context_tele.pwb.wr_ptr,buffer+first_space,len-first_space);
				context_tele.pwb.wr_ptr+=(len-first_space);
				return len;

			}
			else
			{
				memcpy(context_tele.pwb.wr_ptr,buffer,len);
				if(context_tele.pwb.wr_ptr+len>=context_tele.pwb.end_ptr)
				{
					context_tele.pwb.wr_ptr=context_tele.pwb.start_ptr;
				}
				else
				{
					context_tele.pwb.wr_ptr+=len;
				}
				return len;

			}
		}
		else
		{
			//less space

			if(first_space>0)
			{
				memcpy(context_tele.pwb.wr_ptr,buffer,first_space);
				context_tele.pwb.wr_ptr=context_tele.pwb.start_ptr;
				memcpy(context_tele.pwb.wr_ptr,buffer+first_space,second_space);
				context_tele.pwb.wr_ptr+=second_space;
				return first_space+second_space;
			}
			else
			{
				context_tele.pwb.wr_ptr=context_tele.pwb.start_ptr;
				memcpy(context_tele.pwb.wr_ptr,buffer,second_space);
				context_tele.pwb.wr_ptr+=second_space;
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
#ifdef DEBUG_TELE

	if(fp2==0)
	{
		fp2=fopen("data_cb.dat","wb+");
	}

	fwrite(data,1,len,fp2);
#endif
	pthread_mutex_lock(&context_tele.rd_wr_Mutex);

	while(data_remain>0&&context_tele.flags&TELTE_RUNNING_FLAG)
	{
		data_copy+=copy_data_to_pes_buffer((char*)data+data_copy,data_remain);
		data_remain-=data_copy;
	}
	pthread_mutex_unlock(&context_tele.rd_wr_Mutex);
	pthread_cond_signal(&context_tele.rd_wr_Cond);
}

static int open_demux_pes_filter(int dev_no,int pid,int type)
{
	int fhandle=-1;
	AM_ErrorCode_t ret = AM_SUCCESS;
	struct dmx_pes_filter_params  param;
	if(dev_no>MAX_DMX_COUNT)
	{
		ret=AM_TT_ERR_OPEN_PES;
		goto exit;
	}
	ret=AM_DMX_AllocateFilter(dev_no,&fhandle);
	if(ret)
	{
		ret=AM_TT_ERR_OPEN_PES;
		goto exit;

	}
	ret=AM_DMX_SetCallback(dev_no,fhandle,pes_filter_callback,0);
	if(ret)
	{
		ret=AM_TT_ERR_OPEN_PES;
		goto error;
	}
	ret=AM_DMX_SetBufferSize(dev_no,fhandle,0x80000);
	if(ret)
	{
		ret=AM_TT_ERR_OPEN_PES;
		goto error;
	}
	param.pid=pid;
	param.output=1;
	param.pes_type=type;
	ret=AM_DMX_SetPesFilter(dev_no,fhandle,&param);

	if(ret)
	{
		ret=AM_TT_ERR_OPEN_PES;
		goto error;
	}

	ret=AM_DMX_StartFilter(dev_no,fhandle);

	if(ret)
	{
		ret=AM_TT_ERR_OPEN_PES;
		goto error;
	}

	context_tele.fhandle=fhandle;
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
	if(context_tele.fhandle>=0)
	{
		AM_DMX_FreeFilter(context_tele.dmx_id,context_tele.fhandle);

	}
	return 0;
}

#endif


AM_ErrorCode_t AM_TT_Start(int dmx_id, int pid, int mag, int page, int sub_page)
{
	int ret=AM_SUCCESS;
	int pageIndex;

    	pageIndex = page;
    	pageIndex |= (mag ? (mag*0x100) : 0x800);

	ret=AM_VT_Init(pageIndex);
	
#ifdef LINUX_PES_FILTER
	ret=open_demux_pes_filter(dmx_id,pid,2);
#else
	ret=AM_PesFilter_Open(dmx_id,pid,2);
#endif
	if(ret)
	{
		ret=AM_TT_ERR_OPEN_PES;
		goto error_open;

	}
#ifdef LINUX_PES_FILTER
	context_tele.pwb.wr_ptr=context_tele.pwb.start_ptr;
	context_tele.pwb.end_ptr=context_tele.pwb.start_ptr+context_tele.pwb.length;
	context_tele.pwb.rd_ptr=context_tele.pwb.end_ptr; 	

#else
	ret=AM_PesFilter_Buffer(dmx_id,&context_tele.pwb);
	if(ret)
	{
		ret=AM_TT_ERR_SET_BUFFER;
		goto error_set_buffer;
	}
#endif
	context_tele.dmx_id=dmx_id;
    	SetTeletextMaxPageNum(TELETEXT_MAX_PAGENUM);
    	TeletextRegisterfunc(teletext_page_process_callback);

	context_tele.wPageIndex = pageIndex;
	context_tele.wPageSubCode = sub_page;
	context_tele.status=TELETEXT_STARTED;
	teletext_history_reset();
	goto exit;
#ifndef LINUX_PES_FILTER
error_set_buffer:
	AM_PesFilter_Close(dmx_id);
#endif
error_open:
	AM_VT_Exit();
exit:
	/*if no error, create the teletext thread*/
	if (! ret)
	{
		context_tele.flags=TELTE_RUNNING_FLAG;
		pthread_create(&context_tele.teletext_thread,NULL,teletext_thread,NULL);
	}
	else
	{
		AM_DEBUG(0, "Start teletext failed");
		context_tele.flags&=~TELTE_RUNNING_FLAG;
	}
	
	return ret;

}

AM_ErrorCode_t AM_TT_Quit()
{
	Vtdrawer_deinit();
	Mosaic_deinit();
	pthread_mutex_destroy(&context_tele.rd_wr_Mutex);
	pthread_cond_destroy(&context_tele.rd_wr_Cond);
	
	return 0;
}

AM_ErrorCode_t AM_TT_Stop()
{
#ifdef LINUX_PES_FILTER
	close_demux_pes_filter();
#else
	AM_PesFilter_Close(context_tele.dmx_id);
#endif
	context_tele.status=TELETEXT_STOPED;

	AM_DEBUG(1,"@@ wait for teletext thread exit...");
	if (context_tele.flags&TELTE_RUNNING_FLAG)
	{
		context_tele.flags&=~TELTE_RUNNING_FLAG;
		pthread_cond_signal(&context_tele.rd_wr_Cond);
		pthread_join(context_tele.teletext_thread,NULL);
	}
	AM_DEBUG(1,"@@ teletext thread exit ok");
	
	AM_VT_Exit();
	return 0;

}


static INT32S teletext_fill_rectangle(INT32S left, INT32S top, INT32U width, INT32U height, INT32U color)
{
	if(context_tele.fr)
	{
		context_tele.fr(left,top,width,height,color);
	}
	return 0;
}

static INT32S teletext_draw_text(INT32S x, INT32S y, INT32U width, INT32U height, INT16U *text, INT32S len, INT32U color, INT32U w_scale, INT32U h_scale)
{
	if(context_tele.dt)
	{
		context_tele.dt(x,y,width,height,text,len,color,w_scale,h_scale);
	}
	return 0;

}

static INT32U teletext_convert_color(INT32U index, INT32U type)
{
	if(context_tele.cc)
	{
		return context_tele.cc(index, type);
	}
	return 0;


}

static INT32U teletext_get_font_height(void)
{
	if(context_tele.gfh)
	{
		return context_tele.gfh();
	}
	return 20;


}

static INT32U teletext_get_font_max_width(void)
{
	if(context_tele.gfw)
	{
		return context_tele.gfw();
	}
	return 10;


}

static void teletext_clean_osd()
{
	if(context_tele.co)		
	{
		context_tele.co();
	}
}



AM_ErrorCode_t AM_TT_RegisterDrawOps(AM_TT_DrawOps_t *ops)
{
	context_tele.fr=ops->fill_rect;
	context_tele.dt=ops->draw_text;
	context_tele.cc=ops->conv_color;
	context_tele.gfh=ops->font_height;
	context_tele.gfw=ops->font_width;
	context_tele.draw_begin = ops->draw_begin;
	context_tele.update_display=ops->disp_update;
	context_tele.co=ops->clear_disp;

	return 0;
}

AM_ErrorCode_t AM_TT_NextSubPage()
{
    INT32U dwPageCode;
    INT32S nRetCode = 0;
    INT16U wOldPageIndex = context_tele.wPageIndex;

    if (context_tele.status != TELETEXT_STARTED)
    {
        return AM_TT_ERR_NOT_START;
    }

    dwPageCode = MAKELONG(context_tele.wPageIndex, context_tele.wPageSubCode);

    nRetCode = GetNextDisplaySubPage(dwPageCode, &context_tele.sCurrPage, FALSE);
    if (nRetCode == 0)
    {
        return AM_TT_ERR_PAGE_NOT_EXSIT;
    }

    GetDisplayHeader(&context_tele.sCurrPage, TRUE);
    
    TELETEXT_DRAW(&context_tele, ({
    DrawPage(&context_tele.sCurrPage, 0, VTDoubleProfile);
    }));
    
    context_tele.wPageIndex =  LOWWORD(nRetCode);
    context_tele.wPageSubCode = HIGHWORD(nRetCode);

    if(wOldPageIndex != context_tele.wPageIndex)
    {
        SetTeletextCurrPageIndex(context_tele.wPageIndex);
        teletext_history_push_page(context_tele.wPageIndex);
    }

    return AM_SUCCESS;
}

AM_ErrorCode_t AM_TT_PreviousSubPage()
{
    INT32U dwPageCode;
    INT32S nRetCode = 0;
    INT16U wOldPageIndex = context_tele.wPageIndex;

    if (context_tele.status != TELETEXT_STARTED)
    {
        return AM_TT_ERR_NOT_START;
    }

    dwPageCode = MAKELONG(context_tele.wPageIndex, context_tele.wPageSubCode);

    nRetCode = GetNextDisplaySubPage(dwPageCode, &context_tele.sCurrPage, TRUE);
    if (nRetCode == 0)
    {
        return AM_TT_ERR_PAGE_NOT_EXSIT;
    }

    GetDisplayHeader(&context_tele.sCurrPage, TRUE);

    TELETEXT_DRAW(&context_tele, ({
    DrawPage(&context_tele.sCurrPage, 0, VTDoubleProfile);
    }));

    context_tele.wPageIndex =  LOWWORD(nRetCode);
    context_tele.wPageSubCode = HIGHWORD(nRetCode);

    if(wOldPageIndex != context_tele.wPageIndex)
    {
        SetTeletextCurrPageIndex(context_tele.wPageIndex);
        teletext_history_push_page(context_tele.wPageIndex);
    }

    return AM_SUCCESS;

}


AM_ErrorCode_t AM_TT_NextPage()
{
    INT32U dwPageCode;
    INT32S nRetCode = 0;

    if (context_tele.status != TELETEXT_STARTED)
    {
        return AM_TT_ERR_NOT_START;

    }

    dwPageCode = MAKELONG(context_tele.wPageIndex, context_tele.wPageSubCode);


    nRetCode = GetNextDisplayPage(dwPageCode, &context_tele.sCurrPage, FALSE);

    if (nRetCode == 0)
    {
        return AM_TT_ERR_PAGE_NOT_EXSIT;;
    }
    if (context_tele.auto_page_status == TRUE)
    {
        teletext_set_codepage(VTCODEPAGE_NONE);
        if (context_tele.TtxCodepage >= VTCODEPAGE_ENGLISH && context_tele.TtxCodepage < VTCODEPAGE_LASTONE)
        {
            SetCodepage(context_tele.TtxCodepage);
        }
    }


    GetDisplayHeader(&context_tele.sCurrPage, TRUE);

    TELETEXT_DRAW(&context_tele, ({
    DrawPage(&context_tele.sCurrPage, 0, VTDoubleProfile);
    }));

    context_tele.wPageIndex =  LOWWORD(nRetCode);
    context_tele.wPageSubCode = HIGHWORD(nRetCode);

    SetTeletextCurrPageIndex(context_tele.wPageIndex);
    teletext_history_push_page(context_tele.wPageIndex);


    return AM_SUCCESS;

}

AM_ErrorCode_t AM_TT_PreviousPage()
{
    INT32U dwPageCode;
    INT32S nRetCode = 0;

    if (context_tele.status != TELETEXT_STARTED)
    {
        return AM_TT_ERR_NOT_START;

    }
    

    dwPageCode = MAKELONG(context_tele.wPageIndex, context_tele.wPageSubCode);

    nRetCode = GetNextDisplayPage(dwPageCode, &context_tele.sCurrPage, TRUE);

    if (nRetCode == 0)
    {
        return AM_TT_ERR_PAGE_NOT_EXSIT;
    }


    if (context_tele.auto_page_status == TRUE)
    {
        teletext_set_codepage(VTCODEPAGE_NONE);
        if (context_tele.TtxCodepage >= VTCODEPAGE_ENGLISH && context_tele.TtxCodepage < VTCODEPAGE_LASTONE)
        {
            SetCodepage(context_tele.TtxCodepage);
        }
    }

    GetDisplayHeader(&context_tele.sCurrPage, TRUE);

    TELETEXT_DRAW(&context_tele, ({
    DrawPage(&context_tele.sCurrPage, 0, VTDoubleProfile);
    }));

    context_tele.wPageIndex =  LOWWORD(nRetCode);
    context_tele.wPageSubCode = HIGHWORD(nRetCode);

    SetTeletextCurrPageIndex(context_tele.wPageIndex);
    teletext_history_push_page(context_tele.wPageIndex);

    return AM_SUCCESS;

}

AM_ErrorCode_t AM_TT_PerformColorLink(unsigned char color)
{
    INT32U dwPageCode = 0;
    INT16U wPageHex = 0;
    INT16U wPageSubCode = 0;
    INT32U uLoadedPageCode = 0;
    unsigned char auto_page_status;

    if (context_tele.status != TELETEXT_STARTED)
    {
        return AM_TT_ERR_NOT_START;
    }

    dwPageCode = context_tele.sCurrPage.EditorialLink[color];

    wPageHex = LOWWORD(dwPageCode);
    wPageSubCode = HIGHWORD(dwPageCode);

    if (wPageHex == 0 || (wPageHex & 0xFF) == 0xFF)
    {
        return AM_FAILURE;;
    }

    if (wPageSubCode >= 0x3F7F)
    {
        wPageSubCode = 0xFFFF;
    }

    if (wPageHex == VTPAGE_PREVIOUS)
    {
        wPageHex = teletext_history_pop_lastpage(context_tele.wPageIndex);
        dwPageCode = MAKELONG(wPageHex, wPageSubCode);
    }

    uLoadedPageCode = GetDisplayPage(dwPageCode, &context_tele.sCurrPage);
#if 0
    context_tele.get_auto_code_page(&auto_page_status);
#endif
    if (uLoadedPageCode != 0)
    {
    if (context_tele.auto_page_status == TRUE)
    {
        teletext_set_codepage(VTCODEPAGE_NONE);
        if (context_tele.TtxCodepage >= VTCODEPAGE_ENGLISH && context_tele.TtxCodepage < VTCODEPAGE_LASTONE)
        {
            SetCodepage(context_tele.TtxCodepage);
        }
    }



        GetDisplayHeader(&context_tele.sCurrPage, TRUE);

        TELETEXT_DRAW(&context_tele, ({
        DrawPage(&context_tele.sCurrPage, 0, VTDoubleProfile);
        }));

        context_tele.wPageIndex =  LOWWORD(uLoadedPageCode);
        context_tele.wPageSubCode = HIGHWORD(uLoadedPageCode);
        SetTeletextCurrPageIndex(context_tele.wPageIndex);
        teletext_history_push_page(context_tele.wPageIndex);
    }
    else
    {
        context_tele.wPageIndex = wPageHex;
        SetWaitingPage(wPageHex, TRUE);

	TELETEXT_DRAW(&context_tele, ({
        teletext_draw_display_message(VT_PAGENOTFIND);
	}));
    }

    return AM_SUCCESS;
}

AM_ErrorCode_t AM_TT_RegisterEventCb(AM_TT_EventCb_t cb)
{
	context_tele.ecb=cb;
	return AM_SUCCESS;
}




