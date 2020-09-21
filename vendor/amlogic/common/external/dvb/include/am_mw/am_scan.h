/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file am_scan.h
 * \brief Channel scanner module
 *
 * \author Xia Lei Peng <leipeng.xia@amlogic.com>
 * \date 2010-10-27: create the document
 ***************************************************************************/

#ifndef _AM_SCAN_H
#define _AM_SCAN_H

#include <am_types.h>
#include <am_mem.h>
#include <am_fend.h>
#include <am_fend_diseqc_cmd.h>
#include <am_fend_ctrl.h>
#include <am_si.h>
#include <am_db.h>

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

//virtual define
#define TUNER_COLOR_AUTO    ((tuner_std_id)0x02000000)


/****************************************************************************
 * Type definitions
 ***************************************************************************/

typedef void* AM_SCAN_Handle_t;

/**\brief Error code of the scan module*/
enum AM_SCAN_ErrorCode
{
	AM_SCAN_ERROR_BASE=AM_ERROR_BASE(AM_MOD_SCAN),
	AM_SCAN_ERR_INVALID_PARAM,			 /**< Invalid parameter*/
	AM_SCAN_ERR_INVALID_HANDLE,          /**< Invalid handle*/
	AM_SCAN_ERR_NOT_SUPPORTED,           /**< not surport action*/
	AM_SCAN_ERR_NO_MEM,                  /**< out of memory*/
	AM_SCAN_ERR_CANNOT_LOCK,             /**< cannot lock*/
	AM_SCAN_ERR_CANNOT_GET_NIT,			 /**< Receive NIT faile*/
	AM_SCAN_ERR_CANNOT_CREATE_THREAD,    /**< cannot create thread*/
	AM_SCAN_ERR_END
};

/**\brief event type of scan process,used in AM_SCAN_Progress_t*/
enum AM_SCAN_ProgressEvt
{
	AM_SCAN_PROGRESS_SCAN_BEGIN = 0,	/**< start scan，Parameter is the scan handle*/
	AM_SCAN_PROGRESS_SCAN_END = 1,		/**< scan end，Parameter is the scan result code*/
	AM_SCAN_PROGRESS_NIT_BEGIN = 2,		/**< start get nit table when scan mode is auto mode*/
	AM_SCAN_PROGRESS_NIT_END = 3,		/**< got nit table end when scan mode is auto mode*/
	AM_SCAN_PROGRESS_TS_BEGIN = 4,		/**< start scan a ts,Parameter is AM_SCAN_TSProgress_t*/
	AM_SCAN_PROGRESS_TS_END = 5,		/**< scan for a TS finished*/
	AM_SCAN_PROGRESS_PAT_DONE = 6,		/**< PAT table search is completed of the current TS，Parameter is dvbpsi_pat_t*/
	AM_SCAN_PROGRESS_PMT_DONE = 7,		/**< ALL PMT table search is completed of the current TS，Parameter is dvbpsi_pmt_t*/
	AM_SCAN_PROGRESS_CAT_DONE = 8,		/**< CAT table search is completed of the current TS，Parameter is dvbpsi_cat_t*/
	AM_SCAN_PROGRESS_SDT_DONE = 9,		/**< SDT table search is completed of the current TS，Parameter is dvbpsi_sdt_t*/
	AM_SCAN_PROGRESS_MGT_DONE = 10,		/**< MGT table search is completed of the current TS，Parameter is mgt_section_info_t*/
	AM_SCAN_PROGRESS_VCT_DONE = 11,		/**< VCT table search is completed of the current TS，Parameter is vct_section_info_t*/
	AM_SCAN_PROGRESS_STORE_BEGIN = 12,	/**< start store*/
	AM_SCAN_PROGRESS_STORE_END = 13,		/**< store finished*/
	AM_SCAN_PROGRESS_BLIND_SCAN = 14,	/**< Satellite blind search progress，Parameter is AM_SCAN_BlindScanProgress_t*/
	AM_SCAN_PROGRESS_ATV_TUNING = 15,	/**< ATV tuning a new frequency*/
	AM_SCAN_PROGRESS_NEW_PROGRAM = 16,	/**< Searched a new program which will be stored*/
	AM_SCAN_PROGRESS_DVBT2_PLP_BEGIN = 17,	/**< Start a DVB-T2 data PLP, parameter is AM_SCAN_PLPProgress_t*/
	AM_SCAN_PROGRESS_DVBT2_PLP_END = 18,	/**< DVB-T2 PLP search end, parameter is AM_SCAN_PLPProgress_t*/
	AM_SCAN_PROGRESS_SCAN_EXIT = 19,					/**< exit scan*/
	AM_SCAN_PROGRESS_NEW_PROGRAM_MORE = 20,	/**< new a program*/
};

