/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file am_epg_internal.h
 * \brief EPG监控模块内部头文件
 *
 * \author Xia Lei Peng <leipeng.xia@amlogic.com>
 * \date 2010-11-04: create the document
 ***************************************************************************/

#ifndef _AM_EPG_INTERNAL_H
#define _AM_EPG_INTERNAL_H

#include <pthread.h>

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

/****************************************************************************
 * Type definitions
 ***************************************************************************/

/*EPG 内部事件类型*/
enum
{
	AM_EPG_EVT_FEND 		= 0x01, /**< 前端事件*/
	AM_EPG_EVT_PAT_DONE 	= 0x02, /**< PAT表已经接收完毕*/
	AM_EPG_EVT_PMT_DONE 	= 0x04, /**< PMT表已经接收完毕*/
	AM_EPG_EVT_CAT_DONE 	= 0x08, /**< CAT表已经接收完毕*/
	AM_EPG_EVT_SDT_DONE 	= 0x10, /**< SDT表已经接收完毕*/
	AM_EPG_EVT_NIT_DONE 	= 0x20, /**< NIT表已经接收完毕*/
	AM_EPG_EVT_TDT_DONE 	= 0x40, /**< TDT表已经接收完毕*/
	AM_EPG_EVT_EIT4E_DONE 	= 0x80, /**< EIT pf actual表已经接收完毕*/
	AM_EPG_EVT_EIT4F_DONE 	= 0x100, /**< EIT pf other表已经接收完毕*/
	AM_EPG_EVT_EIT50_DONE 	= 0x200, /**< EIT sche actual(tableid=0x50)表已经接收完毕*/
	AM_EPG_EVT_EIT51_DONE 	= 0x400, /**< EIT sche actual(tableid=0x51)表已经接收完毕*/
	AM_EPG_EVT_EIT60_DONE 	= 0x800, /**< EIT sche other(tableid=0x60)表已经接收完毕*/
	AM_EPG_EVT_EIT61_DONE 	= 0x1000, /**< EIT sche other(tableid=0x61)表已经接收完毕*/
	AM_EPG_EVT_SET_MODE		= 0x2000, /**< 设置监控模式*/
	AM_EPG_EVT_SET_EITPF_CHECK_TIME = 0x4000, 	/**< 设置eit pf自动更新间隔*/
	AM_EPG_EVT_SET_EITSCHE_CHECK_TIME = 0x8000, /**< 设置自动更新间隔*/
	AM_EPG_EVT_QUIT			= 0x10000, /**< 退出搜索事件*/
	AM_EPG_EVT_SET_MON_SRV	= 0x20000, /**< 设置当前监控的service*/
	AM_EPG_EVT_STT_DONE		= 0x40000, /**< STT接收完毕*/
	AM_EPG_EVT_MGT_DONE		= 0x80000,	/**< MGT接收完毕*/
	AM_EPG_EVT_RRT_DONE		= 0x100000,	/**< RRT接收完毕*/
	AM_EPG_EVT_PSIP_EIT_DONE	= 0x200000, /**< ATSC 某个 EIT 接收完毕*/
	AM_EPG_EVT_VCT_DONE		= 0x400000,	/**< VCT接收完毕*/
	AM_EPG_EVT_PSIP_ETT_DONE	= 0x800000,/**< ATSC 某个 ETT 接收完毕*/
	AM_EPG_EVT_PSIP_CEA_DONE	= 0x1000000,/**< ATSC 某个 CEA 接收完毕*/
};

typedef struct AM_EPG_Monitor_s AM_EPG_Monitor_t ;

/**\brief 子表接收控制*/
typedef struct
{		
	uint16_t		ext;
	uint8_t			ver;
	uint8_t			sec;
	uint8_t			last;
	uint8_t			seg_last;	/**< segment_last_section_num 只对EIT有效*/	
	uint16_t        leng;       /*sec_length*/
	uint8_t			mask[32];
}AM_EPG_SubCtl_t;

/**\brief 表接收控制，有多个子表时使用*/
typedef struct
{
	int				fid;
	int				evt_flag;		/**< 事件标志*/
	uint16_t		pid;
	uint8_t 		tid;
	uint8_t			tid_mask;
	int				check_time;			/**< 检查更新时间,ms单位，用于EIT表定时刷新*/
	int				data_arrive_time;	/**< 数据到达时间，用于多子表时收齐判断*/
	int				repeat_distance;	/**< 多子表时允许的最小数据重复间隔，用于多子表时收齐判断*/
	char			tname[20];
	void 			(*done)(struct AM_EPG_Monitor_s *);
	void 			(*proc_sec)(struct AM_EPG_Monitor_s *, void *);
	
	uint16_t subs;				/**< 子表个数*/
	AM_EPG_SubCtl_t *subctl; 	/**< 子表控制数据*/

	int				last_repeat_time;
}AM_EPG_TableCtl_t;

