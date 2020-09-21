/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file
 * \brief Driver event module
 *
 * Event is driver asyncronized message callback mechanism.
 * User can register multiply callback functions in one event.
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-09-10: create the document
 ***************************************************************************/

#ifndef _AM_EVT_H
#define _AM_EVT_H

#include "am_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

#define AM_EVT_TYPE_BASE(no)    ((no)<<24)

/****************************************************************************
 * Error code definitions
 ****************************************************************************/

/**\brief Error code of the event module*/
enum AM_EVT_ErrorCode
{
	AM_EVT_ERROR_BASE=AM_ERROR_BASE(AM_MOD_EVT),
	AM_EVT_ERR_NO_MEM,                  /**< Not enough memory*/
	AM_EVT_ERR_NOT_SUBSCRIBED,          /**< The event is not subscribed*/
	AM_EVT_ERR_END
};

/****************************************************************************
 * Type definitions
 ***************************************************************************/

/**\brief Event callback function
 * \a dev_no The device number.
 * \a event_type The event type.
 * \a param The event callback's parameter.
 * \a data User defined data registered when event subscribed.
 */
typedef void (*AM_EVT_Callback_t)(long dev_no, int event_type, void *param, void *data);

/****************************************************************************
 * Function prototypes  
 ***************************************************************************/

/**\brief Subscribe an event
 * \param dev_no Device number generated the event
 * \param event_type Event type
 * \param cb Callback function's pointer
 * \param data User defined parameter of the callback function
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_EVT_Subscribe(long dev_no, int event_type, AM_EVT_Callback_t cb, void *data);

/**\brief Unsubscribe an event
 * \param dev_no Device number generated the event
 * \param event_type Event type
 * \param cb Callback function's pointer
 * \param data User defined parameter of the callback function
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_EVT_Unsubscribe(long dev_no, int event_type, AM_EVT_Callback_t cb, void *data);

/**\brief Signal an event occured
 * \param dev_no The device number generate the event
 * \param event_type Event type
 * \param[in] param Parameter of the event
 */
extern AM_ErrorCode_t AM_EVT_Signal(long dev_no, int event_type, void *param);

extern AM_ErrorCode_t AM_EVT_Init();
extern AM_ErrorCode_t AM_EVT_Destory();

#ifdef __cplusplus
}
#endif

#endif

