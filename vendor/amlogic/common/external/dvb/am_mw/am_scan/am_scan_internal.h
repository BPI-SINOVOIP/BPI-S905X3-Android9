/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file am_scan_internal.h
 * \brief 搜索模块内部数据
 *
 * \author Xia Lei Peng <leipeng.xia@amlogic.com>
 * \date 2010-10-27: create the document
 ***************************************************************************/

#ifndef _AM_SCAN_INTERNAL_H
#define _AM_SCAN_INTERNAL_H

#include <pthread.h>

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Macro definitions
 ***************************************************************************/
/*卫星盲扫最大缓冲TP个数*/
#define AM_SCAN_MAX_BS_TP_CNT 128
/*Max service name languages*/
#define AM_SCAN_MAX_SRV_NAME_LANG 4

static const   v4l2_std_id  V4L2_COLOR_STD_PAL  =   ((v4l2_std_id)0x04000000);
static const   v4l2_std_id  V4L2_COLOR_STD_NTSC = ((v4l2_std_id)0x08000000);
static const   v4l2_std_id  V4L2_COLOR_STD_SECAM =  ((v4l2_std_id)0x10000000);
//virtual
static const   v4l2_std_id  V4L2_COLOR_STD_AUTO  =    ((v4l2_std_id)0x02000000);

/****************************************************************************
 * Type definitions
 ***************************************************************************/

typedef struct AM_SCAN_Scanner_s AM_SCAN_Scanner_t ;

/*当前搜索各表接收状态*/
enum
{
	AM_SCAN_RECVING_COMPLETE 	= 0,
	AM_SCAN_RECVING_PAT 		= 0x01,
	AM_SCAN_RECVING_PMT 		= 0x02,
	AM_SCAN_RECVING_CAT 		= 0x04,
	AM_SCAN_RECVING_SDT 		= 0x08,
	AM_SCAN_RECVING_NIT			= 0x10,
	AM_SCAN_RECVING_BAT			= 0x20,
	AM_SCAN_RECVING_MGT			= 0x40,
	AM_SCAN_RECVING_VCT			= 0x80,
	AM_SCAN_RECVING_WAIT_FEND	= 0x100,
};

/*SCAN内部事件类型*/
enum
{
	AM_SCAN_EVT_FEND 		= 0x01, /**< 前端事件*/
	AM_SCAN_EVT_PAT_DONE 	= 0x02, /**< PAT表已经接收完毕*/
	AM_SCAN_EVT_PMT_DONE 	= 0x04, /**< PMT表已经接收完毕*/
	AM_SCAN_EVT_CAT_DONE 	= 0x08, /**< CAT表已经接收完毕*/
	AM_SCAN_EVT_SDT_DONE 	= 0x10, /**< SDT表已经接收完毕*/
	AM_SCAN_EVT_NIT_DONE 	= 0x20, /**< NIT表已经接收完毕*/
	AM_SCAN_EVT_BAT_DONE 	= 0x40, /**< BAT表已经接收完毕*/
	AM_SCAN_EVT_MGT_DONE 	= 0x80, /**< MGT表已经接收完毕*/
	AM_SCAN_EVT_VCT_DONE 	= 0x100, /**< VCT表已经接收完毕*/
	AM_SCAN_EVT_QUIT		= 0x200, /**< 退出搜索事件*/
	AM_SCAN_EVT_START		= 0x400,/**< 开始搜索事件*/
	AM_SCAN_EVT_BLIND_SCAN_DONE = 0x800,	/**< 当前卫星盲扫完毕*/
};

/*SCAN 所在阶段*/
enum
{
	AM_SCAN_STAGE_WAIT_START,
	AM_SCAN_STAGE_NIT,
	AM_SCAN_STAGE_TS,
	AM_SCAN_STAGE_DONE
};

/*卫星盲扫阶段*/
enum
{
	AM_SCAN_BS_STAGE_HL,
	AM_SCAN_BS_STAGE_VL,
	AM_SCAN_BS_STAGE_HH,
	AM_SCAN_BS_STAGE_VH
};

