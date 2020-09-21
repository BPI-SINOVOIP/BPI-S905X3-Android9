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

#include "bt_vendor_lib.h"
#include "bt_mtk.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <cutils/sockets.h>
#include <signal.h>
#include <fcntl.h>

#define VENDOR_LIBRARY_VERSION "700.0.17122701"

//===============        V A R I A B L E S       =======================
//static unsigned int remaining_length = 0; // the remaing data length
static uint8_t data_buffer[1024]; // store the hci event
static uint8_t* remaining_data_buffer = NULL; // pointer to current data position
//static uint8_t* current_pos = NULL;
static void whole_chip_reset(void);
static int fw_dump_started = 0;
static int fw_dump_fp = -1;
static char fw_dump_log_path[64]={0};

#ifndef INVALID_FD
#define INVALID_FD (-1)
#endif

// copy from hci_hal.h
typedef enum {
  DATA_TYPE_COMMAND = 1,
  DATA_TYPE_ACL     = 2,
  DATA_TYPE_SCO     = 3,
  DATA_TYPE_EVENT   = 4
} serial_data_type_t;

#if (defined(MTK_VENDOR_OPCODE) && (MTK_VENDOR_OPCODE == TRUE))
static void mtk_bt_notify_incoming_msg(void* param);
#endif

//===============        P R I V A T E  A P I      =======================

void do_signal_kill(int signum)
{
    if (signum == SIGIO) {
        LOG_DBG("BT VENDOR DRIVER GET KILL SIGNAL\n");
        whole_chip_reset();
    } else {
        LOG_DBG("BT VENDOR DRIVER GET UNKNOWN SIGNAL (%d)\n", signum);
    }
}

#if (defined(MTK_VENDOR_OPCODE) && (MTK_VENDOR_OPCODE == TRUE))
static void _mtk_bt_handle_voice_search_data(const uint8_t *buf, const unsigned int buf_len)
{
#if defined(MTK_LINUX)
#else
    if ( (buf_len == 12 || buf_len == 31) &&
         (buf[2] == 0x08 || buf[2] == 0x1b) && buf[3] == 0x00 &&
         buf[8] == 0x1b && buf[9] == 0x35 && buf[10] == 0x00 )
    {
        //int i;
        //int ret;

        // GATT data pattern for key code (len = 4):
        //          Key Press           : 4b 00 00 0x
        //          Key Release         : 4b 00 00 00
        //          Voice key press     : 4b 00 00 07
        //          Voice key release   : 4b 00 00 00
        // GATT data pattern for voice data :
        //          Frist       : 35 00 04
        //          Second      : 35 00 xx xx xx (total 22Byte)
        static int g_connection_created = FALSE;
        static int g_voice_sockfd = -1;
        static struct sockaddr_un g_voice_sockaddr;
        static socklen_t g_voice_sockaddr_len;
       // static const char* const serverAddr="voice.source.address";
        int err;
#ifdef BT_DRV_PRINT_DBG_LOG
    if ( buf_len == 12 )
        LOG_DBG("hci_event(%d):%02X\n", buf_len, buf[11]);
    else
        LOG_DBG("hci_event(%d):%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\n",
                buf_len,
                buf[11],
                buf[12],
                buf[13],
                buf[14],
                buf[15],
                buf[16],
                buf[17],
                buf[18],
                buf[19],
                buf[20],
                buf[21],
                buf[22],
                buf[23],
                buf[24],
                buf[25],
                buf[26],
                buf[27],
                buf[28],
                buf[29],
                buf[30]);
#endif
        if ( buf_len == 12 && buf[11] == 0x04 && g_connection_created == TRUE && g_voice_sockfd )
        {
            LOG_DBG("close existing socket\n");
            close(g_voice_sockfd);
            g_connection_created = FALSE;
        }

        if ( g_connection_created == FALSE )
        {
            //extern int socket_make_sockaddr_un(const char *name, int ns_id, struct sockaddr_un *p_addr, socklen_t *alen);
            err = 1;//socket_make_sockaddr_un(serverAddr,
                    //                      ANDROID_SOCKET_NAMESPACE_ABSTRACT,
                    //                      &g_voice_sockaddr, &g_voice_sockaddr_len);
            if ( err )
            {
                LOG_ERR("%s : Create socket failed.", __FUNCTION__);
                return;
            }

            g_voice_sockfd = socket(AF_LOCAL, SOCK_DGRAM, 0);
            if (g_voice_sockfd < 0)
            {
                LOG_ERR("%s : Create server socket failed.", __FUNCTION__);
                return;
            }
            g_connection_created = TRUE;
        }

        if ( g_voice_sockfd < 0 )
        {
            LOG_ERR("Create server socket failed.");
            return;
        }

        err = sendto(g_voice_sockfd, buf+11, buf_len-11 , MSG_DONTWAIT , (struct sockaddr*) &g_voice_sockaddr, g_voice_sockaddr_len) ;
        if ( err < 0 )
        {
            static int counter=0;
            counter++;
            if ( counter == 100 )
            {
                LOG_ERR("%s : send FAILED (%s:%d) x 100\n", __FUNCTION__, strerror(errno), errno);
                counter = 0;
            }
        }
    }
#endif
}
#endif