/**\brief scan event type*/
enum AM_SCAN_EventType
{
	AM_SCAN_EVT_BASE=AM_EVT_TYPE_BASE(AM_MOD_SCAN),
	AM_SCAN_EVT_PROGRESS,	/**< scan process event，parameter is AM_SCAN_Progress_t*/
	AM_SCAN_EVT_SIGNAL,		/**< the signal information of the current scan frequency， parameter is AM_SCAN_SignalInfo_t*/
	AM_SCAN_EVT_END
};

/**\brief service type*/
enum AM_SCAN_ServiceType
{
	AM_SCAN_SRV_UNKNOWN	= 0,	/**< unkown type*/
	AM_SCAN_SRV_DTV		= 1,	/**< Digital TV type*/
	AM_SCAN_SRV_DRADIO	= 2,	/**< Digital broadcast type*/
	AM_SCAN_SRV_ATV		= 3,	/**< Analog TV type*/
};

/**\brief DTV standard */
typedef enum
{
	AM_SCAN_DTV_STD_DVB		= 0x00,	/**< DVB standard*/
	AM_SCAN_DTV_STD_ATSC	= 0x01,	/**< ATSC standard*/
	AM_SCAN_DTV_STD_ISDB	= 0x02,	/**< ISDB standard*/
	AM_SCAN_DTV_STD_MAX,
}AM_SCAN_DTVStandard_t;


/**\brief TV scan mode*/
enum AM_SCAN_Mode
{
	AM_SCAN_MODE_ATV_DTV,	/**< First search all ATV, then search all DTV*/
	AM_SCAN_MODE_DTV_ATV,	/**< First search all DTV, then search all ATV*/
	AM_SCAN_MODE_ADTV,		/**< A/DTV Use the same frequency table，search the A/DTV one by one In a frequency*/
};

/**\brief DTV scan mode*/
enum AM_SCAN_DTVMode
{
	AM_SCAN_DTVMODE_AUTO 			= 0x01,	/**< auto scan mode*/
	AM_SCAN_DTVMODE_MANUAL 			= 0x02,	/**< manual scan mode*/
	AM_SCAN_DTVMODE_ALLBAND 		= 0x03, /**< all band scan mode*/
	AM_SCAN_DTVMODE_SAT_BLIND		= 0x04,	/**< Satellite blind scan mode*/
	AM_SCAN_DTVMODE_NONE			= 0x07,	/**< none*/
	/* OR option(s)*/
	AM_SCAN_DTVMODE_SEARCHBAT		= 0x08, /**< Whether to search BAT table*/
	AM_SCAN_DTVMODE_SAT_UNICABLE	= 0x10,	/**< Satellite Unicable mode*/
	AM_SCAN_DTVMODE_FTA				= 0x20,	/**< Only scan free programs*/
	AM_SCAN_DTVMODE_NOTV			= 0x40, /**< Donot store tv programs*/
	AM_SCAN_DTVMODE_NORADIO			= 0x80, /**< Donot store radio programs*/
	AM_SCAN_DTVMODE_ISDBT_ONESEG	= 0x100, /**< Scan ISDBT oneseg in layer A*/
	AM_SCAN_DTVMODE_ISDBT_FULLSEG	= 0x200, /**< Scan ISDBT fullseg*/
	AM_SCAN_DTVMODE_SCRAMB_TSHEAD	= 0x400, /**< is check scramb by ts head*/
	AM_SCAN_DTVMODE_NOVCT			= 0x800, /**< Donot store in vct but not in pmt programs*/
	AM_SCAN_DTVMODE_NOVCTHIDE		= 0x1000, /**< Donot store in vct hide flag is set 1*/
	AM_SCAN_DTVMODE_CHECKDATA		= 0x2000, /**< Check AV data.*/
	AM_SCAN_DTVMODE_INVALIDPID		= 0x4000,  /**< SKIP no video and audio pid.*/
    AM_SCAN_DTVMODE_CHECK_AUDIODATA	= 0x8000  /**< Check Audio data, remove it that nodata.*/
};

