/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to MediaTek Inc. and/or its licensors. Without
 * the prior written permission of MediaTek inc. and/or its licensors, any
 * reproduction, modification, use or disclosure of MediaTek Software, and
 * information contained herein, in whole or in part, shall be strictly
 * prohibited.
 *
 * MediaTek Inc. (C) 2014. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER
 * ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 * NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH
 * RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 * INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES
 * TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
 * RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
 * OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK
 * SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE
 * RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S
 * ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE
 * RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE
 * MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE
 * CHARGE PAID BY RECEIVER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek
 * Software") have been modified by MediaTek Inc. All revisions are subject to
 * any receiver's applicable license agreements with MediaTek Inc.
 */
     
#ifndef _BT_MTK_H
#define _BT_MTK_H

#include "../../../../../../system/bt/hci/include/bt_vendor_lib.h"
#include "os_dep.h"


#define HCI_CMD_MAX_SIZE        251


/***********   Structure Definitions   ***********/

typedef enum {
  CMD_SUCCESS,
  CMD_FAIL,
  CMD_PENDING,
} HCI_CMD_STATUS_T;


typedef INT32 (*SETUP_UART_PARAM_T)(UINT32 u4Baud, UINT32 u4FlowControl);

typedef struct {
  UINT32 chip_id;
  UINT32 bt_baud;
  UINT32 host_baud;
  UINT32 flow_ctrl;
  SETUP_UART_PARAM_T host_uart_cback;
  PUCHAR patch_ext_data;
  UINT32 patch_ext_len;
  UINT32 patch_ext_offset;
  PUCHAR patch_data;
  UINT32 patch_len;
  UINT32 patch_offset;
} BT_INIT_VAR_T;

/* Thread control block for Controller initialize */
typedef struct {
  pthread_t worker_thread;
  pthread_mutex_t mutex;
  pthread_mutexattr_t attr;
  pthread_cond_t cond;
  BOOL worker_thread_running;
} BT_INIT_CB_T;

typedef enum {
  BT_HCI_CMD = 0x01,
  BT_ACL,
  BT_SCO,
  BT_HCI_EVENT
} BT_HDR_T;

/***********   Function Declaration   ***********/
void set_callbacks(const bt_vendor_callbacks_t* p_cb);
void clean_callbacks(void);
int init_uart(void);
void close_uart(void);
int mtk_prepare_off(void);
void clean_resource(void);
void vendor_fw_cfg(void);
void vendor_op_lmp_set_mode(void);

#endif
