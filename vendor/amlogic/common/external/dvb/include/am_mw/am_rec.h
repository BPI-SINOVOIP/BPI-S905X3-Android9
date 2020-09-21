/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file am_rec.h
 * \brief Record manager module
 *
 * \author Xia Lei Peng <leipeng.xia@amlogic.com>
 * \date 2011-3-30: create the document
 ***************************************************************************/

#ifndef _AM_REC_H
#define _AM_REC_H

#include <am_types.h>
#include <am_evt.h>
#include <sqlite3.h>
#include <am_dvr.h>
#include <am_av.h>
#include <am_si.h>
#include "am_crypt.h"

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Macro definitions
 ***************************************************************************/
#define AM_REC_PATH_MAX 1024
#define AM_REC_NAME_MAX 255
#define AM_REC_SUFFIX_MAX 10

#define AM_REC_MediaInfo_t AM_AV_TimeshiftMediaInfo_t

/****************************************************************************
 * Error code definitions
 ****************************************************************************/

/**\brief Error code of record manager module*/
enum AM_REC_ErrorCode
{
	AM_REC_ERROR_BASE=AM_ERROR_BASE(AM_MOD_REC),
	AM_REC_ERR_INVALID_PARAM,               /**< Invalid parameter*/
	AM_REC_ERR_NO_MEM,                	/**< Not enough memory*/
	AM_REC_ERR_CANNOT_CREATE_THREAD,	/**< Cannot create new thread*/
	AM_REC_ERR_BUSY,                        /**< Recorder is already used*/
	AM_REC_ERR_CANNOT_OPEN_FILE,		/**< Cannot open the file*/
	AM_REC_ERR_CANNOT_WRITE_FILE,		/**< Write file failed*/
	AM_REC_ERR_CANNOT_ACCESS_FILE,		/**< Access deny*/
	AM_REC_ERR_DVR,                         /**< DVR device error*/
	AM_REC_ERR_END
};

/****************************************************************************
 * Type definitions
 ***************************************************************************/

/**Record manager handle*/
typedef void* AM_REC_Handle_t;

/**\brief State of the recording*/
enum
{
	AM_REC_STAT_NOT_START,	/**< Not started*/
	AM_REC_STAT_WAIT_START,	/**< Will start immediately, waiting the user's commit*/
	AM_REC_STAT_RECORDING,	/**< Recording*/
	AM_REC_STAT_COMPLETE,	/**< Completed, parameter type is AM_REC_RecEndPara_t*/
};

/**\brief Event type of the record manager*/
enum AM_REC_EventType
{
	AM_REC_EVT_BASE=AM_EVT_TYPE_BASE(AM_MOD_REC),
	AM_REC_EVT_RECORD_START,			/**< Recording started*/
	AM_REC_EVT_RECORD_END,				/**< Recording end*/
	AM_REC_EVT_END
};

/**\brief End parameter of the recording*/
typedef struct
{
	AM_REC_Handle_t hrec;	/**< Recording manager handle*/
	int error_code;	        /**< 0 means success, or M_REC_ErrorCode*/
	long long total_size;	/**< The file's total length*/
	int total_time;	        /**< The total duration time in seconds*/
}AM_REC_RecEndPara_t;

 /**\brief Record manager's create parameters*/
typedef struct 
{
	int		fend_dev;		/**< Frontend device number*/
	int		dvr_dev;		/**< DVR device number*/
	int		async_fifo_id;	        /**< Ayncfifo device's index*/
	char	store_dir[AM_REC_PATH_MAX];	/**< Store file's path*/
#ifdef SUPPORT_CAS
	int		is_smp;			/*Secure media path enable or not*/
	uint8_t *secure_buf;	/*used to store DVR data*/
	char	cas_dat_path[AM_REC_PATH_MAX];
#endif
}AM_REC_CreatePara_t;

typedef struct
{
	int count;
	uint16_t pids[AM_DVR_MAX_PID_COUNT];
}AM_REC_Pids_t;

