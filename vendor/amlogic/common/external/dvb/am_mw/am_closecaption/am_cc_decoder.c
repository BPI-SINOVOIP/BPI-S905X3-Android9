/*===============================================================
 *       Copyright: 
 *       Filename:
 *       Description: 
 *       Version:
 *       Author:
 *       Last modified:
 *       Compiler:
 *       Other Description:
 *
 *===============================================================*/

/*===============================================================
 *                    include files
 *==============================================================*/
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <am_types.h>
#include "am_cc_decoder.h"
#include "ksc5601_to_unicode.h"
#include "am_debug.h"

#include "am_vbi.h"
#include "am_xds.h" 
#include "am_cc_decoder_internal.h"

/*===============================================================
 *                    macro define
 *==============================================================*/
//#define DEBUG_CC

//#define USER_REAL_CC608_PARSE

#ifndef DEBUG_CC
#ifdef ANDROID
#define TAG      "ATSC_CC_DECODER"
#define CC_DECODE_DBG(a...) __android_log_print(ANDROID_LOG_INFO, TAG, a)
#else
#define CC_DECODE_DBG(a...) printf(a)
#endif//end ANDROID
#endif//end DEBUG_CC
#define CC_CNT          512
#define MAX_CC          512
#define MAGIC_NUM      (0xABCD)
#define MAGIC_NUM_CNT   (10)

#define CC_ROLL_UP     (0x81)
#define CC_POP_ON      (0x82)
#define CC_PAINT_ON    (0x83)

//=================================================================
char cc_chars[CC_CNT];
static int cc_cmds[128];
static int cc_char_idx = 0;
static char last_cmd1 = 0, last_cmd2 = 0;
int korea_cc_data[CC_CNT];
int korea_cc_data_idx = 0;
static int cc_korea_us = 0;
char cc_guess_char[CC_CNT];
int cc_guess_idx = 0;
static int last_text_row = -1;
static int cc_country = -1;
static AM_CC_SourceType_t cc_souretype = CC_ATSC_USER_DATA ;
int cc_cur_type = -1;
int temp_cc1_char[CC_CNT];
int temp_cc1_char_idx = 0;
int ntsc_cc_cmd[CC_CNT];
int ntsc_cc_cmd_idx = 0;

int ntsc_cc1_flag = 0;
int temp_row = 0;
int ntsc_cc_show_style = 0;
int ntsc_cc_last_show_style = 0;
int last_preamble_code1 = 0;
int last_preamble_code2 = 0;
AM_CCEvent_t last_level1Event = ILLEGALCODE;
AM_CCEvent_t last_level2Event = ILLEGALCODE;
static int last_ntsc_special_char = 0 ;/*use to drop the duplicated latin extend char*/

int ntsc_extend_12_char[32]=
{
	0xC1,     0xC9,   0xD3,   0xDA, 0xDC,   0xFC,   0x2018,  0xA1,
	0x204E,   0x2C8,  0x2014, 0xA9, 0x2120, 0x2022, 0x201C,  0x201D,
	0xC0,     0xC2,   0xC7,   0xC8, 0xCA,   0xCB,   0xEB,    0xCE,    
	0xCF,     0xEF,   0xD4,   0xD9, 0xF9,   0xDB,   0xAB,    0xBB,
};

int ntsc_extend_13_char[32]={
	0xC3,   0xE3,   0xCD,   0xCC,   0xEC,     0xD2,     0xF2,   0xD5,
	0xF5,   0x7B,   0x7D,   0x5C,   0x5E,     0x5F,     0x7C,   0x7E,
	0xC3,   0xE4,   0xD6,   0xF6,   0xDF,     0xA5,     0xA4,   0xA6,
	0xC5,   0xE5,   0xD8,   0xF8,   0x23A1,   0x23A4,   0x23A3, 0x23A6,
};

struct am_dtvcc_service {
	/* Interpretation Layer. */
	//struct dtvcc_window		window[8];
	//struct dtvcc_window *		curr_window;
	//dtvcc_window_map		created;

	/* Service Layer. */
	char service_data[MAX_CC];
	unsigned int service_data_in;

	/** The time when we last received data for this service. */
	//struct cc_timestamp		timestamp;
};

struct am_dtvcc_decoder {
	struct am_dtvcc_service service[6];
	/* Packet Layer. */
	char packet[MAX_CC];
	unsigned int packet_size;
	/* Next expected DTVCC packet sequence_number. Only the two
	   most significant bits are valid. < 0 if no sequence_number
	   has been received yet. */
	int next_sequence_number;
	/** The time when we last received data. */
	//struct cc_timestamp		timestamp;
};

struct am_dtvcc_decoder am_dtvcc_handle;

typedef void(*AM_CC_CallBack)(char *str, int cnt, int data_buf[], int cmd_buf[], void *user_data);
static AM_CC_CallBack atsc_cc_notify = NULL;
static void *atsc_cc_user_data = NULL;
extern void AM_CC_Set_CallBack(AM_CC_CallBack notify, void *user_data);

typedef void(*AM_VCHIP_CallBack)(int vchip_stat, void *user_data);
static AM_VCHIP_CallBack ntsc_vchip_notify = NULL;
static void *ntsc_vchip_user_data = NULL;
int vbi_has_no_xds = 0;
extern void AM_VCHIP_Set_CallBack(AM_VCHIP_CallBack notify, void *user_data);
static unsigned char filterF1XDS = 0;
static AM_ServiceType_t currentServiceType = INVALIDSERVICE;
static AM_CCDataChannel_t  curCCDecodeChannel = ILLEGALCHANNEL;
static AM_CCEvent_t lastCtrlCmd = ILLEGALCODE ; /*for ExtCode, MiscCtrlCode, MidRowCode, PAC */
/*===============================================================
 *  Function Name:
 *  Function Description:
 *  Function Parameters:
 *  Function Return:
 *===============================================================*/
void am_print_int_data_left(int len, int *p_buf)
{
	if(len == 0)
		return;

	if(len == 1){
		CC_DECODE_DBG("0x%04x ",p_buf[0]);
	}

	if(len == 2){
		CC_DECODE_DBG("0x%04x 0x%04x ",p_buf[0],p_buf[1]);
	}

	if(len == 3){
		CC_DECODE_DBG("0x%04x 0x%04x 0x%04x ",p_buf[0],p_buf[1],p_buf[2]);
	}

	if(len == 4){
		CC_DECODE_DBG("0x%04x 0x%04x 0x%04x 0x%04x ",p_buf[0],p_buf[1],p_buf[2],p_buf[3]);
	}

	if(len == 5){
		CC_DECODE_DBG("0x%04x 0x%04x 0x%04x 0x%04x 0x%04x ",p_buf[0],p_buf[1],p_buf[2],p_buf[3],p_buf[4]);
	}

	if(len == 6){
		CC_DECODE_DBG("0x%04x 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x ",p_buf[0],p_buf[1],p_buf[2],p_buf[3],p_buf[4],p_buf[5]);
	}

	if(len == 7){
		CC_DECODE_DBG("0x%04x 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x ",p_buf[0],p_buf[1],p_buf[2],p_buf[3],p_buf[4],p_buf[5],p_buf[6]);
	}

	return;
}
/*===============================================================
 *  Function Name:
 *  Function Description:
 *  Function Parameters:
 *  Function Return:
 *===============================================================*/
void am_print_int_data(int len, int *p_buf)
{
	int len_temp = len;
	int idx = 0, temp_data;
	char temp_chars[256];

	CC_DECODE_DBG(">>>>>>>>>>>>>>>>>>>>>>> The int data %d are as following:<<<<<<<<<<<<<<<<<<<<<<<<\n", len);

	if(!(len > 0))
		return;

#if 0
	memset(temp_chars, 0, sizeof(temp_chars));
	for(idx = 0; idx < len; idx++){
		memcpy(&temp_data, &p_buf[idx], 1);
		temp_chars[idx] = (char)(temp_data&0x7F);
	}
	CC_DECODE_DBG("The String is:\n%s.\n", temp_chars);
#endif

	if(len < 8){
		am_print_int_data_left(len, p_buf);
		CC_DECODE_DBG("\n");
		return;
	}

	while(len_temp > 7){
		CC_DECODE_DBG("0x%04x 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x\n",p_buf[idx+0],p_buf[idx+1],p_buf[idx+2],
				p_buf[idx+3],p_buf[idx+4],p_buf[idx+5],p_buf[idx+6], p_buf[idx+7]);
		if(len_temp > 7){
			len_temp = len_temp - 8;
			idx = idx + 8;
		}
	}

	am_print_int_data_left(len - idx, p_buf + idx);
	CC_DECODE_DBG("\n");
}
/*===============================================================
 *  Function Name:
 *  Function Description:
 *  Function Parameters:
 *  Function Return:
 *===============================================================*/
void am_print_data_left(int len, char *p_buf)
{
	if(len == 0)
		return;

	if(len == 1){
		CC_DECODE_DBG("0x%02x ",p_buf[0]);
	}

	if(len == 2){
		CC_DECODE_DBG("0x%02x 0x%02x ",p_buf[0],p_buf[1]);
	}

	if(len == 3){
		CC_DECODE_DBG("0x%02x 0x%02x 0x%02x ",p_buf[0],p_buf[1],p_buf[2]);
	}

	if(len == 4){
		CC_DECODE_DBG("0x%02x 0x%02x 0x%02x 0x%02x ",p_buf[0],p_buf[1],p_buf[2],p_buf[3]);
	}

	if(len == 5){
		CC_DECODE_DBG("0x%02x 0x%02x 0x%02x 0x%02x 0x%02x ",p_buf[0],p_buf[1],p_buf[2],p_buf[3],p_buf[4]);
	}

	if(len == 6){
		CC_DECODE_DBG("0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x ",p_buf[0],p_buf[1],p_buf[2],p_buf[3],p_buf[4],p_buf[5]);
	}

	if(len == 7){
		CC_DECODE_DBG("0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x ",p_buf[0],p_buf[1],p_buf[2],p_buf[3],p_buf[4],p_buf[5],p_buf[6]);
	}

	CC_DECODE_DBG("\n");
	return;
}
/**===============================================================
 *  Function Name:
 *  Function Description:
 *  Function Parameters:
 *  Function Return:
 *===============================================================*/
void am_print_data(int len, char *p_buf)
{
	int len_temp = len;
	int idx = 0;

	CC_DECODE_DBG("The data are as following:\n");

	if(len < 8){
		am_print_data_left(len, p_buf);
		CC_DECODE_DBG("\n");
		return;
	}

	while(len_temp > 7){
		CC_DECODE_DBG("0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",p_buf[idx+0],p_buf[idx+1],p_buf[idx+2],
				p_buf[idx+3],p_buf[idx+4],p_buf[idx+5],p_buf[idx+6], p_buf[idx+7]);
		if(len_temp > 7){
			len_temp = len_temp - 8;
			idx = idx + 8;
		}
	}

	am_print_data_left(len - idx, p_buf + idx);
}
/*===============================================================
 *  Function Name:
 *  Function Description:
 *  Function Parameters:
 *  Function Return:
 *===============================================================*/
void AM_CC_Set_CallBack(AM_CC_CallBack notify, void *user_data)
{
    if(notify != NULL) {
        atsc_cc_notify = notify;
        atsc_cc_user_data = user_data;
    }
}

/*===============================================================
 *  Function Name:
 *  Function Description:
 *  Function Parameters:
 *  Function Return:
 *===============================================================*/
static void am_us_atsc_callback()
{
	CC_DECODE_DBG("######## korea_cc_data_idx:%d",korea_cc_data_idx);
	//if(korea_cc_data_idx > 0)
	{
		am_print_int_data(korea_cc_data_idx, korea_cc_data);
		if(atsc_cc_notify != NULL)
			atsc_cc_notify(cc_chars, korea_cc_data_idx, korea_cc_data, cc_cmds, atsc_cc_user_data);

		memset(cc_cmds, 0, sizeof(cc_cmds));
	}
}
/*===============================================================
 *  Function Name: AM_VCHIP_Set_CallBack
 *  Function Description: set atv vchip callback
 *  Function Parameters: notify:callback function
 *  Function Return: void
 *===============================================================*/
void AM_VCHIP_Set_CallBack(AM_VCHIP_CallBack notify, void *user_data)
{
    if(notify != NULL) {
        ntsc_vchip_notify = notify;
        ntsc_vchip_user_data = user_data;
    }
}
static void am_ntsc_vchip_callback()
{
	int vchip_stat = Am_Get_Vchip_Change_Status();
	if(ntsc_vchip_notify != NULL && vchip_stat)
	{
		ntsc_vchip_notify(vchip_stat, ntsc_vchip_user_data);
	}
}
/*===============================================================
 *  Function Name:
 *  Function Description:
 *  Function Parameters:
 *  Function Return:
 *===============================================================*/
static int util_convert_ntsc_special_char_to_unicode(int type_char, int ntsc_char)
{
	if(type_char == 0x11){
		switch(ntsc_char)
		{
			//R in circle
			case 0x30:return 0xAE;
			//¡£
			case 0x31:return 0xB0;  
			///1/2
			case 0x32:return 0xBD;
			//inverse query
			case 0x33:return 0xBF;  
			//TM
			case 0x34:return 0x2122;
			//a slash in c
			case 0x35:return 0xA2;  
			//Euro symbol
			case 0x36:return 0xA3;
			//music note
			case 0x37:return 0x266A; 
			//¨¤
			case 0x38:return 0xE0;
			//transparent space
			case 0x39:return 0x25AF; //it's a smile symbol  
			//¨¨
			case 0x3a:return 0xE8;
			//^+a
			case 0x3b:return 0xE2;  
			//^+e
			case 0x3c:return 0xEA;
			//^+i
			case 0x3d:return 0xEE;  
			//^+o
			case 0x3e:return 0xF4;
			//^+u
			case 0x3f:return 0xFB; 
			default:return 0x20;
		}
	}

	if(type_char == 0x12){
		switch(ntsc_char){
			case 0x20 ... 0x3F:
				return ntsc_extend_12_char[ntsc_char - 0x20]|0x8000;
			default:
				return 0x20;
		}
	}

	if(type_char == 0x13){
		switch(ntsc_char){
			case 0x20 ... 0x3F:
				return ntsc_extend_13_char[ntsc_char - 0x20]|0x8000;
			default:
				return 0x20;
		}
	}

	if(type_char == 0xFF){
		switch(ntsc_char){
			case 0x2A:return 0xE1;
			case 0x5C:return 0xE9;
			case 0x5E:return 0xED;
			case 0x5F:return 0xF3;
			case 0x60:return 0xFA;
			case 0x7B:return 0xE7;
			case 0x7C:return 0xF7;
			case 0x7D:return 0xD1;
			case 0x7E:return 0xF1;
			case 0x7F:return 0x25AE;
			default:return 0xFFFF;
		}
	}

	return 0x20;
}
/*===============================================================
 *  Function Name:
 *  Function Description:
 *  Function Parameters:
 *  Function Return:
 *===============================================================*/
static void am_korea_show_caption()
{
	int idx = 0;

	memset(cc_cmds, 0, sizeof(cc_cmds));
	memset(korea_cc_data, 0, sizeof(korea_cc_data));
	cc_cmds[10] = 0;
	cc_cmds[16] = 0;//to store data only
	cc_cmds[0] = cc_cur_type;
	cc_cmds[30] = 0xFF92;
	cc_cmds[31] = temp_row++;
	cc_cmds[32] = 5;

	{
		int MAX_LINE = 4, cur_line_cnt = 0, idx = 0, temp_line_cnt = 0, tag_idx = 0;

		//to calculate how many line still in current buf
		for(idx = 0; idx < temp_cc1_char_idx;)
		{
			if(temp_cc1_char[idx] == MAGIC_NUM)
			{
				CC_DECODE_DBG("11 The row:%d, col:%d, color:0x%x\n", temp_cc1_char[idx+1], temp_cc1_char[idx+2], temp_cc1_char[idx+3]);
				idx+=MAGIC_NUM_CNT;
				cur_line_cnt++;
				continue;
			}
			idx++;
		}

		if(cur_line_cnt > MAX_LINE)
		{
			int delete_line_cnt = cur_line_cnt - MAX_LINE;
			int temp_data_array[512];
			temp_line_cnt = 0;
			for(idx = 0; idx < temp_cc1_char_idx;)
			{
				if(temp_cc1_char[idx] == MAGIC_NUM)
				{
					CC_DECODE_DBG("11 The row:%d, col:%d, color:0x%x\n", temp_cc1_char[idx+1], temp_cc1_char[idx+2], temp_cc1_char[idx+3]);
					if(temp_line_cnt == delete_line_cnt)
					{
						tag_idx = idx;
						break;
					}
					idx+=MAGIC_NUM_CNT;
					temp_line_cnt++;
					continue;
				}
				idx++;
			}

			for(idx = tag_idx; idx < temp_cc1_char_idx; idx++)
				temp_data_array[idx-tag_idx] = temp_cc1_char[idx];

			temp_cc1_char_idx = temp_cc1_char_idx - tag_idx;
			memset(temp_cc1_char, 0, 512);

			for(idx = 0; idx < temp_cc1_char_idx; idx++)
				temp_cc1_char[idx] = temp_data_array[idx];
		}
	}

#if 0
	//to remove the char before extend char
	{
		int i, temp_char[512], temp_data, total_char = 0;
		char temp_string[512];
		int str_idx = 0;
		CC_DECODE_DBG("before remove total char is:%d\n", temp_cc1_char_idx);
		for(i = 0; i < temp_cc1_char_idx; i++)
		{
			if(temp_cc1_char[i] == MAGIC_NUM)
			{
				CC_DECODE_DBG("11 The row:%d, col:%d, color:0x%x\n", temp_cc1_char[i+1], temp_cc1_char[i+2], temp_cc1_char[i+3]);
				i+=(MAGIC_NUM_CNT-1);
				continue;
			}
			//for extend char
			if(temp_cc1_char[i]&0x8000 && i > 0)
			{
				//CC_DECODE_DBG("The %d extend char:0x%x, before char:0x%x\n", i, temp_cc1_char[i], temp_cc1_char[i-1]);
				temp_cc1_char[i] = temp_cc1_char[i]&0x7FFF;
				temp_cc1_char[i-1] = 0;
			}
		}

		for(i = 0; i < temp_cc1_char_idx; i++)
		{
			if(temp_cc1_char[i] == MAGIC_NUM)
			{
				int idx = 0;
				//CC_DECODE_DBG("22 The magic num found and is:0x%x 0x%x\n", temp_cc1_char[i], temp_cc1_char[i+MAGIC_NUM_CNT-1]);
				for(idx = i; idx < i+MAGIC_NUM_CNT; idx++)
					temp_char[total_char++] = temp_cc1_char[idx];

				i+=(MAGIC_NUM_CNT-1);
				continue;
			}
			if(temp_cc1_char[i] != 0)
				temp_char[total_char++] = temp_cc1_char[i];
		}

		memset(temp_string, 0, 512);
		memset(temp_cc1_char, 0, 512);
		temp_cc1_char_idx = total_char;
		str_idx = 0;
		for(i = 0; i < total_char; i++)
		{
			temp_cc1_char[i] = temp_char[i];

			//for debug only
			if(temp_char[i]== MAGIC_NUM)
			{
				int idx = 0;
				//CC_DECODE_DBG("33 The magic num found and is:0x%x 0x%x\n", temp_char[i], temp_char[i+MAGIC_NUM_CNT-1]);
				if(str_idx > 0){
					CC_DECODE_DBG("11 Str is:%s\n", temp_string);
				}

				for(idx = i; idx < i+MAGIC_NUM_CNT; idx++)
					temp_cc1_char[idx] = temp_char[idx];

				i+=(MAGIC_NUM_CNT-1);
				str_idx = 0;
				memset(temp_string, 0, 512);
				continue;
			}
			temp_string[str_idx++] =(char) temp_char[i]&0xFF;
		}

		if(str_idx > 0){
			CC_DECODE_DBG("22 Str is:%s\n", temp_string);
		}

		CC_DECODE_DBG("after remove total char is:%d\n", temp_cc1_char_idx);
	}
#endif

	korea_cc_data_idx = temp_cc1_char_idx;
	for(idx = 0; idx < korea_cc_data_idx; idx++)
		korea_cc_data[idx] = temp_cc1_char[idx];

	am_us_atsc_callback();

	return;
}
/*==============================================================
 *  Function Name:
 *  Function Description:
 *  Function Parameters:
 *  Function Return:
 *===============================================================*/
