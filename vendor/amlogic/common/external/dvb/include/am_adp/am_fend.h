/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file
 * \brief DVB Frontend Module
 *
 * Basic data structures definition in "linux/dvb/frontend.h"
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-06-07: create the document
 ***************************************************************************/

#ifndef _AM_FEND_H
#define _AM_FEND_H

#include "am_types.h"
#include "am_evt.h"
#include "am_dmx.h"
/*add for config define for linux dvb *.h*/
#include "am_config.h"
#include <linux/dvb/frontend.h>

// #ifdef FE_SET_FRONTEND
// #undef FE_SET_FRONTEND
// #define FE_SET_FRONTEND FE_SET_FRONTEND_EX
// #endif

// #ifdef FE_GET_FRONTEND
// #undef FE_GET_FRONTEND
// #define FE_GET_FRONTEND FE_GET_FRONTEND_EX
// #endif

//#define dvb_frontend_parameters dvb_frontend_parameters_ex

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

/****************************************************************************
 * Error code definitions
 ****************************************************************************/

/**\brief Error code of the fend module*/
enum AM_FEND_ErrorCode
{
	AM_FEND_ERROR_BASE=AM_ERROR_BASE(AM_MOD_FEND),
	AM_FEND_ERR_NO_MEM,                   /**< Not enough memory*/
	AM_FEND_ERR_BUSY,                     /**< The device has already been openned*/
	AM_FEND_ERR_INVALID_DEV_NO,           /**< Invalid device numbe*/
	AM_FEND_ERR_NOT_OPENNED,              /**< The device not open*/
	AM_FEND_ERR_CANNOT_CREATE_THREAD,     /**< Cannot create new thread*/
	AM_FEND_ERR_NOT_SUPPORTED,            /**< Not supported*/
	AM_FEND_ERR_CANNOT_OPEN,              /**< Cannot open device*/
	AM_FEND_ERR_TIMEOUT,                  /**< Timeout*/
	AM_FEND_ERR_INVOKE_IN_CB,             /**< Invoke in callback function*/
	AM_FEND_ERR_IO,                       /**< IO error*/
	AM_FEND_ERR_BLINDSCAN, 				  /**< Blindscan error*/
	AM_FEND_ERR_BLINDSCAN_INRUNNING, 	  /**< In Blindscan*/
	AM_FEND_ERR_END
};

/****************************************************************************
 * Event type definitions
 ****************************************************************************/

/**\brief DVB frontend event type*/
enum AM_FEND_EventType
{
	AM_FEND_EVT_BASE=AM_EVT_TYPE_BASE(AM_MOD_FEND),
	AM_FEND_EVT_STATUS_CHANGED,    /**< Frontend status changed, argument's type: struct dvb_frontend_event*/
	AM_FEND_EVT_ROTOR_MOVING,    /**< Rotor moving*/
	AM_FEND_EVT_ROTOR_STOP,    /**< Rotor stop*/
	AM_FEND_EVT_SHORT_CIRCUIT, /**< Frontend circuit*/
	AM_FEND_EVT_SHORT_CIRCUIT_REPAIR, /**< Frontend circuit repair*/
	AM_VLFEND_EVT_STATUS_CHANGED,
	AM_FEND_EVT_END
};

/****************************************************************************
 * Type definitions
 ***************************************************************************/

/**\brief frontend open parameter*/
typedef struct
{
	int mode;
	#define FE_AUTO (-1)    /**< Do not care the mode*/
	#define FE_UNKNOWN (-2) /**< Set mode to unknown, something like reset*/
} AM_FEND_OpenPara_t;

/**\brief frontend callback function*/
typedef void (*AM_FEND_Callback_t) (int dev_no, struct dvb_frontend_event *evt, void *user_data);

/**\brief Frontend blindscan status*/
typedef enum
{
	AM_FEND_BLIND_START,			/**< Blindscan start*/
	AM_FEND_BLIND_UPDATEPROCESS,	/**< Blindscan update process*/
	AM_FEND_BLIND_UPDATETP			/**< Blindscan update transport program*/
} AM_FEND_BlindStatus_t;

/**\brief Blindscan event*/
typedef struct
{
	AM_FEND_BlindStatus_t    status; /**< Blindscan status*/
	union
	{
		unsigned int freq;
		unsigned int process;
	};
} AM_FEND_BlindEvent_t;