/*sqlite3 stmts for DVB*/
enum
{
	QUERY_NET,
	INSERT_NET,
	UPDATE_NET,
	QUERY_TS,
	INSERT_TS,
	UPDATE_TS,
	DELETE_TS_SRVS,
	QUERY_SRV,
	INSERT_SRV,
	UPDATE_SRV,
	QUERY_SRV_BY_TYPE,
	UPDATE_CHAN_NUM,
	DELETE_TS_EVTS,
	QUERY_TS_BY_FREQ_ORDER,
	DELETE_SRV_GRP,
	QUERY_SRV_TS_NET_ID,
	UPDATE_CHAN_SKIP,
	QUERY_MAX_CHAN_NUM_BY_TYPE,
	QUERY_MAX_CHAN_NUM,
	QUERY_MAX_MAJOR_CHAN_NUM,
	UPDATE_MAJOR_CHAN_NUM,
	QUERY_SRV_BY_CHAN_NUM,
	QUERY_SAT_PARA_BY_POS_NUM,
	QUERY_SAT_PARA_BY_LO_LA,
	INSERT_SAT_PARA,
	QUERY_SRV_BY_LCN,
	QUERY_MAX_LCN,
	UPDATE_LCN,
	UPDATE_DEFAULT_CHAN_NUM,
	QUERY_SRV_TYPE,
	QUERY_MAX_DEFAULT_CHAN_NUM_BY_TYPE,
	QUERY_MAX_DEFAULT_CHAN_NUM,
	QUERY_SRV_BY_SD_LCN,
	QUERY_SRV_BY_DEF_CHAN_NUM_ORDER,
	QUERY_SRV_BY_LCN_ORDER,
	UPDATE_ANALOG_TS,
	UPDATE_SAT_PARA,
	QUERY_SAT_TP_BY_POLAR,
	UPDATE_TS_FREQ_SYMB,
	QUERY_DVBS_SAT_BY_SAT_ORDER,
	QUERY_DVBS_TP_BY_FREQ_ORDER,
	QUERY_EXIST_SRV_BY_CHAN_ORDER,
	QUERY_NONEXIST_SRV_BY_SRV_ID_ORDER,
	DELETE_TS_EVTS_BOOKINGS,
	SELECT_DBTSID_BY_NUM_AND_SRC,
	UPDATE_TS_FREQ,
	MAX_STMT
};

enum
{
    ATV_10KHZ = 10000,
    ATV_50KHZ = 50000,
    ATV_250KHZ = 250000,
    ATV_500KHZ = 500000,
    ATV_750KHZ = 750000,
    ATV_1MHZ = 1000000,
    ATV_1_5MHZ = 1500000,
    ATV_1_75MHZ = 1750000,
    ATV_2MHZ = 2000000,
    ATV_2_5MHZ = 2500000,
    ATV_2_75MHZ = 2750000,
    ATV_3MHZ = 3000000,
    ATV_6MHZ = 6000000,
    ATV_1GHZ = 1000000000,
    ATV_FINE_TUNE_STEP_HZ = 50000,
};

enum
{
	DEFAULT_AFC_UNLOCK_STEP = ATV_1MHZ,
	DEFAULT_CVBS_UNLOCK_STEP = ATV_1_5MHZ,
	DEFAULT_CVBS_LOCK_STEP = ATV_6MHZ,
};

/**\brief 频点搜索标志*/
enum
{
	AM_SCAN_FE_FL_ATV = 1,	/**< 需进行ATV搜索*/
	AM_SCAN_FE_FL_DTV = 2,	/**< 需进行DTV搜索*/
};

/**\brief ATV频率范围检查类型*/
enum
{
	AM_SCAN_ATV_FREQ_RANGE_CHECK,
	AM_SCAN_ATV_FREQ_LOOP_CHECK,
	AM_SCAN_ATV_FREQ_INCREASE_CHECK,
	AM_SCAN_ATV_FREQ_DECREASE_CHECK,
};

/**\brief 子表接收控制*/
typedef struct
{
	uint16_t		ext;
	uint8_t			ver;
	uint8_t			sec;
	uint8_t			last;
	uint8_t			mask[32];
}AM_SCAN_SubCtl_t;

/**\brief 表接收控制，有多个子表时使用*/
typedef struct
{
	int				fid;
	int				recv_flag;
	int				evt_flag;
	struct timespec	end_time;
	int				timeout;
	int				data_arrive_time;	/**< 数据到达时间，用于多子表时收齐判断*/
	int				repeat_distance;/**< 多子表时允许的最小数据重复间隔，用于多子表时收齐判断*/
	uint16_t		pid;
	uint16_t		ext;
	uint8_t 		tid;
	char			tname[10];
	void 			(*done)(struct AM_SCAN_Scanner_s *);

	uint16_t subs;				/**< 子表个数*/
	AM_SCAN_SubCtl_t *subctl; 	/**< 子表控制数据*/
}AM_SCAN_TableCtl_t;

/**\brief 搜索频点数据*/
typedef struct
{
	int flag; /**< 搜索标志*/
	int skip; /**< 搜索终止标识  6表示跳过该频点*/
	AM_FENDCTRL_DVBFrontendParameters_t	fe_para; /**< 数字参数*/
}AM_SCAN_FrontEndPara_t;

/**\brief 卫星盲扫控制数据*/
typedef struct
{
	int								start_freq;	/**< 起始频率*/
	int								stop_freq;	/**< 结束频率*/
	int								stage;		/**< HL,HH,VL,VH*/
	AM_SCAN_DTVBlindScanProgress_t		progress;	/**< Blind Scan 进度*/
	int								get_tp_cnt; 	/**< 已搜索到的所有 TP 个数*/
	int								get_invalid_tp_cnt; 	/**< 已搜索到的所有无效 TP 个数*/
	int								searched_tp_cnt;	/**< 已搜索到的 TP 个数*/
	struct dvb_frontend_parameters	searched_tps[AM_SCAN_MAX_BS_TP_CNT];	/**< 已搜索到的TP*/
}AM_SCAN_BlindScanCtrl_t;


