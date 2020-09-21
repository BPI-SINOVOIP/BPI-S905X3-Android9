/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package. *
* Description:
*/
#include <errno.h>
#include <assert.h>
#include <fcntl.h>
#include <am_iconv.h>
#include <am_debug.h>
#include <am_scan.h>
#include "am_db.h"

/**PAL B*/
#define STD_PAL_B	    0x00000001
/**PAL B1*/
#define STD_PAL_B1     0x00000002
/**PAL G*/
#define STD_PAL_G	    0x00000004
/**PAL H*/
#define STD_PAL_H	    0x00000008
/**PAL I*/
#define STD_PAL_I	    0x00000010
/**PAL D*/
#define STD_PAL_D	    0x00000020
/**PAL D1*/
#define STD_PAL_D1     0x00000040
/**PAL K*/
#define STD_PAL_K	    0x00000080
/**PAL M*/
#define STD_PAL_M	    0x00000100
/**PAL N*/
#define STD_PAL_N	    0x00000200
/**PAL Nc*/
#define STD_PAL_Nc     0x00000400
/**PAL 60*/
#define STD_PAL_60     0x00000800
/**NTSC M*/
#define STD_NTSC_M     0x00001000
/**NTSC M JP*/
#define STD_NTSC_M_JP  0x00002000
/**NTSC 443*/
#define STD_NTSC_443   0x00004000
/**NTSC M KR*/
#define STD_NTSC_M_KR  0x00008000
/**SECAM B*/
#define STD_SECAM_B    0x00010000
/**SECAM D*/
#define STD_SECAM_D    0x00020000
/**SECAM G*/
#define STD_SECAM_G    0x00040000
/**SECAM H*/
#define STD_SECAM_H    0x00080000
/**SECAM K*/
#define STD_SECAM_K    0x00100000
/**SECAM K1*/
#define STD_SECAM_K1   0x00200000
/**SECAM L*/
#define STD_SECAM_L    0x00400000
/**SECAM LC*/
#define STD_SECAM_LC   0x00800000
/**ATSC VSB8*/
#define STD_ATSC_8_VSB   0x01000000
/**ATSC VSB16*/
#define STD_ATSC_16_VSB  0x02000000
/**NTSC*/
#define STD_NTSC	    STD_NTSC_M|STD_NTSC_M_JP|STD_NTSC_M_KR
/**SECAM DK*/
#define STD_SECAM_DK   STD_SECAM_D|STD_SECAM_K|STD_SECAM_K1
/**SECAM*/
#define STD_SECAM	    STD_SECAM_B|STD_SECAM_G|STD_SECAM_H|STD_SECAM_DK|STD_SECAM_L|STD_SECAM_LC
/**PAL BG*/
#define STD_PAL_BG     STD_PAL_B|STD_PAL_B1|STD_PAL_G
/**PAL DK*/
#define STD_PAL_DK     STD_PAL_D|STD_PAL_D1|STD_PAL_K
/**PAL*/
#define STD_PAL	    STD_PAL_BG|STD_PAL_DK|STD_PAL_H|STD_PAL_I

//#define TUNER_STD_MN  = STD_PAL_M|STD_PAL_N|STD_PAL_Nc| STD_NTSC
#define STD_B		    STD_PAL_B	  |STD_PAL_B1	|STD_SECAM_B
#define STD_GH 	    STD_PAL_G	  | STD_PAL_H	|STD_SECAM_G  |STD_SECAM_H
#define STD_DK 	    STD_PAL_DK   | STD_SECAM_DK
#define STD_M		    STD_PAL_M	  | STD_NTSC_M
#define STD_BG 	    STD_PAL_BG   | STD_SECAM_B | STD_SECAM_G 

#define COLOR_AUTO  0x02000000
#define COLOR_PAL	 0x04000000
#define COLOR_NTSC  0x08000000
#define COLOR_SECAM 0x10000000

enum stmts_t
{
	QUERY_TS,
	UPDATE_TS,
	INSERT_TS,
	QUERY_SRV,
	INSERT_SRV,
	UPDATE_SRV,
	MAX_SQL,
};
static const char *path = "/param/dtv.db";
sqlite3 *hdb;