/**\brief ATVscan mode*/
enum AM_SCAN_ATVMode
{
	AM_SCAN_ATVMODE_AUTO	= 0x01,	/**< auto scan mode*/
	AM_SCAN_ATVMODE_MANUAL	= 0x02,	/**< manual scan mode*/
	AM_SCAN_ATVMODE_FREQ	= 0x03,	/**< all band scan mode*/
	AM_SCAN_ATVMODE_NONE	= 0x07,	/**< none*/
};

/**\brief scan store mode */
typedef enum
{
	AM_SCAN_ATV_STOREMODE_DEFAULT   = 0x00, /**< atv store default mode, store all program*/
	AM_SCAN_ATV_STOREMODE_NOPAL     = 0x01,	/**< atv store NO pal programs*/
	AM_SCAN_ATV_STOREMODE_NONTSC	= 0x02,	/**< atv store NO ntsc programs*/
	AM_SCAN_ATV_STOREMODE_ALL		= 0x04,	/**< atv store all*/
	AM_SCAN_DTV_STOREMODE_NOSECAM	= 0x08,	/**< Dtv store NO SECCAM */
	AM_SCAN_DTV_STOREMODE_NOMODE1	= 0x10,	/**< Dtv store NO MODE 1*/
	AM_SCAN_DTV_STOREMODE_NOMODE2	= 0x20,	/**< Dtv store NO MODE 2*/
	AM_SCAN_DTV_STOREMODE_NOMODE3	= 0x40,	/**< Dtv store NO MODE 3*/
}AM_SCAN_StoreMode;


/**\brief code of scan result*/
enum AM_SCAN_ResultCode
{
	AM_SCAN_RESULT_OK,			/**< Normal end*/
	AM_SCAN_RESULT_UNLOCKED,	/**< No frequency locked*/
};
/**\brief status of scan */
enum AM_SCAN_Status
{
	AM_SCAN_STATUS_RUNNING,	/**< scan running*/
	AM_SCAN_STATUS_PAUSED,	/**< scan paused*/
	AM_SCAN_STATUS_PAUSED_USER	/**< scan paused by user*/
};
/**\brief mode of scan process*/
enum AM_SCAN_PROCMode
{
	AM_SCAN_PROCMODE_NORMAL                  = 0x00,  /**< normal mode*/
	AM_SCAN_PROCMODE_AUTOPAUSE_ON_ATV_FOUND  = 0x01,	/**< auto pause when found atv*/
	AM_SCAN_PROCMODE_AUTOPAUSE_ON_DTV_FOUND  = 0x02,	/**< auto pause when found dtv*/

	AM_SCAN_PROCMODE_STORE_CB_COMPLICATED = 0x10,	/**< call store_cb whenever scanner needs*/
};

/**\brief TS signal type*/
typedef enum
{
	AM_SCAN_TS_DIGITAL,	/**< digital signal type*/
	AM_SCAN_TS_ANALOG		/**< analog signal type*/
}AM_SCAN_TSType_t;

