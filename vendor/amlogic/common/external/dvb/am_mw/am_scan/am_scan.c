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
/**\file am_scan.c
 * \brief SCAN模块
 *
 * \author Xia Lei Peng <leipeng.xia@amlogic.com>
 * \date 2010-10-27: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 2

#include <errno.h>
#include <assert.h>
#include <fcntl.h>
#include <limits.h>
#include <am_iconv.h>
#include <am_debug.h>
#include <am_scan.h>
#include "am_scan_internal.h"
#include <am_dmx.h>
#include <am_time.h>
#include <am_aout.h>
#include <am_av.h>
#include <am_cond.h>
#include <linux/ioctl.h>
#include <linux/types.h>
#include "am_check_scramb.h"
#include "atv_frontend.h"
#include "am_vlfend.h"
#include <stdbool.h>
/****************************************************************************
 * Macro definitions
 ***************************************************************************/

#define SCAN_FEND_DEV_NO 0
#define SCAN_DMX_DEV_NO 0
#define SCAN_DVR_DEV_ID 2
#define SCAN_DVR_SRC 0
#define SCAN_DMX_DEV_ID 2
#define SCAN_DMX_SRC 2
/*位操作*/
#define BIT_MASK(b) (1 << ((b) % 8))
#define BIT_SLOT(b) ((b) / 8)
#define BIT_SET(a, b) ((a)[BIT_SLOT(b)] |= BIT_MASK(b))
#define BIT_CLEAR(a, b) ((a)[BIT_SLOT(b)] &= ~BIT_MASK(b))
#define BIT_TEST(a, b) ((a)[BIT_SLOT(b)] & BIT_MASK(b))

/*超时ms定义*/
#define PAT_TIMEOUT 3000
#define PMT_TIMEOUT 3000
#define SDT_TIMEOUT 6000
#define NIT_TIMEOUT 10000
#define CAT_TIMEOUT 3000
#define BAT_TIMEOUT 10000
int am_scan_mgt_timeout = 2500;
#define MGT_TIMEOUT am_scan_mgt_timeout
#define STT_TIMEOUT 0
int am_scan_vct_timeout = 2500;
#define VCT_TIMEOUT am_scan_vct_timeout
#define RRT_TIMEOUT 70000
#define ISDBT_PAT_TIMEOUT 1000

/*ISDBT one-seg PMTs*/
#define ISDBT_ONESEG_PMT_BEGIN 0x1fc8
#define ISDBT_ONESEG_PMT_END   0x1fcf

/*子表最大个数*/
#define MAX_BAT_SUBTABLE_CNT 32

/*多子表时接收重复最小间隔*/
#define BAT_REPEAT_DISTANCE 3000

/*DVBC默认前端参数*/
#define DEFAULT_DVBC_MODULATION QAM_64
#define DEFAULT_DVBC_SYMBOLRATE 6875000

/*DVBT默认前端参数*/
#define DEFAULT_DVBT_BW BANDWIDTH_8_MHZ
#define DEFAULT_DVBT_FREQ_START    474000
#define DEFAULT_DVBT_FREQ_STOP     858000

/*DVBS盲扫起始结束频率*/
#define DEFAULT_DVBS_BS_FREQ_START	950000000 /*950MHz*/
#define DEFAULT_DVBS_BS_FREQ_STOP	2150000000U /*2150MHz*/

/*电视广播排序是否统一编号*/
#define SORT_TOGETHER

/*清除一个subtable控制数据*/
#define SUBCTL_CLEAR(sc)\
	AM_MACRO_BEGIN\
		memset((sc)->mask, 0, sizeof((sc)->mask));\
		(sc)->ver = 0xff;\
	AM_MACRO_END

/*添加数据列表中*/
#define ADD_TO_LIST(_t, _l)\
	AM_MACRO_BEGIN\
		(_t)->p_next = (_l);\
		_l = _t;\
	AM_MACRO_END

/*添加数据到列表尾部*/
#define APPEND_TO_LIST(_type,_t,_l)\
	AM_MACRO_BEGIN\
		_type *tmp = _l;\
		_type *last = NULL;\
		while (tmp != NULL) {\
			last = tmp;\
			tmp = tmp->p_next;\
		}\
		if (last == NULL)\
			_l = _t;\
		else\
			last->p_next = _t;\
	AM_MACRO_END

/*释放一个表的所有SI数据*/
#define RELEASE_TABLE_FROM_LIST(_t, _l)\
	AM_MACRO_BEGIN\
		_t *tmp, *next;\
		tmp = (_t *)(_l);\
		while (tmp){\
			next = tmp->p_next;\
			AM_SI_ReleaseSection(scanner->dtvctl.hsi, tmp->i_table_id, (void*)tmp);\
			tmp = next;\
		}\
		(_l) = NULL;\
	AM_MACRO_END

/*解析section并添加到列表*/
#define COLLECT_SECTION(type, list)\
	AM_MACRO_BEGIN\
		type *p_table;\
		if (AM_SI_DecodeSection(scanner->dtvctl.hsi, sec_ctrl->pid, (uint8_t*)data, len, (void**)&p_table) == AM_SUCCESS)\
		{\
			p_table->p_next = NULL;\
			APPEND_TO_LIST(type, p_table, list); /*添加到搜索结果列表中*/\
			am_scan_tablectl_mark_section(sec_ctrl, &header); /*设置为已接收*/\
		}else\
		{\
			AM_DEBUG(1, "Decode Section failed");\
		}\
	AM_MACRO_END

/*通知一个事件*/
#define SIGNAL_EVENT(e, d)\
	AM_MACRO_BEGIN\
		pthread_mutex_unlock(&scanner->lock);\
		AM_EVT_Signal((long)scanner, e, d);\
		pthread_mutex_lock(&scanner->lock);\
	AM_MACRO_END

/*触发一个进度事件*/
#define SET_PROGRESS_EVT(t, d)\
	AM_MACRO_BEGIN\
		AM_SCAN_Progress_t prog;\
		prog.evt = t;\
		prog.data = (void*)(long)d;\
		SIGNAL_EVENT(AM_SCAN_EVT_PROGRESS, (void*)&prog);\
	AM_MACRO_END

#define GET_MODE(_m) ((_m) & 0x07)

#define dvb_fend_para(_p)	((struct dvb_frontend_parameters*)(&_p))

#define IS_DVBT2_TS(_para) (_para.m_type==FE_OFDM && \
	_para.terrestrial.ofdm_mode == OFDM_DVBT2)
#define IS_ISDBT_TS(_para) (_para.m_type==FE_ISDBT)
#define IS_ATSC_QAM256_TS(_para) (_para.m_type==FE_ATSC && \
	_para.atsc.para.u.vsb.modulation==QAM_256)
#define IS_ATSC_TS(_para) (_para.m_type==FE_ATSC)
#define IS_ATSC_QAM_TS(_para) (IS_ATSC_TS(_para) && \
	(_para.atsc.para.u.vsb.modulation==QAM_256 || \
	_para.atsc.para.u.vsb.modulation==QAM_64))
#define IS_DVBT2() IS_DVBT2_TS(scanner->start_freqs[scanner->curr_freq].fe_para)
#define IS_ISDBT() IS_ISDBT_TS(scanner->start_freqs[scanner->curr_freq].fe_para)
#define IS_ATSC_QAM256() IS_ATSC_QAM256_TS(scanner->start_freqs[scanner->curr_freq].fe_para)

#define cur_fe_para		scanner->start_freqs[scanner->curr_freq].fe_para
#define dtv_start_para		scanner->start_para.dtv_para
#define atv_start_para		scanner->start_para.atv_para

#define HELPER_CB(_id_, _para_) do { \
	if (scanner->helper[(_id_)].cb) \
		scanner->helper[(_id_)].cb((_id_), (_para_), scanner->helper[(_id_)].user); \
	}while(0)

/****************************************************************************
 * static data
 ***************************************************************************/

/*DVB-C标准频率表*/
static int dvbc_std_freqs[] =
{
52500,  60250,  68500,  80000,
88000,  115000, 123000, 131000,
139000, 147000, 155000, 163000,
171000, 179000, 187000, 195000,
203000, 211000, 219000, 227000,
235000, 243000, 251000, 259000,
267000, 275000, 283000, 291000,
299000, 307000, 315000, 323000,
331000, 339000, 347000, 355000,
363000, 371000, 379000, 387000,
395000, 403000, 411000, 419000,
427000, 435000, 443000, 451000,
459000, 467000, 474000, 482000,
490000, 498000, 506000, 514000,
522000, 530000, 538000, 546000,
554000, 562000, 570000, 578000,
586000, 594000, 602000, 610000,
618000, 626000, 634000, 642000,
650000, 658000, 666000, 674000,
682000, 690000, 698000, 706000,
714000, 722000, 730000, 738000,
746000, 754000, 762000, 770000,
778000, 786000, 794000, 802000,
810000, 818000, 826000, 834000,
842000, 850000, 858000, 866000,
874000
};

/*SQLite3 stmts*/
const char *sql_stmts[MAX_STMT] =
{
/*QUERY_NET,*/	                     "select db_id from net_table where src=? and network_id=?",
/*INSERT_NET,*/	                     "insert into net_table(network_id, src) values(?,?)",
/*UPDATE_NET,*/	                     "update net_table set name=? where db_id=?",
/*QUERY_TS,*/	                     "select db_id from ts_table where src=? and freq=? and db_sat_para_id=? and polar=?",
/*INSERT_TS,*/	                     "insert into ts_table(src,freq,db_sat_para_id,polar) values(?,?,?,?)",
/*UPDATE_TS,*/	                     "update ts_table set db_net_id=?,ts_id=?,symb=?,mod=?,bw=?,snr=?,ber=?,strength=?,std=?,aud_mode=?,dvbt_flag=?,flags=0 where db_id=?",
/*DELETE_TS_SRVS,*/	                     "delete  from srv_table where db_ts_id=?",
/*QUERY_SRV,*/	                     "select db_id from srv_table where db_net_id=? and db_ts_id=? and dvbt2_plp_id=? and service_id=?",
/*INSERT_SRV,*/	                     "insert into srv_table(db_net_id, db_ts_id,dvbt2_plp_id,service_id) values(?,?,?,?)",
/*UPDATE_SRV,*/	                     "update srv_table set src=?, name=?,service_type=?,eit_schedule_flag=?, eit_pf_flag=?,\
	 running_status=?,free_ca_mode=?,volume=?,aud_track=?,vid_pid=?,vid_fmt=?,\
	 current_aud=-1,aud_pids=?,aud_fmts=?,aud_langs=?, aud_types=?,\
	 current_sub=-1,sub_pids=?,sub_types=?,sub_composition_page_ids=?,sub_ancillary_page_ids=?,sub_langs=?,\
	 current_ttx=-1,ttx_pids=?,ttx_types=?,ttx_magazine_nos=?,ttx_page_nos=?,ttx_langs=?,\
	 skip=0,lock=0,chan_num=?,major_chan_num=?,minor_chan_num=?,access_controlled=?,hidden=?,\
	 hide_guide=?, source_id=?,favor=?,db_sat_para_id=?,scrambled_flag=?,lcn=-1,hd_lcn=-1,sd_lcn=-1,\
	 default_chan_num=-1,chan_order=?,lcn_order=0,service_id_order=0,hd_sd_order=0,pmt_pid=?,dvbt2_plp_id=?,sdt_ver=?,pcr_pid=?,aud_exten=? where db_id=?",
/*QUERY_SRV_BY_TYPE,*/	             "select db_id,service_type from srv_table where db_ts_id=? order by service_id",
/*UPDATE_CHAN_NUM,*/	             "update srv_table set chan_num=? where db_id=?",
/*DELETE_TS_EVTS,*/	             "delete  from evt_table where db_ts_id=?",
/*QUERY_TS_BY_FREQ_ORDER,*/	     "select db_id from ts_table where src=? order by freq",
/*DELETE_SRV_GRP,*/	             "delete from grp_map_table where db_srv_id in (select db_id from srv_table where db_ts_id=?)",
/*QUERY_SRV_TS_NET_ID,*/	     "select srv_table.service_id, ts_table.ts_id, net_table.network_id from srv_table, ts_table, net_table where srv_table.db_id=? and ts_table.db_id=srv_table.db_ts_id and net_table.db_id=srv_table.db_net_id",
/*UPDATE_CHAN_SKIP,*/	             "update srv_table set skip=? where db_id=?",
/*QUERY_MAX_CHAN_NUM_BY_TYPE,*/   "select max(chan_num) from srv_table where service_type=? and src=?",
/*QUERY_MAX_CHAN_NUM,*/	     "select max(chan_num) from srv_table where src=?",
/*QUERY_MAX_MAJOR_CHAN_NUM,*/	     "select max(major_chan_num) from srv_table",
/*UPDATE_MAJOR_CHAN_NUM,*/	     "update srv_table set major_chan_num=?, chan_num=? where db_id=(select db_id from srv_table where db_ts_id=?)",
/*QUERY_SRV_BY_CHAN_NUM,*/	     "select db_id from srv_table where src=? and chan_num=?",
/*QUERY_SAT_PARA_BY_POS_NUM,*/     "select db_id from sat_para_table where lnb_num=? and motor_num=? and pos_num=?",
/*QUERY_SAT_PARA_BY_LO_LA,*/	     "select db_id from sat_para_table where lnb_num=? and sat_longitude=?",
/*INSERT_SAT_PARA,*/	             "insert into sat_para_table(sat_name,lnb_num,lof_hi,lof_lo,lof_threshold,signal_22khz,voltage,motor_num,pos_num,lo_direction,la_direction,\
	longitude,latitude,diseqc_mode,tone_burst,committed_cmd,uncommitted_cmd,repeat_count,sequence_repeat,fast_diseqc,cmd_order,sat_longitude) \
	values(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)",
/*QUERY_SRV_BY_LCN,*/	             "select db_id from srv_table where src=? and lcn=?",
/*QUERY_MAX_LCN,*/	             "select max(lcn) from srv_table where src=?",
/*UPDATE_LCN,*/	                     "update srv_table set lcn=?,hd_lcn=?,sd_lcn=? where db_id=?",
/*UPDATE_DEFAULT_CHAN_NUM,*/	     "update srv_table set default_chan_num=? where db_id=?",
/*QUERY_SRV_TYPE,*/	             "select service_type from srv_table where db_id=?",
/*QUERY_MAX_DEFAULT_CHAN_NUM_BY_TYPE,*/     "select max(default_chan_num) from srv_table where service_type=? and src=?",
/*QUERY_MAX_DEFAULT_CHAN_NUM,*/	       "select max(default_chan_num) from srv_table where src=?",
/*QUERY_SRV_BY_SD_LCN,*/	               "select db_id,hd_lcn from srv_table where src=? and sd_lcn=?",
/*QUERY_SRV_BY_DEF_CHAN_NUM_ORDER,*/	       "select db_id,service_type from srv_table where src=? order by default_chan_num",
/*QUERY_SRV_BY_LCN_ORDER,*/	               "select db_id,service_type from srv_table where src=? order by lcn",
/*UPDATE_ANALOG_TS,*/	                     "update ts_table set freq=?,std=?,aud_mode=? where db_id=?",
/*UPDATE_SAT_PARA,*/	                     "update sat_para_table set sat_name=?,lof_hi=?,lof_lo=?,lof_threshold=?,signal_22khz=?,\
	voltage=?,lo_direction=?,la_direction=?,longitude=?,latitude=?,diseqc_mode=?,\
	tone_burst=?,committed_cmd=?,uncommitted_cmd=?,repeat_count=?,sequence_repeat=?,\
	fast_diseqc=?,cmd_order=?,lnb_num=?,motor_num=?,pos_num=? where db_id=?",
/*QUERY_SAT_TP_BY_POLAR,*/	             "select db_id,freq,symb from ts_table where src=? and db_sat_para_id=? and polar=?",
/*UPDATE_TS_FREQ_SYMB,*/	             "update ts_table set freq=?, symb=? where db_id=?",
/*QUERY_DVBS_SAT_BY_SAT_ORDER,*/	     "select db_id from sat_para_table order by db_id",
/*QUERY_DVBS_TP_BY_FREQ_ORDER,*/	     "select db_id from ts_table where db_sat_para_id=? order by freq",
/*QUERY_EXIST_SRV_BY_CHAN_ORDER,*/	     "select db_id,service_type from srv_table where src=? and chan_num>0 order by chan_order",
/*QUERY_NONEXIST_SRV_BY_SRV_ID_ORDER,*/    "select db_id,service_type from srv_table where db_ts_id=? and chan_num<=0 order by service_id",
/*DELETE_TS_EVTS_BOOKINGS,*/	             "delete from booking_table where db_evt_id in (select db_id from evt_table where db_ts_id=?)",
/*SELECT_DBTSID_BY_NUM_AND_SRC,*/	     "select db_ts_id,db_id from srv_table where src = ? and chan_order = ?",
/*UPDATE_TS_FREQ,*/	                     "update ts_table set db_net_id=?,ts_id=?,symb=?,mod=?,bw=?,snr=?,ber=?,strength=?,std=?,aud_mode=?,dvbt_flag=?,flags=0,freq=? where db_id=?",
};

/****************************************************************************
 * static functions
 ***************************************************************************/
static AM_ErrorCode_t am_scan_start_ts(AM_SCAN_Scanner_t *scanner, int step);
static AM_ErrorCode_t am_scan_start_next_ts(AM_SCAN_Scanner_t *scanner);
static AM_ErrorCode_t am_scan_start_current_ts(AM_SCAN_Scanner_t *scanner);
static AM_ErrorCode_t am_scan_request_section(AM_SCAN_Scanner_t *scanner, AM_SCAN_TableCtl_t *scl);
static AM_ErrorCode_t am_scan_request_pmts(AM_SCAN_Scanner_t *scanner);
static AM_ErrorCode_t am_scan_request_next_pmt(AM_SCAN_Scanner_t *scanner);
static AM_ErrorCode_t am_scan_try_nit(AM_SCAN_Scanner_t *scanner);
static AM_ErrorCode_t am_scan_start_dtv(AM_SCAN_Scanner_t *scanner);
static AM_ErrorCode_t am_scan_start_atv(AM_SCAN_Scanner_t *scanner);
static AM_ErrorCode_t am_scan_isdbt_prepare_oneseg(AM_SCAN_TS_t *ts);
static void am_scan_check_need_pause(AM_SCAN_Scanner_t *scanner, int pause_flag);

extern AM_ErrorCode_t AM_SI_ConvertDVBTextCode(char *in_code,int in_len,char *out_code,int out_len);

int  am_audiostd_to_enmu(int data);
int  am_videostd_to_enmu(int data);







static void am_scan_rec_tab_init(AM_SCAN_RecTab_t *tab)
{
	memset(tab, 0, sizeof(AM_SCAN_RecTab_t));
}

static void am_scan_rec_tab_release(AM_SCAN_RecTab_t *tab)
{
	if(tab->srv_ids){
		free(tab->srv_ids);
		free(tab->tses);
	}
}

static int am_scan_rec_tab_add_srv(AM_SCAN_RecTab_t *tab, int id, AM_SCAN_TS_t *ts)
{
	int i;

	for(i=0; i<tab->srv_cnt; i++){
		if(tab->srv_ids[i]==id)
			return 0;
	}

	if(tab->srv_cnt == tab->buf_size){
		int size = AM_MAX(tab->buf_size*2, 32);
		int *buf, *buf1;

		buf = realloc(tab->srv_ids, sizeof(int)*size);
		if(!buf)
			return -1;
		buf1 = realloc(tab->tses, sizeof(AM_SCAN_TS_t)*size);
		if(!buf1) {
			free(buf);
			return -1;
		}

		tab->buf_size = size;
		tab->srv_ids  = buf;
		tab->tses = (AM_SCAN_TS_t **)buf1;
	}

	tab->srv_ids[tab->srv_cnt] = id;
	tab->tses[tab->srv_cnt++] = ts;

	return 0;
}

static int am_scan_rec_tab_have_src(AM_SCAN_RecTab_t *tab, int id)
{
	int i;

	for(i=0; i<tab->srv_cnt; i++){
		if(tab->srv_ids[i]==id)
			return 1;
	}

	return 0;
}

static int am_scan_format_atv_freq(int tmp_freq)
{
	int tmp_val = 0;

	tmp_val = (tmp_freq % ATV_1MHZ) / ATV_10KHZ;

	if (tmp_val >= 0 && tmp_val <= 30)
	{
		tmp_val = 25;
	}
	else if (tmp_val >= 70 && tmp_val <= 80)
	{
		tmp_val = 75;
	}

	tmp_freq = (tmp_freq / ATV_1MHZ) * ATV_1MHZ + tmp_val * ATV_10KHZ;

	return tmp_freq;
}




/**\brief 插入一个网络记录，返回其索引*/
static int insert_net(sqlite3_stmt **stmts, int src, int orig_net_id)
{
	int db_id = -1;

	/*query wether it exists*/
	sqlite3_bind_int(stmts[QUERY_NET], 1, src);
	sqlite3_bind_int(stmts[QUERY_NET], 2, orig_net_id);
	if (sqlite3_step(stmts[QUERY_NET]) == SQLITE_ROW)
	{
		db_id = sqlite3_column_int(stmts[QUERY_NET], 0);
	}
	sqlite3_reset(stmts[QUERY_NET]);

	/*if not exist , insert a new record*/
	if (db_id == -1)
	{
		sqlite3_bind_int(stmts[INSERT_NET], 1, orig_net_id);
		sqlite3_bind_int(stmts[INSERT_NET], 2, src);
		if (sqlite3_step(stmts[INSERT_NET]) == SQLITE_DONE)
		{
			sqlite3_bind_int(stmts[QUERY_NET], 1, src);
			sqlite3_bind_int(stmts[QUERY_NET], 2, orig_net_id);
			if (sqlite3_step(stmts[QUERY_NET]) == SQLITE_ROW)
			{
				db_id = sqlite3_column_int(stmts[QUERY_NET], 0);
			}
			sqlite3_reset(stmts[QUERY_NET]);
		}
		sqlite3_reset(stmts[INSERT_NET]);
	}

	return db_id;
}

/**\brief 插入一个TS记录，返回其索引*/
static int insert_ts(sqlite3_stmt **stmts, int src, int freq, int symb, int db_satpara_id, int polar)
{
	int db_id = -1;

	/*query wether it exists*/
	if (src != FE_QPSK)
	{
		sqlite3_bind_int(stmts[QUERY_TS], 1, src);
		sqlite3_bind_int(stmts[QUERY_TS], 2, freq);
		sqlite3_bind_int(stmts[QUERY_TS], 3, db_satpara_id);
		sqlite3_bind_int(stmts[QUERY_TS], 4, polar);
		if (sqlite3_step(stmts[QUERY_TS]) == SQLITE_ROW)
		{
			db_id = sqlite3_column_int(stmts[QUERY_TS], 0);
		}
		sqlite3_reset(stmts[QUERY_TS]);
	}
	else
	{
		int efreq, esymb;
		AM_DEBUG(1, "%d/%d, db_sat_id %d, polar %d", freq, symb, db_satpara_id, polar);
		/* In dvbs, we must check the same tp */
		sqlite3_bind_int(stmts[QUERY_SAT_TP_BY_POLAR], 1, src);
		sqlite3_bind_int(stmts[QUERY_SAT_TP_BY_POLAR], 2, db_satpara_id);
		sqlite3_bind_int(stmts[QUERY_SAT_TP_BY_POLAR], 3, polar);
		while (sqlite3_step(stmts[QUERY_SAT_TP_BY_POLAR]) == SQLITE_ROW)
		{
			db_id = sqlite3_column_int(stmts[QUERY_SAT_TP_BY_POLAR], 0);
			efreq = sqlite3_column_int(stmts[QUERY_SAT_TP_BY_POLAR], 1);
			esymb = sqlite3_column_int(stmts[QUERY_SAT_TP_BY_POLAR], 2);
			if( (AM_ABSSUB(efreq, freq) * 833) < AM_MIN(symb, esymb))
			{
				AM_DEBUG(1, "DVBS Same TP, current(%d/%d), exist(%d/%d)", freq, symb, efreq, esymb);
				sqlite3_bind_int(stmts[UPDATE_TS_FREQ_SYMB], 1, freq);
				sqlite3_bind_int(stmts[UPDATE_TS_FREQ_SYMB], 2, symb);
				sqlite3_bind_int(stmts[UPDATE_TS_FREQ_SYMB], 3, db_id);
				sqlite3_step(stmts[UPDATE_TS_FREQ_SYMB]);
				sqlite3_reset(stmts[UPDATE_TS_FREQ_SYMB]);
				sqlite3_reset(stmts[QUERY_SAT_TP_BY_POLAR]);
				return db_id;
			}
		}
		sqlite3_reset(stmts[QUERY_SAT_TP_BY_POLAR]);
		db_id = -1;
	}
	/*if not exist , insert a new record*/
	if (db_id == -1)
	{
		sqlite3_bind_int(stmts[INSERT_TS], 1, src);
		sqlite3_bind_int(stmts[INSERT_TS], 2, freq);
		sqlite3_bind_int(stmts[INSERT_TS], 3, db_satpara_id);
		sqlite3_bind_int(stmts[INSERT_TS], 4, polar);
		if (sqlite3_step(stmts[INSERT_TS]) == SQLITE_DONE)
		{
			sqlite3_bind_int(stmts[QUERY_TS], 1, src);
			sqlite3_bind_int(stmts[QUERY_TS], 2, freq);
			sqlite3_bind_int(stmts[QUERY_TS], 3, db_satpara_id);
			sqlite3_bind_int(stmts[QUERY_TS], 4, polar);
			if (sqlite3_step(stmts[QUERY_TS]) == SQLITE_ROW)
			{
				db_id = sqlite3_column_int(stmts[QUERY_TS], 0);
			}
			sqlite3_reset(stmts[QUERY_TS]);
		}
		sqlite3_reset(stmts[INSERT_TS]);
	}

	return db_id;
}

/**\brief 插入一个SRV记录，返回其索引*/
static int insert_srv(sqlite3_stmt **stmts, int db_net_id, int db_ts_id, int plp_id, int srv_id)
{
	int db_id = -1;

	/*query wether it exists*/
	sqlite3_bind_int(stmts[QUERY_SRV], 1, db_net_id);
	sqlite3_bind_int(stmts[QUERY_SRV], 2, db_ts_id);
	sqlite3_bind_int(stmts[QUERY_SRV], 3, plp_id);
	sqlite3_bind_int(stmts[QUERY_SRV], 4, srv_id);
	if (sqlite3_step(stmts[QUERY_SRV]) == SQLITE_ROW)
	{
		db_id = sqlite3_column_int(stmts[QUERY_SRV], 0);
	}
	sqlite3_reset(stmts[QUERY_SRV]);

	/*if not exist , insert a new record*/
	if (db_id == -1)
	{
		sqlite3_bind_int(stmts[INSERT_SRV], 1, db_net_id);
		sqlite3_bind_int(stmts[INSERT_SRV], 2, db_ts_id);
		sqlite3_bind_int(stmts[INSERT_SRV], 3, plp_id);
		sqlite3_bind_int(stmts[INSERT_SRV], 4, srv_id);
		if (sqlite3_step(stmts[INSERT_SRV]) == SQLITE_DONE)
		{
			sqlite3_bind_int(stmts[QUERY_SRV], 1, db_net_id);
			sqlite3_bind_int(stmts[QUERY_SRV], 2, db_ts_id);
			sqlite3_bind_int(stmts[QUERY_SRV], 3, plp_id);
			sqlite3_bind_int(stmts[QUERY_SRV], 4, srv_id);
			if (sqlite3_step(stmts[QUERY_SRV]) == SQLITE_ROW)
			{
				db_id = sqlite3_column_int(stmts[QUERY_SRV], 0);
			}
			sqlite3_reset(stmts[QUERY_SRV]);
		}
		sqlite3_reset(stmts[INSERT_SRV]);
	}

	return db_id;
}

/**\brief 将audio数据格式化成字符串已便存入数据库*/
static void format_audio_strings(AM_SI_AudioInfo_t *ai, char *pids, char *fmts, char *langs,char *audio_type,char *audio_exten)
{
	int i;

	if (ai->audio_count < 0)
		ai->audio_count = 0;

	pids[0] = 0;
	fmts[0] = 0;
	langs[0] = 0;
	audio_type[0] = 0;
	audio_exten[0] = 0;
	for (i=0; i<ai->audio_count; i++)
	{
		if (i == 0)
		{
			snprintf(pids, 256, "%d", ai->audios[i].pid);
			snprintf(fmts, 256, "%d", ai->audios[i].fmt);
			sprintf(langs, "%s", ai->audios[i].lang);
			snprintf(audio_type, 256, "%d",ai->audios[i].audio_type);
			snprintf(audio_exten, 256, "%d",ai->audios[i].audio_exten);
			//AM_DEBUG(1, "@@ exten=[0x%x][%d]@@",ai->audios[i].audio_exten,ai->audios[i].audio_exten);
		}
		else
		{
			snprintf(pids, 256, "%s %d", pids, ai->audios[i].pid);
			snprintf(fmts, 256, "%s %d", fmts, ai->audios[i].fmt);
			snprintf(langs, 256, "%s %s", langs, ai->audios[i].lang);
			snprintf(audio_type, 256, "%s %d",audio_type,ai->audios[i].audio_type);
			snprintf(audio_exten, 256, "%s %d",audio_exten,ai->audios[i].audio_exten);
			//AM_DEBUG(1, "@@ exten=[0x%x][%d]@@",ai->audios[i].audio_exten,ai->audios[i].audio_exten);
		}
	}
}

static void format_subtitle_strings(AM_SI_SubtitleInfo_t *si, char *pids, char *types, char *cids, char *aids, char *langs)
{
	int i;

	if (si->subtitle_count< 0)
		si->subtitle_count = 0;

	pids[0]  = 0;
	types[0] = 0;
	cids[0]  = 0;
	aids[0]  = 0;
	langs[0] = 0;
	for (i=0; i<si->subtitle_count; i++)
	{
		if (i == 0)
		{
			snprintf(pids, 256,  "%d", si->subtitles[i].pid);
			snprintf(types, 256, "%d", si->subtitles[i].type);
			snprintf(cids, 256,  "%d", si->subtitles[i].comp_page_id);
			snprintf(aids, 256,  "%d", si->subtitles[i].anci_page_id);
			sprintf(langs, "%s", si->subtitles[i].lang);
		}
		else
		{
			snprintf(pids, 256,  "%s %d", pids, si->subtitles[i].pid);
			snprintf(types, 256, "%s %d", types, si->subtitles[i].type);
			snprintf(cids, 256,  "%s %d", cids, si->subtitles[i].comp_page_id);
			snprintf(aids, 256,  "%s %d", aids, si->subtitles[i].anci_page_id);
			snprintf(langs, 256, "%s %s", langs, si->subtitles[i].lang);
		}
	}
}

static void format_teletext_strings(AM_SI_TeletextInfo_t *ti, char *pids, char *types, char *magnos, char *pgnos, char *langs)
{
	int i;

	if (ti->teletext_count< 0)
		ti->teletext_count = 0;

	pids[0]   = 0;
	types[0]  = 0;
	magnos[0] = 0;
	pgnos[0]  = 0;
	langs[0]  = 0;
	for (i=0; i<ti->teletext_count; i++)
	{
		if (i == 0)
		{
			snprintf(pids, 256,   "%d", ti->teletexts[i].pid);
			snprintf(types, 256,  "%d", ti->teletexts[i].type);
			snprintf(magnos, 256, "%d", ti->teletexts[i].magazine_no);
			snprintf(pgnos,256,   "%d", ti->teletexts[i].page_no);
			sprintf(langs,  "%s", ti->teletexts[i].lang);
		}
		else
		{
			snprintf(pids, 256,   "%s %d", pids, ti->teletexts[i].pid);
			snprintf(types, 256,  "%s %d", types, ti->teletexts[i].type);
			snprintf(magnos, 256, "%s %d", magnos, ti->teletexts[i].magazine_no);
			snprintf(pgnos, 256,  "%s %d", pgnos, ti->teletexts[i].page_no);
			snprintf(langs, 256,  "%s %s", langs, ti->teletexts[i].lang);
		}
	}
}

/**\brief 查询一个satellite parameter记录是否已经存在 */
static int get_sat_para_record(sqlite3_stmt **stmts, AM_SCAN_DTVSatellitePara_t *sat_para)
{
	sqlite3_stmt *stmt;
	int db_id = -1;

	if (sat_para->sec.m_lnbs.m_cursat_parameters.m_rotorPosNum > 0)
	{
		/* by motor_num + position_num */
		stmt = stmts[QUERY_SAT_PARA_BY_POS_NUM];
		sqlite3_bind_int(stmt, 1, sat_para->lnb_num);
		sqlite3_bind_int(stmt, 2, sat_para->motor_num);
		sqlite3_bind_int(stmt, 3, sat_para->sec.m_lnbs.m_cursat_parameters.m_rotorPosNum);
	}
	else
	{
		/* by satellite longitude*/
		stmt = stmts[QUERY_SAT_PARA_BY_LO_LA];
		sqlite3_bind_int(stmt, 1, sat_para->lnb_num);
		sqlite3_bind_double(stmt, 2, sat_para->sec.m_lnbs.m_rotor_parameters.m_gotoxx_parameters.m_sat_longitude);
	}

	if (sqlite3_step(stmt) == SQLITE_ROW)
	{
		db_id = sqlite3_column_int(stmt, 0);
	}
	sqlite3_reset(stmt);

	return db_id;
}