typedef struct atv_channel
{
	int freq;
	int std;
}atv_channel_t;
atv_channel_t preset_atv[16]=
{
	//ge er 4
	{49750000, (STD_PAL_DK|COLOR_PAL)},
	{211250000, (STD_NTSC|COLOR_NTSC)},
	{471250000, (STD_PAL_BG|COLOR_PAL)},
	{543250000, (STD_PAL_I|COLOR_PAL)},
	//yantai 4
	{57750000, (STD_PAL_DK|COLOR_PAL)},
	{128250000, (STD_PAL_DK|COLOR_PAL)},
	{360250000, (STD_PAL_DK|COLOR_PAL)},
	{855250000, (STD_PAL_DK|COLOR_PAL)},
	//kunshan 8
	{62250000, (STD_PAL_BG|COLOR_PAL)},
	{85250000, (STD_PAL_DK|COLOR_PAL)},
	{168250000, (STD_PAL_DK|COLOR_PAL)},
	{191250000, (STD_PAL_DK|COLOR_SECAM)},
	{471250000, (STD_PAL_DK|COLOR_PAL)},
	{503250000, (STD_PAL_I|COLOR_PAL)},
	{743250000, (STD_PAL_DK|COLOR_PAL)},
	{776250000, (STD_PAL_DK|COLOR_PAL)},
	
};
const char *atv_sql_stmts[MAX_SQL] = 
{
	"select db_id from ts_table where src=? and freq=?",
	"update ts_table set snr=?,ber=? where db_id=?",
	"insert into ts_table(src,db_net_id,ts_id,freq,symb,mod,bw,snr,ber,strength,db_sat_para_id,polar,std,aud_mode,flags,dvbt_flag) \
	 values(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)",
	"select db_id from srv_table where src=? and db_ts_id=?",	 
	"insert into srv_table(src,db_net_id,db_ts_id,name,service_id,service_type,eit_schedule_flag,eit_pf_flag,running_status,free_ca_mode,\
	 volume,aud_track,pmt_pid,vid_pid,vid_fmt,scrambled_flag,current_aud,aud_pids,aud_fmts,aud_langs,aud_types,current_sub,sub_pids,\
	 sub_types,sub_composition_page_ids,sub_ancillary_page_ids,sub_langs,current_ttx,ttx_pids,ttx_types,ttx_magazine_nos,ttx_page_nos,ttx_langs,chan_num,\
	 skip,lock,favor,lcn,sd_lcn,hd_lcn,default_chan_num,chan_order,lcn_order,service_id_order,hd_sd_order,db_sat_para_id,dvbt2_plp_id,major_chan_num,\
	 minor_chan_num,access_controlled,hidden,hide_guide,source_id,sdt_ver) \
	 values(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)",
	"update srv_table set volume=? where db_id=?",
};
static int store_insert_ts(sqlite3_stmt **stmts, int freq, int std)
{
	int dbid;
	char sqlstr[128];
	printf("@@ Storing a analog ts freq = %d@@\n",freq);
		
	dbid = -1;
	/*query wether it exists*/
	sqlite3_bind_int(stmts[QUERY_TS], 1, FE_ANALOG);
	sqlite3_bind_int(stmts[QUERY_TS], 2, freq);
	if (sqlite3_step(stmts[QUERY_TS]) == SQLITE_ROW)
	{
		dbid = sqlite3_column_int(stmts[QUERY_TS], 0);
	}
	sqlite3_reset(stmts[QUERY_TS]);

	if(dbid != -1)
	{
		printf("delete from ts_table where freq=%d and db_id=%d\n",freq,dbid);
		snprintf(sqlstr, sizeof(sqlstr), "delete from ts_table where src=%d and db_id=%d",FE_ANALOG,dbid);
		sqlite3_exec(hdb, sqlstr, NULL, NULL, NULL);
		dbid = -1;
	}
	if(dbid == -1)
	{
		sqlite3_bind_int(stmts[INSERT_TS], 1, 4);
		sqlite3_bind_int(stmts[INSERT_TS], 2, -1);
		sqlite3_bind_int(stmts[INSERT_TS], 3, -1);
		sqlite3_bind_int(stmts[INSERT_TS], 4, freq);
		sqlite3_bind_int(stmts[INSERT_TS], 5, 0);
		sqlite3_bind_int(stmts[INSERT_TS], 6, 0);
		sqlite3_bind_int(stmts[INSERT_TS], 7, 0);
		sqlite3_bind_int(stmts[INSERT_TS], 8, 0);
		sqlite3_bind_int(stmts[INSERT_TS], 9, 0);
		sqlite3_bind_int(stmts[INSERT_TS], 10, 0);
		sqlite3_bind_int(stmts[INSERT_TS], 11, -1);
		sqlite3_bind_int(stmts[INSERT_TS], 12, -1);
		sqlite3_bind_int(stmts[INSERT_TS], 13, std);
		sqlite3_bind_int(stmts[INSERT_TS], 14, 1);
		sqlite3_bind_int(stmts[INSERT_TS], 15, 0);
		sqlite3_bind_int(stmts[INSERT_TS], 16, 0);
		if (sqlite3_step(stmts[INSERT_TS]) == SQLITE_DONE)
		{
			sqlite3_bind_int(stmts[QUERY_TS], 1, FE_ANALOG);
			sqlite3_bind_int(stmts[QUERY_TS], 2, freq);
			if (sqlite3_step(stmts[QUERY_TS]) == SQLITE_ROW)
			{
				dbid = sqlite3_column_int(stmts[QUERY_TS], 0);
			}
			sqlite3_reset(stmts[QUERY_TS]);
		}
		
		sqlite3_reset(stmts[INSERT_TS]);
	}
	
	if (dbid == -1)
	{
		printf("insert new ts error\n");
	}
	else
	{
		sqlite3_bind_int(stmts[UPDATE_TS], 1, 0);
		sqlite3_bind_int(stmts[UPDATE_TS], 2, 0);
		sqlite3_bind_int(stmts[UPDATE_TS], 3, dbid);
		sqlite3_step(stmts[UPDATE_TS]);
		sqlite3_reset(stmts[UPDATE_TS]);
		printf("insert new ts success\n");
	}
	
	return dbid;
}
static int atv_insert_srv(sqlite3_stmt **stmts, int db_ts_id,int channum)
{
	int db_id = -1;
	char name[64];
	char sqlstr[128];
	printf("add srv_table where db_ts_id=%d and db_id=%d\n",db_ts_id,db_id);
	snprintf(name, sizeof(name), "%s", "xxxATV Program");
	/*query wether it exists*/
	sqlite3_bind_int(stmts[QUERY_SRV], 1, FE_ANALOG);
	sqlite3_bind_int(stmts[QUERY_SRV], 2, db_ts_id);
	if (sqlite3_step(stmts[QUERY_SRV]) == SQLITE_ROW)
	{
		db_id = sqlite3_column_int(stmts[QUERY_SRV], 0);
	}
	sqlite3_reset(stmts[QUERY_SRV]);

	if(db_id != -1)
	{
		printf("delete from srv_table where db_ts_id=%d and db_id=%d\n",db_ts_id,db_id);
		snprintf(sqlstr, sizeof(sqlstr), "delete from srv_table where src=%d and db_ts_id=%d",FE_ANALOG,db_id);
		sqlite3_exec(hdb, sqlstr, NULL, NULL, NULL);
		db_id = -1;
	}

	/*if not exist , insert a new record*/
	if (db_id == -1)
	{
		sqlite3_bind_int(stmts[INSERT_SRV], 1, FE_ANALOG);
		sqlite3_bind_int(stmts[INSERT_SRV], 2, -1);
		sqlite3_bind_int(stmts[INSERT_SRV], 3, db_ts_id);
		sqlite3_bind_text(stmts[INSERT_SRV], 4, name, strlen(name), SQLITE_STATIC);
		sqlite3_bind_int(stmts[INSERT_SRV], 5, 0xffff);
		sqlite3_bind_int(stmts[INSERT_SRV], 6, 3);
		sqlite3_bind_int(stmts[INSERT_SRV], 7, 0);
		sqlite3_bind_int(stmts[INSERT_SRV], 8, 0);
		sqlite3_bind_int(stmts[INSERT_SRV], 9, 0);
		sqlite3_bind_int(stmts[INSERT_SRV], 10, 1);
		sqlite3_bind_int(stmts[INSERT_SRV], 11, 50);
		sqlite3_bind_int(stmts[INSERT_SRV], 12, 1);
		sqlite3_bind_int(stmts[INSERT_SRV], 13, 0x1fff);
		sqlite3_bind_int(stmts[INSERT_SRV], 14, 0x1fff);
		sqlite3_bind_int(stmts[INSERT_SRV], 15, -1);
		sqlite3_bind_int(stmts[INSERT_SRV], 16, 0);
		sqlite3_bind_int(stmts[INSERT_SRV], 17, -1);
		sqlite3_bind_int(stmts[INSERT_SRV], 18, 0x1fff);
		sqlite3_bind_int(stmts[INSERT_SRV], 19, -1);
		sqlite3_bind_text(stmts[INSERT_SRV], 20, "Audio1", strlen("Audio1"), SQLITE_STATIC);
		sqlite3_bind_int(stmts[INSERT_SRV], 21, 0);
		sqlite3_bind_int(stmts[INSERT_SRV], 22, -1);
		sqlite3_bind_text(stmts[INSERT_SRV], 23, "", strlen(""), SQLITE_STATIC);
		sqlite3_bind_text(stmts[INSERT_SRV], 24, "", strlen(""), SQLITE_STATIC);
		sqlite3_bind_text(stmts[INSERT_SRV], 25, "", strlen(""), SQLITE_STATIC);
		sqlite3_bind_text(stmts[INSERT_SRV], 26, "", strlen(""), SQLITE_STATIC);
		sqlite3_bind_text(stmts[INSERT_SRV], 27, "", strlen(""), SQLITE_STATIC);
		sqlite3_bind_int(stmts[INSERT_SRV], 28, -1);
		sqlite3_bind_text(stmts[INSERT_SRV], 29, "", strlen(""), SQLITE_STATIC);
		sqlite3_bind_text(stmts[INSERT_SRV], 30, "", strlen(""), SQLITE_STATIC);
		sqlite3_bind_text(stmts[INSERT_SRV], 31, "", strlen(""), SQLITE_STATIC);
		sqlite3_bind_text(stmts[INSERT_SRV], 32, "", strlen(""), SQLITE_STATIC);
		sqlite3_bind_text(stmts[INSERT_SRV], 33, "", strlen(""), SQLITE_STATIC);
		sqlite3_bind_int(stmts[INSERT_SRV], 34, channum);
		sqlite3_bind_int(stmts[INSERT_SRV], 35, 0);
		sqlite3_bind_int(stmts[INSERT_SRV], 36, 0);
		sqlite3_bind_int(stmts[INSERT_SRV], 37, 0);
		sqlite3_bind_int(stmts[INSERT_SRV], 38, -1);
		sqlite3_bind_int(stmts[INSERT_SRV], 39, -1);
		sqlite3_bind_int(stmts[INSERT_SRV], 40, -1);
		sqlite3_bind_int(stmts[INSERT_SRV], 41, channum);
		sqlite3_bind_int(stmts[INSERT_SRV], 42, channum);
		sqlite3_bind_int(stmts[INSERT_SRV], 43, channum);
		sqlite3_bind_int(stmts[INSERT_SRV], 44, channum);
		sqlite3_bind_int(stmts[INSERT_SRV], 45, channum);
		sqlite3_bind_int(stmts[INSERT_SRV], 46, -1);
		sqlite3_bind_int(stmts[INSERT_SRV], 47, 0xff);
		sqlite3_bind_int(stmts[INSERT_SRV], 48, 0);
		sqlite3_bind_int(stmts[INSERT_SRV], 49, 0);
		sqlite3_bind_int(stmts[INSERT_SRV], 50, 0);
		sqlite3_bind_int(stmts[INSERT_SRV], 51, 0);
		sqlite3_bind_int(stmts[INSERT_SRV], 52, 0);
		sqlite3_bind_int(stmts[INSERT_SRV], 53, 0);
		sqlite3_bind_int(stmts[INSERT_SRV], 54, 0xff);
		if (sqlite3_step(stmts[INSERT_SRV]) == SQLITE_DONE)
		{
			sqlite3_bind_int(stmts[QUERY_SRV], 1, FE_ANALOG);
			sqlite3_bind_int(stmts[QUERY_SRV], 2, db_ts_id);
			if (sqlite3_step(stmts[QUERY_SRV]) == SQLITE_ROW)
			{
				db_id = sqlite3_column_int(stmts[QUERY_SRV], 0);
			}
			sqlite3_reset(stmts[QUERY_SRV]);
		}
		sqlite3_reset(stmts[INSERT_SRV]);
	}

	if (db_id == -1)
	{
		printf("insert new srv error\n");
	}
	else
	{
		sqlite3_bind_int(stmts[UPDATE_SRV], 1, 40);
		sqlite3_bind_int(stmts[UPDATE_SRV], 2, db_id);
		sqlite3_step(stmts[UPDATE_SRV]);
		sqlite3_reset(stmts[UPDATE_SRV]);
		printf("insert new srv success\n");
	}
	
	return db_id;
}
static void am_scan_clear_source(sqlite3 *hdb, int src)
{
	char sqlstr[128];
	
	/*删除network记录*/
	snprintf(sqlstr, sizeof(sqlstr), "delete from net_table where src=%d",src);
	sqlite3_exec(hdb, sqlstr, NULL, NULL, NULL);
	/*清空TS记录*/
	snprintf(sqlstr, sizeof(sqlstr), "delete from ts_table where src=%d",src);
	sqlite3_exec(hdb, sqlstr, NULL, NULL, NULL);
	/*清空service group记录*/
	snprintf(sqlstr, sizeof(sqlstr), "delete from grp_map_table where db_srv_id in (select db_id from srv_table where src=%d)",src);
	sqlite3_exec(hdb, sqlstr, NULL, NULL, NULL);
	/*清空SRV记录*/
	snprintf(sqlstr, sizeof(sqlstr), "delete from srv_table where src=%d",src);
	sqlite3_exec(hdb, sqlstr, NULL, NULL, NULL);
	/*清空booking记录*/
    snprintf(sqlstr, sizeof(sqlstr), "delete from booking_table where db_srv_id in (select db_srv_id from evt_table where src=%d)",src);
    sqlite3_exec(hdb, sqlstr, NULL, NULL, NULL);
	/*清空event记录*/
	snprintf(sqlstr, sizeof(sqlstr), "delete from evt_table where src=%d",src);
	sqlite3_exec(hdb, sqlstr, NULL, NULL, NULL);
}


