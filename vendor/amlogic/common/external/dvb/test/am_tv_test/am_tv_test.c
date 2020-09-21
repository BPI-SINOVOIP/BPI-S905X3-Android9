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
/**\file am_tv_test.c
 * \brief TV模块测试程序
 *
 * \author Xia Lei Peng <leipeng.xia@amlogic.com>
 * \date 2010-11-12: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 1
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include <string.h>
#include <assert.h>
#include <am_debug.h>
#include <am_fend.h>
#include <am_fend_ctrl.h>
#include <am_dmx.h>
#include <am_db.h>
#include <am_av.h>
#include <am_tv.h>
#include <am_scan.h>
#include <am_epg.h>

#include "am_caman.h"
#include "am_ci.h"
#include "ca_ci.h"

/****************************************************************************
 * Macro definitions
 ***************************************************************************/
#define FEND_DEV_NO 0
#define AV_DEV_NO 0
#define DMX_DEV_NO 0

#define MAX_PROG_CNT 300
#define MAX_NAME_LEN 64

/****************************************************************************
 * Type definitions
 ***************************************************************************/


enum
{
	MENU_TYPE_MENU,
	MENU_TYPE_APP
};


typedef struct 
{
	const int	id;
	const int	parent;
	const char	*title;
	const char	*cmd;
	const int	type;
	void (*app)(void);
	void (*leave)(void);
	void (*input)(const char *cmd);
}CommandMenu;



static CommandMenu *curr_menu;
static AM_FENDCTRL_DVBFrontendParameters_t fend_param;
static AM_Bool_t go = AM_TRUE;
static AM_SCAN_Handle_t hscan;
static AM_TV_Handle_t htv;
static AM_EPG_Handle_t hepg;
static int chan_num[MAX_PROG_CNT];
static char programs[MAX_PROG_CNT][MAX_NAME_LEN];

static const char *weeks[]=
{
	"星期天",
	"星期一",
	"星期二",
	"星期三",
	"星期四",
	"星期五",
	"星期六",
};
	
/****************************************************************************
 * static Functions
 ***************************************************************************/
static void show_main_menu(void);


static void epg_evt_call_back(long dev_no, int event_type, void *param, void *user_data)
{
	char *errmsg = NULL;
	sqlite3 *hdb;

	switch (event_type)
	{
		case AM_EPG_EVT_NEW_EIT:
		{
			dvbpsi_eit_t *eit = (dvbpsi_eit_t*)param;
			dvbpsi_eit_event_t *event;
			dvbpsi_descriptor_t *descr;
			char name[64+1];
			char sql[256];
			int row = 1;
			int srv_dbid, net_dbid, ts_dbid;
			int start, end;
			uint16_t mjd;
			uint8_t hour, min, sec;
			
			AM_DEBUG(2, "got event AM_EPG_EVT_NEW_EIT ");
			AM_DEBUG(2, "Service id 0x%x:", eit->i_service_id);

			AM_DB_HANDLE_PREPARE(hdb);

			/*查询service*/
			snprintf(sql, sizeof(sql), "select db_net_id,db_ts_id,db_id from srv_table where db_ts_id=\
										(select db_id from ts_table where db_net_id=\
										(select db_id from net_table where network_id='%d') and ts_id='%d')\
										and service_id='%d' limit 1", eit->i_network_id, eit->i_ts_id, eit->i_service_id);
			if (AM_DB_Select(hdb, sql, &row, "%d,%d,%d", &net_dbid, &ts_dbid, &srv_dbid) != AM_SUCCESS || row == 0)
			{
				/*No such service*/
				break;
			}

			AM_SI_LIST_BEGIN(eit->p_first_event, event)
				row = 1;
				/*查找该事件是否已经被添加*/
				snprintf(sql, sizeof(sql), "select db_id from evt_table where db_srv_id='%d' \
											and event_id='%d'", srv_dbid, event->i_event_id);
				if (AM_DB_Select(hdb, sql, &row, "%d", &srv_dbid) == AM_SUCCESS && row > 0)
				{
					continue;
				}
				name[0] = 0;

				/*取UTC时间*/
				mjd = (uint16_t)(event->i_start_time >> 24);
				hour = (uint8_t)(event->i_start_time >> 16);
				min = (uint8_t)(event->i_start_time >> 8);
				sec = (uint8_t)event->i_start_time;
				start = AM_EPG_MJD2SEC(mjd) + AM_EPG_BCD2SEC(hour, min, sec);
				/*取持续事件*/
				hour = (uint8_t)(event->i_duration >> 16);
				min = (uint8_t)(event->i_duration >> 8);
				sec = (uint8_t)event->i_duration;
				end = AM_EPG_BCD2SEC(hour, min, sec);

				end += start;
				
				AM_SI_LIST_BEGIN(event->p_first_descriptor, descr)
					if (descr->i_tag == AM_SI_DESCR_SHORT_EVENT && descr->p_decoded)
					{
						dvbpsi_short_event_dr_t *pse = (dvbpsi_short_event_dr_t*)descr->p_decoded;

						AM_SI_ConvertDVBTextCode((char*)pse->i_event_name, pse->i_event_name_length,\
										name, 64);
						name[64] = 0;
						AM_DEBUG(2, "event_id 0x%x, name '%s'", event->i_event_id, name);
						
						break;
					}
				AM_SI_LIST_END()
				snprintf(sql, sizeof(sql), "insert into evt_table(db_net_id, db_ts_id, db_srv_id, event_id, name, start, end) \
											values('%d','%d','%d','%d','%s','%d','%d')", net_dbid, ts_dbid, srv_dbid, event->i_event_id,\
											name, start, end);
				if (sqlite3_exec(hdb, sql, NULL, NULL, &errmsg) != SQLITE_OK)
					AM_DEBUG(1, "%s", errmsg);
				
			AM_SI_LIST_END()
			break;
		}
		default:
			break;
	}
}