static void am_ntsc_show_caption(int show_flag)
{
	int idx = 0;

	memset(cc_cmds, 0, sizeof(cc_cmds));
	memset(korea_cc_data, 0, sizeof(korea_cc_data));
	cc_cmds[10] = 0;
	cc_cmds[16] = 0;//to store data only
	cc_cmds[0] = cc_cur_type;
	cc_cmds[30] = 0xFF92;
	cc_cmds[31] = temp_row++;
	cc_cmds[32] = 5;

	{
		int MAX_LINE = 4, cur_line_cnt = 0, idx = 0, temp_line_cnt = 0, tag_idx = 0;

		//to calculate how many line still in current buf
		for(idx = 0; idx < temp_cc1_char_idx;)
		{
			if(temp_cc1_char[idx] == MAGIC_NUM)
			{
				CC_DECODE_DBG("11 The row:%d, col:%d, color:0x%x\n", temp_cc1_char[idx+1], temp_cc1_char[idx+2], temp_cc1_char[idx+3]);
				idx+=MAGIC_NUM_CNT;
				cur_line_cnt++;
				continue;
			}
			idx++;
		}

		if(cur_line_cnt > MAX_LINE)
		{
			int delete_line_cnt = cur_line_cnt - MAX_LINE;
			int temp_data_array[512];
			temp_line_cnt = 0;
			for(idx = 0; idx < temp_cc1_char_idx;)
			{
				if(temp_cc1_char[idx] == MAGIC_NUM)
				{
					CC_DECODE_DBG("11 The row:%d, col:%d, color:0x%x\n", temp_cc1_char[idx+1], temp_cc1_char[idx+2], temp_cc1_char[idx+3]);
					if(temp_line_cnt == delete_line_cnt)
					{
						tag_idx = idx;
						break;
					}
					idx+=MAGIC_NUM_CNT;
					temp_line_cnt++;
					continue;
				}
				idx++;
			}

			for(idx = tag_idx; idx < temp_cc1_char_idx; idx++)
				temp_data_array[idx-tag_idx] = temp_cc1_char[idx];

			temp_cc1_char_idx = temp_cc1_char_idx - tag_idx;
			memset(temp_cc1_char, 0, 512);

			for(idx = 0; idx < temp_cc1_char_idx; idx++)
				temp_cc1_char[idx] = temp_data_array[idx];
		}
	}

	//to remove the char before extend char
	{
		int i, temp_char[512], temp_data, total_char = 0;
		char temp_string[512];
		int str_idx = 0;
		
		CC_DECODE_DBG("before remove total char is:%d\n", temp_cc1_char_idx);
		for(i = 0; i < temp_cc1_char_idx; i++)
		{
			if(temp_cc1_char[i] == MAGIC_NUM)
			{
				CC_DECODE_DBG("11 The row:%d, col:%d, color:0x%x\n", temp_cc1_char[i+1], temp_cc1_char[i+2], temp_cc1_char[i+3]);
				i+=(MAGIC_NUM_CNT-1);
				continue;
			}
			//for extend char
			if(temp_cc1_char[i]&0x8000 && i > 0)
			{
				//CC_DECODE_DBG("The %d extend char:0x%x, before char:0x%x\n", i, temp_cc1_char[i], temp_cc1_char[i-1]);
				temp_cc1_char[i] = temp_cc1_char[i]&0x7FFF;
				temp_cc1_char[i-1] = 0;
			}
		}

		for(i = 0; i < temp_cc1_char_idx; i++)
		{
			if(temp_cc1_char[i] == MAGIC_NUM)
			{
				int idx = 0;
				//CC_DECODE_DBG("22 The magic num found and is:0x%x 0x%x\n", temp_cc1_char[i], temp_cc1_char[i+MAGIC_NUM_CNT-1]);
				for(idx = i; idx < i+MAGIC_NUM_CNT; idx++)
					temp_char[total_char++] = temp_cc1_char[idx];

				i+=(MAGIC_NUM_CNT-1);
				continue;
			}
			if(temp_cc1_char[i] != 0)
				temp_char[total_char++] = temp_cc1_char[i];
		}

		memset(temp_string, 0, 512);
		memset(temp_cc1_char, 0, 512);
		temp_cc1_char_idx = total_char;
		str_idx = 0;
		for(i = 0; i < total_char; i++)
		{
			temp_cc1_char[i] = temp_char[i];

			//for debug only
			if(temp_char[i]== MAGIC_NUM)
			{
				int idx = 0;
				//CC_DECODE_DBG("33 The magic num found and is:0x%x 0x%x\n", temp_char[i], temp_char[i+MAGIC_NUM_CNT-1]);
				if(str_idx > 0){
					CC_DECODE_DBG("\n%s----> Str is:%s\n",__FUNCTION__, temp_string);
				}

				for(idx = i; idx < i+MAGIC_NUM_CNT; idx++)
	   			  temp_cc1_char[idx] = temp_char[idx];

				i+=(MAGIC_NUM_CNT-1);
				str_idx = 0;
				memset(temp_string, 0, 512);
				continue;
			}
			temp_string[str_idx++] =(char) temp_char[i]&0xFF;
		}

		if(str_idx > 0){
			CC_DECODE_DBG("22 Str is:%s\n", temp_string);           
		}

		CC_DECODE_DBG("after remove total char is:%d\n", temp_cc1_char_idx);
	}

	korea_cc_data_idx = temp_cc1_char_idx;
	for(idx = 0; idx < korea_cc_data_idx; idx++)
	  korea_cc_data[idx] = temp_cc1_char[idx];

	am_us_atsc_callback();

	return;
}

/*===============================================================
 *  Function Name:
 *  Function Description:
 *  Function Parameters:
 *  Function Return:
 *===============================================================*/
static void am_parse_usa_ntsc_cc_data(int cc_data_1, int cc_data_2)
{
	int temp_data_1 = cc_data_1;
	int temp_data_2 = cc_data_2;
	cc_data_1 = cc_data_1&0x7F;
	cc_data_2 = cc_data_2&0x7F;

	CC_DECODE_DBG("@@@@@@@@ control code received: 0x%x 0x%x\n", cc_data_1, cc_data_2);

	if(cc_data_1 == 0 && cc_data_2 == 0)
	{
		return;
	}

	if(cc_data_1 < 0x20){         

		if (cc_data_1 >= 0x01 && cc_data_1 <= 0x0F ){
			/*
			 * XDS 
			 * If the BUF1 Code is > 01h and < 0Fh and we have not
			 * started capturing XDS , then this is an XDS Control
			 * code char so call XDS Decoder.
			 * Note that XDS Data only comes in Line21 Field 2
			 */
			CC_DECODE_DBG("xdsControlCode\n");
                        if(CC_NTSC_VBI_LINE21 == AM_Get_CC_SourceType())//only enable XDS for NTSC VBI		
			    AM_Decode_XDSData(cc_data_1,cc_data_2);
		} 

		if(!(cc_data_1 > 0xF && cc_data_1 < 0x18)){
			ntsc_cc1_flag = 0;
			memset(temp_cc1_char, 0, 512);
			temp_cc1_char_idx = 0;
			//memset(ntsc_cc_cmd, 0, 512);
			//ntsc_cc_cmd_idx = 0;
			ntsc_cc_show_style = 0;
			ntsc_cc_last_show_style = 0;
			CC_DECODE_DBG("not CH1 data so return 0x%x 0x%x\n", cc_data_1, cc_data_2);
			return;
		}else{
			ntsc_cc1_flag = 1;
		}
	}

	//CC_DECODE_DBG("----->>>>> temp_cc1_char_idx:%d, ntsc_cc1_flag:%d\n",temp_cc1_char_idx, ntsc_cc1_flag);
	if(ntsc_cc1_flag == 0)
		return;

	if(cc_data_1 > 0xF && cc_data_1 < 0x18)
	{
		if((cc_data_1 == 0x11 && (cc_data_2 > 0x2F && cc_data_2 < 0x40))||
				(cc_data_1 == 0x12 && (cc_data_2 > 0x1F && cc_data_2 < 0x40))||
				(cc_data_1 == 0x13 && (cc_data_2 > 0x1F && cc_data_2 < 0x40)))
		{
			CC_DECODE_DBG("The special or extend char:0x%x 0x%x\n", cc_data_1, cc_data_2);
			goto NTSC_CMD_END;
		}

		CC_DECODE_DBG("------11 cmd is:0x%x 0x%x the char idx is:%d, cmd count:%d\n", cc_data_1, cc_data_2,temp_cc1_char_idx, ntsc_cc_cmd_idx);

		//pop-on style
		if(cc_data_1 == 0x14 && cc_data_2 == 0x20)
		{
			CC_DECODE_DBG("######### haha pop on style be set ************************\n");
			if(temp_cc1_char_idx > MAGIC_NUM_CNT)
			{
				CC_DECODE_DBG("------->>>>> show caption because there are char with cmd 0x20 \n");
				am_ntsc_show_caption(1);

				memset(temp_cc1_char, 0, 512);
				temp_cc1_char_idx = 0;
				//memset(ntsc_cc_cmd, 0, 512);
				//ntsc_cc_cmd_idx = 0;
				ntsc_cc_show_style = 0;
				CC_DECODE_DBG("The char cnt is:%d, ntsc_cc_cmd_idx:%d\n", temp_cc1_char_idx, ntsc_cc_cmd_idx);
			}            
			ntsc_cc_show_style = CC_POP_ON;
			ntsc_cc_last_show_style = CC_POP_ON;
			return;
		}

		//roll-up style
		if((cc_data_1 == 0x14 && cc_data_2 == 0x25) ||
				(cc_data_1 == 0x14 && cc_data_2 == 0x26) ||
				(cc_data_1 == 0x14 && cc_data_2 == 0x27))
		{
			CC_DECODE_DBG("######### roll up style be set ^^^^^^^^^^^^^^^^^^^^\n");
			if(temp_cc1_char_idx > MAGIC_NUM_CNT && ntsc_cc_last_show_style != CC_ROLL_UP)
			{
				CC_DECODE_DBG("------->>>>> show caption because there are char with cmd roll up \n");
				am_ntsc_show_caption(1);

				memset(temp_cc1_char, 0, 512);
				temp_cc1_char_idx = 0;
				//memset(ntsc_cc_cmd, 0, 512);
				//ntsc_cc_cmd_idx = 0;
				ntsc_cc_show_style = 0;
				CC_DECODE_DBG("The char cnt is:%d, ntsc_cc_cmd_idx:%d\n", temp_cc1_char_idx, ntsc_cc_cmd_idx);
			}
			ntsc_cc_show_style = CC_ROLL_UP;
			ntsc_cc_last_show_style = CC_ROLL_UP;
			return;
		}

		//paint-on which should show the char received on OSD ASAP so may be do it like roll-up
		if(cc_data_1 == 0x14 && cc_data_2 == 0x29)
		{
			CC_DECODE_DBG("######### paint on style be set $$$$$$$$$$$$$$$$$$$$$$$$\n");
			if(temp_cc1_char_idx > MAGIC_NUM_CNT && ntsc_cc_last_show_style != CC_PAINT_ON)
			{
				CC_DECODE_DBG("------->>>>> show caption because there are char with cmd 0x29 \n");
				am_ntsc_show_caption(1);

				memset(temp_cc1_char, 0, 512);
				temp_cc1_char_idx = 0;
				//memset(ntsc_cc_cmd, 0, 512);
				//ntsc_cc_cmd_idx = 0;
				ntsc_cc_show_style = 0;
				CC_DECODE_DBG("The char cnt is:%d, ntsc_cc_cmd_idx:%d\n", temp_cc1_char_idx, ntsc_cc_cmd_idx);
			}
			ntsc_cc_show_style = CC_PAINT_ON;
			ntsc_cc_last_show_style = CC_PAINT_ON;
			return;
		}

		//to set the line format if preamble address code be received
		if((ntsc_cc_show_style == CC_ROLL_UP || 
		    ntsc_cc_show_style == CC_POP_ON ||
		    ntsc_cc_show_style == CC_PAINT_ON) && cc_data_2 > 0x3F)
		{
			int font_row = 0, font_col = 0, font_color = 0;
			int row_number[16]={11, 15, 1, 2, 3, 4, 12, 13,14, 15, 5, 6, 7, 8, 9, 10};
			int col_number[16]={0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 8, 12, 16, 20, 24, 28};
			int color_number[16]={0xFFF, 0x0F0, 0x00F, 0x0FF, 0xF00, 0xFF0, 0xF0F, 0xFFF, 
					      0xFFF, 0xFFF, 0xFFF, 0xFFF, 0xFFF, 0xFFF, 0xFFF, 0xFFF};
			int crc_sum = 0;
                        if((last_preamble_code1 == cc_data_1) && (cc_data_2 == last_preamble_code2))
                        {
                            CC_DECODE_DBG("duplicated preamble code received and nothing would be done");
                            last_preamble_code1 = 0;
                            last_preamble_code2 = 0;
                            return;
                        }
                        
                        last_preamble_code1 = cc_data_1;
                        last_preamble_code2 = cc_data_2;
                        
			font_row = row_number[(cc_data_1&0x7)<<1|(cc_data_2&0x20)>>5];
			font_col = col_number[(cc_data_2&0x10)>>1|(cc_data_2&0xE)>>1];
			font_color = color_number[(cc_data_2&0x10)>>1|(cc_data_2&0xE)>>1];
			temp_cc1_char[temp_cc1_char_idx++] = MAGIC_NUM;
			temp_cc1_char[temp_cc1_char_idx++] = font_row;
			temp_cc1_char[temp_cc1_char_idx++] = font_col;
			temp_cc1_char[temp_cc1_char_idx++] = font_color;
			temp_cc1_char[temp_cc1_char_idx++] = 0;
			temp_cc1_char[temp_cc1_char_idx++] = 0;
			temp_cc1_char[temp_cc1_char_idx++] = 0;
			temp_cc1_char[temp_cc1_char_idx++] = 0;
			temp_cc1_char[temp_cc1_char_idx++] = 0;
			crc_sum = MAGIC_NUM^font_row^font_col^font_color;
			temp_cc1_char[temp_cc1_char_idx++] = crc_sum;//CRC check sum?
			CC_DECODE_DBG("******* Preamble address code:row:0x%x, col:0x%x, color:0x%x, CRC:0x%x\n", font_row, font_col, font_color, crc_sum);
			//since we using preamble code to update the cc to UI, which should do every time you received char
			if(ntsc_cc_show_style != CC_PAINT_ON)
				return;
			CC_DECODE_DBG("going to update the CC in paint on mode\n");
		}

		//middle row code which will not be processed
		if((cc_data_1 == 0x11 || cc_data_1 == 0x19)&& (cc_data_2 > 0x1F && cc_data_2 < 0x30))
		{
			CC_DECODE_DBG("^^^^^^^^^ middle row code ignored and blank will be added ^^^^^^^^^^^^^^^\n");
			temp_cc1_char[temp_cc1_char_idx++] = 0x20;
			return;
		}

		switch(ntsc_cc_show_style)
		{
			case CC_POP_ON:
				if(cc_data_2 == 0x2F && temp_cc1_char_idx > MAGIC_NUM_CNT)
				{
					CC_DECODE_DBG("------->>>>> show caption in CC_POP_ON with cmd 0x2F \n");
					am_ntsc_show_caption(1);

					memset(temp_cc1_char, 0, 512);
					temp_cc1_char_idx = 0;
					//memset(ntsc_cc_cmd, 0, 512);
					//ntsc_cc_cmd_idx = 0;
					ntsc_cc_show_style = 0;
					CC_DECODE_DBG("The char cnt is:%d, ntsc_cc_cmd_idx:%d\n", temp_cc1_char_idx, ntsc_cc_cmd_idx);
				}
				return;

			case CC_ROLL_UP:
				if(cc_data_2 == 0x2D && temp_cc1_char_idx > MAGIC_NUM_CNT)
				{
					CC_DECODE_DBG("------->>>>> show caption in CC_ROLL_UP with cmd 0x2D \n");
					am_ntsc_show_caption(1);
					//ntsc_cc_show_style = 0;
					CC_DECODE_DBG("The char cnt is:%d, ntsc_cc_cmd_idx:%d\n", temp_cc1_char_idx, ntsc_cc_cmd_idx);
				}
				else  if(cc_data_2 == 0x2C && temp_cc1_char_idx > MAGIC_NUM_CNT)
				{
					CC_DECODE_DBG("------->>>>> show caption in CC_ROLL_UP with cmd 0x2C \n");
					am_ntsc_show_caption(1);

					memset(temp_cc1_char, 0, 512);
					temp_cc1_char_idx = 0;
					//memset(ntsc_cc_cmd, 0, 512);
					//ntsc_cc_cmd_idx = 0;
					ntsc_cc_show_style = 0;
					CC_DECODE_DBG("The char cnt is:%d, ntsc_cc_cmd_idx:%d\n", temp_cc1_char_idx, ntsc_cc_cmd_idx);
				}
				return;

			case CC_PAINT_ON:
				if((cc_data_2 == 0x2C) && temp_cc1_char_idx > MAGIC_NUM_CNT)
				{
					CC_DECODE_DBG("------->>>>> show caption in CC_PAINT_ON with cmd 0x2C \n");
					am_ntsc_show_caption(1);

					memset(temp_cc1_char, 0, 512);
					temp_cc1_char_idx = 0;
					//memset(ntsc_cc_cmd, 0, 512);
					//ntsc_cc_cmd_idx = 0;
					ntsc_cc_show_style = 0;
					CC_DECODE_DBG("The char cnt is:%d, ntsc_cc_cmd_idx:%d\n", temp_cc1_char_idx, ntsc_cc_cmd_idx);
				}
				else if(cc_data_2 > 0x3F && temp_cc1_char_idx > MAGIC_NUM_CNT)
				{
					CC_DECODE_DBG("------->>>>> show caption in CC_PAINT_ON with cmd preamble code \n");
					am_ntsc_show_caption(1);
					CC_DECODE_DBG("The char cnt is:%d, ntsc_cc_cmd_idx:%d\n", temp_cc1_char_idx, ntsc_cc_cmd_idx);
				}
				return;

			default:
				CC_DECODE_DBG("@@@@@@ not set the CC show style so return @@@@@@@\n");
				return;
		}
	}

NTSC_CMD_END:

	temp_cc1_char_idx%=512;

	if(cc_data_1 > 0x1F)
	{
		int temp_char = util_convert_ntsc_special_char_to_unicode(0xFF, cc_data_1);
		if(temp_char != 0xFFFF){
			temp_cc1_char[temp_cc1_char_idx++] = temp_char;
		}else{
			temp_cc1_char[temp_cc1_char_idx++] = cc_data_1;
		}
		CC_DECODE_DBG("11 The char:0x%02x is:%c temp_char:0x%x\n",temp_data_1, cc_data_1, temp_char);
	}

	if(cc_data_2 > 0x1F && cc_data_1 > 0x1F)
	{
		int temp_char = util_convert_ntsc_special_char_to_unicode(0xFF, cc_data_2);
		if(temp_char != 0xFFFF){
			temp_cc1_char[temp_cc1_char_idx++] = temp_char;
		}else{
			temp_cc1_char[temp_cc1_char_idx++] = cc_data_2;
		}
		CC_DECODE_DBG("22 The char:0x%02x is:%c temp_char:0x%x\n",temp_data_2, cc_data_2, temp_char);
	}

	//the special char which we should convert to unicode and send to UI
	if(cc_data_1 == 0x11 || cc_data_1 == 0x12 || cc_data_1 == 0x13){
		int convert_char = util_convert_ntsc_special_char_to_unicode(cc_data_1, cc_data_2);
		temp_cc1_char[temp_cc1_char_idx++] = convert_char;
		CC_DECODE_DBG("=====ntsc special char1:0x%x, char2:0x%x, convert_char:0x%x\n", temp_data_1, temp_data_2, convert_char);       
	}
}