//===============        I N T E R F A C E S      =======================
static int mtk_bt_init(const bt_vendor_callbacks_t* p_cb, unsigned char *local_bdaddr)
{
    LOG_DBG("%s : VENDOR LIBRARY VERSION =%s\n", __FUNCTION__, VENDOR_LIBRARY_VERSION);
    (void)local_bdaddr;
    LOG_TRC();
    set_callbacks(p_cb);
	remaining_data_buffer = data_buffer;
    return 0;
}

static int mtk_bt_op(bt_vendor_opcode_t opcode, void *param)
{
    int ret = 0;
    int oflag;
    int bt_fd;

    switch(opcode)
    {
      case BT_VND_OP_POWER_CTRL:
        LOG_DBG("BT_VND_OP_POWER_CTRL %d\n", *((int*)param));
        break;

      case BT_VND_OP_USERIAL_OPEN:
        LOG_DBG("BT_VND_OP_USERIAL_OPEN\n");		
        ((int*)param)[0] = init_uart();
        bt_fd = ((int *)param)[0];
        if (bt_fd >= 0) {
            fcntl(bt_fd, F_SETOWN, getpid());
            oflag = fcntl(bt_fd, F_GETFL);
            fcntl(bt_fd, F_SETFL, oflag | FASYNC);
            signal(SIGIO, do_signal_kill);
            LOG_DBG("BT VENDOR start wait signal from kernel[%d]...", bt_fd);
        }
#if defined(MTK_BPERF_ENABLE) && (MTK_BPERF_ENABLE == TRUE)
        bperf_init();
#endif
        ret = 1; // CMD/EVT/ACL-In/ACL-Out via the same fd
        break;

      case BT_VND_OP_USERIAL_CLOSE:
        LOG_DBG("BT_VND_OP_USERIAL_CLOSE\n");
        close_uart();
#if defined(MTK_BPERF_ENABLE) && (MTK_BPERF_ENABLE == TRUE)
        bperf_uninit();
#endif
        break;

      case BT_VND_OP_FW_CFG:
        LOG_DBG("BT_VND_OP_FW_CFG\n");
        vendor_fw_cfg();
        break;

      case BT_VND_OP_SCO_CFG:
        LOG_DBG("BT_VND_OP_SCO_CFG\n");
        break;

      case BT_VND_OP_GET_LPM_IDLE_TIMEOUT:
        LOG_DBG("BT_VND_OP_GET_LPM_IDLE_TIMEOUT\n");
        *((uint32_t*)param) = 5000; //ms
        break;

      case BT_VND_OP_LPM_SET_MODE:
        LOG_DBG("BT_VND_OP_LPM_SET_MODE %d\n", *((uint8_t*)param));
        vendor_op_lmp_set_mode();
        break;

      case BT_VND_OP_LPM_WAKE_SET_STATE:
        break;

      case BT_VND_OP_EPILOG:
        LOG_DBG("BT_VND_OP_EPILOG\n");
        ret = mtk_prepare_off();
        break;
#if (defined(MTK_VENDOR_OPCODE) && (MTK_VENDOR_OPCODE == TRUE))
      case BT_VND_OP_HANDLE_VENDOR_MESSAGE:
        mtk_bt_notify_incoming_msg(param);
        break;
#endif
      default:
        LOG_DBG("Unknown operation %d\n", opcode);
        break;
    }

    return ret;
}

static void mtk_bt_cleanup()
{
    fw_dump_started = 0;
    if ( fw_dump_fp >0)
    {
        LOG_DBG("Close fw dump file : %s", fw_dump_log_path);
        close(fw_dump_fp);
    }

    LOG_TRC();
	remaining_data_buffer = NULL;
    clean_resource();
    clean_callbacks();
    return;
}

#if (defined(MTK_VENDOR_OPCODE) && (MTK_VENDOR_OPCODE == TRUE))
static void mtk_bt_handle_cmd(const uint8_t *buf, const unsigned int buf_len)
{
    (void)buf;
    (void)buf_len;
#if defined(MTK_BPERF_ENABLE) && (MTK_BPERF_ENABLE == TRUE)
    bperf_notify_cmd(buf, buf_len);
#endif
    return;
}
#endif