static void display_pf_info(void)
{
	char evt[64];
	char sql[200];
	int srv_dbid;
	int now, start, end, lnow;
	int row = 1;
	int chan_num;
	struct tm stim, etim;
	sqlite3 *hdb;

	if (AM_TV_GetCurrentChannel(htv, &srv_dbid) != AM_SUCCESS)
		return;
		
	AM_EPG_GetUTCTime(&now);

	printf("------------------------------------------------------------------------------------\n");

	AM_DB_HANDLE_PREPARE(hdb);

	/*频道号，频道名称*/
	snprintf(sql, sizeof(sql), "select chan_num,name from srv_table where db_id='%d'", srv_dbid);
	if (AM_DB_Select(hdb, sql, &row, "%d,%s:64", &chan_num, evt) == AM_SUCCESS && row > 0)
	{
		printf("  %03d  %s\t\t|", chan_num, evt);
	}
	
	/*当前事件信息*/
	snprintf(sql, sizeof(sql), "select name,start,end from evt_table where db_srv_id='%d' and \
								start<='%d' and end>='%d'", srv_dbid, now, now);
	if (AM_DB_Select(hdb, sql, &row, "%s:64,%d,%d", evt, &start, &end) == AM_SUCCESS && row > 0)
	{
		AM_EPG_UTCToLocal(start, &start);
		AM_EPG_UTCToLocal(end, &end);
		gmtime_r((time_t*)&start, &stim);
		gmtime_r((time_t*)&end, &etim);
		printf("\t%02d:%02d - %02d:%02d  %s", stim.tm_hour, stim.tm_min, etim.tm_hour, etim.tm_min, evt);
	}
	
	/*当前时间*/
	AM_EPG_UTCToLocal(now, &lnow);
	gmtime_r((time_t*)&lnow, &stim);
	printf("\n%d-%02d-%02d %02d:%02d\t|", 
						   stim.tm_year + 1900, stim.tm_mon+1,
						   stim.tm_mday, stim.tm_hour,
						   stim.tm_min);	
	/*下一事件信息*/
	snprintf(sql, sizeof(sql), "select name,start,end from evt_table where db_srv_id='%d' and \
								start>='%d' order by start limit 1", srv_dbid, now);
	if (AM_DB_Select(hdb, sql, &row, "%s:64,%d,%d", evt, &start, &end) == AM_SUCCESS && row > 0)
	{
		AM_EPG_UTCToLocal(start, &start);
		AM_EPG_UTCToLocal(end, &end);
		gmtime_r((time_t*)&start, &stim);
		gmtime_r((time_t*)&end, &etim);
		printf("\t%02d:%02d - %02d:%02d  %s", stim.tm_hour, stim.tm_min, etim.tm_hour, etim.tm_min, evt);
	}
	printf("\n------------------------------------------------------------------------------------\n");
}

static void display_detail_epg(int day)
{
	static char aevt[200][64];
	static int st[64];
	static int et[64];
	char sql[200];
	int srv_dbid;
	int now, start, end, lnow;
	int row = 1;
	struct tm stim, etim;
	sqlite3 *hdb;

	if (day < 0)
		return ;
		
	if (AM_TV_GetCurrentChannel(htv, &srv_dbid) != AM_SUCCESS)
		return;

	AM_DB_HANDLE_PREPARE(hdb);

	AM_EPG_GetUTCTime(&now);

	AM_EPG_UTCToLocal(now, &lnow);
	gmtime_r((time_t*)&lnow, &stim);
	/*计算今天零点时间*/
	lnow -= stim.tm_hour*3600 + stim.tm_min*60 + stim.tm_sec;

	/*计算需要显示的那一天的起始时间和结束时间*/
	start = lnow + day * 3600 * 24;
	end = start + 3600 * 24;

	gmtime_r((time_t*)&start, &stim);

	printf("------------------------------------------------------------------------------------\n");
	printf("\t%d-%02d-%02d %s\n", 
						   stim.tm_year + 1900, stim.tm_mon+1,
						   stim.tm_mday,weeks[stim.tm_wday]);	
	printf("------------------------------------------------------------------------------------\n");
						   
	AM_EPG_LocalToUTC(start, &start);
	AM_EPG_LocalToUTC(end, &end);
	
	/*取事件信息*/
	snprintf(sql, sizeof(sql), "select name,start,end from evt_table where db_srv_id='%d' and \
								start>='%d' and start<='%d' order by start", srv_dbid, start, end);
	row = 200;
	if (AM_DB_Select(hdb, sql, &row, "%s:64,%d,%d", aevt, st, et) == AM_SUCCESS && row > 0)
	{
		int i;

		for (i=0; i<row; i++)
		{
			AM_EPG_UTCToLocal(st[i], &st[i]);
			AM_EPG_UTCToLocal(et[i], &et[i]);
			gmtime_r((time_t*)&st[i], &stim);
			gmtime_r((time_t*)&et[i], &etim);
			printf("\t%02d:%02d - %02d:%02d  %s\n", stim.tm_hour, stim.tm_min, etim.tm_hour, etim.tm_min, aevt[i]);
		}
	}

	printf("------------------------------------------------------------------------------------\n");
	printf("Input '0' '1' '2' '3' '4' '5' '6' to view epg offset today\n");
	printf("------------------------------------------------------------------------------------\n");
}

