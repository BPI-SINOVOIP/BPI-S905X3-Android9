/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:

 */
/**\file
 * \brief aml config
 * \author hualing.chen <hualing.chen@amlogic.com>
 * \date 2017-04-25: create the document
 ***************************************************************************/

#ifndef _AM_CONFIG_H
#define _AM_CONFIG_H

#ifndef CONFIG_AMLOGIC_DVB_COMPAT
/**\brief int value: 1*/
#define CONFIG_AMLOGIC_DVB_COMPAT       (1)
#endif

#ifndef __DVB_CORE__
/**\brief int value: 1*/
#define __DVB_CORE__       (1)
#endif

#include <linux/dvb/frontend.h>

typedef struct atv_status_s atv_status_t;
typedef struct tuner_param_s tuner_param_t;
//typedef enum fe_ofdm_mode fe_ofdm_mode_t;

#define fe_ofdm_mode_t enum fe_ofdm_mode
#endif