/**\brief 添加一个satellite parameter记录，如果已存在，则返回已存在的记录db_id*/
static int insert_sat_para(sqlite3_stmt **stmts, AM_SCAN_DTVSatellitePara_t *sat_para)
{
	sqlite3_stmt *stmt;
	int db_id = -1;

	/* Already exist ? */
	db_id = get_sat_para_record(stmts, sat_para);

	/* insert a new record */
	if (db_id == -1)
	{
		AM_DEBUG(1, "@@ Insert a new satellite parameter record !!! @@");
		AM_DEBUG(1, "name(%s),lnb_num(%d),lof_hi(%d),lof_lo(%d),lof_threshold(%d),22k(%d),vol(%d),motor(%d),pos_num(%d),lodir(%d),ladir(%d),\
		lo(%f),la(%f),diseqc_mode(%d),toneburst(%d),cdc(%d),ucdc(%d),repeats(%d),seq_repeat(%d),fast_diseqc(%d),cmd_order(%d), sat_longitude(%d)",
		sat_para->sat_name,sat_para->lnb_num,sat_para->sec.m_lnbs.m_lof_hi,sat_para->sec.m_lnbs.m_lof_lo,sat_para->sec.m_lnbs.m_lof_threshold,
		sat_para->sec.m_lnbs.m_cursat_parameters.m_22khz_signal,sat_para->sec.m_lnbs.m_cursat_parameters.m_voltage_mode,
		sat_para->motor_num,sat_para->sec.m_lnbs.m_cursat_parameters.m_rotorPosNum,
		0,
		0,
		sat_para->sec.m_lnbs.m_rotor_parameters.m_gotoxx_parameters.m_longitude,
		sat_para->sec.m_lnbs.m_rotor_parameters.m_gotoxx_parameters.m_latitude,
		sat_para->sec.m_lnbs.m_diseqc_parameters.m_diseqc_mode,
		sat_para->sec.m_lnbs.m_diseqc_parameters.m_toneburst_param,
		sat_para->sec.m_lnbs.m_diseqc_parameters.m_committed_cmd,
		sat_para->sec.m_lnbs.m_diseqc_parameters.m_uncommitted_cmd,
		sat_para->sec.m_lnbs.m_diseqc_parameters.m_repeats,
		sat_para->sec.m_lnbs.m_diseqc_parameters.m_seq_repeat,
		sat_para->sec.m_lnbs.m_diseqc_parameters.m_use_fast,
		sat_para->sec.m_lnbs.m_diseqc_parameters.m_command_order,
		sat_para->sec.m_lnbs.m_rotor_parameters.m_gotoxx_parameters.m_sat_longitude);
		stmt = stmts[INSERT_SAT_PARA];
		sqlite3_bind_text(stmt, 1, sat_para->sat_name, strlen(sat_para->sat_name), SQLITE_STATIC);
		sqlite3_bind_int(stmt, 2, sat_para->lnb_num);
		sqlite3_bind_int(stmt, 3, sat_para->sec.m_lnbs.m_lof_hi);
		sqlite3_bind_int(stmt, 4, sat_para->sec.m_lnbs.m_lof_lo);
		sqlite3_bind_int(stmt, 5, sat_para->sec.m_lnbs.m_lof_threshold);
		sqlite3_bind_int(stmt, 6, sat_para->sec.m_lnbs.m_cursat_parameters.m_22khz_signal);
		sqlite3_bind_int(stmt, 7, sat_para->sec.m_lnbs.m_cursat_parameters.m_voltage_mode);
		sqlite3_bind_int(stmt, 8, sat_para->motor_num);
		sqlite3_bind_int(stmt, 9, sat_para->sec.m_lnbs.m_cursat_parameters.m_rotorPosNum);
		sqlite3_bind_int(stmt, 10, 0);
		sqlite3_bind_int(stmt, 11, 0);
		sqlite3_bind_double(stmt, 12, sat_para->sec.m_lnbs.m_rotor_parameters.m_gotoxx_parameters.m_longitude);
		sqlite3_bind_double(stmt, 13, sat_para->sec.m_lnbs.m_rotor_parameters.m_gotoxx_parameters.m_latitude);
		sqlite3_bind_int(stmt, 14, sat_para->sec.m_lnbs.m_diseqc_parameters.m_diseqc_mode);
		sqlite3_bind_int(stmt, 15, sat_para->sec.m_lnbs.m_diseqc_parameters.m_toneburst_param);
		sqlite3_bind_int(stmt, 16, sat_para->sec.m_lnbs.m_diseqc_parameters.m_committed_cmd);
		sqlite3_bind_int(stmt, 17, sat_para->sec.m_lnbs.m_diseqc_parameters.m_uncommitted_cmd);
		sqlite3_bind_int(stmt, 18, sat_para->sec.m_lnbs.m_diseqc_parameters.m_repeats);
		sqlite3_bind_int(stmt, 19, sat_para->sec.m_lnbs.m_diseqc_parameters.m_seq_repeat ? 1 : 0);
		sqlite3_bind_int(stmt, 20, sat_para->sec.m_lnbs.m_diseqc_parameters.m_use_fast ? 1 : 0);
		sqlite3_bind_int(stmt, 21, sat_para->sec.m_lnbs.m_diseqc_parameters.m_command_order);
		sqlite3_bind_int(stmt, 22, sat_para->sec.m_lnbs.m_rotor_parameters.m_gotoxx_parameters.m_sat_longitude);

		if (sqlite3_step(stmt) == SQLITE_DONE)
			db_id = get_sat_para_record(stmts, sat_para);

		sqlite3_reset(stmt);
	}

	return db_id;
}

static void update_sat_para(int db_id, sqlite3_stmt **stmts, AM_SCAN_DTVSatellitePara_t *sat_para)
{
	sqlite3_stmt *stmt;

	stmt = stmts[UPDATE_SAT_PARA];
	sqlite3_bind_text(stmt, 1, sat_para->sat_name, strlen(sat_para->sat_name), SQLITE_STATIC);
	sqlite3_bind_int(stmt, 2, sat_para->sec.m_lnbs.m_lof_hi/1000);
	sqlite3_bind_int(stmt, 3, sat_para->sec.m_lnbs.m_lof_lo/1000);
	sqlite3_bind_int(stmt, 4, sat_para->sec.m_lnbs.m_lof_threshold/1000);
	sqlite3_bind_int(stmt, 5, sat_para->sec.m_lnbs.m_cursat_parameters.m_22khz_signal);
	sqlite3_bind_int(stmt, 6, sat_para->sec.m_lnbs.m_cursat_parameters.m_voltage_mode);
	sqlite3_bind_int(stmt, 7, 0); /* Not used */
	sqlite3_bind_int(stmt, 8, 0);  /* Not used */
	sqlite3_bind_double(stmt, 9, sat_para->sec.m_lnbs.m_rotor_parameters.m_gotoxx_parameters.m_longitude);
	sqlite3_bind_double(stmt, 10, sat_para->sec.m_lnbs.m_rotor_parameters.m_gotoxx_parameters.m_latitude);
	sqlite3_bind_int(stmt, 11, sat_para->sec.m_lnbs.m_diseqc_parameters.m_diseqc_mode);
	sqlite3_bind_int(stmt, 12, sat_para->sec.m_lnbs.m_diseqc_parameters.m_toneburst_param);
	sqlite3_bind_int(stmt, 13, sat_para->sec.m_lnbs.m_diseqc_parameters.m_committed_cmd);
	sqlite3_bind_int(stmt, 14, sat_para->sec.m_lnbs.m_diseqc_parameters.m_uncommitted_cmd);
	sqlite3_bind_int(stmt, 15, sat_para->sec.m_lnbs.m_diseqc_parameters.m_repeats);
	sqlite3_bind_int(stmt, 16, sat_para->sec.m_lnbs.m_diseqc_parameters.m_seq_repeat ? 1 : 0);
	sqlite3_bind_int(stmt, 17, sat_para->sec.m_lnbs.m_diseqc_parameters.m_use_fast ? 1 : 0);
	sqlite3_bind_int(stmt, 18, sat_para->sec.m_lnbs.m_diseqc_parameters.m_command_order);
	sqlite3_bind_int(stmt, 19, 0);  /* Not used */
	sqlite3_bind_int(stmt, 20, sat_para->motor_num);
	sqlite3_bind_int(stmt, 21, sat_para->sec.m_lnbs.m_cursat_parameters.m_rotorPosNum);
	sqlite3_bind_int(stmt, 22, db_id);
	sqlite3_step(stmt);

	sqlite3_reset(stmt);
}

/**\brief 获取一个TS所在网络*/
static int am_scan_get_ts_network(sqlite3_stmt **stmts, int src, dvbpsi_nit_t *nits, AM_SCAN_TS_t *ts)
{
	int orig_net_id = -1, net_dbid = -1;
	dvbpsi_nit_t *nit;
	dvbpsi_descriptor_t *descr;

	if (ts->type == AM_SCAN_TS_ANALOG)
		return net_dbid;

	/*获取所在网络*/
	if (ts->digital.sdts)
	{
		/*按照SDT中描述的orignal_network_id查找网络记录，不存在则新添加一个network记录*/
		orig_net_id = ts->digital.sdts->i_network_id;
	}
	else if (IS_DVBT2_TS(ts->digital.fend_para) && ts->digital.dvbt2_data_plp_num > 0 && ts->digital.dvbt2_data_plps[0].sdts)
	{
		orig_net_id = ts->digital.dvbt2_data_plps[0].sdts->i_network_id;
	}

	if (orig_net_id != -1)
	{
		net_dbid = insert_net(stmts, src, orig_net_id);
		if (net_dbid != -1)
		{
			char netname[256];

			netname[0] = '\0';
			/*查找网络名称*/
			if (nits && (orig_net_id == nits->i_network_id))
			{
				AM_SI_LIST_BEGIN(nits, nit)
				AM_SI_LIST_BEGIN(nit->p_first_descriptor, descr)
				if (descr->p_decoded && descr->i_tag == AM_SI_DESCR_NETWORK_NAME)
				{
					dvbpsi_network_name_dr_t *pnn = (dvbpsi_network_name_dr_t*)descr->p_decoded;

					/*取网络名称*/
					if (descr->i_length > 0)
					{
						AM_SI_ConvertDVBTextCode((char*)pnn->i_network_name, descr->i_length,\
									netname, 255);
						netname[255] = 0;
						break;
					}
				}
				AM_SI_LIST_END()
				AM_SI_LIST_END()
			}

			AM_DEBUG(1, "###Network Name is '%s'", netname);
			sqlite3_bind_text(stmts[UPDATE_NET], 1, netname, -1, SQLITE_STATIC);
			sqlite3_bind_int(stmts[UPDATE_NET], 2, net_dbid);
			sqlite3_step(stmts[UPDATE_NET]);
			sqlite3_reset(stmts[UPDATE_NET]);
		}
		else
		{
			AM_DEBUG(1, "insert new network error");
		}
	}

	return net_dbid;
}

static dvbpsi_pat_t *get_valid_pats(AM_SCAN_TS_t *ts)
{
	dvbpsi_pat_t *valid_pat = NULL;

	if (!IS_DVBT2_TS(ts->digital.fend_para))
	{
		valid_pat = ts->digital.pats;
	}
	else if (IS_ISDBT_TS(ts->digital.fend_para))
	{
		/* process for isdbt one-seg inserted PAT, which ts_id is 0xffff */
		valid_pat = ts->digital.pats;
		while (valid_pat != NULL && valid_pat->i_ts_id == 0xffff)
		{
			valid_pat = valid_pat->p_next;
		}

		if (valid_pat == NULL && ts->digital.pats != NULL)
		{
			valid_pat = ts->digital.pats;

			if (ts->digital.sdts != NULL)
				valid_pat->i_ts_id = ts->digital.sdts->i_ts_id;
		}
	}
	else
	{
		int plp;

		for (plp=0; plp<ts->digital.dvbt2_data_plp_num; plp++)
		{
			if (ts->digital.dvbt2_data_plps[plp].pats != NULL)
			{
				valid_pat = ts->digital.dvbt2_data_plps[plp].pats;
				break;
			}
		}
	}

	return valid_pat;
}

/**\brief check program is need skip*/
static int am_scan_check_skip(AM_SCAN_ServiceInfo_t *srv_info, int mode)
{
	int pkt_num;
	int i;
	int ret = 1;
	/* Skip program for FTA mode */
	if (srv_info->scrambled_flag && (mode & AM_SCAN_DTVMODE_FTA))
	{
		AM_DEBUG(1, "Skip program '%s' vid[%d]for FTA mode", srv_info->name, srv_info->vid);
		return ret;
	}
	/* Skip program for vct hide is set 1, we need hide this channel */
	if (srv_info->hidden == 1 && (mode & AM_SCAN_DTVMODE_NOVCTHIDE))
	{
		AM_DEBUG(1, "Skip program '%s' vid[%d]for vct hide mode", srv_info->name, srv_info->vid);
		return ret;
	}

	/* Skip program for service_type mode */
	if (srv_info->srv_type == AM_SCAN_SRV_DTV && (mode & AM_SCAN_DTVMODE_NOTV))
	{
		AM_DEBUG(1, "Skip program '%s' for NO-TV mode", srv_info->name);
		return ret;
	}
	if (srv_info->srv_type == AM_SCAN_SRV_DRADIO && (mode & AM_SCAN_DTVMODE_NORADIO))
	{
		AM_DEBUG(1, "Skip program '%s' for NO-RADIO mode", srv_info->name);
		return ret;
	}
	if (srv_info->srv_type == AM_SCAN_SRV_UNKNOWN &&
		(mode & AM_SCAN_DTVMODE_INVALIDPID)) {
		AM_DEBUG(1, "Skip program '%s' for NO-valid pid mode", srv_info->name);
		return ret;
	}
	if (mode & AM_SCAN_DTVMODE_CHECKDATA) {
		/*check has ts package of video pid */
		AM_Check_Has_Tspackage(srv_info->vid, &pkt_num);
		/*if no video ts package, need check has ts package of audio pid */
		if (!pkt_num) {
			for (i = 0; i < srv_info->aud_info.audio_count; i++) {
				AM_Check_Has_Tspackage(srv_info->aud_info.audios[i].pid, &pkt_num);
			}
		}
		if (!pkt_num) {
			AM_DEBUG(1, "Skip program '%s' for NO-ts package", srv_info->name);
			return ret;
		}
	}
	/*only check has ts package of audio pid, turn the track with data into first place*/
	if (mode & AM_SCAN_DTVMODE_CHECK_AUDIODATA) {
		int pid,fmt,audio_type,audio_exten;
		char lang[10] = {0};
	for (i = 0; i < srv_info->aud_info.audio_count; i++) {
		pkt_num = 0;
		AM_Check_Has_Tspackage(srv_info->aud_info.audios[i].pid, &pkt_num);
		if (pkt_num) {
		if (i == 0) {
			AM_DEBUG(1,"first index hasdata,needn't insert, pid[%d]=%d",i,srv_info->aud_info.audios[i].pid);
		}else {
			pid = srv_info->aud_info.audios[0].pid;
			fmt = srv_info->aud_info.audios[0].fmt;
			memcpy(lang, srv_info->aud_info.audios[0].lang, 10);
			audio_type = srv_info->aud_info.audios[0].audio_type;
			audio_exten = srv_info->aud_info.audios[0].audio_exten;
			srv_info->aud_info.audios[0].pid = srv_info->aud_info.audios[i].pid;
			srv_info->aud_info.audios[0].fmt = srv_info->aud_info.audios[i].fmt;
			memcpy(srv_info->aud_info.audios[0].lang, srv_info->aud_info.audios[i].lang, 10);
			srv_info->aud_info.audios[0].audio_type = srv_info->aud_info.audios[i].audio_type;
			srv_info->aud_info.audios[0].audio_exten = srv_info->aud_info.audios[i].audio_exten;
			srv_info->aud_info.audios[i].pid = pid;
			srv_info->aud_info.audios[i].fmt = fmt;
			memcpy(srv_info->aud_info.audios[i].lang, lang, 10);
			srv_info->aud_info.audios[i].audio_type = audio_type;
			srv_info->aud_info.audios[i].audio_exten = audio_exten;
		}
		break;
	}
	}
	}

	ret = 0;
	return ret;
}
/***\brief 更新一个TS数据*/
static void am_scan_update_ts(sqlite3_stmt **stmts, AM_SCAN_Result_t *result, int net_dbid, int dbid, AM_SCAN_TS_t *ts)
{
	int ts_id = -1;
	int symbol_rate=0, modulation=0, dvbt_flag=0;

	sqlite3_bind_int(stmts[UPDATE_TS_FREQ], 1, net_dbid);
	sqlite3_bind_int(stmts[UPDATE_TS_FREQ], 2, ts_id);
	sqlite3_bind_int(stmts[UPDATE_TS_FREQ], 3, symbol_rate);
	sqlite3_bind_int(stmts[UPDATE_TS_FREQ], 4, modulation);
	sqlite3_bind_int(stmts[UPDATE_TS_FREQ], 5, (ts->type!=AM_SCAN_TS_ANALOG) ? (int)dvb_fend_para(ts->digital.fend_para)->u.ofdm.bandwidth : 0);
	sqlite3_bind_int(stmts[UPDATE_TS_FREQ], 6, (ts->type!=AM_SCAN_TS_ANALOG) ? ts->digital.snr : 0);
	sqlite3_bind_int(stmts[UPDATE_TS_FREQ], 7, (ts->type!=AM_SCAN_TS_ANALOG) ? ts->digital.ber : 0);
	sqlite3_bind_int(stmts[UPDATE_TS_FREQ], 8, (ts->type!=AM_SCAN_TS_ANALOG) ? ts->digital.strength : 0);
	sqlite3_bind_int(stmts[UPDATE_TS_FREQ], 9, (ts->type==AM_SCAN_TS_ANALOG) ? ts->analog.std : -1);
	sqlite3_bind_int(stmts[UPDATE_TS_FREQ], 10,(ts->type==AM_SCAN_TS_ANALOG) ?  1/*Stereo*/ : -1);
	sqlite3_bind_int(stmts[UPDATE_TS_FREQ], 11, dvbt_flag);
	sqlite3_bind_int(stmts[UPDATE_TS_FREQ], 12, ts->analog.freq);
	sqlite3_bind_int(stmts[UPDATE_TS_FREQ], 13, dbid);

	sqlite3_step(stmts[UPDATE_TS_FREQ]);
	sqlite3_reset(stmts[UPDATE_TS_FREQ]);
}

/***\brief 更新一个TS数据*/
static void am_scan_update_ts_info(sqlite3_stmt **stmts, AM_SCAN_Result_t *result, int net_dbid, int dbid, AM_SCAN_TS_t *ts)
{
	int ts_id = -1;
	int symbol_rate=0, modulation=0, dvbt_flag=0;

	/*更新TS数据*/
	sqlite3_bind_int(stmts[UPDATE_TS], 1, net_dbid);
	if (ts->type != AM_SCAN_TS_ANALOG)
	{
		dvbpsi_pat_t *pats = get_valid_pats(ts);
		if (pats != NULL && !ts->digital.use_vct_tsid)
			ts_id = pats->i_ts_id;
		else if (ts->digital.vcts != NULL)
			ts_id = ts->digital.vcts->i_extension;

		if (ts->digital.fend_para.m_type == FE_QAM)
		{
			symbol_rate = (int)dvb_fend_para(ts->digital.fend_para)->u.qam.symbol_rate;
			modulation = (int)dvb_fend_para(ts->digital.fend_para)->u.qam.modulation;
		}
		else if (ts->digital.fend_para.m_type == FE_QPSK)
		{
			symbol_rate = (int)dvb_fend_para(ts->digital.fend_para)->u.qpsk.symbol_rate;
		}
		else if (ts->digital.fend_para.m_type == FE_ATSC)
		{
			modulation = (int)dvb_fend_para(ts->digital.fend_para)->u.vsb.modulation;
		}
		AM_DEBUG(1, "Update ts: type %d, modulation %d", ts->digital.fend_para.m_type, modulation);
	}

	sqlite3_bind_int(stmts[UPDATE_TS], 2, ts_id);
	sqlite3_bind_int(stmts[UPDATE_TS], 3, symbol_rate);
	sqlite3_bind_int(stmts[UPDATE_TS], 4, modulation);
	sqlite3_bind_int(stmts[UPDATE_TS], 5, (ts->type!=AM_SCAN_TS_ANALOG) ? (int)dvb_fend_para(ts->digital.fend_para)->u.ofdm.bandwidth : 0);
	sqlite3_bind_int(stmts[UPDATE_TS], 6, (ts->type!=AM_SCAN_TS_ANALOG) ? ts->digital.snr : 0);
	sqlite3_bind_int(stmts[UPDATE_TS], 7, (ts->type!=AM_SCAN_TS_ANALOG) ? ts->digital.ber : 0);
	sqlite3_bind_int(stmts[UPDATE_TS], 8, (ts->type!=AM_SCAN_TS_ANALOG) ? ts->digital.strength : 0);
	sqlite3_bind_int(stmts[UPDATE_TS], 9, (ts->type==AM_SCAN_TS_ANALOG) ? ts->analog.std : -1);
	sqlite3_bind_int(stmts[UPDATE_TS], 10,(ts->type==AM_SCAN_TS_ANALOG) ?  1/*Stereo*/ : -1);
	if (ts->type!=AM_SCAN_TS_ANALOG)
	{
		if (IS_DVBT2_TS(ts->digital.fend_para))
			dvbt_flag = (int)ts->digital.fend_para.terrestrial.ofdm_mode;
		else if (IS_ISDBT_TS(ts->digital.fend_para))
			dvbt_flag = (result->start_para->dtv_para.mode&AM_SCAN_DTVMODE_ISDBT_ONESEG) ? Layer_A : Layer_A_B_C;
	}
	sqlite3_bind_int(stmts[UPDATE_TS], 11, dvbt_flag);
	sqlite3_bind_int(stmts[UPDATE_TS], 12, dbid);
	sqlite3_step(stmts[UPDATE_TS]);
	sqlite3_reset(stmts[UPDATE_TS]);

	/*清除与该TS下service关联的分组数据*/
	sqlite3_bind_int(stmts[DELETE_SRV_GRP], 1, dbid);
	sqlite3_step(stmts[DELETE_SRV_GRP]);
	sqlite3_reset(stmts[DELETE_SRV_GRP]);

	/*清除该TS下所有service*/
	sqlite3_bind_int(stmts[DELETE_TS_SRVS], 1, dbid);
	sqlite3_step(stmts[DELETE_TS_SRVS]);
	sqlite3_reset(stmts[DELETE_TS_SRVS]);

	/*清除该TS下所有events所对应的booings*/
	sqlite3_bind_int(stmts[DELETE_TS_EVTS_BOOKINGS], 1, dbid);
	sqlite3_step(stmts[DELETE_TS_EVTS_BOOKINGS]);
	sqlite3_reset(stmts[DELETE_TS_EVTS_BOOKINGS]);

	/*清除该TS下所有events*/
	AM_DEBUG(1, "Delete all events in TS %d", dbid);
	sqlite3_bind_int(stmts[DELETE_TS_EVTS], 1, dbid);
	sqlite3_step(stmts[DELETE_TS_EVTS]);
	sqlite3_reset(stmts[DELETE_TS_EVTS]);
}

/**\brief 初始化一个service结构*/
static void am_scan_init_service_info(AM_SCAN_ServiceInfo_t *srv_info)
{
	memset(srv_info, 0, sizeof(AM_SCAN_ServiceInfo_t));
	srv_info->vid = 0x1fff;
	srv_info->vfmt = -1;
	srv_info->free_ca = 1;
	srv_info->srv_id = 0xffff;
	srv_info->srv_dbid = -1;
	srv_info->satpara_dbid = -1;
	srv_info->pmt_pid = 0x1fff;
	srv_info->plp_id = -1;
	srv_info->sdt_version = 0xff;
}

/**\brief 更新一个service数据到数据库*/
static void am_scan_update_service_info(sqlite3_stmt **stmts, AM_SCAN_Result_t *result, AM_SCAN_ServiceInfo_t *srv_info)
{
#define str(i) (char*)(strings + i)

	static char strings[15][256];

	if (srv_info->src != FE_ANALOG)
	{
		int standard = result->start_para->dtv_para.standard;
		int mode = result->start_para->dtv_para.mode;

		/* Transform service types for different dtv standards */
		if (standard != AM_SCAN_DTV_STD_ATSC)
		{
			if (srv_info->srv_type == 0x1)
				srv_info->srv_type = AM_SCAN_SRV_DTV;
			else if (srv_info->srv_type == 0x2)
				srv_info->srv_type = AM_SCAN_SRV_DRADIO;
		}
		else
		{
			if (srv_info->srv_type == 0x2)
				srv_info->srv_type = AM_SCAN_SRV_DTV;
			else if (srv_info->srv_type == 0x3)
				srv_info->srv_type = AM_SCAN_SRV_DRADIO;
		}


		/* if video valid, set this program to tv type,
		 * if audio valid, but video not found, set it to radio type,
		 * if both invalid, but service_type found in SDT/VCT, set to unknown service,
		 * this mechanism is OPTIONAL
		 */
		if (srv_info->vid < 0x1fff)
		{
			srv_info->srv_type = AM_SCAN_SRV_DTV;
		}
		else if (srv_info->aud_info.audio_count > 0)
		{
			srv_info->srv_type = AM_SCAN_SRV_DRADIO;
		}
		else if (srv_info->srv_type == AM_SCAN_SRV_DTV ||
			srv_info->srv_type == AM_SCAN_SRV_DRADIO)
		{
			srv_info->srv_type = AM_SCAN_SRV_UNKNOWN;
		}

		if (am_scan_check_skip(srv_info, mode)) {
			return;
		}
		/* Set default name to tv/radio program if no name specified */
		if (!strcmp(srv_info->name, "") &&
			(srv_info->srv_type == AM_SCAN_SRV_DTV ||
			srv_info->srv_type == AM_SCAN_SRV_DRADIO))
		{
			strcpy(srv_info->name, "xxxNo Name");
		}
	}
	if (srv_info->src == FE_ANALOG) {
		int mode = result->start_para->store_mode;
		//not store pal type program
		if (((srv_info->analog_std & V4L2_COLOR_STD_PAL) == V4L2_COLOR_STD_PAL) &&
			(mode & AM_SCAN_ATV_STOREMODE_NOPAL) == AM_SCAN_ATV_STOREMODE_NOPAL) {
			AM_DEBUG(1, "Skip program '%s' for atv pal mode", srv_info->name);
			return;
		}
		if (((srv_info->analog_std & V4L2_COLOR_STD_SECAM) == V4L2_COLOR_STD_SECAM) &&
			(mode & AM_SCAN_DTV_STOREMODE_NOSECAM) == AM_SCAN_DTV_STOREMODE_NOSECAM) {
			AM_DEBUG(1, "Skip program '%s' for atv secam mode", srv_info->name);
			return;
		}
	}
	if (stmts == NULL)
	{
		AM_SCAN_ProgramProgress_t pp;
		AM_SCAN_Scanner_t *scanner = (AM_SCAN_Scanner_t*)result->reserved;

		memset(&pp, 0, sizeof(pp));
		pp.scrambled = !!srv_info->scrambled_flag;
		pp.service_id = srv_info->srv_id;
		pp.service_type = srv_info->srv_type;
		AM_DEBUG(1, "send type  [%d] Program '%s'  mode %d vid: %d", srv_info->srv_type, srv_info->name, srv_info->scrambled_flag, srv_info->vid);
		snprintf(pp.name, sizeof(pp.name), "%s", srv_info->name);
		SET_PROGRESS_EVT(AM_SCAN_PROGRESS_NEW_PROGRAM, &pp);
		return;
	}

	/* Update to database */
	AM_DEBUG(1, "Updating Program: '%s', db_srv_id(%d), srv_type(%d)",
		srv_info->name, srv_info->srv_dbid, srv_info->srv_type);
	sqlite3_bind_int(stmts[UPDATE_SRV], 1, srv_info->src);
	sqlite3_bind_text(stmts[UPDATE_SRV], 2, srv_info->name, strlen(srv_info->name), SQLITE_STATIC);
	sqlite3_bind_int(stmts[UPDATE_SRV], 3, srv_info->srv_type);
	sqlite3_bind_int(stmts[UPDATE_SRV], 4, srv_info->eit_sche);
	sqlite3_bind_int(stmts[UPDATE_SRV], 5, srv_info->eit_pf);
	sqlite3_bind_int(stmts[UPDATE_SRV], 6, srv_info->rs);
	sqlite3_bind_int(stmts[UPDATE_SRV], 7, srv_info->free_ca);
	sqlite3_bind_int(stmts[UPDATE_SRV], 8, 50);
	sqlite3_bind_int(stmts[UPDATE_SRV], 9, AM_AOUT_OUTPUT_DUAL_LEFT);
	AM_DEBUG(1, "Video: pid(%d), fmt(%d)", srv_info->vid,srv_info->vfmt);
	sqlite3_bind_int(stmts[UPDATE_SRV], 10, srv_info->vid);
	sqlite3_bind_int(stmts[UPDATE_SRV], 11, srv_info->vfmt);
	format_audio_strings(&srv_info->aud_info, str(0), str(1), str(2),str(13),str(14));
	AM_DEBUG(1, "Audios: pids(%s), fmts(%s), langs(%s) type(%s), exten(%s)", str(0), str(1), str(2),str(13),str(14));
	sqlite3_bind_text(stmts[UPDATE_SRV], 12, str(0), strlen(str(0)), SQLITE_STATIC);
	sqlite3_bind_text(stmts[UPDATE_SRV], 13, str(1), strlen(str(1)), SQLITE_STATIC);
	sqlite3_bind_text(stmts[UPDATE_SRV], 14, str(2), strlen(str(2)), SQLITE_STATIC);
	sqlite3_bind_text(stmts[UPDATE_SRV], 15, str(13), strlen(str(13)), SQLITE_STATIC);
	format_subtitle_strings(&srv_info->sub_info, str(3), str(4), str(5), str(6), str(7));
	AM_DEBUG(1, "Subtitles: pids(%s), types(%s), cids(%s), aids(%s), langs(%s)",
		str(3), str(4), str(5), str(6), str(7));
	sqlite3_bind_text(stmts[UPDATE_SRV], 16, str(3), strlen(str(3)), SQLITE_STATIC);
	sqlite3_bind_text(stmts[UPDATE_SRV], 17, str(4), strlen(str(4)), SQLITE_STATIC);
	sqlite3_bind_text(stmts[UPDATE_SRV], 18, str(5), strlen(str(5)), SQLITE_STATIC);
	sqlite3_bind_text(stmts[UPDATE_SRV], 19, str(6), strlen(str(6)), SQLITE_STATIC);
	sqlite3_bind_text(stmts[UPDATE_SRV], 20, str(7), strlen(str(7)), SQLITE_STATIC);
	format_teletext_strings(&srv_info->ttx_info, str(8), str(9), str(10), str(11), str(12));
	AM_DEBUG(1, "Teletexts: pids(%s), types(%s), magnos(%s), pgnos(%s), langs(%s)",
		str(8), str(9), str(10), str(11), str(12));
	sqlite3_bind_text(stmts[UPDATE_SRV], 21, str(8), strlen(str(8)), SQLITE_STATIC);
	sqlite3_bind_text(stmts[UPDATE_SRV], 22, str(9), strlen(str(9)), SQLITE_STATIC);
	sqlite3_bind_text(stmts[UPDATE_SRV], 23, str(10), strlen(str(10)), SQLITE_STATIC);
	sqlite3_bind_text(stmts[UPDATE_SRV], 24, str(11), strlen(str(11)), SQLITE_STATIC);
	sqlite3_bind_text(stmts[UPDATE_SRV], 25, str(12), strlen(str(12)), SQLITE_STATIC);
	sqlite3_bind_int(stmts[UPDATE_SRV], 26, srv_info->chan_num);
	sqlite3_bind_int(stmts[UPDATE_SRV], 27, srv_info->major_chan_num);
	sqlite3_bind_int(stmts[UPDATE_SRV], 28, srv_info->minor_chan_num);
	sqlite3_bind_int(stmts[UPDATE_SRV], 29, srv_info->access_controlled);
	sqlite3_bind_int(stmts[UPDATE_SRV], 30, srv_info->hidden);
	sqlite3_bind_int(stmts[UPDATE_SRV], 31, srv_info->hide_guide);
	sqlite3_bind_int(stmts[UPDATE_SRV], 32, srv_info->source_id);
	sqlite3_bind_int(stmts[UPDATE_SRV], 33, 0);
	sqlite3_bind_int(stmts[UPDATE_SRV], 34, srv_info->satpara_dbid);
	sqlite3_bind_int(stmts[UPDATE_SRV], 35, srv_info->scrambled_flag);
	sqlite3_bind_int(stmts[UPDATE_SRV], 36, srv_info->chan_num);
	sqlite3_bind_int(stmts[UPDATE_SRV], 37, srv_info->pmt_pid);
	sqlite3_bind_int(stmts[UPDATE_SRV], 38, srv_info->plp_id);
	sqlite3_bind_int(stmts[UPDATE_SRV], 39, srv_info->sdt_version);
	sqlite3_bind_int(stmts[UPDATE_SRV], 40, srv_info->pcr_pid);
	/*add audio exten*/
	sqlite3_bind_text(stmts[UPDATE_SRV], 41, str(14),strlen(str(14)), SQLITE_STATIC);
	sqlite3_bind_int(stmts[UPDATE_SRV], 42, srv_info->srv_dbid);
	sqlite3_step(stmts[UPDATE_SRV]);
	sqlite3_reset(stmts[UPDATE_SRV]);
}

/**\brief 提取CA加扰标识*/
static void am_scan_extract_ca_scrambled_flag(dvbpsi_descriptor_t *p_first_descriptor, AM_SCAN_ServiceInfo_t *psrv_info, int mode, int *pmt_scramble_flag)
{
	dvbpsi_descriptor_t *descr;
	if (*pmt_scramble_flag == 1) {
		//AM_DEBUG(1, "pmt_scramble_flag is 1, vid( pid %d)", psrv_info->vid);
		psrv_info->scrambled_flag = 1;
		return;
	}

	if ((mode & AM_SCAN_DTVMODE_SCRAMB_TSHEAD)) {
		int i = 0;
		int video_scramb = 0;
		int audio_scramb = 0;
		int pkt_num, scramb_pkt_num;

		AM_Check_Scramb_GetInfo(psrv_info->vid, &pkt_num, &scramb_pkt_num);
		if (scramb_pkt_num * 10 > pkt_num)
			video_scramb = 1;

		for (i = 0; i < psrv_info->aud_info.audio_count; i++) {
			AM_Check_Scramb_GetInfo(psrv_info->aud_info.audios[i].pid, &pkt_num, &scramb_pkt_num);
			if (scramb_pkt_num * 10 > pkt_num) {
				audio_scramb = 1;
				break;
			}
		}

		if (audio_scramb != 0 || video_scramb != 0) {
				psrv_info->scrambled_flag = 1;
				*pmt_scramble_flag = 1;
				//AM_DEBUG(1, "Program set pmt_scramble_flag 1");
			} else {
				psrv_info->scrambled_flag = 0;
				*pmt_scramble_flag = 0;
				//AM_DEBUG(1, "Program set pmt_scramble_flag 0");
			}
		} else {
		/*for ca des mode*/
		AM_SI_LIST_BEGIN(p_first_descriptor, descr)
			if (descr->i_tag == AM_SI_DESCR_CA && ! psrv_info->scrambled_flag)
			{
				//AM_DEBUG(1, "Found CA descr, set scrambled flag to 1");
				psrv_info->scrambled_flag = 1;
				break;
			}
		AM_SI_LIST_END()
	}
}

/**\brief 从SDT表获取相关service信息*/
static void am_scan_extract_srv_info_from_sdt(AM_SCAN_Result_t *result, dvbpsi_sdt_t *sdts, AM_SCAN_ServiceInfo_t *srv_info)
{
	dvbpsi_sdt_service_t *srv;
	dvbpsi_sdt_t *sdt;
	dvbpsi_descriptor_t *descr;
	const uint8_t split = 0x80;
	const int name_size = (int)sizeof(srv_info->name);
	int curr_name_len = 0, tmp_len;
	char name[AM_DB_MAX_SRV_NAME_LEN+1];

	UNUSED(result);

#define COPY_NAME(_s, _slen)\
	AM_MACRO_BEGIN\
		int copy_len = ((curr_name_len+_slen)>=name_size) ? (name_size-curr_name_len) : _slen;\
		if (copy_len > 0){\
			memcpy(srv_info->name+curr_name_len, _s, copy_len);\
			curr_name_len += copy_len;\
		}\
	AM_MACRO_END


	AM_SI_LIST_BEGIN(sdts, sdt)
	AM_SI_LIST_BEGIN(sdt->p_first_service, srv)
		/*从SDT表中查找该service并获取信息*/
		if (srv->i_service_id == srv_info->srv_id)
		{
			AM_DEBUG(1 ,"SDT for service %d found!", srv_info->srv_id);
			srv_info->eit_sche = (uint8_t)srv->b_eit_schedule;
			srv_info->eit_pf = (uint8_t)srv->b_eit_present;
			srv_info->rs = srv->i_running_status;
			srv_info->free_ca = (uint8_t)srv->b_free_ca;
			srv_info->sdt_version = sdt->i_version;

			AM_SI_LIST_BEGIN(srv->p_first_descriptor, descr)
			if (descr->p_decoded && descr->i_tag == AM_SI_DESCR_SERVICE)
			{
				dvbpsi_service_dr_t *psd = (dvbpsi_service_dr_t*)descr->p_decoded;

				if (psd->i_service_name_length > 0)
				{
					name[0] = 0;
					AM_SI_ConvertDVBTextCode((char*)psd->i_service_name, psd->i_service_name_length,\
								name, AM_DB_MAX_SRV_NAME_LEN);
					name[AM_DB_MAX_SRV_NAME_LEN] = 0;

					/*3bytes language code, using xxx to simulate*/
					COPY_NAME("xxx", 3);
					/*following by name text*/
					tmp_len = strlen(name);
					COPY_NAME(name, tmp_len);
				}
				/*业务类型*/
				srv_info->srv_type = psd->i_service_type;
				/*service type 0x16 and 0x19 is user defined, as digital television service*/
				/*service type 0xc0 is type of partial reception service in ISDBT*/
				if((srv_info->srv_type == 0x16) || (srv_info->srv_type == 0x19) || (srv_info->srv_type == 0xc0))
				{
					srv_info->srv_type = 0x1;
				}

				break;
			}
			AM_SI_LIST_END()
#if 0       //The upper layer don't have the logic of parse multi service names according to 0x80, Use only one for the time being.
			/* store multilingual service name */
			AM_SI_LIST_BEGIN(srv->p_first_descriptor, descr)
			if (descr->p_decoded && descr->i_tag == AM_SI_DESCR_MULTI_SERVICE_NAME)
			{
				int i;
				dvbpsi_multi_service_name_dr_t *pmsnd = (dvbpsi_multi_service_name_dr_t*)descr->p_decoded;

				for (i=0; i<pmsnd->i_name_count; i++)
				{
					name[0] = 0;
					AM_SI_ConvertDVBTextCode((char*)pmsnd->p_service_name[i].i_service_name,
						pmsnd->p_service_name[i].i_service_name_length,
						name, AM_DB_MAX_SRV_NAME_LEN);
					name[AM_DB_MAX_SRV_NAME_LEN] = 0;

					if (curr_name_len > 0)
					{
						/*extra split mark*/
						COPY_NAME(&split, 1);
					}
					/*3bytes language code*/
					COPY_NAME(pmsnd->p_service_name[i].i_iso_639_code, 3);
					/*following by name text*/
					tmp_len = strlen(name);
					COPY_NAME(name, tmp_len);
				}
			}
			AM_SI_LIST_END()
#endif
			/* set the ending null byte */
			if (curr_name_len >= name_size)
				srv_info->name[name_size-1] = 0;
			else
				srv_info->name[curr_name_len] = 0;

			break;
		}
	AM_SI_LIST_END()
	AM_SI_LIST_END()
}

