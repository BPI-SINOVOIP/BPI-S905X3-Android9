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
/**\file
 * \brief AMLogic adaptor layer全局锁
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-07-05: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 0

#include <am_debug.h>
#include <am_mem.h>
#include "../am_adp_internal.h"

/****************************************************************************
 * Data definitions
 ***************************************************************************/

pthread_mutex_t am_gAdpLock = PTHREAD_MUTEX_INITIALIZER;