/**\brief DTV channel sorting method*/
typedef enum
{
	AM_SCAN_SORT_BY_FREQ_SRV_ID,	/**< Sort according to the frequency of size, with the same frequency by service_id sort*/
	AM_SCAN_SORT_BY_SCAN_ORDER,		/**< Sorted according to the order of scan*/
	AM_SCAN_SORT_BY_LCN,			/**< Sorted according to the LCN*/
	AM_SCAN_SORT_BY_HD_SD,		/**< Sorted according to the service Resolving power*/
}AM_SCAN_DTVSortMethod_t;

enum {
	AM_SCAN_HELPER_ID_FE_TYPE_CHANGE, /**<request to change fe type, fe_type_t will be used as para*/
	AM_SCAN_HELPER_ID_MAX,
};

/**\brief Frequency info of scan progress*/
typedef struct
{
	int								index;		/**< Current scan frequency index*/
	int								total;		/**< the count of frequency needed to search*/
	AM_FENDCTRL_DVBFrontendParameters_t	fend_para;	/**< Current scan frequency information*/
}AM_SCAN_TSProgress_t;

/**\brief DVB-T2 PLP progress data*/
typedef struct
{
	int				index;	/**< current searching plp index, start from 0*/
	int				total;	/**< total plps in current TS*/
	struct AM_SCAN_TS_s	*ts;	/**< current searching TS*/
}AM_SCAN_PLPProgress_t;

/**\brief Search progress info*/
typedef struct
{
	int		evt;	/**< event type，see AM_SCAN_ProgressEvt*/
	void	*data;  /**< event data，see AM_SCAN_ProgressEvt describe*/
}AM_SCAN_Progress_t;

/**\brief DTV Satellite blind info*/
typedef struct
{
	int	progress;	/**< Blind progress，0-100*/
	int	polar;		/**< The polarization mode of the current blind scan，see AM_FEND_Polarisation_t，-1 means unkown type*/
	int lo;			/**< Current vibration frequency of blind sweep，see AM_FEND_Localoscollatorfreq_t， -1 means unknown current local oscillator*/
	int freq;		/**< Current scan frequency point*/
	int new_tp_cnt;	/**< the new TP number to search, <=0 means no new search to the TP*/
	struct dvb_frontend_parameters	*new_tps;	/**<the new TP information*/
}AM_SCAN_DTVBlindScanProgress_t;

/**\brief New program progress data*/
typedef struct
{
	int service_id;      /**< service id*/
	int service_type;	 /**< See AM_SCAN_ServiceType*/
	AM_Bool_t scrambled; /**< Is scrambled*/
	char name[1024];     /**< Program name*/
}AM_SCAN_ProgramProgress_t;

/**\brief singnal and frequency info of current scan frequency*/
typedef struct
{
	AM_Bool_t locked;	/**< singnal lock*/
	int snr;					/**< current singnal snr*/
	int ber;					/**< current singnal ber*/
	int strength;			/**< current singnal strength*/
	int frequency;		/**< current frequency*/
}AM_SCAN_DTVSignalInfo_t;

/**\brief Satellite configuration parameters*/
typedef struct
{
	char									sat_name[64];	/**< The name of a satellite, used to uniquely identify a satellite.*/
	int										lnb_num;		/**< LNB number*/
	int										motor_num;		/**< Motor number*/
	AM_SEC_DVBSatelliteEquipmentControl_t	sec;			/**< Satellite configuration parameters*/
}AM_SCAN_DTVSatellitePara_t;