static void auto_scan_input(const char *cmd)
{

}

static void manual_scan_input(const char *cmd)
{

}

static void tv_list_input(const char *cmd)
{
	if(!strncmp(cmd, "u", 1))
	{
		AM_TV_ChannelUp(htv);
	}
	else if(!strncmp(cmd, "d", 1))
	{
		AM_TV_ChannelDown(htv);
	}
	else if(!strncmp(cmd, "n", 1))
	{
		int num;

		sscanf(cmd+1, "%d", &num);
		AM_TV_PlayChannel(htv, num);
	}
	else if (!strncmp(cmd, "radio", 5))
	{
		
	}

	display_pf_info();
}

static void radio_list_input(const char *cmd)
{
	if(!strncmp(cmd, "u", 1))
	{
		AM_TV_ChannelUp(htv);
	}
	else if(!strncmp(cmd, "d", 1))
	{
		AM_TV_ChannelDown(htv);
	}
	else if(!strncmp(cmd, "n", 1))
	{
		int num;

		sscanf(cmd+1, "%d", &num);
		AM_TV_PlayChannel(htv, num);
	}
	else if (!strncmp(cmd, "tv", 2))
	{
		
	}
	display_pf_info();
}

static void freq_input(const char *cmd)
{
	
}

static void restore_input(const char *cmd)
{
	
}

static void epg_input(const char *cmd)
{
	int day;

	sscanf(cmd, "%d", &day);

	display_detail_epg(day);
}

#if 0
static void store_db_to_file(void)
{
	char *errmsg;

	if (sqlite3_exec(hdb, "attach 'database.db' as filedb", NULL, NULL, &errmsg) != SQLITE_OK)
	{
		printf("error, %s", errmsg);
		return;
	}

	/*删除原来数据*/
	sqlite3_exec(hdb, "drop table if exists filedb.net_table", NULL, NULL, NULL);
	sqlite3_exec(hdb, "drop table if exists filedb.ts_table", NULL, NULL, NULL);
	sqlite3_exec(hdb, "drop table if exists filedb.srv_table", NULL, NULL, NULL);

	/*从内存数据库加载数据到文件中*/
	if (sqlite3_exec(hdb, "create table filedb.net_table as select * from net_table", NULL, NULL, &errmsg) != SQLITE_OK)
	{
		printf("error, %s", errmsg);
	}
	if (sqlite3_exec(hdb, "create table filedb.ts_table as select * from ts_table", NULL, NULL, &errmsg) != SQLITE_OK)
	{
		printf("error, %s", errmsg);
	}
	if (sqlite3_exec(hdb, "create table filedb.srv_table as select * from srv_table", NULL, NULL, &errmsg) != SQLITE_OK)
	{
		printf("error, %s", errmsg);
	}
	
	if (sqlite3_exec(hdb, "detach filedb", NULL, NULL, &errmsg) != SQLITE_OK)
	{
		printf("error, %s", errmsg);
	}
}

static void load_db_from_file(void)
{
	char *errmsg;

	if (sqlite3_exec(hdb, "attach 'database.db' as filedb", NULL, NULL, &errmsg) != SQLITE_OK)
	{
		AM_DEBUG(1, "error, %s", errmsg);
		return;
	}

	/*从文件中加载数据到内存*/
	if (sqlite3_exec(hdb, "insert into net_table select * from filedb.net_table", NULL, NULL, &errmsg) != SQLITE_OK)
	{
		AM_DEBUG(1, "error, %s", errmsg);
	}
	if (sqlite3_exec(hdb, "insert into ts_table select * from filedb.ts_table", NULL, NULL, &errmsg) != SQLITE_OK)
	{
		AM_DEBUG(1, "error, %s", errmsg);
	}
	if (sqlite3_exec(hdb, "insert into srv_table select * from filedb.srv_table", NULL, NULL, &errmsg) != SQLITE_OK)
	{
		AM_DEBUG(1, "error, %s", errmsg);
	}
	
	if (sqlite3_exec(hdb, "detach filedb", NULL, NULL, &errmsg) != SQLITE_OK)
	{
		AM_DEBUG(1, "error, %s", errmsg);
	}
}
#endif

static void progress_evt_callback(long dev_no, int event_type, void *param, void *user_data)
{
	switch (event_type)
	{
		case AM_SCAN_EVT_PROGRESS:
		{
			AM_SCAN_Progress_t *prog = (AM_SCAN_Progress_t*)param;

			if (!prog)
				return;

			switch (prog->evt)
			{
				case AM_SCAN_PROGRESS_SCAN_END:
					printf("-------------------------------------------------------------\n");
					printf("Scan End, input 'b' back to main menu\n");
					printf("-------------------------------------------------------------\n\n");

					AM_CAMAN_Resume();

					printf("CAMAN Resumed. Start Playing..\n");

					/*播放*/
					AM_TV_Play(htv);
					
					break;
				case AM_SCAN_PROGRESS_STORE_END:
					/*保存到文件*/
					//store_db_to_file();
					break;
				default:
					//AM_DEBUG(1, "Unkonwn EVT");
					break;
			}
		}
	}

}