/**===============================================================
 *  Function Name:
 *  Function Description:
 *  Function Parameters:
 *  Function Return:
 *===============================================================*/
static int am_convert_korea_font_color(int color_idx)
{
	int ret_color = 0;
	if(color_idx&0x30){
		ret_color|=0xF00;
	}

	if(color_idx&0xC){
		ret_color|=0xF0;
	}

	if(color_idx&0x3){
		ret_color|=0xF;
	}

	CC_DECODE_DBG("The raw color idx is:0x%x, the ret color is:0x%x.\n", color_idx, ret_color);
	return ret_color;
}
/**===============================================================
 *  Function Name:
 *  Function Description:
 *  Function Parameters:

 *  Function Return:
 *===============================================================*/
static void am_parse_korea_cc_command(char *buffer, int cnt)
{
	int idx = 0;
	char temp_data;
	char temp_buf[CC_CNT];
	char temp_buf_char[8];
	int SPL_CMD_RECEIVED = 0;
	int char_idx = 0;
	char temp_buf_korea[CC_CNT];
	int char_idx_korea = 0;
	int font_color = 0xFFF;
	int anchor_v = 0, anchor_h = 0;

	memset(temp_buf, 0, CC_CNT);
	memset(temp_buf_korea, 0, CC_CNT);
	//temp_cc1_char_idx = 0;
	//memset(temp_cc1_char, 0, CC_CNT);
	memset(cc_cmds, 0, sizeof(cc_cmds));

	CC_DECODE_DBG("==================== to call korea atsc cc parse =====================\n");
	am_print_data(cnt, buffer);
	while(idx < cnt){
		temp_data = buffer[idx];
		CC_DECODE_DBG("ATSC The idx is:%d, the temp_data is:0x%02x.\n", idx, temp_data);

		//this should be processed both for extended char and code set range from 0x00~0x1F and 0x80~0x9F
		if(temp_data == 0x10){
			int special_char = 0;
			CC_DECODE_DBG("!!!!!!!!!!!!!!!!!!!!!The ext1 code 0x10 be founded 0x%x. \n", buffer[idx+1]);
			special_char = 0x20;//am_atsc_c_ext_code_2_unicode(buffer[idx+1]);
			//korea_cc_data[korea_cc_data_idx] = special_char;
			//korea_cc_data_idx++;
			temp_cc1_char[temp_cc1_char_idx++] = special_char;
			CC_DECODE_DBG("ext1*********** the data is:0x%x.\n", special_char);
			idx+=2;
			break;
		}

		if(temp_data == 0x18){
			//idx+=3;
			CC_DECODE_DBG("************the korea data:0x%x, 0x%x\n",buffer[idx+1], buffer[idx+2]);
			//continue;
		}

		if(temp_data == 0x3){
			idx+=2;
			CC_DECODE_DBG("^^^^^^^^^^^^^^ The ext1 code 0x3 be founded and break.\n");
			break;
		}

		switch(temp_data){
			//single byte synmatic element
			case 0x00 ... 0x0F:
				CC_DECODE_DBG("------->>>>>>>>    0x00 ... 0x0F      <<<<<<<<<<<<<<<<<\n");
				idx++;
				break;

				///2 bytes
			case 0x11 ... 0x17:
				CC_DECODE_DBG("------->>>>>>>>    0x11 ... 0x17      <<<<<<<<<<<<<<<<<\n");
				idx+=2;
				break;

				///3 bytes
			case 0x18:
				if(SPL_CMD_RECEIVED > 0)
				{
					int temp_unicode = ksc5601_to_unicode(buffer[idx+1], buffer[idx+2]);
					//korea_cc_data[korea_cc_data_idx] = temp_unicode;
					//korea_cc_data_idx++;
					temp_cc1_char[temp_cc1_char_idx++] = temp_unicode;
					CC_DECODE_DBG("The korea char unicode is:0x%x.\n", temp_unicode);
				}
				idx+=3;
				break;

				///3 bytes
			case 0x19 ... 0x1F:
				CC_DECODE_DBG("------->>>>>>>>    0x19 ... 0x1F      <<<<<<<<<<<<<<<<<\n");
				idx+=3;
				break;

				//CW0~CW7, 0x93~0x96 reserved, single byte
			case 0x80 ... 0x87:
			case 0x93 ... 0x96:                
				CC_DECODE_DBG("CW The cmd is:0x%x.\n", temp_data);
				idx++;
				break;

				//DF0~DF7 7 bytes
			case 0x98 ... 0x9F:
				CC_DECODE_DBG("DF The cmd is:0x%x\n", temp_data);
				anchor_v = buffer[idx+2]&0x7F;
				anchor_v%=75;
				anchor_h = buffer[idx+3];
				anchor_v%=210; //assume it's 16:9 system
				CC_DECODE_DBG("cmd DF h/v:%d/%d", anchor_h, anchor_v);
				idx+=7;
				break;

				//CLW 2 bytes
			case 0x88:
				CC_DECODE_DBG("CLW The cmd is:0x%x\n", temp_data);
				idx+=2;
				break;

				//DLW 2 bytes
			case 0x8C:
				CC_DECODE_DBG("----->>>>>The DLW command received:0x%x\n", temp_data);
				CC_DECODE_DBG("DLW The cmd is:0x%x.", temp_data);
				idx+=2;
				break;

				//DSW 2 bytes
			case 0x89:
				CC_DECODE_DBG("----->>>>>The DSW command received:0x%x\n", temp_data);
				CC_DECODE_DBG("DSW The cmd is:0x%x.", temp_data);
				idx+=2;
				break;

				//HDW 2 bytes
			case 0x8A:
				CC_DECODE_DBG("----->>>>>The HDW command received:0x%x\n", temp_data);
				CC_DECODE_DBG("HDW The cmd is:0x%x.", temp_data);
				idx+=2;
				break;

				//TGW 2 bytes
			case 0x8B:
				CC_DECODE_DBG("----->>>>>The TGW command received:0x%x\n", temp_data);
				CC_DECODE_DBG("TGW The cmd is:0x%x.", temp_data);
				idx+=2;
				break;

				//SWA 5 bytes
			case 0x97:
				CC_DECODE_DBG("----->>>>>The SWA command received:0x%x\n", temp_data);
				CC_DECODE_DBG("SWA The cmd is:0x%x.", temp_data);
				idx+=5;
				break;

				//SPA 3 bytes
			case 0x90:
				CC_DECODE_DBG("----->>>>>The SPL command received:0x%x\n", temp_data);
				CC_DECODE_DBG("SPA The cmd is:0x%x.", temp_data);
				idx+=3;
				break;

				//SPC 4 bytes
			case 0x91:
				CC_DECODE_DBG("----->>>>>The SPC command received:0x%x\n", temp_data);
				CC_DECODE_DBG("SPC The cmd is:0x%x.", temp_data);
				idx+=2;//why USA ATSC is 4 while Korea is 2?
				break;

				//SPL
			case 0x92:
				CC_DECODE_DBG("SPL The cmd is:0x%x\n", temp_data);
				CC_DECODE_DBG("--------row:%d, column:%d, last_text_row:%d",buffer[idx+1], buffer[idx+2], last_text_row);
				{
					int font_row, font_col, crc_sum;
					font_row = buffer[idx+1]&0xF;
					font_row%=16;
					font_col = buffer[idx+2]&0x3F;
					font_col%=42;
					crc_sum = MAGIC_NUM^font_row^font_col^font_color;
					CC_DECODE_DBG("----->>>>>The SPL command received:0x%x, Row/Col:%d/%d\n", temp_data, 
                                                      font_row, font_col);
					temp_cc1_char[temp_cc1_char_idx++] = MAGIC_NUM;
					temp_cc1_char[temp_cc1_char_idx++] = font_row+(15*anchor_v/75);
					temp_cc1_char[temp_cc1_char_idx++] = font_col;
					temp_cc1_char[temp_cc1_char_idx++] = am_convert_korea_font_color(font_color); //color
					temp_cc1_char[temp_cc1_char_idx++] = anchor_h*1800/210;
					temp_cc1_char[temp_cc1_char_idx++] = anchor_v*900/75;
					temp_cc1_char[temp_cc1_char_idx++] = 0;
					temp_cc1_char[temp_cc1_char_idx++] = 0;
					temp_cc1_char[temp_cc1_char_idx++] = 0;
					temp_cc1_char[temp_cc1_char_idx++] = crc_sum;//CRC check sum?
					SPL_CMD_RECEIVED = 1;
				}
				idx+=3;
				break;

				//DLY 2 bytes
			case 0x8D:
				CC_DECODE_DBG("----->>>>>The DLY command received:0x%x\n", temp_data);
				CC_DECODE_DBG("DLY The cmd is:0x%x.", temp_data);
				idx+=2;
				break;

				//DLC  1 byte
			case 0x8E:
				CC_DECODE_DBG("----->>>>>The DLC command received:0x%x\n", temp_data);
				CC_DECODE_DBG("DLC The cmd is:0x%x.", temp_data);
				idx+=1;
				break;

				//RST 1 byte
			case 0x8F:
				CC_DECODE_DBG("----->>>>>The RST command received:0x%x\n", temp_data);
				CC_DECODE_DBG("RST The cmd is:0x%x.", temp_data);
				idx+=1;
				break;

				//this will not happen since, you konw, it's korea CC
			case 0x20 ... 0x7E:
			case 0xA0 ... 0xFF:
				idx+=1;
				break;

			case 0x7F:
				idx+=1;
				break;

			default:
				idx+=1;
				CC_DECODE_DBG("The default value is:0x%02x\n", temp_data);
				break;
		}
	}

PARSE_END:
	CC_DECODE_DBG("----------------- atsc cc parse end ----------------------\n");
	return;
}

/**===============================================================
 *  Function Name:
 *  Function Description: this char would be shown correctly if 
 *                 the ttf fonts lib fully supports these unicode
 *  Function Parameters:
 *  Function Return:
 *===============================================================*/
static int am_atsc_c_ext_code_2_unicode(int cc_extend_code)
{
	switch(cc_extend_code)
	{
		//TSP
		case 0x20:return 0x2002;//en space
		//NBTSP
		case 0x21:return 0x2002;
		//...
		case 0x25:return 0x22EF;
		//S
		case 0x2A:return 0x160;
		//s
		case 0x3A:return 0x161;
		//OE
		case 0x2C:return 0x152;
		//oe
		case 0x3C:return 0x153;
		//solid block which fills the entire character position with the text foreground color
		case 0x30:return 0x25AE;
		//'
		case 0x31:return 0x2018;
		//'
		case 0x32:return 0x2019;
		//"
		case 0x33:return 0x201C;
		//"
		case 0x34:return 0x201D;
		//.
		case 0x35:return 0x2022;
		//TM symbol
		case 0x39:return 0x2122;
		//sm symbol    
		case 0x3D:return 0x2120;
		//Y plus ..
		case 0x3F:return 0x178;
		/// 1/8
		case 0x76:return 0x215B;
		// 3/8
		case 0x77:return 0x215C;
		// 5/8
		case 0x78:return 0x215D;
		// 7/8
		case 0x79:return 0x215E;
		// |
		case 0x7A:return 0x23A2;
		// 7
		case 0x7B:return 0x23A4;
		// L
		case 0x7C:return 0x23A3;
		//-
		case 0x7D:return 0x2014;//0x23E4;
		// J
		case 0x7E:return 0x23A6;
		// don't know F without the second line
		case 0x7F:return 0x23A1;
		//cc symbol
		case 0xA0:return 0xFFFD;//? icon  //0x263A;laugh
		default:
		//CC_DECODE_DBG("The special char is:0x%x.\n", cc_extend_code);
	                 return cc_extend_code;
	}
}

/**===============================================================
 *  Function Name:
 *  Function Description:
 *  Function Parameters:
 *  Function Return:
 *===============================================================*/
