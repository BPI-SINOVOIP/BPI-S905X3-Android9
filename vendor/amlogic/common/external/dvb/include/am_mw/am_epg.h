/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file am_epg.h
 * \brief EPG scanner module
 *
 * \author Xia Lei Peng <leipeng.xia@amlogic.com>
 * \date 2010-11-04: create the document
 ***************************************************************************/

#ifndef _AM_EPG_H
#define _AM_EPG_H

#include <am_types.h>
#include <am_mem.h>
#include <am_fend.h>
#include <am_si.h>
#include <am_db.h>

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

/**\brief Convert 16 bits MJD value in SI to seconds*/
#define AM_EPG_MJD2SEC(m) (((m)-40587)*86400)

/**\brief Convert hour/minute/second BCD value in SI to seconds*/
#define AM_EPG_BCD2SEC(h,m,s)\
		((3600 * ((10*(((h) & 0xF0)>>4)) + ((h) & 0xF))) + \
		(60 * ((10*(((m) & 0xF0)>>4)) + ((m) & 0xF))) + \
		((10*(((s) & 0xF0)>>4)) + ((s) & 0xF)))

#define EVT_NAME_LEN 256
#define ITEM_DESCR_LEN 256
#define ITEM_CHAR_LEN 1024
#define EVT_TEXT_LEN 1024
#define EXT_TEXT_LEN 4096
#define MAX_LANGUAGE_CNT 4
#define MAX_ITEM_CNT 16

#define COMPOSED_NAME_LEN ((EVT_NAME_LEN/*single len*/+1/*extra 0x80 for language splitting*/+3/*language code*/)*MAX_LANGUAGE_CNT)
#define COMPOSED_DESCR_LEN ((EVT_TEXT_LEN+4)*MAX_LANGUAGE_CNT)
#define COMPOSED_EXT_ITEM_LEN ((ITEM_DESCR_LEN+ITEM_CHAR_LEN+2/*:\n*/+4)*MAX_LANGUAGE_CNT)
#define COMPOSED_EXT_DESCR_LEN ((EXT_TEXT_LEN+4)*MAX_LANGUAGE_CNT)


/****************************************************************************
 * Type definitions
 ***************************************************************************/

typedef void* AM_EPG_Handle_t;

/**\brief Error code of the EPG scanner module*/
enum AM_EPG_ErrorCode
{
	AM_EPG_ERROR_BASE=AM_ERROR_BASE(AM_MOD_EPG),
	AM_EPG_ERR_INVALID_PARAM,			/**< Invalid parameter*/
	AM_EPG_ERR_NOT_SUPPORTED,           /**< Not supported*/
	AM_EPG_ERR_NO_MEM,                  /**< Not enough memory*/
	AM_EPG_ERR_CANNOT_CREATE_THREAD,    /**< Cannot create thread*/
	AM_EPG_ERR_CANNOT_CREATE_SI,		/**< Cannot create SI parser*/
	AM_EPG_ERR_SUBSCRIBE_EVENT_FAILED,	/**< Subscribe EPG event failed*/
	AM_EPG_ERR_END
};

/**\brief EPG scanner working mode*/
enum AM_EPG_Mode
{
	AM_EPG_SCAN_PAT = 0x01,	/**< Scan PAT*/
	AM_EPG_SCAN_PMT = 0x02,	/**< Scan PMT*/
	AM_EPG_SCAN_CAT = 0x04,	/**< Scan CAT*/
	AM_EPG_SCAN_SDT = 0x08,	/**< Scan SDT*/
	AM_EPG_SCAN_NIT = 0x10,	/**< Scan NIT*/
	AM_EPG_SCAN_TDT = 0x20,	/**< Scan TDT*/
	AM_EPG_SCAN_EIT_PF_ACT 		= 0x40,	/**< Scan EIF PF of current TS*/
	AM_EPG_SCAN_EIT_PF_OTH 		= 0x80,	/**< Scan EIT PF of other TS*/
	AM_EPG_SCAN_EIT_SCHE_ACT	= 0x100,	/**< Scan EIT schedule of current TS*/
	AM_EPG_SCAN_EIT_SCHE_OTH 	= 0x200,	/**< Scan EIF schedule of other TS*/
	