/**\brief 从visual channel中获取相关service信息*/
static void am_scan_extract_srv_info_from_vc(dvbpsi_atsc_vct_channel_t *vcinfo, AM_SCAN_ServiceInfo_t *srv_info)
{
	char name[32] = {0};

	srv_info->major_chan_num = vcinfo->i_major_number;
	srv_info->minor_chan_num = vcinfo->i_minor_number;

	srv_info->chan_num = (vcinfo->i_major_number<<16) | (vcinfo->i_minor_number&0xffff);
	srv_info->hidden = vcinfo->b_hidden;
	srv_info->hide_guide = vcinfo->b_hide_guide;
	srv_info->source_id = vcinfo->i_source_id;
	memcpy(srv_info->name, "xxx", 3);
	if (AM_SI_ConvertToUTF8(vcinfo->i_short_name, 14, name, 32, "utf-16") != AM_SUCCESS)
		strcpy(name, "No Name");
	memcpy(srv_info->name+3, name, sizeof(name));
	srv_info->name[sizeof(name)+3] = 0;
	/*业务类型*/
	srv_info->srv_type = vcinfo->i_service_type;

	AM_DEBUG(1 ,"Program(%d)('%s':%d-%d) in current TSID(%d) found!",
		srv_info->srv_id, srv_info->name,
		srv_info->major_chan_num, srv_info->minor_chan_num,
		vcinfo->i_channel_tsid);
}

static void add_audio(AM_SI_AudioInfo_t *ai, int aud_pid, int aud_fmt, char lang[3])
{
	int i;

	for (i=0; i<ai->audio_count; i++)
	{
		if (ai->audios[i].pid == aud_pid &&
			ai->audios[i].fmt == aud_fmt &&
			! memcmp(ai->audios[i].lang, lang, 3))
		{
			AM_DEBUG(1, "Skipping a exist audio: pid %d, fmt %d, lang %c%c%c",
				aud_pid, aud_fmt, lang[0], lang[1], lang[2]);
			return;
		}
	}
	if (ai->audio_count >= AM_SI_MAX_AUD_CNT)
	{
		AM_DEBUG(1, "Too many audios, Max count %d", AM_SI_MAX_AUD_CNT);
		return;
	}
	if (ai->audio_count < 0)
		ai->audio_count = 0;
	ai->audios[ai->audio_count].pid = aud_pid;
	ai->audios[ai->audio_count].fmt = aud_fmt;
	memset(ai->audios[ai->audio_count].lang, 0, sizeof(ai->audios[ai->audio_count].lang));
	if (lang[0] != 0)
	{
		memcpy(ai->audios[ai->audio_count].lang, lang, 3);
	}
	else
	{
		snprintf(ai->audios[ai->audio_count].lang, sizeof(ai->audios[ai->audio_count].lang), "Audio%d", ai->audio_count+1);
	}

	AM_DEBUG(1, "Add a audio: pid %d, fmt %d, language: %s", aud_pid, aud_fmt, ai->audios[ai->audio_count].lang);

	ai->audio_count++;
}

/**\brief 获取一个Program的PMT PID*/
static int get_pmt_pid(dvbpsi_pat_t *pats, int program_number)
{
	dvbpsi_pat_t *pat;
	dvbpsi_pat_program_t *prog;

	AM_SI_LIST_BEGIN(pats, pat)
	AM_SI_LIST_BEGIN(pat->p_first_program, prog)
		if (prog->i_number == program_number)
			return prog->i_pid;
	AM_SI_LIST_END()
	AM_SI_LIST_END()

	return 0x1fff;
}
/** Update Analog TS to database */
static void update_analog_ts_bynum(sqlite3_stmt **stmts, AM_SCAN_Result_t *result, AM_SCAN_TS_t *ts, int atv_update_num)
{
	int dbid;
	char lang_tmp[3];
	AM_SCAN_ServiceInfo_t srv_info;
	AM_Bool_t store = (stmts != NULL);
	int r = -1;
	int dbtsid = -1;

	if (ts->type != AM_SCAN_TS_ANALOG)
		return;

	if (store)
	{
		AM_DEBUG(1, "@@ Storing a analog ts @@");

		dbid = -1;

		AM_DEBUG(1, "ATV SCAN STORE CURRENT NUM = %d", atv_update_num);

		sqlite3_bind_int(stmts[SELECT_DBTSID_BY_NUM_AND_SRC], 1, FE_ANALOG);
		sqlite3_bind_int(stmts[SELECT_DBTSID_BY_NUM_AND_SRC], 2, atv_update_num);
		if (sqlite3_step(stmts[SELECT_DBTSID_BY_NUM_AND_SRC])  == SQLITE_ROW)
		{
			dbtsid = sqlite3_column_int(stmts[SELECT_DBTSID_BY_NUM_AND_SRC], 0);
			dbid = sqlite3_column_int(stmts[SELECT_DBTSID_BY_NUM_AND_SRC], 1);
		}
		sqlite3_reset(stmts[SELECT_DBTSID_BY_NUM_AND_SRC]);
		AM_DEBUG(1, "dbtsid = %d , dbid = %d", dbtsid, dbid);
		if (dbtsid != -1)
		{
			am_scan_update_ts(stmts, result, -1, dbtsid, ts);

			am_scan_init_service_info(&srv_info);
			srv_info.satpara_dbid = -1;
			srv_info.src = FE_ANALOG;
			srv_info.vfmt = -1;

			memset(lang_tmp, 0, sizeof(lang_tmp));
			add_audio(&srv_info.aud_info, 0x1fff, -1, lang_tmp);
			srv_info.srv_type = AM_SCAN_SRV_ATV;
			strcpy(srv_info.name, "xxxATV Program");

			srv_info.chan_num = atv_update_num;
			srv_info.srv_dbid = dbid;
			srv_info.analog_std = ts->analog.std;
			am_scan_update_service_info(stmts, result, &srv_info);
		}

	}
}

/**\brief Store a Analog TS to database */
static void store_analog_ts(sqlite3_stmt **stmts, AM_SCAN_Result_t *result, AM_SCAN_TS_t *ts)
{
	int dbid;
	char lang_tmp[3];
	AM_SCAN_ServiceInfo_t srv_info;
	AM_Bool_t store = (stmts != NULL);

	if (ts->type != AM_SCAN_TS_ANALOG)
		return;

	if (store)
	{
		AM_DEBUG(1, "@@ Storing a analog ts @@");

		dbid = -1;
		/*手动搜索时，如果已存储有任何频道则替换当前频道，否则新添加1频道*/
		if (result->start_para->atv_para.mode == AM_SCAN_ATVMODE_MANUAL)
		{
			int r;
			/*当前是否已经保存有ATV频道*/
			sqlite3_bind_int(stmts[QUERY_TS_BY_FREQ_ORDER], 1, FE_ANALOG);
			r = sqlite3_step(stmts[QUERY_TS_BY_FREQ_ORDER]);
			sqlite3_reset(stmts[QUERY_TS_BY_FREQ_ORDER]);
			if (r == SQLITE_ROW)
			{
				if (result->start_para->atv_para.channel_id < 0)
				{
					/*已保存有频道，则替换起始频率所在的频道，且保持频道号不变*/
					sqlite3_bind_int(stmts[QUERY_TS], 1, FE_ANALOG);
					sqlite3_bind_int(stmts[QUERY_TS], 2, dvb_fend_para(result->start_para->atv_para.fe_paras[2])->frequency);
					sqlite3_bind_int(stmts[QUERY_TS], 3, -1);
					sqlite3_bind_int(stmts[QUERY_TS], 4, -1);
					if (sqlite3_step(stmts[QUERY_TS]) == SQLITE_ROW)
					{
						dbid = sqlite3_column_int(stmts[QUERY_TS], 0);
					}
					sqlite3_reset(stmts[QUERY_TS]);
				}
				else
				{
					dbid = result->start_para->atv_para.channel_id;
				}

				if (dbid == -1)
				{
					AM_DEBUG(1, "Cannot get ts record for this ATV manual freq, will not store.");
				}
				else
				{
					AM_DEBUG(1, "ATV manual scan replace current channel, channel id = %d", dbid);
					/* 仅更新TS数据 */
					sqlite3_bind_int(stmts[UPDATE_ANALOG_TS], 1, ts->analog.freq);
					sqlite3_bind_int(stmts[UPDATE_ANALOG_TS], 2, ts->analog.std);
					sqlite3_bind_int(stmts[UPDATE_ANALOG_TS], 3, 1/*Stereo*/);
					sqlite3_bind_int(stmts[UPDATE_ANALOG_TS], 4, dbid);
					sqlite3_step(stmts[UPDATE_ANALOG_TS]);
					sqlite3_reset(stmts[UPDATE_ANALOG_TS]);
				}
				return;
			}
		}

		dbid = insert_ts(stmts, FE_ANALOG, ts->analog.freq, 0, -1, -1);
		if (dbid == -1)
		{
			AM_DEBUG(1, "insert new ts error");
			return;
		}

		/* 更新TS数据 */
		am_scan_update_ts_info(stmts, result, -1, dbid, ts);
	}

	/* 存储ATV频道 */
	am_scan_init_service_info(&srv_info);
	srv_info.satpara_dbid = -1;
	srv_info.src = FE_ANALOG;

	if (store)
	{
		/*添加新业务到数据库*/
		srv_info.srv_dbid = insert_srv(stmts, -1, dbid, srv_info.plp_id, 0xffff);
		if (srv_info.srv_dbid == -1)
		{
			AM_DEBUG(1, "insert new srv error");
			return;
		}
	}
	srv_info.vfmt = -1;
	memset(lang_tmp, 0, sizeof(lang_tmp));
	add_audio(&srv_info.aud_info, 0x1fff, -1, lang_tmp);
	srv_info.chan_num = 0;
	srv_info.srv_type = AM_SCAN_SRV_ATV;
	strcpy(srv_info.name, "xxxATV Program");
	srv_info.analog_std = ts->analog.std;
	if (result->start_para->atv_para.mode == AM_SCAN_ATVMODE_MANUAL)
	{
		srv_info.chan_num = 1;
	}
	else if (result->start_para->dtv_para.standard == AM_SCAN_DTV_STD_ATSC)
	{
		srv_info.major_chan_num = ts->tp_index + 1;
		srv_info.minor_chan_num = 0;
		srv_info.chan_num = (srv_info.major_chan_num<<16) | (srv_info.minor_chan_num&0xffff);
	}
	/* 更新service数据 */
	am_scan_update_service_info(stmts, result, &srv_info);
}

/**\brief 存储一个TS到数据库, ATSC*/
static void store_atsc_ts(sqlite3_stmt **stmts, AM_SCAN_Result_t *result, AM_SCAN_TS_t *ts)
{
	sqlite3 *hdb;
	dvbpsi_atsc_vct_t *vct;
	dvbpsi_atsc_vct_channel_t *vcinfo;
	dvbpsi_pmt_t *pmt;
	dvbpsi_pmt_es_t *es;
	dvbpsi_descriptor_t *descr;
	int src = result->start_para->dtv_para.source;
	int mode = result->start_para->dtv_para.mode;
	int net_dbid = -1, dbid = -1, orig_net_id = -1, satpara_dbid = -1;
	AM_SCAN_ServiceInfo_t srv_info;
	AM_Bool_t stream_found_in_vct = AM_FALSE;
	AM_Bool_t program_found_in_vct = AM_FALSE;
	AM_Bool_t store = (stmts != NULL);

	if(ts->digital.vcts
		&& ts->digital.pats
		&& (ts->digital.vcts->i_extension != ts->digital.pats->i_ts_id)
		&& (ts->digital.pats->i_ts_id == 0))
		ts->digital.use_vct_tsid = 1;

	if (store)
	{
		AM_DB_HANDLE_PREPARE(hdb);

		/*没有PAT或VCT，不存储*/
		if (!ts->digital.pats && !ts->digital.vcts)
		{
			AM_DEBUG(1, "No PAT or VCT found in ts, will not store to dbase");
			return;
		}

		net_dbid = -1;

		/*检查该TS是否已经添加*/
		dbid = insert_ts(stmts, src,
			(int)ts->digital.fend_para.atsc.para.frequency,
			0, satpara_dbid, -1);
		if (dbid == -1)
		{
			AM_DEBUG(1, "insert new ts error");
			return;
		}

		/* 更新TS数据 */
		am_scan_update_ts_info(stmts, result, net_dbid, dbid, ts);
	}

	/*遍历PMT表*/
	AM_SI_LIST_BEGIN(ts->digital.pmts, pmt)
		memset(&srv_info, 0, sizeof(AM_SCAN_ServiceInfo_t));
		am_scan_init_service_info(&srv_info);
		srv_info.satpara_dbid = satpara_dbid;
		srv_info.srv_id = pmt->i_program_number;
		srv_info.src = src;
		srv_info.pmt_pid = get_pmt_pid(ts->digital.pats, pmt->i_program_number);
		srv_info.pcr_pid = pmt->i_pcr_pid;

		if (store)
		{
			/*添加新业务到数据库*/
			srv_info.srv_dbid = insert_srv(stmts, net_dbid, dbid, srv_info.plp_id, srv_info.srv_id);
			if (srv_info.srv_dbid == -1)
			{
				AM_DEBUG(1, "insert new srv error");
				continue;
			}
		}


		/* looking for CA descr */
		if (! srv_info.scrambled_flag)
		{
			am_scan_extract_ca_scrambled_flag(pmt->p_first_descriptor, &srv_info, mode, &pmt->i_scramble_flag);
		}
		/*取ES流信息*/
		AM_SI_LIST_BEGIN(pmt->p_first_es, es)
			/* 提取音视频流 */
			AM_SI_ExtractAVFromES(es, &srv_info.vid, &srv_info.vfmt, &srv_info.aud_info);
			/* 查找CA加扰标识 */
			if (! srv_info.scrambled_flag)
				am_scan_extract_ca_scrambled_flag(es->p_first_descriptor, &srv_info, mode, &pmt->i_scramble_flag);
		AM_SI_LIST_END()

		program_found_in_vct = AM_FALSE;
		AM_SI_LIST_BEGIN(ts->digital.vcts, vct)
		AM_SI_LIST_BEGIN(vct->p_first_channel, vcinfo)
			/*Skip inactive program*/
			if (vcinfo->i_program_number == 0  || vcinfo->i_program_number == 0xffff)
				continue;

			/*从VCT表中查找该service并获取信息*/
			if ((ts->digital.use_vct_tsid || (vct->i_extension == ts->digital.pats->i_ts_id))
				&& vcinfo->i_channel_tsid == vct->i_extension)
			{
				if (vcinfo->i_program_number == pmt->i_program_number)
				{
					/*从VCT中尝试添加音视频流*/
					AM_SI_ExtractAVFromVC(vcinfo, &srv_info.vid, &srv_info.vfmt, &srv_info.aud_info);

					am_scan_extract_srv_info_from_vc(vcinfo, &srv_info);

					program_found_in_vct = AM_TRUE;
					goto VCT_END;
				}
			}
			else
			{
				AM_DEBUG(1, "Program(%d ts:%d) in VCT(ts:%d) found, current (ts:%d)",
						vcinfo->i_program_number, vcinfo->i_channel_tsid,
						vct->i_extension, ts->digital.pats->i_ts_id);
				continue;
			}
		AM_SI_LIST_END()
		AM_SI_LIST_END()
VCT_END:
		/*Store this service*/
		am_scan_update_service_info(stmts, result, &srv_info);
	AM_SI_LIST_END()

	if ((mode & AM_SCAN_DTVMODE_NOVCT)) {
		AM_DEBUG(1, "Skip programs  in vct but not in pmt mode");
		return;
	}
	/* All programs in PMTs added, now trying the programs in VCT but NOT in PMT */
	AM_SI_LIST_BEGIN(ts->digital.vcts, vct)
	AM_SI_LIST_BEGIN(vct->p_first_channel, vcinfo)
		AM_Bool_t found_in_pmt = AM_FALSE;

		am_scan_init_service_info(&srv_info);
		srv_info.satpara_dbid = satpara_dbid;
		srv_info.srv_id = vcinfo->i_program_number;
		srv_info.src = src;

		/*Skip inactive program*/
		if (vcinfo->i_program_number == 0  || vcinfo->i_program_number == 0xffff)
			continue;

		/* Is already added in PMT? */
		AM_SI_LIST_BEGIN(ts->digital.pmts, pmt)
			if (vcinfo->i_program_number == pmt->i_program_number)
			{
				found_in_pmt = AM_TRUE;
				break;
			}
		AM_SI_LIST_END()

		if (found_in_pmt)
			continue;

		if (store)
		{
			srv_info.srv_dbid = insert_srv(stmts, net_dbid, dbid, srv_info.plp_id, srv_info.srv_id);
			if (srv_info.srv_dbid == -1)
			{
				AM_DEBUG(1, "insert new srv error");
				continue;
			}
		}
		if (vcinfo->i_channel_tsid == vct->i_extension)
		{
			AM_SI_ExtractAVFromVC(vcinfo, &srv_info.vid, &srv_info.vfmt, &srv_info.aud_info);
			am_scan_extract_srv_info_from_vc(vcinfo, &srv_info);

			/*Store this service*/
			am_scan_update_service_info(stmts, result, &srv_info);
		}
		else
		{
			AM_DEBUG(1, "Program(%d ts:%d) in VCT(ts:%d) found",
			vcinfo->i_program_number, vcinfo->i_channel_tsid,
			vct->i_extension);
			continue;
		}
	AM_SI_LIST_END()
	AM_SI_LIST_END()
}

/**\brief 存储一个TS到数据库, DVB*/
static void store_dvb_ts(sqlite3_stmt **stmts, AM_SCAN_Result_t *result, AM_SCAN_TS_t *ts, AM_SCAN_RecTab_t *tab)
{
	dvbpsi_pmt_t *pmt;
	dvbpsi_pmt_es_t *es;
	dvbpsi_descriptor_t *descr;
	int src = result->start_para->dtv_para.source;
	int mode = result->start_para->dtv_para.mode;
	int net_dbid = -1, dbid = -1, orig_net_id = -1, satpara_dbid = -1;
	AM_SCAN_ServiceInfo_t srv_info;
	sqlite3 *hdb;
	AM_Bool_t store = (stmts != NULL);
	dvbpsi_pat_t *valid_pat = NULL;
	uint8_t plp_id;

	if (store)
	{
		AM_DB_HANDLE_PREPARE(hdb);

		/*没有PAT，不存储*/
		valid_pat = get_valid_pats(ts);
		if (valid_pat == NULL)
		{
			AM_DEBUG(1, "No PAT found in ts, will not store to dbase");
			return;
		}

		if (src == FE_QPSK)
		{
			/* 存储卫星配置 */
			satpara_dbid = insert_sat_para(stmts, &result->start_para->dtv_para.sat_para);
			if (satpara_dbid == -1)
			{
				AM_DEBUG(1, "Cannot insert satellite parameter!");
				return;
			}
		}

		AM_DEBUG(1, "@@ Storing ts, source is %d @@", src);
		net_dbid = am_scan_get_ts_network(stmts, src, result->nits, ts);

		/*检查该TS是否已经添加*/
		dbid = insert_ts(stmts, src,
			(int)dvb_fend_para(ts->digital.fend_para)->frequency,
			(src==FE_QPSK)?(int)dvb_fend_para(ts->digital.fend_para)->u.qpsk.symbol_rate:0,
			satpara_dbid,
			(src==FE_QPSK)?(int)ts->digital.fend_para.sat.polarisation:-1);

		if (dbid == -1)
		{
			AM_DEBUG(1, "insert new ts error");
			return;
		}

		/* 更新TS数据 */
		am_scan_update_ts_info(stmts, result, net_dbid, dbid, ts);
	}

	/*存储DTV节目，遍历PMT表*/
	if (ts->digital.pmts || (IS_DVBT2_TS(ts->digital.fend_para) && ts->digital.dvbt2_data_plp_num > 0))
	{
		int loop_count, lc;
		dvbpsi_sdt_t *sdt_list;
		dvbpsi_pmt_t *pmt_list;
		dvbpsi_pat_t *pat_list;

		/* For DVB-T2, search for each PLP, else search in current TS*/
		loop_count = IS_DVBT2_TS(ts->digital.fend_para) ? ts->digital.dvbt2_data_plp_num : 1;
		AM_DEBUG(1, "plp num %d", loop_count);

		for (lc=0; lc<loop_count; lc++)
		{
			pat_list = IS_DVBT2_TS(ts->digital.fend_para) ? ts->digital.dvbt2_data_plps[lc].pats : ts->digital.pats;
			pmt_list = IS_DVBT2_TS(ts->digital.fend_para) ? ts->digital.dvbt2_data_plps[lc].pmts : ts->digital.pmts;
			sdt_list =  IS_DVBT2_TS(ts->digital.fend_para) ? ts->digital.dvbt2_data_plps[lc].sdts : ts->digital.sdts;
			plp_id = IS_DVBT2_TS(ts->digital.fend_para) ? ts->digital.dvbt2_data_plps[lc].id : -1;
			AM_DEBUG(1, "plp_id %d", plp_id);

			AM_SI_LIST_BEGIN(pmt_list, pmt)
				am_scan_init_service_info(&srv_info);
				srv_info.satpara_dbid = satpara_dbid;
				srv_info.srv_id = pmt->i_program_number;
				srv_info.src = src;
				srv_info.pmt_pid = get_pmt_pid(pat_list, pmt->i_program_number);
				srv_info.pcr_pid = pmt->i_pcr_pid;
				srv_info.plp_id  = plp_id;

				if (store)
				{
					/*添加新业务到数据库*/
					srv_info.srv_dbid = insert_srv(stmts, net_dbid, dbid, srv_info.plp_id, srv_info.srv_id);
					if (srv_info.srv_dbid == -1)
					{
						AM_DEBUG(1, "insert new srv error");
						continue;
					}

					am_scan_rec_tab_add_srv(tab, srv_info.srv_dbid, ts);
				}

				/* looking for CA descr */
				if (! srv_info.scrambled_flag)
				{
					am_scan_extract_ca_scrambled_flag(pmt->p_first_descriptor, &srv_info, mode, &pmt->i_scramble_flag);
				}

				/*取ES流信息*/
				AM_SI_LIST_BEGIN(pmt->p_first_es, es)
					/* 提取音视频流 */
					AM_SI_ExtractAVFromES(es, &srv_info.vid, &srv_info.vfmt, &srv_info.aud_info);

					if (store)
					{
						/* 提取subtitle & teletext */
						AM_SI_ExtractDVBSubtitleFromES(es, &srv_info.sub_info);
						AM_SI_ExtractDVBTeletextFromES(es, &srv_info.ttx_info);
					}

					/* 查找CA加扰标识 */
					if (! srv_info.scrambled_flag)
						am_scan_extract_ca_scrambled_flag(es->p_first_descriptor, &srv_info, mode, &pmt->i_scramble_flag);
				AM_SI_LIST_END()
				/*获取节目名称，类型等信息*/
				am_scan_extract_srv_info_from_sdt(result, sdt_list, &srv_info);

				/*Store this service*/
				am_scan_update_service_info(stmts, result, &srv_info);
			AM_SI_LIST_END()

			/* All programs in PMTs added, now trying the programs in SDT but NOT in PMT */
			dvbpsi_sdt_service_t *srv;
			dvbpsi_sdt_t *sdt;

			AM_SI_LIST_BEGIN(sdt_list, sdt)
			AM_SI_LIST_BEGIN(sdt->p_first_service, srv)
				AM_Bool_t found_in_pmt = AM_FALSE;

				/* Is already added in PMT? */
				AM_SI_LIST_BEGIN(pmt_list, pmt)
					if (srv->i_service_id == pmt->i_program_number)
					{
						found_in_pmt = AM_TRUE;
						break;
					}
				AM_SI_LIST_END()

				if (found_in_pmt)
					continue;

				am_scan_init_service_info(&srv_info);
				srv_info.satpara_dbid = satpara_dbid;
				srv_info.srv_id = srv->i_service_id;
				srv_info.src = src;

				if (store)
				{
					srv_info.srv_dbid = insert_srv(stmts, net_dbid, dbid, srv_info.plp_id, srv_info.srv_id);
					if (srv_info.srv_dbid == -1)
					{
						AM_DEBUG(1, "insert new srv error");
						continue;
					}

					am_scan_rec_tab_add_srv(tab, srv_info.srv_dbid, ts);
				}

				am_scan_extract_srv_info_from_sdt(result, sdt_list, &srv_info);
				am_scan_update_service_info(stmts, result, &srv_info);

				/*as no pmt for this srv, set type to data for invisible*/
				srv_info.srv_type = 0;

			AM_SI_LIST_END()
			AM_SI_LIST_END()

		}
	}
}

/**\brief 清除数据库中某个源的所有数据*/
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

static void am_scan_clear_satellite(sqlite3 *hdb, int db_sat_id)
{
	char sqlstr[512];

	if (db_sat_id < 0)
		return;

	AM_DEBUG(1, "Delete tses & srvs for db_sat_para_id=%d", db_sat_id);
	snprintf(sqlstr, sizeof(sqlstr), "delete from ts_table where db_sat_para_id=%d",db_sat_id);
	sqlite3_exec(hdb, sqlstr, NULL, NULL, NULL);
	snprintf(sqlstr, sizeof(sqlstr), "delete from grp_map_table where db_srv_id in (select db_id from srv_table where db_sat_para_id=%d)",db_sat_id);
	sqlite3_exec(hdb, sqlstr, NULL, NULL, NULL);
	snprintf(sqlstr, sizeof(sqlstr), "delete from evt_table where db_srv_id in (select db_id from srv_table where db_sat_para_id=%d)",db_sat_id);
	sqlite3_exec(hdb, sqlstr, NULL, NULL, NULL);
	snprintf(sqlstr, sizeof(sqlstr), "delete from srv_table where db_sat_para_id=%d",db_sat_id);
	sqlite3_exec(hdb, sqlstr, NULL, NULL, NULL);

	AM_DEBUG(1, "Remove this sat para record...");
	snprintf(sqlstr, sizeof(sqlstr), "delete from sat_para_table where db_id=%d",db_sat_id);
	sqlite3_exec(hdb, sqlstr, NULL, NULL, NULL);
}

/**\brief 从一个NIT中查找某个service的LCN*/
static AM_Bool_t am_scan_get_lcn_from_nit(int org_net_id, int ts_id, int srv_id, dvbpsi_nit_t *nits,
int *sd_lcn, int *sd_visible, int *hd_lcn, int *hd_visible)
{
	dvbpsi_nit_ts_t *ts;
	dvbpsi_descriptor_t *dr;
	dvbpsi_nit_t *nit;

	*sd_lcn = -1;
	*hd_lcn = -1;
	AM_SI_LIST_BEGIN(nits, nit)
		AM_SI_LIST_BEGIN(nit->p_first_ts, ts)
			if(ts->i_ts_id==ts_id && ts->i_orig_network_id==org_net_id){
				AM_SI_LIST_BEGIN(ts->p_first_descriptor, dr)
					if(dr->p_decoded && ((dr->i_tag == AM_SI_DESCR_LCN_83))){
						if(dr->i_tag==AM_SI_DESCR_LCN_83)
						{
							dvbpsi_logical_channel_number_83_dr_t *lcn_dr = (dvbpsi_logical_channel_number_83_dr_t*)dr->p_decoded;
							dvbpsi_logical_channel_number_83_t *lcn = lcn_dr->p_logical_channel_number;
							int j;

							for(j=0; j<lcn_dr->i_logical_channel_numbers_number; j++){
								if(lcn->i_service_id == srv_id){
									*sd_lcn = lcn->i_logical_channel_number;
									*sd_visible = lcn->i_visible_service_flag;
									AM_DEBUG(1, "sd lcn for service %d ---> %d", srv_id, *sd_lcn);
									if (*hd_lcn == -1) {
										/* break to wait for lcn88 */
										break;
									} else {
										goto lcn_found;
									}
								}
								lcn++;
							}
						}
					}
					else if (dr->i_tag==AM_SI_DESCR_LCN_88)
					{
						dvbpsi_logical_channel_number_88_dr_t *lcn_dr = (dvbpsi_logical_channel_number_88_dr_t*)dr->p_decoded;
						dvbpsi_logical_channel_number_88_t *lcn = lcn_dr->p_logical_channel_number;
						int j;

						for(j=0; j<lcn_dr->i_logical_channel_numbers_number; j++){
							if(lcn->i_service_id == srv_id){
								*hd_lcn = lcn->i_logical_channel_number;
								*hd_visible = lcn->i_visible_service_flag;
								if (*sd_lcn == -1) {
									/* break to wait for lcn83 */
									break;
								} else {
									goto lcn_found;
								}
							}
							lcn++;
						}
					}
				AM_SI_LIST_END()
			}
		AM_SI_LIST_END()
	AM_SI_LIST_END()

lcn_found:
	if (*sd_lcn != -1 || *hd_lcn != -1)
		return AM_TRUE;

	return AM_FALSE;
}

/**\brief LCN排序处理*/
static void am_scan_lcn_proc(AM_SCAN_Result_t *result, sqlite3_stmt **stmts, AM_SCAN_RecTab_t *srv_tab)
{
#define LCN_CONFLICT_START 900
#define UPDATE_SRV_LCN(_l, _hl, _sl, _d) \
AM_MACRO_BEGIN\
	sqlite3_bind_int(stmts[UPDATE_LCN], 1, _l);\
	sqlite3_bind_int(stmts[UPDATE_LCN], 2, _hl);\
	sqlite3_bind_int(stmts[UPDATE_LCN], 3, _sl);\
	sqlite3_bind_int(stmts[UPDATE_LCN], 4, _d);\
	sqlite3_step(stmts[UPDATE_LCN]);\
	sqlite3_reset(stmts[UPDATE_LCN]);\
AM_MACRO_END
	if (result->tses && result->start_para->dtv_para.standard != AM_SCAN_DTV_STD_ATSC)
	{
		dvbpsi_nit_t *nit;
		AM_SCAN_TS_t *ts;
		dvbpsi_descriptor_t *dr;
		int i, conflict_lcn_start = LCN_CONFLICT_START;

		/*找到当前无LCN频道或LCN冲突频道的频道号起始位置*/
		sqlite3_bind_int(stmts[QUERY_MAX_LCN], 1, result->start_para->dtv_para.source);
		if(sqlite3_step(stmts[QUERY_MAX_LCN])==SQLITE_ROW)
		{
			conflict_lcn_start = sqlite3_column_int(stmts[QUERY_MAX_LCN], 0)+1;
			if (conflict_lcn_start < LCN_CONFLICT_START)
				conflict_lcn_start = LCN_CONFLICT_START;
			AM_DEBUG(1, "Have LCN, will set conflict LCN from %d", conflict_lcn_start);
		}
		sqlite3_reset(stmts[QUERY_MAX_LCN]);

		for(i=0; i<srv_tab->srv_cnt; i++)
		{
			int r;
			int srv_id, ts_id, org_net_id;
			int num = -1, visible = 1, hd_num = -1;
			int sd_lcn = -1, hd_lcn = -1;
			int sd_visible = 1, hd_visible = 1;
			AM_Bool_t swapped = AM_FALSE;
			AM_Bool_t got_lcn = AM_FALSE;

			sqlite3_bind_int(stmts[QUERY_SRV_TS_NET_ID], 1, srv_tab->srv_ids[i]);
			r = sqlite3_step(stmts[QUERY_SRV_TS_NET_ID]);
			if(r==SQLITE_ROW)
			{
				srv_id = sqlite3_column_int(stmts[QUERY_SRV_TS_NET_ID], 0);
				ts_id  = sqlite3_column_int(stmts[QUERY_SRV_TS_NET_ID], 1);
				org_net_id = sqlite3_column_int(stmts[QUERY_SRV_TS_NET_ID], 2);

				AM_DEBUG(1, "Looking for lcn of net %x, ts %x, srv %x", org_net_id, ts_id, srv_id);
				if (srv_tab->tses[i] != NULL &&
					am_scan_get_lcn_from_nit(org_net_id, ts_id, srv_id, srv_tab->tses[i]->digital.nits,
					&sd_lcn, &sd_visible, &hd_lcn, &hd_visible))
				{
					/* find in its own ts */
					AM_DEBUG(1, "Found lcn in its own TS");
					got_lcn = AM_TRUE;
				}
				if (! got_lcn)
				{
					/* find in other tses */
					AM_SI_LIST_BEGIN(result->tses, ts)
						if (ts != srv_tab->tses[i])
						{
							if (am_scan_get_lcn_from_nit(org_net_id, ts_id, srv_id, ts->digital.nits,
								&sd_lcn, &sd_visible, &hd_lcn, &hd_visible))
							{
								AM_DEBUG(1, "Found lcn in other TS");
								break;
							}
						}
					AM_SI_LIST_END()
				}
			}
			sqlite3_reset(stmts[QUERY_SRV_TS_NET_ID]);

			/* Skip Non-TV&Radio services */
			sqlite3_bind_int(stmts[QUERY_SRV_TYPE], 1, srv_tab->srv_ids[i]);
			r = sqlite3_step(stmts[QUERY_SRV_TYPE]);
			if (r == SQLITE_ROW)
			{
				int srv_type = sqlite3_column_int(stmts[QUERY_SRV_TYPE], 0);
				if (srv_type != 1 && srv_type != 2)
				{
					sqlite3_reset(stmts[QUERY_SRV_TYPE]);
					continue;
				}
			}
			sqlite3_reset(stmts[QUERY_SRV_TYPE]);

			AM_DEBUG(1, "save: sd_lcn is %d", sd_lcn);
			/* default we use SD */
			num = sd_lcn;
			hd_num = hd_lcn;
			visible = sd_visible;

			/* need to swap lcn ? */
			if (sd_lcn != -1 && hd_lcn != -1)
			{
				/* 是否存在一个service的lcn == 本service的hd_lcn? */
				sqlite3_bind_int(stmts[QUERY_SRV_BY_SD_LCN], 1, result->start_para->dtv_para.source);
				sqlite3_bind_int(stmts[QUERY_SRV_BY_SD_LCN], 2, hd_lcn);
				if(sqlite3_step(stmts[QUERY_SRV_BY_SD_LCN])==SQLITE_ROW)
				{
					int cft_db_id = sqlite3_column_int(stmts[QUERY_SRV_BY_SD_LCN], 0);
					int tmp_hd_lcn = sqlite3_column_int(stmts[QUERY_SRV_BY_SD_LCN], 1);

					AM_DEBUG(1, "SWAP LCN: from %d -> %d", tmp_hd_lcn, hd_lcn);
					UPDATE_SRV_LCN(tmp_hd_lcn, tmp_hd_lcn, hd_lcn, cft_db_id);
					AM_DEBUG(1, "SWAP LCN: from %d -> %d", hd_lcn, sd_lcn);
					num = hd_lcn; /* Use sd_lcn instead of hd_lcn */
					visible = hd_visible;
					swapped = AM_TRUE;
				}
				sqlite3_reset(stmts[QUERY_SRV_BY_SD_LCN]);
			}
			else if (sd_lcn == -1)
			{
				/* only hd lcn */
				num = hd_lcn;
				visible = hd_visible;
			}

			if(!visible){
				sqlite3_bind_int(stmts[UPDATE_CHAN_SKIP], 1, 1);
				sqlite3_bind_int(stmts[UPDATE_CHAN_SKIP], 2, srv_tab->srv_ids[i]);
				sqlite3_step(stmts[UPDATE_CHAN_SKIP]);
				sqlite3_reset(stmts[UPDATE_CHAN_SKIP]);
			}

			if (! swapped)
			{
				if (num >= 0)
				{
					/*该频道号是否已存在*/
					sqlite3_bind_int(stmts[QUERY_SRV_BY_LCN], 1, result->start_para->dtv_para.source);
					sqlite3_bind_int(stmts[QUERY_SRV_BY_LCN], 2, num);
					if(sqlite3_step(stmts[QUERY_SRV_BY_LCN])==SQLITE_ROW)
					{
						AM_DEBUG(1, "Find a conflict LCN, set from %d -> %d", num, conflict_lcn_start);
						/*已经存在，将当前service放到900后*/
						num = conflict_lcn_start++;
					}
					sqlite3_reset(stmts[QUERY_SRV_BY_LCN]);
				}
				else
				{
					/*未找到LCN， 将当前service放到900后*/
					num = conflict_lcn_start++;
					AM_DEBUG(1, "No LCN found for this program, automatically set to %d", num);
				}
			}

			UPDATE_SRV_LCN(num, hd_lcn, sd_lcn, srv_tab->srv_ids[i]);
		}
	}
}

