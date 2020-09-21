/***************************************************************************
 * Copyright (C) 2017 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * Description:
 */
/**\file
 * \brief Condition variable.
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2017-11-07: create the document
 ***************************************************************************/

#ifndef _AM_COND_H
#define _AM_COND_H

#include "am_types.h"
#include <pthread.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define pthread_cond_init(c, a)\
	({\
		pthread_condattr_t attr;\
		pthread_condattr_t *pattr;\
		int r;\
		if (a) {\
			pattr = a;\
		} else {\
			pattr = &attr;\
			pthread_condattr_init(pattr);\
		}\
		pthread_condattr_setclock(pattr, CLOCK_MONOTONIC);\
		r = pthread_cond_init(c, pattr);\
		if (pattr == &attr) {\
			pthread_condattr_destroy(pattr);\
		}\
		r;\
	 })

#ifdef __cplusplus
}
#endif

#endif