#ifdef TEST_CA_CI

#define MMI_STATE_CLOSED 0
#define MMI_STATE_OPEN 1
#define MMI_STATE_ENQ 2
#define MMI_STATE_MENU 3

static int mmi_state = MMI_STATE_CLOSED;
static void hex_dump(void *data, int size)
{
    /* dumps size bytes of *data to stdout. Looks like:
     * [0000] 75 6E 6B 6E 6F 77 6E 20
     *                  30 FF 00 00 00 00 39 00 unknown 0.....9.
     * (in a single line of course)
     */

    unsigned char *p = data;
    unsigned char c;
    int n;
    char bytestr[4] = {0};
    char addrstr[10] = {0};
    char hexstr[ 16*3 + 5] = {0};
    char charstr[16*1 + 5] = {0};
    for(n=1;n<=size;n++) {
        if (n%16 == 1) {
            /* store address for this line */
            snprintf(addrstr, sizeof(addrstr), "%.4x",
               ((unsigned int)p-(unsigned int)data) );
        }
            
        c = *p;
        if (isalnum(c) == 0) {
            c = '.';
        }

        /* store hex str (for left side) */
        snprintf(bytestr, sizeof(bytestr), "%02X ", *p);
        strncat(hexstr, bytestr, sizeof(hexstr)-strlen(hexstr)-1);

        /* store char str (for right side) */
        snprintf(bytestr, sizeof(bytestr), "%c", c);
        strncat(charstr, bytestr, sizeof(charstr)-strlen(charstr)-1);

        if(n%16 == 0) { 
            /* line completed */
            printf("[%4.4s]   %-50.50s  %s\n", addrstr, hexstr, charstr);
            hexstr[0] = 0;
            charstr[0] = 0;
        } else if(n%8 == 0) {
            /* half line: add whitespaces */
            strncat(hexstr, "  ", sizeof(hexstr)-strlen(hexstr)-1);
            strncat(charstr, " ", sizeof(charstr)-strlen(charstr)-1);
        }
        p++; /* next byte */
    }

    if (strlen(hexstr) > 0) {
        /* print rest of buffer if not empty */
        printf("[%4.4s]   %-50.50s  %s\n", addrstr, hexstr, charstr);
    }
}

static int caman_cb(char *name, AM_CA_Msg_t *msg)
{
	AM_ErrorCode_t err;

	if(!msg)
	{
		AM_DEBUG(1, "\t\t-->|APP ??? msg (NULL) ???");
		return -1;
	}

	if(msg->dst != AM_CA_MSG_DST_APP)
	{
		AM_DEBUG(1, "\t\t-->|APP ??? msg dst is not app(%d) ???", msg->dst);
		return -1;
	}

	if(0 == strcmp("ci0", name))
	{
		ca_ci_msg_t *cimsg = (ca_ci_msg_t *)msg->data;
		//hex_dump(msg->data, msg->len);
		switch(cimsg->type)
		{
			#define PIN(i, x) AM_DEBUG(1, "\t\t-->|"# i"("# x"):\t0x%x", x)
			#define PIS(i, x, l) AM_DEBUG(1, "\t\t-->|"# i"("# x"):\t%.*s", l, x) 
			#define PInS(i, n, x, l) AM_DEBUG(1, "\t\t-->|"# i":\t(%d):%.*s", n, l, x)
			#define PI_S(i, x, l) AM_DEBUG(1, "\t\t-->|"# i":\t%.*s", l, x)

			case ca_ci_msg_type_appinfo:
				{
					ca_ci_appinfo_t *appinfo = (ca_ci_appinfo_t*)cimsg->msg;

					#define PN(x) PIN(APPINFO, x)
					#define PS(x, n) PIS(APPINFO, x, n)

					PN(appinfo->application_type);
					PN(appinfo->application_manufacturer);
					PN(appinfo->manufacturer_code);
					PS(appinfo->menu_string, appinfo->menu_string_length);

					#undef PN
					#undef PS
				}
				break;
			case ca_ci_msg_type_cainfo:
				{
					int i;
					ca_ci_cainfo_t *cainfo = (ca_ci_cainfo_t*)cimsg->msg;
			
					for(i=0; i<cainfo->ca_id_count; i++) {
						PIN(CAINFO, i);
						PIN(CAINFO, cainfo->ca_ids[i]);
					}
				}
				break;

			case ca_ci_msg_type_mmi_close:
				mmi_state = MMI_STATE_CLOSED;
				break;

			case ca_ci_msg_type_mmi_display_control:
				mmi_state = MMI_STATE_OPEN;
				break;

			case ca_ci_msg_type_mmi_enq:
				{
					ca_ci_mmi_enq_t *enq = (ca_ci_mmi_enq_t*)cimsg->msg;

					PIN(ENQ, enq->blind_answer);
					PIN(ENQ, enq->expected_answer_length);
					PIS(ENQ, enq->text, enq->text_size);
					
					mmi_state = MMI_STATE_ENQ;
				}
				break;

			case ca_ci_msg_type_mmi_menu:
			case ca_ci_msg_type_mmi_list:
				{
					unsigned char *p = cimsg->msg;
					int l; int i, s;
					#define _getint(p) (p[0] | (p[1]<<8) | (p[2]<<16) | (p[3]<<24))
					l=_getint(p);
					PI_S(MENU:titile, &p[4], l); p+=(l+4);
					l=_getint(p);
					PI_S(MENU:subtitile, &p[4], l); p+=(l+4);
					l=_getint(p);
					PI_S(MENU:bottom, &p[4], l); p+=(l+4);
					s=_getint(p); p+=4;
					for(i=0; i<s; i++)
					{
						l=_getint(p);
						PInS(MENU:item, i+1, &p[4], l); p+=(l+4);
					}
					mmi_state = MMI_STATE_MENU;
				}
				break;

			default:
				AM_DEBUG(1, "\t\t-->|APP ??? unkown ci msg type(%d) ??? ", cimsg->type);
				break;

			#undef PIN
			#undef PIS
		}

		if((err = AM_CAMAN_freeMsg(msg)) != AM_SUCCESS)
		{
			AM_DEBUG(1, "\t\t-->|APP ??? free msg fail[%x]", err);
		}
	}
	else
	{
		AM_DEBUG(1, "\t\t-->|??? ca(%s) ???", name);
		return -1;
	}

	return 0;
}