void am_parse_atsc_cc_command(char *buffer, int cnt)
{
	int idx = 0;
	char temp_data;
	char temp_buf[CC_CNT];
	char temp_buf_char[8];
	int char_idx = 0;
	char temp_buf_korea[CC_CNT];
	int char_idx_korea = 0;
	int anchor_v = 0, anchor_h = 0;

	memset(temp_buf, 0, CC_CNT);
	memset(temp_buf_korea, 0, CC_CNT);

	CC_DECODE_DBG("==================== to call atsc cc parse =====================\n");
	am_print_data(cnt, buffer);
	while(idx < cnt){
		temp_data = buffer[idx];
		CC_DECODE_DBG("ATSC The idx is:%d, the temp_data is:0x%02x 0x%02x 0x%02x\n", idx, buffer[idx], buffer[idx+1], buffer[idx+2]);

		//this should be processed both for extended char and code set range from 0x00~0x1F and 0x80~0x9F
		if(temp_data == 0x10){
			int special_char = 0;
			CC_DECODE_DBG("!!!!!!!!!!!!!!!!!!!!!The ext1 code 0x10 be founded 0x%x. \n", buffer[idx+1]);
			special_char = am_atsc_c_ext_code_2_unicode(buffer[idx+1]);
			//korea_cc_data[korea_cc_data_idx] = special_char;
			//korea_cc_data_idx++;
			temp_cc1_char[temp_cc1_char_idx++] = special_char|0x8000;
			CC_DECODE_DBG("0x10 ###### special data is:0x%02x.\n", temp_data);
			idx+=2;
			continue;
		}

		if(temp_data == 0x18){
			//idx+=3;
			CC_DECODE_DBG("************the korea data:0x%x, 0x%x\n",buffer[idx+1], buffer[idx+2]);
			//continue;
		}

		if(temp_data == 0x3){
			idx+=2;
			CC_DECODE_DBG("^^^^^^^^^^^^^^ The ext1 code 0x3 be founded and break.\n");
			break;
		}

		if(temp_cc1_char_idx > MAGIC_NUM_CNT && temp_data == 0xD)
		{
			CC_DECODE_DBG("CR The cmd is:0x%x.", temp_data);
			am_ntsc_show_caption(1);
			//temp_cc1_char_idx = 0;
			//memset(temp_cc1_char, 0, sizeof(temp_cc1_char));
			return;
		}


		//to reset if no more char in the buffer
		if(temp_data == 0xD)
		{
			CC_DECODE_DBG("........ to rest the buffer since we received CR command ......\n");
			temp_cc1_char_idx = 0;
			memset(temp_cc1_char, 0, sizeof(temp_cc1_char));
			return;
		}
		if(temp_data > 0x9F){
			//idx+=3;
			//CC_DECODE_DBG(">>>>>>>> the data 0x%02x is larger than 0x9F.\n", temp_data);
			//continue;
		}

		if(temp_data == 0x8 && buffer[idx+1] > 0x1F){
			CC_DECODE_DBG("the BS code be found and this is a special char:0x%x\n", buffer[idx+1]);
			temp_cc1_char[temp_cc1_char_idx++] = buffer[idx+1]|0x8000;
			idx+=2;
			continue;
		}

		switch(temp_data){
			//single byte synmatic element
			case 0x00 ... 0x0F:
				idx++;
				break;

			///2 bytes
			case 0x11 ... 0x17:
				idx+=2;
				break;

			///3 bytes
			case 0x18:
			//korea_cc_data[korea_cc_data_idx] = ksc5601_to_unicode(buffer[idx+1], buffer[idx+2]);
			//korea_cc_data_idx++;
				CC_DECODE_DBG("0x18 ###### special data is:0x%02x.\n", temp_data);
				temp_cc1_char[temp_cc1_char_idx++] = ksc5601_to_unicode(buffer[idx+1], buffer[idx+2]);;
				idx+=3;
				break;

			///3 bytes
			case 0x19 ... 0x1F:
				idx+=3;
				break;

			//CW0~CW7, 0x93~0x96 reserved, single byte
			case 0x80 ... 0x87:
			case 0x93 ... 0x96:
				CC_DECODE_DBG("CW The cmd is:0x%x.", temp_data);
				idx++;
				break;

			//DF0~DF7 7 bytes
			case 0x98 ... 0x9F:
				anchor_v = buffer[idx+2]&0x7F;
				anchor_v%=75;
				anchor_h = buffer[idx+3];
				anchor_v%=210; //assume it's 16:9 system
				CC_DECODE_DBG("----->>>>>The DF command received:0x%x anchor V/H:%d/%d\n", temp_data, anchor_v, anchor_h);                
				idx+=7;
				break;

			//CLW 2 bytes
			case 0x88:
				CC_DECODE_DBG("CLW The cmd is:0x%x.", temp_data);
				idx+=2;
				break;

			//DLW 2 bytes
			case 0x8C:
				CC_DECODE_DBG("DLW The cmd is:0x%x.", temp_data);
				am_ntsc_show_caption(1);
				temp_cc1_char_idx = 0; 
				memset(temp_cc1_char, 0, sizeof(temp_cc1_char));
				idx+=2;
				break;

			//DSW 2 bytes
			case 0x89:
				CC_DECODE_DBG("DSW The cmd is:0x%x.", temp_data);
				idx+=2;
				break;

			//HDW 2 bytes
			case 0x8A:
				CC_DECODE_DBG("HDW The cmd is:0x%x.", temp_data);
				idx+=2;
				break;

			//TGW 2 bytes
			case 0x8B:
				CC_DECODE_DBG("----->>>>>The TGW command received:0x%x.\n", temp_data);
				am_ntsc_show_caption(1);
				temp_cc1_char_idx = 0; 
				memset(temp_cc1_char, 0, sizeof(temp_cc1_char));
				idx+=2;
				break;

			//SWA 5 bytes
			case 0x97:
				CC_DECODE_DBG("SWA The cmd is:0x%x.", temp_data);
				idx+=5;
				break;

			//SPA 3 bytes
			case 0x90:
				CC_DECODE_DBG("SPA The cmd is:0x%x.", temp_data);
				idx+=3;
				break;

			//SPC 4 bytes
			case 0x91:
				CC_DECODE_DBG("SPC The cmd is:0x%x.", temp_data);
				//we add a space here only
				temp_cc1_char[temp_cc1_char_idx++] = 0x20;
				idx+=5;//why is not 4 which is the standard specified
				break;

			//SPL
			case 0x92:
				CC_DECODE_DBG( "--------row:%d, column:%d, last_text_row:%d",buffer[idx+1], buffer[idx+2], last_text_row);
				{
					int font_row, font_col, font_color, crc_sum;
					font_row = buffer[idx+1]&0xF;
					font_row%=16;
					font_col = buffer[idx+2]&0x3F;
					font_col%=42;
					crc_sum = MAGIC_NUM^font_row^font_col^font_color;
					CC_DECODE_DBG("----->>>>>The SPL command received:0x%x, Row/Col:%d/%d\n", temp_data, font_row, font_col);
					temp_cc1_char[temp_cc1_char_idx++] = MAGIC_NUM;
					temp_cc1_char[temp_cc1_char_idx++] = font_row+(15*anchor_v/75);
					temp_cc1_char[temp_cc1_char_idx++] = font_col;
					temp_cc1_char[temp_cc1_char_idx++] = 0xF0; //color
					temp_cc1_char[temp_cc1_char_idx++] = anchor_h*1800/210;
					temp_cc1_char[temp_cc1_char_idx++] = anchor_v*900/75;
					temp_cc1_char[temp_cc1_char_idx++] = 0;
					temp_cc1_char[temp_cc1_char_idx++] = 0;
					temp_cc1_char[temp_cc1_char_idx++] = 0;
					temp_cc1_char[temp_cc1_char_idx++] = crc_sum;//CRC check sum?
				}
				idx+=3;
				break;

			//DLY 2 bytes
			case 0x8D:
				CC_DECODE_DBG("----->>>>>The DLY command received:0x%x.\n", temp_data);              
				idx+=2;
				break;

			//DLC  1 byte
			case 0x8E:
				CC_DECODE_DBG("----->>>>>The DLC command received:0x%x.\n", temp_data);
				idx+=1;
				break;

			//RST 1 byte
			case 0x8F:
				CC_DECODE_DBG("----->>>>>The RST command received:0x%x.\n", temp_data);
				idx+=1;
				break;

			case 0x20 ... 0x7E:
				{
					idx+=1;
					temp_buf[char_idx] = temp_data;
					//korea_cc_data[korea_cc_data_idx] = temp_data;
					//korea_cc_data_idx++;

					temp_cc1_char[temp_cc1_char_idx++] = temp_data;
					CC_DECODE_DBG("###### the data is:0x%02x %c\n", temp_data, temp_data);
					char_idx++;
				}
				break;
			case 0xA0 ... 0xFF:
				{
					idx+=1;
					temp_buf[char_idx] = temp_data;
					//korea_cc_data[korea_cc_data_idx] = temp_data;
					//korea_cc_data_idx++;

					temp_cc1_char[temp_cc1_char_idx++] = temp_data;
					CC_DECODE_DBG("###### the data is:0x%02x.\n", temp_data);
					char_idx++;
				}
				break;

			case 0x7F:
				{
					idx+=1;
					//korea_cc_data[korea_cc_data_idx] = 0x266A;
					//korea_cc_data_idx++;
					temp_cc1_char[temp_cc1_char_idx++] = 0x266A;
					CC_DECODE_DBG("###### the data is:0x%02x.\n", temp_data);
					char_idx++;
				}
				break;

			default:
				idx+=1;
				CC_DECODE_DBG("The default value is:0x%02x.\n", temp_data);
				break;
		}
	}

PARSE_END:
	CC_DECODE_DBG("----------------- atsc cc parse end ----------------------\n");
	return;
}

/*===============================================================
 *  Function Name:
 *  Function Description:
 *  Function Parameters:
 *  Function Return:
 *===============================================================*/
void am_parse_korea_cc_data_handle(int data1, int data2)
{
	int idx = 0;
	if((data1 == 0xd || data2 == 0xd) && (cc_char_idx > 0)){
		if(data2 == 0xd)
			cc_chars[cc_char_idx++] = data1;
		am_parse_korea_cc_command(cc_chars, cc_char_idx);

		CC_DECODE_DBG("The korea_cc_data_idx is:%d.", korea_cc_data_idx);
		//for(idx = 0; idx < korea_cc_data_idx; idx++)
		//CC_DECODE_DBG("parse korea cc data %d is:0x%x, %d.", idx, korea_cc_data[idx], korea_cc_data[idx]);
		am_print_int_data(temp_cc1_char_idx, temp_cc1_char);

		am_korea_show_caption();

		//temp_cc1_char_idx = 0;
		//memset(temp_cc1_char, 0, CC_CNT);
		cc_char_idx = 0;
		memset(cc_chars, 0, CC_CNT);

	}else{
		cc_chars[cc_char_idx++] = data1;
		cc_chars[cc_char_idx++] = data2;
	}
}

/**===============================================================
 *  Function Name:
 *  Function Description:
 *  Function Parameters:
 *  Function Return:
 *===============================================================*/
void am_parse_atsc_cc_data_handle(int data1, int data2)
{
	int idx;
	if(data1 == 0x8b){
		if(cc_char_idx > 0){
			am_parse_atsc_cc_command(cc_chars, cc_char_idx);
			korea_cc_data[0] = 0xFFFE;
			korea_cc_data_idx = 1;

			CC_DECODE_DBG("The atsc cc data idx is:%d.", korea_cc_data_idx);
			CC_DECODE_DBG("The atsc string is:%s.", cc_chars);
			for(idx = 0; idx < cc_char_idx; idx++)
				CC_DECODE_DBG("parse atsc cc data %d is:0x%x", idx, cc_chars[idx]);

			if(atsc_cc_notify != NULL)
				atsc_cc_notify(cc_chars, cc_char_idx, korea_cc_data, cc_cmds, atsc_cc_user_data);
			//memset(cc_chars, 0, CC_CNT);
			cc_char_idx = 0;
		}else{
			memset(cc_chars, 0, CC_CNT);
			cc_char_idx = 0;
		}
	}else if(data2 == 0x8b){
		cc_chars[cc_char_idx++] = data1;
		if(cc_char_idx > 0){
			am_parse_atsc_cc_command(cc_chars, cc_char_idx);
			memset(cc_chars, 0, CC_CNT);
			cc_char_idx = 0;
		}else{
			memset(cc_chars, 0, CC_CNT);
			cc_char_idx = 0;
		}
	}else{
		cc_chars[cc_char_idx++] = data1;
		cc_chars[cc_char_idx++] = data2;
	}
}

/*===============================================================
 *  Function Name:
 *  Function Description:
 *  Function Parameters:
 *  Function Return:
 *==============================================================*/
void am_init_dtvcc_varible(int flag)
{
	memset(&am_dtvcc_handle, 0, sizeof(am_dtvcc_handle));
	CC_DECODE_DBG("@@@@@@@@ set the sequence number to -1 @@@@@@@@\n");
	am_dtvcc_handle.next_sequence_number = -1;
	return;
}
/**===============================================================
 *  Function Name:
 *  Function Description:
 *  Function Parameters:
 *  Function Return:
 *===============================================================*/
void am_process_dtvcc(struct am_dtvcc_decoder *am_cc_decoder)
{
	unsigned int packet_size_code;
	unsigned int packet_size, i;
	unsigned int sequence_number;

	if(NULL == am_cc_decoder){
		CC_DECODE_DBG("NULL pointer %s %d found.\n", __FUNCTION__, __LINE__);
		return;
	}

	CC_DECODE_DBG("\n\n^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");
	am_print_data(am_cc_decoder->packet_size, am_cc_decoder->packet);
	sequence_number = (am_cc_decoder->packet[0]&0xC0)>>6;
	packet_size_code = am_cc_decoder->packet[0]&0x3F;
	CC_DECODE_DBG("11 next_sequence_number:0x%x.\n", am_cc_decoder->next_sequence_number);
	CC_DECODE_DBG("sequence_number:0x%x, packet_size_code is:%d, total:%d.\n", sequence_number, packet_size_code, am_cc_decoder->packet_size);

	if (am_cc_decoder->next_sequence_number >= 0 && 0 != ((am_cc_decoder->packet[0] ^ am_cc_decoder->next_sequence_number) & 0xC0)) 
	{
		CC_DECODE_DBG("discard the data.........\n");
		am_init_dtvcc_varible(0);
		return;
	}

	//am_cc_decoder->next_sequence_number = sequence_number;
	am_cc_decoder->next_sequence_number = am_cc_decoder->packet[0] + 0x40;
	CC_DECODE_DBG("22 next_sequence_number:0x%x.\n", am_cc_decoder->next_sequence_number);

	packet_size = MAX_CC;
	if (packet_size_code > 0)
		packet_size = packet_size_code*2;
	if(packet_size > am_cc_decoder->packet_size){
		CC_DECODE_DBG("The size not match so return.\n");
		//am_init_dtvcc_varible(0);
		//return;
		packet_size = am_cc_decoder->packet_size;
	}
	/* Service Layer. */

	/* CEA 708-C Section 6.2.5, 6.3: Service Blocks and syntactic
	   elements must not cross Caption Channel Packet
	   boundaries. */

	for (i = 1; i < packet_size;) {
		unsigned int service_number = 0;
		unsigned int block_size = 0;
		unsigned int header_size = 0;
		unsigned int c = 0;

		header_size = 1;

		/* service_number [3], block_size [5],
		   (null_fill [2], extended_service_number [6]),
		   (Block_data [n * 8]) */

		c = am_cc_decoder ->packet[i];
		service_number = (c & 0xE0) >> 5;
		CC_DECODE_DBG("===========c is:0x%x, The service number is:%d.\n", c, service_number);
		/* CEA 708-C Section 6.3: Ignore block_size if
		   service_number is zero. */
		if (0 == service_number) {
			/* NULL Service Block Header, no more data in this Caption Channel Packet. */
			break;
		}

		/* CEA 708-C Section 6.2.1: Apparently block_size zero
		   is valid, although properly it should only occur in
		   NULL Service Block Headers. */
		block_size = c & 0x1F;
		CC_DECODE_DBG("--------------The block_size is:%d.\n", block_size);
		if (7 == service_number) {
			if (i + 1 > packet_size){
				am_init_dtvcc_varible(0);
				return;
			}

			header_size = 2;
			c = am_cc_decoder->packet[i + 1];

			/* We also check the null_fill bits. */
			if (c < 7 || c > 63){
				am_init_dtvcc_varible(0);
				return;
			}
			service_number = c;
		}

		if (i + header_size + block_size > packet_size){
			am_init_dtvcc_varible(0);
			return;
		}

		if (service_number == 1) {
			struct am_dtvcc_service *ds;
			unsigned int in;
			ds = &am_cc_decoder->service[service_number - 1];
			in = ds->service_data_in;
			memcpy (ds->service_data + in, am_cc_decoder->packet + i + header_size, block_size);
			ds->service_data_in = in + block_size;
			//if(!(block_size == 2)) //we need to find why 0x78 is inserted to the CC
			am_parse_atsc_cc_command(ds->service_data, block_size);
		}

		i += header_size + block_size;
	}

	am_init_dtvcc_varible(0);

}

/**===============================================================
 *  Function Name:
 *  Function Description:
 *  Function Parameters:
 *  Function Return:
 *===============================================================*/
