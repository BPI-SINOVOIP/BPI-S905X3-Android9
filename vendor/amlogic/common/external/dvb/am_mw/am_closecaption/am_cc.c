/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file am_cc.c
 * \brief cc module
 *
 * \author Ke Gong <ke.gong@amlogic.com>
 * \date 2015-08-11: create the document
 ***************************************************************************/
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <poll.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include "am_misc.h"
#include "am_av.h"
#include "am_aout.h"
#include "am_cc_decoder.h"
#include "am_userdata.h"
#include "am_debug.h"
#include <am_vbi.h>
#include "am_vbi_internal.h"
#include "am_xds.h"

/*
*===============================================================
*                    macro define
*===============================================================
*/
#ifdef ANDROID
#define TAG      "ATSC_CC"
#define ATSC_CC_DBG(a...) __android_log_print(ANDROID_LOG_INFO, TAG, a)
#else
#define ATSC_CC_DBG(a...) printf(a)
#endif

#ifdef PTHREAD_RECURSIVE_MUTEX_INITIALIZER
static pthread_mutex_t cc_lock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER;
#else
static pthread_mutex_t cc_lock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
#endif
#define am_cc_lock     pthread_mutex_lock(&cc_lock)
#define am_cc_unlock   pthread_mutex_unlock(&cc_lock)

pthread_t cc_thread_id;
static int run_cc_thread = 0;
static int run_cc_vbi_thread = CC_STATE_STOP ;
static int fid;

//extern void AM_CC_Cmd(int cmd);
/*
** set ntsc current major and monir numner,format as:
** MSB 4 btyes as major number,LSB 4 bytes as minor numner
** like as :0x20000 means NTSC channel 2-0
** this convience to query dvb.db database base on ntsc virtual 
** channel numner,set by APP
*/
extern void AM_Set_XDSCurrentChanNumber(int ntscchannumber);


void *pthread_cc(void)
{
        uint8_t cc_data[256];/*In fact, 99 is enough*/
        int cc_data_cnt;

        while (run_cc_thread > 0)
        {
            am_cc_lock;
            if(run_cc_thread == CC_STATE_STOP){
                am_cc_unlock;
                continue;
            }
            am_cc_unlock;
            cc_data_cnt = AM_USERDATA_Read(0, cc_data, sizeof(cc_data), 1000);
            if (cc_data_cnt > 4 && 
            cc_data[0] == 0x47 &&
            cc_data[1] == 0x41 && 
            cc_data[2] == 0x39 &&
            cc_data[3] == 0x34)
            {			
                if (cc_data[4] == 0x03 /* 0x03 indicates cc_data */)
                {
                    //printf("===========================================\n");
                    //printf("the %d times found %d byte cc data.\n", count, cc_data_cnt);
                    //am_print_data(cc_data_cnt, cc_data);
                    AM_Parse_ATSC_CC_Data(cc_data+5, cc_data_cnt-5);//needn't to parse the first 5bytes
                }
            }
            usleep(1000);
        }

        return NULL;
}


static void am_dump_bytes(int dev_no, int fid, const uint8_t *data, int len, void *user_data){

        ATSC_CC_DBG("dump_bytes  len = %d\n",len);
        AM_NTSC_VBI_Decoder_Test( dev_no,  fid, data,  len,  user_data);
}