static void run_ci(void)
{
	AM_CA_Msg_t msg;
	ca_ci_msg_t cimsg;

	cimsg.type = ca_ci_msg_type_enter_menu;
	msg.type=0;
	msg.dst = AM_CA_MSG_DST_CA;
	msg.data = (unsigned char*)&cimsg;
	msg.len = sizeof(cimsg);
	AM_CAMAN_putMsg("ci0", &msg);
}

static void leave_ci(void)
{
	AM_CA_Msg_t msg;
	int lmsg = sizeof(ca_ci_msg_t)+sizeof(ca_ci_close_mmi_t);
	ca_ci_msg_t *cimsg = malloc(lmsg);
	assert(cimsg);

	ca_ci_close_mmi_t *close_mmi = (ca_ci_close_mmi_t *)cimsg->msg;

	close_mmi->cmd_id = AM_CI_MMI_CLOSE_MMI_CMD_ID_IMMEDIATE;
	close_mmi->delay = 0;
	cimsg->type = ca_ci_msg_type_close_mmi;
	msg.type=0;
	msg.dst = AM_CA_MSG_DST_CA;
	msg.data = (unsigned char*)cimsg;
	msg.len = lmsg;
	AM_CAMAN_putMsg("ci0", &msg);
	
	free(cimsg);
}

static void ci_input(const char *cmd)
{
	AM_CA_Msg_t msg;
	ca_ci_msg_t *cimsg;
	int lmsg;

	if(0==strcmp(cmd, "u"))
	{	
		lmsg = sizeof(ca_ci_msg_t)+sizeof(ca_ci_answer_enq_t);
		cimsg = malloc(lmsg);
		assert(cimsg);
		
		ca_ci_answer_enq_t *aenq = (ca_ci_answer_enq_t *)cimsg->msg;

		aenq->answer_id = AM_CI_MMI_ANSW_CANCEL;
		aenq->size = 0;
		cimsg->type = ca_ci_msg_type_answer;
		msg.type=0;
		msg.dst = AM_CA_MSG_DST_CA;
		msg.data = (unsigned char*)cimsg;
		msg.len = lmsg;
		AM_CAMAN_putMsg("ci0", &msg);

		free(cimsg);

		return;
	}
	
	switch(mmi_state) {
		case MMI_STATE_CLOSED:
		case MMI_STATE_OPEN:
			break;

		case MMI_STATE_ENQ:
			{
				int len = strlen(cmd);
				lmsg = sizeof(ca_ci_msg_t)+sizeof(ca_ci_answer_enq_t)+len;
				cimsg = malloc(lmsg);
				assert(cimsg);
				
				ca_ci_answer_enq_t *aenq = (ca_ci_answer_enq_t *)cimsg->msg;

				if(!len){
					aenq->answer_id = AM_CI_MMI_ANSW_CANCEL;
					aenq->size = 0;
				}else{
					aenq->answer_id = AM_CI_MMI_ANSW_ANSWER;
					aenq->size = len;
					memcpy(aenq->answer, cmd, len);
				}
				cimsg->type = ca_ci_msg_type_answer;
				msg.type=0;
				msg.dst = AM_CA_MSG_DST_CA;
				msg.data = (unsigned char*)cimsg;
				msg.len = lmsg;
				AM_CAMAN_putMsg("ci0", &msg);

				free(cimsg);
			}

			mmi_state = MMI_STATE_OPEN;
			break;

		case MMI_STATE_MENU:
			{
				lmsg = sizeof(ca_ci_msg_t)+sizeof(ca_ci_answer_menu_t);
				cimsg = malloc(lmsg);
				assert(cimsg);
				
				ca_ci_answer_menu_t *amenu = (ca_ci_answer_menu_t *)cimsg->msg;

				amenu->select = atoi(cmd);
				
				cimsg->type = ca_ci_msg_type_answer_menu;
				msg.type=0;
				msg.dst = AM_CA_MSG_DST_CA;
				msg.data = (unsigned char*)cimsg;
				msg.len = lmsg;
				AM_CAMAN_putMsg("ci0", &msg);

				free(cimsg);
			}
		
			mmi_state = MMI_STATE_OPEN;
			break;
	}
	
}
#endif

static void run_quit(void)
{
	go = AM_FALSE;
}