void AM_Parse_ATSC_CC_Data(char *buffer, int frequency)
{
	int process_cc_data_flag, additional_data_flag, cc_count;
	int idx = 0, unicode_data = 0;
	int cc_valid, cc_type, cc_data_1, cc_data_2, dtvcc_cnt = 0;

	if(NULL == buffer){
		CC_DECODE_DBG("%s %d buffer null returned.\n", __FUNCTION__, __LINE__);
		return;
	}

	if(!((cc_country > CMD_SET_COUNTRY_BEGIN && cc_country < CMD_SET_COUNTRY_END)&&
				(cc_cur_type > CMD_CC_BEGIN && cc_cur_type < CMD_CC_END)))
	{
		CC_DECODE_DBG("The count is:0x%x, the cc type is:0x%x, which are wrong",  cc_country, cc_cur_type);          
		return;
	}
	if(buffer[1] == 0 && buffer[2] == 0)
		return;
	process_cc_data_flag = buffer[0]&0x40;
	additional_data_flag = buffer[0]&0x20;
	cc_count = buffer[0]&0x1F;

	//CC_DECODE_DBG("process flag:0x%x, additonal flag:0x%x, cc count:0x%x.\n", process_cc_data_flag, additional_data_flag, cc_count);
	buffer+=2;
	for(idx = 0; idx < cc_count; idx++){
		cc_valid = (buffer[0]&0x4)>>2;
		cc_type = buffer[0]&0x3;
		cc_data_1 = buffer[1];
		cc_data_2 = buffer[2];
		//if((cc_valid > 0) &&(cc_data_1 > 0 || cc_data_2 > 0) && cc_type == 0 && process_cc_data_flag > 0){
		if((cc_data_1 > 0 || cc_data_2 > 0))
		{
#if 0
			if(cc_type ==2)
			{
				if(cc_korea_us == 0){
					cc_guess_idx+=2;
					if(cc_data_1 == 0x18 || cc_data_2 == 0x18){
						CC_DECODE_DBG("this is the korea CC........................................\n");
						cc_korea_us = 0xFFFF;
					}

					if(cc_korea_us == 0 && cc_guess_idx > 128){
						CC_DECODE_DBG("this is the USA CC........................................\n");
						cc_korea_us = 0xFFFE;
					}
					continue;
				}else if(cc_korea_us == 0xFFFF){
					am_parse_korea_cc_data_handle(cc_data_1, cc_data_2);
				}else if(cc_korea_us == 0xFFFE){
					am_parse_atsc_cc_data_handle(cc_data_1, cc_data_2);
				}
			}
			else
			{

			}
#else         
			if(cc_type == 0 && (cc_cur_type >= CMD_CC_1 && cc_cur_type <= CMD_TT_4)&& cc_country == CMD_SET_COUNTRY_USA)
			{
				am_parse_usa_ntsc_cc_data(cc_data_1, cc_data_2);
			}
			else if(cc_type > 1 && (cc_cur_type >= CMD_SERVICE_1 && cc_cur_type <= CMD_SERVICE_6))
			{
				CC_DECODE_DBG("=====idx:%d count:%d, valid:%d type:%d data1:0x%02x data2:0x%02x\n",idx, cc_count,
					       cc_valid, cc_type, cc_data_1, cc_data_2);
				if(cc_country == CMD_SET_COUNTRY_KOREA){
					if(cc_data_1 == 0xd || cc_data_2 == 0xd)
						CC_DECODE_DBG("The CR command received............................\n");
					if(cc_type == 2)
						am_parse_korea_cc_data_handle(cc_data_1, cc_data_2);
				}else{
					if(cc_type == 0x3){
						dtvcc_cnt = am_dtvcc_handle.packet_size;
						//CC_DECODE_DBG("333333 the cnt is:%d.\n", dtvcc_cnt);
						if (dtvcc_cnt > 0) {
							/* End of DTVCC packet. */
							CC_DECODE_DBG("To process packet and type is 33333333.\n");
							//dtvcc_decode_packet (&td->dtvcc, &now, pts);
							am_process_dtvcc(&am_dtvcc_handle);
						}
						if (!cc_valid) {
							/* No new data. */
							CC_DECODE_DBG("No new data......\n");
							am_dtvcc_handle.packet_size = 0;
						} else {
							am_dtvcc_handle.packet[0] = cc_data_1;
							am_dtvcc_handle.packet[1] = cc_data_2;
							am_dtvcc_handle.packet_size = 2;
						}
					}else if(cc_type == 0x2){
						dtvcc_cnt = am_dtvcc_handle.packet_size;

						if (dtvcc_cnt <= 0) {
							/* Missed packet start. */
							CC_DECODE_DBG("Missed packet start......\n");
						} else if (!cc_valid) {
							/* End of DTVCC packet. */
							//dtvcc_decode_packet (&td->dtvcc, &now, pts);
							CC_DECODE_DBG("To process packet and type is 2222222.\n");
							am_process_dtvcc(&am_dtvcc_handle);
							am_dtvcc_handle.packet_size = 0;
						} else if (dtvcc_cnt >= MAX_CC) {
							/* Packet buffer overflow. */
							CC_DECODE_DBG("Packet buffer overflow......\n");
							am_init_dtvcc_varible(0);
							am_dtvcc_handle.packet_size = 0;
						} else {
							am_dtvcc_handle.packet[dtvcc_cnt] = cc_data_1;
							am_dtvcc_handle.packet[dtvcc_cnt + 1] = cc_data_2;
							am_dtvcc_handle.packet_size = dtvcc_cnt + 2;
						}
					}
					//am_parse_atsc_cc_data_handle(cc_data_1, cc_data_2);
				}
			}
#endif
		}

		buffer+=3;
	}

	return;
	}

	/*===============================================================
	 *  Function Name:
	 *  Function Description:
	 *  Function Parameters:
	 *  Function Return:
	 *===============================================================*/
	void AM_Set_CC_Country(int country)
	{
		CC_DECODE_DBG("The cc country has been set to country:0x%x.", country);
		cc_country = country;
		return;
	}

	/*===============================================================
	 *  Function Name:
	 *  Function Description:
	 *  Function Parameters:
	 *  Function Return:
	 *===============================================================*/
	void AM_Set_CC_SourceType(int cmd)
	{
				CC_DECODE_DBG("The cc_source_type set cmd= 0x%0x\n", cmd);
		if(cmd == CMD_CC_SET_VBIDATA)
		{
			cc_souretype = CC_NTSC_VBI_LINE21;
		}
		else if(cmd == CMD_CC_SET_USERDATA)
		{
			cc_souretype = CC_ATSC_USER_DATA;
		}else
		{
		}
		CC_DECODE_DBG("The cc_source_type set as 0x%0x\n", cc_souretype);
		return;
	}

	/*===============================================================
	 *  Function Name:
	 *  Function Description:
	 *  Function Parameters:
	 *  Function Return:
	 *===============================================================*/
	AM_CC_SourceType_t AM_Get_CC_SourceType(void)
	{
		CC_DECODE_DBG("The cc_source_type is %d\n", cc_souretype);
		return cc_souretype ;
	}

	/*===============================================================
	 *  Function Name:
	 *  Function Description:
	 *  Function Parameters:
	 *  Function Return:
	 *===============================================================*/

	void AM_Set_Cur_CC_Type(int type)
	{
		CC_DECODE_DBG("The cc type has been set to type:0x%x\n", type);
		cc_cur_type = type;
                switch(type){
		      case CMD_CC_1:
                        currentServiceType = PRIMARYLINE21DATASERVICE1;
                        break;

                     case CMD_CC_2:
                        currentServiceType = PRIMARYLINE21DATASERVICE2;
                        break;

                     case CMD_CC_3:
                        currentServiceType = SECONDARYLINE21DATASERVICE1;
                        break;

                     case CMD_CC_4:
                        currentServiceType = SECONDARYLINE21DATASERVICE2;
                        break;

                     case CMD_SERVICE_1:
                        currentServiceType = PRIMARYCAPTIONSERVICE;
                        break;

                     case CMD_SERVICE_2:
                        currentServiceType = SECONDARYLANGUAGESERVICE1;
                        break;

                     case CMD_SERVICE_3:
                        currentServiceType = SECONDARYLANGUAGESERVICE2;
                        break;

                     case CMD_SERVICE_4:
                        currentServiceType = SECONDARYLANGUAGESERVICE3;
                        break;

                     case CMD_SERVICE_5:
                        currentServiceType = SECONDARYLANGUAGESERVICE4;
                        break;

                     case CMD_SERVICE_6:
                        currentServiceType = SECONDARYLANGUAGESERVICE5;
                        break;
                     
 		     default :break;

                }
		return;
	}

	/*===============================================================
	 *  Function Name:
	 *  Function Description:
	 *  Function Parameters:
	 *  Function Return:
	 *===============================================================*/
	void AM_Force_Clear_CC()
	{
		CC_DECODE_DBG("===== force clear cc ========");  
		temp_cc1_char_idx = 0;
		memset(temp_cc1_char, 0, sizeof(temp_cc1_char));
		am_ntsc_show_caption(1);
		return;
	}
	/*===============================================================
	 *  Function Name :  am_cc_check_oddparity
	 *  Function Description:  oddParityCheck 
	 *  Function Parameters:
	 *  Function Return:
	 *===============================================================*/
	static int am_cc_check_oddparity(int b)
	{
		int chk, k;

		chk = (b & 1);
		for (k=0; k<7; k++){
			b >>= 1;
			chk ^= (b & 1);
		}

		return(chk & 1);
	} 

	/*===============================================================
	 *  Function Name:
	 *  Function Description:
	 *  Function Parameters:
	 *  Function Return:
	 *===============================================================*/
	static int am_get_specialcharacter(unsigned char data)
	{
		switch(data)
		{
                        case 0x30:  return 0xAE;   /* Mapped to Extended ASCII Code registered trademark sign */
                        case 0x31:  return 0xB0;   /* Mapped to Extended ASCII Code degree                    */
                        case 0x32:  return 0xBD;   /* Mapped to Extended ASCII Code fraction 1/2              */
                        case 0x33:  return 0xBF;   /* Mapped to Extended ASCII Code inverted question mark    */
                        case 0x34:  return 0x99;   /* Mapped to Extended ASCII Code trademark (TM)            */
                        case 0x35:  return 0xA2;   /* Mapped to Extended ASCII Code cents sign                */
                        case 0x36:  return 0xA3;   /* Mapped to Extended ASCII Code pound sterling sign       */
                        case 0x37:  return 0x266A; /* Mapped to Extended ASCII Code music note                */
                        case 0x38:  return 0xE0;   /* Mapped to Extended ASCII Code small a grave             */
                        case 0x39:  return 0x25AF; /* Mapped to Extended ASCII Code smile symbol              */
                        case 0x3A:  return 0xE8;   /* Mapped to Extended ASCII Code small e grave             */
                        case 0x3B:  return 0xE2;   /* Mapped to Extended ASCII Code small a circumflex        */
                        case 0x3C:  return 0xEA;   /* Mapped to Extended ASCII Code small e circumflex        */
                        case 0x3D:  return 0xEE;   /* Mapped to Extended ASCII Code small i circumflex        */
                        case 0x3E:  return 0xF4;   /* Mapped to Extended ASCII Code small o circumflex        */
                        case 0x3F:  return 0xFB;   /* Mapped to Extended ASCII Code small u circumflex        */
                        default:    return 0x20;

		} 
	}
	/*===============================================================
	 *  Function Name:
	 *  Function Description:
	 *  Function Parameters:
	 *  Function Return:
	 *===============================================================*/
	static int am_get_608characters(unsigned char data)
	{
		switch(data)
		{
			case 0x2A: return 0xE1;     /* Refer EIA608 Annexure-D Pg89 */
			case 0x5C: return 0xE9;     /* Refer EIA608 Annexure-D Pg89 */
			case 0x5E: return 0xED;     /* Refer EIA608 Annexure-D Pg89 */
			case 0x5F: return 0xF3;     /* Refer EIA608 Annexure-D Pg89 */
			case 0x60: return 0xFA;     /* Refer EIA608 Annexure-D Pg89 */
			case 0x7B: return 0xE7;     /* Refer EIA608 Annexure-D Pg89 */
			case 0x7C: return 0xF7;     /* Mapped to divide sign */
			case 0x7D: return 0xD1;     /* Refer EIA608 Annexure-D Pg89 */
			case 0x7E: return 0xF1;     /* Refer EIA608 Annexure-D Pg89 */
			case 0x7F: return 0x25AE;   /* Mapped to ascii solid block */
			default:   return data;     /* Regular ascii codes */
		}	
	}
	/*===============================================================
	 *  Function Name:
	 *  Function Description:
	 *  Function Parameters:
	 *  Function Return:
	 *===============================================================*/
	static int am_get_spanishfrenchcharacter(unsigned char data)
	{
		switch(data)
		{
			case 0x20: return 0xC1;  
			case 0x21: return 0xC9;  
			case 0x22: return 0xD3;  
			case 0x23: return 0xDA;  
			case 0x24: return 0xDC;  
			case 0x25: return 0xFC;  
			case 0x26: return 0x2018;
			case 0x27: return 0xA1;  
			case 0x28: return 0x2A/*0x204E*/;
			case 0x29: return 0x2C8; 
			case 0x2A: return 0x2014;
			case 0x2B: return 0xA9;  
			case 0x2C: return 0x2120;
			case 0x2D: return 0x2022;
			case 0x2E: return 0x201C;
			case 0x2F: return 0x201D;
			case 0x30: return 0xC0;  
			case 0x31: return 0xC2;  
			case 0x32: return 0xC7;  
			case 0x33: return 0xC8;  
			case 0x34: return 0xCA;  
			case 0x35: return 0xCB;  
			case 0x36: return 0xEB;  
			case 0x37: return 0xCE;  
			case 0x38: return 0xCF;  
			case 0x39: return 0xEF;  
			case 0x3A: return 0xD4;  
			case 0x3B: return 0xD9;  
			case 0x3C: return 0xF9;  
			case 0x3D: return 0xDB;  
			case 0x3E: return 0xAB;  
			case 0x3F: return 0xBB;  
			default: return 0x20;
		}
	}
	/*===============================================================
	 *  Function Name:
	 *  Function Description:get Portuguese,German,Danish charset
	 *  Function Parameters:
	 *  Function Return:
	 *===============================================================*/
	static int am_get_PGDcharacter(unsigned char data)
	{
		switch(data)
		{
			case 0x20: return 0xC3;   
			case 0x21: return 0xE3;   
			case 0x22: return 0xCD;   
			case 0x23: return 0xCC;   
			case 0x24: return 0xEC;   
			case 0x25: return 0xD2;   
			case 0x26: return 0xF2;   
			case 0x27: return 0xD5;   
			case 0x28: return 0xF5;   
			case 0x29: return 0x7B;   
			case 0x2A: return 0x7D;   
			case 0x2B: return 0x5C;   
			case 0x2C: return 0x5E;   
			case 0x2D: return 0x5F;   
			case 0x2E: return 0x7C;   
			case 0x2F: return 0x7E;   
			case 0x30: return 0xC3;   
			case 0x31: return 0xE4;   
			case 0x32: return 0xD6;   
			case 0x33: return 0xF6;   
			case 0x34: return 0xDF;   
			case 0x35: return 0xA5;   
			case 0x36: return 0xA4;   
			case 0x37: return 0xA6;   
			case 0x38: return 0xC5;   
			case 0x39: return 0xE5;   
			case 0x3A: return 0xD8;   
			case 0x3B: return 0xF8;   
			case 0x3C: return 0x23A1; 
			case 0x3D: return 0x23A4; 
			case 0x3E: return 0x23A3; 
			case 0x3F: return 0x23A6; 
			default:   return 0x20;
		}
	}

	/*
	 *
	 * am_get_DCevent :
	 *
	 * Decodes the Line21 caption commands and returns the
	 * corresponding events.
	 *
	 */

	static AM_CCEvent_t am_get_DCevent(unsigned char data)
	{
		data = data & 0x1F;
		switch(data)
		{
			case 0x00: return WHITENOUNDERLINE;
			case 0x01: return WHITEUNDERLINE;
			case 0x02: return GREENNOUNDERLINE;
			case 0x03: return GREENUNDERLINE;
			case 0x04: return BLUENOUNDERLINE;
			case 0x05: return BLUEUNDERLINE;
			case 0x06: return CYANNOUNDERLINE;
			case 0x07: return CYANUNDERLINE;
			case 0x08: return REDNOUNDERLINE;
			case 0x09: return REDUNDERLINE;
			case 0x0A: return YELLOWNOUNDERLINE;
			case 0x0B: return YELLOWUNDERLINE;
			case 0x0C: return MAGENTANOUNDERLINE;
			case 0x0D: return MAGENTAUNDERLINE;
			case 0x0E: return ITALICSNOUNDERLINE;
			case 0x0F: return ITALICSUNDERLINE;
			case 0x10: return WHITEINDENT0NOUNDERLINE;
			case 0x11: return WHITEINDENT0UNDERLINE;
			case 0x12: return WHITEINDENT4NOUNDERLINE;
			case 0x13: return WHITEINDENT4UNDERLINE;
			case 0x14: return WHITEINDENT8NOUNDERLINE;
			case 0x15: return WHITEINDENT8UNDERLINE;
			case 0x16: return WHITEINDENT12NOUNDERLINE;
			case 0x17: return WHITEINDENT12UNDERLINE;
			case 0x18: return WHITEINDENT16NOUNDERLINE;
			case 0x19: return WHITEINDENT16UNDERLINE;
			case 0x1A: return WHITEINDENT20NOUNDERLINE;
			case 0x1B: return WHITEINDENT20UNDERLINE;
			case 0x1C: return WHITEINDENT24NOUNDERLINE;
			case 0x1D: return WHITEINDENT24UNDERLINE;
			case 0x1E: return WHITEINDENT28NOUNDERLINE;
			case 0x1F: return WHITEINDENT28UNDERLINE;
			default:   return ILLEGALCODE;
		}

	}
	/* API to parse variant case of CC608 Ctrl commane*/
	/*===============================================================^M
	 *  Function Name:^M
	 *  Function Description:^M
	 *  Function Parameters:^M
	 *  Function Return:^M
	 *  cc data detail according cc608:
	 *	 11h/19h	  30h->3Fh	Special Printable Characters
	 *	 12h/1Ah		20h->3Fh	Extended Character Set
	 *	 13h/1Bh		20h->3Fh	Extended Character Set
	 *	 20h-7Fh		20h-7Fh		Printable Text
	 *===============================================================*/
	static void am_handle_printtabletext(AM_CCUserData_t userData)
	{
                CC_DECODE_DBG("\n[%s:%d]===================>:0x%x :0x%x \n",__FUNCTION__,__LINE__,userData.buf1,userData.buf2 );
		temp_cc1_char_idx %=512;	
		switch(userData.level1Event){
			case SPECIALPRINTABLETEXT :
			case SPECIALPRINTABLETEXT1:				
				//the special char which we should convert to unicode and send to UI
                                if(userData.buf1 == last_ntsc_special_char){
				    last_ntsc_special_char = 0;
			            return;
                                }
				temp_cc1_char[temp_cc1_char_idx++] = userData.buf1;//AM_CC_Decode_Line21VBI already convert it as unicode
   				last_ntsc_special_char = userData.buf1;
				CC_DECODE_DBG("=====ntsc special char1:0x%x \n", userData.buf1);		  		
				break;

			default :  
				if(0xFF != userData.buf1)
                                    temp_cc1_char[temp_cc1_char_idx++] = userData.buf1;
                                CC_DECODE_DBG("cc data1 is the char:%c -> 0x%x \n",userData.buf1,userData.buf1);
                                if(0xFF != userData.buf2)
                                    temp_cc1_char[temp_cc1_char_idx++] = userData.buf2;
                                CC_DECODE_DBG("cc data2 is the char:%c -> 0x%x \n",userData.buf2,userData.buf2);
                                if(0xFF != userData.buf1 || 0xFF != userData.buf2)
                                    last_ntsc_special_char = 0;
				break; 
		}	
	}
	/*===============================================================^M
	 *  Function Name:^M
	 *  Function Description:^M
	 *  Function Parameters:^M
	 *  Function Return:^M
	 *  cc data detail according cc608:
	 *	 11h/19h		40h->4Fh	PAC Row 1
	 *				60h->7Fh	PAC Row 2
	 *	 12h/1Ah	        40h->5Fh	PAC Row 3
	 *			        60h->7Fh	PAC Row 4
	 *
	 *	 13h/1Bh	        40h->5Fh	PAC Row 12
	 *			        60h->7Fh	PAC Row 13
	 *
	 *	 14h/1Ch	        40h->5Fh	PAC Row 14
	 *			        60h->7Fh	PAC Row 15
	 *
	 *	 15h/1Dh	        40h->5Fh	PAC Row 5
	 *			        60h->7Fh	PAC Row 6
	 *
	 *	 16h/1Eh	        40h->5Fh	PAC Row 7
	 *			        60h->7Fh	PAC Row 8
	 *
	 *	 17h/1Fh	        40h->5Fh	PAC Row 9
	 *			        60h->7Fh	PAC Row 10
	 *===============================================================*/
	static void am_handle_preamblecodes(AM_CCUserData_t userData)
	{
		if( CC_POP_ON  == ntsc_cc_show_style ||
				CC_ROLL_UP == ntsc_cc_show_style ||
				CC_PAINT_ON == ntsc_cc_show_style
		  )
		{
			int font_row = 0, font_col = 0, font_color = 0;
				//int row_number[16]={11, 15, 1, 2, 3, 4, 12, 13,14, 15, 5, 6, 7, 8, 9, 10};
			int col_number[16]={0, 0, 0, 0, 0, 0, 0, 0,0, 4, 8, 12, 16, 20, 24, 28};
			int color_number[16]={0xFFF, 0x0F0, 0x00F, 0x0FF, 0xF00, 0xFF0, 0xF0F, 
					0xFFF,0xFFF, 0xFFF, 0xFFF, 0xFFF, 0xFFF, 0xFFF, 0xFFF, 0xFFF};
			int crc_sum = 0;

			int cc_data_1 = userData.buf1;
			int cc_data_2 = userData.buf2;

			CC_DECODE_DBG("\n[%s:%d]===================>:0x%x :0x%x \n",__FUNCTION__,__LINE__,userData.buf1,userData.buf2 );

                        if((last_level1Event == userData.level1Event) && (last_level1Event == userData.level1Event))
                        {
                            CC_DECODE_DBG("duplicated preamble code received and nothing would be done");
                            last_level1Event = ILLEGALCODE;
	                    last_level2Event = ILLEGALCODE;

			    return;
                        }

                        last_level1Event = userData.level1Event;
                        last_level2Event = userData.level2Event;

			font_row = userData.level1Event-ROW1+1; //row_number[(cc_data_1&0x7)<<1|(cc_data_2&0x20)>>5];
			font_col = col_number[(cc_data_2&0x10)>>1|(cc_data_2&0xE)>>1];
			font_color = color_number[(cc_data_2&0x10)>>1|(cc_data_2&0xE)>>1];
			temp_cc1_char[temp_cc1_char_idx++] = MAGIC_NUM;
			temp_cc1_char[temp_cc1_char_idx++] = font_row;
			temp_cc1_char[temp_cc1_char_idx++] = font_col;
			temp_cc1_char[temp_cc1_char_idx++] = font_color;
			temp_cc1_char[temp_cc1_char_idx++] = 0;
			temp_cc1_char[temp_cc1_char_idx++] = 0;
			temp_cc1_char[temp_cc1_char_idx++] = 0;
			temp_cc1_char[temp_cc1_char_idx++] = 0;
			temp_cc1_char[temp_cc1_char_idx++] = 0;
			crc_sum = MAGIC_NUM^font_row^font_col^font_color;
			temp_cc1_char[temp_cc1_char_idx++] = crc_sum;//CRC check sum?
			CC_DECODE_DBG("******* Preamble address code:row:0x%x, col:0x%x, color:0x%x, CRC:0x%x\n", font_row, font_col, font_color, crc_sum);
			//since we using preamble code to update the cc to UI, which should do every time you received char
			if(CC_PAINT_ON== ntsc_cc_show_style){
				if(temp_cc1_char_idx > MAGIC_NUM_CNT)
				{
					CC_DECODE_DBG("------->>>>> show caption in CC_PAINT_ON with cmd preamble code \n");
					am_ntsc_show_caption(1);
					CC_DECODE_DBG("The char cnt is:%d, ntsc_cc_cmd_idx:%d\n", temp_cc1_char_idx, ntsc_cc_cmd_idx);
				}
				CC_DECODE_DBG("going to update the CC in paint on mode\n");
			}
		}
	}

	/*===============================================================^M
	 *  Function Name:^M
	 *  Function Description:^M
	 *  Function Parameters:^M
	 *  Function Return:^M
	 *  cc data detail according cc608:
	 *  11h/19h		20h->2Fh	Mid Row Code
	 *===============================================================*/
	static void am_handle_midrowcodes(AM_CCUserData_t userData)
	{
		CC_DECODE_DBG("\n^^^^^^^^^ middle row code reserved and ignored and blank will be added ^^^^^^^^^^^^^^^\n");
		temp_cc1_char[temp_cc1_char_idx++] = 0x20;//atually it need set CC pen attrubite like color and reserve for future.
                CC_DECODE_DBG("\n[%s:%d]===================>:0x%x :0x%x \n",__FUNCTION__,__LINE__,userData.buf1,userData.buf2 );
	} 

	/*===============================================================^M
	 *  Function Name:^M
	 *  Function Description:^M
	 *  Function Parameters:^M
	 *  Function Return:^M
	 *  cc data detail according cc608:
	 *	 10h/18h 	20h->2Fh	Extended Code (background/foreground color)
	 *===============================================================*/
	static void am_handle_extendedcodes(AM_CCUserData_t userData)
	{
           CC_DECODE_DBG("\n[%s:%d]===================>:0x%x :0x%x \n",__FUNCTION__,__LINE__,userData.buf1,userData.buf2 );

	}

	/*===============================================================^M
	 *  Function Name:^M
	 *  Function Description:^M
	 *  Function Parameters:^M
	 *  Function Return:^M
	 *  cc data detail according c608:
	 *	  14h/1Ch		20h->2Fh	Misc Control Code
	 *	  15h/1Dh		20h->2Fh	Misc Control Code
	 *	  17h/1Fh		21h->23h	Misc Control Code
	 *===============================================================*/
	static void am_handle_misccodes(AM_CCUserData_t userData)
	{
		if( PRIMARYLINE21CODE == userData.level1Event   ||
				SECONDARYLINE21CODE == userData.level1Event ||
				ILLEGALCODE == userData.level1Event )
		{
			lastCtrlCmd = userData.level2Event;
			switch(userData.level2Event){
				case RESUMECAPTIONLOADING:
					CC_DECODE_DBG("#########  pop on style be set ************************\n");
					if(temp_cc1_char_idx > MAGIC_NUM_CNT)
					{
						CC_DECODE_DBG("------->>>>> show caption because there are char with cmd 0x20 \n");
						am_ntsc_show_caption(1);
						memset(temp_cc1_char, 0, 512);
						temp_cc1_char_idx = 0;
						//memset(ntsc_cc_cmd, 0, 512);
						//ntsc_cc_cmd_idx = 0;
						ntsc_cc_show_style = 0;
						CC_DECODE_DBG("The char cnt is:%d, ntsc_cc_cmd_idx:%d\n", temp_cc1_char_idx, ntsc_cc_cmd_idx);
					}
					ntsc_cc_show_style = CC_POP_ON;
					ntsc_cc_last_show_style = CC_POP_ON;
					break;

				case ROLLUP2ROWS: /* Caption Mode - Roll_up style */
				case ROLLUP3ROWS:
				case ROLLUP4ROWS:
					CC_DECODE_DBG("######### roll up style be set ^^^^^^^^^^^^^^^^^^^^\n");
					if(temp_cc1_char_idx > MAGIC_NUM_CNT && ntsc_cc_last_show_style != CC_ROLL_UP)
					{
						CC_DECODE_DBG("------->>>>> show caption because there are char with cmd roll up \n");
						am_ntsc_show_caption(1);
						memset(temp_cc1_char, 0, 512);
						temp_cc1_char_idx = 0;
						//memset(ntsc_cc_cmd, 0, 512);
						//ntsc_cc_cmd_idx = 0;
						ntsc_cc_show_style = 0;
						CC_DECODE_DBG("The char cnt is:%d, ntsc_cc_cmd_idx:%d\n", temp_cc1_char_idx, ntsc_cc_cmd_idx);
					}
					ntsc_cc_show_style = CC_ROLL_UP;
					ntsc_cc_last_show_style = CC_ROLL_UP;
					break; 

				case RESUMEDIRECTCAPTIONING:
					CC_DECODE_DBG("######### paint on style be set $$$$$$$$$$$$$$$$$$$$$$$$\n");
					if(temp_cc1_char_idx > MAGIC_NUM_CNT && ntsc_cc_last_show_style != CC_PAINT_ON)
					{
						CC_DECODE_DBG("------->>>>> show caption because there are char with cmd 0x29 \n");
						am_ntsc_show_caption(1);
						memset(temp_cc1_char, 0, 512);
						temp_cc1_char_idx = 0;
						//memset(ntsc_cc_cmd, 0, 512);
						//ntsc_cc_cmd_idx = 0;
						ntsc_cc_show_style = 0;
						CC_DECODE_DBG("The char cnt is:%d, ntsc_cc_cmd_idx:%d\n", temp_cc1_char_idx, ntsc_cc_cmd_idx);
					}
					ntsc_cc_show_style = CC_PAINT_ON;
					ntsc_cc_last_show_style = CC_PAINT_ON;
					break;

				case ERASEDISPLAYEDMEMORY :
					if( CC_POP_ON  == ntsc_cc_show_style || CC_ROLL_UP == ntsc_cc_show_style){
						if(temp_cc1_char_idx > MAGIC_NUM_CNT)
						{
							CC_DECODE_DBG("------->>>>> show caption in CC_ROLL_UP with cmd 0x2C -> eraseDisplayedMemory\n");
							am_ntsc_show_caption(1);
								
							memset(temp_cc1_char, 0, 512);
							temp_cc1_char_idx = 0;
							//memset(ntsc_cc_cmd, 0, 512);
							//ntsc_cc_cmd_idx = 0;
							ntsc_cc_show_style = 0;
							CC_DECODE_DBG("The char cnt is:%d, ntsc_cc_cmd_idx:%d\n", temp_cc1_char_idx, ntsc_cc_cmd_idx);
						}
					}
					break;

				case CARRIAGERETURN :
					if( CC_ROLL_UP  == ntsc_cc_show_style){
						if(temp_cc1_char_idx > MAGIC_NUM_CNT)
						{
							CC_DECODE_DBG("------->>>>> show caption in CC_POP_ON with cmd 0x2D-> carriageReturn \n");              
							am_ntsc_show_caption(1);
							//ntsc_cc_show_style = 0;
							CC_DECODE_DBG("The char cnt is:%d, ntsc_cc_cmd_idx:%d\n", temp_cc1_char_idx, ntsc_cc_cmd_idx);
						}
					}
					break;

				case ENDOFCAPTION :
					if( CC_POP_ON  == ntsc_cc_show_style){
						if(temp_cc1_char_idx > MAGIC_NUM_CNT)
						{
							CC_DECODE_DBG("------->>>>> show caption in CC_POP_ON with cmd 0x2F-> endofcaption \n");
							am_ntsc_show_caption(1);
							memset(temp_cc1_char, 0, 512);
							temp_cc1_char_idx = 0;
							//memset(ntsc_cc_cmd, 0, 512);
							//ntsc_cc_cmd_idx = 0;
							ntsc_cc_show_style = 0;
							CC_DECODE_DBG("The char cnt is:%d, ntsc_cc_cmd_idx:%d\n", temp_cc1_char_idx, ntsc_cc_cmd_idx);
						}
					}
					break;

				default : break;  		
			}  	
		}
		else
		{
			CC_DECODE_DBG("\n[%s] : bad Code\n\n", __FUNCTION__);
		}

	}
	/*===============================================================
	 *  Function Name:
	 *  Function Description:
	 *  Function Parameters:
	 *  Function Return:
	 *===============================================================*/
	static void am_process_ntsc_cc_command(AM_CCUserData_t userData)
	{
		//process CC608 state machine
		int cc_data_1,cc_data_2;

		cc_data_1 = userData.buf1;
		cc_data_2 = userData.buf2;
            
                CC_DECODE_DBG("\n[%s:%d]===================>:0x%x :0x%x \n",__FUNCTION__,__LINE__,cc_data_1,cc_data_2 );

		switch(userData.mainCommand)
		{
			case MISCCONTROLCODE:
				am_handle_misccodes(userData);
				break;
			case EXTENDEDCODE:
				am_handle_extendedcodes(userData);
				break;
			case PREAMBLEADDRESSCODE:
				am_handle_preamblecodes(userData);
				break;
			case MIDROWCONTROLCODE:
				am_handle_midrowcodes(userData);
				break;
			case PRINTABLETEXT:
				am_handle_printtabletext(userData);
				break;
			default: break;
		}

	}
	/*===============================================================^M
	 *  Function Name:^M
	 *  Function Description:^M
	 *  Function Parameters:^M
	 *  Function Return:^M
	 *===============================================================*/

	static void am_process_miscctrcmd_4_korea(unsigned char temp1,AM_CCUserData_t *userData)
	{

		switch(temp1){
			case 0x20:
				CC_DECODE_DBG("\nMCC  - resumeCaptionLoading,CH%d\n",(int)userData->channel);
				userData->level2Event = RESUMECAPTIONLOADING;
				break;
			case 0x21:
				CC_DECODE_DBG("\nMCC  - backspace ,CH%d\n",(int)userData->channel);
				userData->level2Event = RESUMEDIRECTCAPTIONING;	//backspace;
				break;
			case 0x24:
				CC_DECODE_DBG("\nMCC  - delteToEndOfRow ,CH%d\n",(int)userData->channel);
				userData->level2Event=DELETETOEOR;
				break;
			case 0x25:
				CC_DECODE_DBG("\nMCC  - rollUp2rows,CH%d\n",(int)userData->channel);
				userData->level2Event=ROLLUP2ROWS;
				break;
			case 0x26:
				CC_DECODE_DBG("\nMCC  - rollUp3rows ,CH%d\n",(int)userData->channel);
				userData->level2Event=ROLLUP3ROWS;
				break;
			case 0x27:
				CC_DECODE_DBG("\nMCC  - rollUp4rows,CH%d\n",(int)userData->channel);
				userData->level2Event=ROLLUP4ROWS;
				break;
			case 0x28:
				CC_DECODE_DBG("\nMCC  - flashOn,CH%d\n",(int)userData->channel);
				userData->level2Event=ROLLUP2ROWS;	//flashOn;
				break;
			case 0x29:
				CC_DECODE_DBG("\nMCC  - resumeDirectCaptioning,CH%d\n",(int)userData->channel);
				userData->level2Event=ROLLUP3ROWS;	//resumeDirectCaptioning;
				break;
			case 0x2A:
				CC_DECODE_DBG("\nMCC  - textRestart,CH%d\n",(int)userData->channel);
				userData->level2Event=ROLLUP4ROWS;	//textRestart;
				break;
			case 0x2B:
				CC_DECODE_DBG("\nMCC  - resumeTextDisplay,CH%d\n",(int)userData->channel);
				userData->level2Event=RESUMETEXTDISPLAY;
				break;
			case 0x2C:
				CC_DECODE_DBG("\nMCC  - eraseDisplayedMemory,CH%d\n",(int)userData->channel);
				userData->level2Event=ERASEDISPLAYEDMEMORY;
				break;
			case 0x2D:
				CC_DECODE_DBG("\nMCC  - carriageReturn,CH%d\n",(int)userData->channel);
				userData->level2Event=ERASENONDISPLAYEDMEMORY;	//carriageReturn;
				break;
			case 0x2E:
				CC_DECODE_DBG("\nMCC- eraseNonDisplayedMemory");
				userData->level2Event=CARRIAGERETURN;	//eraseNonDisplayedMemory;
				break;
			case 0x2F:
				CC_DECODE_DBG("\nMCC- endOfCaption");
				userData->level2Event=ENDOFCAPTION;
				break;

		}
	}
	/*===============================================================
	 *  Function Name : AM_CC_Decode_Line21VBI
	 *  Function Parameters:data1,data2,n_lines
	 *  Function Return:
	 *
	 *      Software decoder for the Line21 Data
	 *       Control Code : Command Interpretation for 608 Decoder.
	 *
	 *	01 -> 0F  :	XDS Control Code
	 *	10 -> 1F  :	Caption/TextMode Control Code
	 *	20 -> 7F  : Printable Text Characters.
	 *
	 *
	 *	Caption/TextMode Command Interpretaion :
	 *
	 *      ------------------------------------------------------------------------
	 * 	Byte 1		Byte2		Description
	 * 	------------------------------------------------------------------------
	 *
	 * 	01h-0Fh					XDS Data
	 *
	 *	10h/18h 	20h->2Fh	Extended Code (background/foreground color)
	 *			40h->5fh	PAC Row 11
	 *
	 *	11h/19h		20h->2Fh	Mid Row Code
	 *			30h->3Fh	Special Printable Characters
	 *			40h->4Fh	PAC Row 1
	 *			60h->7Fh	PAC Row 2
	 *
	 *	12h/1Ah		20h->3Fh	Extended Character Set
	 *			40h->5Fh	PAC Row 3
	 *			60h->7Fh	PAC Row 4
	 *
	 *	13h/1Bh		20h->3Fh	Extended Character Set
	 *			40h->5Fh	PAC Row 12
	 *			60h->7Fh	PAC Row 13
	 *
	 *	14h/1Ch		20h->2Fh	Misc Control Code
	 *			40h->5Fh	PAC Row 14
	 *			60h->7Fh	PAC Row 15
	 *
	 *	15h/1Dh		20h->2Fh	Misc Control Code
	 *			40h->5Fh	PAC Row 5
	 *			60h->7Fh	PAC Row 6
	 *
	 *	16h/1Eh		40h->5Fh	PAC Row 7
	 *			60h->7Fh	PAC Row 8
	 *
	 *	17h/1Fh		21h->23h	Misc Control Code
	 *			24h->2Ah	Extended Code Special Assignement
	 *			2Dh->2Fh	Etexnded Code Special Assginement
	 *			40h->5Fh	PAC Row 9
	 *			60h->7Fh	PAC Row 10
	 *
	 *	20h-7Fh		20h-7Fh		Printable Text
	 *
	 *=====================================================================================*/
	AM_ErrorCode_t AM_CC_Decode_Line21VBI(unsigned char data1,unsigned char data2,unsigned int n_lines)
	{
		AM_ErrorCode_t err = AM_SUCCESS;
		AM_CCUserData_t userData;
		unsigned char   temp1,temp2;

		CC_DECODE_DBG("\n%s >>>>>>>>>> line21 cc data 0x%x,0x%x ,cc_type :0x%x cc_country:0x%x <<<<<<<<<<\n",
			       __FUNCTION__,data1,data2,cc_cur_type,cc_country);

#ifdef USER_REAL_CC608_PARSE    //use real CC608 parsere

		//if(cc_cur_type == CMD_CC_1 && cc_country == CMD_SET_COUNTRY_USA)
			am_parse_usa_ntsc_cc_data(data1, data2);

#else

		n_lines = 284 ;
		temp1 = data1 & 0x7F;

		if(am_cc_check_oddparity(data1)==0){

			data1 = 0;
			if (am_cc_check_oddparity(data2)==0){

				if (temp1>=0x01 && temp1<=0x0F )
				{     
					CC_DECODE_DBG("\n[%s:%d] cc data1 parity check error!\n",__FUNCTION__,__LINE__);
					return (AM_FAILURE);
				}
				else if ( temp1>0x1F ){
					data2 = 0;
				}
			}
		} else {
			if (am_cc_check_oddparity(data2)==0){
				if (temp1>=0x01 && temp1<=0x0F) 
				{
					CC_DECODE_DBG("\n[%s:%d] cc data2 parity check error!\n",__FUNCTION__,__LINE__);
					return (AM_FAILURE);
				}
				else if (temp1>=0x10 && temp1<=0x1F) 
				{
					CC_DECODE_DBG("\n[%s:%d] cc data2 parity check error!\n",__FUNCTION__,__LINE__);
					return (AM_FAILURE);
				}
				else 
					data2 = 0;
			}
		}


		temp1 = data1 & 0x7F;
		temp2 = data2 & 0x7F;

		if(temp1 == 0 && temp2 == 0)
		{   
			CC_DECODE_DBG("\n[%s:%d] receive cc NULL data!\n",__FUNCTION__,__LINE__);
			err = AM_FAILURE;
			return err;
		}

		userData.mainCommand = ILLEGALCODE;
		userData.level1Event = ILLEGALCODE;
		userData.level2Event = ILLEGALCODE;
		userData.channel = ILLEGALCHANNEL;
		userData.buf1 = 0;
		userData.buf2 = 0;

		CC_DECODE_DBG("\nDecode- [%d] = raw data :0x%x 0x%x  cc data :0x%x 0x%x\n", (int)n_lines, (int)data1, (int)data2,temp1,temp2);
		vbi_has_no_xds++;

		//XDS
		if (temp1 >= 0x01 && temp1 <= 0x0F ){
			if ( n_lines == 284 ){
				/*
				 * XDS 
				 * If the BUF1 Code is > 01h and < 0Fh and we have not
				 * started capturing XDS , then this is an XDS Control
				 * code char so call XDS Decoder.
				 * Note that XDS Data only comes in Line21 Field 2
				 *
				 * */

				userData.mainCommand = XDSCONTROLCODE;
				userData.level1Event = CCEVENT_RESERVED;
				userData.level2Event = CCEVENT_RESERVED;
				userData.channel = ILLEGALCHANNEL;
				CC_DECODE_DBG("xdsControlCode\n");

				userData.buf2 = temp2;
				vbi_has_no_xds =0;

				err = AM_Decode_XDSData(temp1,temp2);
				am_ntsc_vchip_callback();

			} /* if */
			/*field 1 also possible contain XDS data*/
			if ( temp1 == 0x0F )
				filterF1XDS = 0;
			else 
				filterF1XDS = 1;
			err = AM_SUCCESS;
			
			return err;
		}
		else if(temp1 >= 0x10 && temp1 <= 0x1F)
			{
				/* Reset this flag since interrupted by CC data */
				filterF1XDS = 0;	
				CC_DECODE_DBG("\n[ Line21Code 0x%x 0x%x]",temp1,temp2);

				/* Proceed further for decoding on the condition that the current
				 * field is 1 and caption selected is primaryLine21 service
				 * OR
				 * field is 2 and caption selected is secondary language caption.
				 *
				 */
				if(vbi_has_no_xds > 30)
				{
					vbi_has_no_xds =0;
					AM_Clr_Vchip_Event();
					am_ntsc_vchip_callback();
				}

				/* LINE 21 CLOSED CAPTIONS */
				switch(temp1){
					case 0x10:  /* CH1 */
					case 0x18:  /* CH2 */
						if (temp2 >= 0x20 && temp2 <= 0x2F){ /* Extended Control Codes, Background and Foreground Colors */
							userData.mainCommand = EXTENDEDCODE;
							userData.level1Event = IGNORECODE;
							userData.channel = (temp1 == 0x10 ? CC_CHANNEL1 : CC_CHANNEL2);
							userData.buf1 = 0;
							userData.buf2 = 0;

							switch( temp2 ){
								case 0x20:
									userData.level2Event = BGWHITEOPAQUE;
									CC_DECODE_DBG("\nExtCode   :bgWhiteOpaque" );
									break;
								case 0x21:
									userData.level2Event = BGWHITESEMITRANSPARENT;
									CC_DECODE_DBG("\nExtCode  : bgWhiteSemiTransparent " );
									break;
								case 0x22:
									userData.level2Event = BGGREENOPAQUE;
									CC_DECODE_DBG("\nExtCode  : bgGreenOpaque" );
									break;
								case 0x23:
									userData.level2Event = BGGREENSEMITRANSPARENT;
									CC_DECODE_DBG("\nExtCode  : bgGreenSemiTransparent" );
									break;
								case 0x24:
									userData.level2Event = BGBLUEOPAQUE;
									CC_DECODE_DBG("\nExtCode  :bgBlueOpaque " );
									break;
								case 0x25:
									userData.level2Event = BGBLUESEMITRANSPARENT;
									CC_DECODE_DBG("\nExtCode  : bgBlueSemiTransparent" );
									break;
								case 0x26:
									userData.level2Event = BGCYANOPAQUE;
									CC_DECODE_DBG("\nExtCode  : bgCyanOpaque" );
									break;
								case 0x27:
									userData.level2Event = BGCYANSEMITRANSPARENT;
									CC_DECODE_DBG("\nExtCode  : bgCyanSemiTransparent" );
									break;
								case 0x28:
									userData.level2Event = BGREDOPAQUE;
									CC_DECODE_DBG("\nExtCode  : bgRedOpaque" );
									break;
								case 0x29:
									userData.level2Event = BGREDSEMITRANSPARENT;
									CC_DECODE_DBG("\nExtCode  : bgRedSemiTransparent " );
									break;
								case 0x2A:
									userData.level2Event = BGYELLOWOPAQUE;
									CC_DECODE_DBG("\nExtCode  : bgYellowOpaque" );
									break;
								case 0x2B:
									userData.level2Event = BGYELLOWSEMITRANSPARENT;
									CC_DECODE_DBG("\nExtCode  : bgYellowSemiTransparent" );
									break;
								case 0x2C:
									userData.level2Event = BGMAGENTAOPAQUE;
									CC_DECODE_DBG("\nExtCode  : bgMagentaOpaque" );
									break;
								case 0x2D:
									userData.level2Event = BGMAGENTASEMITRANSPARENT;
									CC_DECODE_DBG("\nExtCode  : bgMagentaSemiTransparent " );
									break;
								case 0x2E:
									userData.level2Event = BGBLACKOPAQUE;
									CC_DECODE_DBG("\nExtCode  : bgBlackOpaque " );
									break;
								case 0x2F:
									userData.level2Event = BGBLACKSEMITRANSPARENT;
									CC_DECODE_DBG("\nExtCode  : bgBlackSemiTransparent " );
									break;
								default:
									CC_DECODE_DBG("\nIllegal Codes");
									userData.mainCommand = ILLEGALCODE;
									userData.level1Event = ILLEGALCODE;
									userData.level2Event = ILLEGALCODE;
									userData.channel = ILLEGALCHANNEL;
									break;
							}
						} else if(temp2 >= 0x40 && temp2 <= 0x5F){ /* Preamble Address Codes - Row 11*/
							userData.mainCommand = PREAMBLEADDRESSCODE;
							userData.level1Event = ROW11;
							userData.level2Event = am_get_DCevent(temp2);
							userData.channel = (temp1 == 0x10 ? CC_CHANNEL1 : CC_CHANNEL2);
							userData.buf1 = temp1;//0;
							userData.buf2 = temp2;//0;
							CC_DECODE_DBG("\nPAC-Row11  \n" );
						} else {
							CC_DECODE_DBG("\nIllegal Codes");
							userData.mainCommand = ILLEGALCODE;
							userData.level1Event = ILLEGALCODE;
							userData.level2Event = ILLEGALCODE;
							userData.channel = ILLEGALCHANNEL;
							userData.buf1 = 0;
							userData.buf2 = 0;
						}
						break;

					case 0x11:  //CH1
					case 0x19:  //CH2
						if (temp2 >= 0x20 && temp2 <= 0x2F){ /* Mid Row Code */
							userData.mainCommand = MIDROWCONTROLCODE;
							userData.level1Event = IGNORECODE;
							userData.channel = (temp1 == 0x11 ? CC_CHANNEL1 : CC_CHANNEL2);
							userData.buf1 = 0;
							userData.buf2 = 0;

							switch( temp2 ){
								case 0x20:
									CC_DECODE_DBG("\nMRC- whiteNoUnderline  " );
									userData.level2Event = WHITENOUNDERLINE;
									break;
								case 0x21:
									CC_DECODE_DBG("\nMRC- whiteUnderline  " );
									userData.level2Event = WHITEUNDERLINE;
									break;
								case 0x22:
									CC_DECODE_DBG("\nMRC- greenNoUnderline  " );
									userData.level2Event = GREENNOUNDERLINE;
									break;
								case 0x23:
									CC_DECODE_DBG("\nMRC- greenUnderline  " );
									userData.level2Event = GREENUNDERLINE;
									break;
								case 0x24:
									CC_DECODE_DBG("\nMRC- blueNoUnderline  " );
									userData.level2Event = BLUENOUNDERLINE;
									break;
								case 0x25:
									CC_DECODE_DBG("\nMRC- blueUnderline  " );
									userData.level2Event = BLUEUNDERLINE;
									break;
								case 0x26:
									CC_DECODE_DBG("\nMRC- cyanNoUnderline  " );
									userData.level2Event = CYANNOUNDERLINE;
									break;
								case 0x27:
									CC_DECODE_DBG("\nMRC- cyanUnderline  " );
									userData.level2Event = CYANUNDERLINE;
									break;
								case 0x28:
									CC_DECODE_DBG("\nMRC- redNoUnderline  " );
									userData.level2Event = REDNOUNDERLINE;
									break;
								case 0x29:
									CC_DECODE_DBG("\nMRC- redUnderline  " );
									userData.level2Event = REDUNDERLINE;
									break;
								case 0x2A:
									CC_DECODE_DBG("\nMRC- yellowNoUnderline  " );
									userData.level2Event = YELLOWNOUNDERLINE;
									break;
								case 0x2B:
									CC_DECODE_DBG("\nMRC- yellowUnderline  " );
									userData.level2Event = YELLOWUNDERLINE;
									break;
								case 0x2C:
									CC_DECODE_DBG("\nMRC- magentaNoUnderline  " );
									userData.level2Event = MAGENTANOUNDERLINE;
									break;
								case 0x2D:
									CC_DECODE_DBG("\nMRC- magentaUnderline  " );
									userData.level2Event = MAGENTAUNDERLINE;
									break;
								case 0x2E:
									CC_DECODE_DBG("\nMRC- itlaicsNoUnderline  " );
									userData.level2Event = ITALICSNOUNDERLINE;
									break;
								case 0x2F:
									CC_DECODE_DBG("\nMRC- itlaicsUnderline  " );
									userData.level2Event = ITALICSUNDERLINE;
									break;
								default:
									CC_DECODE_DBG("\nMRC- ILLEGALCODE");
									userData.mainCommand = ILLEGALCODE;
									userData.level1Event = ILLEGALCODE;
									userData.level2Event = ILLEGALCODE;
									userData.channel = ILLEGALCHANNEL;
									break;
							}
						}else if(temp2 >= 0x30 && temp2 <= 0x3F){ /* Special Character - Printable Text */
							userData.mainCommand = PRINTABLETEXT;
							userData.level1Event = SPECIALPRINTABLETEXT1;
							userData.level2Event = IGNORECODE;
							userData.channel = (temp1 == 0x11 ? CC_CHANNEL1 : CC_CHANNEL2);
							userData.buf1 = am_get_specialcharacter(temp2);
							userData.buf2 = 0;
							CC_DECODE_DBG("\nText[%c:0x%x]",(int)userData.buf1,temp2);
						} else if(temp2 >= 0x40 && temp2 <= 0x5F){ /* Preamble Address Codes - Row 1*/
							userData.mainCommand = PREAMBLEADDRESSCODE;
							userData.level1Event = ROW1;
							userData.level2Event = am_get_DCevent(temp2);
							userData.channel = (temp1 == 0x11 ? CC_CHANNEL1 : CC_CHANNEL2);
							userData.buf1 = temp1;//0;
							userData.buf2 = temp2;//0;
							CC_DECODE_DBG("\nPAC-Row1  \n" );
						} else if(temp2 >= 0x60 && temp2 <= 0x7F){ /* Preamble Address Codes - Row 2 */
							userData.mainCommand = PREAMBLEADDRESSCODE;
							userData.level1Event = ROW2;
							userData.level2Event = am_get_DCevent(temp2);
							userData.channel = (temp1 == 0x11 ? CC_CHANNEL1 : CC_CHANNEL2);
							userData.buf1 = temp1;//0;
							userData.buf2 = temp2;//0;
							CC_DECODE_DBG("\nPAC-Row2  \n" );
						} else {
							CC_DECODE_DBG("\nIllegal Codes");
							userData.mainCommand = ILLEGALCODE;
							userData.level1Event = ILLEGALCODE;
							userData.level2Event = ILLEGALCODE;
							userData.channel = ILLEGALCHANNEL;
							userData.buf1 = 0;
							userData.buf2 = 0;
						}
						break;

					case 0x12: //CH1
					case 0x1A: //CH2
						if (temp2 >= 0x20 && temp2 <= 0x3F){
							/*
							 * Charatcer Set
							 * Spanish  0x20 - 0x27
							 * Misc     0x28 - 0x2F
							 * French   0x30 - 0x3F
							 */
							userData.mainCommand = PRINTABLETEXT;
							userData.level1Event = SPECIALPRINTABLETEXT;
							userData.level2Event = IGNORECODE;
							userData.channel = (temp1 == 0x12 ? CC_CHANNEL1 : CC_CHANNEL2);
							userData.buf1 = am_get_spanishfrenchcharacter(temp2)| 0x8000;
							userData.buf2 = 0;
						        CC_DECODE_DBG("\nText[%c->0x%x]",(int)userData.buf1,temp2);
						} else if(temp2 >= 0x40 && temp2 <= 0x5F){ /* Preamble Address Codes - Row 3*/
							userData.mainCommand = PREAMBLEADDRESSCODE;
							userData.level1Event = ROW3;
							userData.level2Event = am_get_DCevent(temp2);
							userData.channel = (temp1 == 0x12 ? CC_CHANNEL1 : CC_CHANNEL2);
							userData.buf1 = temp1;//0;
							userData.buf2 = temp2;//0;
							CC_DECODE_DBG("\nPAC-Row3  \n" );
						} else if(temp2 >= 0x60 && temp2 <= 0x7F){ /* Preamble Address Codes - Row 4 */
							userData.mainCommand = PREAMBLEADDRESSCODE;
							userData.level1Event = ROW4;
							userData.level2Event = am_get_DCevent(temp2);
							userData.channel = (temp1 == 0x12 ? CC_CHANNEL1 : CC_CHANNEL2);
							userData.buf1 = temp1;//0;
							userData.buf2 = temp2;//0;
							CC_DECODE_DBG("\nPAC-Row4  \n" );
						} else {
							CC_DECODE_DBG("\nIllegal Codes");
							userData.mainCommand = ILLEGALCODE;
							userData.level1Event = ILLEGALCODE;
							userData.level2Event = ILLEGALCODE;
							userData.channel = ILLEGALCHANNEL;
							userData.buf1 = 0;
							userData.buf2 = 0;
						}
						break;

					case 0x13: //CH1
					case 0x1B: //CH2
						if(temp2 >= 0x20 && temp2 <= 0x3F){
							/*
							 * Character Set:
							 * Portugese 0x20 - 0x2F
							 * German    0x30 - 0x37
							 * Danish    0x38 - 0x3F
							 */
							userData.mainCommand = PRINTABLETEXT;
							userData.level1Event = SPECIALPRINTABLETEXT;
							userData.level2Event = IGNORECODE;
							userData.channel = (temp1 == 0x13 ? CC_CHANNEL1 : CC_CHANNEL2);
							userData.buf1 = am_get_PGDcharacter(temp2)|0x8000;
							userData.buf2 = 0;
							CC_DECODE_DBG("\nText[%c->0x%x]",(int)userData.buf1,temp2);
						} else if(temp2 >= 0x40 && temp2 <= 0x5F){ /* Preamble Address Codes - Row 12*/
							userData.mainCommand = PREAMBLEADDRESSCODE;
							userData.level1Event = ROW12;
							userData.level2Event = am_get_DCevent(temp2);
							userData.channel = (temp1 == 0x13 ? CC_CHANNEL1 : CC_CHANNEL2);
							userData.buf1 = temp1;//0;
							userData.buf2 = temp2;//0;
							CC_DECODE_DBG("\nPAC-Row12  \n" );
						} else if(temp2 >= 0x60 && temp2 <= 0x7F){ /* Preamble Address Codes - Row 13 */
							userData.mainCommand = PREAMBLEADDRESSCODE;
							userData.level1Event = ROW13;
							userData.level2Event = am_get_DCevent(temp2);
							userData.channel = (temp1 == 0x13 ? CC_CHANNEL1 : CC_CHANNEL2);
							userData.buf1 = 0;
							userData.buf2 = 0;
							CC_DECODE_DBG("\nPAC-Row13  \n" );
						} else {
							CC_DECODE_DBG("\nIllegal Codes");
							userData.mainCommand = ILLEGALCODE;
							userData.level1Event = ILLEGALCODE;
							userData.level2Event = ILLEGALCODE;
							userData.channel = ILLEGALCHANNEL;
							userData.buf1 = 0;
							userData.buf2 = 0;
						}
						break;

					case 0x14: //CH1
					case 0x1C: //CH2
						if ( temp2>=0x20 && temp2<=0x2F ){ /* Misc Control Codes - Line 21 */
							userData.mainCommand = MISCCONTROLCODE;
							userData.level1Event = PRIMARYLINE21CODE;
							userData.channel = (temp1 == 0x14 ? CC_CHANNEL1 : CC_CHANNEL2);
							userData.buf1 = 0;
							userData.buf2= 0;

							switch(temp2){
								case 0x20:
									CC_DECODE_DBG("\nMCC  - resumeCaptionLoading" );
									userData.level2Event = RESUMECAPTIONLOADING;
									break;
								case 0x21:
									CC_DECODE_DBG("\nMCC  - backspace" );
									userData.level2Event = BACKSPACE;
									break;
								case 0x24:
									CC_DECODE_DBG("\nMCC  - delteToEndOfRow" );
									userData.level2Event=DELETETOEOR;
									break;
								case 0x25:
									CC_DECODE_DBG("\nMCC  - rollUp2rows" );
									userData.level2Event=ROLLUP2ROWS;
									break;
								case 0x26:
									CC_DECODE_DBG("\nMCC  - rollUp3rows" );
									userData.level2Event=ROLLUP3ROWS;
									break;
								case 0x27:
									CC_DECODE_DBG("\nMCC  - rollUp4rows" );
									userData.level2Event=ROLLUP4ROWS;
									break;
								case 0x28:
									CC_DECODE_DBG("\nMCC  - flashOn" );
									userData.level2Event=FLASHON;
									break;
								case 0x29:
									CC_DECODE_DBG("\nMCC  - resumeDirectCaptioning" );
									userData.level2Event=RESUMEDIRECTCAPTIONING;
									break;
								case 0x2A:
									CC_DECODE_DBG("\nMCC  - textRestart" );
									userData.level2Event=TEXTRESTART;
									break;
								case 0x2B:
									CC_DECODE_DBG("\nMCC  - resumeTextDisplay" );
									userData.level2Event=RESUMETEXTDISPLAY;
									break;
								case 0x2C:
									CC_DECODE_DBG("\nMCC  - eraseDisplayedMemory" );
									userData.level2Event=ERASEDISPLAYEDMEMORY;
									break;
								case 0x2D:
									CC_DECODE_DBG("\nMCC  - carriageReturn" );
									userData.level2Event=CARRIAGERETURN;
									break;
								case 0x2E:
									CC_DECODE_DBG("\nMCC  - eraseNonDisplayedMemory" );
									userData.level2Event=ERASENONDISPLAYEDMEMORY;
									break;
								case 0x2F:
									CC_DECODE_DBG("\nMCC  - endOfCaption" );
									userData.level2Event=ENDOFCAPTION;
									break;
								default:
									CC_DECODE_DBG("\nMCCLine21- ILLEGALCODE");
									userData.mainCommand = ILLEGALCODE;
									userData.level1Event = ILLEGALCODE;
									userData.level2Event = ILLEGALCODE;
									userData.channel = ILLEGALCHANNEL;
									break;
							}
						} else if(temp2 >= 0x40 && temp2 <= 0x5F){ /* Preamble Address Codes - Row 14*/
							userData.mainCommand = PREAMBLEADDRESSCODE;
							userData.level1Event = ROW14;
							userData.level2Event = am_get_DCevent(temp2);
							userData.channel = (temp1 == 0x14 ? CC_CHANNEL1 : CC_CHANNEL2);
							userData.buf1 = 0;
							userData.buf2 = 0;
							CC_DECODE_DBG("\nPAC-Row14  \n" );
						} else if(temp2 >= 0x60 && temp2 <= 0x7F){ /* Preamble Address Codes - Row 15 */
							userData.mainCommand = PREAMBLEADDRESSCODE;
							userData.level1Event = ROW15;
							userData.level2Event = am_get_DCevent(temp2);
							userData.channel = (temp1 == 0x14 ? CC_CHANNEL1 : CC_CHANNEL2);
							userData.buf1 = temp1;//0;
							userData.buf2 = temp2;//0;
							CC_DECODE_DBG("\nPAC-Row15  \n" );
						} else {
							CC_DECODE_DBG("\nIllegal Codes");
							userData.mainCommand = ILLEGALCODE;
							userData.level1Event = ILLEGALCODE;
							userData.level2Event = ILLEGALCODE;
							userData.channel = ILLEGALCHANNEL;
							userData.buf1 = 0;
							userData.buf2 = 0;
						}
						break;

					case 0x15: //CH1
					case 0x1D: //CH2
						if ( temp2>=0x20 && temp2<=0x2F){ /* Misc Control Codes - Line284 */
							userData.mainCommand = MISCCONTROLCODE;
							userData.level1Event = SECONDARYLINE21CODE;
							userData.channel = (temp1 == 0x15 ? CC_CHANNEL1 : CC_CHANNEL2);
							userData.buf1 = 0;
							userData.buf2= 0;

							switch( temp2 ){
								if(cc_country == CMD_SET_COUNTRY_KOREA){
							 		am_process_miscctrcmd_4_korea(temp2,&userData);
								} else {
									case 0x20:
										CC_DECODE_DBG("\nMCC  - resumeCaptionLoading" );
										userData.level2Event = RESUMECAPTIONLOADING;
										break;
									case 0x21:
										CC_DECODE_DBG("\nMCC  - backspace" );
										userData.level2Event = BACKSPACE;
										break;
									case 0x24:
										CC_DECODE_DBG("\nMCC  - delteToEndOfRow" );
										userData.level2Event=DELETETOEOR;
										break;
									case 0x25:
										CC_DECODE_DBG("\nMCC  - rollUp2rows" );
										userData.level2Event=ROLLUP2ROWS;
										break;
									case 0x26:
										CC_DECODE_DBG("\nMCC  - rollUp3rows" );
										userData.level2Event=ROLLUP3ROWS;
										break;
									case 0x27:
										CC_DECODE_DBG("\nMCC  - rollUp4rows" );
										userData.level2Event=ROLLUP4ROWS;
										break;
									case 0x28:
										CC_DECODE_DBG("\nMCC  - flashOn" );
										userData.level2Event=FLASHON;
										break;
									case 0x29:
										CC_DECODE_DBG("\nMCC  - resumeDirectCaptioning" );
										userData.level2Event=RESUMEDIRECTCAPTIONING;
										break;
									case 0x2A:
										CC_DECODE_DBG("\nMCC  - textRestart" );
										userData.level2Event=TEXTRESTART;
										break;
									case 0x2B:
										CC_DECODE_DBG("\nMCC  - resumeTextDisplay" );
										userData.level2Event=RESUMETEXTDISPLAY;
										break;
									case 0x2C:
										CC_DECODE_DBG("\nMCC  - eraseDisplayedMemory" );
										userData.level2Event=ERASEDISPLAYEDMEMORY;
										break;
									case 0x2D:
										CC_DECODE_DBG("\nMCC  - carriageReturn" );
										userData.level2Event=CARRIAGERETURN;
										break;
									case 0x2E:
										CC_DECODE_DBG("\nMCC- eraseNonDisplayedMemory");
										userData.level2Event=ERASENONDISPLAYEDMEMORY;
										break;
									case 0x2F:
										CC_DECODE_DBG("\nMCC- endOfCaption");
										userData.level2Event=ENDOFCAPTION;
										break;
								}

								default:
								CC_DECODE_DBG("\nMCC- ILLEGALCODE");
								userData.mainCommand = ILLEGALCODE;
								userData.level1Event = ILLEGALCODE;
								userData.level2Event = ILLEGALCODE;
								userData.channel = ILLEGALCHANNEL;
								break;
							}
						} else if(temp2 >= 0x40 && temp2 <= 0x5F) { /* Preamble Address Codes - Row 5*/
							userData.mainCommand = PREAMBLEADDRESSCODE;
							userData.level1Event = ROW5;
							userData.level2Event = am_get_DCevent(temp2);
							userData.channel = (temp1 == 0x15 ? CC_CHANNEL1 : CC_CHANNEL2);
							userData.buf1 = temp1;//0;
							userData.buf2 = temp2;//0;
							CC_DECODE_DBG("\nPAC-Row5  \n" );
						} else if(temp2 >= 0x60 && temp2 <= 0x7F) { /* Preamble Address Codes - Row 6 */
							userData.mainCommand = PREAMBLEADDRESSCODE;
							userData.level1Event = ROW6;
							userData.level2Event = am_get_DCevent(temp2);
							userData.channel = (temp1 == 0x15 ? CC_CHANNEL1 : CC_CHANNEL2);
							userData.buf1 = temp1;//0;
							userData.buf2 = temp2;//0;
							CC_DECODE_DBG("\nPAC-Row6  \n" );
						} else {
							CC_DECODE_DBG("\nIllegal Codes");
							userData.mainCommand = ILLEGALCODE;
							userData.level1Event = ILLEGALCODE;
							userData.level2Event = ILLEGALCODE;
							userData.channel = ILLEGALCHANNEL;
							userData.buf1 = 0;
							userData.buf2 = 0;
						}
						break;

					case 0x16: //CH1
					case 0x1E: //CH2
						if (temp2 >= 0x40 && temp2 <= 0x5F){ /* Preamble Address Codes - Row 7*/
							userData.mainCommand = PREAMBLEADDRESSCODE;
							userData.level1Event = ROW7;
							userData.level2Event = am_get_DCevent(temp2);
							userData.channel = (temp1 == 0x16 ? CC_CHANNEL1 : CC_CHANNEL2);
							userData.buf1 = temp1;//0;
							userData.buf2 = temp2;//0;
							CC_DECODE_DBG("\nPAC-Row7  \n" );
						} else if(temp2 >= 0x60 && temp2 <= 0x7F){ /* Preamble Address Codes - Row 8 */
							userData.mainCommand = PREAMBLEADDRESSCODE;
							userData.level1Event = ROW8;
							userData.level2Event = am_get_DCevent(temp2);
							userData.channel = (temp1 == 0x16 ? CC_CHANNEL1 : CC_CHANNEL2);
							userData.buf1 = temp1;//0;
							userData.buf2 = temp2;//0;
							CC_DECODE_DBG("\nPAC-Row8  \n" );
						} else {
							CC_DECODE_DBG("\nIllegal Codes");
							userData.mainCommand = ILLEGALCODE;
							userData.level1Event = ILLEGALCODE;
							userData.level2Event = ILLEGALCODE;
							userData.channel = ILLEGALCHANNEL;
							userData.buf1 = 0;
							userData.buf2 = 0;
						}
						break;

					case 0x17: //CH1
					case 0x1F: //CH2
						if (temp2 >= 0x21 && temp2 <= 0x23){ /* Misc Control Codes */
							userData.mainCommand = MISCCONTROLCODE;
							userData.level1Event = IGNORECODE;
							userData.channel = (temp1 == 0x17 ? CC_CHANNEL1 : CC_CHANNEL2);
							userData.buf1 = 0;
							userData.buf2= 0;

							switch(temp2){
								case 0x21:
									userData.level2Event = TABOFFSET1;
									CC_DECODE_DBG("\nMCC - TabOffset1  " );
									break;
								case 0x22:
									userData.level2Event = TABOFFSET2;
									CC_DECODE_DBG("\nMCC - TabOffset2  " );
									break;
								case 0x23:
									userData.level2Event = TABOFFSET3;
									CC_DECODE_DBG("\nMCC - TabOffset3  " );
									break;
								default:
									CC_DECODE_DBG("\nIllegal Codes");
									userData.mainCommand = ILLEGALCODE;
									userData.level1Event = ILLEGALCODE;
									userData.level2Event = ILLEGALCODE;
									userData.channel = ILLEGALCHANNEL;
									break;
							}
						} else if (temp2 >= 0x24 && temp2 <= 0x2F){ /* Ext Control Codes */
							userData.mainCommand = EXTENDEDCODE;
							userData.level1Event = IGNORECODE;
							userData.channel = (temp1 == 0x17 ? CC_CHANNEL1 : CC_CHANNEL2);
							userData.buf1 = 0;
							userData.buf2 = 0;

							switch( temp2 ){
								case 0x2D:
									userData.level2Event = BGTRANSPARENT;
									CC_DECODE_DBG("\nExtCode :bgTransparent  " );
									break;
								case 0x2E:
									userData.level2Event = FGBLACK;
									CC_DECODE_DBG("\nExtCode :fgBlack  " );
									break;
								case 0x2F:
									userData.level2Event = FGBLACKUNDERLINE;
									CC_DECODE_DBG("\nExtCode :fgBlackUnderline  " );
									break;
								default:
									CC_DECODE_DBG("\nIllegal Codes");
									userData.mainCommand = ILLEGALCODE;
									userData.level1Event = ILLEGALCODE;
									userData.level2Event = ILLEGALCODE;
									userData.channel = ILLEGALCHANNEL;
									break;
							}
						} else if(temp2 >= 0x40 && temp2 <= 0x5F){ /* Preamble Address Codes - Row 9*/
							userData.mainCommand = PREAMBLEADDRESSCODE;
							userData.level1Event = ROW9;
							userData.level2Event = am_get_DCevent(temp2);
							userData.channel = (temp1 == 0x17 ? CC_CHANNEL1 : CC_CHANNEL2);
							userData.buf1 = temp1;//0;
							userData.buf2 = temp2;//0;
							CC_DECODE_DBG("\nPAC-Row9  \n" );
						} else if(temp2 >= 0x60 && temp2 <= 0x7F){ /* Preamble Address Codes - Row 10 */
							userData.mainCommand = PREAMBLEADDRESSCODE;
							userData.level1Event = ROW10;
							userData.level2Event = am_get_DCevent(temp2);
							userData.channel = (temp1 == 0x17 ? CC_CHANNEL1 : CC_CHANNEL2);
							userData.buf1 = temp1;//0;
							userData.buf2 = temp2;//0;
							CC_DECODE_DBG("\nPAC-Row10  \n" );
						} else {
							CC_DECODE_DBG("\nIllegal Codes");
							userData.mainCommand = ILLEGALCODE;
							userData.level1Event = ILLEGALCODE;
							userData.level2Event = ILLEGALCODE;
							userData.channel = ILLEGALCHANNEL;
							userData.buf1 = 0;
							userData.buf2 = 0;
						}
						break;

					default:
						CC_DECODE_DBG("\nIllegal Codes");
						userData.mainCommand = ILLEGALCODE;
						userData.level1Event = ILLEGALCODE;
						userData.level2Event = ILLEGALCODE;
						userData.channel = ILLEGALCHANNEL;
						userData.buf1 = 0;
						userData.buf2 = 0;
						break;
				}//End Main switch

				userData.serviceType = currentServiceType;
			}
			else
				//XDS
				if ( ((temp1 >= 0x20 && temp1 <= 0x7F) || (temp1 == 0x00)) &&
						((temp2 >= 0x20 && temp2 <= 0x7F) || (temp2 == 0x00)) &&
						(  filterF1XDS ) )
				{
					CC_DECODE_DBG("\n==========>xdsControlCode :raw data :0x%x 0x%x  cc data :0x%x 0x%x\n", (int)data1, (int)data2,temp1,temp2);
					err = AM_Decode_XDSData(temp1,temp2);

					userData.mainCommand = XDSDATA;
					userData.level1Event = CCEVENT_RESERVED;
					userData.level2Event = CCEVENT_RESERVED;
					userData.channel = ILLEGALCHANNEL;
					userData.buf1 = temp1;
					userData.buf2 = temp2;

					return err;
				}
				else

					if ( (temp1>=0x20 && temp1<=0x7F) || (temp2>=0x20 && temp2<=0x7F) ){
						/*
						 * LINE21 CLOSED CAPTIONS
						 * These text charatcers belong to CC since StartCaptureXDS = FALSE.
						 * This flag shows that our previous characters were not XDS control
						 * codes and so we are still in CC Mode.
						 * */

						/* Proceed further for decoding on the condition that the current
						 * field is 1 and caption selected is primaryLine21 service
						 * OR
						 * field is 2 and caption selected is secondary language caption.
						 */
						CC_DECODE_DBG("\n[%s:%d]===================>\n",__FUNCTION__,__LINE__ );
						if(vbi_has_no_xds > 30)
						{
							vbi_has_no_xds =0;
							AM_Clr_Vchip_Event();
							am_ntsc_vchip_callback();
						}

						if(currentServiceType != PRIMARYLINE21DATASERVICE1 &&
								currentServiceType != PRIMARYLINE21DATASERVICE2 &&
								currentServiceType != SECONDARYLINE21DATASERVICE1 &&
								currentServiceType != SECONDARYLINE21DATASERVICE2)
							return err;

						/* Printable Text */
						userData.mainCommand = PRINTABLETEXT;
						userData.level1Event = IGNORECODE;
						userData.level2Event = IGNORECODE;
						userData.channel = curCCDecodeChannel; 

						/*
						 * Handle Normal and Special Character cases
						 * for 608 also.
						 */
						if(cc_country == CMD_SET_COUNTRY_KOREA){
							if (n_lines == 284)
							{
								if (temp1 >= 0x20 && temp1 <= 0x7F)
								{
									userData.buf1 = temp1 - 0x20 + 0xA0;
									userData.buf2 = temp2 + 0x80;
								}
							}
						} else {

							if (temp1>=0x20 && temp1<=0x7F){
								userData.buf1 = am_get_608characters(temp1);
							} else{
								userData.buf1 = 0xFF;//means no sense byte and no need send to APP
							}

							if (temp2 >= 0x20 && temp2 <= 0x7F){
								userData.buf2 = am_get_608characters(temp2);
							} else{
								userData.buf2  = 0xFF;//means no sense byte and no need send to APP
							}
						}

						userData.serviceType = currentServiceType;
						CC_DECODE_DBG("\nText[%c%c] CH%d\n ",(int)temp1,(int)temp2, (int) userData.channel);
					} else { /* Some illegal / Unknown characters */
						userData.mainCommand = ILLEGALCODE;
						userData.level1Event = ILLEGALCODE;
						userData.level2Event = ILLEGALCODE;
						userData.channel = ILLEGALCHANNEL;
						userData.buf1 = temp1;
						userData.buf2 = temp2;
						userData.serviceType = currentServiceType;
						CC_DECODE_DBG("\nN/A[0x%X,0x%X] chn:%d ",(int)temp1,(int)temp2, (int) userData.channel);
					}

		if (currentServiceType != PRIMARYLINE21DATASERVICE1 &&
				currentServiceType != PRIMARYLINE21DATASERVICE2 &&
				currentServiceType != SECONDARYLINE21DATASERVICE1 &&
				currentServiceType != SECONDARYLINE21DATASERVICE2)
			return err;

		if ( userData.mainCommand!=PRINTABLETEXT && userData.channel!=ILLEGALCHANNEL ){ 
			curCCDecodeChannel = userData.channel;
			CC_DECODE_DBG("\ncurCC" );
		}

 		if(userData.mainCommand != PRINTABLETEXT)
		{
			if(currentServiceType == PRIMARYLINE21DATASERVICE1 && userData.channel != CC_CHANNEL1)
				return AM_SUCCESS;

			if(currentServiceType == PRIMARYLINE21DATASERVICE2 && userData.channel != CC_CHANNEL2)
				return AM_SUCCESS;

			if(currentServiceType == SECONDARYLINE21DATASERVICE1 && userData.channel != CC_CHANNEL1)
				return AM_SUCCESS;

			if(currentServiceType == SECONDARYLINE21DATASERVICE2 && userData.channel != CC_CHANNEL2)
				return AM_SUCCESS;
		}
		if( userData.mainCommand != ILLEGALCODE)
			am_process_ntsc_cc_command(userData);

#endif
		return err;    
	}