	/*For ATSC*/
	AM_EPG_SCAN_MGT	= 0x400,	/**< Scan ATSC MGT*/
	AM_EPG_SCAN_VCT	= 0x800,	/**< Scan ATSC VCT*/
	AM_EPG_SCAN_STT	= 0x1000,	/**< Scan ATSC STT*/
	AM_EPG_SCAN_RRT	= 0x2000,	/**< Scan ATSC RRT*/
	AM_EPG_SCAN_PSIP_EIT = 0x4000,	/**< Scan ATSC EIT*/
	AM_EPG_SCAN_PSIP_ETT = 0x8000,	/**< Scan ATSC ETT*/
	AM_EPG_SCAN_PSIP_CEA = 0x10000,	/**< Scan ATSC CEA*/
	AM_EPG_SCAN_PSIP_EIT_VERSION_CHANGE = 0x20000, /**< Scan ATSC EIT version change flag*/
	
	/*Composed mode*/
	AM_EPG_SCAN_EIT_PF_ALL		= AM_EPG_SCAN_EIT_PF_ACT | AM_EPG_SCAN_EIT_PF_OTH,/**< Scan EIT PF of all the TS*/
	AM_EPG_SCAN_EIT_SCHE_ALL 	= AM_EPG_SCAN_EIT_SCHE_ACT | AM_EPG_SCAN_EIT_SCHE_OTH,/**< Scan EIT schedule of all the TS*/
	AM_EPG_SCAN_EIT_ALL 		= AM_EPG_SCAN_EIT_PF_ALL | AM_EPG_SCAN_EIT_SCHE_ALL, /**< Scan all EIT*/
	AM_EPG_SCAN_ALL				= AM_EPG_SCAN_PAT|AM_EPG_SCAN_PMT|AM_EPG_SCAN_CAT|AM_EPG_SCAN_SDT|AM_EPG_SCAN_NIT|AM_EPG_SCAN_TDT|AM_EPG_SCAN_EIT_ALL|
									AM_EPG_SCAN_MGT|AM_EPG_SCAN_VCT|AM_EPG_SCAN_STT|AM_EPG_SCAN_RRT|AM_EPG_SCAN_PSIP_EIT|AM_EPG_SCAN_PSIP_ETT|AM_EPG_SCAN_PSIP_CEA,
};

/**\brief EPG event type*/
enum AM_EPG_EventType
{
	AM_EPG_EVT_BASE=AM_EVT_TYPE_BASE(AM_MOD_EPG),
	AM_EPG_EVT_NEW_PAT,    /**< New PAT received, parameter type is dvbpsi_pat_t*/
	AM_EPG_EVT_NEW_PMT,    /**< New PMT received, parameter type is dvbpsi_pmt_t*/
	AM_EPG_EVT_NEW_CAT,    /**< New CAT received, parameter type is dvbpsi_cat_t*/
	AM_EPG_EVT_NEW_SDT,    /**< New SDT received, parameter type is dvbpsi_sdt_t*/
	AM_EPG_EVT_NEW_NIT,    /**< New NIT received, parameter type is dvbpsi_nit_t*/
	AM_EPG_EVT_NEW_TDT,    /**< New NIT received, parameter type is dvbpsi_tot_t*/
	AM_EPG_EVT_NEW_EIT,    /**< New EIT received, parameter type is dvbpsi_eit_t*/
	AM_EPG_EVT_NEW_STT,    /**< New ATSC STT received, parameter type is stt_section_info_t*/
	AM_EPG_EVT_NEW_RRT,    /**< New ATSC RRT received, parameter type is rrt_section_info_t*/
	AM_EPG_EVT_NEW_MGT,    /**< New ATSC MGT received, parameter type is mgt_section_info_t*/
	AM_EPG_EVT_NEW_VCT,    /**< New ATSC VCT received, parameter type is vct_section_info_t*/
	AM_EPG_EVT_NEW_PSIP_EIT,/**< New ATSC EIT received, parameter type is eit_section_info_t*/
	AM_EPG_EVT_NEW_PSIP_ETT,/**< New ATSC ETT received, parameter type is ett_section_info_t*/
	AM_EPG_EVT_EIT_DONE,   /**< All EIT received*/
	AM_EPG_EVT_UPDATE_EVENTS,	/**< EIT received, update the application's event information*/
	AM_EPG_EVT_UPDATE_PROGRAM_AV,	/**< Program's AV information is changed*/
	AM_EPG_EVT_UPDATE_PROGRAM_NAME,	/**< Program name is changed*/
	AM_EPG_EVT_UPDATE_TS,	/**< TS information is updated*/
	AM_EPG_EVT_NEW_CEA,    /**< New ATSC CEA received, parameter type is dvbpsi_atsc_cea_t*/
	AM_EPG_EVT_PMT_RATING,    /**< New PMT received, parameter type is AM_EPG_PmtRating_t*/
	AM_EPG_EVT_END
};