typedef struct
{
	int    srv_cnt;
	int    buf_size;
	int   *srv_ids;
	AM_SCAN_TS_t **tses;
}AM_SCAN_RecTab_t;

/**\brief service结构*/
typedef struct
{
	uint8_t srv_type, eit_sche, eit_pf, rs, free_ca, access_controlled, hidden, hide_guide, plp_id;
	int vid, aid1, aid2, srv_id, pmt_pid, pcr_pid;
	int vfmt, chan_num, afmt_tmp, vfmt_tmp, scrambled_flag, major_chan_num, minor_chan_num, source_id;
	int src, srv_dbid, satpara_dbid;
	char name[(AM_DB_MAX_SRV_NAME_LEN+4)*AM_SCAN_MAX_SRV_NAME_LANG + 1];
	char str_apids[256];
	char str_afmts[256];
	char str_alangs[256];
	char str_atypes[256];
	char *default_text_lang;
	char *text_langs;
	AM_SI_AudioInfo_t aud_info;
	AM_SI_SubtitleInfo_t sub_info;
	AM_SI_TeletextInfo_t ttx_info;
	AM_SI_IsdbsubtitleInfo_t isdb_info;
	int sdt_version;
	int analog_std;
}AM_SCAN_ServiceInfo_t;

/**\brief 搜索中间数据*/
struct AM_SCAN_Scanner_s
{
	AM_SCAN_CreatePara_t			start_para;		/**< AM_SCAN_Create()传入的参数*/
	int								evt_flag;		/**< 事件标志*/
	int								recv_status;	/**< 接收状态*/
	int								stage;			/**< 执行阶段*/
	pthread_mutex_t     			lock;   		/**< 保护互斥体*/
	pthread_cond_t      			cond;    		/**< 条件变量*/
	pthread_t          				thread;         /**< 状态监控线程*/

	int								curr_freq;		/**< 当前正在搜索的频点*/
	int								curr_plp;		/**< Current PLP index*/
	int								start_freqs_cnt;/**< 需要搜索的频点个数*/
	struct dvb_frontend_event		fe_evt;			/**< 前段事件*/
	AM_SCAN_FrontEndPara_t		 	*start_freqs;	/**< 需要搜索的频点列表*/
	AM_SCAN_TS_t					*curr_ts;		/**< 当前正在搜索的TS数据*/

	AM_SCAN_StoreCb 				store_cb;		/**< 存储回调*/
	AM_SCAN_Result_t				result;			/**< 搜索结果*/
	int								end_code;		/**< 搜索结束码*/
	void							*user_data;		/**< 用户数据*/
	AM_Bool_t						store;			/**< 是否存储*/
	bool 							atv_open;

	struct
	{
		AM_Bool_t						start;
		int								start_idx;		/**< 起始频点参数在start_freqs中的索引*/
		AM_SI_Handle_t                                  hsi;			/**< SI解析句柄*/
		AM_SCAN_TableCtl_t				patctl;			/**< PAT接收控制*/
		AM_SCAN_TableCtl_t				pmtctl[5];		/**< PMT接收控制*/
		AM_SCAN_TableCtl_t				catctl;			/**< CAT接收控制*/
		AM_SCAN_TableCtl_t				sdtctl;			/**< SDT接收控制*/
		AM_SCAN_TableCtl_t				nitctl;			/**< NIT接收控制*/
		AM_SCAN_TableCtl_t				batctl;			/**< BAT接收控制*/
		/*ATSC tables*/
		AM_SCAN_TableCtl_t				mgtctl;			/**< MGT接收控制*/
		AM_SCAN_TableCtl_t				vctctl;			/**< VCT接收控制*/
		dvbpsi_pat_t					*cur_pat;		/**< 当前搜索PMT时搜使用的PAT section */
		dvbpsi_pat_program_t			*cur_prog;		/**< 当前正在接收PMT的Program*/
		AM_SCAN_BlindScanCtrl_t			bs_ctl;			/**< 盲扫控制*/
	}dtvctl;		/**< DTV控制*/

	struct
	{
		AM_Bool_t start;
		AM_Bool_t (*am_scan_atv_cvbs_lock)(void*);/*同步delay函数*/
		int start_idx;		/**< 起始频点参数在start_freqs中的索引*/
		int range_check;
		int direction;	/**< -1/+1*/
		int step;		/**< step*/
		int min_freq;
		int max_freq;
		int start_freq;
		int afc_locked_freq;
	}atvctl;		/**< ATV控制*/

	pthread_mutex_t     			lock_pause;
	pthread_cond_t      			cond_pause;
	int                                     request_destory;

	int                                     status;
	int                                     proc_mode;

	AM_SCAN_Helper_t          helper[AM_SCAN_HELPER_ID_MAX];
};




/****************************************************************************
 * Function prototypes
 ***************************************************************************/


#ifdef __cplusplus
}
#endif

#endif

