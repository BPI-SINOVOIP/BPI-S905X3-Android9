/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Description:
 */

#ifndef  _GE2D_MAIN_H
#define  _GE2D_MAIN_H
#ifdef __KERNEL__
#include "ge2d.h"
#include <linux/interrupt.h>
#include <linux/compat.h>
#include <mach/am_regs.h>
#include <linux/amlogic/amports/canvas.h>
#include <linux/fb.h>
#include <linux/list.h>
#include <asm/uaccess.h>
#include <linux/sysfs.h>
#include  <linux/spinlock.h>
#include <linux/kthread.h>
#include <linux/ion.h>
#endif
#include "ge2d_wq.h"
/**************************************************************
**                                                                              **
**    macro define                                                              **
**                                                                              **
***************************************************************/

#define         GE2D_CLASS_NAME                     "ge2d"

#define         GE2D_STRETCHBLIT_NOALPHA_NOBLOCK    0x4708
#define         GE2D_BLIT_NOALPHA_NOBLOCK           0x4707
#define         GE2D_BLEND_NOBLOCK                  0x4706
#define         GE2D_BLIT_NOBLOCK                   0x4705
#define         GE2D_STRETCHBLIT_NOBLOCK            0x4704
#define         GE2D_FILLRECTANGLE_NOBLOCK          0x4703


#define         GE2D_STRETCHBLIT_NOALPHA            0x4702
#define         GE2D_BLIT_NOALPHA                   0x4701
#define         GE2D_BLEND                          0x4700
#define         GE2D_BLIT                           0x46ff
#define         GE2D_STRETCHBLIT                    0x46fe
#define         GE2D_FILLRECTANGLE                  0x46fd
/* #define         GE2D_SRCCOLORKEY                          0x46fc */
#define         GE2D_SET_COEF                       0x46fb
/* #define         GE2D_CONFIG_EX                            0x46fa */
/* #define         GE2D_CONFIG                               0x46f9 */
#define         GE2D_ANTIFLICKER_ENABLE             0x46f8

#define         GE2D_IOC_MAGIC  'G'


#define    GE2D_CONFIG          _IOW(GE2D_IOC_MAGIC, 0x00, config_para_t)
#define    GE2D_CONFIG_EX       _IOW(GE2D_IOC_MAGIC, 0x01, config_para_ex_t)
#define    GE2D_SRCCOLORKEY     _IOW(GE2D_IOC_MAGIC, 0x02, config_para_t)
/**************************************************************
**                                                                              **
**    type  define                                                              **
**                                                                              **
***************************************************************/
#ifdef __KERNEL__
typedef  struct {
    char                name[20];
    unsigned int        open_count;
    int                 major;
    unsigned  int       dbg_enable;
    struct class        *cla;
    struct device       *dev;
    struct ion_client   *ion_client;
}ge2d_device_t;
#endif
#endif