/**\brief Blindscan callback function*/
typedef void (*AM_FEND_BlindCallback_t) (int dev_no, AM_FEND_BlindEvent_t *evt, void *user_data);


/****************************************************************************
 * Function prototypes  
 ***************************************************************************/

/**\brief open a frontend device
 * \param dev_no frontend device number
 * \param[in] para frontend's device open parameters
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_FEND_Open(int dev_no, const AM_FEND_OpenPara_t *para);

/**\brief close a frontend device
 * \param dev_no frontend device number
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_FEND_OpenEx(int dev_no, const AM_FEND_OpenPara_t *para, int *fe_fd);
extern AM_ErrorCode_t AM_FEND_Close(int dev_no);
extern AM_ErrorCode_t AM_FEND_CloseEx(int dev_no, AM_Bool_t reset);


/**\brief set frontend deivce mode
 * \param dev_no frontend device number
 * \param mode frontend demod mode
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_FEND_SetMode(int dev_no, int mode);

/**\brief get frontend device infomation
 * \param dev_no frontend device number
 * \param[out] info return frontend infomation struct
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_FEND_GetInfo(int dev_no, struct dvb_frontend_info *info);

/**\brief get a frontend device's ts source
 * \param dev_no frontend device number
 * \param[out] src retrun device's ts source
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_FEND_GetTSSource(int dev_no, AM_DMX_Source_t *src);

/**\brief set frontend parameter
 * \param dev_no frontend device number
 * \param[in] para frontend parameter
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_FEND_SetPara(int dev_no, const struct dvb_frontend_parameters *para);

/**
 * \brief set frontend property
 * \param dev_no frontend device number
 * \param[in] prop frontend device property
 * \return
 *   - AM_SUCCESS onSuccess
 *   - or error code
 */
extern AM_ErrorCode_t AM_FEND_SetProp(int dev_no, const struct dtv_properties *prop);

/**\brief get frontend parameter
 * \param dev_no frontend device number
 * \param[out] para return frontend parameter
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_FEND_GetPara(int dev_no, struct dvb_frontend_parameters *para);

/**
 * \brief get frontend property
 * \param dev_no frontend device number
 * \param[out] prop return frontend device property
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_FEND_GetProp(int dev_no, struct dtv_properties *prop);

/**\brief get frontend lock status
 * \param dev_no frontend device number
 * \param[out] status return frontend lock status
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_FEND_GetStatus(int dev_no, fe_status_t *status);

/**\brief get frontend device's SNR
 * \param dev_no frontend device number
 * \param[out] snr return SNR
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_FEND_GetSNR(int dev_no, int *snr);

/**\brief get frontend device's BER
 * \param dev_no frontend device number
 * \param[out] ber return frontend's BER
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_FEND_GetBER(int dev_no, int *ber);

/**\brief get frontend device's signal strength
 * \param dev_no frontend device number
 * \param[out] strength return signal strength
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_FEND_GetStrength(int dev_no, int *strength);

/**\brief get frontend device's callback function
 * \param dev_no frontend device number
 * \param[out] cb return callback function
 * \param[out] user_data callback function's parameters
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_FEND_GetCallback(int dev_no, AM_FEND_Callback_t *cb, void **user_data);

/**\brief register the frontend callback function
 * \param dev_no frontend device function
 * \param[in] cb callback function
 * \param[in] user_data callback function's parameter
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_FEND_SetCallback(int dev_no, AM_FEND_Callback_t cb, void *user_data);

/**\brief enable or disable frontend callback
 * \param dev_no frontend device number
 * \param[in] enable_cb enable or disable
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_FEND_SetActionCallback(int dev_no, AM_Bool_t enable_cb);

/**\brief try to lock frontend and wait lock status
 * \param dev_no device frontend number
 * \param[in] para frontend parameters
 * \param[out] status return frontend lock status
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_FEND_Lock(int dev_no, const struct dvb_frontend_parameters *para, fe_status_t *status);

/**\brief set frontend thread delay time
 * \param dev_no frontend device number
 * \param delay time value in millisecond,  0 means no time interval and frontend thread will stop
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_FEND_SetThreadDelay(int dev_no, int delay);

/**\brief reset digital satellite equipment control
 * \param dev_no frontend device number
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_FEND_DiseqcResetOverload(int dev_no); 

/**\brief send digital satellite command
 * \param dev_no frontend device number
 * \param[in] cmd control command
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_FEND_DiseqcSendMasterCmd(int dev_no, struct dvb_diseqc_master_cmd* cmd); 

/**\brief recv digital satellite command's reply
 * \param dev_no frontend device number
 * \param[out] reply digital satellite's reply
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_FEND_DiseqcRecvSlaveReply(int dev_no, struct dvb_diseqc_slave_reply* reply); 

/**\brief sent command to control tone burst
 * \param dev_no frontend device number
 * \param minicmd tone burst command
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_FEND_DiseqcSendBurst(int dev_no, fe_sec_mini_cmd_t minicmd); 

/**\brief set tone mode
 * \param dev_no frontend device number
 * \param tone device tone mode
 * \return
 *   - AM_SUCCESS On Success
 *   - or error code
 */