/**\brief 按LCN顺序进行默认排序 */
static void am_scan_dtv_default_sort_by_lcn(AM_SCAN_Result_t *result, sqlite3_stmt **stmts, AM_SCAN_RecTab_t *srv_tab)
{
	UNUSED(srv_tab);

	if (result->tses && result->start_para->dtv_para.standard != AM_SCAN_DTV_STD_ATSC)
	{
		int row = AM_DB_MAX_SRV_CNT_PER_SRC, i, j;
		int r, db_id, rr, srv_type;

		i=1;
		j=1;

		/* 重新按default_chan_num 对已有channel排序 */
		sqlite3_bind_int(stmts[QUERY_SRV_BY_LCN_ORDER], 1, result->start_para->dtv_para.source);
		r = sqlite3_step(stmts[QUERY_SRV_BY_LCN_ORDER]);
		while (r == SQLITE_ROW)
		{
			db_id = sqlite3_column_int(stmts[QUERY_SRV_BY_LCN_ORDER], 0);
			srv_type = sqlite3_column_int(stmts[QUERY_SRV_BY_LCN_ORDER], 1);
			if (srv_type == 0x1)
			{
				/*电视节目*/
				sqlite3_bind_int(stmts[UPDATE_DEFAULT_CHAN_NUM], 1, i++);
				sqlite3_bind_int(stmts[UPDATE_DEFAULT_CHAN_NUM], 2, db_id);
				sqlite3_step(stmts[UPDATE_DEFAULT_CHAN_NUM]);
				sqlite3_reset(stmts[UPDATE_DEFAULT_CHAN_NUM]);
			}
			else if (srv_type == 0x2)
			{
				/*广播节目*/
				sqlite3_bind_int(stmts[UPDATE_DEFAULT_CHAN_NUM], 1, j++);
				sqlite3_bind_int(stmts[UPDATE_DEFAULT_CHAN_NUM], 2, db_id);
				sqlite3_step(stmts[UPDATE_DEFAULT_CHAN_NUM]);
				sqlite3_reset(stmts[UPDATE_DEFAULT_CHAN_NUM]);
			}

			r = sqlite3_step(stmts[QUERY_SRV_BY_LCN_ORDER]);
		}
		sqlite3_reset(stmts[QUERY_SRV_BY_LCN_ORDER]);
	}
}

/**\brief 按搜索顺序进行默认排序 */
static void am_scan_dtv_default_sort_by_scan_order(AM_SCAN_Result_t *result, sqlite3_stmt **stmts, AM_SCAN_RecTab_t *srv_tab)
{
	if (result->tses && result->start_para->dtv_para.standard != AM_SCAN_DTV_STD_ATSC)
	{
		int row = AM_DB_MAX_SRV_CNT_PER_SRC, i, j;
		int r, db_id, rr, srv_type;

		i=1;
		j=1;

		/* 重新按default_chan_num 对已有channel排序 */
		sqlite3_bind_int(stmts[QUERY_SRV_BY_DEF_CHAN_NUM_ORDER], 1, result->start_para->dtv_para.source);
		r = sqlite3_step(stmts[QUERY_SRV_BY_DEF_CHAN_NUM_ORDER]);
		while (r == SQLITE_ROW)
		{
			db_id = sqlite3_column_int(stmts[QUERY_SRV_BY_DEF_CHAN_NUM_ORDER], 0);
			if (! am_scan_rec_tab_have_src(srv_tab, db_id))
			{
				srv_type = sqlite3_column_int(stmts[QUERY_SRV_BY_DEF_CHAN_NUM_ORDER], 1);
				if (srv_type == 0x1)
				{
					/*电视节目*/
					sqlite3_bind_int(stmts[UPDATE_DEFAULT_CHAN_NUM], 1, i++);
					sqlite3_bind_int(stmts[UPDATE_DEFAULT_CHAN_NUM], 2, db_id);
					sqlite3_step(stmts[UPDATE_DEFAULT_CHAN_NUM]);
					sqlite3_reset(stmts[UPDATE_DEFAULT_CHAN_NUM]);
				}
				else if (srv_type == 0x2)
				{
					/*广播节目*/
					sqlite3_bind_int(stmts[UPDATE_DEFAULT_CHAN_NUM], 1, j++);
					sqlite3_bind_int(stmts[UPDATE_DEFAULT_CHAN_NUM], 2, db_id);
					sqlite3_step(stmts[UPDATE_DEFAULT_CHAN_NUM]);
					sqlite3_reset(stmts[UPDATE_DEFAULT_CHAN_NUM]);
				}
			}
			r = sqlite3_step(stmts[QUERY_SRV_BY_DEF_CHAN_NUM_ORDER]);
		}
		sqlite3_reset(stmts[QUERY_SRV_BY_DEF_CHAN_NUM_ORDER]);

		AM_DEBUG(1, "Insert channel default channel num from TV:%d, Radio:%d", i, j);
		/* 将本次搜索到的节目排到最后 */
		for (r=0; r<srv_tab->srv_cnt; r++)
		{
			sqlite3_bind_int(stmts[QUERY_SRV_TYPE], 1, srv_tab->srv_ids[r]);
			rr = sqlite3_step(stmts[QUERY_SRV_TYPE]);
			if (rr == SQLITE_ROW)
			{
				srv_type = sqlite3_column_int(stmts[QUERY_SRV_TYPE], 0);
				if (srv_type == 1)
				{
					/*电视节目*/
					sqlite3_bind_int(stmts[UPDATE_DEFAULT_CHAN_NUM], 1, i++);
					sqlite3_bind_int(stmts[UPDATE_DEFAULT_CHAN_NUM], 2, srv_tab->srv_ids[r]);
					sqlite3_step(stmts[UPDATE_DEFAULT_CHAN_NUM]);
					sqlite3_reset(stmts[UPDATE_DEFAULT_CHAN_NUM]);
				}
				else if (srv_type == 2)
				{
					/*广播节目*/
					sqlite3_bind_int(stmts[UPDATE_DEFAULT_CHAN_NUM], 1, j++);
					sqlite3_bind_int(stmts[UPDATE_DEFAULT_CHAN_NUM], 2, srv_tab->srv_ids[r]);
					sqlite3_step(stmts[UPDATE_DEFAULT_CHAN_NUM]);
					sqlite3_reset(stmts[UPDATE_DEFAULT_CHAN_NUM]);
				}
			}
			sqlite3_reset(stmts[QUERY_SRV_TYPE]);
		}
	}
}

/**\brief 按频点大小和service_id大小排序*/
static void am_scan_dtv_default_sort_by_service_id(AM_SCAN_Result_t *result, sqlite3_stmt **stmts, AM_SCAN_RecTab_t *srv_tab)
{
	if (result->tses && result->start_para->dtv_para.standard != AM_SCAN_DTV_STD_ATSC)
	{
		int row = AM_DB_MAX_SRV_CNT_PER_SRC, i, j, max_num;
		int r, db_ts_id, db_id, rr, srv_type;

		i=1;
		j=1;

		if(!result->start_para->dtv_para.resort_all)
		{
			if (! result->start_para->dtv_para.mix_tv_radio)
			{
				sqlite3_bind_int(stmts[QUERY_MAX_DEFAULT_CHAN_NUM_BY_TYPE], 1, 1);
				sqlite3_bind_int(stmts[QUERY_MAX_DEFAULT_CHAN_NUM_BY_TYPE], 2, result->start_para->dtv_para.source);
				r = sqlite3_step(stmts[QUERY_MAX_DEFAULT_CHAN_NUM_BY_TYPE]);
				if(r==SQLITE_ROW)
				{
					/*Current max num for TV programs*/
					i = sqlite3_column_int(stmts[QUERY_MAX_DEFAULT_CHAN_NUM_BY_TYPE], 0)+1;
				}
				sqlite3_reset(stmts[QUERY_MAX_DEFAULT_CHAN_NUM_BY_TYPE]);

				sqlite3_bind_int(stmts[QUERY_MAX_DEFAULT_CHAN_NUM_BY_TYPE], 1, 2);
				sqlite3_bind_int(stmts[QUERY_MAX_DEFAULT_CHAN_NUM_BY_TYPE], 2, result->start_para->dtv_para.source);
				r = sqlite3_step(stmts[QUERY_MAX_DEFAULT_CHAN_NUM_BY_TYPE]);
				if(r==SQLITE_ROW)
				{
					/*Current max num for Radio programs*/
					j = sqlite3_column_int(stmts[QUERY_MAX_DEFAULT_CHAN_NUM_BY_TYPE], 0)+1;
				}
				sqlite3_reset(stmts[QUERY_MAX_DEFAULT_CHAN_NUM_BY_TYPE]);
			}
			else
			{
				sqlite3_bind_int(stmts[QUERY_MAX_DEFAULT_CHAN_NUM], 1, result->start_para->dtv_para.source);
				r = sqlite3_step(stmts[QUERY_MAX_DEFAULT_CHAN_NUM]);
				if(r==SQLITE_ROW)
				{
					i = j = sqlite3_column_int(stmts[QUERY_MAX_DEFAULT_CHAN_NUM], 0)+1;
				}
				sqlite3_reset(stmts[QUERY_MAX_DEFAULT_CHAN_NUM]);
			}

			i = (i<=0) ? 1 : i;
			j = (j<=0) ? 1 : j;
		}


		if (! result->start_para->dtv_para.mix_tv_radio)
		{
			sqlite3_bind_int(stmts[QUERY_TS_BY_FREQ_ORDER], 1, result->start_para->dtv_para.source);
			r = sqlite3_step(stmts[QUERY_TS_BY_FREQ_ORDER]);

			while (r == SQLITE_ROW)
			{
				/*同频点下按service_id排序*/
				db_ts_id = sqlite3_column_int(stmts[QUERY_TS_BY_FREQ_ORDER], 0);
				sqlite3_bind_int(stmts[QUERY_SRV_BY_TYPE], 1, db_ts_id);
				rr = sqlite3_step(stmts[QUERY_SRV_BY_TYPE]);
				while (rr == SQLITE_ROW)
				{
					db_id = sqlite3_column_int(stmts[QUERY_SRV_BY_TYPE], 0);

					if(result->start_para->dtv_para.resort_all || am_scan_rec_tab_have_src(srv_tab, db_id))
					{
						srv_type = sqlite3_column_int(stmts[QUERY_SRV_BY_TYPE], 1);
						if (srv_type == 1)
						{
							/*电视节目*/
							sqlite3_bind_int(stmts[UPDATE_DEFAULT_CHAN_NUM], 1, i);
							sqlite3_bind_int(stmts[UPDATE_DEFAULT_CHAN_NUM], 2, db_id);
							sqlite3_step(stmts[UPDATE_DEFAULT_CHAN_NUM]);
							sqlite3_reset(stmts[UPDATE_DEFAULT_CHAN_NUM]);
							i++;
						}
						else if (srv_type == 2)
						{
							/*广播节目*/
							sqlite3_bind_int(stmts[UPDATE_DEFAULT_CHAN_NUM], 1, j);
							sqlite3_bind_int(stmts[UPDATE_DEFAULT_CHAN_NUM], 2, db_id);
							sqlite3_step(stmts[UPDATE_DEFAULT_CHAN_NUM]);
							sqlite3_reset(stmts[UPDATE_DEFAULT_CHAN_NUM]);
							j++;
						}
					}

					rr = sqlite3_step(stmts[QUERY_SRV_BY_TYPE]);
				}
				sqlite3_reset(stmts[QUERY_SRV_BY_TYPE]);
				r = sqlite3_step(stmts[QUERY_TS_BY_FREQ_ORDER]);
			}
			sqlite3_reset(stmts[QUERY_TS_BY_FREQ_ORDER]);
		}
		else
		{
			sqlite3_bind_int(stmts[QUERY_TS_BY_FREQ_ORDER], 1, result->start_para->dtv_para.source);
			r = sqlite3_step(stmts[QUERY_TS_BY_FREQ_ORDER]);

			while (r == SQLITE_ROW)
			{
				/*同频点下按service_id排序*/
				db_ts_id = sqlite3_column_int(stmts[QUERY_TS_BY_FREQ_ORDER], 0);
				sqlite3_bind_int(stmts[QUERY_SRV_BY_TYPE], 1, db_ts_id);
				rr = sqlite3_step(stmts[QUERY_SRV_BY_TYPE]);
				while (rr == SQLITE_ROW)
				{
					db_id = sqlite3_column_int(stmts[QUERY_SRV_BY_TYPE], 0);

					if(result->start_para->dtv_para.resort_all || am_scan_rec_tab_have_src(srv_tab, db_id))
					{
						srv_type = sqlite3_column_int(stmts[QUERY_SRV_BY_TYPE], 1);
						if (srv_type == 1)
						{
							/*电视节目*/
							sqlite3_bind_int(stmts[UPDATE_DEFAULT_CHAN_NUM], 1, i);
							sqlite3_bind_int(stmts[UPDATE_DEFAULT_CHAN_NUM], 2, db_id);
							sqlite3_step(stmts[UPDATE_DEFAULT_CHAN_NUM]);
							sqlite3_reset(stmts[UPDATE_DEFAULT_CHAN_NUM]);
							i++;
						}
						}
					rr = sqlite3_step(stmts[QUERY_SRV_BY_TYPE]);
				}
				sqlite3_reset(stmts[QUERY_SRV_BY_TYPE]);
				r = sqlite3_step(stmts[QUERY_TS_BY_FREQ_ORDER]);
			}
			sqlite3_reset(stmts[QUERY_TS_BY_FREQ_ORDER]);
			/* Radio programs */
			sqlite3_bind_int(stmts[QUERY_TS_BY_FREQ_ORDER], 1, result->start_para->dtv_para.source);
			r = sqlite3_step(stmts[QUERY_TS_BY_FREQ_ORDER]);
			while (r == SQLITE_ROW)
			{
				/*广播节目放到最后*/
				db_ts_id = sqlite3_column_int(stmts[QUERY_TS_BY_FREQ_ORDER], 0);
				sqlite3_bind_int(stmts[QUERY_SRV_BY_TYPE], 1, db_ts_id);
				rr = sqlite3_step(stmts[QUERY_SRV_BY_TYPE]);
				while (rr == SQLITE_ROW)
				{
					db_id = sqlite3_column_int(stmts[QUERY_SRV_BY_TYPE], 0);
					if(result->start_para->dtv_para.resort_all || am_scan_rec_tab_have_src(srv_tab, db_id))
					{
						srv_type = sqlite3_column_int(stmts[QUERY_SRV_BY_TYPE], 1);
						if (srv_type == 2)
						{
							sqlite3_bind_int(stmts[UPDATE_DEFAULT_CHAN_NUM], 1, i);
							sqlite3_bind_int(stmts[UPDATE_DEFAULT_CHAN_NUM], 2, db_id);
							sqlite3_step(stmts[UPDATE_DEFAULT_CHAN_NUM]);
							sqlite3_reset(stmts[UPDATE_DEFAULT_CHAN_NUM]);
							i++;
						}
					}
					rr = sqlite3_step(stmts[QUERY_SRV_BY_TYPE]);
				}
				sqlite3_reset(stmts[QUERY_SRV_BY_TYPE]);
				r = sqlite3_step(stmts[QUERY_TS_BY_FREQ_ORDER]);
			}
			sqlite3_reset(stmts[QUERY_TS_BY_FREQ_ORDER]);
		}
	}
}

/**\brief Sort the ATV Program by freqency order */
static void am_scan_atv_default_sort_by_freq(AM_SCAN_Result_t *result, sqlite3_stmt **stmts)
{
	int row = AM_DB_MAX_SRV_CNT_PER_SRC, i;
	int r, db_ts_id, db_id, rr, srv_type;

	if (result->start_para->dtv_para.standard != AM_SCAN_DTV_STD_ATSC &&
		result->start_para->atv_para.mode != AM_SCAN_ATVMODE_MANUAL)
	{
		i=1;

		sqlite3_bind_int(stmts[QUERY_TS_BY_FREQ_ORDER], 1, FE_ANALOG);
		r = sqlite3_step(stmts[QUERY_TS_BY_FREQ_ORDER]);

		while (r == SQLITE_ROW)
		{
			db_ts_id = sqlite3_column_int(stmts[QUERY_TS_BY_FREQ_ORDER], 0);
			sqlite3_bind_int(stmts[QUERY_SRV_BY_TYPE], 1, db_ts_id);
			rr = sqlite3_step(stmts[QUERY_SRV_BY_TYPE]);
			while (rr == SQLITE_ROW)
			{
				db_id = sqlite3_column_int(stmts[QUERY_SRV_BY_TYPE], 0);
				srv_type = sqlite3_column_int(stmts[QUERY_SRV_BY_TYPE], 1);
				if (srv_type == AM_SCAN_SRV_ATV)
				{
					sqlite3_bind_int(stmts[UPDATE_DEFAULT_CHAN_NUM], 1, i);
					sqlite3_bind_int(stmts[UPDATE_DEFAULT_CHAN_NUM], 2, db_id);
					sqlite3_step(stmts[UPDATE_DEFAULT_CHAN_NUM]);
					sqlite3_reset(stmts[UPDATE_DEFAULT_CHAN_NUM]);
					i++;
				}
				rr = sqlite3_step(stmts[QUERY_SRV_BY_TYPE]);
			}
			sqlite3_reset(stmts[QUERY_SRV_BY_TYPE]);
			r = sqlite3_step(stmts[QUERY_TS_BY_FREQ_ORDER]);
		}
		sqlite3_reset(stmts[QUERY_TS_BY_FREQ_ORDER]);
	}
}

static void am_scan_dtv_default_sort_by_hd_sd(AM_SCAN_Result_t *result, sqlite3_stmt **stmts, AM_SCAN_RecTab_t *srv_tab)
{
	AM_DEBUG(1, "Donot support hd sd sort method for non-dvbs yet, use service id method instead");
	am_scan_dtv_default_sort_by_service_id(result, stmts, srv_tab);
}


/**\brief DVBS按卫星角度，频点大小和service_id大小排序*/
static void am_scan_dvbs_default_sort_by_service_id(AM_SCAN_Result_t *result, sqlite3_stmt **stmts)
{
	if (result->tses && result->start_para->dtv_para.standard != AM_SCAN_DTV_STD_ATSC)
	{
		int i, j, r, db_ts_id, db_id, rr, srv_type, db_sat_id;

		i=1;
		j=1;

		/* Donot change the exist service channel order */
		sqlite3_bind_int(stmts[QUERY_EXIST_SRV_BY_CHAN_ORDER], 1, result->start_para->dtv_para.source);
		while (sqlite3_step(stmts[QUERY_EXIST_SRV_BY_CHAN_ORDER]) == SQLITE_ROW)
		{
			db_id = sqlite3_column_int(stmts[QUERY_EXIST_SRV_BY_CHAN_ORDER], 0);
			srv_type = sqlite3_column_int(stmts[QUERY_EXIST_SRV_BY_CHAN_ORDER], 1);
			if (srv_type == 1)
			{
				/*电视节目*/
				sqlite3_bind_int(stmts[UPDATE_DEFAULT_CHAN_NUM], 1, i);
				sqlite3_bind_int(stmts[UPDATE_DEFAULT_CHAN_NUM], 2, db_id);
				sqlite3_step(stmts[UPDATE_DEFAULT_CHAN_NUM]);
				sqlite3_reset(stmts[UPDATE_DEFAULT_CHAN_NUM]);
				i++;
			}
			else if (srv_type == 2)
			{
				/*广播节目*/
				sqlite3_bind_int(stmts[UPDATE_DEFAULT_CHAN_NUM], 1, j);
				sqlite3_bind_int(stmts[UPDATE_DEFAULT_CHAN_NUM], 2, db_id);
				sqlite3_step(stmts[UPDATE_DEFAULT_CHAN_NUM]);
				sqlite3_reset(stmts[UPDATE_DEFAULT_CHAN_NUM]);
				j++;
			}

		}
		sqlite3_reset(stmts[QUERY_EXIST_SRV_BY_CHAN_ORDER]);

		/*Add for new-searched services*/
		/*首先按卫星db_id排序*/
		r = sqlite3_step(stmts[QUERY_DVBS_SAT_BY_SAT_ORDER]);
		while (r == SQLITE_ROW)
		{
			db_sat_id = sqlite3_column_int(stmts[QUERY_DVBS_SAT_BY_SAT_ORDER], 0);
			if (db_sat_id < 0)
			{
				r = sqlite3_step(stmts[QUERY_DVBS_SAT_BY_SAT_ORDER]);
				continue;
			}
			/*同一个卫星下按频点排序*/
			sqlite3_bind_int(stmts[QUERY_DVBS_TP_BY_FREQ_ORDER], 1, db_sat_id);
			r = sqlite3_step(stmts[QUERY_DVBS_TP_BY_FREQ_ORDER]);
			while (r == SQLITE_ROW)
			{
				/*同频点下按service_id排序*/
				db_ts_id = sqlite3_column_int(stmts[QUERY_DVBS_TP_BY_FREQ_ORDER], 0);
				sqlite3_bind_int(stmts[QUERY_NONEXIST_SRV_BY_SRV_ID_ORDER], 1, db_ts_id);
				rr = sqlite3_step(stmts[QUERY_NONEXIST_SRV_BY_SRV_ID_ORDER]);
				while (rr == SQLITE_ROW)
				{
					db_id = sqlite3_column_int(stmts[QUERY_NONEXIST_SRV_BY_SRV_ID_ORDER], 0);

					srv_type = sqlite3_column_int(stmts[QUERY_NONEXIST_SRV_BY_SRV_ID_ORDER], 1);
					if (srv_type == 1)
					{
						/*电视节目*/
						sqlite3_bind_int(stmts[UPDATE_DEFAULT_CHAN_NUM], 1, i);
						sqlite3_bind_int(stmts[UPDATE_DEFAULT_CHAN_NUM], 2, db_id);
						sqlite3_step(stmts[UPDATE_DEFAULT_CHAN_NUM]);
						sqlite3_reset(stmts[UPDATE_DEFAULT_CHAN_NUM]);
						i++;
					}
					else if (srv_type == 2)
					{
						/*广播节目*/
						sqlite3_bind_int(stmts[UPDATE_DEFAULT_CHAN_NUM], 1, j);
						sqlite3_bind_int(stmts[UPDATE_DEFAULT_CHAN_NUM], 2, db_id);
						sqlite3_step(stmts[UPDATE_DEFAULT_CHAN_NUM]);
						sqlite3_reset(stmts[UPDATE_DEFAULT_CHAN_NUM]);
						j++;
					}

					rr = sqlite3_step(stmts[QUERY_NONEXIST_SRV_BY_SRV_ID_ORDER]);
				}
				sqlite3_reset(stmts[QUERY_NONEXIST_SRV_BY_SRV_ID_ORDER]);
				r = sqlite3_step(stmts[QUERY_DVBS_TP_BY_FREQ_ORDER]);
			}
			sqlite3_reset(stmts[QUERY_DVBS_TP_BY_FREQ_ORDER]);
			r = sqlite3_step(stmts[QUERY_DVBS_SAT_BY_SAT_ORDER]);
		}
		sqlite3_reset(stmts[QUERY_DVBS_SAT_BY_SAT_ORDER]);
	}
}


/**\brief DVBS按高清在前，标清在后排序*/
static void am_scan_dvbs_default_sort_by_hd_sd(AM_SCAN_Result_t *result, sqlite3_stmt **stmts)
{
	if (result->tses && result->start_para->dtv_para.standard != AM_SCAN_DTV_STD_ATSC)
	{
		int i, j, r, db_ts_id, db_id, rr, srv_type, db_sat_id, vfmt;

		i=1;
		j=1;

		/*高清节目排在前面*/

		/*首先按卫星db_id排序*/
		r = sqlite3_step(stmts[QUERY_DVBS_SAT_BY_SAT_ORDER]);
		while (r == SQLITE_ROW)
		{
			db_sat_id = sqlite3_column_int(stmts[QUERY_DVBS_SAT_BY_SAT_ORDER], 0);
			if (db_sat_id < 0)
			{
				r = sqlite3_step(stmts[QUERY_DVBS_SAT_BY_SAT_ORDER]);
				continue;
			}
			/*同一个卫星下按频点排序*/
			sqlite3_bind_int(stmts[QUERY_DVBS_TP_BY_FREQ_ORDER], 1, db_sat_id);
			r = sqlite3_step(stmts[QUERY_DVBS_TP_BY_FREQ_ORDER]);
			while (r == SQLITE_ROW)
			{
				/*同频点下按service_id排序*/
				db_ts_id = sqlite3_column_int(stmts[QUERY_DVBS_TP_BY_FREQ_ORDER], 0);
				sqlite3_bind_int(stmts[QUERY_SRV_BY_TYPE], 1, db_ts_id);
				rr = sqlite3_step(stmts[QUERY_SRV_BY_TYPE]);
				while (rr == SQLITE_ROW)
				{
					db_id = sqlite3_column_int(stmts[QUERY_SRV_BY_TYPE], 0);
					srv_type = sqlite3_column_int(stmts[QUERY_SRV_BY_TYPE], 1);
					vfmt = sqlite3_column_int(stmts[QUERY_SRV_BY_TYPE], 2);
					if (srv_type == 1 && (vfmt == VFORMAT_MPEG4 || vfmt == VFORMAT_H264))
					{
						/*HD电视节目*/
						sqlite3_bind_int(stmts[UPDATE_DEFAULT_CHAN_NUM], 1, i);
						sqlite3_bind_int(stmts[UPDATE_DEFAULT_CHAN_NUM], 2, db_id);
						sqlite3_step(stmts[UPDATE_DEFAULT_CHAN_NUM]);
						sqlite3_reset(stmts[UPDATE_DEFAULT_CHAN_NUM]);
						i++;
					}
					rr = sqlite3_step(stmts[QUERY_SRV_BY_TYPE]);
				}
				sqlite3_reset(stmts[QUERY_SRV_BY_TYPE]);
				r = sqlite3_step(stmts[QUERY_DVBS_TP_BY_FREQ_ORDER]);
			}
			sqlite3_reset(stmts[QUERY_DVBS_TP_BY_FREQ_ORDER]);
			r = sqlite3_step(stmts[QUERY_DVBS_SAT_BY_SAT_ORDER]);
		}
		sqlite3_reset(stmts[QUERY_DVBS_SAT_BY_SAT_ORDER]);

		AM_DEBUG(1, "SD programs start from %d", i);
		/*首先按卫星db_id排序*/
		r = sqlite3_step(stmts[QUERY_DVBS_SAT_BY_SAT_ORDER]);
		while (r == SQLITE_ROW)
		{
			db_sat_id = sqlite3_column_int(stmts[QUERY_DVBS_SAT_BY_SAT_ORDER], 0);
			if (db_sat_id < 0)
			{
				r = sqlite3_step(stmts[QUERY_DVBS_SAT_BY_SAT_ORDER]);
				continue;
			}
			/*同一个卫星下按频点排序*/
			sqlite3_bind_int(stmts[QUERY_DVBS_TP_BY_FREQ_ORDER], 1, db_sat_id);
			r = sqlite3_step(stmts[QUERY_DVBS_TP_BY_FREQ_ORDER]);
			while (r == SQLITE_ROW)
			{
				/*同频点下按service_id排序*/
				db_ts_id = sqlite3_column_int(stmts[QUERY_DVBS_TP_BY_FREQ_ORDER], 0);
				sqlite3_bind_int(stmts[QUERY_SRV_BY_TYPE], 1, db_ts_id);
				rr = sqlite3_step(stmts[QUERY_SRV_BY_TYPE]);
				while (rr == SQLITE_ROW)
				{
					db_id = sqlite3_column_int(stmts[QUERY_SRV_BY_TYPE], 0);
					srv_type = sqlite3_column_int(stmts[QUERY_SRV_BY_TYPE], 1);
					vfmt = sqlite3_column_int(stmts[QUERY_SRV_BY_TYPE], 2);
					if (srv_type == 1 && vfmt != VFORMAT_MPEG4 && vfmt != VFORMAT_H264)
					{
						/*SD电视节目*/
						sqlite3_bind_int(stmts[UPDATE_DEFAULT_CHAN_NUM], 1, i);
						sqlite3_bind_int(stmts[UPDATE_DEFAULT_CHAN_NUM], 2, db_id);
						sqlite3_step(stmts[UPDATE_DEFAULT_CHAN_NUM]);
						sqlite3_reset(stmts[UPDATE_DEFAULT_CHAN_NUM]);
						i++;
					}
					else if (srv_type == 2)
					{
						/*广播节目*/
						sqlite3_bind_int(stmts[UPDATE_DEFAULT_CHAN_NUM], 1, j);
						sqlite3_bind_int(stmts[UPDATE_DEFAULT_CHAN_NUM], 2, db_id);
						sqlite3_step(stmts[UPDATE_DEFAULT_CHAN_NUM]);
						sqlite3_reset(stmts[UPDATE_DEFAULT_CHAN_NUM]);
						j++;
					}

					rr = sqlite3_step(stmts[QUERY_SRV_BY_TYPE]);
				}
				sqlite3_reset(stmts[QUERY_SRV_BY_TYPE]);
				r = sqlite3_step(stmts[QUERY_DVBS_TP_BY_FREQ_ORDER]);
			}
			sqlite3_reset(stmts[QUERY_DVBS_TP_BY_FREQ_ORDER]);
			r = sqlite3_step(stmts[QUERY_DVBS_SAT_BY_SAT_ORDER]);
		}
		sqlite3_reset(stmts[QUERY_DVBS_SAT_BY_SAT_ORDER]);
	}
}