/**\brief EPG scanner's operation mode*/
enum AM_EPG_ModeOp
{
	AM_EPG_MODE_OP_ADD, 	 /**< Add new scanning tables*/
	AM_EPG_MODE_OP_REMOVE,	 /**< Remove scanning tables*/
	AM_EPG_MODE_OP_SET,      /**< Reset the scanning tables*/
};

/**EPG Table type*/
enum AM_EPG_TAB
{
	AM_EPG_TAB_PAT,		/**<PAT*/
	AM_EPG_TAB_PMT,		/**<PMT*/
	AM_EPG_TAB_CAT,		/**<CAT*/
	AM_EPG_TAB_SDT,		/**<SDT*/
	AM_EPG_TAB_NIT,		/**<NIT*/
	AM_EPG_TAB_TDT,		/**<TDT*/
	AM_EPG_TAB_EIT,		/**<EIT*/
	AM_EPG_TAB_STT,		/**<STT*/
	AM_EPG_TAB_RRT,		/**<RRT*/
	AM_EPG_TAB_MGT,		/**<MGT*/
	AM_EPG_TAB_VCT,		/**<VCT*/
	AM_EPG_TAB_PSIP_EIT,	/**<PSIP EIT*/
	AM_EPG_TAB_PSIP_ETT,	/**<PSIP ETT*/
	AM_EPG_TAB_PSIP_CEA,	/**<PSIP ETT*/
	AM_EPG_TAB_MAX
};

typedef struct AM_EPG_Event_s AM_EPG_Event_t;

typedef void (*AM_EPG_Events_UpdateCB_t) (AM_EPG_Handle_t handle, int event_count, AM_EPG_Event_t *pevents);
typedef void (*AM_EPG_PMT_UpdateCB_t) (AM_EPG_Handle_t handle, dvbpsi_pmt_t *pmts);
typedef void (*AM_EPG_TAB_UpdateCB_t) (AM_EPG_Handle_t handle, int type, void *tables, void *user_data);

/**\brief EPG scanner's create parameters*/
typedef struct
{
	int  i_program_number;	/**< program number*/
	char rating[1024];  /**< rating buf*/
}AM_EPG_PmtRating_t;

/**\brief EPG scanner's create parameters*/
typedef struct
{
	int fend_dev;	/**< Frontend device's number*/
	int dmx_dev;	/**< Demux device's number*/
	int source;	/**< TS input source*/
	sqlite3	*hdb;	/**< Database handle*/
	char text_langs[128];
}AM_EPG_CreatePara_t;

struct AM_EPG_Event_s {
	int src;
	unsigned short srv_id;
	unsigned short ts_id;
	unsigned short net_id;
	unsigned short evt_id;
	int start;
	int end;
	int nibble;
	int parental_rating;
	char name[COMPOSED_NAME_LEN+1];
	char desc[COMPOSED_DESCR_LEN+1];
	char ext_item[COMPOSED_EXT_ITEM_LEN + 1];
	char ext_descr[COMPOSED_EXT_DESCR_LEN + 1];
	int sub_flag;
	int sub_status;
	int source_id;
	char rrt_ratings[1024];
};