static void run_freq_setting(void)
{
	printf("Input main frequency(Hz):");
	scanf("%u", &fend_param.terrestrial.para.frequency);
	printf("New main frequency is %u kHz\n", fend_param.terrestrial.para.frequency);
	printf("-------------------------------------------------------------\n");
	printf("Input 'b' back to main menu\n");
	printf("-------------------------------------------------------------\n\n");
}

static void run_restore(void)
{
	char asw;
	
	printf("Are you sure to restore?(y/n)\n");
	scanf("%c", &asw);

	if (asw == 'y')
	{
		sqlite3 *hdb;

		AM_DB_HANDLE_PREPARE(hdb);

		AM_TV_StopPlay(htv);
	
		AM_DEBUG(1, "Restoring...");
		sqlite3_exec(hdb, "delete from net_table", NULL, NULL, NULL);
		sqlite3_exec(hdb, "delete from ts_table", NULL, NULL, NULL);
		sqlite3_exec(hdb, "delete from srv_table", NULL, NULL, NULL);
		sqlite3_exec(hdb, "delete from evt_table", NULL, NULL, NULL);
		//store_db_to_file();
		AM_DEBUG(1, "OK");
	}
	
	printf("-------------------------------------------------------------\n");
	printf("Input 'b' back to main menu\n");
	printf("-------------------------------------------------------------\n\n");
}

static void run_auto_scan(void)
{
	AM_SCAN_CreatePara_t para;

	AM_TV_StopPlay(htv);

	AM_CAMAN_Pause();

	/*停止监控所有表*/
	AM_EPG_ChangeMode(hepg, AM_EPG_MODE_OP_REMOVE,  AM_EPG_SCAN_ALL);
	
	memset(&para, 0, sizeof(para));
	para.fend_dev_id = FEND_DEV_NO;
	para.mode = AM_SCAN_MODE_DTV_ATV;
	para.store_cb = NULL;
	para.dtv_para.standard = AM_SCAN_DTV_STD_DVB;
	para.dtv_para.dmx_dev_id = DMX_DEV_NO;
	para.dtv_para.source = FE_OFDM;
	para.dtv_para.mode = AM_SCAN_DTVMODE_ALLBAND;
	para.dtv_para.sort_method = AM_SCAN_SORT_BY_FREQ_SRV_ID;
	para.dtv_para.fe_cnt = 0;
	para.dtv_para.fe_paras = NULL;
	para.dtv_para.resort_all = 0;
	AM_SCAN_Create(&para, &hscan);
	/*注册搜索进度通知事件*/
	AM_EVT_Subscribe(hscan, AM_SCAN_EVT_PROGRESS, progress_evt_callback, NULL);

	AM_SCAN_Start(hscan);
}

static void run_manual_scan(void)
{
	AM_FENDCTRL_DVBFrontendParameters_t fend_para;
	AM_SCAN_CreatePara_t para;
	
	AM_TV_StopPlay(htv);

	AM_CAMAN_Pause();

	/*停止监控所有表*/
	AM_EPG_ChangeMode(hepg, AM_EPG_MODE_OP_REMOVE,  AM_EPG_SCAN_ALL);
	
	fend_para = fend_param;
	fend_para.m_type = FE_OFDM;
	printf("Input frequency(Hz):");
	scanf("%u", &fend_para.terrestrial.para.frequency);
	fend_para.terrestrial.para.u.ofdm.bandwidth = BANDWIDTH_8_MHZ;

	memset(&para, 0, sizeof(para));
	para.fend_dev_id = FEND_DEV_NO;
	para.mode = AM_SCAN_MODE_DTV_ATV;
	para.store_cb = NULL;

	para.atv_para.mode = AM_SCAN_ATVMODE_NONE;
	
	para.dtv_para.mode = AM_SCAN_DTVMODE_MANUAL;
	para.dtv_para.standard = AM_SCAN_DTV_STD_DVB;
	para.dtv_para.dmx_dev_id = DMX_DEV_NO;
	para.dtv_para.source = FE_OFDM;
	para.dtv_para.sort_method = AM_SCAN_SORT_BY_FREQ_SRV_ID;
	para.dtv_para.fe_cnt = 1;
	para.dtv_para.fe_paras = &fend_para;
	para.dtv_para.resort_all = 0;

	printf("-------------------<\n");
	AM_SCAN_Create(&para, &hscan);

	printf("------------------2-<\n");
	/*注册搜索进度通知事件*/
	AM_EVT_Subscribe(hscan, AM_SCAN_EVT_PROGRESS, progress_evt_callback, NULL);

	printf("------------------3-<\n");
	AM_SCAN_Start(hscan);
	printf("------------------4-<\n");
}

static void leave_scan(void)
{
	if (hscan != 0)
	{
		AM_DEBUG(1, "Destroying scan");
		AM_EVT_Unsubscribe(hscan, AM_SCAN_EVT_PROGRESS, progress_evt_callback, NULL);
		AM_SCAN_Destroy(hscan, AM_TRUE);
		hscan = 0;

		/*开启EPG监控*/
		//AM_EPG_ChangeMode(hepg, AM_EPG_MODE_OP_ADD,  AM_EPG_SCAN_ALL /*& (~AM_EPG_SCAN_EIT_SCHE_ALL)*/);
	}
}