/**\brief Recording parameters*/
typedef struct
{
	AM_Bool_t is_timeshift;	        /**< Is timeshifting or not*/
	dvbpsi_pat_program_t program;   /**< Program information*/
	AM_REC_MediaInfo_t media_info;  /**< Media information*/
	int total_time;			/**< Total duration time in seconds, <=0 means recoding will stopped manually*/
	int64_t total_size;                 /**< Max size, <=0 means no limit*/
	char prefix_name[AM_REC_NAME_MAX];    /**< Filename prefix*/
	char suffix_name[AM_REC_SUFFIX_MAX];  /**< Filename suffix*/
	AM_Crypt_Ops_t *crypt_ops;
	AM_REC_Pids_t ext_pids;
#ifdef SUPPORT_CAS
	AM_CAS_encrypt enc_cb;
	uint32_t cb_param;
#endif
}AM_REC_RecPara_t;

/**\brief Recording information*/
typedef struct
{
	char file_path[AM_REC_PATH_MAX];	/**< Current filename*/
	long long file_size;	/**< File length*/
	int cur_rec_time;	/**< Current time in seconds*/
	AM_REC_CreatePara_t create_para;	/**< Create parameters*/
	AM_REC_RecPara_t record_para;	/**< Recording parameters*/
}AM_REC_RecInfo_t;

#ifdef SUPPORT_CAS
typedef struct
{
	uint32_t addr;
	uint32_t len;
}AM_Rec_SecureBlock_t;
#endif

/****************************************************************************
 * Function prototypes  
 ***************************************************************************/

/**\brief Create a new record manager
 * \param [in] para Create parameters
 * \param [out] handle Return the manager's handle
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_REC_Create(AM_REC_CreatePara_t *para, AM_REC_Handle_t *handle);

/**\brief Release a record manager
 * \param handle Record manager handle
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_REC_Destroy(AM_REC_Handle_t handle);

/**\brief Start recording
 * \param handle Record manager handle
 * \param [in] start_para Start parameters
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_REC_StartRecord(AM_REC_Handle_t handle, AM_REC_RecPara_t *start_para);

/**\brief Stop recording
 * \param handle Record manager handle
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_REC_StopRecord(AM_REC_Handle_t handle);

/**\brief Set the user defined data to the record manager
 * \param handle Record manager handle
 * \param [in] user_data User defined data
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_REC_SetUserData(AM_REC_Handle_t handle, void *user_data);

/**\brief Get the user defined data from the record manager
 * \param handle Record manager handle
 * \param [out] user_data Return the user defined data
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_REC_GetUserData(AM_REC_Handle_t handle, void **user_data);

/**\brief Set the record file's path
 * \param handle Record manager handle
 * \param [in] path File path
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_REC_SetRecordPath(AM_REC_Handle_t handle, const char *path);

/**\brief Get the current recording information
 * \param handle Record manager handle
 * \param [out] info Return the current information
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_REC_GetRecordInfo(AM_REC_Handle_t handle, AM_REC_RecInfo_t *info);

#define REC_TFILE_FLAG_AUTO_CREATE 1
#define REC_TFILE_FLAG_DETACH 2
extern AM_ErrorCode_t AM_REC_SetTFile(AM_REC_Handle_t handle, AM_TFile_t tfile, int flag);
extern AM_ErrorCode_t AM_REC_GetTFile(AM_REC_Handle_t handle, AM_TFile_t *tfile, int *flag);

extern AM_ErrorCode_t AM_REC_PauseRecord(AM_REC_Handle_t handle);
extern AM_ErrorCode_t AM_REC_ResumeRecord(AM_REC_Handle_t handle);

extern AM_ErrorCode_t AM_REC_GetMediaInfoFromFile(const char *name, AM_REC_MediaInfo_t *info);
#ifdef __cplusplus
}
#endif

#endif