/**\brief single TS data of scan results*/
typedef struct AM_SCAN_TS_s
{
	AM_SCAN_TSType_t type;		/**< Identification digital/analog*/
	union
	{
		struct
		{
			int snr;		/**< SNR*/
			int ber;		/**< BER*/
			int strength;	/**< Strength*/
			AM_FENDCTRL_DVBFrontendParameters_t fend_para;	/**< Frequency information*/
			dvbpsi_nit_t *nits;		/**< the NIT table of scaned，each frequency point separately to save the NIT table, for LCN and other applications*/
			dvbpsi_pat_t *pats;		/**< the PAT table of scaned*/
			dvbpsi_cat_t *cats;		/**< the CAT table of scaned*/
			dvbpsi_pmt_t *pmts;		/**< the PMT table of scaned*/
			dvbpsi_sdt_t *sdts;		/**< the SDT table of scaned*/
			dvbpsi_atsc_mgt_t *mgts;		/**< the MGT table of scaned*/
			dvbpsi_atsc_vct_t *vcts;		/**< the VCT table of scaned*/
			int use_vct_tsid;
			int				dvbt2_data_plp_num;	/**< DVB-T2 DATA PLP count*/
			struct
			{
				uint8_t					id;			/**< the data plp id*/
				dvbpsi_pat_t				*pats;		/**< the pats in this data PLP*/
				dvbpsi_pmt_t				*pmts;		/**< the pmts in this data PLP*/
				dvbpsi_sdt_t 				*sdts;		/**< the sdt actuals in this data PLP*/
			}dvbt2_data_plps[256];
		}digital;

		struct
		{
			int freq;		/**< frequency*/
			int std;		/**< tuner std*/
			int audmode;	/**< audmode std*/
			int logicalChannelNum;	/**< logical channel number*/
		}analog;
	};

	int tp_index; /**< Position in the frequency table*/

	struct AM_SCAN_TS_s *p_next;	/**< Point to the next TS*/
}AM_SCAN_TS_t;
/**\brief ATV lock parameters*/
typedef struct
{
    void* pData;
    v4l2_std_id pOutColorSTD;
    int checkStable;
} AM_SCAN_ATV_LOCK_PARA_t;

/**\brief ATV scan parameter definition*/
typedef struct
{
	int mode;		/**< ATV scan mode,see AM_SCAN_ATVMode*/
	int channel_id;	/**< Used for manual scan*/
	int direction;	/**< set on Manual mode*/
	int afe_dev_id;			/**< AFE device number*/
	int default_std;		/**< tuner std*/
	int afc_range;			/**< AFC range in Hz*/
	int afc_unlocked_step;	/**< AFC unlocked step frequency in Hz*/
	int cvbs_unlocked_step;	/**< CVBS unlocked step frequency in Hz*/
	int cvbs_locked_step;	/**< CVBS locked step frequency in Hz*/
	int fe_cnt;		/**< Number of front end parameters*/
    AM_Bool_t (*am_scan_atv_cvbs_lock)(void*);/**<synchronization delay function*/
	AM_FENDCTRL_DVBFrontendParameters_t *fe_paras;	/**< Front end parameter list，The first three parameters were min_freq、 max_freq、 start_freq when mode not eq AM_SCAN_ATVMODE_FREQ，*/
	int channel_num;	/**< channel number*/
	int storeMode;		/**0:insert 1:update*/
}AM_SCAN_ATVCreatePara_t;

/**\brief DTV scan parameter*/
typedef struct
{
	int mode;						/**< DTV scan mode*/
	int dmx_dev_id;  				/**< demux device number*/
	int source;      				/**< Front end modulation mode*/
	AM_SCAN_DTVStandard_t standard;    /**< scan standard，DVB/ATSC*/
	AM_SCAN_DTVSatellitePara_t sat_para;	/**< Satellite parameter configuration,Only when the source bit is Satellite*/
	AM_SCAN_DTVSortMethod_t sort_method;	/**< Channel sorting method*/
	AM_Bool_t resort_all;					/**< Resort all of the data in the database service*/
	AM_Bool_t clear_source;		/**< clear the source anyway in auto mode*/
	AM_Bool_t mix_tv_radio;		/**< sort TV & radio together */
	int fe_cnt;								/**< Number of front end parameters*/
	AM_FENDCTRL_DVBFrontendParameters_t *fe_paras;/**< Front end parameter list,
																											auto scan: main frequency list
																											manual scan: single frequency
																											full scan: full band frequency list
																											*/
}AM_SCAN_DTVCreatePara_t;