#if (defined(MTK_VENDOR_OPCODE) && (MTK_VENDOR_OPCODE == TRUE))
static void mtk_bt_handle_acl(const uint8_t *buf, const unsigned int buf_len)
{
    (void)buf;
    (void)buf_len;
#if defined(MTK_LINUX)
#else
    /* HT RC Voice Search (2541) */
    if ( (buf_len == 12 || buf_len == 31) &&
         (buf[2] == 0x08 || buf[2] == 0x1b) && buf[3] == 0x00 &&
         buf[8] == 0x1b && buf[9] == 0x35 && buf[10] == 0x00 )
    {
		_mtk_bt_handle_voice_search_data(buf, buf_len);
    }
#endif

#if defined(MTK_BPERF_ENABLE) && (MTK_BPERF_ENABLE == TRUE)
    bperf_notify_data(buf, buf_len);
#endif
    return;
}
#endif

#if (defined(MTK_VENDOR_OPCODE) && (MTK_VENDOR_OPCODE == TRUE))
static void mtk_bt_handle_event(const uint8_t *buf, const unsigned int buf_len)
{
    (void)buf;
    (void)buf_len;
#if defined(MTK_BPERF_ENABLE) && (MTK_BPERF_ENABLE == TRUE)
    bperf_notify_event(buf, buf_len);
#endif
    return;
}
#endif
#if (defined(MTK_VENDOR_OPCODE) && (MTK_VENDOR_OPCODE == TRUE))
static void mtk_bt_notify_incoming_msg(void* param)
{
    bt_vendor_op_handle_vendor_msg_t* incoming_msg = (bt_vendor_op_handle_vendor_msg_t*)param;
    const uint8_t type = incoming_msg->type;
    const uint16_t buf_len = incoming_msg->len;
    const uint8_t* buf = incoming_msg->data;
    unsigned int length = buf_len;

    uint8_t* buffer = (uint8_t*)buf;
    BT_HDR_T hdr = 0x00;
    unsigned int size = 0;

#ifdef BT_DRV_PRINT_DBG_LOG
    LOG_DBG("%s=(%d=%s)=========(len=%d) %02X %02X %02X %02X %02X %02X %02X %02X\n",
                                                                    __FUNCTION__,
                                                                    type,
                                                                    type==DATA_TYPE_COMMAND?"CMD":
                                                                    type==DATA_TYPE_ACL?"ACL":
                                                                    type==DATA_TYPE_SCO?"SCO":
                                                                    type==DATA_TYPE_EVENT?"EVENT":"UNKNOWN",
                                                                    buf_len,
                                                                    buf[0],
                                                                    buf[1],
                                                                    buf[2],
                                                                    buf[3],
                                                                    buf[4],
                                                                    buf[5],
                                                                    buf[6],
                                                                    buf[7]);

#endif
    unsigned char reset_event[] = {0x04, 0xFF, 0x04, 0x00, 0x01, 0xFF, 0xFF};

    switch (type)
    {
        case DATA_TYPE_COMMAND:
            mtk_bt_handle_cmd(buf, buf_len);
            break;

        case DATA_TYPE_ACL:
            mtk_bt_handle_acl(buf, buf_len);
            break;

        case DATA_TYPE_EVENT:
            // handle reset condition
            if ( type == DATA_TYPE_EVENT && buf_len == 6 &&
                buf[0] == 0xFF && buf[1] == 0x04 && buf[2] == 0x00 &&
                buf[3] == 0x01 && buf[4] == 0xFF && buf[5] == 0xFF )
            {
                LOG_DBG("%s : kill self to trigger reset", __FUNCTION__);
                whole_chip_reset();
            }
            else
            {
                mtk_bt_handle_event(buf, buf_len);
            }
            break;

        case DATA_TYPE_SCO:
        default:
            LOG_ERR("Not handled event type %d !", type);
            break;
    }
    return;
}
#endif

#if defined(MTK_LINUX)
EXPORT_SYMBOL const bt_vendor_interface_t BLUETOOTH_VENDOR_LIB_INTERFACE = {
#else
const bt_vendor_interface_t BLUETOOTH_VENDOR_LIB_INTERFACE = {
#endif
    sizeof(bt_vendor_interface_t),
    mtk_bt_init,
    mtk_bt_op,
    mtk_bt_cleanup,
};

static void whole_chip_reset(void)
{
    LOG_ERR("Restarting BT process");
    usleep(10000); /* 10 milliseconds */
    /* Killing the process to force a restart as part of fault tolerance */
    kill(getpid(), SIGKILL);
}
