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
 * \brief 输入设备测试程序
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-05-21: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 5

#include <am_debug.h>
#include <am_inp.h>
#include <string.h>
#include <unistd.h>

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

#define INP_DEV_NO    (0)
#define INP_TIMEOUT   (500)

/****************************************************************************
 * Functions
 ***************************************************************************/

static AM_Bool_t async_end;

static AM_Bool_t is_end_key(int code)
{
	return (code=='q');
}

static void inp_cb(int dev_no, struct input_event *evt, void *user_data)
{
	printf("cb: event type: %d, key code: 0x%x, value:%d\n", evt->type, evt->code, evt->value);
	if(is_end_key(evt->code)) async_end = AM_TRUE;
}

static void inp_disable_cb(int dev_no, struct input_event *evt, void *user_data)
{
	printf("ERROR: get key when input disabled\n");
}

int main(int argc, char **argv)
{
	AM_INP_OpenPara_t para;
	struct input_event evt;
	AM_ErrorCode_t ret;
	AM_Bool_t loop = AM_TRUE;
	
	while(loop)
	{
		char buf[64];
		
		printf("input test\n");
		printf("s.sync mode\n");
		printf("a.async mode\n");
		printf("d.disable input\n");
		printf("q.quit\n");
		if(scanf("%s", buf)!=1) continue;
		
		switch(buf[0])
		{
			case 'q':
			case 'Q':
				loop=AM_FALSE;
			break;
			case 'a':
			case 'A':
				memset(&para, 0, sizeof(para));
				para.enable_thread = AM_TRUE;
				AM_TRY(AM_INP_Open(INP_DEV_NO, &para));
				async_end = AM_FALSE;
				AM_TRY(AM_INP_SetCallback(INP_DEV_NO, inp_cb, NULL));
				while(!async_end)
				{
					usleep(1000000);
				}
				AM_TRY(AM_INP_SetCallback(INP_DEV_NO, NULL, NULL));
				AM_INP_Close(INP_DEV_NO);
			break;
			case 's':
			case 'S':
				memset(&para, 0, sizeof(para));
				AM_TRY(AM_INP_Open(INP_DEV_NO, &para));
				while(1)
				{
					ret = AM_INP_WaitEvent(INP_DEV_NO, &evt, INP_TIMEOUT);
					if(ret==AM_SUCCESS)
					{
						printf("event type: %d, key code: 0x%x, value: %d\n", evt.type, evt.code, evt.value);
						if(is_end_key(evt.code)) break;
					}
				}
				AM_INP_Close(INP_DEV_NO);
			break;
			case 'd':
			case 'D':
				printf("disable input\n");
				memset(&para, 0, sizeof(para));
				AM_TRY(AM_INP_Open(INP_DEV_NO, &para));
				
				AM_INP_Disable(INP_DEV_NO);
				
				printf("wait 5000 ms, read input in sync mode\n");
				
				ret = AM_INP_WaitEvent(INP_DEV_NO, &evt, 5000);
				if(ret==AM_SUCCESS)
				{
					printf("ERROR: get key when input disabled\n");
				}
				AM_INP_Close(INP_DEV_NO);
				
				
				memset(&para, 0, sizeof(para));
				para.enable_thread = AM_TRUE;
				AM_TRY(AM_INP_Open(INP_DEV_NO, &para));
				
				AM_INP_Disable(INP_DEV_NO);
				
				printf("wait 5000 ms, read input in async mode\n");
				
				AM_TRY(AM_INP_SetCallback(INP_DEV_NO, inp_disable_cb, NULL));
				usleep(5000000);
				AM_TRY(AM_INP_SetCallback(INP_DEV_NO, NULL, NULL));
				
				printf("enable input\n");
				AM_INP_Enable(INP_DEV_NO);
				AM_INP_Close(INP_DEV_NO);
			break;
		}
	}
	
	return 0;
}