extern AM_ErrorCode_t AM_FEND_SetTone(int dev_no, fe_sec_tone_mode_t tone); 

/**\brief set device voltage
 * \param dev_no frontend device number
 * \param voltage device voltage
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_FEND_SetVoltage(int dev_no, fe_sec_voltage_t voltage); 

/**\brief enable device High Lnb Voltage
 * \param dev_no frontend device number
 * \param arg 0 means disable, !=0 means enable
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_FEND_EnableHighLnbVoltage(int dev_no, long arg);                          
                                                                           
/**\brief start satellite blind scan
 * \param dev_no frontend device number
 * \param[in] cb blind scan callback function
 * \param[in] user_data callback function parameter
 * \param start_freq start frequency unit HZ
 * \param stop_freq stop frequency unit HZ
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_FEND_BlindScan(int dev_no, AM_FEND_BlindCallback_t cb, void *user_data, unsigned int start_freq, unsigned int stop_freq);

/**\brief exit blind scan
 * \param dev_no frontend device number
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_FEND_BlindExit(int dev_no); 

/**\brief get blind scan process
 * \param dev_no frontend device number
 * \param[out] process blind scan process 0-100
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_FEND_BlindGetProcess(int dev_no, unsigned int *process);

/**\brief 卫星盲扫信息
 * \param dev_no 前端设备号
 * \param[in out] para in 盲扫频点信息缓存区大小，out 盲扫频点个数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_fend.h)
 */
AM_ErrorCode_t AM_FEND_BlindGetTPCount(int dev_no, unsigned int *count);


/**\brief get blind scan TP infomation
 * \param dev_no frontend device number
 * \param[out] para blindscan parameters
 * \param[out] count parameters's count
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_FEND_BlindGetTPInfo(int dev_no, struct dvb_frontend_parameters *para, unsigned int count);

/**\brief try to tune
 *\param dev_no frontend device number
 *\param freq frequency unit Hz
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_FEND_FineTune(int dev_no, unsigned int freq);

/**\brief set CVBS AMP OUT
 *\param dev_no frontend device number
 *\param amp AMP
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_FEND_SetCvbsAmpOut(int dev_no, unsigned int amp);

/**\brief get atv status
 *\param dev_no frontend device number
 *\param[out] atv_status return atv status struct
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
AM_ErrorCode_t AM_FEND_GetAtvStatus(int dev_no,  atv_status_t *atv_status);

/**\brief try to set AFC
 *\param dev_no frontend device number
 *\param afc AFC
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
AM_ErrorCode_t AM_FEND_SetAfc(int dev_no, unsigned int afc);


/**\brief try to set sub sys
 *\param dev_no frontend device number
 *\param sub_sys fend type,T or T2,C-A C-B OR C-A
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
AM_ErrorCode_t AM_FEND_SetSubSystem(int dev_no, unsigned int sub_sys);


/**\brief try to get sub sys
 *\param dev_no frontend device number
 *\param sub_sys fend type,use to indentify T or T2,C-A C-B OR C-C
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
AM_ErrorCode_t AM_FEND_GetSubSystem(int dev_no, unsigned int *sub_sys);

#ifdef __cplusplus
}
#endif

#endif

