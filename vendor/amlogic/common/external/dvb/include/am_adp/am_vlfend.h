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
 * \author nengwen.chen <nengwen.chen@amlogic.com>
 * \date 2018-04-16: create the document
 ***************************************************************************/

#ifndef _AM_VFFEND_H_
#define _AM_VFFEND_H_

#include "am_types.h"
#include "am_evt.h"
#include "am_dmx.h"
/*add for config define for linux dvb *.h*/
#include <am_config.h>
#include <linux/dvb/frontend.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define VLFEND_FL_RUN_CB        (1)
#define VLFEND_FL_LOCK          (2)

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
extern AM_ErrorCode_t AM_VLFEND_Open(int dev_no, const AM_FEND_OpenPara_t *para);

/**\brief close a frontend device
 * \param dev_no frontend device number
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_VLFEND_Close(int dev_no);
extern AM_ErrorCode_t AM_VLFEND_CloseEx(int dev_no, AM_Bool_t reset);


/**\brief set frontend deivce mode
 * \param dev_no frontend device number
 * \param mode frontend demod mode
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_VLFEND_SetMode(int dev_no, int mode);

/**\brief set frontend parameter
 * \param dev_no frontend device number
 * \param[in] para frontend parameter
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_VLFEND_SetPara(int dev_no, const struct dvb_frontend_parameters *para);

/**\brief get frontend parameter
 * \param dev_no frontend device number
 * \param[out] para return frontend parameter
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_VLFEND_GetPara(int dev_no, struct dvb_frontend_parameters *para);

/**
 * \brief set frontend property
 * \param dev_no frontend device number
 * \param[in] prop frontend device property
 * \return
 *   - AM_SUCCESS onSuccess
 *   - or error code
 */
extern AM_ErrorCode_t AM_VLFEND_SetProp(int dev_no, const struct dtv_properties *prop);

/**
 * \brief get frontend property
 * \param dev_no frontend device number
 * \param[out] prop return frontend device property
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_VLFEND_GetProp(int dev_no, struct dtv_properties *prop);

/**\brief get frontend lock status
 * \param dev_no frontend device number
 * \param[out] status return frontend lock status
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_VLFEND_GetStatus(int dev_no, fe_status_t *status);

/**\brief get frontend frequency afc
 * \param dev_no frontend device number
 * \param[out] status return frontend frequency afc
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_VLFEND_GetAFC(int dev_no, int *afc);

/**\brief get frontend device's callback function
 * \param dev_no frontend device number
 * \param[out] cb return callback function
 * \param[out] user_data callback function's parameters
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_VLFEND_GetCallback(int dev_no, AM_FEND_Callback_t *cb, void **user_data);

/**\brief register the frontend callback function
 * \param dev_no frontend device function
 * \param[in] cb callback function
 * \param[in] user_data callback function's parameter
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_VLFEND_SetCallback(int dev_no, AM_FEND_Callback_t cb, void *user_data);

/**\brief enable or disable frontend callback
 * \param dev_no frontend device number
 * \param[in] enable_cb enable or disable
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_VLFEND_SetActionCallback(int dev_no, AM_Bool_t enable_cb);

/**\brief try to lock frontend and wait lock status
 * \param dev_no device frontend number
 * \param[in] para frontend parameters
 * \param[out] status return frontend lock status
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_VLFEND_Lock(int dev_no, const struct dvb_frontend_parameters *para, fe_status_t *status);

/**\brief set frontend thread delay time
 * \param dev_no frontend device number
 * \param delay time value in millisecond,  0 means no time interval and frontend thread will stop
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_VLFEND_SetThreadDelay(int dev_no, int delay);

/**\brief get frontend atv tune status
 * \param dev_no frontend device number
 * \param atv_status tune status value
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_VLFEND_GetAtvStatus(int dev_no, atv_status_t *atv_status);

/**\brief start frontend detect standard
 * \param dev_no frontend device number
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
extern AM_ErrorCode_t AM_VLFEND_DetectStandard(int dev_no);

#ifdef __cplusplus
}
#endif

#endif