static void run_tv_list(void)
{
	int row = MAX_PROG_CNT;
	int i;
	sqlite3 *hdb;

	AM_DB_HANDLE_PREPARE(hdb);

	/*从数据库取节目*/
	if (AM_DB_Select(hdb, "select chan_num,name from srv_table where service_type='1' order by chan_num", 
					&row, "%d,%s:64", chan_num, programs) != AM_SUCCESS)
	{
		AM_DEBUG(1, "Get program from dbase failed");
		return;
	}
	if (row == 0)
	{
		AM_DEBUG(1, "No TV program stored");
	}
	/*打印TV列表*/
	printf("-------------------------------------------------------------\n");
	for (i=0; i<row; i++)
	{
		printf("\t\t%03d	%s\n", chan_num[i], programs[i]);
	}
	printf("-------------------------------------------------------------\n");
	printf("Input \t'b' back to main menu, \n\t'u' channel plus,\
			 \n\t'd' channel minus, \n\t'n [number]' play channel 'number',\
			 \n\t'e' 7 days epg\n");
	printf("-------------------------------------------------------------\n\n");
}

static void run_radio_list(void)
{
	int row = MAX_PROG_CNT;
	int i;
	sqlite3 *hdb;

	AM_DB_HANDLE_PREPARE(hdb);

	/*从数据库取节目*/
	if (AM_DB_Select(hdb, "select chan_num,name from srv_table where service_type='2' order by chan_num", 
					&row, "%d,%s:64", chan_num, programs) != AM_SUCCESS)
	{
		AM_DEBUG(1, "Get program from dbase failed");
		return;
	}
	if (row == 0)
	{
		AM_DEBUG(1, "No Radio program stored");
	}
	/*打印TV列表*/
	printf("-------------------------------------------------------------\n");
	for (i=0; i<row; i++)
	{
		printf("\t\t%03d	%s\n", chan_num[i], programs[i]);
	}
	printf("-------------------------------------------------------------\n");
	printf("Input 'b' back to main menu, 'u' channel plus, 'd' channel minus, n [number] play channel 'number'\n");
	printf("-------------------------------------------------------------\n\n");
}

static void run_epg(void)
{
	/*显示当前频道当天的EPG*/
	display_detail_epg(0);
}

static CommandMenu menus[]=
{
	{0, -1,"Main Menu", 		NULL,	MENU_TYPE_MENU,	NULL,				NULL, 		NULL},
	{1, 0, "1. Auto Scan", 		"1",	MENU_TYPE_APP, 	run_auto_scan,		leave_scan,	auto_scan_input},
	{2, 0, "2. Manual Scan",	"2",	MENU_TYPE_APP,	run_manual_scan,	leave_scan,	manual_scan_input},
	{3, 0, "3. TV List", 		"3",	MENU_TYPE_APP, 	run_tv_list,		NULL,		tv_list_input},
	{4, 0, "4. Radio List", 	"4",	MENU_TYPE_APP, 	run_radio_list,		NULL,		radio_list_input},
	{5, 0, "5. Freq Setting", 	"5",	MENU_TYPE_APP,	run_freq_setting,	NULL,		freq_input},
	{6, 0, "6. Restore",		"6",	MENU_TYPE_APP, 	run_restore,		NULL,		restore_input},
	{7, 0, "7. Quit",			"7",	MENU_TYPE_APP,	run_quit,			NULL,		NULL},
#ifdef TEST_CA_CI
	{8, 0, "8. CI",			"8",	MENU_TYPE_APP,	run_ci,			leave_ci,		ci_input},
	{9, 3, NULL,				"e",	MENU_TYPE_APP,	run_epg,			NULL,		epg_input},
#else
	{8, 3, NULL,				"e",	MENU_TYPE_APP,	run_epg,			NULL,		epg_input},
#endif
};

static void display_menu(void)
{
	int i;

	assert(curr_menu);

	printf("-------------------------------------------\n");
	printf("\t\t%s\n", curr_menu->title);
	printf("-------------------------------------------\n");
	for (i=0; i<AM_ARRAY_SIZE(menus); i++)
	{
		if (menus[i].parent == curr_menu->id)
			printf("\t\t%s\n", menus[i].title);
	}
	printf("-------------------------------------------\n");
}


static void show_main_menu(void)
{
		
	curr_menu = &menus[0];
	display_menu();
}

static int menu_input(const char *cmd)
{
	int i;

	assert(curr_menu);

	if(!strcmp(cmd, "b"))
	{
		if (curr_menu->leave)
			curr_menu->leave();
		
		/*退回上级菜单*/
		if (curr_menu->parent != -1)
		{
			curr_menu = &menus[curr_menu->parent];
			if (curr_menu->type == MENU_TYPE_MENU)
			{
				display_menu();
			}
			else if (curr_menu->app)
			{
				curr_menu->app();
			}
		}
		return 0;
	}

	/*从预定义中查找按键处理*/		
	for (i=0; i<AM_ARRAY_SIZE(menus); i++)
	{
		if (menus[i].parent == curr_menu->id && !strcmp(cmd, menus[i].cmd))
		{
			curr_menu = &menus[i];
			if (menus[i].type == MENU_TYPE_MENU)
			{
				display_menu();
			}
			else if (menus[i].app)
			{
				menus[i].app();
			}
			return 0;
		}
	}

	return -1;
}

//#define TEST_CA_DUMMY
//#define TEST_CA_CI