/*
**   handle closecaption command input from application
**
*/
void AM_CC_Cmd(int cmd)
{
    if(cmd == CMD_CC_START){
      if(CC_ATSC_USER_DATA == AM_Get_CC_SourceType()){
          int ud_dev_no = 0;
          AM_USERDATA_OpenPara_t para;
          uint8_t cc_data[256];/*In fact, 99 is enough*/
          int cc_data_cnt;

          if(run_cc_thread == CC_STATE_STOP){
              am_cc_lock;
              ATSC_CC_DBG("********* to start the cc thread *************");
              run_cc_thread = CC_STATE_RUNNING;
              am_cc_unlock;
              return;
          }  
        
          if (AM_USERDATA_Open(0, &para) != AM_SUCCESS){
            return;
          }
          am_cc_lock;
          run_cc_thread = CC_STATE_RUNNING;
          am_cc_unlock;
          pthread_create(&cc_thread_id, NULL, (void*)pthread_cc, NULL);
      }
      else if(CC_NTSC_VBI_LINE21 == AM_Get_CC_SourceType()){
          AM_VBI_OpenPara_t para;
  
  
          if(CC_STATE_RUNNING  == run_cc_vbi_thread){
               ATSC_CC_DBG("cc_vbi_thread alrady created, please stop and re-start again!\n");
               return;
          }

          memset(&para, 0, sizeof(para));

          ATSC_CC_DBG("***************Start CC_VBI thread !\n");
          if(AM_Initialize_XDSDataServices() != AM_SUCCESS) {
              ATSC_CC_DBG("###############AM_Initialize_XDSDataServices fail!\n");
              return;
          }

          if(AM_NTSC_VBI_Open(VBI_DEV_NO, &para) != AM_SUCCESS) {
              ATSC_CC_DBG("###############AM_NTSC_VBI_Open fail!\n");
              return;
          } 

          if(AM_NTSC_VBI_AllocateFilter(VBI_DEV_NO, &fid) != AM_SUCCESS) {
               ATSC_CC_DBG("###############AM_NTSC_VBI_AllocateFilter fail!\n");
               AM_NTSC_VBI_Close(VBI_DEV_NO);
               AM_Terminate_XDSDataServices();
               return;
          }

          if(AM_NTSC_VBI_SetCallback(VBI_DEV_NO, fid, am_dump_bytes, NULL) !=  AM_SUCCESS) {
               ATSC_CC_DBG("###############AM_NTSC_VBI_SetCallback fail!\n");
               AM_NTSC_VBI_FreeFilter(VBI_DEV_NO,fid);
               AM_NTSC_VBI_Close(VBI_DEV_NO);
               AM_Terminate_XDSDataServices();
               return;
          }

          AM_NTSC_VBI_SetBufferSize(VBI_DEV_NO, fid, 1024);
          if(AM_NTSC_VBI_StartFilter(VBI_DEV_NO, fid) != AM_SUCCESS) {
               ATSC_CC_DBG("###############AM_NTSC_VBI_StartFilter fail!\n");
               AM_NTSC_VBI_FreeFilter(VBI_DEV_NO,fid);
               AM_NTSC_VBI_Close(VBI_DEV_NO);
               AM_Terminate_XDSDataServices();
               return;

          }
 
           run_cc_vbi_thread = CC_STATE_RUNNING;
      }

    }
    else if(cmd == CMD_CC_STOP){
        if(CC_ATSC_USER_DATA == AM_Get_CC_SourceType()){
            if(run_cc_thread != CC_STATE_RUNNING){
                ATSC_CC_DBG("cc thread not created, please create first!");
            	return;
            } 
        
            am_cc_lock;
            ATSC_CC_DBG("********* to stop the cc thread *************");
            run_cc_thread = CC_STATE_STOP;
            am_cc_unlock;
      }
      else if(CC_NTSC_VBI_LINE21 == AM_Get_CC_SourceType()){
           
            if(CC_STATE_RUNNING != run_cc_vbi_thread){
               ATSC_CC_DBG("###############yet not start VBI CC thread!\n");
               return ;
	    } 
            AM_NTSC_VBI_StopFilter(VBI_DEV_NO,fid);
            AM_NTSC_VBI_FreeFilter(VBI_DEV_NO,fid);
            if(AM_NTSC_VBI_Close(VBI_DEV_NO)!= AM_SUCCESS) {
              ATSC_CC_DBG("###############AM_NTSC_VBI_Close  fail!\n");
              return;
            }
            
            AM_Terminate_XDSDataServices();
            run_cc_vbi_thread = CC_STATE_STOP;
      }

    }
    else if(cmd > CMD_SET_COUNTRY_BEGIN && cmd < CMD_SET_COUNTRY_END){
        //AM_Set_CC_SourceType(CMD_CC_SET_VBIDATA);
        AM_Set_CC_Country(cmd);
    }
    else if(cmd > CMD_CC_BEGIN && cmd < CMD_CC_END){
        am_cc_lock;
        ATSC_CC_DBG("*********  set type to stop the cc thread *************");
        run_cc_thread = CC_STATE_STOP;
        am_cc_unlock;
        
        AM_Force_Clear_CC();
        AM_Set_Cur_CC_Type(cmd);
		/*
        if(cmd > CMD_CC_BEGIN && cmd < CMD_SERVICE_1)
            AM_Set_CC_SourceType(CMD_CC_SET_VBIDATA);
        else
            AM_Set_CC_SourceType(CMD_CC_SET_USERDATA);
        */
        am_cc_lock;
        ATSC_CC_DBG("********* set type to start the cc thread *************");
        run_cc_thread = CC_STATE_RUNNING;
        am_cc_unlock;

        return;
    }
    else if(CMD_CC_SET_VBIDATA == cmd || CMD_CC_SET_USERDATA == cmd){
        AM_Set_CC_SourceType(cmd);
    }
    else if(CMD_CC_SET_CHAN_NUM == cmd){
         AM_Set_XDSCurrentChanNumber(cmd);//presume set as NTSC channel number like,for example 2-0 is 0x20000 for test xds storage process
    }
    else if(CMD_VCHIP_RST_CHGSTAT == cmd){
         Am_Reset_Vchip_Change_Status(); //stop NTSC vchip callback
    }    
}

void AM_Set_CurrentChanNumber(int ntscchannumber)
{
	AM_Set_XDSCurrentChanNumber(ntscchannumber);
}

