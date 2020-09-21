/*
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
#ifndef _AM_CI_INTERNAL_H_
#define _AM_CI_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif


typedef struct _ci_s _ci_t;

int ci_caman_lock(AM_CI_Handle_t handle, int lock);


#ifdef __cplusplus
}
#endif

#endif/*_AM_CI_INTERNAL_H_*/