static void am_scan_atv_store(AM_SCAN_Result_t *result)
{
	AM_SCAN_TS_t *ts;
	char sqlstr[128];
	sqlite3_stmt	*stmts[MAX_STMT];
	int i, ret, db_sat_id = -1;
	int src = result->start_para->dtv_para.source;
	AM_SCAN_DTVSatellitePara_t *sat_para = &result->start_para->dtv_para.sat_para;
	AM_SCAN_RecTab_t srv_tab;
	AM_Bool_t has_atv = AM_FALSE;
	sqlite3 *hdb;
	int atvauto_num = 0;

	assert(result);

	AM_DB_HANDLE_PREPARE(hdb);
	am_scan_rec_tab_init(&srv_tab);

	/*Prepare sqlite3 stmts*/
	memset(stmts, 0, sizeof(sqlite3_stmt*) * MAX_STMT);
	for (i=0; i<MAX_STMT; i++)
	{
		ret = sqlite3_prepare(hdb, sql_stmts[i], -1, &stmts[i], NULL);
		if (ret != SQLITE_OK)
		{
			AM_DEBUG(1, "Prepare sqlite3 failed, stmts[%d] ret = %x", i, ret);
			goto store_end;
		}
	}

	AM_DEBUG(1, "Storing tses ...");
	/*依次存储每个TS*/
	AM_SI_LIST_BEGIN(result->tses, ts)
		if (ts->type == AM_SCAN_TS_ANALOG)
		{
			has_atv = AM_TRUE;
			if (result->start_para->atv_para.mode == AM_SCAN_ATVMODE_MANUAL)
			{
				atvauto_num = result->start_para->atv_para.channel_num;
			}
			else
			{
				atvauto_num ++;
			}
			update_analog_ts_bynum(stmts, result, ts, atvauto_num);
		}
	AM_SI_LIST_END()
store_end:
	for (i=0; i<MAX_STMT; i++)
	{
		if (stmts[i] != NULL)
			sqlite3_finalize(stmts[i]);
	}

	am_scan_rec_tab_release(&srv_tab);
}
/**\brief 默认搜索完毕存储函数*/
static void am_scan_default_store(AM_SCAN_Result_t *result)
{
	AM_SCAN_TS_t *ts;
	char sqlstr[128];
	sqlite3_stmt	*stmts[MAX_STMT];
	int i, ret, db_sat_id = -1;
	int src = result->start_para->dtv_para.source;
	AM_SCAN_DTVSatellitePara_t *sat_para = &result->start_para->dtv_para.sat_para;
	AM_SCAN_RecTab_t srv_tab;
	AM_Bool_t sorted = 0, has_atv = AM_FALSE, has_dtv = AM_FALSE;
	sqlite3 *hdb;

	assert(result);

	AM_DB_HANDLE_PREPARE(hdb);
	am_scan_rec_tab_init(&srv_tab);

	/*Prepare sqlite3 stmts*/
	memset(stmts, 0, sizeof(sqlite3_stmt*) * MAX_STMT);
	for (i=0; i<MAX_STMT; i++)
	{
		ret = sqlite3_prepare(hdb, sql_stmts[i], -1, &stmts[i], NULL);
		if (ret != SQLITE_OK)
		{
			AM_DEBUG(1, "Prepare sqlite3 failed, hdb[%p] stmts[%d] ret = %x", hdb, i, ret);
			goto store_end;
		}
	}

	AM_DEBUG(1, "Storing tses ...");
	/*依次存储每个TS*/
	AM_SI_LIST_BEGIN(result->tses, ts)
		if (ts->type == AM_SCAN_TS_ANALOG)
		{
			if (! has_atv)
			{
				if (result->start_para->atv_para.mode == AM_SCAN_ATVMODE_AUTO)
					am_scan_clear_source(hdb, FE_ANALOG);
				has_atv = AM_TRUE;
			}
			store_analog_ts(stmts, result, ts);
		}
		else
		{
			if (! has_dtv)
			{
				if (src == FE_QPSK)
				{
					db_sat_id = get_sat_para_record(stmts, sat_para);
					if (GET_MODE(result->start_para->dtv_para.mode) == AM_SCAN_DTVMODE_SAT_BLIND)
					{
						am_scan_clear_satellite(hdb, db_sat_id);
					}
				}
				else if (GET_MODE(result->start_para->dtv_para.mode) != AM_SCAN_DTVMODE_MANUAL)
				{
					/*自动搜索和全频段搜索时删除该源下的所有信息*/
					am_scan_clear_source(hdb, src);
				}

				has_dtv = AM_TRUE;
			}
			if (result->start_para->dtv_para.standard != AM_SCAN_DTV_STD_ATSC)
				store_dvb_ts(stmts, result, ts, &srv_tab);
			else
				store_atsc_ts(stmts, result, ts);
		}
	AM_SI_LIST_END()

	/* Sort the ATV Programs */
	if (has_atv)
	{
		am_scan_atv_default_sort_by_freq(result, stmts);
	}
	/* Sort the DTV Programs */
	if (has_dtv)
	{
		/* 生成lcn排序结果 */
		am_scan_lcn_proc(result, stmts, &srv_tab);

		/* Generate service id order */
		if (src == FE_QPSK)
			am_scan_dvbs_default_sort_by_service_id(result, stmts);
		else
			am_scan_dtv_default_sort_by_service_id(result, stmts, &srv_tab);

		AM_DEBUG(1, "Updating service_id_order ...");
		sqlite3_exec(hdb, "update srv_table set service_id_order=default_chan_num \
			where default_chan_num>0", NULL, NULL, NULL);

		/* Generate lcn order */
		am_scan_dtv_default_sort_by_lcn(result, stmts, &srv_tab);
		AM_DEBUG(1, "Updating lcn_order ...");
		sqlite3_exec(hdb, "update srv_table set lcn_order=default_chan_num \
			where default_chan_num>0", NULL, NULL, NULL);

		/* Generate hd/sd order */
		if (src == FE_QPSK)
			am_scan_dvbs_default_sort_by_hd_sd(result, stmts);
		else
			am_scan_dtv_default_sort_by_hd_sd(result, stmts, &srv_tab);
		AM_DEBUG(1, "Updating hd_sd_order ...");
		sqlite3_exec(hdb, "update srv_table set hd_sd_order=default_chan_num \
			where default_chan_num>0", NULL, NULL, NULL);

		/* Gernerate program number by sort method */
		if (result->start_para->dtv_para.sort_method == AM_SCAN_SORT_BY_FREQ_SRV_ID)
		{
			sqlite3_exec(hdb, "update srv_table set default_chan_num=service_id_order \
				where service_id_order>0", NULL, NULL, NULL);
		}
		else if (result->start_para->dtv_para.sort_method == AM_SCAN_SORT_BY_LCN)
		{
			sqlite3_exec(hdb, "update srv_table set default_chan_num=lcn_order \
				where lcn_order>0", NULL, NULL, NULL);
		}
		else if (result->start_para->dtv_para.sort_method == AM_SCAN_SORT_BY_HD_SD)
		{
			sqlite3_exec(hdb, "update srv_table set default_chan_num=hd_sd_order \
				where hd_sd_order>0", NULL, NULL, NULL);
		}
		else
		{
			am_scan_dtv_default_sort_by_scan_order(result, stmts, &srv_tab);
		}
	}

	if (result->start_para->dtv_para.standard == AM_SCAN_DTV_STD_ATSC)
	{
		sqlite3_exec(hdb, "update srv_table set default_chan_num=chan_num \
			 where chan_num>0", NULL, NULL, NULL);
	}

	/* 生成最终 chan_num */
	AM_DEBUG(1, "Updating chan_num & chan_order from default_chan_num ...");
	sqlite3_exec(hdb, "update srv_table set chan_num=default_chan_num, \
		chan_order=default_chan_num where default_chan_num>0", NULL, NULL, NULL);

store_end:
	for (i=0; i<MAX_STMT; i++)
	{
		if (stmts[i] != NULL)
			sqlite3_finalize(stmts[i]);
	}

	am_scan_rec_tab_release(&srv_tab);
}

/**\brief 清空一个表控制标志*/
static void am_scan_tablectl_clear(AM_SCAN_TableCtl_t * scl)
{
	scl->data_arrive_time = 0;

	if (scl->subs && scl->subctl)
	{
		int i;

		memset(scl->subctl, 0, sizeof(AM_SCAN_SubCtl_t) * scl->subs);
		for (i=0; i<scl->subs; i++)
		{
			scl->subctl[i].ver = 0xff;
		}
	}
}

/**\brief 初始化一个表控制结构*/
static AM_ErrorCode_t am_scan_tablectl_init(AM_SCAN_TableCtl_t * scl, int recv_flag, int evt_flag,
											int timeout, uint16_t pid, uint8_t tid, const char *name,
											uint16_t sub_cnt, void (*done)(struct AM_SCAN_Scanner_s *),
											int distance)
{
	memset(scl, 0, sizeof(AM_SCAN_TableCtl_t));\
	scl->fid = -1;
	scl->recv_flag = recv_flag;
	scl->evt_flag = evt_flag;
	scl->timeout = timeout;
	scl->pid = pid;
	scl->tid = tid;
	scl->done = done;
	scl->repeat_distance = distance;
	strcpy(scl->tname, name);

	scl->subs = sub_cnt;
	if (scl->subs)
	{
		scl->subctl = (AM_SCAN_SubCtl_t*)malloc(sizeof(AM_SCAN_SubCtl_t) * scl->subs);
		if (!scl->subctl)
		{
			scl->subs = 0;
			AM_DEBUG(1, "Cannot init tablectl, no enough memory");
			return AM_SCAN_ERR_NO_MEM;
		}

		am_scan_tablectl_clear(scl);
	}

	return AM_SUCCESS;
}

/**\brief 反初始化一个表控制结构*/
static void am_scan_tablectl_deinit(AM_SCAN_TableCtl_t * scl)
{
	if (scl->subctl)
	{
		free(scl->subctl);
		scl->subctl = NULL;
	}
}

/**\brief 判断一个表的所有section是否收齐*/
static AM_Bool_t am_scan_tablectl_test_complete(AM_SCAN_TableCtl_t * scl)
{
	static uint8_t test_array[32] = {0};
	int i;

	for (i=0; i<scl->subs; i++)
	{
		if ((scl->subctl[i].ver != 0xff) &&
			memcmp(scl->subctl[i].mask, test_array, sizeof(test_array)))
			return AM_FALSE;
	}

	return AM_TRUE;
}

/**\brief 判断一个表的指定section是否已经接收*/
static AM_Bool_t am_scan_tablectl_test_recved(AM_SCAN_TableCtl_t * scl, AM_SI_SectionHeader_t *header)
{
	int i;

	if (!scl->subctl)
		return AM_TRUE;

	for (i=0; i<scl->subs; i++)
	{
		if ((scl->subctl[i].ext == header->extension) &&
			(scl->subctl[i].ver == header->version) &&
			(scl->subctl[i].last == header->last_sec_num) &&
			!BIT_TEST(scl->subctl[i].mask, header->sec_num))
		{
			AM_DEBUG(1,"%s %d\n", __FUNCTION__, __LINE__);
			AM_DEBUG(1, "test_recved true:\npid 0x%x\ttid 0x%x\n sext %d\thext %d\n sver %d\thver %d\n slast %d\thlast "
					         "%d\n",
			         scl->pid, scl->tid, scl->subctl[i].ext, header->extension, scl->subctl[i].ver, header->version,
			         scl->subctl[i].last, header->last_sec_num);
			return AM_TRUE;
		}
		else
		{
			AM_DEBUG(1, "%s %d\n", __FUNCTION__, __LINE__);
			AM_DEBUG(1, "test_recved false:\npid 0x%x\ttid 0x%x\n sext %d\thext %d\n sver %d\thver %d\n slast "
					         "%d\thlast "
					         "%d\n",
			         scl->pid, scl->tid, scl->subctl[i].ext, header->extension, scl->subctl[i].ver, header->version,
			         scl->subctl[i].last, header->last_sec_num);
		}
	}
	return AM_FALSE;
}

/**\brief 在一个表中增加一个section已接收标识*/
static AM_ErrorCode_t am_scan_tablectl_mark_section(AM_SCAN_TableCtl_t * scl, AM_SI_SectionHeader_t *header)
{
	int i;
	AM_SCAN_SubCtl_t *sub, *fsub;

	if (!scl->subctl)
		return AM_SUCCESS;

	sub = fsub = NULL;
	for (i=0; i<scl->subs; i++)
	{
		if (scl->subctl[i].ext == header->extension)
		{
			sub = &scl->subctl[i];
			break;
		}
		/*记录一个空闲的结构*/
		if ((scl->subctl[i].ver == 0xff) && !fsub)
			fsub = &scl->subctl[i];
	}

	if (!sub && !fsub)
	{
		AM_DEBUG(1, "No more subctl for adding new %s subtable", scl->tname);
		return AM_FAILURE;
	}
	if (!sub)
		sub = fsub;

	/*发现新版本，重新设置接收控制*/
	if (sub->ver != 0xff && (sub->ver != header->version ||\
		sub->ext != header->extension || sub->last != header->last_sec_num))
		SUBCTL_CLEAR(sub);

	if (sub->ver == 0xff)
	{
		int i;

		/*接收到的第一个section*/
		sub->last = header->last_sec_num;
		sub->ver = header->version;
		sub->ext = header->extension;

		for (i=0; i<(sub->last+1); i++)
			BIT_SET(sub->mask, i);
	}

	/*设置已接收标识*/
	BIT_CLEAR(sub->mask, header->sec_num);

	if (scl->data_arrive_time == 0)
		AM_TIME_GetClock(&scl->data_arrive_time);

	return AM_SUCCESS;
}

/**\brief 释放过滤器，并保证在此之后不会再有无效数据*/
static void am_scan_free_filter(AM_SCAN_Scanner_t *scanner, int *fid)
{
	if (*fid == -1)
		return;

	AM_DMX_FreeFilter(dtv_start_para.dmx_dev_id, *fid);
	*fid = -1;
	pthread_mutex_unlock(&scanner->lock);
	/*等待无效数据处理完毕*/
	AM_DMX_Sync(dtv_start_para.dmx_dev_id);
	pthread_mutex_lock(&scanner->lock);
}

/**\brief NIT搜索完毕(包括超时)处理*/
static void am_scan_nit_done(AM_SCAN_Scanner_t *scanner)
{
	dvbpsi_nit_t *nit;
	dvbpsi_nit_ts_t *ts;
	dvbpsi_descriptor_t *descr;
	struct dvb_frontend_parameters *param;
	int i;

	am_scan_free_filter(scanner, &scanner->dtvctl.nitctl.fid);
	/*清除事件标志*/
	//scanner->evt_flag &= ~scanner->dtvctl.nitctl.evt_flag;

	if (scanner->stage == AM_SCAN_STAGE_TS)
	{
		/*清除搜索标识*/
		scanner->recv_status &= ~scanner->dtvctl.nitctl.recv_flag;
		AM_DEBUG(1, "NIT Done in TS stage!");
		return;
	}

	if (! scanner->result.nits)
	{
		AM_DEBUG(1, "No NIT found ,try next frequency");
		am_scan_try_nit(scanner);
		return;
	}

	/*清除搜索标识*/
	scanner->recv_status &= ~scanner->dtvctl.nitctl.recv_flag;

	if (scanner->start_freqs)
		free(scanner->start_freqs);
	scanner->start_freqs = NULL;
	scanner->start_freqs_cnt = 0;
	scanner->curr_freq = -1;

	/*首先统计NIT中描述的TS个数*/
	AM_SI_LIST_BEGIN(scanner->result.nits, nit)
		AM_SI_LIST_BEGIN(nit->p_first_ts, ts)
		scanner->start_freqs_cnt++;
		AM_SI_LIST_END()
	AM_SI_LIST_END()

	if (scanner->start_freqs_cnt == 0)
	{
		AM_DEBUG(1, "No TS in NIT");
		goto NIT_END;
	}
	scanner->start_freqs = (AM_SCAN_FrontEndPara_t*)\
							malloc(sizeof(AM_SCAN_FrontEndPara_t) * scanner->start_freqs_cnt);
	if (!scanner->start_freqs)
	{
		AM_DEBUG(1, "No enough memory for building ts list");
		scanner->start_freqs_cnt = 0;
		goto NIT_END;
	}
	memset(scanner->start_freqs, 0, sizeof(AM_SCAN_FrontEndPara_t) * scanner->start_freqs_cnt);

	scanner->start_freqs_cnt = 0;

	/*从NIT搜索结果中取出频点列表存到start_freqs中*/
	AM_SI_LIST_BEGIN(scanner->result.nits, nit)
		/*遍历每个TS*/
		AM_SI_LIST_BEGIN(nit->p_first_ts, ts)
			AM_SI_LIST_BEGIN(ts->p_first_descriptor, descr)
			/*取DVBC频点信息*/
			if (dtv_start_para.source == FE_QAM && descr->p_decoded && descr->i_tag == AM_SI_DESCR_CABLE_DELIVERY)
			{
				dvbpsi_cable_delivery_dr_t *pcd = (dvbpsi_cable_delivery_dr_t*)descr->p_decoded;

				scanner->start_freqs[scanner->start_freqs_cnt].fe_para.m_type = FE_QAM;
				param = &scanner->start_freqs[scanner->start_freqs_cnt].fe_para.cable.para;
				param->frequency = pcd->i_frequency;
				param->u.qam.modulation = pcd->i_modulation_type;
				param->u.qam.symbol_rate = pcd->i_symbol_rate;
				scanner->start_freqs_cnt++;
				AM_DEBUG(1, "Add frequency %u, symbol_rate %u, modulation %u, onid %d, ts_id %d", param->frequency,
						param->u.qam.symbol_rate, param->u.qam.modulation, ts->i_orig_network_id, ts->i_ts_id);
			}
			else if (dtv_start_para.source == FE_OFDM && descr->p_decoded && descr->i_tag == AM_SI_DESCR_TERRESTRIAL_DELIVERY)
			{
				dvbpsi_terr_deliv_sys_dr_t *pcd = (dvbpsi_terr_deliv_sys_dr_t*)descr->p_decoded;

				scanner->start_freqs[scanner->start_freqs_cnt].fe_para.m_type = FE_OFDM;
				param = &scanner->start_freqs[scanner->start_freqs_cnt].fe_para.terrestrial.para;
				param->frequency = pcd->i_centre_frequency*10;
				param->u.ofdm.bandwidth = pcd->i_bandwidth;
				scanner->start_freqs_cnt++;
				AM_DEBUG(1, "Add frequency %u, bw %u, onid %d, ts_id %d", param->frequency,
						param->u.ofdm.bandwidth, ts->i_orig_network_id, ts->i_ts_id);
			}
			else if (dtv_start_para.source == FE_QPSK && descr->p_decoded && descr->i_tag == AM_SI_DESCR_SATELLITE_DELIVERY)
			{
				dvbpsi_sat_deliv_sys_dr_t *pcd = (dvbpsi_sat_deliv_sys_dr_t*)descr->p_decoded;

				scanner->start_freqs[scanner->start_freqs_cnt].fe_para.m_type = FE_QPSK;
				scanner->start_freqs[scanner->start_freqs_cnt].fe_para.sat.polarisation = pcd->i_polarization>1 ? AM_FEND_POLARISATION_NOSET : pcd->i_polarization;
				param = &scanner->start_freqs[scanner->start_freqs_cnt].fe_para.sat.para;
				param->frequency = pcd->i_frequency; /* GHz->KHz */
				param->u.qpsk.symbol_rate = pcd->i_symbol_rate;
				scanner->start_freqs_cnt++;
				AM_DEBUG(1, "Add frequency %u(%u), symbol_rate %u, onid %d, ts_id %d", param->frequency,pcd->i_frequency,
						param->u.qpsk.symbol_rate, ts->i_orig_network_id, ts->i_ts_id);
			}
			AM_SI_LIST_END()
		AM_SI_LIST_END()
	AM_SI_LIST_END()

NIT_END:
	AM_DEBUG(1, "Total found %d frequencies in NIT", scanner->start_freqs_cnt);
	if (scanner->start_freqs_cnt == 0)
	{
		AM_DEBUG(1, "No Delivery system descriptor in NIT");
	}
	if (scanner->recv_status == AM_SCAN_RECVING_COMPLETE)
	{
		/*开始搜索TS*/
		SET_PROGRESS_EVT(AM_SCAN_PROGRESS_TS_END, NULL);
		AM_DEBUG(1, "----------NIT VERSION = %d--------", scanner->result.nits->i_version);
		SET_PROGRESS_EVT(AM_SCAN_PROGRESS_NIT_END, scanner->result.nits->i_version);
		scanner->stage = AM_SCAN_STAGE_TS;
		am_scan_start_next_ts(scanner);
	}
}

/**\brief BAT搜索完毕(包括超时)处理*/
static void am_scan_bat_done(AM_SCAN_Scanner_t *scanner)
{
	am_scan_free_filter(scanner, &scanner->dtvctl.batctl.fid);
	/*清除事件标志*/
	//scanner->evt_flag &= ~scanner->dtvctl.batctl.evt_flag;
	/*清除搜索标识*/
	scanner->recv_status &= ~scanner->dtvctl.batctl.recv_flag;

	if (scanner->recv_status == AM_SCAN_RECVING_COMPLETE)
	{
		/*开始搜索TS*/
		SET_PROGRESS_EVT(AM_SCAN_PROGRESS_TS_END, NULL);
		SET_PROGRESS_EVT(AM_SCAN_PROGRESS_NIT_END, NULL);
		scanner->stage = AM_SCAN_STAGE_TS;
		am_scan_start_next_ts(scanner);
	}
}

/**\brief PAT搜索完毕(包括超时)处理*/
static void am_scan_pat_done(AM_SCAN_Scanner_t *scanner)
{
	am_scan_free_filter(scanner, &scanner->dtvctl.patctl.fid);
	/*清除事件标志*/
	//scanner->evt_flag &= ~scanner->dtvctl.patctl.evt_flag;
	/*清除搜索标识*/
	scanner->recv_status &= ~scanner->dtvctl.patctl.recv_flag;

	SET_PROGRESS_EVT(AM_SCAN_PROGRESS_PAT_DONE, (void*)scanner->curr_ts->digital.pats);

	/* try to add fixed pmt-pid programs for ISDBT */
	if (IS_ISDBT())
	{
		am_scan_isdbt_prepare_oneseg(scanner->curr_ts);
	}

	/* Set the current PAT section */
	scanner->dtvctl.cur_pat = scanner->curr_ts->digital.pats;

	/*开始搜索PMT表*/
	am_scan_request_pmts(scanner);
}

/**\brief PMT搜索完毕(包括超时)处理*/
static void am_scan_pmt_done(AM_SCAN_Scanner_t *scanner)
{
	int i;
	int ret = 0;
	for (i=0; i<(int)AM_ARRAY_SIZE(scanner->dtvctl.pmtctl); i++)
	{
		if (scanner->dtvctl.pmtctl[i].fid < 0)
			continue;
		ret = am_scan_tablectl_test_complete(&scanner->dtvctl.pmtctl[i]);
		if (ret && scanner->dtvctl.pmtctl[i].data_arrive_time != 0)
		{
			AM_DEBUG(1, "Stop filter for PMT, program %d", scanner->dtvctl.pmtctl[i].ext);
			am_scan_free_filter(scanner, &scanner->dtvctl.pmtctl[i].fid);
		}
		else
		{
			AM_DEBUG(1, "fid %d test_complete %d arrive_time %d\n", scanner->dtvctl.pmtctl[i].fid,
			         ret, scanner->dtvctl.pmtctl[i].data_arrive_time);
		}
	}

	/*清除事件标志*/
	//scanner->evt_flag &= ~scanner->dtvctl.pmtctl.evt_flag;

	/*开始搜索尚未接收的PMT表*/
	am_scan_request_pmts(scanner);
}

/**\brief CAT搜索完毕(包括超时)处理*/
static void am_scan_cat_done(AM_SCAN_Scanner_t *scanner)
{
	am_scan_free_filter(scanner, &scanner->dtvctl.catctl.fid);
	/*清除事件标志*/
	//scanner->evt_flag &= ~scanner->dtvctl.catctl.evt_flag;
	/*清除搜索标识*/
	scanner->recv_status &= ~scanner->dtvctl.catctl.recv_flag;

	SET_PROGRESS_EVT(AM_SCAN_PROGRESS_CAT_DONE, (void*)scanner->curr_ts->digital.cats);
}

/**\brief SDT搜索完毕(包括超时)处理*/
static void am_scan_sdt_done(AM_SCAN_Scanner_t *scanner)
{
	am_scan_free_filter(scanner, &scanner->dtvctl.sdtctl.fid);
	/*清除事件标志*/
	//scanner->evt_flag &= ~scanner->dtvctl.sdtctl.evt_flag;
	/*清除搜索标识*/
	scanner->recv_status &= ~scanner->dtvctl.sdtctl.recv_flag;

	SET_PROGRESS_EVT(AM_SCAN_PROGRESS_SDT_DONE,
		IS_DVBT2() ? (void*)scanner->curr_ts->digital.dvbt2_data_plps[scanner->curr_plp].sdts
		: (void*)scanner->curr_ts->digital.sdts);
}

/**\brief MGT搜索完毕(包括超时)处理*/
static void am_scan_mgt_done(AM_SCAN_Scanner_t *scanner)
{
	am_scan_free_filter(scanner, &scanner->dtvctl.mgtctl.fid);
	/*清除事件标志*/
	//scanner->evt_flag &= ~scanner->dtvctl.mgtctl.evt_flag;
	/*清除搜索标识*/
	scanner->recv_status &= ~scanner->dtvctl.mgtctl.recv_flag;

	SET_PROGRESS_EVT(AM_SCAN_PROGRESS_MGT_DONE, (void*)scanner->curr_ts->digital.mgts);

	/*开始搜索VCT表*/
	if (scanner->curr_ts->digital.mgts)
	{
		AM_Bool_t is_cable_vct = AM_FALSE;
		AM_Bool_t is_tvct = AM_FALSE;

		dvbpsi_atsc_mgt_table_t *p_table = scanner->curr_ts->digital.mgts->p_first_table;

		do {
			if (p_table) {
				if (p_table->i_table_type == 0 || p_table->i_table_type == 1) {
					is_tvct = AM_TRUE;
				}
				else if (p_table->i_table_type == 2 || p_table->i_table_type == 3) {
					is_cable_vct = AM_TRUE;
				}
			}
		} while (p_table = p_table->p_next);

		AM_DEBUG(1, "mgt has vct cvct:[%d] tvct:[%d]", is_cable_vct, is_tvct);
		if (!is_cable_vct && is_tvct) {
			scanner->dtvctl.vctctl.tid = AM_SI_TID_PSIP_TVCT;
		} else {
			scanner->dtvctl.vctctl.tid = AM_SI_TID_PSIP_CVCT;
		}
		am_scan_request_section(scanner, &scanner->dtvctl.vctctl);
	}
#if 0
	else
	{
		/*没有MGT, 尝试搜索PAT/PMT*/
		am_scan_request_section(scanner, &scanner->dtvctl.patctl);
	}
#endif
}

/**\brief VCT搜索完毕(包括超时)处理*/
static void am_scan_vct_done(AM_SCAN_Scanner_t *scanner)
{
	atsc_descriptor_t *descr;
	vct_section_info_t *vct;
	vct_channel_info_t *chan;

	am_scan_free_filter(scanner, &scanner->dtvctl.vctctl.fid);
	/*清除事件标志*/
	//scanner->evt_flag &= ~scanner->vctctl.evt_flag;
	/*清除搜索标识*/
	scanner->recv_status &= ~scanner->dtvctl.vctctl.recv_flag;

	SET_PROGRESS_EVT(AM_SCAN_PROGRESS_VCT_DONE, (void*)scanner->curr_ts->digital.vcts);
}

/**\brief 根据过滤器号取得相应控制数据*/
static AM_SCAN_TableCtl_t *am_scan_get_section_ctrl_by_fid(AM_SCAN_Scanner_t *scanner, int fid)
{
	AM_SCAN_TableCtl_t *scl = NULL;
	int i;

	if (scanner->dtvctl.patctl.fid == fid)
	{
		scl = &scanner->dtvctl.patctl;
	}
	else if (dtv_start_para.standard == AM_SCAN_DTV_STD_ATSC)
	{
		if (scanner->dtvctl.mgtctl.fid == fid)
			scl = &scanner->dtvctl.mgtctl;
		else if (scanner->dtvctl.vctctl.fid == fid)
			scl = &scanner->dtvctl.vctctl;
	}
	else
	{
		if (scanner->dtvctl.catctl.fid == fid)
			scl = &scanner->dtvctl.catctl;
		else if (scanner->dtvctl.sdtctl.fid == fid)
			scl = &scanner->dtvctl.sdtctl;
		else if (scanner->dtvctl.nitctl.fid == fid)
			scl = &scanner->dtvctl.nitctl;
		else if (scanner->dtvctl.batctl.fid == fid)
			scl = &scanner->dtvctl.batctl;
	}

	if (scl == NULL)
	{
		for (i=0; i<(int)AM_ARRAY_SIZE(scanner->dtvctl.pmtctl); i++)
		{
			if (scanner->dtvctl.pmtctl[i].fid == fid)
			{
				scl = &scanner->dtvctl.pmtctl[i];
				break;
			}
		}
	}
	return scl;
}

/**\brief 从一个CVCT中添加虚拟频道*/
static void am_scan_add_vc_from_vct(AM_SCAN_Scanner_t *scanner, vct_section_info_t *vct)
{
	vct_channel_info_t *tmp, *new;
	AM_Bool_t found;

	AM_SI_LIST_BEGIN(vct->vct_chan_info, tmp)
		new = (vct_channel_info_t *)malloc(sizeof(vct_channel_info_t));
		if (new == NULL)
		{
			AM_DEBUG(1, "Error, no enough memory for adding a new VC");
			continue;
		}
		/*here we share the desc pointer*/
		*new = *tmp;
		new->p_next = NULL;

		found = AM_FALSE;
		/*Is this vc already added?*/
		AM_SI_LIST_BEGIN(scanner->result.vcs, tmp)
			if (tmp->channel_TSID == new->channel_TSID &&
				tmp->program_number == new->program_number)
			{
				found = AM_TRUE;
				break;
			}
		AM_SI_LIST_END()

		if (! found)
		{
			/*Add this vc to result.vcs*/
			ADD_TO_LIST(new, scanner->result.vcs);
		}
	AM_SI_LIST_END()
}


/**\brief 数据处理函数*/
static void am_scan_section_handler(int dev_no, int fid, const uint8_t *data, int len, void *user_data)
{
	AM_SCAN_Scanner_t *scanner = (AM_SCAN_Scanner_t*)user_data;
	AM_SCAN_TableCtl_t * sec_ctrl;
	AM_SI_SectionHeader_t header;

	UNUSED(dev_no);

	if (scanner == NULL)
	{
		AM_DEBUG(1, "Scan: Invalid param user_data in dmx callback");
		return;
	}
	if (data == NULL)
		return;

	pthread_mutex_lock(&scanner->lock);
	/*获取接收控制数据*/
	sec_ctrl = am_scan_get_section_ctrl_by_fid(scanner, fid);
	if (sec_ctrl)
	{
		/* the section_syntax_indicator bit must be 1 */
		if ((data[1]&0x80) == 0)
		{
			AM_DEBUG(1, "Scan: section_syntax_indicator is 0, skip this section");
			goto parse_end;
		}

		if (AM_SI_GetSectionHeader(scanner->dtvctl.hsi, (uint8_t*)data, len, &header) != AM_SUCCESS)
		{
			AM_DEBUG(1, "Scan: section header error");
			goto parse_end;
		}

		/*该section是否已经接收过*/
		if (am_scan_tablectl_test_recved(sec_ctrl, &header))
		{
			AM_DEBUG(1,"%s section %d repeat!", sec_ctrl->tname, header.sec_num);
			/*当有多个子表时，判断收齐的条件为 收到重复section + 所有子表收齐 + 重复section间隔时间大于某个值*/
			if (sec_ctrl->subs > 1)
			{
				int now;

				AM_TIME_GetClock(&now);
				if (am_scan_tablectl_test_complete(sec_ctrl) &&
					((now - sec_ctrl->data_arrive_time) > sec_ctrl->repeat_distance))
				{
					AM_DEBUG(1, "%s Done!!", sec_ctrl->tname);
					scanner->evt_flag |= sec_ctrl->evt_flag;
					pthread_cond_signal(&scanner->cond);
				}
			}

			goto parse_end;
		}
		/*数据处理*/
		switch (header.table_id)
		{
			case AM_SI_TID_PAT:
				AM_DEBUG(1, "PAT section %d/%d arrived", header.sec_num, header.last_sec_num);
				if (scanner->curr_ts)
				{
					COLLECT_SECTION(dvbpsi_pat_t, scanner->curr_ts->digital.pats);
					if (IS_DVBT2())
						scanner->curr_ts->digital.dvbt2_data_plps[scanner->curr_plp].pats = scanner->curr_ts->digital.pats;
				}
				break;
			case AM_SI_TID_PMT:
				if (scanner->curr_ts)
				{
					AM_DEBUG(1, "PMT %d arrived", header.extension);
					if (IS_DVBT2())
						COLLECT_SECTION(dvbpsi_pmt_t, scanner->curr_ts->digital.dvbt2_data_plps[scanner->curr_plp].pmts);
					else
						COLLECT_SECTION(dvbpsi_pmt_t, scanner->curr_ts->digital.pmts);
				}
				break;
			case AM_SI_TID_SDT_ACT:
				AM_DEBUG(1, "SDT actual section %d/%d arrived", header.sec_num, header.last_sec_num);
				if (scanner->curr_ts)
				{
					if (IS_DVBT2())
						COLLECT_SECTION(dvbpsi_sdt_t, scanner->curr_ts->digital.dvbt2_data_plps[scanner->curr_plp].sdts);
					else
						COLLECT_SECTION(dvbpsi_sdt_t, scanner->curr_ts->digital.sdts);
				}
				break;
			case AM_SI_TID_CAT:
				if (scanner->curr_ts)
					COLLECT_SECTION(dvbpsi_cat_t, scanner->curr_ts->digital.cats);
				break;
			case AM_SI_TID_NIT_ACT:
				if (scanner->stage != AM_SCAN_STAGE_TS)
				{
					COLLECT_SECTION(dvbpsi_nit_t, scanner->result.nits);
				}
				else
				{
					COLLECT_SECTION(dvbpsi_nit_t, scanner->curr_ts->digital.nits);
				}
				break;
			case AM_SI_TID_BAT:
				AM_DEBUG(1, "BAT ext 0x%x, sec %d, last %d", header.extension, header.sec_num, header.last_sec_num);
				COLLECT_SECTION(dvbpsi_bat_t, scanner->result.bats);
				break;
			case AM_SI_TID_PSIP_MGT:
				if (scanner->curr_ts)
					COLLECT_SECTION(dvbpsi_atsc_mgt_t, scanner->curr_ts->digital.mgts);
				break;
			case AM_SI_TID_PSIP_TVCT:
			case AM_SI_TID_PSIP_CVCT:
				if (scanner->curr_ts)
				{
					COLLECT_SECTION(dvbpsi_atsc_vct_t, scanner->curr_ts->digital.vcts);
					//if (scanner->curr_ts->digital.tvcts != NULL)
					//	am_scan_add_vc_from_vct(scanner, scanner->curr_ts->digital.vcts);
				}
				break;
			default:
				AM_DEBUG(1, "Scan: Unkown section data, table_id 0x%x", data[0]);
				goto parse_end;
				break;
		}

		/*数据处理完毕，查看该表是否已接收完毕所有section*/
		if (am_scan_tablectl_test_complete(sec_ctrl) && sec_ctrl->subs == 1)
		{
			/*该表接收完毕*/
			AM_DEBUG(1, "%s Done!", sec_ctrl->tname);
			scanner->evt_flag |= sec_ctrl->evt_flag;
			pthread_cond_signal(&scanner->cond);
		}
	}
	else
	{
		AM_DEBUG(1, "Scan: Unknown filter id %d in dmx callback", fid);
	}

parse_end:
	pthread_mutex_unlock(&scanner->lock);

}

/**\brief 请求一个表的section数据*/
static AM_ErrorCode_t am_scan_request_section(AM_SCAN_Scanner_t *scanner, AM_SCAN_TableCtl_t *scl)
{
	struct dmx_sct_filter_params param;

	if (scl->fid != -1)
	{
		am_scan_free_filter(scanner, &scl->fid);
	}

	/*分配过滤器*/
	if (AM_DMX_AllocateFilter(dtv_start_para.dmx_dev_id, &scl->fid) != AM_SUCCESS)
		return AM_DMX_ERR_NO_FREE_FILTER;
	/*设置处理函数*/
	AM_TRY(AM_DMX_SetCallback(dtv_start_para.dmx_dev_id, scl->fid, am_scan_section_handler, (void*)scanner));

	/*设置过滤器参数*/
	memset(&param, 0, sizeof(param));
	param.pid = scl->pid;
	param.filter.filter[0] = scl->tid;
	param.filter.mask[0] = 0xff;

	// if (scl->tid == AM_SI_TID_PSIP_TVCT || scl->tid == AM_SI_TID_PSIP_CVCT) {
	// 	param.filter.mask[0] = 0xfe;
	// }

	/*For PMT, we must filter its extension*/
	if (scl->tid == AM_SI_TID_PMT && scl->ext != 0xffff/* a special mark for isdbt one-seg program */)
	{
		param.filter.filter[1] = (uint8_t)((scl->ext&0xff00)>>8);
		param.filter.mask[1] = 0xff;
		param.filter.filter[2] = (uint8_t)(scl->ext);
		param.filter.mask[2] = 0xff;
	}

	/*Current next indicator must be 1*/
	param.filter.filter[3] = 0x01;
	param.filter.mask[3] = 0x01;

	param.flags = DMX_CHECK_CRC;
	/*设置超时时间*/
	AM_TIME_GetTimeSpecTimeout(scl->timeout, &scl->end_time);

	AM_TRY(AM_DMX_SetSecFilter(dtv_start_para.dmx_dev_id, scl->fid, &param));
	AM_TRY(AM_DMX_SetBufferSize(dtv_start_para.dmx_dev_id, scl->fid, 16*1024));
	AM_TRY(AM_DMX_StartFilter(dtv_start_para.dmx_dev_id, scl->fid));

	/*设置接收状态*/
	scanner->recv_status |= scl->recv_flag;

	return AM_SUCCESS;
}

static int am_scan_get_recving_pmt_count(AM_SCAN_Scanner_t *scanner)
{
	int i, count = 0;

	for (i=0; i<(int)AM_ARRAY_SIZE(scanner->dtvctl.pmtctl); i++)
	{
		if (scanner->dtvctl.pmtctl[i].fid >= 0)
			count++;
	}

	AM_DEBUG(1, "%d PMTs are receiving...", count);
	return count;
}