static void store_atv(sqlite3_stmt **stmts, int freq, int std, int channo)
{
	int dbid;
	
	dbid = store_insert_ts(stmts, freq, std);
	
	atv_insert_srv(stmts,dbid,channo);
}

static void PresetATV_store(int begin, int end)
{
	AM_SCAN_TS_t *ts;
	sqlite3_stmt	*stmts[MAX_SQL];
	int ret, i=0, size = end - begin;
	

	AM_DB_HANDLE_PREPARE(hdb);

	/*Prepare sqlite3 stmts*/
	memset(stmts, 0, sizeof(sqlite3_stmt*) * MAX_SQL);
	//memset(stmts, 0, sizeof(sqlite3_stmt*) * MAX_STMT);
	for (i=0; i<MAX_SQL; i++)
	{
		ret = sqlite3_prepare(hdb, atv_sql_stmts[i], -1, &stmts[i], NULL);
		if (ret != SQLITE_OK)
		{
			printf("Prepare sqlite3 failed, stmts[%d] ret = %x\n", i, ret);
			goto store_end;
		}
	}
	
	printf("Storing atv ...\n");
	/*依次存储每个TS*/
	am_scan_clear_source(hdb, FE_ANALOG);
	for(i=0;i < size; i++)
	{
		store_atv(stmts, preset_atv[begin+i].freq, preset_atv[begin+i].std,i+1);
	}

store_end:
	for (i=0; i<MAX_SQL; i++)
	{
		if (stmts[i] != NULL)
			sqlite3_finalize(stmts[i]);
	}
}
int main(int argc, char **argv)
{
    sqlite3 *mHandle=NULL;
	int begin, end;
	
	if(path != NULL) {
		if (sqlite3_open(path, &mHandle) != SQLITE_OK) {
			printf("open db(%s) error\n", path);
			return -1;
		}
		//setup and path set
		AM_DB_Setup((char*)path, mHandle);
		begin = strtol(argv[1], NULL, 0);
		end = strtol(argv[2], NULL, 0);
		PresetATV_store(begin, end);
	}
	
	printf("insert new ts & srv success\n");
	return -1;
}