/****************************************************************************
 * Function prototypes  
 ***************************************************************************/
 
/**\brief Create a new EPG scanner
 * \param [in] para Create parameters
 * \param [out] handle Return the scanner's handle
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_EPG_Create(AM_EPG_CreatePara_t *para, AM_EPG_Handle_t *handle);

/**\brief Release a EPG scanner
 * \param handle EPG scanner handle
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_EPG_Destroy(AM_EPG_Handle_t handle);

/**\brief Set the EPG scanner's working mode
 * \param handle EPG scanner handle
 * \param op Operation type
 * \param mode EPG scanning mode
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_EPG_ChangeMode(AM_EPG_Handle_t handle, int op, int mode);

/**\brief Set the current service, scan PMT and EIT actual pf
 * \param handle EPG scanner handle
 * \param db_srv_id Service's database record index
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_EPG_MonitorService(AM_EPG_Handle_t handle, int db_srv_id);

/**\brief Set the interval time of EIT PF scanning
 * \param handle EPG scanner handle
 * \param distance Interval time in milliseconds
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_EPG_SetEITPFCheckDistance(AM_EPG_Handle_t handle, int distance);

/**\brief Set the interval time of EIT schedule scanning
 * \param handle EPG scanner handle
 * \param distance Interval time in milliseconds
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_EPG_SetEITScheCheckDistance(AM_EPG_Handle_t handle, int distance);


/**\brief Convert the string's character encoding type
 * \param [in] in_code Input characters buffer
 * \param in_len Input characters length in bytes
 * \param [out] out_code Output character buffer
 * \param out_len Output buffer's size
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_EPG_ConvertCode(char *in_code,int in_len,char *out_code,int out_len);

/**\brief Get the current UTC time received from EPG
 * \param [out] utc_time UTC time
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_EPG_GetUTCTime(int *utc_time);

/**\brief Convert the UTC time to local time
 * \param utc_time UTC time
 * \param [out] local_time Local time
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_EPG_UTCToLocal(int utc_time, int *local_time);

/**\brief Convert the local time to UTC time
 * \param local_time Local time
 * \param [out] utc_time UTC time
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_EPG_LocalToUTC(int local_time, int *utc_time);

/**\brief Set the local time offset from the UTC time
 * \param offset Time offset
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_EPG_SetLocalOffset(int offset);

/**\brief Subscribe an event
 * \param handle EPG EPG scanner
 * \param db_evt_id The event's database record index
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_EPG_SubscribeEvent(AM_EPG_Handle_t handle, int db_evt_id);

/**\brief Set the user defined data to the EPG scanner handle
 * \param handle EPG scanner handle
 * \param [in] user_data User defined data
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_EPG_SetUserData(AM_EPG_Handle_t handle, void *user_data);

/**\brief Get the user defined data from the EPG scanner handle
 * \param handle EPG scanner handle
 * \param [out] user_data Return the user defined data
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_EPG_GetUserData(AM_EPG_Handle_t handle, void **user_data);


extern AM_ErrorCode_t AM_EPG_MonitorServiceByID(AM_EPG_Handle_t handle, int ts_id, int srv_id, AM_EPG_PMT_UpdateCB_t pmt_cb);

extern AM_ErrorCode_t AM_EPG_SetEventsCallback(AM_EPG_Handle_t handle, AM_EPG_Events_UpdateCB_t cb);

extern void AM_EPG_FreeEvents(int event_count, AM_EPG_Event_t *pevents);

extern AM_ErrorCode_t AM_EPG_SetTablesCallback(AM_EPG_Handle_t handle, int table_type, AM_EPG_TAB_UpdateCB_t cb, void *user_data);

extern AM_ErrorCode_t AM_EPG_DisableDefProc(AM_EPG_Handle_t handle, AM_Bool_t disable);


#ifdef __cplusplus
}
#endif

#endif