static AM_ErrorCode_t am_scan_request_pmts(AM_SCAN_Scanner_t *scanner)
{
	int i, recv_count;
	AM_ErrorCode_t ret = AM_SUCCESS;
	dvbpsi_pat_program_t *cur_prog = scanner->dtvctl.cur_prog;

	if (! scanner->curr_ts)
	{
		/*A serious error*/
		AM_DEBUG(1, "Error, no current ts selected");
		return AM_SCAN_ERROR_BASE;
	}
	if (! scanner->curr_ts->digital.pats)
		return AM_SCAN_ERROR_BASE;

	for (i=0; i<(int)AM_ARRAY_SIZE(scanner->dtvctl.pmtctl); i++)
	{
		if (scanner->dtvctl.pmtctl[i].fid < 0)
		{
			if (cur_prog)
			{
				/* Start next */
				cur_prog = cur_prog->p_next;
			}
			if (! cur_prog)
			{
				/* find a valid PAT section */
				while (scanner->dtvctl.cur_pat && ! scanner->dtvctl.cur_pat->p_first_program)
				{
					AM_DEBUG(1,"Scan: no PMT found in this PAT section, try next PAT section.");
					scanner->dtvctl.cur_pat = scanner->dtvctl.cur_pat->p_next;
				}
				/* Start from its first program */
				if (scanner->dtvctl.cur_pat)
				{
					AM_DEBUG(1, "Start PMT from a new PAT section %p...", scanner->dtvctl.cur_pat);
					cur_prog = scanner->dtvctl.cur_pat->p_first_program;
					scanner->dtvctl.cur_pat = scanner->dtvctl.cur_pat->p_next;
				}
			}

			AM_DEBUG(1, "current program %p", cur_prog);
			while (cur_prog && cur_prog->i_number == 0)
			{
				AM_DEBUG(1, "Skip for program number = 0");
				cur_prog = cur_prog->p_next;
			}

			recv_count = am_scan_get_recving_pmt_count(scanner);

			if (! cur_prog)
			{
				if (recv_count == 0)
				{
					AM_DEBUG(1,"All PMTs Done!");

					/*清除搜索标识*/
					scanner->recv_status &= ~scanner->dtvctl.pmtctl[0].recv_flag;
					SET_PROGRESS_EVT(AM_SCAN_PROGRESS_PMT_DONE, (void*)scanner->curr_ts->digital.pmts);
				}
				scanner->dtvctl.cur_prog = NULL;
				return AM_SUCCESS;
			}

			AM_DEBUG(1, "Start PMT for program %d, pmt_pid 0x%x", cur_prog->i_number, cur_prog->i_pid);
			am_scan_tablectl_clear(&scanner->dtvctl.pmtctl[i]);
			scanner->dtvctl.pmtctl[i].pid = cur_prog->i_pid;
			scanner->dtvctl.pmtctl[i].ext = cur_prog->i_number;

			ret = am_scan_request_section(scanner, &scanner->dtvctl.pmtctl[i]);
			if (ret == AM_DMX_ERR_NO_FREE_FILTER)
			{
				if (recv_count <= 0)
				{
					/* Can this case be true ? */
					AM_DEBUG(1, "No more filter for recving PMT, skip program %d", cur_prog->i_number);
					scanner->dtvctl.cur_prog = cur_prog;
				}
				else
				{
					AM_DEBUG(1, "No more filter for recving PMT, will start when getting a free filter");
				}

				return ret;
			}
			else if (ret != AM_SUCCESS)
			{
				AM_DEBUG(1, "Start PMT for program %d failed", cur_prog->i_number);
			}

			scanner->dtvctl.cur_prog = cur_prog;
		}
	}

	return ret;
}

/**\brief 从下一个主频点中搜索NIT表*/
static AM_ErrorCode_t am_scan_try_nit(AM_SCAN_Scanner_t *scanner)
{
	AM_ErrorCode_t ret;
	AM_SCAN_TSProgress_t tp;

	if (scanner->stage != AM_SCAN_STAGE_NIT)
	{
		scanner->stage = AM_SCAN_STAGE_NIT;
		/*开始搜索NIT*/
		SET_PROGRESS_EVT(AM_SCAN_PROGRESS_NIT_BEGIN, NULL);
	}

	if (scanner->curr_freq >= 0)
		SET_PROGRESS_EVT(AM_SCAN_PROGRESS_TS_END, NULL);

	do
	{
		scanner->curr_freq++;
		if (scanner->curr_freq < 0 || scanner->curr_freq >= scanner->start_freqs_cnt ||
			dvb_fend_para(cur_fe_para)->frequency <= 0)
		{
			AM_DEBUG(1, "Cannot get nit after all trings!");
			if (scanner->start_freqs)
			{
				free(scanner->start_freqs);
				scanner->start_freqs = NULL;
				scanner->start_freqs_cnt = 0;
				scanner->curr_freq = -1;
			}
			/*搜索结束*/
			scanner->stage = AM_SCAN_STAGE_DONE;
			SET_PROGRESS_EVT(AM_SCAN_PROGRESS_SCAN_END, (void*)(long)scanner->end_code);

			return AM_SCAN_ERR_CANNOT_GET_NIT;
		}

		AM_DEBUG(1, "Tring to recv NIT in frequency %u ...",
			dvb_fend_para(cur_fe_para)->frequency);

		tp.index = scanner->curr_freq;
		tp.total = scanner->start_freqs_cnt;
		tp.fend_para = cur_fe_para;
		SET_PROGRESS_EVT(AM_SCAN_PROGRESS_TS_BEGIN, (void*)&tp);

		HELPER_CB(AM_SCAN_HELPER_ID_FE_TYPE_CHANGE, (void*)(long)(cur_fe_para.m_type));

		AM_VLFEND_SetMode(scanner->start_para.vlfend_dev_id, 0);
		ret = AM_FENDCTRL_SetPara(scanner->start_para.fend_dev_id, &cur_fe_para);
		if (ret == AM_SUCCESS)
		{
			scanner->recv_status |= AM_SCAN_RECVING_WAIT_FEND;
		}
		else
		{
			SET_PROGRESS_EVT(AM_SCAN_PROGRESS_TS_END, NULL);
			AM_DEBUG(1, "AM_FENDCTRL_SetPara Failed, try next frequency");
		}

	}while(ret != AM_SUCCESS);

	return ret;
}

/**\brief 检查一个ATV频率的范围，超出范围后做相应调整*/
void am_scan_atv_check_freq_range(AM_SCAN_Scanner_t *scanner, int check_type, uint32_t *check_freq)
{
    uint32_t min_freq, max_freq;

    min_freq = (uint32_t)scanner->atvctl.min_freq;
    max_freq = (uint32_t)scanner->atvctl.max_freq;

    if (check_type == AM_SCAN_ATV_FREQ_RANGE_CHECK) {
        if (*check_freq > max_freq)
        {
            *check_freq = max_freq;
        }
        else if (*check_freq < min_freq)
        {
            *check_freq = min_freq;
        }
    } else if (check_type == AM_SCAN_ATV_FREQ_LOOP_CHECK) {
        if (*check_freq > max_freq)
        {
            *check_freq = min_freq;
        }
        else if (*check_freq < min_freq)
        {
            *check_freq = max_freq;
        }
    }else if (check_type == AM_SCAN_ATV_FREQ_INCREASE_CHECK) {
        if (*check_freq > max_freq)
        {
            *check_freq = max_freq;
        }
        else if (*check_freq < min_freq)
        {
            *check_freq = min_freq;
        }
    } else if (check_type == AM_SCAN_ATV_FREQ_DECREASE_CHECK) {
        if (*check_freq < max_freq)
        {
            *check_freq = max_freq;
        }
        else if (*check_freq > min_freq)
        {
            *check_freq = min_freq;
        }
    }
}

/**\brief 所有ATV/DTV搜索均已完成*/
static AM_ErrorCode_t am_scan_all_done(AM_SCAN_Scanner_t *scanner)
{
	AM_DEBUG(1, "All ATV & DTV Done !");

	if (scanner->start_freqs)
	{
		free(scanner->start_freqs);
		scanner->start_freqs = NULL;
		scanner->start_freqs_cnt = 0;
		scanner->curr_freq = -1;
	}

	/*搜索结束*/
	scanner->stage = AM_SCAN_STAGE_DONE;
	SET_PROGRESS_EVT(AM_SCAN_PROGRESS_SCAN_END, (void*)(long)scanner->end_code);

	return AM_SUCCESS;
}

/**\brief ATV搜索下一个频率*/
static AM_ErrorCode_t am_scan_atv_step_tune(AM_SCAN_Scanner_t *scanner)
{
	AM_ErrorCode_t ret = AM_FAILURE;

	//check if user pause
	am_scan_check_need_pause(scanner, AM_SCAN_STATUS_PAUSED_USER);

	if (cur_fe_para.m_type == FE_ANALOG)
	{
		if (atv_start_para.mode == AM_SCAN_ATVMODE_FREQ)
		{
			return am_scan_start_next_ts(scanner);
		}

		/* step to next */
		do
		{
			/* Auto/Manual scan reach the max freq ? */
			if ((((int)dvb_fend_para(cur_fe_para)->frequency >= scanner->atvctl.max_freq && (scanner->atvctl.direction == 1))
				|| ((int)dvb_fend_para(cur_fe_para)->frequency <= scanner->atvctl.max_freq && (scanner->atvctl.direction == 0))) &&
				(atv_start_para.mode == AM_SCAN_ATVMODE_AUTO || atv_start_para.mode == AM_SCAN_ATVMODE_MANUAL))
			{
				AM_DEBUG(1, "ATV auto/manual scan reach max freq.");
				return am_scan_start_next_ts(scanner);
			}

			/* try next frequency */
			dvb_fend_para(cur_fe_para)->frequency += scanner->atvctl.step * scanner->atvctl.direction;
			/* frequency range check */
			am_scan_atv_check_freq_range(scanner, scanner->atvctl.range_check, &dvb_fend_para(cur_fe_para)->frequency);
			AM_DEBUG(1, "ATV step tune: %s %dHz", scanner->atvctl.direction>0?"+":"-", scanner->atvctl.step);

			SET_PROGRESS_EVT(AM_SCAN_PROGRESS_ATV_TUNING, (void*)(long)(dvb_fend_para(cur_fe_para)->frequency));

			AM_DEBUG(1, "Trying to tune atv frequency %uHz (range:%d ~ %d)...",
				dvb_fend_para(cur_fe_para)->frequency,
				scanner->atvctl.min_freq, scanner->atvctl.max_freq);

			/* Set frontend */
			AM_FEND_SetMode(scanner->start_para.fend_dev_id, FE_ANALOG);/* release FE for VLFE */
			AM_VLFEND_SetMode(scanner->start_para.vlfend_dev_id, FE_ANALOG);/* set mode for VLFE */
			ret = AM_VLFEND_SetPara(scanner->start_para.vlfend_dev_id, &cur_fe_para);
			if (ret == AM_SUCCESS)
			{
				scanner->recv_status |= AM_SCAN_RECVING_WAIT_FEND;
			}
			else
			{
				AM_DEBUG(1, "Set frontend failed, try next frequency...");
			}
		}while(ret != AM_SUCCESS);
	}

	return ret;
}


static void get_freg_from_para(AM_SCAN_FrontEndPara_t *start_freq, int *freg)
{
	switch (start_freq->fe_para.m_type)
	{
		case FE_QPSK:
			*freg = start_freq->fe_para.sat.para.frequency;
			break;
		case FE_QAM:
			*freg = start_freq->fe_para.sat.para.frequency;
			break;
		case FE_OFDM:
			*freg = start_freq->fe_para.terrestrial.para.frequency;
			break;
		case FE_ATSC:
			*freg = start_freq->fe_para.atsc.para.frequency;
			break;
		case FE_ANALOG:
			*freg = start_freq->fe_para.analog.para.frequency;
			break;
		case FE_DTMB:
			*freg = start_freq->fe_para.dtmb.para.frequency;
			break;
		case FE_ISDBT:
			*freg = start_freq->fe_para.isdbt.para.frequency;
			break;
		default:
			break;
	}

}

static void set_freq_skip(AM_SCAN_Scanner_t *scanner)
{
	int i = 0;
	int cur_freq = -1;
	int near_freq = -1;

	if (scanner == NULL || scanner->start_freqs == NULL || scanner->curr_freq >= scanner->start_freqs_cnt)
		return;
	get_freg_from_para(&(scanner->start_freqs[scanner->curr_freq]),&cur_freq);
	for (i=scanner->curr_freq; i<scanner->start_freqs_cnt; i++)
	{
		if (scanner->start_freqs[i].skip == 6) {
			continue;
		}
		near_freq = -1;
		get_freg_from_para(&(scanner->start_freqs[i]),&near_freq);
		if (near_freq != -1 && (abs(near_freq - cur_freq) <= 3000000)) {
			scanner->start_freqs[i].skip = 6;
			AM_DEBUG(2, " ##### find it ##### near_freq[%d]=[%d]type[%d] ~~ cur_freq[%d]=[%d]type[%d]",
					i, near_freq, scanner->start_freqs[i].fe_para.m_type, scanner->curr_freq, cur_freq, cur_fe_para.m_type);
		}
	}

}


static bool check_freq_skip(AM_SCAN_Scanner_t *scanner)
{

	if (scanner->start_freqs[scanner->curr_freq].skip == 6) {
		AM_DEBUG(2, "This freg should be skip,start_freqs[%d].skip=%d type=%d",
				scanner->curr_freq, scanner->start_freqs[scanner->curr_freq].skip, cur_fe_para.m_type);
		return true;
	}
	return false;
}

static AM_ErrorCode_t am_scan_start_ts(AM_SCAN_Scanner_t *scanner, int step)
{
	AM_ErrorCode_t ret = AM_FAILURE;
	AM_SCAN_TSProgress_t tp;
	int frequency;
	bool skip_flag = false;

	if (scanner->stage != AM_SCAN_STAGE_TS)
		scanner->stage = AM_SCAN_STAGE_TS;

	//check if user pause
	am_scan_check_need_pause(scanner, AM_SCAN_STATUS_PAUSED_USER);

	if (scanner->curr_freq >= 0)
	{
		/* Pass stmts=NULL to notify new searched programs,
		 * this mechanism ensure that the programs notified will
		 * be actually stored.
		 */
		if (scanner->curr_ts != NULL)
		{
			AM_SCAN_NewProgram_Data_t npd;

			{/*notify services found*/
				if (cur_fe_para.m_type == FE_ANALOG)
					store_analog_ts(NULL, &scanner->result, scanner->curr_ts);
				else if (dtv_start_para.standard == AM_SCAN_DTV_STD_ATSC)
					store_atsc_ts(NULL, &scanner->result, scanner->curr_ts);
				else
					store_dvb_ts(NULL, &scanner->result, scanner->curr_ts, NULL);
			}

			/*store when found*/
			if (scanner->proc_mode & AM_SCAN_PROCMODE_STORE_CB_COMPLICATED) {
				if ((scanner->start_para.atv_para.mode == AM_SCAN_ATVMODE_AUTO
					|| scanner->start_para.atv_para.mode == AM_SCAN_ATVMODE_FREQ)
					|| (scanner->start_para.dtv_para.mode != AM_SCAN_DTVMODE_MANUAL)) {
					if (scanner->store_cb) {
						if (scanner->start_para.dtv_para.mode & AM_SCAN_DTVMODE_SCRAMB_TSHEAD) {
							AM_DEBUG(1, "AM_Check_Scramb_Stop --2");
							AM_Check_Scramb_Stop();
						}
						scanner->store_cb(&scanner->result);
					}
				}
			}
			AM_DEBUG(1, "AM_SCAN_PROGRESS_NEW_PROGRAM_MORE --1");
			npd.result = &scanner->result;
			npd.newts = scanner->curr_ts;
			SET_PROGRESS_EVT(AM_SCAN_PROGRESS_NEW_PROGRAM_MORE, (void*)&npd);
		}

		SET_PROGRESS_EVT(AM_SCAN_PROGRESS_TS_END, scanner->curr_ts);
	}

	scanner->curr_ts = NULL;

	do
	{
		if (scanner->curr_freq >= 0 && scanner->start_freqs[scanner->curr_freq].flag && !skip_flag)
		{
			/* if DTV not found in this freq, try atv */
			if (scanner->start_freqs[scanner->curr_freq].flag == AM_SCAN_FE_FL_ATV &&
				cur_fe_para.m_type != FE_ANALOG)
			{

				if (atv_start_para.mode == AM_SCAN_ATVMODE_MANUAL)
					dvb_fend_para(cur_fe_para)->u.analog.flag |= ANALOG_FLAG_MANUL_SCAN;
				else
					dvb_fend_para(cur_fe_para)->u.analog.flag |= ANALOG_FLAG_ENABLE_AFC;

				cur_fe_para.m_type = FE_ANALOG;
				dvb_fend_para(cur_fe_para)->u.analog.std = atv_start_para.default_std;
				if (atv_start_para.afc_range <= 0)
					atv_start_para.afc_range = ATV_2MHZ;
				dvb_fend_para(cur_fe_para)->u.analog.afc_range = atv_start_para.afc_range;
			}
			skip_flag = false;
		}
		else
		{
			/* try next freq */
			scanner->curr_freq += step;

			if (scanner->curr_freq < 0 || scanner->curr_freq >= scanner->start_freqs_cnt)
			{
				AM_DEBUG(1, "All TSes Complete!");
				return am_scan_all_done(scanner);
			}

			if (step == 1 && check_freq_skip(scanner)) {
				skip_flag = true;
				continue;
			}

			if (scanner->curr_freq >= (scanner->atvctl.start_idx + atv_start_para.fe_cnt) &&
				! scanner->dtvctl.start)
			{
				return am_scan_start_dtv(scanner);
			}
			else if (scanner->curr_freq >= (scanner->dtvctl.start_idx + dtv_start_para.fe_cnt) &&
				! scanner->atvctl.start&&atv_start_para.mode!= AM_SCAN_ATVMODE_NONE)
			{
				return am_scan_start_atv(scanner);
			}
		}

		tp.index = scanner->curr_freq;
		tp.total = scanner->start_freqs_cnt;
		tp.fend_para = cur_fe_para;
		SET_PROGRESS_EVT(AM_SCAN_PROGRESS_TS_BEGIN, (void*)&tp);

		AM_DEBUG(1, "Start scanning %s frequency[%d] %u ...",
			(cur_fe_para.m_type==FE_ANALOG)?"atv":"dtv",
			scanner->curr_freq,
			dvb_fend_para(cur_fe_para)->frequency);

		HELPER_CB(AM_SCAN_HELPER_ID_FE_TYPE_CHANGE, (void*)(long)(cur_fe_para.m_type));

		if (cur_fe_para.m_type == FE_ANALOG)
			scanner->start_freqs[scanner->curr_freq].flag &= ~AM_SCAN_FE_FL_ATV;
		else
			scanner->start_freqs[scanner->curr_freq].flag &= ~AM_SCAN_FE_FL_DTV;

		 if (IS_DVBT2()){
		        struct dtv_properties prop;
		        struct dtv_property property;

		        prop.num = 1;
		        prop.props = &property;
		        memset(&property, 0, sizeof(property));
		        property.cmd = DTV_DVBT2_PLP_ID;
		        property.u.data = -1;
		        AM_DEBUG(1, "Set PLP%d to receive non-SDT tables...", property.u.data);
		        AM_FEND_SetProp(scanner->start_para.fend_dev_id, &prop);
		}

		/* try set frontend */
		if (cur_fe_para.m_type == FE_ANALOG) {
			AM_FEND_SetMode(scanner->start_para.fend_dev_id, FE_ANALOG);/* release FE for VLFE */
			AM_VLFEND_SetMode(scanner->start_para.vlfend_dev_id, FE_ANALOG);/* set mode for VLFE */
			ret = AM_VLFEND_SetPara(scanner->start_para.vlfend_dev_id, &cur_fe_para);
		} else {
			AM_VLFEND_SetMode(scanner->start_para.vlfend_dev_id, 0);/* release VLFE for FE */
			ret = AM_FENDCTRL_SetPara(scanner->start_para.fend_dev_id, &cur_fe_para);
		}
		if (ret == AM_SUCCESS)
		{
			scanner->recv_status |= AM_SCAN_RECVING_WAIT_FEND;
		}
		else
		{
			SET_PROGRESS_EVT(AM_SCAN_PROGRESS_TS_END, scanner->curr_ts);
			AM_DEBUG(1, "Set frontend failed, try next ts...");
		}

	}while(ret != AM_SUCCESS);

	return ret;
}

/**\brief 从频率列表中选择下一个频点开始搜索*/
static AM_ErrorCode_t am_scan_start_next_ts(AM_SCAN_Scanner_t *scanner)
{
	return am_scan_start_ts(scanner, 1);
}

/**\brief 从频率列表中选择当前频点开始搜索*/
static AM_ErrorCode_t am_scan_start_current_ts(AM_SCAN_Scanner_t *scanner)
{
	return am_scan_start_ts(scanner, 0);
}

static void am_scan_fend_callback(long dev_no, int event_type, void *param, void *user_data)
{
	struct dvb_frontend_event *evt = (struct dvb_frontend_event*)param;
	AM_SCAN_Scanner_t *scanner = (AM_SCAN_Scanner_t*)user_data;

	UNUSED(dev_no);
	UNUSED(event_type);

	if (!scanner || !evt || (evt->status == 0))
		return;

	pthread_mutex_lock(&scanner->lock);
	scanner->fe_evt = *evt;
	scanner->evt_flag |= AM_SCAN_EVT_FEND;
	pthread_cond_signal(&scanner->cond);
	pthread_mutex_unlock(&scanner->lock);
}

static void atv_scan_fend_callback(long dev_no, int event_type, void *param, void *user_data)
{
	//struct v4l2_frontend_event *evt = (struct v4l2_frontend_event*)param;
	struct dvb_frontend_event *evt = (struct dvb_frontend_event*)param;
	AM_SCAN_Scanner_t *scanner = (AM_SCAN_Scanner_t*)user_data;

	UNUSED(dev_no);

	if (!scanner || !evt || (evt->status == 0))
		return;

	pthread_mutex_lock(&scanner->lock);
	/*
	scanner->fe_evt.status = evt->status;
	scanner->fe_evt.parameters.frequency = evt->parameters.frequency;
	scanner->fe_evt.parameters.u.analog.afc_range = evt->parameters.afc_range;
	scanner->fe_evt.parameters.u.analog.audmode = evt->parameters.audmode;
	scanner->fe_evt.parameters.u.analog.flag = evt->parameters.flag;
	scanner->fe_evt.parameters.u.analog.soundsys = evt->parameters.soundsys;
	scanner->fe_evt.parameters.u.analog.std = evt->parameters.std;
	*/
	scanner->fe_evt = *evt;
	scanner->evt_flag |= AM_SCAN_EVT_FEND;
	pthread_cond_signal(&scanner->cond);
	pthread_mutex_unlock(&scanner->lock);
}


/**\brief 卫星BlindScan回调*/
static void am_scan_blind_scan_callback(int dev_no, AM_FEND_BlindEvent_t *evt, void *user_data)
{
	AM_SCAN_Scanner_t *scanner = (AM_SCAN_Scanner_t*)user_data;

	if (!scanner)
		return;

	AM_DEBUG(1, "bs callback wait lock, scanner %p", scanner);
	pthread_mutex_lock(&scanner->lock);
	AM_DEBUG(1, "bs callback get lock");
	if (evt->status == AM_FEND_BLIND_UPDATETP)
	{
		int cnt = AM_SCAN_MAX_BS_TP_CNT;
		int i = 0;
		int j = 0;
		/* in order to filter invalid tp */
		int invalid_tp_cnt_cur = 0;
		struct dvb_frontend_parameters tps[AM_SCAN_MAX_BS_TP_CNT];
		struct dvb_frontend_parameters *new_tp_start;

		AM_FEND_BlindGetTPInfo(scanner->start_para.fend_dev_id, tps, (unsigned int)cnt);

		if (cnt > scanner->dtvctl.bs_ctl.get_tp_cnt)
		{
			scanner->dtvctl.bs_ctl.progress.new_tp_cnt = cnt - scanner->dtvctl.bs_ctl.get_tp_cnt;
			scanner->dtvctl.bs_ctl.progress.new_tps = tps + scanner->dtvctl.bs_ctl.get_tp_cnt;
			new_tp_start = &scanner->dtvctl.bs_ctl.searched_tps[scanner->dtvctl.bs_ctl.searched_tp_cnt];

			/* Center freq -> Transponder freq */
			for (i=0; i<scanner->dtvctl.bs_ctl.progress.new_tp_cnt; i++)
			{
				AM_DEBUG(1, "bs callback centre %d", scanner->dtvctl.bs_ctl.progress.new_tps[i].frequency);

				AM_SEC_FreqConvert(scanner->start_para.fend_dev_id,
					scanner->dtvctl.bs_ctl.progress.new_tps[i].frequency,
					&scanner->dtvctl.bs_ctl.progress.new_tps[i].frequency);

				AM_DEBUG(1, "bs callback tp %d", scanner->dtvctl.bs_ctl.progress.new_tps[i].frequency);
				if(AM_SEC_FilterInvalidTp(scanner->start_para.fend_dev_id, scanner->dtvctl.bs_ctl.progress.new_tps[i].frequency))
				{
					AM_DEBUG(1, "AM_SEC_FilterInvalidTp true");
					invalid_tp_cnt_cur++;
					scanner->dtvctl.bs_ctl.get_invalid_tp_cnt++;
					continue;
				}
				else
				{
					new_tp_start[j] = scanner->dtvctl.bs_ctl.progress.new_tps[i];
					j++;
				}
			}

			scanner->dtvctl.bs_ctl.progress.new_tp_cnt -= invalid_tp_cnt_cur;
			scanner->dtvctl.bs_ctl.progress.new_tps = new_tp_start;

			SET_PROGRESS_EVT(AM_SCAN_PROGRESS_BLIND_SCAN, (void*)&scanner->dtvctl.bs_ctl.progress);

			scanner->dtvctl.bs_ctl.get_tp_cnt = cnt;
			scanner->dtvctl.bs_ctl.searched_tp_cnt = cnt - scanner->dtvctl.bs_ctl.get_invalid_tp_cnt;
			scanner->dtvctl.bs_ctl.progress.new_tp_cnt = 0;
			scanner->dtvctl.bs_ctl.progress.new_tps = NULL;
		}
	}
	else if (evt->status == AM_FEND_BLIND_UPDATEPROCESS)
	{
		scanner->dtvctl.bs_ctl.progress.progress = (scanner->dtvctl.bs_ctl.stage - AM_SCAN_BS_STAGE_HL)*25 + evt->process/4;
		SET_PROGRESS_EVT(AM_SCAN_PROGRESS_BLIND_SCAN, (void*)&scanner->dtvctl.bs_ctl.progress);

		if (evt->process >= 100)
		{
			/* Current blind scan stage done */
			scanner->evt_flag |= AM_SCAN_EVT_BLIND_SCAN_DONE;
			pthread_cond_signal(&scanner->cond);
		}
	}
	else if (evt->status == AM_FEND_BLIND_START)
	{
		scanner->dtvctl.bs_ctl.progress.freq = evt->freq;
		AM_SEC_FreqConvert(scanner->start_para.fend_dev_id,
					scanner->dtvctl.bs_ctl.progress.freq,
					(unsigned int*)&scanner->dtvctl.bs_ctl.progress.freq);
		AM_DEBUG(1, "bs callback evt: BLIND_START");
		SET_PROGRESS_EVT(AM_SCAN_PROGRESS_BLIND_SCAN, (void*)&scanner->dtvctl.bs_ctl.progress);
		AM_DEBUG(1, "bs callback evt: BLIND_START end");
	}

	pthread_mutex_unlock(&scanner->lock);
}

/**\brief 启动一次卫星盲扫*/
static AM_ErrorCode_t am_scan_start_blind_scan(AM_SCAN_Scanner_t *scanner)
{
	AM_FEND_Polarisation_t p;
	AM_FEND_Localoscollatorfreq_t l;
	AM_SEC_DVBSatelliteEquipmentControl_t *sec;
	AM_ErrorCode_t ret = AM_FAILURE;
	AM_SEC_DVBSatelliteEquipmentControl_t setting;

	AM_DEBUG(1, "stage %d", scanner->dtvctl.bs_ctl.stage);
	if (scanner->dtvctl.bs_ctl.stage > (AM_SCAN_BS_STAGE_VH + 1))
	{
		AM_DEBUG(1, "Currently is not in BlindScan mode");
		return AM_FAILURE;
	}

	sec = &dtv_start_para.sat_para.sec;

	while (scanner->dtvctl.bs_ctl.stage <= AM_SCAN_BS_STAGE_VH && ret != AM_SUCCESS)
	{
		AM_DEBUG(1, "Start blind scan at stage %d", scanner->dtvctl.bs_ctl.stage);
		if (scanner->dtvctl.bs_ctl.stage == AM_SCAN_BS_STAGE_HL ||
			scanner->dtvctl.bs_ctl.stage == AM_SCAN_BS_STAGE_HH)
			p = AM_FEND_POLARISATION_H;
		else
			p = AM_FEND_POLARISATION_V;

		if (scanner->dtvctl.bs_ctl.stage == AM_SCAN_BS_STAGE_VL ||
			scanner->dtvctl.bs_ctl.stage == AM_SCAN_BS_STAGE_HL)
			l = AM_FEND_LOCALOSCILLATORFREQ_L;
		else
			l = AM_FEND_LOCALOSCILLATORFREQ_H;

		scanner->dtvctl.bs_ctl.progress.polar = p;
		scanner->dtvctl.bs_ctl.progress.lo = l;
		scanner->dtvctl.bs_ctl.searched_tp_cnt = 0;


		if ((sec->m_lnbs.m_lof_hi == sec->m_lnbs.m_lof_lo && l == AM_FEND_LOCALOSCILLATORFREQ_H) ||
			(sec->m_lnbs.m_cursat_parameters.m_voltage_mode == _14V && p == AM_FEND_POLARISATION_H) ||
		    (sec->m_lnbs.m_cursat_parameters.m_voltage_mode == _18V && p == AM_FEND_POLARISATION_V))
		{
			AM_DEBUG(1, "Skip blind scan stage %d", scanner->dtvctl.bs_ctl.stage);
		}
		else
		{
			AM_SEC_GetSetting(scanner->start_para.fend_dev_id, &setting);
			setting.m_lnbs.b_para.polarisation = p;
			setting.m_lnbs.b_para.ocaloscollatorfreq = l;
			AM_SEC_SetSetting(scanner->start_para.fend_dev_id, &setting);
			AM_SEC_PrepareBlindScan(scanner->start_para.fend_dev_id);

			/* int bs ctl */
			scanner->dtvctl.bs_ctl.get_tp_cnt = 0;
			scanner->dtvctl.bs_ctl.get_invalid_tp_cnt = 0;

			/* start blind scan */
			ret = AM_FEND_BlindScan(scanner->start_para.fend_dev_id, am_scan_blind_scan_callback,
				(void*)scanner, scanner->dtvctl.bs_ctl.start_freq, scanner->dtvctl.bs_ctl.stop_freq);
		}

		if (ret != AM_SUCCESS)
		{
			scanner->dtvctl.bs_ctl.progress.progress += 25;
			SET_PROGRESS_EVT(AM_SCAN_PROGRESS_BLIND_SCAN, (void*)&scanner->dtvctl.bs_ctl.progress);
			scanner->dtvctl.bs_ctl.stage++;
		}
	}

	if (ret != AM_SUCCESS && scanner->dtvctl.bs_ctl.stage >= AM_SCAN_BS_STAGE_VH)
	{
		AM_DEBUG(1, "All blind scan done! Now scan for each TP...");
		/* Change the scan mode to All band */
		dtv_start_para.mode = AM_SCAN_DTVMODE_ALLBAND | (dtv_start_para.mode & 0xf1);
		return am_scan_start_next_ts(scanner);
	}

	AM_DEBUG(1,"start blind scan return ret %d", ret);
	return ret;
}

/**\brief Start next data plp */
static AM_ErrorCode_t am_scan_dvbt2_start_next_data_plp(AM_SCAN_Scanner_t *scanner)
{
	AM_ErrorCode_t ret = AM_FAILURE;
	struct dtv_properties prop;
	struct dtv_property property;
	AM_SCAN_PLPProgress_t plp_prog;
	int i;

#define SET_PLP_PROGRESS(_p)\
	AM_MACRO_BEGIN\
		plp_prog.index = scanner->curr_plp;\
		plp_prog.total = scanner->curr_ts->digital.dvbt2_data_plp_num;\
		plp_prog.ts = scanner->curr_ts;\
		SET_PROGRESS_EVT(_p, &plp_prog);\
	AM_MACRO_END

	do
	{
		SET_PLP_PROGRESS(AM_SCAN_PROGRESS_DVBT2_PLP_END);
		scanner->curr_plp++;
		if (scanner->curr_plp >= scanner->curr_ts->digital.dvbt2_data_plp_num)
		{
			AM_DEBUG(1, "All data PLP complete, try next ts");
			scanner->curr_ts->digital.pats = NULL;

			/* if no dvbt2 plp found, try dvbt instead */
			for (i=0; i<scanner->curr_ts->digital.dvbt2_data_plp_num; i++)
			{
				if (scanner->curr_ts->digital.dvbt2_data_plps[i].pats != NULL)
					break;
			}
			if (i >= scanner->curr_ts->digital.dvbt2_data_plp_num)
			{
				scanner->start_freqs[scanner->curr_freq].
					fe_para.terrestrial.
					ofdm_mode = OFDM_DVBT;

				AM_DEBUG(1, "No DVBT2 plp data found, try DVBT...");
				return am_scan_start_current_ts(scanner);
			}
			else
			{
				return am_scan_start_next_ts(scanner);
			}
		}

		prop.num = 1;
		prop.props = &property;
		memset(&property, 0, sizeof(property));
		property.cmd = DTV_DVBT2_PLP_ID;
		property.u.data =scanner->curr_ts->digital.dvbt2_data_plps[scanner->curr_plp].id;
		AM_DEBUG(1, "Searching in PLP%d ...", property.u.data);
		SET_PLP_PROGRESS(AM_SCAN_PROGRESS_DVBT2_PLP_BEGIN);
		ret = AM_FEND_SetProp(scanner->start_para.fend_dev_id, &prop);
		if (ret != AM_SUCCESS)
		{
			AM_DEBUG(1, "Set PLP failed, try next ...");
		}
		else
		{
			scanner->curr_ts->digital.pats = NULL;
			//scanner->cur_prog = NULL;
			am_scan_tablectl_clear(&scanner->dtvctl.patctl);
			for (i=0; i<(int)AM_ARRAY_SIZE(scanner->dtvctl.pmtctl); i++)
			{
				am_scan_tablectl_clear(&scanner->dtvctl.pmtctl[i]);
			}
			am_scan_tablectl_clear(&scanner->dtvctl.catctl);
			am_scan_tablectl_clear(&scanner->dtvctl.sdtctl);

			/*请求数据*/
			am_scan_request_section(scanner, &scanner->dtvctl.patctl);
			am_scan_request_section(scanner, &scanner->dtvctl.catctl);
			am_scan_request_section(scanner, &scanner->dtvctl.sdtctl);

			/*In order to support lcn, we need scan NIT for each TS */
			am_scan_tablectl_clear(&scanner->dtvctl.nitctl);
			am_scan_request_section(scanner, &scanner->dtvctl.nitctl);
			am_scan_tablectl_clear(&scanner->dtvctl.sdtctl);
			ret = am_scan_request_section(scanner, &scanner->dtvctl.sdtctl);
		}
	}while (ret != AM_SUCCESS);

	return ret;
}

/**\brief ISDBT switching layer*/
static AM_ErrorCode_t am_scan_isdbt_switch_layer(AM_SCAN_Scanner_t *scanner, int layer)
{
	struct dtv_properties props;
	struct dtv_property prop;

	prop.cmd    = DTV_ISDBT_LAYER_ENABLED;
	prop.u.data = layer;

	props.num = 1;
	props.props = &prop;

	AM_DEBUG(1, "Switching to ISDBT layer %d(0-All, 1-A, 2-B, 3-C)", layer);

	return AM_FEND_SetProp(scanner->start_para.fend_dev_id, &props);
}

