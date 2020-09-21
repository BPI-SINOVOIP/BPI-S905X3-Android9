/*
 *
 * btusb_lite.h
 *
 *
 *
 * Copyright (C) 2011-2013 Broadcom Corporation.
 *
 *
 *
 * This software is licensed under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation (the "GPL"), and may
 * be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GPL for more details.
 *
 *
 * A copy of the GPL is available at http://www.broadcom.com/licenses/GPLv2.php
 * or by writing to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA
 *
 *
 */
#ifndef BTUSB_LITE_H
#define BTUSB_LITE_H

/* Forward declaration of BTUSB CB */
struct btusb_cb;

#include "bt_target.h"
#include "hcidefs.h"
#include "bd.h"
#include "uipc_msg.h"
#include "btusb_lite_av.h"
#include "btusb_lite_avdt.h"
#include "btusb_lite_l2c.h"
#include "btusb_lite_hci.h"

/*
 * Definitions
 */
/* IPC Length Header Size (16 bits) */
#define BTUSB_LITE_IPC_HDR_LEN_SIZE (sizeof(UINT16))

/* IPC Event Header Size (16 bits) */
#define BTUSB_LITE_IPC_HDR_EVT_SIZE (sizeof(UINT16))

/* IPC Header contains a Length and an Event code */
#define BTUSB_LITE_IPC_HDR_SIZE (BTUSB_LITE_IPC_HDR_LEN_SIZE + BTUSB_LITE_IPC_HDR_EVT_SIZE)

struct btusb_lite_mgt_cb
{
    int opened;
};

struct btusb_lite_stat
{
    unsigned long event_bytes;
    unsigned long event_completed;
};

struct btusb_lite_from_app
{
    BT_HDR *p_rx_msg;
    UINT8 rx_header[BTUSB_LITE_IPC_HDR_SIZE];
    UINT8 rx_header_len;
    UINT16 rx_len;  /* Decoded Rx Payload length */
};

struct btusb_lite_to_app
{
    BUFFER_Q ipc_queue; /* IPC message queue */
    BT_HDR *p_ipc_msg;
    BT_HDR *p_hdr_msg;
    BT_HDR *p_hci_msg;
};

/* Lite Stack mode */
#define BTU_FULL_STACK_ACTIVE       0
#define BTU_LITE_STACK_ACTIVE       1
#define BTU_TRANSPORT_HOLD          2
#define BTU_FULL_TRANSPORT_ACTIVE   3
#define BTU_LITE_TRANSPORT_ACTIVE   4

#define BTU_IPC_CMD_SET_TRANSPORT_STATE 0   /* Set transport state (param=transprt state) */
#define BTU_IPC_CMD_DISABLE_TRANSPORT   1   /* Set transport hardware (param=1 to disable) */

struct btusb_lite_btu_cb
{
    UINT8 transport_state;
    UINT8 transport_disabled;
};

typedef enum
{
    BTPCM_LITE_PCM_CLOSED = 0,
    BTPCM_LITE_PCM_OPENED,
    BTPCM_LITE_PCM_CONFIGURED,
    BTPCM_LITE_PCM_STARTED
} btusb_lite_pcm_state_t;

struct btusb_lite_pcm_ccb
{
    btusb_lite_pcm_state_t state;
    int channel;
    int frequency;
};

struct btusb_lite_encoder_ccb
{
    int channel;
    int opened;
    int encoded_frame_size;
    int pcm_frame_size;
    int header_size;
    int type; /* SBC, SEC, etc. */
    tBTA_AV_AUDIO_CODEC_INFO encoder;
};

struct btusb_lite_av_cb
{
    UINT8 audio_open_cnt;
    UINT16 curr_mtu;
    UINT8 multi_av;
    struct btusb_lite_pcm_ccb pcm;
    struct btusb_lite_encoder_ccb encoder;
    struct btusb_lite_av_scb scb[BTA_AV_NUM_STRS];
    BT_HDR *p_buf_working;
    UINT8 option;
    UINT8 m_pt;
    int header_len;
    int payload_len;
    UINT32 timestamp;
};

/* Main Lite Control Block */
struct btusb_lite_cb
{
    struct proc_dir_entry *p_lite_pde;
    int opened;
    struct btusb_lite_stat stat;
    struct btusb_lite_from_app from_app;
    struct btusb_lite_to_app to_app;
    struct btusb_lite_mgt_cb mgt;   /* Management */
    struct btusb_lite_btu_cb btu; /* BTU */
    struct btusb_lite_l2c_cb l2c; /* L2C */
    struct btusb_lite_av_cb av; /* AV */
    struct btusb_lite_avdt_cb avdt;
};

/*******************************************************************************
 **
 ** Function        btusb_lite_create
 **
 ** Description     Create BTUSB Lite interface
 **
 ** Returns         status (< 0 if error)
 **
 *******************************************************************************/
int btusb_lite_create(struct btusb_cb *p_dev, struct usb_interface *p_interface);

/*******************************************************************************
 **
 ** Function        btusb_lite_delete
 **
 ** Description     Delete BTUSB Lite interface
 **
 ** Returns         none
 **
 *******************************************************************************/
void btusb_lite_delete(struct btusb_cb *p_dev, struct usb_interface *p_interface);

/*******************************************************************************
 **
 ** Function        btusb_lite_stop_all
 **
 ** Description     Stop all sound streams
 **
 ** Returns         void
 **
 *******************************************************************************/
void btusb_lite_stop_all(struct btusb_cb *p_dev);

/*******************************************************************************
 **
 ** Function        btusb_lite_is_hci_over_ipc
 **
 ** Description     Check if HCI is over IPC (Lite Interface).
 **
 ** Returns         int (1 if HCI is over IPC otherwise 0)
 **
 *******************************************************************************/
int btusb_lite_is_hci_over_ipc(struct btusb_cb *p_dev);

#endif /* BTUSB_LITE_H*/

