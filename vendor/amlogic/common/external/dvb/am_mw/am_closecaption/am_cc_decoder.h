/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package. *
* Description:
*/
#ifndef _AM_CC_DECODER_H_
#define _AM_CC_DECODER_H_

#include <am_types.h>

typedef enum
{
 CC_STATE_RUNNING      = 0x1001,
 CC_STATE_STOP                 ,

 CMD_CC_START          = 0x2001,
 CMD_CC_STOP                   ,     

 CMD_CC_BEGIN          = 0x3000,
 CMD_CC_1                      ,
 CMD_CC_2                      ,
 CMD_CC_3                      ,
 CMD_CC_4                      ,

 //this doesn't support currently
 CMD_TT_1              = 0x3005,
 CMD_TT_2                      ,
 CMD_TT_3                      ,
 CMD_TT_4                      ,
 
 //cc service
 CMD_SERVICE_1         = 0x4001,
 CMD_SERVICE_2                 ,
 CMD_SERVICE_3                 ,
 CMD_SERVICE_4                 ,
 CMD_SERVICE_5                 ,
 CMD_SERVICE_6                 ,
 CMD_CC_END                    ,
 
 CMD_SET_COUNTRY_BEGIN = 0x5000,
 CMD_SET_COUNTRY_USA           ,
 CMD_SET_COUNTRY_KOREA         ,
 CMD_SET_COUNTRY_END           ,
 /*set CC source type according ATV or DTV*/
 CMD_CC_SET_VBIDATA   = 0x7001,
 CMD_CC_SET_USERDATA ,

 CMD_CC_SET_CHAN_NUM = 0x8001,
 CMD_VCHIP_RST_CHGSTAT = 0x9001,
 CMD_CC_MAX
}AM_CLOSECAPTION_cmd_t ;


typedef enum AM_CC_SourceType {
  CC_NTSC_VBI_LINE21 = 0,
  CC_ATSC_USER_DATA
} AM_CC_SourceType_t ;



#ifdef __cplusplus
extern "C"
{
#endif

//void am_print_data(int len, char *p_buf);
void AM_Parse_ATSC_CC_Data(char *buffer, int cnt);
//void am_init_dtvcc_varible(int flag);
void AM_Set_CC_Country(int country);
void AM_Set_CC_SourceType(int cmd);
void AM_Set_Cur_CC_Type(int type);
void AM_Force_Clear_CC();
AM_CC_SourceType_t AM_Get_CC_SourceType(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif     /*_AM_CC_DECODER_H_ */