/**\brief prepare isdbt one-seg mode*/
static AM_ErrorCode_t am_scan_isdbt_prepare_oneseg(AM_SCAN_TS_t *ts)
{
	int i;
	AM_Bool_t found;
	dvbpsi_pat_t *new_pat, *pat;
	dvbpsi_pat_program_t *prog;
	const uint16_t oneseg_pmt_count = ISDBT_ONESEG_PMT_END - ISDBT_ONESEG_PMT_BEGIN + 1;

	dvbpsi_NewPAT(new_pat, 0xffff/*Mark*/, 0, 1);

	if (new_pat == NULL)
		return AM_SCAN_ERR_NO_MEM;

	new_pat->p_next = ts->digital.pats;

	for (i=0; i<oneseg_pmt_count; i++)
	{
		found = AM_FALSE;
		AM_SI_LIST_BEGIN(ts->digital.pats, pat)
			AM_SI_LIST_BEGIN(pat->p_first_program, prog)
				if (prog->i_pid == (ISDBT_ONESEG_PMT_BEGIN + i))
				{
					AM_DEBUG(1, "PMT(pid=0x%x) already in PAT, skip this.", prog->i_pid);
					found = AM_TRUE;
					break;
				}
			AM_SI_LIST_END()

			if (found)
				break;
		AM_SI_LIST_END()

		if (! found)
		{
			dvbpsi_PATAddProgram(new_pat, 0xffff/*Mark*/, ISDBT_ONESEG_PMT_BEGIN + i);
		}
	}

	ts->digital.pats = new_pat;

	return AM_SUCCESS;
}

/**\brief 启动DTV搜索*/
static AM_ErrorCode_t am_scan_start_dtv(AM_SCAN_Scanner_t *scanner)
{
	int i;

	if (scanner->dtvctl.start)
	{
		AM_DEBUG(1, "dtv scan already start");
		return AM_SUCCESS;
	}

	if (scanner->atv_open)
	{
		AM_VLFEND_Close(scanner->start_para.vlfend_dev_id);
		scanner->atv_open = AM_FALSE;
	}
	AM_DEBUG(1, "@@@ Start dtv scan use standard: %s @@@", (dtv_start_para.standard==AM_SCAN_DTV_STD_DVB)?"DVB" :
		(dtv_start_para.standard==AM_SCAN_DTV_STD_ISDB)?"ISDB" : "ATSC");

	/*创建SI解析器*/
	AM_TRY(AM_SI_Create(&scanner->dtvctl.hsi));

	/*接收控制数据初始化*/
	am_scan_tablectl_init(&scanner->dtvctl.patctl, AM_SCAN_RECVING_PAT, AM_SCAN_EVT_PAT_DONE, PAT_TIMEOUT,
						AM_SI_PID_PAT, AM_SI_TID_PAT, "PAT", 1, am_scan_pat_done, 0);

	for (i=0; i<(int)AM_ARRAY_SIZE(scanner->dtvctl.pmtctl); i++)
	{
		am_scan_tablectl_init(&scanner->dtvctl.pmtctl[i], AM_SCAN_RECVING_PMT, AM_SCAN_EVT_PMT_DONE, PMT_TIMEOUT,
						0x1fff, AM_SI_TID_PMT, "PMT", 1, am_scan_pmt_done, 0);
	}
	am_scan_tablectl_init(&scanner->dtvctl.catctl, AM_SCAN_RECVING_CAT, AM_SCAN_EVT_CAT_DONE, CAT_TIMEOUT,
							AM_SI_PID_CAT, AM_SI_TID_CAT, "CAT", 1, am_scan_cat_done, 0);
	am_scan_tablectl_init(&scanner->dtvctl.mgtctl, AM_SCAN_RECVING_MGT, AM_SCAN_EVT_MGT_DONE, MGT_TIMEOUT,
					AM_SI_ATSC_BASE_PID, AM_SI_TID_PSIP_MGT, "MGT", 1, am_scan_mgt_done, 0);
	am_scan_tablectl_init(&scanner->dtvctl.vctctl, AM_SCAN_RECVING_VCT, AM_SCAN_EVT_VCT_DONE, VCT_TIMEOUT,
					AM_SI_ATSC_BASE_PID, AM_SI_TID_PSIP_CVCT, "VCT", 1, am_scan_vct_done, 0);
	am_scan_tablectl_init(&scanner->dtvctl.sdtctl, AM_SCAN_RECVING_SDT, AM_SCAN_EVT_SDT_DONE, SDT_TIMEOUT,
						AM_SI_PID_SDT, AM_SI_TID_SDT_ACT, "SDT", 1, am_scan_sdt_done, 0);
	am_scan_tablectl_init(&scanner->dtvctl.nitctl, AM_SCAN_RECVING_NIT, AM_SCAN_EVT_NIT_DONE, NIT_TIMEOUT,
						AM_SI_PID_NIT, AM_SI_TID_NIT_ACT, "NIT", 1, am_scan_nit_done, 0);
	am_scan_tablectl_init(&scanner->dtvctl.batctl, AM_SCAN_RECVING_BAT, AM_SCAN_EVT_BAT_DONE, BAT_TIMEOUT,
						AM_SI_PID_BAT, AM_SI_TID_BAT, "BAT", MAX_BAT_SUBTABLE_CNT,
						am_scan_bat_done, BAT_REPEAT_DISTANCE);

	scanner->curr_freq = scanner->dtvctl.start_idx - 1;
	scanner->dtvctl.start = 1;
	scanner->end_code = AM_SCAN_RESULT_UNLOCKED;

	if (dtv_start_para.source == FE_QPSK)
	{
		AM_DEBUG(1, "Set SEC settings...");

		AM_SEC_SetSetting(scanner->start_para.fend_dev_id, &dtv_start_para.sat_para.sec);
	}

	if (IS_DVBT2())
	{
		struct dtv_properties prop;
		struct dtv_property property;

		prop.num = 1;
		prop.props = &property;
		memset(&property, 0, sizeof(property));
		property.cmd = DTV_DVBT2_PLP_ID;
		property.u.data = -1;
		AM_DEBUG(1, "Set PLP%d to receive non-SDT tables...", property.u.data);
		AM_FEND_SetProp(scanner->start_para.fend_dev_id, &prop);
	}

	/* 启动搜索 */
	if (dtv_start_para.standard == AM_SCAN_DTV_STD_DVB)
	{
		if (GET_MODE(dtv_start_para.mode) == AM_SCAN_DTVMODE_AUTO)
		{
			/*自动搜索模式时按指定主频点列表开始NIT表请求*/
			return am_scan_try_nit(scanner);
		}
		else if (GET_MODE(dtv_start_para.mode) == AM_SCAN_DTVMODE_SAT_BLIND)
		{
			/* 启动卫星盲扫 */
			scanner->dtvctl.bs_ctl.stage = AM_SCAN_BS_STAGE_HL;
			scanner->dtvctl.bs_ctl.progress.progress = 0;
			scanner->dtvctl.bs_ctl.start_freq = DEFAULT_DVBS_BS_FREQ_START;
			scanner->dtvctl.bs_ctl.stop_freq = DEFAULT_DVBS_BS_FREQ_STOP;
			scanner->start_freqs_cnt = atv_start_para.fe_cnt;

			AM_SEC_PrepareBlindScan(scanner->start_para.fend_dev_id);

			return am_scan_start_blind_scan(scanner);
		}
	}

	/*其他模式直接按指定频点开始搜索*/
	return am_scan_start_next_ts(scanner);
}

/**\brief 停止DTV搜索*/
static AM_ErrorCode_t am_scan_stop_dtv(AM_SCAN_Scanner_t *scanner)
{
	if (scanner->dtvctl.start)
	{
		int i;
		AM_SCAN_TS_t *ts, *tnext;

		AM_DEBUG(1, "Stopping dtv ...");

		/* If in satellite blind scan mode, stop it first */
		if (GET_MODE(dtv_start_para.mode) == AM_SCAN_DTVMODE_SAT_BLIND)
		{
			/* we must unlock, as BlindExit will wait the callback done */
			pthread_mutex_unlock(&scanner->lock);
			/* Stop blind scan */
			AM_FEND_BlindExit(scanner->start_para.fend_dev_id);
			pthread_mutex_lock(&scanner->lock);
		}

		/*DMX释放*/
		if (scanner->dtvctl.patctl.fid != -1)
			AM_DMX_FreeFilter(dtv_start_para.dmx_dev_id, scanner->dtvctl.patctl.fid);
		for (i=0; i<(int)AM_ARRAY_SIZE(scanner->dtvctl.pmtctl); i++)
		{
			if (scanner->dtvctl.pmtctl[i].fid != -1)
			AM_DMX_FreeFilter(dtv_start_para.dmx_dev_id, scanner->dtvctl.pmtctl[i].fid);
		}
		if (scanner->dtvctl.catctl.fid != -1)
			AM_DMX_FreeFilter(dtv_start_para.dmx_dev_id, scanner->dtvctl.catctl.fid);
		if (scanner->dtvctl.nitctl.fid != -1)
			AM_DMX_FreeFilter(dtv_start_para.dmx_dev_id, scanner->dtvctl.nitctl.fid);
		if (scanner->dtvctl.sdtctl.fid != -1)
			AM_DMX_FreeFilter(dtv_start_para.dmx_dev_id, scanner->dtvctl.sdtctl.fid);
		if (scanner->dtvctl.batctl.fid != -1)
			AM_DMX_FreeFilter(dtv_start_para.dmx_dev_id, scanner->dtvctl.batctl.fid);
		if (scanner->dtvctl.mgtctl.fid != -1)
			AM_DMX_FreeFilter(dtv_start_para.dmx_dev_id, scanner->dtvctl.mgtctl.fid);
		if (scanner->dtvctl.vctctl.fid != -1)
			AM_DMX_FreeFilter(dtv_start_para.dmx_dev_id, scanner->dtvctl.vctctl.fid);

		/*等待回调函数执行完毕*/
		pthread_mutex_unlock(&scanner->lock);
		AM_DEBUG(1, "Waiting for dmx callback...");
		AM_DMX_Sync(dtv_start_para.dmx_dev_id);
		AM_DEBUG(1, "OK, hsi is %p", scanner->dtvctl.hsi);
		pthread_mutex_lock(&scanner->lock);

		/*表控制释放*/
		am_scan_tablectl_deinit(&scanner->dtvctl.patctl);
		for (i=0; i<(int)AM_ARRAY_SIZE(scanner->dtvctl.pmtctl); i++)
		{
			am_scan_tablectl_deinit(&scanner->dtvctl.pmtctl[i]);
		}
		am_scan_tablectl_deinit(&scanner->dtvctl.catctl);
		am_scan_tablectl_deinit(&scanner->dtvctl.nitctl);
		am_scan_tablectl_deinit(&scanner->dtvctl.sdtctl);
		am_scan_tablectl_deinit(&scanner->dtvctl.batctl);
		am_scan_tablectl_deinit(&scanner->dtvctl.mgtctl);
		am_scan_tablectl_deinit(&scanner->dtvctl.vctctl);

		if (scanner->start_freqs != NULL)
			free(scanner->start_freqs);
		/*释放SI资源*/
		RELEASE_TABLE_FROM_LIST(dvbpsi_nit_t, scanner->result.nits);
		RELEASE_TABLE_FROM_LIST(dvbpsi_bat_t, scanner->result.bats);
		ts = scanner->result.tses;
		while (ts)
		{
			tnext = ts->p_next;
			//AM_DEBUG(1, "hsi is %d, ts->digital.pats is %p", scanner->dtvctl.hsi, ts->digital.pats);

			for (i=0; i<ts->digital.dvbt2_data_plp_num; i++)
			{
				if(ts->digital.dvbt2_data_plps[i].pats == ts->digital.pats)
					ts->digital.pats = NULL;
				if(ts->digital.dvbt2_data_plps[i].pmts == ts->digital.pmts)
					ts->digital.pmts = NULL;
				if(ts->digital.dvbt2_data_plps[i].sdts == ts->digital.sdts)
					ts->digital.sdts = NULL;

				RELEASE_TABLE_FROM_LIST(dvbpsi_pat_t, ts->digital.dvbt2_data_plps[i].pats);
				RELEASE_TABLE_FROM_LIST(dvbpsi_pmt_t, ts->digital.dvbt2_data_plps[i].pmts);
				RELEASE_TABLE_FROM_LIST(dvbpsi_sdt_t, ts->digital.dvbt2_data_plps[i].sdts);
			}

			RELEASE_TABLE_FROM_LIST(dvbpsi_pat_t, ts->digital.pats);
			RELEASE_TABLE_FROM_LIST(dvbpsi_pmt_t, ts->digital.pmts);
			RELEASE_TABLE_FROM_LIST(dvbpsi_cat_t, ts->digital.cats);
			RELEASE_TABLE_FROM_LIST(dvbpsi_sdt_t, ts->digital.sdts);
			RELEASE_TABLE_FROM_LIST(dvbpsi_atsc_vct_t, ts->digital.vcts);
			RELEASE_TABLE_FROM_LIST(dvbpsi_atsc_mgt_t, ts->digital.mgts);

			free(ts);

			ts = tnext;
		}

		AM_SI_Destroy(scanner->dtvctl.hsi);
		scanner->dtvctl.start = 0;
		AM_DEBUG(1, "dtv stopped !");
	}

	return AM_SUCCESS;
}

/**\brief 启动ATV搜索*/
static AM_ErrorCode_t am_scan_start_atv(AM_SCAN_Scanner_t *scanner)
{
	if (scanner->atvctl.start)
	{
		AM_DEBUG(1, "atv scan already start = %d", scanner->atvctl.start);
		return AM_SUCCESS;
	}
	AM_DEBUG(1, "@@@ Start atv scan use mode: %d @@@", atv_start_para.mode);

	//open atv device
	if (!scanner->atv_open)
	{
		AM_FEND_OpenPara_t para;

		para.mode = FE_ANALOG;
		AM_VLFEND_Open(scanner->start_para.vlfend_dev_id, &para);
		AM_EVT_Subscribe(scanner->start_para.vlfend_dev_id, AM_VLFEND_EVT_STATUS_CHANGED, atv_scan_fend_callback, (void*)scanner);

		scanner->atv_open = AM_TRUE;
	}
	if (atv_start_para.mode == AM_SCAN_ATVMODE_FREQ)
	{
		scanner->atvctl.direction = 1;
		scanner->atvctl.range_check = AM_SCAN_ATV_FREQ_INCREASE_CHECK;
		scanner->atvctl.step = 0;
		scanner->atvctl.min_freq = 0;
		scanner->atvctl.max_freq = 0;
		scanner->atvctl.start_freq = dvb_fend_para(scanner->start_freqs[scanner->atvctl.start_idx].fe_para)->frequency;
		scanner->curr_freq = scanner->atvctl.start_idx;
	}
	else if (atv_start_para.mode == AM_SCAN_ATVMODE_MANUAL)
	{
		scanner->atvctl.direction = (atv_start_para.direction>0) ? 1 : -1;
		scanner->atvctl.range_check = (atv_start_para.direction>0) ? AM_SCAN_ATV_FREQ_INCREASE_CHECK : AM_SCAN_ATV_FREQ_DECREASE_CHECK;
		scanner->atvctl.step = 0;
		scanner->atvctl.min_freq = dvb_fend_para(scanner->start_freqs[scanner->atvctl.start_idx].fe_para)->frequency;
		scanner->atvctl.max_freq = dvb_fend_para(scanner->start_freqs[scanner->atvctl.start_idx+1].fe_para)->frequency;
		scanner->atvctl.start_freq = dvb_fend_para(scanner->start_freqs[scanner->atvctl.start_idx+2].fe_para)->frequency ;
		//if(scanner->atvctl.direction == 1)
		//	dvb_fend_para(scanner->start_freqs[scanner->atvctl.start_idx+2].fe_para)->frequency+=ATV_3MHZ;
		//else
		//	dvb_fend_para(scanner->start_freqs[scanner->atvctl.start_idx+2].fe_para)->frequency-=ATV_3MHZ;
		//AM_DEBUG(1,"%s zk scanner->atvctl.start_freq = %d", __FUNCTION__,scanner->atvctl.start_freq);
		/* start from start freq */
		scanner->curr_freq = scanner->atvctl.start_idx + 2;
	}
	else
	{
		scanner->atvctl.direction = 1;
		scanner->atvctl.range_check = AM_SCAN_ATV_FREQ_INCREASE_CHECK;
		scanner->atvctl.step = 0;
		scanner->atvctl.min_freq = (int)dvb_fend_para(scanner->start_freqs[scanner->atvctl.start_idx].fe_para)->frequency;
		scanner->atvctl.max_freq = (int)dvb_fend_para(scanner->start_freqs[scanner->atvctl.start_idx+1].fe_para)->frequency;
		scanner->atvctl.start_freq = scanner->atvctl.min_freq;
		dvb_fend_para(scanner->start_freqs[scanner->atvctl.start_idx+2].fe_para)->frequency = scanner->atvctl.min_freq;
		/* start from start freq */
		scanner->curr_freq = scanner->atvctl.start_idx + 2;
	}

	scanner->atvctl.start = 1;



	return am_scan_start_next_ts(scanner);
}

/**\brief 停止ATV搜索*/
static AM_ErrorCode_t am_scan_stop_atv(AM_SCAN_Scanner_t *scanner)
{
	if (scanner->atvctl.start)
	{
		AM_SCAN_TS_t *ts, *tnext;
		int i;

		AM_DEBUG(1, "Stopping atv ...");
		AM_DEBUG(1, "Closing AFE device ...");

		if (scanner->atv_open)
		{
			AM_VLFEND_Close(scanner->start_para.vlfend_dev_id);
			scanner->atv_open = AM_FALSE;
		}

		ts = scanner->result.tses;
		while (ts)
		{
			tnext = ts->p_next;
			if(ts->type == AM_SCAN_TS_DIGITAL) {
				for (i=0; i<ts->digital.dvbt2_data_plp_num; i++)
				{
					if(ts->digital.dvbt2_data_plps[i].pats == ts->digital.pats)
						ts->digital.pats = NULL;
					if(ts->digital.dvbt2_data_plps[i].pmts == ts->digital.pmts)
						ts->digital.pmts = NULL;
					if(ts->digital.dvbt2_data_plps[i].sdts == ts->digital.sdts)
						ts->digital.sdts = NULL;

					RELEASE_TABLE_FROM_LIST(dvbpsi_pat_t, ts->digital.dvbt2_data_plps[i].pats);
					RELEASE_TABLE_FROM_LIST(dvbpsi_pmt_t, ts->digital.dvbt2_data_plps[i].pmts);
					RELEASE_TABLE_FROM_LIST(dvbpsi_sdt_t, ts->digital.dvbt2_data_plps[i].sdts);
				}

				RELEASE_TABLE_FROM_LIST(dvbpsi_pat_t, ts->digital.pats);
				RELEASE_TABLE_FROM_LIST(dvbpsi_pmt_t, ts->digital.pmts);
				RELEASE_TABLE_FROM_LIST(dvbpsi_cat_t, ts->digital.cats);
				RELEASE_TABLE_FROM_LIST(dvbpsi_sdt_t, ts->digital.sdts);
				RELEASE_TABLE_FROM_LIST(dvbpsi_atsc_vct_t, ts->digital.vcts);
				RELEASE_TABLE_FROM_LIST(dvbpsi_atsc_mgt_t, ts->digital.mgts);
			}

			free(ts);

			ts = tnext;
		}
		scanner->result.tses = NULL;


		scanner->atvctl.start = 0;
		AM_DEBUG(1, "atv stopped !");
	}

	return AM_SUCCESS;
}

/**\brief 启动搜索*/
static AM_ErrorCode_t am_scan_start(AM_SCAN_Scanner_t *scanner)
{
	/*注册前端事件*/
	AM_EVT_Subscribe(scanner->start_para.fend_dev_id, AM_FEND_EVT_STATUS_CHANGED, am_scan_fend_callback, (void*)scanner);

	if (scanner->start_para.mode == AM_SCAN_MODE_ATV_DTV)
	{
		return am_scan_start_atv(scanner);
	}
	else
	{
		if (scanner->start_para.mode == AM_SCAN_MODE_ADTV)
		{
			/* open atv devices */
		}

		return am_scan_start_dtv(scanner);
	}
}

/**\brief 完成一次卫星盲扫*/
static void am_scan_solve_blind_scan_done_evt(AM_SCAN_Scanner_t *scanner)
{
	AM_ErrorCode_t ret = AM_FAILURE;
	int i, index;

	if (scanner->dtvctl.bs_ctl.stage == 0)
	{
		/* start from the new-searched TPs */
		dtv_start_para.fe_cnt = 0;
	}

	index = scanner->dtvctl.start_idx + dtv_start_para.fe_cnt;

	/*Copy 本次搜索到的新TP到start_freqs*/
	for (i=0; i<scanner->dtvctl.bs_ctl.searched_tp_cnt && dtv_start_para.fe_cnt < AM_SCAN_MAX_BS_TP_CNT; i++)
	{
		scanner->start_freqs[index].flag | AM_SCAN_FE_FL_DTV;
		scanner->start_freqs[index].fe_para.m_type = dtv_start_para.source;
		scanner->start_freqs[index].fe_para.sat.polarisation = scanner->dtvctl.bs_ctl.progress.polar;
		*dvb_fend_para(scanner->start_freqs[index].fe_para) = scanner->dtvctl.bs_ctl.searched_tps[i];
		index++;
		scanner->start_freqs_cnt++;
		dtv_start_para.fe_cnt++;
	}

	AM_DEBUG(1, "Blind scan stage done, start %d, total fe %d, dtv fe %d",
		scanner->dtvctl.start_idx, scanner->start_freqs_cnt, dtv_start_para.fe_cnt);

	/* 退出本次盲扫 */
	AM_FEND_BlindExit(scanner->start_para.fend_dev_id);

	scanner->dtvctl.bs_ctl.stage++;
	am_scan_start_blind_scan(scanner);
}

AM_ErrorCode_t AM_Scan_Update_Digital_FrontendPara(AM_FENDCTRL_DVBFrontendParameters_t *para, unsigned int frequency)
{

	AM_ErrorCode_t ret = AM_SUCCESS;

	switch (para->m_type)
	{
		case FE_QPSK:
			para->sat.para.frequency = frequency;
			break;
		case FE_QAM:
			para->cable.para.frequency = frequency;
			break;
		case FE_OFDM:
			para->terrestrial.para.frequency = frequency;
			break;
		case FE_ATSC:
			para->atsc.para.frequency = frequency;
			break;
		case FE_ANALOG:
			para->analog.para.frequency = frequency;
			break;
		case FE_DTMB:
			para->dtmb.para.frequency = frequency;
			break;
		case FE_ISDBT:
			para->isdbt.para.frequency = frequency;
			break;
		default:
			break;
	}
	return ret;
}
/**\brief 当前频点锁定后的处理*/
static int am_scan_new_ts_locked_proc(AM_SCAN_Scanner_t *scanner)
{
	uint8_t plp_ids[256];
	uint8_t plp_num = 0;

	AM_SCAN_ATV_LOCK_PARA_t atv_lock_para;
	if (scanner->end_code == AM_SCAN_RESULT_UNLOCKED)
		scanner->end_code = AM_SCAN_RESULT_OK;

	/* Get DVB-T2 data plps */
	if (IS_DVBT2())
	{
		struct dtv_properties prop;
		struct dtv_property property;
		AM_SCAN_PLPProgress_t plp_prog;

		prop.num = 1;
		prop.props = &property;

		memset(&property, 0, sizeof(property));
		property.cmd = DTV_DVBT2_PLP_ID;
		property.u.buffer.reserved1[1] = UINT_MAX;/*get plps*/
		property.u.buffer.reserved2 = plp_ids;
		AM_FEND_GetProp(scanner->start_para.fend_dev_id, &prop);
		plp_num = property.u.buffer.reserved1[0];
		AM_DEBUG(1, "DVB-T2 get %d PLPs\n", plp_num);

		memset(&property, 0, sizeof(property));
		property.cmd = DTV_DVBT2_PLP_ID;
		property.u.data = plp_ids[0];
		AM_DEBUG(1, "Set PLP%d to receive non-SDT tables...", property.u.data);
		AM_FEND_SetProp(scanner->start_para.fend_dev_id, &prop);

		plp_prog.index = 0;
		plp_prog.total = plp_num;
		plp_prog.ts = NULL;
		SET_PROGRESS_EVT(AM_SCAN_PROGRESS_DVBT2_PLP_BEGIN, &plp_prog);
	}

	if (scanner->stage == AM_SCAN_STAGE_TS)
	{
		/* For analog ts, we must check the tv decoder */
		if (0 /*cur_fe_para.m_type == FE_ANALOG */)
		{
			atv_lock_para.pData = scanner->user_data;
			atv_lock_para.checkStable = (scanner->proc_mode & AM_SCAN_PROCMODE_AUTOPAUSE_ON_ATV_FOUND) ? 1 : 0;
			if(!(scanner->atvctl.am_scan_atv_cvbs_lock(&atv_lock_para)))
			{
				AM_DEBUG(1, "cvbs unlock !");
				scanner->atvctl.step = atv_start_para.cvbs_unlocked_step;
				am_scan_atv_step_tune(scanner);
				return 0;
			}
		}

		/*频点锁定，新申请内存以添加该频点信息*/
		scanner->curr_ts = (AM_SCAN_TS_t*)malloc(sizeof(AM_SCAN_TS_t));
		if (scanner->curr_ts == NULL)
		{
			AM_DEBUG(1, "Error, no enough memory for adding a new ts");\
			return -1;
		}
		memset(scanner->curr_ts, 0, sizeof(AM_SCAN_TS_t));
		scanner->curr_ts->tp_index = scanner->curr_freq;

		if (cur_fe_para.m_type == FE_ANALOG)
		{
			AM_SCAN_DTVSignalInfo_t si;
			int formatted_freq;
			AM_SCAN_NewProgram_Data_t npd;
			struct dvb_frontend_parameters para;
			int mode = scanner->start_para.store_mode;

			/*AM_FEND_GetPara(scanner->start_para.fend_dev_id, &para);*/
			AM_VLFEND_GetPara(scanner->start_para.vlfend_dev_id, &para);
			AM_DEBUG(1, "cvbs lock !");
			formatted_freq = am_scan_format_atv_freq(scanner->atvctl.afc_locked_freq);
			memset(&si, 0, sizeof(si));
			si.frequency = formatted_freq;
			si.locked = AM_TRUE;

			AM_DEBUG(1,"request_destory = %#x", scanner->request_destory);

			if (scanner->proc_mode & AM_SCAN_PROCMODE_AUTOPAUSE_ON_ATV_FOUND)
			{
				AM_DEBUG(1, "AUTOPAUSE ATV FOUND");
				pthread_mutex_lock(&scanner->lock_pause);
				scanner->status |= AM_SCAN_STATUS_PAUSED;
				pthread_mutex_unlock(&scanner->lock_pause);
			}
			if (((para.u.analog.std & V4L2_COLOR_STD_PAL) == V4L2_COLOR_STD_PAL) &&
				(mode & AM_SCAN_ATV_STOREMODE_NOPAL) == AM_SCAN_ATV_STOREMODE_NOPAL)
			{
				AM_DEBUG(1, "Skip program for atv pal mode at lock proc");
				goto next_ts;
			} else {
				AM_DEBUG(1, "not Skip program for atv pal mode at lock proc std[%x]pal[%x]mode[%d]", (unsigned int) para.u.analog.std, V4L2_COLOR_STD_PAL, mode);
			}
			if (((para.u.analog.std & V4L2_COLOR_STD_SECAM) == V4L2_COLOR_STD_SECAM) &&
				(mode & AM_SCAN_DTV_STOREMODE_NOSECAM) == AM_SCAN_DTV_STOREMODE_NOSECAM)
			{
				AM_DEBUG(1, "Skip program for atv secam mode at lock proc");
				goto next_ts;
			} else {
				AM_DEBUG(1, "not Skip program for atv secam mode at lock proc std[%x]pal[%x]mode[%d]", (unsigned int) para.u.analog.std, V4L2_COLOR_STD_PAL, mode);
			}

			SIGNAL_EVENT(AM_SCAN_EVT_SIGNAL, (void*)&si);

			/* Analog processing */
			scanner->curr_ts->type = AM_SCAN_TS_ANALOG;
			scanner->curr_ts->analog.freq = formatted_freq;

			//scanner->curr_ts->analog.std = scanner->start_para.atv_para.default_std;
			//scanner->curr_ts->analog.std = scanner->fe_evt.parameters.u.analog.std;
			scanner->curr_ts->analog.std = para.u.analog.std;
			scanner->curr_ts->analog.audmode = para.u.analog.audmode;
			//scanner->curr_ts->analog.std = dvb_fend_para(cur_fe_para)->u.analog.std;
			AM_DEBUG(1, "scanner->curr_ts->analog.std [0x%x] audmode [0x%x]", scanner->curr_ts->analog.std, scanner->curr_ts->analog.audmode);

			/*添加到搜索结果列表*/
			APPEND_TO_LIST(AM_SCAN_TS_t, scanner->curr_ts, scanner->result.tses);

			/*store when found*/
			if (scanner->proc_mode & AM_SCAN_PROCMODE_STORE_CB_COMPLICATED) {
				if (scanner->start_para.atv_para.mode == AM_SCAN_ATVMODE_AUTO
					|| scanner->start_para.atv_para.mode == AM_SCAN_ATVMODE_FREQ) {
					if (scanner->store_cb) {
						scanner->store_cb(&scanner->result);
					}
				}
			}

			npd.result = &scanner->result;
			npd.newts = scanner->curr_ts;
			AM_DEBUG(1, "AM_SCAN_PROGRESS_NEW_PROGRAM_MORE --analog");
			SET_PROGRESS_EVT(AM_SCAN_PROGRESS_NEW_PROGRAM_MORE, (void*)&npd);
next_ts:
			//check if auto pause
			am_scan_check_need_pause(scanner, AM_SCAN_STATUS_PAUSED);

			/* to support atv manual scan can pause when fonud a channel, then resume by user */
			if (atv_start_para.mode == AM_SCAN_ATVMODE_MANUAL && (scanner->proc_mode & AM_SCAN_PROCMODE_NORMAL))
			{
				AM_DEBUG(1, "ATV manual scan done!");
				/* this will cause start dtv or complete scan */
				am_scan_start_next_ts(scanner);
			}
			else
			{
				scanner->atvctl.step =  atv_start_para.cvbs_locked_step;
				am_scan_atv_step_tune(scanner);
			}
		}
		else
		{
			int i;

			/* Digital processing */
			scanner->curr_ts->type = AM_SCAN_TS_DIGITAL;

			scanner->curr_ts->digital.snr = 0;
			scanner->curr_ts->digital.ber = 0;
			scanner->curr_ts->digital.strength = 0;
			if (scanner->start_para.dtv_para.mode & (AM_SCAN_DTVMODE_SCRAMB_TSHEAD|AM_SCAN_DTVMODE_CHECKDATA)) {
				int ret = AM_Check_Scramb_Start(SCAN_DVR_DEV_ID, SCAN_DVR_SRC, SCAN_DMX_DEV_ID, SCAN_DMX_SRC);
				if (ret != AM_SUCCESS) {
					AM_Check_Scramb_Stop();
					AM_Check_Scramb_Start(SCAN_DVR_DEV_ID, SCAN_DVR_SRC, SCAN_DMX_DEV_ID, SCAN_DMX_SRC);
				}
		    }
			if (dtv_start_para.standard == AM_SCAN_DTV_STD_ATSC)
			{
				am_scan_tablectl_clear(&scanner->dtvctl.patctl);
				for (i=0; i<(int)AM_ARRAY_SIZE(scanner->dtvctl.pmtctl); i++)
				{
					am_scan_tablectl_clear(&scanner->dtvctl.pmtctl[i]);
				}
				/*am_scan_tablectl_clear(&scanner->dtvctl.catctl);*/
				am_scan_tablectl_clear(&scanner->dtvctl.mgtctl);
				am_scan_tablectl_clear(&scanner->dtvctl.vctctl);

				/*请求数据*/
				am_scan_request_section(scanner, &scanner->dtvctl.patctl);
				/*am_scan_request_section(scanner, &scanner->dtvctl.catctl);*/
				am_scan_request_section(scanner, &scanner->dtvctl.mgtctl);
			}
			else
			{
				if (IS_DVBT2())
				{
					scanner->curr_ts->digital.dvbt2_data_plp_num = plp_num;
					for (i=0; i<plp_num; i++)
					{
						scanner->curr_ts->digital.dvbt2_data_plps[i].id = plp_ids[i];
						scanner->curr_ts->digital.dvbt2_data_plps[i].sdts = NULL;
					}
					scanner->curr_plp = 0;
				}
				else if (IS_ISDBT())
				{
					int layer;

					if (dtv_start_para.mode&AM_SCAN_DTVMODE_ISDBT_ONESEG)
						layer = Layer_A;
					else
						layer = Layer_A_B_C;

					am_scan_isdbt_switch_layer(scanner, layer);
				}

				am_scan_tablectl_clear(&scanner->dtvctl.patctl);
				for (i=0; i<(int)AM_ARRAY_SIZE(scanner->dtvctl.pmtctl); i++)
				{
					am_scan_tablectl_clear(&scanner->dtvctl.pmtctl[i]);
				}
				am_scan_tablectl_clear(&scanner->dtvctl.catctl);
				am_scan_tablectl_clear(&scanner->dtvctl.sdtctl);

				/*请求数据*/
				am_scan_request_section(scanner, &scanner->dtvctl.patctl);
				/*am_scan_request_section(scanner, &scanner->dtvctl.catctl);*/
				am_scan_request_section(scanner, &scanner->dtvctl.sdtctl);

				am_scan_tablectl_clear(&scanner->dtvctl.nitctl);
				//if (dtv_start_para.source != FE_QPSK||dtv_start_para.source !=FE_QAM)
				if(scanner->curr_freq>=0){
					if ((cur_fe_para.m_type!= FE_QPSK)&&(cur_fe_para.m_type !=FE_QAM))
					{
						/*In order to support lcn, we need scan NIT for each TS */
						am_scan_request_section(scanner, &scanner->dtvctl.nitctl);
					}
				}
			}

			scanner->curr_ts->digital.fend_para = cur_fe_para;
			//update freq for locked real freq
			if (scanner->fe_evt.parameters.frequency > 0) {
				//AM_DEBUG(1, "real scanner->fe_evt.parameters.frequency %d", scanner->fe_evt.parameters.frequency);
				AM_Scan_Update_Digital_FrontendPara(&scanner->curr_ts->digital.fend_para, scanner->fe_evt.parameters.frequency);
			}
			scanner->start_freqs[scanner->curr_freq].flag = 0;
			/*添加到搜索结果列表*/
			APPEND_TO_LIST(AM_SCAN_TS_t, scanner->curr_ts, scanner->result.tses);
		}
	}
	else if (scanner->stage == AM_SCAN_STAGE_NIT)
	{
		am_scan_tablectl_clear(&scanner->dtvctl.nitctl);
		am_scan_tablectl_clear(&scanner->dtvctl.batctl);

		/*请求NIT, BAT数据*/
		am_scan_request_section(scanner, &scanner->dtvctl.nitctl);
		if (dtv_start_para.mode & AM_SCAN_DTVMODE_SEARCHBAT)
			am_scan_request_section(scanner, &scanner->dtvctl.batctl);
	}
	else
	{
		AM_DEBUG(1, "FEND event arrive at unexpected scan stage %d", scanner->stage);
	}

	return 0;
}