typedef struct AM_SCAN_CreatePara_s AM_SCAN_CreatePara_t;

/**\brief data structure of Program search result*/
typedef struct
{
	struct AM_SCAN_CreatePara_s *start_para;/**< AM_SCAN_Create() input parameter*/
	dvbpsi_nit_t *nits;				/**< scaned NIT table*/
	dvbpsi_bat_t *bats;				/**< scaned BAT table*/
	vct_channel_info_t	*vcs;		/**<ATSC C virtual channels*/
	AM_SCAN_TS_t *tses;				/**< ALL TS list*/
	void *reserved;                 /**< reserved data*/
}AM_SCAN_Result_t;

/**\brief store callback
 * \param[in] result store scan result
 */
typedef void (*AM_SCAN_StoreCb) (AM_SCAN_Result_t *result);

/**\brief scan create parameters*/
struct AM_SCAN_CreatePara_s
{
	int fend_dev_id; 					/**< fend device number*/
	int vlfend_dev_id; 					/**< v4l2 fend device number*/
	int mode;							/**< TV scan mode，see AM_SCAN_Mode*/
	AM_SCAN_StoreCb store_cb;			/**< callback of scan finish*/
	sqlite3 *hdb;						/**< the handle of database*/
	AM_SCAN_ATVCreatePara_t atv_para;	/**< ATV scan parameters*/
	AM_SCAN_DTVCreatePara_t dtv_para;	/**< DTV scan parameters*/
	int proc_mode;	/**< see AM_SCAN_PROCMode*/
	int store_mode; /**< see AM_SCAN_StoreMode*/
};
/**\brief scan new program parameters*/
typedef struct AM_SCAN_NewProgram_Data_s {
	AM_SCAN_Result_t *result;	/**< scan result*/
	AM_SCAN_TS_t      *newts;	/**< new scan ts*/
}AM_SCAN_NewProgram_Data_t;

/**\brief helper for scan process*/
typedef struct AM_SCAN_Helper_s {
	int id;
	void *user;
	int (*cb)(int id, void *para, void *user);
} AM_SCAN_Helper_t;

/****************************************************************************
 * Function prototypes
 ***************************************************************************/

 /**\brief creat scan
 * \param [in] para scan creat parameters
 * \param [out] handle scan handle
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_SCAN_Create(AM_SCAN_CreatePara_t *para, AM_SCAN_Handle_t *handle);

/**\brief destroy scam by handle
 * \param [in] handle scan handle
 * \param [in] store store or not
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_SCAN_Destroy(AM_SCAN_Handle_t handle, AM_Bool_t store);

/**\brief start scan
 * \param [in] handle scan handle
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_SCAN_Start(AM_SCAN_Handle_t handle);

/**\brief set user data
 * \param [in] handle scan handle
 * \param [in] user_data user data
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_SCAN_SetUserData(AM_SCAN_Handle_t handle, void *user_data);

/**\brief get user data
 * \param [in] handle scan handle
 * \param [in] user_data user data
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_SCAN_GetUserData(AM_SCAN_Handle_t handle, void **user_data);
/**\brief get scan status
 * \param [in] handle scan handle
 * \param [out] user_data
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_SCAN_GetStatus(AM_SCAN_Handle_t handle, int *status);
/**\brief pause scan
 * \param [in] handle scan handle
 * \param [out] status
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_SCAN_Pause(AM_SCAN_Handle_t handle);
/**\brief resume scan
 * \param handle scan handle
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_SCAN_Resume(AM_SCAN_Handle_t handle);
/**\brief set helper to assist with the scan process
 * \param [in] handle scan handle
 * \param [in] helper to set
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_SCAN_SetHelper(AM_SCAN_Handle_t handle, AM_SCAN_Helper_t *helper);

#ifdef __cplusplus
}
#endif

#endif