static int start_tv_test()
{
	char buf[256];
	AM_EPG_CreatePara_t epara;
	sqlite3 *db;


#if defined(TEST_CA_DUMMY) || defined(TEST_CA_CI)
	AM_CAMAN_OpenParam_t caman_para;
	AM_CA_Opt_t ca_opt;
	memset(&caman_para, 0, sizeof(caman_para));
	
	caman_para.ts.dmx_fd = DMX_DEV_NO;
	caman_para.ts.fend_fd = FEND_DEV_NO;
	AM_TRY(AM_CAMAN_Open(&caman_para));

#endif

#ifdef TEST_CA_DUMMY
	/*register the dummy*/
	memset(&ca_opt, 0, sizeof(ca_opt));
	ca_opt.auto_disable = 0;
	AM_CAMAN_registerCA("dummy",  NULL, &ca_opt);

	AM_CAMAN_openCA("dummy");
#endif


#ifdef TEST_CA_CI
	AM_CI_OpenPara_t  ci_para;
	static AM_CI_Handle_t ci;
	AM_CA_t *ca;

	memset(&ci_para, 0, sizeof(ci_para));
	AM_CI_Open(0, 0, &ci_para, &ci);
//	AM_TRY(AM_CI_Start(ci));

	/*register the ci*/
	AM_CI_CAMAN_getCA(ci, &ca);
	memset(&ca_opt, 0, sizeof(ca_opt));
	ca_opt.auto_disable = 0;
	AM_CAMAN_registerCA("ci0", ca, &ca_opt);
	AM_CAMAN_setCallback("ci0", caman_cb);
	AM_CAMAN_openCA("ci0");
#endif


	if(NULL==opendir("/system"))
	{
		AM_TRY(AM_DB_Setup("/tvtest.db", NULL));
	}
	else
	{
		AM_TRY(AM_DB_Setup("/system/tvtest.db", NULL));
	}


	AM_TRY(AM_DB_GetHandle(&db));	
	AM_TRY(AM_DB_CreateTables(db));

	AM_TV_Create(FEND_DEV_NO, AV_DEV_NO, NULL, &htv);

	epara.fend_dev = FEND_DEV_NO;
	epara.dmx_dev = DMX_DEV_NO;
	epara.source = 0;
	AM_TRY(AM_EPG_Create(&epara, &hepg));

	AM_EVT_Subscribe(hepg, AM_EPG_EVT_NEW_EIT, epg_evt_call_back, NULL);

	//AM_EPG_ChangeMode(hepg, AM_EPG_MODE_OP_ADD,  AM_EPG_SCAN_ALL/* & (~AM_EPG_SCAN_EIT_SCHE_ALL)*/);
	AM_EPG_ChangeMode(hepg, AM_EPG_MODE_OP_REMOVE,  AM_EPG_SCAN_ALL/* & (~AM_EPG_SCAN_EIT_SCHE_ALL)*/);

	/*设置时间偏移*/
	AM_EPG_SetLocalOffset(8*3600);

#if 1
	fend_param.m_type = FE_OFDM;
	fend_param.terrestrial.para.frequency = 474000000;
	fend_param.terrestrial.para.u.ofdm.bandwidth = BANDWIDTH_8_MHZ;
#else	
	fend_param.m_type = AM_FEND_DEMOD_DVBC
	fend_param.cable.para.frequency = 474000000;
	fend_param.cable.para.u.qam.symbol_rate = 6875*1000;
	fend_param.cable.para.u.qam.modulation = QAM_64;
	fend_param.cable.para.u.qam.fec_inner = FEC_AUTO;
#endif

	hscan = 0;
	show_main_menu();
	
	AM_TV_Play(htv);
	
	while (go)
	{
		if (gets(buf) && curr_menu)
		{
			if (menu_input(buf) < 0)
			{
				/*交由应用处理*/
				if (curr_menu->input)
				{
					curr_menu->input(buf);
				}
			}
		}
	}
	AM_EVT_Unsubscribe(hepg, AM_EPG_EVT_NEW_EIT, epg_evt_call_back, NULL);

	AM_EPG_Destroy(hepg);
	
	AM_TV_Destroy(htv);
	
	AM_TRY(AM_DB_UnSetup());

#ifdef TEST_CA_DUMMY
	AM_CAMAN_unregisterCA("dummy");
#endif

#ifdef TEST_CA_CI
	AM_CAMAN_unregisterCA("ci0");
	AM_CI_Close(ci);
#endif

#if defined(TEST_CA_DUMMY) || defined(TEST_CA_CI)
	AM_CAMAN_Close();
#endif

	return 0;
}
int main(int argc, char **argv)
{
	AM_DMX_OpenPara_t para;
	AM_FEND_OpenPara_t fpara;
	AM_AV_OpenPara_t apara;
	
	memset(&fpara, 0, sizeof(fpara));
	fpara.mode = FE_OFDM;
	AM_TRY(AM_FEND_Open(FEND_DEV_NO, &fpara));
	
	memset(&para, 0, sizeof(para));
	AM_TRY(AM_DMX_Open(DMX_DEV_NO, &para));

	memset(&apara, 0, sizeof(apara));
	AM_TRY(AM_AV_Open(AV_DEV_NO, &apara));

	AM_DMX_SetSource(DMX_DEV_NO, AM_DMX_SRC_TS2);
	AM_AV_SetTSSource(AV_DEV_NO, AM_AV_TS_SRC_TS2);

	start_tv_test();

	AM_AV_Close(AV_DEV_NO);
	AM_DMX_Close(DMX_DEV_NO);
	AM_FEND_Close(FEND_DEV_NO);
	
	return 0;
}


/****************************************************************************
 * API functions
 ***************************************************************************/