/**\brief Check if the arrived fend event is valid */
static int am_scan_check_fend_evt(AM_SCAN_Scanner_t *scanner)
{
	unsigned int freq;
	unsigned int cur_drift, max_drift;

	if (scanner->curr_freq < 0 || scanner->curr_freq >= scanner->start_freqs_cnt)
	{
		AM_DEBUG(1, "Unexpected fend_evt arrived: curr_freq = %d", scanner->curr_freq);
		return -1;
	}
	if ( ! (scanner->recv_status & AM_SCAN_RECVING_WAIT_FEND))
	{
		AM_DEBUG(1, "Unexpected fend_evt arrived: donot wait for any fend event");
		return -1;
	}

	freq = scanner->fe_evt.parameters.frequency;

	if (cur_fe_para.m_type == FE_ANALOG)
	{
		if (abs(dvb_fend_para(cur_fe_para)->frequency - freq) > atv_start_para.afc_range)
		{
			AM_DEBUG(1, "Analog frequency exceeds afc_range, expected/got:(%u/%u)",
				dvb_fend_para(cur_fe_para)->frequency, freq);
			if (scanner->fe_evt.status & FE_TIMEDOUT)
				return -1; //ignore
		}
	}
	else if (dtv_start_para.source == FE_QPSK)
	{
		/* For DVBS, we must do some extra control */

		int tmp_drift;

		AM_ErrorCode_t ret = AM_SEC_FreqConvert(scanner->start_para.fend_dev_id, scanner->fe_evt.parameters.frequency, &freq);
		if(ret != AM_SUCCESS)
		{
			AM_DEBUG(1, "error sec freq convert, try next\n");
			scanner->recv_status &= ~AM_SCAN_RECVING_WAIT_FEND;
			return 1;
		}

		tmp_drift = dvb_fend_para(cur_fe_para)->frequency - freq;
		cur_drift = AM_ABS(tmp_drift);
		/*algorithm from kernel*/
		max_drift = cur_fe_para.sat.para.u.qpsk.symbol_rate / 2000;

		if ((cur_drift > max_drift) &&
			!(dtv_start_para.mode&AM_SCAN_DTVMODE_SAT_UNICABLE))
		{
			AM_DEBUG(1, "Unexpected fend_evt arrived dvbs %d %d", cur_drift, max_drift);
			return -1;
		}
	}
	else
	{
		if (IS_ATSC_QAM_TS(cur_fe_para) ?
			abs(dvb_fend_para(cur_fe_para)->frequency - freq) > 3000000 :
			dvb_fend_para(cur_fe_para)->frequency != freq)
		{
			AM_DEBUG(1, "Unexpected fend_evt arrived, expected/got:(%u/%u)",
				dvb_fend_para(cur_fe_para)->frequency, freq);
			return -1;
		}
	}

	return 0;
}

/**\brief SCAN前端事件处理*/
static void am_scan_solve_fend_evt(AM_SCAN_Scanner_t *scanner)
{
	int ret;
	AM_SCAN_DTVSignalInfo_t si;

	ret = am_scan_check_fend_evt(scanner);
	if (ret < 0)
		return;
	else if (ret > 0)
		goto try_next;

	AM_DEBUG(1, "Scan get fend event: %u %s!", scanner->fe_evt.parameters.frequency, \
			(scanner->fe_evt.status&FE_HAS_LOCK) ? "Locked" : "Unlocked");

	memset(&si, 0, sizeof(si));
	/*获取前端信号质量等信息并通知*/
	if (cur_fe_para.m_type != FE_ANALOG &&
		GET_MODE(dtv_start_para.mode) != AM_SCAN_DTVMODE_SAT_BLIND)
	{
		AM_FEND_GetSNR(scanner->start_para.fend_dev_id, &si.snr);
		AM_FEND_GetBER(scanner->start_para.fend_dev_id, &si.ber);
		AM_FEND_GetStrength(scanner->start_para.fend_dev_id, &si.strength);
		si.locked = (scanner->fe_evt.status&FE_HAS_LOCK) ? AM_TRUE : AM_FALSE;
	}
	si.frequency = scanner->fe_evt.parameters.frequency;

	if (cur_fe_para.m_type != FE_ANALOG) /*atv lock = afc lock + cvbs lock*/
		SIGNAL_EVENT(AM_SCAN_EVT_SIGNAL, (void*)&si);

	scanner->recv_status &= ~AM_SCAN_RECVING_WAIT_FEND;

	if (scanner->fe_evt.status & FE_HAS_LOCK)
	{
		if (cur_fe_para.m_type == FE_ANALOG)
		{
			/* Update the frequency */
			scanner->atvctl.afc_locked_freq = scanner->fe_evt.parameters.frequency;
		}

	    set_freq_skip(scanner);

		/* Get locked, process it */
		if (am_scan_new_ts_locked_proc(scanner) < 0) {
			goto try_next;
		} else {
			return;
		}
	}
	else if (scanner->stage == AM_SCAN_STAGE_TS)
	{
		if(IS_DVBT2())
		{
			AM_DEBUG(1, "Can not lock DVBT2, try DVBT...");
			scanner->start_freqs[scanner->curr_freq].
				fe_para.terrestrial.
				ofdm_mode = OFDM_DVBT;
			am_scan_start_current_ts(scanner);
			return;
		} else if (IS_ATSC_QAM256()) {
			AM_DEBUG(1, "Can not lock QAM256, try QAM64...");
			scanner->start_freqs[scanner->curr_freq].
				fe_para.atsc.para.u.vsb.modulation = QAM_64;
			am_scan_start_current_ts(scanner);
			return;
		}
	}

try_next:
	/*尝试下一频点*/
	if (cur_fe_para.m_type == FE_ANALOG)
	{
		scanner->atvctl.step = atv_start_para.afc_unlocked_step;
		am_scan_atv_step_tune(scanner);
	}
	else if (scanner->stage == AM_SCAN_STAGE_NIT)
	{
		am_scan_try_nit(scanner);
	}
	else if (scanner->stage == AM_SCAN_STAGE_TS)
	{
		am_scan_start_next_ts(scanner);
	}

}

/**\brief 比较2个timespec*/
static int am_scan_compare_timespec(const struct timespec *ts1, const struct timespec *ts2)
{
	assert(ts1 && ts2);

	if ((ts1->tv_sec > ts2->tv_sec) ||
		((ts1->tv_sec == ts2->tv_sec)&&(ts1->tv_nsec > ts2->tv_nsec)))
		return 1;

	if ((ts1->tv_sec == ts2->tv_sec) && (ts1->tv_nsec == ts2->tv_nsec))
		return 0;

	return -1;
}

/**\brief 计算下一等待超时时间，存储到ts所指结构中*/
static void am_scan_get_wait_timespec(AM_SCAN_Scanner_t *scanner, struct timespec *ts)
{
	int rel, i;
	struct timespec now;

#define TIMEOUT_CHECK(table)\
	AM_MACRO_BEGIN\
		struct timespec end;\
		if (scanner->recv_status & scanner->dtvctl.table.recv_flag){\
			end = scanner->dtvctl.table.end_time;\
			rel = am_scan_compare_timespec(&scanner->dtvctl.table.end_time, &now);\
			if (rel <= 0){\
				AM_DEBUG(1, "%s:0x%x timeout", scanner->dtvctl.table.tname, scanner->dtvctl.table.pid);\
				scanner->dtvctl.table.data_arrive_time = 1;/*just mark it*/\
				scanner->dtvctl.table.done(scanner);\
			}\
			if (rel > 0 || memcmp(&end, &scanner->dtvctl.table.end_time, sizeof(struct timespec))){\
				if ((ts->tv_sec == 0 && ts->tv_nsec == 0) || \
					(am_scan_compare_timespec(&scanner->dtvctl.table.end_time, ts) < 0))\
					*ts = scanner->dtvctl.table.end_time;\
			}\
		}\
	AM_MACRO_END

	ts->tv_sec = 0;
	ts->tv_nsec = 0;
	AM_TIME_GetTimeSpec(&now);

	/*超时检查*/
	TIMEOUT_CHECK(patctl);
	for (i=0; i<(int)AM_ARRAY_SIZE(scanner->dtvctl.pmtctl); i++)
	{
		if (scanner->dtvctl.pmtctl[i].fid >= 0)
			TIMEOUT_CHECK(pmtctl[i]);
	}
	TIMEOUT_CHECK(catctl);
	if (dtv_start_para.standard == AM_SCAN_DTV_STD_ATSC)
	{
		TIMEOUT_CHECK(mgtctl);
		TIMEOUT_CHECK(vctctl);
	}
	else
	{
		TIMEOUT_CHECK(sdtctl);
		if(scanner->curr_freq>=0){
			if ((cur_fe_para.m_type!= FE_QPSK)/*&&(cur_fe_para.m_type !=FE_QAM)*/){
				TIMEOUT_CHECK(nitctl);
			}
		}
		if (dtv_start_para.mode & AM_SCAN_DTVMODE_SEARCHBAT)
			TIMEOUT_CHECK(batctl);
	}

	/* All tables complete, try next ts */
	if (scanner->stage == AM_SCAN_STAGE_TS && \
		scanner->recv_status == AM_SCAN_RECVING_COMPLETE)
	{
		/*Stop filters*/
		for (i=0; i<(int)AM_ARRAY_SIZE(scanner->dtvctl.pmtctl); i++)
		{
			am_scan_free_filter(scanner, &scanner->dtvctl.pmtctl[i].fid);
		}
		am_scan_free_filter(scanner, &scanner->dtvctl.patctl.fid);
		am_scan_free_filter(scanner, &scanner->dtvctl.catctl.fid);
		am_scan_free_filter(scanner, &scanner->dtvctl.sdtctl.fid);
		am_scan_free_filter(scanner, &scanner->dtvctl.batctl.fid);
		am_scan_free_filter(scanner, &scanner->dtvctl.nitctl.fid);
		am_scan_free_filter(scanner, &scanner->dtvctl.mgtctl.fid);
		am_scan_free_filter(scanner, &scanner->dtvctl.vctctl.fid);

		if (IS_DVBT2()){
			am_scan_dvbt2_start_next_data_plp(scanner);
			TIMEOUT_CHECK(patctl);
		}
		else
			am_scan_start_next_ts(scanner);
	}
}

/**\brief SCAN线程*/
static void *am_scan_thread(void *para)
{
	AM_SCAN_Scanner_t *scanner = (AM_SCAN_Scanner_t*)para;
	AM_SCAN_TS_t *ts, *tnext;
	struct timespec to;
	int ret, evt_flag, i;
	AM_Bool_t go = AM_TRUE;

	pthread_mutex_lock(&scanner->lock);

	while (go)
	{
		/*检查超时，并计算下一个超时时间*/
		am_scan_get_wait_timespec(scanner, &to);

		ret = 0;
		/*等待事件*/
		if(scanner->evt_flag == 0)
		{
			if (to.tv_sec == 0 && to.tv_nsec == 0)
				ret = pthread_cond_wait(&scanner->cond, &scanner->lock);
			else
				ret = pthread_cond_timedwait(&scanner->cond, &scanner->lock, &to);
		}

		if (ret != ETIMEDOUT)
		{
handle_events:
			evt_flag = scanner->evt_flag;
			AM_DEBUG(2, "Evt flag 0x%x", scanner->evt_flag);

			/*开始搜索事件*/
			if ((evt_flag&AM_SCAN_EVT_START) && (scanner->dtvctl.hsi == 0))
			{
				SET_PROGRESS_EVT(AM_SCAN_PROGRESS_SCAN_BEGIN, (void*)scanner);
				am_scan_start(scanner);
			}

			/*前端事件*/
			if (evt_flag & AM_SCAN_EVT_FEND)
				am_scan_solve_fend_evt(scanner);

			/*PAT表收齐事件*/
			if (evt_flag & AM_SCAN_EVT_PAT_DONE)
				scanner->dtvctl.patctl.done(scanner);
			/*PMT表收齐事件*/
			if (evt_flag & AM_SCAN_EVT_PMT_DONE)
				scanner->dtvctl.pmtctl[0].done(scanner);
			/*CAT表收齐事件*/
			if (evt_flag & AM_SCAN_EVT_CAT_DONE)
				scanner->dtvctl.catctl.done(scanner);
			/*SDT表收齐事件*/
			if (evt_flag & AM_SCAN_EVT_SDT_DONE)
				scanner->dtvctl.sdtctl.done(scanner);
			/*NIT表收齐事件*/
			if (evt_flag & AM_SCAN_EVT_NIT_DONE)
				scanner->dtvctl.nitctl.done(scanner);
			/*BAT表收齐事件*/
			if (evt_flag & AM_SCAN_EVT_BAT_DONE)
				scanner->dtvctl.batctl.done(scanner);
			/*MGT表收齐事件*/
			if (evt_flag & AM_SCAN_EVT_MGT_DONE)
				scanner->dtvctl.mgtctl.done(scanner);
			/*VCT表收齐事件*/
			if (evt_flag & AM_SCAN_EVT_VCT_DONE)
				scanner->dtvctl.vctctl.done(scanner);
			/*完成一次卫星盲扫*/
			if (evt_flag & AM_SCAN_EVT_BLIND_SCAN_DONE)
				am_scan_solve_blind_scan_done_evt(scanner);

			/*退出事件*/
			if (evt_flag & AM_SCAN_EVT_QUIT)
			{
				go = AM_FALSE;
				continue;
			}

			/*在调用am_scan_free_filter时可能会产生新事件*/
			scanner->evt_flag &= ~evt_flag;
			if (scanner->evt_flag)
			{
				goto handle_events;
			}
		}
	}

	/* need clear dtv source anyway ? */
	if (dtv_start_para.clear_source &&
		(GET_MODE(dtv_start_para.mode) != AM_SCAN_DTVMODE_MANUAL &&
		GET_MODE(dtv_start_para.mode) != AM_SCAN_DTVMODE_NONE))
	{
		sqlite3 *hdb;

		AM_DB_HANDLE_PREPARE(hdb);
		AM_DEBUG(1, "Clear DTV source %d ...", dtv_start_para.source);
		am_scan_clear_source(hdb, dtv_start_para.source);
	}

	//check if user pause
	am_scan_check_need_pause(scanner, AM_SCAN_STATUS_PAUSED_USER);

	/* need store ? */
	if (scanner->result.tses/* && scanner->stage == AM_SCAN_STAGE_DONE*/ && scanner->store)
	{
		AM_DEBUG(1, "Call store proc");

		//int nit_version = -1;
		//if(scanner->result.nits!=NULL){
		//	AM_DEBUG(1, "Call store proc-----------%d",scanner->result.nits->i_version);
		//	nit_version = scanner->result.nits->i_version;
		//}
		//else
		//	AM_DEBUG(1, "scanner->result.nits==NULL");

		SET_PROGRESS_EVT(AM_SCAN_PROGRESS_STORE_BEGIN, NULL);
		if (scanner->store_cb) {
			if (scanner->start_para.dtv_para.mode & (AM_SCAN_DTVMODE_SCRAMB_TSHEAD|AM_SCAN_DTVMODE_CHECKDATA)) {
				AM_DEBUG(1, "AM_Check_Scramb_Stop --1");
				AM_Check_Scramb_Stop();
			}
			scanner->store_cb(&scanner->result);
		}
		//if(nit_version!=-1)
		//	SET_PROGRESS_EVT(AM_SCAN_PROGRESS_STORE_END, nit_version);

		SET_PROGRESS_EVT(AM_SCAN_PROGRESS_STORE_END, 100);
	}

	am_scan_stop_atv(scanner);
	am_scan_stop_dtv(scanner);

	if (scanner->start_para.mode == AM_SCAN_MODE_ADTV)
	{
		/* close atv devices */
	}

	//check if user pause
	am_scan_check_need_pause(scanner, AM_SCAN_STATUS_PAUSED_USER);

	SET_PROGRESS_EVT(AM_SCAN_PROGRESS_SCAN_EXIT, 100);

	pthread_mutex_unlock(&scanner->lock);

	/*反注册前端事件*/
	AM_EVT_Unsubscribe(scanner->start_para.fend_dev_id, AM_FEND_EVT_STATUS_CHANGED, am_scan_fend_callback, (void*)scanner);
	AM_EVT_Unsubscribe(scanner->start_para.vlfend_dev_id, AM_VLFEND_EVT_STATUS_CHANGED, atv_scan_fend_callback, (void*)scanner);

	pthread_mutex_destroy(&scanner->lock);
	pthread_cond_destroy(&scanner->cond);
	pthread_mutex_destroy(&scanner->lock_pause);
	pthread_cond_destroy(&scanner->cond_pause);
	if (atv_start_para.fe_paras != NULL)
		free(atv_start_para.fe_paras);
	free(scanner);

	return NULL;
}


/**\brief 拷贝数字前端参数*/
static void am_scan_copy_dtv_feparas(AM_SCAN_DTVCreatePara_t *para, AM_Bool_t use_default, AM_SCAN_FrontEndPara_t *fe_start)
{
	int i;
	AM_SCAN_FrontEndPara_t *start = fe_start;

	if (((GET_MODE(para->mode) == AM_SCAN_DTVMODE_ALLBAND)
		||(GET_MODE(para->mode) == AM_SCAN_DTVMODE_AUTO))
		&& use_default )
	{
		struct dvb_frontend_parameters tpara;

		/* User not specify the fe_paras, using default */
		if (para->source == FE_QAM)
		{
			tpara.u.qam.modulation = DEFAULT_DVBC_MODULATION;
			tpara.u.qam.symbol_rate = DEFAULT_DVBC_SYMBOLRATE;
			for (i=0; i<para->fe_cnt; i++,fe_start++)
			{
				tpara.frequency = dvbc_std_freqs[i]*1000;
				*dvb_fend_para(fe_start->fe_para) = tpara;
				fe_start->flag = AM_SCAN_FE_FL_DTV;
				fe_start->skip = 0;
			}
		}
		else if(para->source==FE_OFDM)
		{
			tpara.u.ofdm.bandwidth = DEFAULT_DVBT_BW;
			for (i=0; i<para->fe_cnt; i++,fe_start++)
			{
				tpara.frequency = (DEFAULT_DVBT_FREQ_START+(8000*i))*1000;
				*dvb_fend_para(fe_start->fe_para) = tpara;
				fe_start->flag = AM_SCAN_FE_FL_DTV;
				fe_start->skip = 0;
			}
		}
	}
	else if (GET_MODE(para->mode) != AM_SCAN_DTVMODE_SAT_BLIND)
	{
		for (i=0; i<para->fe_cnt; i++,fe_start++)
		{
			fe_start->fe_para = para->fe_paras[i];
			fe_start->flag = AM_SCAN_FE_FL_DTV;
			fe_start->skip = 0;
		}
	}

	AM_DEBUG(1, "DTV fe_paras count %d", fe_start - start);
}

/**\brief 拷贝数字前端参数*/
static void am_scan_copy_atv_feparas(AM_SCAN_ATVCreatePara_t *para, AM_SCAN_FrontEndPara_t *fe_start)
{
	int i;

	for (i=0; i<para->fe_cnt; i++,fe_start++)
	{
		fe_start->fe_para = para->fe_paras[i];
		AM_DEBUG(1, "ATV fe_para mode %d, freq %u", fe_start->fe_para.m_type, dvb_fend_para(fe_start->fe_para)->frequency);
		if (para->mode != AM_SCAN_ATVMODE_FREQ)
		{
			dvb_fend_para(fe_start->fe_para)->u.analog.std = para->default_std;
			dvb_fend_para(fe_start->fe_para)->u.analog.audmode = para->default_std & 0x00FFFFFF;
		}

		if (para->mode == AM_SCAN_ATVMODE_MANUAL)
		{
			dvb_fend_para(fe_start->fe_para)->u.analog.flag  |= ANALOG_FLAG_MANUL_SCAN;
		}
		else
		{
			dvb_fend_para(fe_start->fe_para)->u.analog.flag |= ANALOG_FLAG_ENABLE_AFC;
		}
		if (para->afc_range <= 0)
			para->afc_range = ATV_2MHZ;
		dvb_fend_para(fe_start->fe_para)->u.analog.afc_range = para->afc_range;
		fe_start->flag = AM_SCAN_FE_FL_ATV;
		fe_start->skip = 0;
	}
	AM_DEBUG(1, "ATV fe_paras count %d", para->fe_cnt);
}


/**\brief 拷贝前端参数*/
static void am_scan_copy_adtv_feparas(int fe_cnt, AM_FENDCTRL_DVBFrontendParameters_t * fe_paras, AM_SCAN_FrontEndPara_t *fe_start)
{
	int i;
	AM_SCAN_FrontEndPara_t *start = fe_start;

	for (i=0; i<fe_cnt; i++,fe_start++)
	{
		fe_start->fe_para = fe_paras[i];
		fe_start->flag = AM_SCAN_FE_FL_ATV | AM_SCAN_FE_FL_DTV;
		fe_start->skip = 0;
	}

	AM_DEBUG(1, "ADTV fe_paras count %d", fe_start - start);
}

static void am_scan_check_need_pause(AM_SCAN_Scanner_t *scanner, int pause_flag)
{
	if (!scanner->request_destory)
	{
		pthread_mutex_lock(&scanner->lock_pause);
		while (scanner->status & pause_flag)
			pthread_cond_wait(&scanner->cond_pause, &scanner->lock_pause);
		pthread_mutex_unlock(&scanner->lock_pause);
	}
}

/****************************************************************************
 * API functions
 ***************************************************************************/

 /**\brief 创建节目搜索
 * \param [in] para 创建参数
 * \param [out] handle 返回SCAN句柄
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_scan.h)
 */
AM_ErrorCode_t AM_SCAN_Create(AM_SCAN_CreatePara_t *para, AM_SCAN_Handle_t *handle)
{
	AM_SCAN_Scanner_t *scanner;
	int rc;
	pthread_mutexattr_t mta;
	int use_default=0;
	int i;

	if (! para || ! handle)
		return AM_SCAN_ERR_INVALID_PARAM;
	*handle = 0;
	if (para->atv_para.mode < AM_SCAN_ATVMODE_NONE)
	{
		if (para->atv_para.fe_cnt < 0 || !para->atv_para.fe_paras)
			return AM_SCAN_ERR_INVALID_PARAM;
		/* need to scan ATV */

		if (para->atv_para.mode != AM_SCAN_ATVMODE_FREQ && para->atv_para.fe_cnt < 3)
		{
			AM_DEBUG(1, "Invalid ATV fe count, must >= 3 to specify the min,max and start freq");
			return AM_SCAN_ERR_INVALID_PARAM;
		}
	}
	else
	{
		para->atv_para.fe_cnt = 0;
	}

	if ((GET_MODE(para->dtv_para.mode) < AM_SCAN_DTVMODE_NONE))
	{
		/* need to scan DTV */
		if (para->dtv_para.fe_cnt < 0 ||
			para->dtv_para.standard >= AM_SCAN_DTV_STD_MAX ||
			((GET_MODE(para->dtv_para.mode) == AM_SCAN_DTVMODE_MANUAL) &&
			(para->dtv_para.fe_cnt == 0 || !para->dtv_para.fe_paras)) ||
			(para->dtv_para.fe_cnt > 0 && !para->dtv_para.fe_paras))
			return AM_SCAN_ERR_INVALID_PARAM;
		if (GET_MODE(para->dtv_para.mode) == AM_SCAN_DTVMODE_MANUAL)
		{
			//para->dtv_para.fe_cnt = 1;
		}
		else if (((GET_MODE(para->dtv_para.mode) ==  AM_SCAN_DTVMODE_ALLBAND) ||
				(GET_MODE(para->dtv_para.mode) ==  AM_SCAN_DTVMODE_AUTO)) &&
				para->dtv_para.fe_cnt < 0)
		{
			use_default = 1;
			para->dtv_para.fe_cnt = (para->dtv_para.source==FE_QAM) ? AM_ARRAY_SIZE(dvbc_std_freqs)
				: ((para->dtv_para.source==FE_OFDM) ? (DEFAULT_DVBT_FREQ_STOP-DEFAULT_DVBT_FREQ_START)/8000+1
				: 0/* Otherwise, user must specify the fe_paras */);
		}
	}
	else
	{
		para->dtv_para.fe_cnt = 0;
	}

	/*Create a scanner*/
	scanner = (AM_SCAN_Scanner_t*)malloc(sizeof(AM_SCAN_Scanner_t));
	if (scanner == NULL)
	{
		AM_DEBUG(1, "Cannot create scan, no enough memory");
		return AM_SCAN_ERR_NO_MEM;
	}
	/*数据初始化*/
	memset(scanner, 0, sizeof(AM_SCAN_Scanner_t));
	scanner->status = AM_SCAN_STATUS_RUNNING;
	AM_DEBUG(1, "am-scan get check_scramble_mode [%d]", para->dtv_para.mode & AM_SCAN_DTVMODE_SCRAMB_TSHEAD);
	/*配置起始频点*/
	if (para->mode == AM_SCAN_MODE_ADTV)
	{
		scanner->start_freqs_cnt = (para->atv_para.fe_cnt > 0) ? para->atv_para.fe_cnt : para->dtv_para.fe_cnt;
	}
	else
	{
		if (GET_MODE(para->dtv_para.mode) == AM_SCAN_DTVMODE_SAT_BLIND)
			scanner->start_freqs_cnt = AM_SCAN_MAX_BS_TP_CNT;
		else
			scanner->start_freqs_cnt = para->dtv_para.fe_cnt;
		/* atv fe + dtv fe */
		scanner->start_freqs_cnt += para->atv_para.fe_cnt;
	}

	if (scanner->start_freqs_cnt > 0)
	{
		scanner->start_freqs = (AM_SCAN_FrontEndPara_t*)malloc(sizeof(AM_SCAN_FrontEndPara_t)*scanner->start_freqs_cnt);
		if (!scanner->start_freqs)
		{
			AM_DEBUG(1, "Cannot create scan, no enough memory");
			free(scanner);
			return AM_SCAN_ERR_NO_MEM;
		}
		memset(scanner->start_freqs, 0, sizeof(AM_SCAN_FrontEndPara_t) * scanner->start_freqs_cnt);
	}
	else
	{
		AM_DEBUG(1, "Donot sepcify any atv/dtv fe_paras");
		free(scanner);
		return AM_SCAN_ERR_INVALID_PARAM;
	}

	if (para->mode == AM_SCAN_MODE_ADTV)
	{
		AM_FENDCTRL_DVBFrontendParameters_t *fep;

		fep = (scanner->start_freqs_cnt == para->atv_para.fe_cnt) ? para->atv_para.fe_paras : para->dtv_para.fe_paras;
		am_scan_copy_adtv_feparas(scanner->start_freqs_cnt, fep, scanner->start_freqs);
		scanner->atvctl.start_idx = 0;
		scanner->dtvctl.start_idx = 0;
	}
	else if (para->mode == AM_SCAN_MODE_ATV_DTV)
	{
		am_scan_copy_atv_feparas(&para->atv_para, scanner->start_freqs);
		am_scan_copy_dtv_feparas(&para->dtv_para, use_default, scanner->start_freqs + para->atv_para.fe_cnt);
		scanner->atvctl.start_idx = 0;
		scanner->dtvctl.start_idx = para->atv_para.fe_cnt;
	}
	else
	{
		am_scan_copy_dtv_feparas(&para->dtv_para, use_default, scanner->start_freqs);
		am_scan_copy_atv_feparas(&para->atv_para, scanner->start_freqs + para->dtv_para.fe_cnt);
		scanner->atvctl.start_idx = para->dtv_para.fe_cnt;
		scanner->dtvctl.start_idx = 0;
	}

	AM_DEBUG(1, "Total fe_paras count %d", scanner->start_freqs_cnt);
	AM_DEBUG(1, "dtv para: resort_all %d, clear_source %d, mixtvradio %d",
		para->dtv_para.resort_all,
		para->dtv_para.clear_source,
		para->dtv_para.mix_tv_radio);

	scanner->start_para = *para;
	if (para->mode == AM_SCAN_MODE_ADTV)
	{
		atv_start_para.mode = AM_SCAN_ATVMODE_FREQ;
		dtv_start_para.mode = AM_SCAN_DTVMODE_ALLBAND;
	}
	dtv_start_para.fe_paras = NULL;
	if (para->atv_para.fe_cnt >= 3)
	{
		/* To store min, max, start freq */
		atv_start_para.fe_paras = (AM_FENDCTRL_DVBFrontendParameters_t*)malloc(sizeof(AM_FENDCTRL_DVBFrontendParameters_t)*3);
		memcpy(atv_start_para.fe_paras, para->atv_para.fe_paras, sizeof(AM_FENDCTRL_DVBFrontendParameters_t)*3);
	}
	else
	{
		atv_start_para.fe_paras = NULL;
	}
	if (atv_start_para.afc_unlocked_step <= 0)
		atv_start_para.afc_unlocked_step = DEFAULT_AFC_UNLOCK_STEP;
	if (atv_start_para.cvbs_unlocked_step <= 0)
		atv_start_para.cvbs_unlocked_step = DEFAULT_CVBS_UNLOCK_STEP;
	if (atv_start_para.cvbs_locked_step <= 0)
		atv_start_para.cvbs_locked_step = DEFAULT_CVBS_LOCK_STEP;
	scanner->result.start_para = &scanner->start_para;
	scanner->result.reserved = (void*)scanner;
    scanner->atvctl.am_scan_atv_cvbs_lock =  para->atv_para.am_scan_atv_cvbs_lock;

	if (! para->store_cb) {
		scanner->store_cb = am_scan_default_store;
		AM_DEBUG(1, "Scan using default_store_proc");
	}
	else
		scanner->store_cb = para->store_cb;

	AM_DEBUG(1, "atv_para.storeMode == %d", para->atv_para.storeMode);
	if (para->atv_para.storeMode == 1)
		scanner->store_cb = am_scan_atv_store;

	AM_DEBUG(1, "proc_mode = %d", para->proc_mode);
	scanner->proc_mode = para->proc_mode;

	pthread_mutexattr_init(&mta);
	pthread_mutexattr_settype(&mta, PTHREAD_MUTEX_RECURSIVE_NP);
	pthread_mutex_init(&scanner->lock, &mta);
	pthread_cond_init(&scanner->cond, NULL);
	pthread_mutex_init(&scanner->lock_pause, &mta);
	pthread_cond_init(&scanner->cond_pause, NULL);
	pthread_mutexattr_destroy(&mta);

	rc = pthread_create(&scanner->thread, NULL, am_scan_thread, (void*)scanner);
	if(rc)
	{
		AM_DEBUG(1, "%s", strerror(rc));
		pthread_mutex_destroy(&scanner->lock);
		pthread_cond_destroy(&scanner->cond);
		pthread_mutex_destroy(&scanner->lock_pause);
		pthread_cond_destroy(&scanner->cond_pause);
		free(scanner->start_freqs);
		free(scanner);
		return AM_SCAN_ERR_CANNOT_CREATE_THREAD;
	}

	*handle = scanner;
	return AM_SUCCESS;
}

/**\brief 启动节目搜索
 * \param handle Scan句柄
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_scan.h)
 */
AM_ErrorCode_t AM_SCAN_Start(AM_SCAN_Handle_t handle)
{
	AM_SCAN_Scanner_t *scanner = (AM_SCAN_Scanner_t*)handle;

	if (scanner)
	{
		pthread_mutex_lock(&scanner->lock);
		/*启动搜索*/
		scanner->evt_flag |= AM_SCAN_EVT_START;
		pthread_cond_signal(&scanner->cond);
		pthread_mutex_unlock(&scanner->lock);
	}

	return AM_SUCCESS;
}

/**\brief 销毀节目搜索
 * \param handle Scan句柄
 * \param store 是否存储
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_scan.h)
 */
AM_ErrorCode_t AM_SCAN_Destroy(AM_SCAN_Handle_t handle, AM_Bool_t store)
{
	AM_SCAN_Scanner_t *scanner = (AM_SCAN_Scanner_t*)handle;

	if (scanner)
	{
		pthread_t t;

		AM_DEBUG(1, "scan destroy");
		scanner->request_destory = 1;

		pthread_mutex_lock(&scanner->lock_pause);
		scanner->status = AM_SCAN_STATUS_RUNNING;
		pthread_cond_signal(&scanner->cond_pause);
		pthread_mutex_unlock(&scanner->lock_pause);

		pthread_mutex_lock(&scanner->lock);
		/*等待搜索线程退出*/
		scanner->evt_flag |= AM_SCAN_EVT_QUIT;
		scanner->store = store;
		t = scanner->thread;
		pthread_cond_signal(&scanner->cond);
		pthread_mutex_unlock(&scanner->lock);
		if (t != pthread_self()) {
			AM_DEBUG(1, "waiting scan thread.");
			pthread_join(t, NULL);
		}
		AM_DEBUG(1, "scan destroy done.");
	}

	return AM_SUCCESS;
}

/**\brief 设置用户数据
 * \param handle Scan句柄
 * \param [in] user_data 用户数据
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_scan.h)
 */
AM_ErrorCode_t AM_SCAN_SetUserData(AM_SCAN_Handle_t handle, void *user_data)
{
	AM_SCAN_Scanner_t *scanner = (AM_SCAN_Scanner_t*)handle;

	if (scanner)
	{
		pthread_mutex_lock(&scanner->lock);
		scanner->user_data = user_data;
		pthread_mutex_unlock(&scanner->lock);
	}

	return AM_SUCCESS;
}

/**\brief 取得用户数据
 * \param handle Scan句柄
 * \param [in] user_data 用户数据
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_scan.h)
 */
AM_ErrorCode_t AM_SCAN_GetUserData(AM_SCAN_Handle_t handle, void **user_data)
{
	AM_SCAN_Scanner_t *scanner = (AM_SCAN_Scanner_t*)handle;

	assert(user_data);

	if (scanner)
	{
		pthread_mutex_lock(&scanner->lock);
		*user_data = scanner->user_data;
		pthread_mutex_unlock(&scanner->lock);
	}

	return AM_SUCCESS;
}

AM_ErrorCode_t AM_SCAN_GetStatus(AM_SCAN_Handle_t handle, int *status)
{
	AM_SCAN_Scanner_t *scanner = (AM_SCAN_Scanner_t*)handle;

	if (!status)
		return AM_SCAN_ERR_INVALID_PARAM;

	if (scanner)
	{
		pthread_mutex_lock(&scanner->lock_pause);
		*status = scanner->status;
		pthread_mutex_unlock(&scanner->lock_pause);
	}
	return AM_SUCCESS;
}

AM_ErrorCode_t AM_SCAN_Pause(AM_SCAN_Handle_t handle)
{
	AM_SCAN_Scanner_t *scanner = (AM_SCAN_Scanner_t*)handle;

	AM_DEBUG(1, "AM_SCAN_Pause");

	if (scanner)
	{
		pthread_mutex_lock(&scanner->lock_pause);
		scanner->status |= AM_SCAN_STATUS_PAUSED_USER;
		pthread_mutex_unlock(&scanner->lock_pause);
	}

	return AM_SUCCESS;
}

AM_ErrorCode_t AM_SCAN_Resume(AM_SCAN_Handle_t handle)
{
	AM_SCAN_Scanner_t *scanner = (AM_SCAN_Scanner_t*)handle;

	AM_DEBUG(1, "AM_SCAN_Resume");

	if (scanner)
	{
		int status;
		pthread_mutex_lock(&scanner->lock_pause);
		if (scanner->status & AM_SCAN_STATUS_PAUSED)
			scanner->status -= AM_SCAN_STATUS_PAUSED;
		else if (scanner->status & AM_SCAN_STATUS_PAUSED_USER)
			scanner->status -= AM_SCAN_STATUS_PAUSED_USER;
		status = scanner->status;
		pthread_cond_signal(&scanner->cond_pause);
		pthread_mutex_unlock(&scanner->lock_pause);

		AM_DEBUG(1, "scanner status: %d", status);
	}

	return AM_SUCCESS;
}

AM_ErrorCode_t AM_SCAN_SetHelper(AM_SCAN_Handle_t handle, AM_SCAN_Helper_t *helper)
{
	AM_SCAN_Scanner_t *scanner = (AM_SCAN_Scanner_t*)handle;

	AM_DEBUG(1, "AM_SCAN_SetHelper [id=%d]", (helper)? helper->id : -1);

	if (scanner)
	{
		if (helper && (helper->id >= 0) && (helper->id < AM_SCAN_HELPER_ID_MAX))
			scanner->helper[helper->id] = *helper;
	}

	return AM_SUCCESS;
}