/**\brief 监控数据*/
struct AM_EPG_Monitor_s
{
	AM_EPG_TableCtl_t 	patctl;
	AM_EPG_TableCtl_t 	pmtctl;
	AM_EPG_TableCtl_t 	catctl;
	AM_EPG_TableCtl_t 	nitctl;
	AM_EPG_TableCtl_t 	totctl;
	AM_EPG_TableCtl_t 	sdtctl;
	AM_EPG_TableCtl_t 	eit4ectl;		/**< 表接收控制数据*/
	AM_EPG_TableCtl_t 	eit4fctl;
	AM_EPG_TableCtl_t 	eit50ctl;
	AM_EPG_TableCtl_t 	eit51ctl;
	AM_EPG_TableCtl_t 	eit60ctl;
	AM_EPG_TableCtl_t 	eit61ctl;
	AM_EPG_TableCtl_t	sttctl;
	AM_EPG_TableCtl_t	mgtctl;
	AM_EPG_TableCtl_t	rrtctl;
	AM_EPG_TableCtl_t	vctctl;
	AM_EPG_TableCtl_t	ceactl;
	AM_EPG_TableCtl_t	psip_eitctl[128]; /**< ATSC PSIP supports up to 128 EITs*/
	AM_EPG_TableCtl_t   psip_ettctl[128]; /**< ATSC PSIP supports up to 128 ETTs*/

	int					src;			/**< 源标识*/
	int					fend_dev;		/**< FEND 设备号*/
	int					dmx_dev;		/**< DMX设备号*/
	int					evt_flag;		/**< 事件标志*/
	int 				mode;			/**< 监控模式*/
	pthread_mutex_t     lock;   		/**< 保护互斥体*/
	pthread_cond_t      cond;    		/**< 条件变量*/
	pthread_t          	thread;         /**< 状态监控线程*/
	AM_SI_Handle_t      hsi;		/**< SI解析句柄*/
	int					eitpf_check_time; 	/**< EIT PF自动检查更新间隔，ms*/
	int					eitsche_check_time; /**< EIT Schedule自动检查更新间隔，ms*/
	int					new_eit_check_time; /**< EIT数据更新检查时间*/
	int					sub_check_time;	/**< 预约播放检查时间*/
	int					psip_eit_count;	/**<ATSC EIT 最大个数, max 128*/
	int					parallel_psip_eit_count; /**<ATSC EIT 同时并行接收的个数*/
	uint8_t				psip_eit_done_flag[16];	/**< ATSC EIT 接收完成标志*/
	AM_Bool_t			eit_looped;
	AM_Bool_t			eit_has_data;		/**< 是否有需要通知更新的EIT数据*/
	int					psip_ett_count; /**<ATSC ETT 最大个数, max 128*/
	int					parallel_psip_ett_count; /**<ATSC ETT 同时并行接收的个数*/
	uint8_t				psip_ett_done_flag[16]; /**< ATSC ETT 接收完成标志*/

	dvbpsi_pat_t		*pats;
	dvbpsi_pmt_t		*pmts;
	dvbpsi_cat_t		*cats;
	dvbpsi_sdt_t		*sdts;
	dvbpsi_nit_t		*nits;
	dvbpsi_eit_t		*eits;
	dvbpsi_tot_t		*tots;
	dvbpsi_atsc_stt_t	*atsc_stts;
	dvbpsi_atsc_mgt_t	*atsc_mgts;
	dvbpsi_atsc_vct_t	*atsc_vcts;
	dvbpsi_atsc_eit_t	*atsc_eits;
	dvbpsi_atsc_ett_t	*atsc_etts;
	dvbpsi_atsc_cea_t	*atsc_ceas;
	rrt_section_info_t	*rrts;

	struct dvb_frontend_event 		fe_evt;			/**< 前端事件*/

	int				mon_service;	/**< 当前监控的db_srv_id*/
	void			*user_data;
	int				curr_ts;	/**< 当前监控的TS*/
	char			text_langs[128];

	AM_EPG_Events_UpdateCB_t evt_cb;
	int                      mon_service_sid;
	int                      curr_ts_tsid;
	AM_EPG_PMT_UpdateCB_t    pmt_cb;

	struct {
		AM_EPG_TAB_UpdateCB_t cb;
		void                  *user_data;
	}tab_cbs[AM_EPG_TAB_MAX];

#define TAB_CB(_mon, _type, _tab) do { \
	if ((_mon)->tab_cbs[_type].cb) \
		(_mon)->tab_cbs[_type].cb((_mon), _type, (void *)(_tab), (_mon)->tab_cbs[_type].user_data); \
	} while(0)
#define IS_CB_SET(_mon, _type) ((_mon)->tab_cbs[_type].cb)

	int                      disable_def_proc;
	int                      psip_eit_request;
	int                      psip_ett_request;

	int                      current_fid;
};


/**\brief 当前时间管理*/
typedef struct
{
	int tdt_utc_time;		/**< 从TDT中获取的当前UTC时间，秒为单位*/
	int tdt_sys_time;		/**< 获取到TDT表时的系统时间，毫秒为单位，用于时间的自动累加*/
	int offset;				/**< 时区偏移，秒为单位，可以为负数*/
}AM_EPG_Time_t;

typedef struct ExtendedEventItem_s
{
	int char_len;
	int descr_len;
	uint8_t item_descr[256]; 
	uint8_t item_char[1024]; /* buffered composed raw data */
}ExtendedEventItem;

typedef struct
{
	char lang[3];
	dvbpsi_extended_event_dr_t *descrs[16];
}ExtendedEventLanguage;

typedef struct
{
	char lang[3];
	dvbpsi_short_event_dr_t *descr;
}ShortEventLanguage;

/****************************************************************************
 * Function prototypes  
 ***************************************************************************/


#ifdef __cplusplus
}
#endif

#endif

