/******************************************************************************
 *
 *  Copyright (C) 2009-2018 Realtek Corporation.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

/******************************************************************************
*
*	Module Name:
*		rtk_parse.c
*
*	Abstract:
*		Contains wifi-bt coex functions implemented by bluedroid stack
*
*	Major Change History:
*	      When             Who       What
*	 	---------------------------------------------------------------
*	    2015-12-15      lamparten   modified
*	    2014-10-23       kyle_xu    modified
*	Notes:
*		  This is designed for wifi-bt Coex in Android 6.0.
*
******************************************************************************/
#define LOG_TAG "rtk_parse"
#define RTKBT_RELEASE_NAME "20190311_BT_ANDROID_9.0"

#include <utils/Log.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/prctl.h>
#include <linux/types.h>
#include <linux/netlink.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <dirent.h>
#include <signal.h>
#include <poll.h>

#include "bt_list.h"
#include "bt_hci_bdroid.h"
#include "rtk_parse.h"
#include <sys/syscall.h>

#define RTK_COEX_VERSION "3.0"

char invite_req[] = "INVITE_REQ";
char invite_rsp[] = "INVITE_RSP";
char attend_req[] = "ATTEND_REQ";
char attend_ack[] = "ATTEND_ACK";
char wifi_leave[] = "WIFI_LEAVE";
char leave_ack[] =  "LEAVE_ACK";
char bt_leave[] =   "BT_LEAVE";

#define CONNECT_PORT        30001
#define CONNECT_PORT_WIFI   30000
//#define NETLINK_USER        31
#define MAX_PAYLOAD         255 /* maximum payload size*/

//L2CAP TYPE
#define L2CAP_CONNECTION_REQ        0x02
#define L2CAP_CONNECTION_RSP        0x03
#define L2CAP_DISCONNECTION_REQ     0x06
#define L2CAP_DISCONNECTION_RSP     0x07

#define TIMER_A2DP_PACKET_COUNT     (SIGRTMAX -5)
#define TIMER_PAN_PACKET_COUNT      (SIGRTMAX -6)
#define TIMER_HOGP_PACKET_COUNT     (SIGRTMAX -7)
#define TIMER_POLLING               (SIGRTMAX -8)

#define PAN_PACKET_COUNT                5
#define PACKET_COUNT_TIOMEOUT_VALUE     1000//ms

//vendor cmd to fw
#define HCI_VENDOR_ENABLE_PROFILE_REPORT_COMMAND        (0x0018 | HCI_GRP_VENDOR_SPECIFIC)
#define HCI_VENDOR_SET_PROFILE_REPORT_COMMAND           (0x0019 | HCI_GRP_VENDOR_SPECIFIC)
#define HCI_VENDOR_MAILBOX_CMD                          (0x008F | HCI_GRP_VENDOR_SPECIFIC)

#define HCI_VENDOR_ADD_BITPOOL_FW                       (0x0051 | HCI_GRP_VENDOR_SPECIFIC)

//subcmd to fw for HCI_VENDOR_MAILBOX_CMD
#define HCI_VENDOR_SUB_CMD_WIFI_CHANNEL_AND_BANDWIDTH_CMD   0x11
#define HCI_VENDOR_SUB_CMD_WIFI_FORCE_TX_POWER_CMD          0x17
#define HCI_VENDOR_SUB_CMD_BT_ENABLE_IGNORE_WLAN_ACT_CMD    0x1B
#define HCI_VENDOR_SUB_CMD_BT_REPORT_CONN_SCO_INQ_INFO      0x23
#define HCI_VENDOR_SUB_CMD_BT_AUTO_REPORT_STATUS_INFO       0x27
#define HCI_VENDOR_SUB_CMD_BT_AUTO_REPORT_ENABLE            0x28
#define HCI_VENDOR_SUB_CMD_BT_SET_TXRETRY_REPORT_PARAM      0x29
#define HCI_VENDOR_SUB_CMD_BT_SET_PTATABLE                  0x2A
#define HCI_VENDOR_SUB_CMD_SET_BT_PSD_MODE                  0x31
#define HCI_VENDOR_SUB_CMD_SET_BT_LNA_CONSTRAINT            0x32
#define HCI_VENDOR_SUB_CMD_GET_AFH_MAP_L                    0x40
#define HCI_VENDOR_SUB_CMD_GET_AFH_MAP_M                    0x41
#define HCI_VENDOR_SUB_CMD_GET_AFH_MAP_H                    0x42
#define HCI_VENDOR_SUB_CMD_RD_REG_REQ                       0x43
#define HCI_VENDOR_SUB_CMD_WR_REG_REQ                       0x44

//sub event from fw
#define HCI_VENDOR_PTA_REPORT_EVENT         0x24
#define    HCI_VENDOR_PTA_AUTO_REPORT_EVENT    0x25

//vendor cmd to wifi driver
#define HCI_OP_HCI_EXTENSION_VERSION_NOTIFY (0x0100 | HCI_GRP_VENDOR_SPECIFIC)
#define HCI_OP_BT_OPERATION_NOTIFY          (0x0102 | HCI_GRP_VENDOR_SPECIFIC)
#define HCI_OP_HCI_BT_INFO_NOTIFY           (0x0106 | HCI_GRP_VENDOR_SPECIFIC)
#define HCI_OP_HCI_BT_COEX_NOTIFY           (0x0107 | HCI_GRP_VENDOR_SPECIFIC)
#define HCI_OP_HCI_BT_PATCH_VER_NOTIFY      (0x0108 | HCI_GRP_VENDOR_SPECIFIC)
#define HCI_OP_HCI_BT_AFH_MAP_NOTIFY        (0x0109 | HCI_GRP_VENDOR_SPECIFIC)
#define HCI_OP_HCI_BT_REGISTER_VALUE_NOTIFY (0x010a | HCI_GRP_VENDOR_SPECIFIC)

//bt operation to notify for HCI_OP_BT_OPERATION_NOTIFY
#define BT_OPCODE_NONE                  0
#define BT_OPCODE_INQUIRY_START         1
#define BT_OPCODE_INQUIRY_END           2
#define BT_OPCODE_PAGE_START            3
#define BT_OPCODE_PAGE_SUCCESS_END      4
#define BT_OPCODE_PAGE_UNSUCCESS_END    5
#define BT_OPCODE_PAIR_START            6
#define BT_OPCODE_PAIR_END              7
#define BT_OPCODE_ENABLE_BT             8
#define BT_OPCODE_DISABLE_BT            9

//bt info reason to wifi for HCI_OP_HCI_BT_INFO_NOTIFY
#define HOST_RESPONSE                   0 //Host response when receive the BT Info Control Event
#define POLLING_RESPONSE                1 //The BT Info response for polling by BT firmware.
#define AUTO_REPORT                     2 //BT auto report by BT firmware.
#define STACK_REPORT_WHILE_DEVICE_D2    3 //Stack report when BT firmware is under power save state(ex:D2)

// vendor event from wifi
#define RTK_HS_EXTENSION_EVENT_WIFI_SCAN            0x01
#define RTK_HS_EXTENSION_EVENT_RADIO_STATUS_NOTIFY  0x02
#define RTK_HS_EXTENSION_EVENT_HCI_BT_INFO_CONTROL  0x03
#define RTK_HS_EXTENSION_EVENT_HCI_BT_COEX_CONTROL  0x04

//op code from wifi for RTK_HS_EXTENSION_EVENT_HCI_BT_COEX_CONTROL
#define BT_PATCH_VERSION_QUERY      0x00
#define IGNORE_WLAN_ACTIVE_CONTROL  0x01
#define LNA_CONSTRAIN_CONTROL       0x02
#define BT_POWER_DECREASE_CONTROL   0x03
#define BT_PSD_MODE_CONTROL         0x04
#define WIFI_BW_CHNL_NOTIFY         0x05
#define QUERY_BT_AFH_MAP            0x06
#define BT_REGISTER_ACCESS          0x07

#define HCI_EXTENSION_VERSION   0x0004

#define HCI_CMD_PREAMBLE_SIZE   3

#define PSM_SDP     0x0001
#define PSM_RFCOMM  0x0003
#define PSM_PAN     0x000F
#define PSM_HID     0x0011
#define PSM_HID_INT 0x0013
#define PSM_AVCTP   0x0017
#define PSM_AVDTP   0x0019
#define PSM_FTP     0x1001
#define PSM_BIP     0x1003
#define PSM_OPP     0x1015
//--Add more if needed--//

enum {
    profile_sco = 0,
    profile_hid = 1,
    profile_a2dp = 2,
    profile_pan = 3,
    profile_hid_interval = 4,
    profile_hogp = 5,
    profile_voice = 6,
    profile_sink = 7,
    profile_max = 8
};

typedef struct RTK_COEX_INFO {
    RT_LIST_ENTRY   list;
    HC_BT_HDR  *    p_buf;
    uint16_t        opcode;
    tINT_CMD_CBACK  p_cback;
}tRTK_COEX_INFO;

//profile info data
typedef struct RTK_PROF_INFO {
    RT_LIST_ENTRY   list;
    uint16_t        handle;
    uint16_t        psm;
    uint16_t        dcid;
    uint16_t        scid;
    uint8_t         profile_index;
}tRTK_PROF_INFO;

//profile info for each connection
typedef struct RTK_CONN_PROF {
    RT_LIST_ENTRY list;
    uint16_t handle;
    uint8_t type;                   //0:l2cap, 1:sco/esco, 2:le
    uint8_t profile_bitmap;         //0:SCO, 1:HID, 2:A2DP, 3:FTP/PAN/OPP, 4: HID_interval, 5:HOGP, 6:VOICE
    int8_t  profile_refcount[8];    //0:SCO, 1:HID, 2:A2DP, 3:FTP/PAN/OPP, 4:TBD, 5:HOGP, 6:VOICE
}tRTK_CONN_PROF;

//profile info for all
typedef struct RTK_PROF {
    RT_LIST_HEAD    conn_hash;      //hash for connections
    RT_LIST_HEAD    profile_list;   //hash for profile info
    RT_LIST_HEAD    coex_list;
    tINT_CMD_CBACK  current_cback;
    pthread_mutex_t profile_mutex;
    pthread_mutex_t coex_mutex;
    pthread_mutex_t btwifi_mutex;
    pthread_t thread_monitor;
    pthread_t thread_data;
    timer_t  timer_a2dp_packet_count;
    timer_t  timer_pan_packet_count;
    timer_t  timer_hogp_packet_count;
    timer_t  timer_polling;
    //struct sockaddr_nl src_addr;    //for netlink
    struct sockaddr_in server_addr; //server addr for kernel socket
    struct sockaddr_in client_addr; //client addr  for kernel socket
    uint32_t a2dp_packet_count;
    uint32_t pan_packet_count;
    uint32_t hogp_packet_count;
    uint32_t voice_packet_count;
    uint8_t  profile_bitmap;
    uint8_t  profile_status;
    int8_t   profile_refcount[8];
    uint8_t  ispairing;
    uint8_t  isinquirying;
    uint8_t  ispaging;
    uint8_t  wifi_state;
    uint8_t  autoreport;
    uint8_t  polling_enable;
    uint8_t  polling_interval;
    volatile uint8_t coex_recv_thread_running;
    //int32_t   nlsocket;
    int32_t  btcoex_chr;
    int32_t  udpsocket;
    uint8_t  piconet_id;
    uint8_t  mode;
    uint8_t  afh_map[10];
    uint16_t hci_reversion;
    uint16_t lmp_subversion;
    uint8_t  wifi_on;
    uint8_t  bt_on;
    //uint8_t  le_profile_index;
}tRTK_PROF;

typedef struct HCI_RETURN_PARAMETER_MAILBOX_REGISTER {
    uint8_t  type;
    uint32_t offset;
    uint32_t value;
}tHCI_RETURN_PARAMETER_MAILBOX_REGISTER;

typedef struct HCI_EVENT_BT_INFO_CONTROL {
    uint8_t     polling_enable;
    uint8_t     polling_time;
    uint8_t     autoreport_enable;
}tHCI_EVENT_BT_INFO_CONTROL;

tRTK_PROF rtk_prof;
volatile int poweroff_allowed = 0;
uint8_t coex_log_enable = 0;
static volatile bool coex_cmd_send = false;

#define BIT(_I)                         (uint16_t)(1<<(_I))
#define is_profile_connected(profile)   ((rtk_prof.profile_bitmap & BIT(profile)) >0)
#define is_profile_busy(profile)        ((rtk_prof.profile_status & BIT(profile)) >0)

static void timeout_handler(int signo, siginfo_t * info, void *context);
static void notify_func(union sigval sig);

static int coex_msg_send(char *tx_msg, int msg_size);
static int coex_msg_recv(uint8_t *recv_msg, uint8_t *msg_size);

#ifndef RTK_PARSE_LOG_BUF_SIZE
#define RTK_PARSE_LOG_BUF_SIZE  1024
#endif
#define RTK_PARSE_LOG_MAX_SIZE  (RTK_PARSE_LOG_BUF_SIZE - 12)

#define LOGI0(t,s) __android_log_write(ANDROID_LOG_INFO, t, s)
static void RtkLogMsg(const char *fmt_str, ...)
{
    static char buffer[RTK_PARSE_LOG_BUF_SIZE];
    if(coex_log_enable)
    {
        va_list ap;
        va_start(ap, fmt_str);
        vsnprintf(&buffer[0], RTK_PARSE_LOG_MAX_SIZE, fmt_str, ap);
        va_end(ap);

        LOGI0("rtk_parse: ", buffer);
    }
    else
    {
        return;
    }
}

static const char sample_freqs[4][8] = {
        "16", "32", "44.1", "48"
};

static const uint8_t sbc_blocks[4] = { 4, 8, 12, 16 };

static const char chan_modes[4][16] = {
        "MONO", "DUAL_CHANNEL", "STEREO", "JOINT_STEREO"
};

static const char alloc_methods[2][12] = {
        "LOUDNESS", "SNR"
};

static const uint8_t subbands[2] = { 4, 8 };

void print_sbc_header(struct sbc_frame_hdr *hdr)
{
    RtkLogMsg("syncword: %02x", hdr->syncword);
    RtkLogMsg("freq %skHz", sample_freqs[hdr->sampling_frequency]);
    RtkLogMsg("blocks %u", sbc_blocks[hdr->blocks]);
    RtkLogMsg("channel mode %s", chan_modes[hdr->channel_mode]);
    RtkLogMsg("allocation method %s", alloc_methods[hdr->allocation_method]);
    RtkLogMsg("subbands %u", subbands[hdr->subbands]);
}

static timer_t OsAllocateTimer(int signo)
{
    struct sigevent sigev;
    timer_t timerid = (timer_t)-1;

    // Create the POSIX timer to generate signo
    //sigev.sigev_notify = SIGEV_THREAD_ID;
    //sigev.sigev_notify_thread_id = syscall(__NR_gettid);
    //sigev.sigev_signo = signo;
    //sigev.sigev_value.sival_ptr = &timerid;

    memset(&sigev, 0, sizeof(sigev));
    sigev.sigev_notify = SIGEV_THREAD;
    sigev.sigev_notify_function = notify_func;
    sigev.sigev_value.sival_int = signo;

    //ALOGE("OsAllocateTimer rtk_parse sigev.sigev_notify_thread_id = syscall(__NR_gettid)!");

    //Create the Timer using timer_create signal
    if (timer_create(CLOCK_REALTIME, &sigev, &timerid) == 0)
    {
        return timerid;
    }
    else
    {
        ALOGE("timer_create error!");
        return (timer_t)-1;
    }
}

static int OsFreeTimer(timer_t timerid)
{
    int ret = 0;
    ret = timer_delete(timerid);
    if(ret != 0)
        ALOGE("timer_delete fail with errno(%d)", errno);

    return ret;
}

static int OsStartTimer(timer_t timerid, int msec, int mode)
{
    struct itimerspec itval;

    itval.it_value.tv_sec = msec / 1000;
    itval.it_value.tv_nsec = (long)(msec % 1000) * (1000000L);

    if (mode == 1)
    {
        itval.it_interval.tv_sec  = itval.it_value.tv_sec;
        itval.it_interval.tv_nsec = itval.it_value.tv_nsec;
    }
    else
    {
        itval.it_interval.tv_sec = 0;
        itval.it_interval.tv_nsec = 0;
    }

    //Set the Timer when to expire through timer_settime
    if (timer_settime(timerid, 0, &itval, NULL) != 0)
    {
        ALOGE("time_settime error!");
        return -1;
    }

    return 0;
}

static int OsStopTimer(timer_t timerid)
{
    return OsStartTimer(timerid, 0, 0);
}

int alloc_polling_timer()
{
/*
    struct sigaction sigact;

    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = SA_SIGINFO;

    //register the Signal Handler
    sigact.sa_sigaction = timeout_handler;

    // Set up sigaction to catch signal first timer
    if (sigaction(TIMER_POLLING, &sigact, NULL) == -1)
    {
        ALOGE("alloc_polling_timer, sigaction failed");
        return -1;
    }
*/
    // Create and set the timer when to expire
    rtk_prof.timer_polling= OsAllocateTimer(TIMER_POLLING);
    RtkLogMsg("alloc polling timer");

    return 0;
}

int free_polling_timer()
{
    return OsFreeTimer(rtk_prof.timer_polling);
}

int stop_polling_timer()
{
    RtkLogMsg("stop polling timer");
    return OsStopTimer(rtk_prof.timer_polling);
}

int start_polling_timer(int value)
{
    RtkLogMsg("start polling timer");
    return OsStartTimer(rtk_prof.timer_polling, value, 1);
}

int alloc_hogp_packet_count_timer()
{
/*
    struct sigaction sigact;

    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = SA_SIGINFO;

    //register the Signal Handler
    sigact.sa_sigaction = timeout_handler;

    // Set up sigaction to catch signal first timer
    if (sigaction(TIMER_HOGP_PACKET_COUNT, &sigact, NULL) == -1)
    {
        ALOGE("alloc_hogp_packet_count_timer, sigaction failed");
        return -1;
    }
*/
    // Create and set the timer when to expire
    rtk_prof.timer_hogp_packet_count= OsAllocateTimer(TIMER_HOGP_PACKET_COUNT);
    RtkLogMsg("alloc hogp packet");

    return 0;
}

int free_hogp_packet_count_timer()
{
    return OsFreeTimer(rtk_prof.timer_hogp_packet_count);
}

int stop_hogp_packet_count_timer()
{
    RtkLogMsg("stop hogp packet");
    return OsStopTimer(rtk_prof.timer_hogp_packet_count);
}

int start_hogp_packet_count_timer()
{
    RtkLogMsg("start hogp packet");
    return OsStartTimer(rtk_prof.timer_hogp_packet_count, PACKET_COUNT_TIOMEOUT_VALUE, 1);
}

int alloc_a2dp_packet_count_timer()
{
/*
    struct sigaction sigact;

    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = SA_SIGINFO;

    //register the Signal Handler
    sigact.sa_sigaction = timeout_handler;

    // Set up sigaction to catch signal first timer
    if (sigaction(TIMER_A2DP_PACKET_COUNT, &sigact, NULL) == -1)
    {
        ALOGE("alloc_a2dp_packet_count_timer, sigaction failed");
        return -1;
    }
*/
    // Create and set the timer when to expire
    rtk_prof.timer_a2dp_packet_count= OsAllocateTimer(TIMER_A2DP_PACKET_COUNT);
    RtkLogMsg("alloc a2dp packet");

    return 0;
}

int free_a2dp_packet_count_timer()
{
    return OsFreeTimer(rtk_prof.timer_a2dp_packet_count);
}

int stop_a2dp_packet_count_timer()
{
    RtkLogMsg("stop a2dp packet");
    return OsStopTimer(rtk_prof.timer_a2dp_packet_count);
}

int start_a2dp_packet_count_timer()
{
    RtkLogMsg("start a2dp packet");
    return OsStartTimer(rtk_prof.timer_a2dp_packet_count, PACKET_COUNT_TIOMEOUT_VALUE, 1);
}

int alloc_pan_packet_count_timer()
{
/*
    struct sigaction sigact;

    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = SA_SIGINFO;

    //register the Signal Handler
    sigact.sa_sigaction = timeout_handler;

    // Set up sigaction to catch signal first timer
    if (sigaction(TIMER_PAN_PACKET_COUNT, &sigact, NULL) == -1)
    {
        ALOGE("alloc_pan_packet_count_timer, sigaction failed");
        return -1;
    }
*/
    // Create and set the timer when to expire
    rtk_prof.timer_pan_packet_count= OsAllocateTimer(TIMER_PAN_PACKET_COUNT);

    RtkLogMsg("alloc pan packet");
    return 0;
}

int free_pan_packet_count_timer()
{
    return OsFreeTimer(rtk_prof.timer_pan_packet_count);
}

int stop_pan_packet_count_timer()
{
    RtkLogMsg("stop pan packet");
    return OsStopTimer(rtk_prof.timer_pan_packet_count);
}

int start_pan_packet_count_timer()
{
    RtkLogMsg("start pan packet");
    return OsStartTimer(rtk_prof.timer_pan_packet_count, PACKET_COUNT_TIOMEOUT_VALUE, 1);
}

static int8_t psm_to_profile_index(uint16_t psm)
{
    switch (psm) {
        case PSM_AVCTP:
        case PSM_SDP:
            return -1;   //ignore

        case PSM_HID:
        case PSM_HID_INT:
            return profile_hid;

        case PSM_AVDTP:
            return profile_a2dp;

        case PSM_PAN:
        case PSM_OPP:
        case PSM_FTP:
        case PSM_BIP:
        case PSM_RFCOMM:
            return profile_pan;

        default:
            return profile_pan;
    }
}

tRTK_CONN_PROF* find_connection_by_handle(tRTK_PROF* h5, uint16_t handle)
{
    RT_LIST_HEAD* head = &h5->conn_hash;
    RT_LIST_ENTRY* iter = NULL, *temp = NULL;
    tRTK_CONN_PROF* desc = NULL;

    LIST_FOR_EACH_SAFELY(iter, temp, head)
    {
        desc = LIST_ENTRY(iter, tRTK_CONN_PROF, list);
        if ((handle & 0xEFF) == desc->handle )  //only last 12 bit are meanful for hci handle
        {
            return desc;
        }
    }
    return NULL;
}

tRTK_CONN_PROF* allocate_connection_by_handle(uint16_t handle)
{
    tRTK_CONN_PROF * phci_conn = NULL;
    phci_conn = malloc(sizeof(tRTK_CONN_PROF));
    if(phci_conn)
        phci_conn->handle = handle;

    return phci_conn;
}

void init_connection_hash(tRTK_PROF* h5)
{
    RT_LIST_HEAD* head = &h5->conn_hash;
    ListInitializeHeader(head);
}

void add_connection_to_hash(tRTK_PROF* h5, tRTK_CONN_PROF* desc)
{
    RT_LIST_HEAD* head = &h5->conn_hash;
    ListAddToTail(&desc->list, head);
}

void delete_connection_from_hash(tRTK_CONN_PROF* desc)
{
    if (desc)
    {
        ListDeleteNode(&desc->list);
        free(desc);
    }
}

void flush_connection_hash(tRTK_PROF* h5)
{
    RT_LIST_HEAD* head = &h5->conn_hash;
    RT_LIST_ENTRY* iter = NULL, *temp = NULL;
    tRTK_CONN_PROF* desc = NULL;

    LIST_FOR_EACH_SAFELY(iter, temp, head)
    {
        desc = LIST_ENTRY(iter, tRTK_CONN_PROF, list);
        if (desc)
        {
            ListDeleteNode(&desc->list);
            free(desc);
        }
    }
    //ListInitializeHeader(head);
}

void init_profile_hash(tRTK_PROF* h5)
{
    RT_LIST_HEAD* head = &h5->profile_list;
    ListInitializeHeader(head);
}

uint8_t list_allocate_add(uint16_t handle, uint16_t psm, int8_t profile_index, uint16_t dcid, uint16_t scid)
{
    tRTK_PROF_INFO* pprof_info = NULL;

    if(profile_index < 0)
    {
        ALOGE("PSM(0x%x) do not need parse", psm);
        return FALSE;
    }

    pprof_info = malloc(sizeof(tRTK_PROF_INFO));
    if (NULL == pprof_info)
    {
        ALOGE("list_allocate_add: allocate error");
        return FALSE;
    }

    pprof_info->handle = handle;
    pprof_info->psm = psm;
    pprof_info->scid = scid;
    pprof_info->dcid = dcid;
    pprof_info->profile_index = profile_index;

    ListAddToTail(&(pprof_info->list), &(rtk_prof.profile_list));

    return TRUE;
}

void delete_profile_from_hash(tRTK_PROF_INFO* desc)
{
    RtkLogMsg("delete profile for handle: %x, psm:%x, dcid:%x, scid:%x", desc->handle, desc->psm, desc->dcid, desc->scid);
    if (desc)
    {
        ListDeleteNode(&desc->list);
        free(desc);
        desc = NULL;
    }
}

void flush_profile_hash(tRTK_PROF* h5)
{
    RT_LIST_HEAD* head = &h5->profile_list;
    RT_LIST_ENTRY* iter = NULL, *temp = NULL;
    tRTK_PROF_INFO* desc = NULL;

    pthread_mutex_lock(&rtk_prof.profile_mutex);
    LIST_FOR_EACH_SAFELY(iter, temp, head)
    {
        desc = LIST_ENTRY(iter, tRTK_PROF_INFO, list);
        delete_profile_from_hash(desc);
    }
    //ListInitializeHeader(head);
    pthread_mutex_unlock(&rtk_prof.profile_mutex);
}

tRTK_PROF_INFO* find_profile_by_handle_scid(tRTK_PROF* h5, uint16_t handle, uint16_t scid)
{
    RT_LIST_HEAD* head = &h5->profile_list;
    RT_LIST_ENTRY* iter = NULL, *temp = NULL;
    tRTK_PROF_INFO* desc = NULL;

    LIST_FOR_EACH_SAFELY(iter, temp, head)
    {
        desc = LIST_ENTRY(iter, tRTK_PROF_INFO, list);
        if (((handle & 0xFFF) == desc->handle ) && (scid == desc->scid))
        {
            return desc;
        }
    }
    return NULL;
}

tRTK_PROF_INFO* find_profile_by_handle_dcid(tRTK_PROF* h5, uint16_t handle, uint16_t dcid)
{
    RT_LIST_HEAD* head = &h5->profile_list;
    RT_LIST_ENTRY* iter = NULL, *temp = NULL;
    tRTK_PROF_INFO* desc = NULL;

    LIST_FOR_EACH_SAFELY(iter, temp, head)
    {
        desc = LIST_ENTRY(iter, tRTK_PROF_INFO, list);
        if (((handle & 0xFFF) == desc->handle ) && (dcid == desc->dcid))
        {
            return desc;
        }
    }
    return NULL;
}

tRTK_PROF_INFO* find_profile_by_handle_dcid_scid(tRTK_PROF* h5, uint16_t handle, uint16_t dcid, uint16_t scid)
{
    RT_LIST_HEAD* head = &h5->profile_list;
    RT_LIST_ENTRY* iter = NULL, *temp = NULL;
    tRTK_PROF_INFO* desc = NULL;

    LIST_FOR_EACH_SAFELY(iter, temp, head)
    {
        desc = LIST_ENTRY(iter, tRTK_PROF_INFO, list);
        if (((handle & 0xFFF) == desc->handle ) && (dcid == desc->dcid) && (scid == desc->scid))
        {
            return desc;
        }
    }
    return NULL;
}

void init_coex_hash(tRTK_PROF* h5)
{
    RT_LIST_HEAD* head = &h5->coex_list;
    ListInitializeHeader(head);
}

void delete_coex_from_hash(tRTK_COEX_INFO* desc)
{
    if (desc)
    {
        ListDeleteNode(&desc->list);
        free(desc);
        desc = NULL;
    }
}

void flush_coex_hash(tRTK_PROF* h5)
{
    RT_LIST_HEAD* head = &h5->coex_list;
    RT_LIST_ENTRY* iter = NULL, *temp = NULL;
    tRTK_COEX_INFO* desc = NULL;

    pthread_mutex_lock(&rtk_prof.coex_mutex);
    LIST_FOR_EACH_SAFELY(iter, temp, head)
    {
        desc = LIST_ENTRY(iter, tRTK_COEX_INFO, list);
        delete_coex_from_hash(desc);
    }
    //ListInitializeHeader(head);
    pthread_mutex_unlock(&rtk_prof.coex_mutex);
}

static void rtk_cmd_complete_cback(void *p_mem)
{
    if(p_mem)
        bt_vendor_cbacks->dealloc(p_mem);
    pthread_mutex_lock(&rtk_prof.coex_mutex);
    RT_LIST_ENTRY* iter = ListGetTop(&(rtk_prof.coex_list));
    tRTK_COEX_INFO* desc = NULL;
    if (iter) {
        desc = LIST_ENTRY(iter, tRTK_COEX_INFO, list);
        if (desc)
        {
            ListDeleteNode(&desc->list);
        }
    }
    else {
        coex_cmd_send = false;
    }
    pthread_mutex_unlock(&rtk_prof.coex_mutex);

    if(rtk_prof.current_cback) {
        rtk_prof.current_cback(p_mem);
        rtk_prof.current_cback = NULL;
    }

    if(desc) {
        ALOGE("%s, transmit_command Opcode:%x",__func__, desc->opcode);
        rtk_prof.current_cback = desc->p_cback;
        bt_vendor_cbacks->xmit_cb(desc->opcode, desc->p_buf, rtk_cmd_complete_cback);
    }

    free(desc);
    return;
}

void rtk_vendor_cmd_to_fw(uint16_t opcode, uint8_t parameter_len, uint8_t* parameter, tINT_CMD_CBACK p_cback)
{
    HC_BT_HDR  *p_buf = NULL;

    if(!rtk_prof.bt_on)
        return;

    if(bt_vendor_cbacks)
        p_buf = (HC_BT_HDR *) bt_vendor_cbacks->alloc(BT_HC_HDR_SIZE + HCI_CMD_PREAMBLE_SIZE + parameter_len);

    if(NULL == p_buf)
    {
        ALOGE("rtk_vendor_cmd_to_fw: HC_BT_HDR alloc error");
        return;
    }
    memset(p_buf, 0, (BT_HC_HDR_SIZE + HCI_CMD_PREAMBLE_SIZE + parameter_len));
    p_buf->event = MSG_STACK_TO_HC_HCI_CMD;
    p_buf->offset = 0;
    p_buf->len = HCI_CMD_PREAMBLE_SIZE + parameter_len;
    p_buf->layer_specific = 0;

    uint8_t *p = (uint8_t *) (p_buf + 1);
    UINT16_TO_STREAM(p, opcode);
    *p++ = parameter_len;
    RtkLogMsg("rtk_vendor_cmd_to_fw: Opcode:%x, parameter_len = %d",opcode, parameter_len);

    if(parameter_len > 0)
    {
        memcpy(p, parameter, parameter_len);
    }
    if(bt_vendor_cbacks)
    {
        pthread_mutex_lock(&rtk_prof.coex_mutex);
        if(!coex_cmd_send) {
            coex_cmd_send = true;
            RtkLogMsg("begin transmit_command Opcode:%x",opcode);
            pthread_mutex_unlock(&rtk_prof.coex_mutex);
            rtk_prof.current_cback = p_cback;
            bt_vendor_cbacks->xmit_cb(opcode, p_buf, rtk_cmd_complete_cback);
        }
        else {
            tRTK_COEX_INFO* pcoex_info = NULL;
            pcoex_info = malloc(sizeof(tRTK_COEX_INFO));
            if (NULL == pcoex_info)
            {
                ALOGE("rtk_vendor_cmd_to_fw: allocate error");
                pthread_mutex_unlock(&rtk_prof.coex_mutex);
                return;
            }

            pcoex_info->p_buf = p_buf;
            pcoex_info->opcode = opcode;
            pcoex_info->p_cback = p_cback;

            ListAddToTail(&(pcoex_info->list), &(rtk_prof.coex_list));
            pthread_mutex_unlock(&rtk_prof.coex_mutex);
        }

    }
    return ;
}

void rtk_notify_profileinfo_to_fw()
{
    RT_LIST_HEAD* head = NULL;
    RT_LIST_ENTRY* iter = NULL, *temp = NULL;
    tRTK_CONN_PROF* hci_conn = NULL;
    uint8_t  handle_number = 0;
    uint32_t buffer_size = 0;
    uint8_t *p_buf = NULL;

    head = &rtk_prof.conn_hash;
    LIST_FOR_EACH_SAFELY(iter, temp, head)
    {
        hci_conn = LIST_ENTRY(iter, tRTK_CONN_PROF, list);
        if (hci_conn && hci_conn->profile_bitmap)
            handle_number++;
    }

    buffer_size = 1 + handle_number*3 + 1;

    p_buf = (uint8_t *) malloc(buffer_size);

    if(NULL == p_buf)
    {
        ALOGE("rtk_notify_profileinfo_to_fw: alloc error");
        return;
    }
    uint8_t *p = (uint8_t *)p_buf;

    RtkLogMsg("rtk_notify_profileinfo_to_fw, BufferSize is %x", buffer_size);
    *p++ = handle_number;
    RtkLogMsg("rtk_notify_profileinfo_to_fw, NumberOfHandles is %x", handle_number);
    head = &rtk_prof.conn_hash;
    LIST_FOR_EACH_SAFELY(iter, temp, head)
    {
        hci_conn = LIST_ENTRY(iter, tRTK_CONN_PROF, list);
        if (hci_conn && hci_conn->profile_bitmap)
        {
            UINT16_TO_STREAM(p, hci_conn->handle);
            RtkLogMsg("rtk_notify_profileinfo_to_fw, handle is %x",hci_conn->handle);
            *p++ = hci_conn->profile_bitmap;
            RtkLogMsg("rtk_notify_profileinfo_to_fw, profile_bitmap is %x",hci_conn->profile_bitmap);
            handle_number --;
        }
        if(0 == handle_number)
            break;
    }

    *p++ = rtk_prof.profile_status;
    RtkLogMsg("rtk_notify_profileinfo_to_fw, profile_status is %x",rtk_prof.profile_status);

    rtk_vendor_cmd_to_fw(HCI_VENDOR_SET_PROFILE_REPORT_COMMAND, buffer_size, p_buf, NULL);

    free(p_buf);

    return ;
}

void update_profile_state(uint8_t profile_index, uint8_t is_busy)
{
    uint8_t need_update = FALSE;

    if((rtk_prof.profile_bitmap & BIT(profile_index)) == 0)
    {
        ALOGE("update_profile_state: ERROR!!! profile(Index: %x) does not exist", profile_index);
        return;
    }

    if(is_busy)
    {
        if((rtk_prof.profile_status & BIT(profile_index)) == 0)
        {
            need_update = TRUE;
            rtk_prof.profile_status |= BIT(profile_index);
        }
    }
    else
    {
        if((rtk_prof.profile_status & BIT(profile_index)) > 0)
        {
            need_update = TRUE;
            rtk_prof.profile_status &= ~(BIT(profile_index));
        }
    }

    if(need_update)
    {
        RtkLogMsg("update_profile_state, rtk_prof.profie_bitmap = %x", rtk_prof.profile_bitmap);
        RtkLogMsg("update_profile_state, rtk_prof.profile_status = %x", rtk_prof.profile_status);
        rtk_notify_profileinfo_to_fw();
    }
}

void rtk_check_setup_timer(int8_t profile_index)
{
    if(profile_index == profile_a2dp) {
        rtk_prof.a2dp_packet_count = 0;
        start_a2dp_packet_count_timer();
    }
    if(profile_index == profile_pan) {
        rtk_prof.pan_packet_count = 0;
        start_pan_packet_count_timer();
    }
    //hogp & voice share one timer now
    if((profile_index == profile_hogp) || (profile_index == profile_voice)) {
        if((0 == rtk_prof.profile_refcount[profile_hogp])
                && (0 == rtk_prof.profile_refcount[profile_voice])) {
            rtk_prof.hogp_packet_count = 0;
            rtk_prof.voice_packet_count = 0;
            start_hogp_packet_count_timer();
        }
    }
}

void rtk_check_del_timer(int8_t profile_index)
{
    if(profile_a2dp == profile_index)
    {
        rtk_prof.a2dp_packet_count = 0;
        stop_a2dp_packet_count_timer();
    }
    if(profile_pan == profile_index)
    {
        rtk_prof.pan_packet_count = 0;
        stop_pan_packet_count_timer();
    }
    if(profile_hogp == profile_index)
    {
        rtk_prof.hogp_packet_count = 0;
        if(rtk_prof.profile_refcount[profile_voice] == 0)
            stop_hogp_packet_count_timer();
    }
    if(profile_voice == profile_index)
    {
        rtk_prof.voice_packet_count = 0;
        if(rtk_prof.profile_refcount[profile_hogp] == 0)
            stop_hogp_packet_count_timer();
    }
}
void update_profile_connection(tRTK_CONN_PROF * phci_conn, int8_t profile_index, uint8_t is_add)
{
    uint8_t need_update = FALSE;
    int kk = 0;

    RtkLogMsg("update_profile_connection: is_add=%d, psm_index=%d", is_add, profile_index);
    if (profile_index < 0)
        return;

    if(is_add)
    {
        if(rtk_prof.profile_refcount[profile_index] == 0)
        {
            need_update = TRUE;
            rtk_prof.profile_bitmap |= BIT(profile_index);

            //SCO is always busy
            if(profile_index == profile_sco)
                rtk_prof.profile_status |= BIT(profile_index);

            rtk_check_setup_timer(profile_index);
        }
        rtk_prof.profile_refcount[profile_index]++;

        if(0 == phci_conn->profile_refcount[profile_index])
        {
            need_update = TRUE;
            phci_conn->profile_bitmap |= BIT(profile_index);
        }
        phci_conn->profile_refcount[profile_index]++;
    }
    else
    {
        rtk_prof.profile_refcount[profile_index]--;
        RtkLogMsg("for test: --, rtk_prof.profile_refcount[%x] = %x", profile_index, rtk_prof.profile_refcount[profile_index]);
        if(rtk_prof.profile_refcount[profile_index] == 0)
        {
            need_update = TRUE;
            rtk_prof.profile_bitmap &= ~(BIT(profile_index));

            //If profile does not exist, Status is meaningless
            rtk_prof.profile_status &= ~(BIT(profile_index));
            rtk_check_del_timer(profile_index);
        }

        phci_conn->profile_refcount[profile_index]--;
        if(0 == phci_conn->profile_refcount[profile_index])
        {
            need_update = TRUE;
            phci_conn->profile_bitmap &= ~(BIT(profile_index));

            //clear profile_hid_interval if need
            if(profile_hid == profile_index)
            {
                if((phci_conn->profile_bitmap &(BIT(profile_hid_interval))))
                {
                    phci_conn->profile_bitmap &= ~(BIT(profile_hid_interval));
                    rtk_prof.profile_refcount[profile_hid_interval]--;
                }
            }
        }
    }

    if(need_update)
    {
        RtkLogMsg("update_profile_connection: rtk_h5.profile_bitmap = %x", rtk_prof.profile_bitmap);
        for(kk=0; kk<8; kk++)
            RtkLogMsg("update_profile_connection: rtk_h5.profile_refcount[%d] = %d", kk, rtk_prof.profile_refcount[kk]);
        rtk_notify_profileinfo_to_fw();
    }
}

void update_hid_active_state(uint16_t handle, uint16_t interval)
{
    uint8_t need_update = 0;
    RtkLogMsg("update_hid_active_state: handle = %x, interval = 0x%x", handle, interval);
    tRTK_CONN_PROF *phci_conn = find_connection_by_handle(&rtk_prof, handle);

    if(phci_conn == NULL)
        return;

    if(((phci_conn->profile_bitmap)&(BIT(profile_hid))) == 0)
    {
        RtkLogMsg("hid not connected in the handle, nothing to be down");
        return;
    }

    if(interval < 60)
    {
        if((phci_conn->profile_bitmap &(BIT(profile_hid_interval))) == 0)
        {
            need_update = 1;
            phci_conn->profile_bitmap |= BIT(profile_hid_interval);

            rtk_prof.profile_refcount[profile_hid_interval]++;
            if(rtk_prof.profile_refcount[profile_hid_interval] == 1)
                rtk_prof.profile_status |= BIT(profile_hid);
        }
    }
    else
    {
        if((phci_conn->profile_bitmap &(BIT(profile_hid_interval))))
        {
            need_update = 1;
            phci_conn->profile_bitmap &= ~(BIT(profile_hid_interval));

            rtk_prof.profile_refcount[profile_hid_interval]--;
            if(rtk_prof.profile_refcount[profile_hid_interval] == 0)
                rtk_prof.profile_status &= ~(BIT(profile_hid));
        }
    }

    if(need_update)
        rtk_notify_profileinfo_to_fw();
}
uint8_t handle_l2cap_con_req(uint16_t handle, uint16_t psm, uint16_t scid, uint8_t direction)
{
    uint8_t status = FALSE;
    tRTK_PROF_INFO* prof_info = NULL;

    int8_t profile_index = psm_to_profile_index(psm);

    if(profile_index < 0) {
        RtkLogMsg("PSM(0x%x) do not need parse", psm);
        return status;
    }

    pthread_mutex_lock(&rtk_prof.profile_mutex);
    if(direction)//1: out
        prof_info = find_profile_by_handle_scid(&rtk_prof, handle, scid);
    else // 0:in
        prof_info = find_profile_by_handle_dcid(&rtk_prof, handle, scid);

    if(prof_info)
    {
        RtkLogMsg("handle_l2cap_con_req: This profile is already exist!!!");
        pthread_mutex_unlock(&rtk_prof.profile_mutex);
        return status;
    }

    if(direction)//1: out
        status = list_allocate_add(handle, psm, profile_index, 0, scid);
    else // 0:in
        status = list_allocate_add(handle, psm, profile_index, scid, 0);

    pthread_mutex_unlock(&rtk_prof.profile_mutex);

    if (!status)
        ALOGE("handle_l2cap_con_req: list_allocate_add failed!");

    return status;
}

uint8_t handle_l2cap_con_rsp(uint16_t handle, uint16_t dcid, uint16_t scid, uint8_t direction, uint8_t result)
{
    tRTK_PROF_INFO* prof_info = NULL;

    pthread_mutex_lock(&rtk_prof.profile_mutex);
    if(!direction)//0, in
        prof_info = find_profile_by_handle_scid(&rtk_prof, handle, scid);
    else //1, out
        prof_info = find_profile_by_handle_dcid(&rtk_prof, handle, scid);

    if (!prof_info)
    {
        //RtkLogMsg("handle_l2cap_con_rsp: prof_info Not Find!!");
        pthread_mutex_unlock(&rtk_prof.profile_mutex);
        return 0;
    }

    if(!result)//success
    {
        RtkLogMsg("l2cap connection success, update connection");
        if(!direction)//0, in
            prof_info->dcid = dcid;
        else//1, out
            prof_info->scid = dcid;

        tRTK_CONN_PROF *phci_conn = find_connection_by_handle(&rtk_prof, handle);
        if(phci_conn)
            update_profile_connection(phci_conn, prof_info->profile_index, TRUE);
    }

    pthread_mutex_unlock(&rtk_prof.profile_mutex);
    return 1;
}

uint8_t handle_l2cap_discon_req(uint16_t handle, uint16_t dcid, uint16_t scid, uint8_t direction)
{
    tRTK_PROF_INFO* prof_info = NULL;
    RtkLogMsg("l2cap_discon_req, handle = %x, dcid = %x, scid = %x, direction = %x", handle, dcid, scid, direction);
    pthread_mutex_lock(&rtk_prof.profile_mutex);
    if(!direction)//0: in
        prof_info = find_profile_by_handle_dcid_scid(&rtk_prof, handle, scid, dcid);
    else //1: out
        prof_info = find_profile_by_handle_dcid_scid(&rtk_prof, handle, dcid, scid);

    if (!prof_info)
    {
        //RtkLogMsg("handle_l2cap_discon_req: prof_info Not Find!");
        pthread_mutex_unlock(&rtk_prof.profile_mutex);
        return 0;
    }

    tRTK_CONN_PROF *phci_conn = find_connection_by_handle(&rtk_prof, handle);
    if(NULL == phci_conn)
    {
        pthread_mutex_unlock(&rtk_prof.profile_mutex);
        return 0;
    }

    update_profile_connection(phci_conn, prof_info->profile_index, FALSE);
    if (prof_info->profile_index == profile_a2dp && (phci_conn->profile_bitmap & BIT(profile_sink)))
        update_profile_connection(phci_conn, profile_sink, FALSE);
    delete_profile_from_hash(prof_info);
    pthread_mutex_unlock(&rtk_prof.profile_mutex);

    return 1;
}

void packets_count(uint16_t handle, uint16_t scid, uint16_t length, uint8_t direction, uint8_t *user_data)
{
    tRTK_PROF_INFO* prof_info = NULL;
    //uint8_t profile_type;

    tRTK_CONN_PROF* hci_conn = find_connection_by_handle(&rtk_prof, handle);
    if(NULL == hci_conn)
        return;

    if(0 == hci_conn->type)//l2cap
    {
        if(!direction)    //0: in
            prof_info = find_profile_by_handle_scid(&rtk_prof, handle, scid);
        else //1: out
            prof_info = find_profile_by_handle_dcid(&rtk_prof, handle, scid);

        if(!prof_info)
        {
            //RtkLogMsg("packets_count: prof_info Not Find!");
            //ALOGE("handle=%x, scid=%x, length=%d,",handle, scid, length);
            return ;
        }

        if((prof_info->profile_index == profile_a2dp) && (length > 100))//avdtp media data
        {
            if(!is_profile_busy(profile_a2dp)){
                struct sbc_frame_hdr *sbc_header;
                struct rtp_header *rtph;
                uint8_t bitpool;
                update_profile_state(profile_a2dp, TRUE);
                if (!direction) {
                    update_profile_connection(hci_conn, profile_sink, true);
                    update_profile_state(profile_sink, TRUE);
                }
                rtph = (struct rtp_header *)user_data;
                RtkLogMsg("rtp: v %u, cc %u, pt %u", rtph->v, rtph->cc, rtph->pt);
                /* move forward */
                user_data += sizeof(struct rtp_header) + rtph->cc * 4 + 1;
                /* point to the sbc frame header */
                sbc_header = (struct sbc_frame_hdr *)user_data;
                bitpool = sbc_header->bitpool;
                print_sbc_header(sbc_header);
                RtkLogMsg("rtp: v %u, cc %u, pt %u", rtph->v, rtph->cc, rtph->pt);
                rtk_vendor_cmd_to_fw(HCI_VENDOR_ADD_BITPOOL_FW, 1, &bitpool, NULL);
            }
            rtk_prof.a2dp_packet_count++;
        }

        if(prof_info->profile_index == profile_pan)
            rtk_prof.pan_packet_count++;
    }
}

static void timeout_handler(int signo, siginfo_t * info, void *context)
{
    RTK_UNUSED(info);
    RTK_UNUSED(context);
    if (signo == TIMER_POLLING)
    {
        RtkLogMsg("polling timeout");
        if(rtk_prof.polling_enable)
        {
            uint8_t temp_cmd[1];
            temp_cmd[0] = HCI_VENDOR_SUB_CMD_BT_REPORT_CONN_SCO_INQ_INFO;
            rtk_vendor_cmd_to_fw(HCI_VENDOR_MAILBOX_CMD, 1, temp_cmd, NULL);
        }
    }
    else if (signo == TIMER_A2DP_PACKET_COUNT)
    {
        RtkLogMsg("count a2dp packet timeout, a2dp_packet_count = %d",rtk_prof.a2dp_packet_count);
        if(rtk_prof.a2dp_packet_count == 0)
        {
            if(is_profile_busy(profile_a2dp))
            {
                RtkLogMsg("timeout_handler: a2dp busy->idle!");
                update_profile_state(profile_a2dp, FALSE);
                if (is_profile_busy(profile_sink)) {
                    RtkLogMsg("timeout_handler: sink busy->idle!");
                    update_profile_state(profile_sink, FALSE);
                }
            }
        }
        rtk_prof.a2dp_packet_count = 0;
    }
    else if (signo == TIMER_HOGP_PACKET_COUNT)
    {
        RtkLogMsg("count hogp packet timeout, hogp_packet_count = %d",rtk_prof.hogp_packet_count);
        if(rtk_prof.hogp_packet_count == 0)
        {
            if(is_profile_busy(profile_hogp))
            {
                RtkLogMsg("timeout_handler: hogp busy->idle!");
                update_profile_state(profile_hogp, FALSE);
            }
        }
        rtk_prof.hogp_packet_count = 0;

        RtkLogMsg("count hogp packet timeout, voice_packet_count = %d",rtk_prof.voice_packet_count);
        if(rtk_prof.voice_packet_count == 0)
        {
            if(is_profile_busy(profile_voice))
            {
                RtkLogMsg("timeout_handler: voice busy->idle!");
                update_profile_state(profile_voice, FALSE);
            }
        }
        rtk_prof.voice_packet_count = 0;
    }
    else if (signo == TIMER_PAN_PACKET_COUNT)
    {
        RtkLogMsg("count pan packet timeout, pan_packet_count = %d",rtk_prof.pan_packet_count);
        if(rtk_prof.pan_packet_count < PAN_PACKET_COUNT)
        {
            if(is_profile_busy(profile_pan))
            {
                RtkLogMsg("timeout_handler: pan busy->idle!");
                update_profile_state(profile_pan, FALSE);
            }
        }
        else
        {
            if(!is_profile_busy(profile_pan))
            {
                RtkLogMsg("timeout_handler: pan idle->busy!");
                update_profile_state(profile_pan, TRUE);
            }
        }
        rtk_prof.pan_packet_count = 0;
    }
    else
    {
        ALOGE("rtk_parse_data timer unspported signo(%d)", signo);
    }
}

static void notify_func(union sigval sig)
{
    int signo = sig.sival_int;
    timeout_handler(signo, NULL, NULL);
}

#if 0
int netlink_send(int nlsk, char *buffer)
{
    struct nlmsghdr* nlhdr;
    struct iovec iov;
    struct msghdr msg;
    struct sockaddr_nl nladdr;

    if(nlsk <= 0)
        return -1;

    memset(&msg, 0 ,sizeof(struct msghdr));
    memset(&nladdr, 0 ,sizeof(struct sockaddr_nl));

    nlhdr = (struct nlmsghdr *)malloc(NLMSG_SPACE(strlen(buffer) + 1));
    strcpy(NLMSG_DATA(nlhdr),buffer);

    nlhdr->nlmsg_len = NLMSG_LENGTH(strlen(buffer) + 1);
    nlhdr->nlmsg_pid = getpid(); //sender pid
    nlhdr->nlmsg_flags = NLM_F_REQUEST;
    nlhdr->nlmsg_type = NLMSG_MIN_TYPE;

    nladdr.nl_family = AF_NETLINK;
    nladdr.nl_pid = 0; //send to kernel
    nladdr.nl_groups = 0;

    iov.iov_base = (void *)nlhdr;
    iov.iov_len = nlhdr->nlmsg_len;

    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_name = (void *)&(nladdr);
    msg.msg_namelen = sizeof(nladdr);

    return sendmsg(nlsk, &msg, 0);
}
#endif

int udpsocket_send(char *tx_msg, int msg_size)
{
    int n; /* message byte size */

    RtkLogMsg("udpsocket_send tx_msg:%s",tx_msg);
    n = sendto(rtk_prof.udpsocket, tx_msg, msg_size, 0, (struct sockaddr *) &rtk_prof.client_addr, sizeof(rtk_prof.client_addr));
    if (n < 0)
    {
        ALOGE("ERROR in sendto");
        return -1;
    }
    return 0;
}

int udpsocket_recv(uint8_t *recv_msg, uint8_t *msg_size)
{
    //struct hostent *hostp;  /* client host info */
    char buf[MAX_PAYLOAD];  /* message buf */
    //char *hostaddrp;        /* dotted decimal host addr string */
    int n;                  /* message byte size */
    struct sockaddr_in recv_addr;
    socklen_t clientlen = sizeof(recv_addr);
    struct pollfd pfd = {
        .events = POLLPRI | POLLIN,
        .revents = 0,
        .fd = rtk_prof.udpsocket
    };

    bzero(buf, MAX_PAYLOAD);

    while (poll(&pfd, 1, 1000) <= 0) {
        if (rtk_prof.coex_recv_thread_running ==0) {
            RtkLogMsg("%s, SIGUSR2 should have caught us before this", __func__);
            return -1;
        }
    }


    n = recvfrom(rtk_prof.udpsocket, buf, MAX_PAYLOAD, 0, (struct sockaddr *) &recv_addr, &clientlen);
    if (n < 0) {
        ALOGE("ERROR in recvfrom");
        return -1;
    } else {
        *msg_size = n;
        memcpy(recv_msg,buf,n);
    }
    return 0;
}


int btcoex_chr_send(char *tx_msg, int msg_size)
{
    int n; /* message byte size */

    RtkLogMsg("btcoex_chr_send tx_msg:%s",tx_msg);
    RTK_NO_INTR (n = write(rtk_prof.btcoex_chr, tx_msg, msg_size));
    if (n < 0)
    {
        ALOGE("ERROR in write");
        return -1;
    }
    return n;
}

int btcoex_chr_recv(uint8_t *recv_msg, uint8_t *msg_size)
{
    char buf[MAX_PAYLOAD];  /* message buf */
    int n = -1;                  /* message byte size */
    struct pollfd pfd = {
        .events = POLLPRI|POLLIN|POLLHUP|POLLERR|POLLRDHUP,
        .revents = 0,
        .fd = rtk_prof.btcoex_chr
    };

    bzero(buf, MAX_PAYLOAD);

    while (poll(&pfd, 1, 1000) <= 0) {
        if (rtk_prof.coex_recv_thread_running == 0) {
            RtkLogMsg("%s, SIGUSR2 should have caught us before this", __func__);
            return -1;
        }
    }

    if (pfd.revents & POLLIN) {
        RTK_NO_INTR(n = read(rtk_prof.btcoex_chr, buf, MAX_PAYLOAD));
        if (n < 0) {
            ALOGE("ERROR in recvfrom");
            return -1;
        } else {
            *msg_size = n;
            memcpy(recv_msg, buf, n);
        }
    }
    else {
        ALOGE("rtk_btcoex is wrong");
        return -1;
    }
    return 0;
}

void rtk_notify_extension_version_to_wifi()
{
    uint8_t para_length = 2;
    char p_buf[para_length + HCI_CMD_PREAMBLE_SIZE];

    if(!rtk_prof.wifi_on)
        return;

    char *p = p_buf;
    UINT16_TO_STREAM(p, HCI_OP_HCI_EXTENSION_VERSION_NOTIFY);
    *p++ = para_length;
    UINT16_TO_STREAM(p, HCI_EXTENSION_VERSION);
    RtkLogMsg("extension version is 0x%x", HCI_EXTENSION_VERSION);
    if(coex_msg_send(p_buf, para_length + HCI_CMD_PREAMBLE_SIZE) < 0)
        ALOGE("rtk_notify_extension_version_to_wifi: udpsocket send error");
}

void rtk_notify_btpatch_version_to_wifi()
{
    uint8_t para_length = 4;
    char p_buf[para_length + HCI_CMD_PREAMBLE_SIZE];

    if(!rtk_prof.wifi_on)
        return;

    char *p = p_buf;
    UINT16_TO_STREAM(p, HCI_OP_HCI_BT_PATCH_VER_NOTIFY);
    *p++ = para_length;
    UINT16_TO_STREAM(p, rtk_prof.hci_reversion);
    UINT16_TO_STREAM(p, rtk_prof.lmp_subversion);
    RtkLogMsg("btpatch_version, length is 0x%x, hci_reversion is 0x%x, lmp_subversion is %x", para_length, rtk_prof.hci_reversion, rtk_prof.lmp_subversion);

    if(coex_msg_send(p_buf, para_length + HCI_CMD_PREAMBLE_SIZE) < 0)
        ALOGE("rtk_notify_btpatch_version_to_wifi: udpsocket send error");
}

void rtk_notify_afhmap_to_wifi()
{
    uint8_t para_length = 13;
    char p_buf[para_length + HCI_CMD_PREAMBLE_SIZE];

    if(!rtk_prof.wifi_on)
        return;

    char *p = p_buf;
    UINT16_TO_STREAM(p, HCI_OP_HCI_BT_AFH_MAP_NOTIFY);
    *p++ = para_length;
    *p++ = rtk_prof.piconet_id;
    *p++ = rtk_prof.mode;
    *p++ = 10;
    memcpy(p, rtk_prof.afh_map, 10);

    RtkLogMsg("afhmap, piconet_id is 0x%x, map type is 0x%x", rtk_prof.piconet_id, rtk_prof.mode);
    uint8_t kk = 0;
    for(kk=0; kk < 10; kk++)
        RtkLogMsg("afhmap data[%d] is 0x%x", kk, rtk_prof.afh_map[kk]);

    if(coex_msg_send(p_buf, para_length + HCI_CMD_PREAMBLE_SIZE) < 0)
        ALOGE("rtk_notify_afhmap_to_wifi: udpsocket send error");
}

void rtk_notify_btcoex_to_wifi(uint8_t opcode, uint8_t status)
{
    uint8_t para_length = 2;
    char p_buf[para_length + HCI_CMD_PREAMBLE_SIZE];

    if(!rtk_prof.wifi_on)
        return;

    char *p = p_buf;
    UINT16_TO_STREAM(p, HCI_OP_HCI_BT_COEX_NOTIFY);
    *p++ = para_length;
    *p++ = opcode;
    if(!status)
        *p++ = 0;
    else
        *p++ = 1;

    RtkLogMsg("btcoex, opcode is 0x%x, status is 0x%x", opcode, status);

    if(coex_msg_send(p_buf, para_length + HCI_CMD_PREAMBLE_SIZE) < 0)
        ALOGE("rtk_notify_btcoex_to_wifi: udpsocket send error");
}

void rtk_notify_btoperation_to_wifi(uint8_t operation, uint8_t append_data_length, uint8_t *append_data)
{
    uint8_t para_length = 3 + append_data_length;
    char p_buf[para_length + HCI_CMD_PREAMBLE_SIZE];

    if(!rtk_prof.wifi_on)
        return;

    char *p = p_buf;
    UINT16_TO_STREAM(p, HCI_OP_BT_OPERATION_NOTIFY);
    *p++ = para_length;
    *p++ = operation;
    *p++ = append_data_length;
    if(append_data_length)
        memcpy(p, append_data, append_data_length);

    RtkLogMsg("btoperation, opration is 0x%x, append_data_length is 0x%x", operation, append_data_length);
    uint8_t kk = 0;
    if(append_data_length)
    {
        for(kk=0; kk < append_data_length; kk++)
            RtkLogMsg("append data is 0x%x", *(append_data+kk));
    }

    if(coex_msg_send(p_buf, para_length + HCI_CMD_PREAMBLE_SIZE) < 0)
        ALOGE("rtk_notify_btoperation_to_wifi: udpsocket send error");
}

void rtk_notify_info_to_wifi(uint8_t reason, uint8_t length, uint8_t* report_info)
{
    uint8_t para_length = 4 + length;
    char p_buf[para_length + HCI_CMD_PREAMBLE_SIZE];
    int i;

    if(!rtk_prof.wifi_on)
        return;

    char *p = p_buf;
    UINT16_TO_STREAM(p, HCI_OP_HCI_BT_INFO_NOTIFY);
    *p++ = para_length;
    *p++ = rtk_prof.polling_enable;
    *p++ = rtk_prof.polling_interval;
    *p++ = reason;
    *p++ = length;

    if(length)
        memcpy(p, report_info, length);

    RtkLogMsg("bt info, length is 0x%x, polling_enable is 0x%x, poiiling_interval is %x",para_length, rtk_prof.polling_enable, rtk_prof.polling_interval);
    RtkLogMsg("bt info, reason is 0x%x, info length is 0x%x", reason, length);
    if(length)
    {
        for(i=0;i<length;i++)
            RtkLogMsg("bt info[%d]: %02x", i, report_info[i]);
    }

    if(coex_msg_send(p_buf, para_length + HCI_CMD_PREAMBLE_SIZE) < 0)
        ALOGE("rtk_notify_info_to_wifi: udpsocket send error");
}

void rtk_notify_regester_to_wifi(uint8_t* reg_value)
{
    uint8_t para_length = 9;
    char p_buf[para_length + HCI_CMD_PREAMBLE_SIZE];

    if(!rtk_prof.wifi_on)
        return;

    char *p = p_buf;
    UINT16_TO_STREAM(p, HCI_OP_HCI_BT_REGISTER_VALUE_NOTIFY);
    *p++ = para_length;
    memcpy(p, reg_value, para_length);

    tHCI_RETURN_PARAMETER_MAILBOX_REGISTER *reg = (tHCI_RETURN_PARAMETER_MAILBOX_REGISTER *)reg_value;
    RtkLogMsg("bt register, register type is %x", reg->type);
    RtkLogMsg("bt register, register offset is %x", reg->offset);
    RtkLogMsg("bt register, register value is %x", reg->value);

    if(coex_msg_send(p_buf, para_length + HCI_CMD_PREAMBLE_SIZE) < 0)
        ALOGE("rtk_notify_regester_to_wifi: udpsocket send error");
}

static void rtk_handle_bt_info_control(uint8_t* p)
{
    tHCI_EVENT_BT_INFO_CONTROL*  info = (tHCI_EVENT_BT_INFO_CONTROL*)p;
    uint8_t temp_cmd[3];

    RtkLogMsg("rtk_prof.polling_enable is %x",rtk_prof.polling_enable);
    RtkLogMsg("receive bt info control event from wifi, polling enable is 0x%x, polling time is 0x%x, auto report is 0x%x",
                    info->polling_enable, info->polling_time, info->autoreport_enable);

    if(info->polling_enable && !rtk_prof.polling_enable)
        start_polling_timer(info->polling_time * 1000);

    if(!info->polling_enable && rtk_prof.polling_enable)
        stop_polling_timer();

    rtk_prof.polling_enable = info->polling_enable;
    rtk_prof.polling_interval = info->polling_time;
    rtk_prof.autoreport = info->autoreport_enable;

    temp_cmd[0] = HCI_VENDOR_SUB_CMD_BT_AUTO_REPORT_ENABLE;
    temp_cmd[1] = 1;
    temp_cmd[2] = info->autoreport_enable;
    rtk_vendor_cmd_to_fw(HCI_VENDOR_MAILBOX_CMD, 3, temp_cmd, NULL);

    rtk_notify_info_to_wifi(HOST_RESPONSE, 0, NULL);
}

static void rtk_handle_bt_coex_control(uint8_t* p)
{
    uint8_t opcode = *p++;
	uint8_t op_len = 0;
    RtkLogMsg("receive bt coex control event from wifi, opration is 0x%x", opcode);
    switch (opcode)
    {
        case BT_PATCH_VERSION_QUERY:
        {
            rtk_notify_btpatch_version_to_wifi();
            break;
        }

        case IGNORE_WLAN_ACTIVE_CONTROL:
        {
            uint8_t opcode_len = *p++;
            uint8_t value = *p++;
            uint8_t temp_cmd[3];
			op_len = opcode_len;
            temp_cmd[0] = HCI_VENDOR_SUB_CMD_BT_ENABLE_IGNORE_WLAN_ACT_CMD;
            temp_cmd[1] = 1;
            temp_cmd[2] = value;
            rtk_vendor_cmd_to_fw(HCI_VENDOR_MAILBOX_CMD, 3, temp_cmd, NULL);
            break;
        }

        case LNA_CONSTRAIN_CONTROL:
        {
            uint8_t opcode_len = *p++;
            uint8_t value = *p++;
            uint8_t temp_cmd[3];
			op_len = opcode_len;
            temp_cmd[0] = HCI_VENDOR_SUB_CMD_SET_BT_LNA_CONSTRAINT;
            temp_cmd[1] = 1;
            temp_cmd[2] = value;
            rtk_vendor_cmd_to_fw(HCI_VENDOR_MAILBOX_CMD, 3, temp_cmd, NULL);
            break;
        }

        case BT_POWER_DECREASE_CONTROL:
        {
            uint8_t opcode_len = *p++;
            uint8_t power_decrease = *p++;
            uint8_t temp_cmd[3];
			op_len = opcode_len;
            temp_cmd[0] = HCI_VENDOR_SUB_CMD_WIFI_FORCE_TX_POWER_CMD;
            temp_cmd[1] = 1;
            temp_cmd[2] = power_decrease;
            rtk_vendor_cmd_to_fw(HCI_VENDOR_MAILBOX_CMD, 3, temp_cmd, NULL);
            break;
        }

        case BT_PSD_MODE_CONTROL:
        {
            uint8_t opcode_len = *p++;
            uint8_t psd_mode = *p++;
            uint8_t temp_cmd[3];
			op_len = opcode_len;
            temp_cmd[0] = HCI_VENDOR_SUB_CMD_SET_BT_PSD_MODE;
            temp_cmd[1] = 1;
            temp_cmd[2] = psd_mode;
            rtk_vendor_cmd_to_fw(HCI_VENDOR_MAILBOX_CMD, 3, temp_cmd, NULL);
            break;
        }

        case WIFI_BW_CHNL_NOTIFY:
        {
            uint8_t opcode_len = *p++;
            uint8_t temp_cmd[5];
			op_len = opcode_len;
            temp_cmd[0] = HCI_VENDOR_SUB_CMD_WIFI_CHANNEL_AND_BANDWIDTH_CMD;
            temp_cmd[1] = 3;
            memcpy(temp_cmd+2, p, 3);//wifi_state, wifi_centralchannel, chnnels_btnotuse
            rtk_vendor_cmd_to_fw(HCI_VENDOR_MAILBOX_CMD, 5, temp_cmd, NULL);
            break;
        }

        case QUERY_BT_AFH_MAP:
        {
            uint8_t opcode_len = *p++;
            rtk_prof.piconet_id = *p++;
            rtk_prof.mode = *p++;
            uint8_t temp_cmd[4];
			op_len = opcode_len;
            temp_cmd[0] = HCI_VENDOR_SUB_CMD_GET_AFH_MAP_L;
            temp_cmd[1] = 2;
            temp_cmd[2] = rtk_prof.piconet_id;
            temp_cmd[3] = rtk_prof.mode;
            rtk_vendor_cmd_to_fw(HCI_VENDOR_MAILBOX_CMD, 4, temp_cmd, NULL);
            break;
        }

        case BT_REGISTER_ACCESS:
        {
            uint8_t opcode_len = *p++;
            uint8_t access_type = *p++;
			op_len = opcode_len;
            if(access_type == 0) //read
            {
                uint8_t temp_cmd[7];
                temp_cmd[0] = HCI_VENDOR_SUB_CMD_RD_REG_REQ;
                temp_cmd[1] = 5;
                temp_cmd[2] = *p++;
                memcpy(temp_cmd+3, p, 4);
                rtk_vendor_cmd_to_fw(HCI_VENDOR_MAILBOX_CMD, 7, temp_cmd, NULL);
            }
            else //write
            {
                uint8_t temp_cmd[11];
                temp_cmd[0] = HCI_VENDOR_SUB_CMD_RD_REG_REQ;
                temp_cmd[1] = 5;
                temp_cmd[2] = *p++;
                memcpy(temp_cmd+3, p, 8);
                rtk_vendor_cmd_to_fw(HCI_VENDOR_MAILBOX_CMD, 11, temp_cmd, NULL);
            }
            break;
        }

        default:
            break;
    }
}

void rtk_handle_event_from_wifi(uint8_t* msg)
{
    uint8_t *p = msg;
    uint8_t event_code = *p++;
    uint8_t total_length = 0;

    RtkLogMsg("receive invite rsp from wifi msg : %s", msg);
    if(memcmp(msg, invite_rsp, sizeof(invite_rsp)) == 0)
    {
#if 0
        RtkLogMsg("receive invite rsp from wifi, close netlink socket if needed");
        if(rtk_prof.nlsocket > 0)
        {
            close(rtk_prof.nlsocket);
            RtkLogMsg("close netlink socket %d", rtk_prof.nlsocket);
        }
#endif
        RtkLogMsg("receive invite rsp from wifi, wifi is already on");
        rtk_prof.wifi_on = 1;
        rtk_notify_extension_version_to_wifi();
    }

    if(memcmp(msg, attend_req, sizeof(attend_req)) == 0)
    {
        RtkLogMsg("receive attend req from wifi, wifi turn on");
        rtk_prof.wifi_on = 1;
        coex_msg_send(attend_ack, sizeof(attend_ack));
        rtk_notify_extension_version_to_wifi();
    }

    if(memcmp(msg, wifi_leave, sizeof(wifi_leave)) == 0)
    {
        RtkLogMsg("receive wifi leave from wifi, wifi turn off");
        rtk_prof.wifi_on = 0;
        coex_msg_send(leave_ack, sizeof(leave_ack));
        if(rtk_prof.polling_enable)
        {
            rtk_prof.polling_enable = 0;
            stop_polling_timer();
        }
    }

    if(memcmp(msg, leave_ack, sizeof(leave_ack)) == 0)
    {
        RtkLogMsg("receive leave ack from wifi");
    }

    if(event_code == 0xFE)
    {
        total_length = *p++;
        uint8_t extension_event = *p++;
        switch(extension_event)
        {
            case  RTK_HS_EXTENSION_EVENT_WIFI_SCAN:
            {
                uint8_t operation = *p;
                RtkLogMsg("receive wifi scan notify evnet from wifi, operation is 0x%x", operation);
                break;
            }

            case  RTK_HS_EXTENSION_EVENT_HCI_BT_INFO_CONTROL:
            {
                rtk_handle_bt_info_control(p);
                break;
            }

            case RTK_HS_EXTENSION_EVENT_HCI_BT_COEX_CONTROL:
            {
                rtk_handle_bt_coex_control(p);
                break;
            }

            default:
                break;
        }
    }

    if(event_code == 0x0E)
    {
        uint16_t wifi_opcode;
        uint8_t op_status;
        p += 2;//length, number of complete packets
        STREAM_TO_UINT16(wifi_opcode, p);
        op_status = *p;
        RtkLogMsg("receive command complete event from wifi, op code is 0x%x, status is 0x%x", wifi_opcode, op_status);
    }
}

static void coex_receive_thread_exit_handler(int sig)
{
    RtkLogMsg("USR2, this signal is %d \n", sig);
    usleep(100);
    pthread_exit(0);
}

static void btwifi_coex_receive_thread(void *arg)
{
    RTK_UNUSED(arg);
    uint8_t msg_recv[MAX_PAYLOAD];
    uint8_t recv_length;
    struct sigaction actions;

    memset(&actions, 0, sizeof(actions));
    sigemptyset(&actions.sa_mask);
    actions.sa_flags = 0;
    actions.sa_handler = coex_receive_thread_exit_handler;

    sigaction(SIGUSR2,&actions,NULL);//int rc = sigaction(SIGUSR2,&actions,NULL);

    RtkLogMsg("btwifi_coex_receive_thread started");
    prctl(PR_SET_NAME, (unsigned long)"btwifi_coex_receive_thread", 0, 0, 0);

    while(rtk_prof.coex_recv_thread_running)
    {
        memset(msg_recv, 0 , MAX_PAYLOAD);
        if (coex_msg_recv(msg_recv, &recv_length) == 0)
            rtk_handle_event_from_wifi(msg_recv);
    }

    RtkLogMsg("btwifi_coex_receive_thread exiting");
    pthread_exit(NULL);
}

int create_udpsocket_socket()
{
    int portno = CONNECT_PORT;
    int optval; /* flag value for setsockopt */

    RtkLogMsg("create udpsocket port: %d\n", portno);

    pthread_mutex_lock(&rtk_prof.btwifi_mutex);

    pthread_attr_t thread_attr_data;
    if (rtk_prof.coex_recv_thread_running)
    {
        ALOGE("udp_receive_thread already exit");
        pthread_mutex_unlock(&rtk_prof.btwifi_mutex);
        return -1 ;
    }

    rtk_prof.coex_recv_thread_running = 1;
    rtk_prof.udpsocket = socket(AF_INET, SOCK_DGRAM, 0);
    RtkLogMsg("create socket %d", rtk_prof.udpsocket);

    if (rtk_prof.udpsocket < 0)
    {
        ALOGE("create udpsocket error...%s\n", strerror(errno));
        rtk_prof.coex_recv_thread_running = 0;
        close(rtk_prof.udpsocket);
        RtkLogMsg("close socket %d", rtk_prof.udpsocket);
        pthread_mutex_unlock(&rtk_prof.btwifi_mutex);
        return -1 ;
    }

    bzero((char *) &rtk_prof.server_addr, sizeof(rtk_prof.server_addr));
    rtk_prof.server_addr.sin_family = AF_INET;
    rtk_prof.server_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    rtk_prof.server_addr.sin_port = htons(CONNECT_PORT);

    bzero((char *) &rtk_prof.client_addr, sizeof(rtk_prof.client_addr));
    rtk_prof.client_addr.sin_family = AF_INET;
    rtk_prof.client_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    rtk_prof.client_addr.sin_port = htons(CONNECT_PORT_WIFI);

    optval = 1;
    setsockopt(rtk_prof.udpsocket, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));

    if (bind(rtk_prof.udpsocket, (struct sockaddr *)&rtk_prof.server_addr, sizeof(rtk_prof.server_addr)) < 0)
    {
        ALOGE("bind udpsocket error...%s\n", strerror(errno));
        rtk_prof.coex_recv_thread_running = 0;
        close(rtk_prof.udpsocket);
        RtkLogMsg("close socket %d", rtk_prof.udpsocket);
        pthread_mutex_unlock(&rtk_prof.btwifi_mutex);
        return -1 ;
    }

    pthread_attr_init(&thread_attr_data);
    if (pthread_create(&rtk_prof.thread_data, &thread_attr_data, (void*)btwifi_coex_receive_thread, NULL) != 0)
    {
        ALOGE("pthread_create failed!");
        pthread_attr_destroy(&thread_attr_data);
        rtk_prof.coex_recv_thread_running = 0;
        pthread_mutex_unlock(&rtk_prof.btwifi_mutex);
        return -1 ;
    }
    pthread_attr_destroy(&thread_attr_data);
    pthread_mutex_unlock(&rtk_prof.btwifi_mutex);
    return 0;
}

int stop_btwifi_coex_receive_thread()
{
    pthread_mutex_lock(&rtk_prof.btwifi_mutex);
    int result = 0;

    RtkLogMsg("notify wifi bt turn off");
    if(rtk_prof.wifi_on)
        coex_msg_send(bt_leave, sizeof(bt_leave));

    if (rtk_prof.coex_recv_thread_running)
    {
        RtkLogMsg("data thread is running, stop it");

        //add for pthread_cancel
        if ((result = pthread_kill(rtk_prof.thread_data, SIGUSR2)) != 0)
        {
            ALOGE("error cancelling data thread");
        }
        rtk_prof.coex_recv_thread_running = 0;

        if ((result = pthread_join(rtk_prof.thread_data, NULL)) < 0)
        {
            ALOGE( "data thread pthread_join() failed result:%d", result);
        }

        if(rtk_prof.udpsocket > 0) {
            RtkLogMsg("close socket %d", rtk_prof.udpsocket);
            if((result = close(rtk_prof.udpsocket)) != 0)
            {
                ALOGE("close socket error!");
            }
        }
        else if(rtk_prof.btcoex_chr > 0) {
            RtkLogMsg("close char device  %d", rtk_prof.btcoex_chr);
            if((result = close(rtk_prof.btcoex_chr)) != 0)
            {
                ALOGE("close char device  error!");
            }
        }
    }
    pthread_mutex_unlock(&rtk_prof.btwifi_mutex);
    return 0;
}

#if 0
int create_netlink_socket()
{
    RtkLogMsg("in creat netlink socket");
    rtk_prof.nlsocket = socket(PF_NETLINK, SOCK_RAW, NETLINK_USER);

    if (rtk_prof.nlsocket < 0)
    {
        ALOGE("create netlink socket error...%s\n", strerror(errno));
        close(rtk_prof.nlsocket);
        RtkLogMsg("close netlink socket %d", rtk_prof.nlsocket);
        return -1 ;
    }
    RtkLogMsg("create netlink socket %d", rtk_prof.nlsocket);
    memset(&rtk_prof.src_addr, 0, sizeof(rtk_prof.src_addr));
    rtk_prof.src_addr.nl_family = AF_NETLINK;
    rtk_prof.src_addr.nl_pid = getpid(); /* self pid */
    rtk_prof.src_addr.nl_groups    = 0 ;    /* not in mcast groups */
    int ret = bind(rtk_prof.nlsocket, (struct sockaddr *)&rtk_prof.src_addr, sizeof(rtk_prof.src_addr));
    if(ret < 0)
    {
        ALOGE("bind netlink socket error...%s\n", strerror(errno));
        close(rtk_prof.nlsocket);
        RtkLogMsg("close netlink socket %d", rtk_prof.nlsocket);
        return -1 ;
    }

    return 0;
}
#endif

int open_btcoex_chrdev()
{
    RtkLogMsg("open_btcoex_chrdev\n");

    pthread_mutex_lock(&rtk_prof.btwifi_mutex);

    pthread_attr_t thread_attr_data;
    if (rtk_prof.coex_recv_thread_running)
    {
        ALOGE("udp_receive_thread already exit");
        pthread_mutex_unlock(&rtk_prof.btwifi_mutex);
        return -1 ;
    }

    rtk_prof.coex_recv_thread_running = 1;
    if ((rtk_prof.btcoex_chr = open("/dev/rtk_btcoex", O_RDWR)) < 0)
    {
        ALOGE("open rtk_btcoex error...%s\n", strerror(errno));
        rtk_prof.coex_recv_thread_running = 0;
        pthread_mutex_unlock(&rtk_prof.btwifi_mutex);
        return -1 ;
    }

    pthread_attr_init(&thread_attr_data);
    if (pthread_create(&rtk_prof.thread_data, &thread_attr_data, (void*)btwifi_coex_receive_thread, NULL) != 0)
    {
        ALOGE("create coexchr_receive_thread failed!");
        pthread_attr_destroy(&thread_attr_data);
        rtk_prof.coex_recv_thread_running = 0;
        pthread_mutex_unlock(&rtk_prof.btwifi_mutex);
        return -1 ;
    }
    pthread_attr_destroy(&thread_attr_data);
    pthread_mutex_unlock(&rtk_prof.btwifi_mutex);
    return 0;
}

void rtk_parse_init(void)
{
    ALOGI("RTKBT_RELEASE_NAME: %s",RTKBT_RELEASE_NAME);
    RtkLogMsg("rtk_profile_init, version: %s", RTK_COEX_VERSION);

    memset(&rtk_prof, 0, sizeof(rtk_prof));
    pthread_mutex_init(&rtk_prof.profile_mutex, NULL);
    pthread_mutex_init(&rtk_prof.coex_mutex, NULL);
    pthread_mutex_init(&rtk_prof.btwifi_mutex, NULL);
    alloc_a2dp_packet_count_timer();
    alloc_pan_packet_count_timer();
    alloc_hogp_packet_count_timer();
    alloc_polling_timer();

    init_profile_hash(&rtk_prof);
    init_connection_hash(&rtk_prof);
    init_coex_hash(&rtk_prof);

    if(create_udpsocket_socket() < 0) {
        ALOGE("UDP socket fail, try to use rtk_btcoex chrdev");
        open_btcoex_chrdev();
    }
}

void rtk_parse_cleanup()
{
    RtkLogMsg("rtk_profile_cleanup");
    free_a2dp_packet_count_timer();
    free_pan_packet_count_timer();
    free_hogp_packet_count_timer();
    free_polling_timer();

    flush_connection_hash(&rtk_prof);
    flush_profile_hash(&rtk_prof);
    pthread_mutex_destroy(&rtk_prof.profile_mutex);
    flush_coex_hash(&rtk_prof);
    pthread_mutex_destroy(&rtk_prof.coex_mutex);

    stop_btwifi_coex_receive_thread();
    pthread_mutex_destroy(&rtk_prof.btwifi_mutex);

    memset(&rtk_prof, 0, sizeof(rtk_prof));
}

static void rtk_handle_vender_mailbox_cmp_evt(uint8_t* p, uint8_t len)
{
    uint8_t status = *p++;
    if(len <= 4)
    {
        RtkLogMsg("receive mailbox cmd from fw, total length <= 4");
        return;
    }
    uint8_t subcmd = *p++;
    RtkLogMsg("receive mailbox cmd from fw, subcmd is 0x%x, status is 0x%x", subcmd, status);
    switch(subcmd)
    {
        case HCI_VENDOR_SUB_CMD_BT_REPORT_CONN_SCO_INQ_INFO:
            if(status == 0) //success
            {
                if((len-5) != 8)
                    RtkLogMsg("rtk_handle_vender_mailbox_cmp_evt:HCI_VENDOR_SUB_CMD_BT_REPORT_CONN_SCO_INQ_INFO len=%d", len);
                rtk_notify_info_to_wifi(POLLING_RESPONSE, (len-5), (uint8_t*)p);
            }
            break;

        case HCI_VENDOR_SUB_CMD_WIFI_CHANNEL_AND_BANDWIDTH_CMD:
            rtk_notify_btcoex_to_wifi(WIFI_BW_CHNL_NOTIFY, status);
            break;

        case HCI_VENDOR_SUB_CMD_WIFI_FORCE_TX_POWER_CMD:
            rtk_notify_btcoex_to_wifi(BT_POWER_DECREASE_CONTROL, status);
            break;

        case HCI_VENDOR_SUB_CMD_BT_ENABLE_IGNORE_WLAN_ACT_CMD:
            rtk_notify_btcoex_to_wifi(IGNORE_WLAN_ACTIVE_CONTROL, status);
            break;

        case HCI_VENDOR_SUB_CMD_SET_BT_PSD_MODE:
            rtk_notify_btcoex_to_wifi(BT_PSD_MODE_CONTROL, status);
            break;

        case HCI_VENDOR_SUB_CMD_SET_BT_LNA_CONSTRAINT:
            rtk_notify_btcoex_to_wifi(LNA_CONSTRAIN_CONTROL, status);
            break;

        case HCI_VENDOR_SUB_CMD_BT_AUTO_REPORT_ENABLE:
            break;

        case HCI_VENDOR_SUB_CMD_BT_SET_TXRETRY_REPORT_PARAM:
            break;

        case HCI_VENDOR_SUB_CMD_BT_SET_PTATABLE:
            break;

        case HCI_VENDOR_SUB_CMD_GET_AFH_MAP_L:
        {
            if(status == 0)//success
            {
                memcpy(rtk_prof.afh_map, p+4, 4); //cmd_idx, length, piconet_id, mode
                uint8_t temp_cmd[4];
                temp_cmd[0] = HCI_VENDOR_SUB_CMD_GET_AFH_MAP_M;
                temp_cmd[1] = 2;
                temp_cmd[2] = rtk_prof.piconet_id;
                temp_cmd[3] = rtk_prof.mode;
                rtk_vendor_cmd_to_fw(HCI_VENDOR_MAILBOX_CMD, 4, temp_cmd, NULL);
            }
            else //fail
            {
                memset(rtk_prof.afh_map, 0, 10);
                rtk_notify_afhmap_to_wifi();
            }
            break;
        }
        case HCI_VENDOR_SUB_CMD_GET_AFH_MAP_M:
        {
            if(status == 0)//success
            {
                memcpy(rtk_prof.afh_map+4, p+4, 4);
                uint8_t temp_cmd[4];
                temp_cmd[0] = HCI_VENDOR_SUB_CMD_GET_AFH_MAP_H;
                temp_cmd[1] = 2;
                temp_cmd[2] = rtk_prof.piconet_id;
                temp_cmd[3] = rtk_prof.mode;
                rtk_vendor_cmd_to_fw(HCI_VENDOR_MAILBOX_CMD, 4, temp_cmd, NULL);
            }
            else //fail
            {
                memset(rtk_prof.afh_map, 0, 10);
                rtk_notify_afhmap_to_wifi();
            }
            break;
        }

        case HCI_VENDOR_SUB_CMD_GET_AFH_MAP_H:
        {
            if(status == 0)
                memcpy(rtk_prof.afh_map+8, p+4, 2);
            else
                memset(rtk_prof.afh_map, 0, 10);

            rtk_notify_afhmap_to_wifi();
            break;
        }

        case HCI_VENDOR_SUB_CMD_RD_REG_REQ:
        {
            if(status == 0)
                rtk_notify_regester_to_wifi(p+3);//cmd_idx,length,regist type
            break;
        }

        case HCI_VENDOR_SUB_CMD_WR_REG_REQ:
            rtk_notify_btcoex_to_wifi(BT_REGISTER_ACCESS, status);
            break;

        default:
            break;
    }
}

static void rtk_handle_cmd_complete_evt(uint8_t*p, uint8_t len)
{
    uint16_t opcode;
    uint8_t status;
    p++;
    STREAM_TO_UINT16(opcode, p);
    switch (opcode)
    {
        case HCI_PERIODIC_INQUIRY_MODE:
        {
            status = *p++;
            if(status && rtk_prof.isinquirying)
            {
                rtk_prof.isinquirying = 0;
                RtkLogMsg("HCI_PERIODIC_INQUIRY_MODE start error, notify wifi inquiry stop");
                rtk_notify_btoperation_to_wifi(BT_OPCODE_INQUIRY_END, 0, NULL);
            }
            break;
        }

        case HCI_READ_LOCAL_VERSION_INFO:
        {
            status = *p++;
            if(!status)
            {
                p++;
                STREAM_TO_UINT16(rtk_prof.hci_reversion, p);
                p+=3;
                STREAM_TO_UINT16(rtk_prof.lmp_subversion, p);
            }
            RtkLogMsg("rtk_prof.hci_reversion = %x", rtk_prof.hci_reversion);
            RtkLogMsg("rtk_prof.lmp_subversion = %x", rtk_prof.lmp_subversion);
            break;
        }

        case HCI_RESET:
        {
            RtkLogMsg("bt start ok");
            coex_msg_send(invite_req, sizeof(invite_req));
#if 0
            if(create_netlink_socket() == 0)
            {
                RtkLogMsg("wifi is already on when bt turn on");
                rtk_prof.wifi_on = 1;
                netlink_send(rtk_prof.nlsocket, invite_req);
            }
            else
                RtkLogMsg("wifi is off when bt turn on, wait for wifi turning on...");
#endif
            break;
        }

        case 0xfc1b:
            RtkLogMsg("received cmd complete event for fc1b");
            poweroff_allowed = 1;
            break;

        case HCI_VENDOR_MAILBOX_CMD:
            rtk_handle_vender_mailbox_cmp_evt(p, len);
            break;

        case HCI_VENDOR_ADD_BITPOOL_FW:
            status = *p++;
            RtkLogMsg("received cmd complete event for HCI_VENDOR_ADD_BITPOOL_FW status:%d",status);

        default:
            break;
    }
}

static void rtk_handle_connection_complete_evt(uint8_t* p)
{
    uint8_t status = 0;
    uint16_t handle = 0;
    status = *p++;
    STREAM_TO_UINT16 (handle, p);
    p +=6;
    uint8_t link_type = *p++;

    if(status == 0)
    {
        if(rtk_prof.ispaging)
        {
            rtk_prof.ispaging = 0;
            RtkLogMsg("notify wifi page success end");
            rtk_notify_btoperation_to_wifi(BT_OPCODE_PAGE_SUCCESS_END, 0, NULL);
        }

        tRTK_CONN_PROF* hci_conn = find_connection_by_handle(&rtk_prof, handle);
        if(hci_conn == NULL)
        {
            hci_conn = allocate_connection_by_handle(handle);
            if(hci_conn)
            {
                add_connection_to_hash(&rtk_prof, hci_conn);
                hci_conn->profile_bitmap = 0;
                memset(hci_conn->profile_refcount, 0, 8);
                if((0 == link_type) ||(2 == link_type))//sco or esco
                {
                    hci_conn->type = 1;
                    update_profile_connection(hci_conn, profile_sco, TRUE);
                }
                else
                    hci_conn->type = 0;
            }
            else
            {
                ALOGE("HciConnAllocate fail");
            }
        }
        else
        {
            RtkLogMsg("HCI Connection handle(0x%x) has already exist!", handle);
            hci_conn->profile_bitmap = 0;
            memset(hci_conn->profile_refcount, 0, 8);
            if((0 == link_type)||(2 == link_type))//sco or esco
            {
                hci_conn->type = 1;
                update_profile_connection(hci_conn, profile_sco, TRUE);
            }
            else
                hci_conn->type = 0;
        }
    }
    else if(rtk_prof.ispaging)
    {
        rtk_prof.ispaging = 0;
        RtkLogMsg("notify wifi page unsuccess end");
        rtk_notify_btoperation_to_wifi(BT_OPCODE_PAGE_UNSUCCESS_END, 0, NULL);
    }
}

static void rtk_handle_disconnect_complete_evt(uint8_t* p)
{
    if(rtk_prof.ispairing)//for slave: connection will be disconnected if authentication fail
    {
        rtk_prof.ispairing = 0;
        RtkLogMsg("notify wifi pair end");
        rtk_notify_btoperation_to_wifi(BT_OPCODE_PAIR_END, 0, NULL);
    }

    uint8_t status = 0;
    uint16_t handle = 0;
    uint8_t reason = 0;
    status = *p++;
    STREAM_TO_UINT16(handle, p);
    reason = *p;

    if(status == 0)
    {
        tRTK_CONN_PROF *hci_conn = find_connection_by_handle(&rtk_prof, handle);
        if(hci_conn)
        {
            switch(hci_conn->type)
            {
                case 0:
                {
                    RT_LIST_ENTRY* iter = NULL, *temp = NULL;
                    tRTK_PROF_INFO* prof_info = NULL;

                    pthread_mutex_lock(&rtk_prof.profile_mutex);
                    LIST_FOR_EACH_SAFELY(iter, temp, &rtk_prof.profile_list)
                    {
                        prof_info = LIST_ENTRY(iter, tRTK_PROF_INFO, list);
                        if ((handle == prof_info->handle) && prof_info->scid && prof_info->dcid)
                        {
                            RtkLogMsg("find info when hci disconnect, handle:%x, psm:%x, dcid:%x, scid:%x", prof_info->handle, prof_info->psm, prof_info->dcid, prof_info->scid);
                            //If both scid and dcid > 0, L2cap connection is exist.
                            update_profile_connection(hci_conn, prof_info->profile_index, FALSE);
                            delete_profile_from_hash(prof_info);
                        }
                    }
                    pthread_mutex_unlock(&rtk_prof.profile_mutex);
                    break;
                }

                case 1:
                    update_profile_connection(hci_conn, profile_sco, FALSE);
                    break;

                case 2:
                {
                    if(hci_conn->profile_bitmap & BIT(profile_hogp))
                        update_profile_connection(hci_conn, profile_hogp, FALSE);

                    if(hci_conn->profile_bitmap & BIT(profile_voice))
                        update_profile_connection(hci_conn, profile_voice, FALSE);

                    update_profile_connection(hci_conn, profile_hid, FALSE);
                    break;
                }

                default:
                    break;
            }
            delete_connection_from_hash(hci_conn);
        }
        else
        {
            ALOGE("HCI Connection handle(0x%x) not found", handle);
        }
    }
}

static void rtk_handle_le_connection_complete_evt(uint8_t* p, bool enhanced)
{
    uint16_t handle, interval;
    uint8_t status;
    tRTK_CONN_PROF* hci_conn = NULL;

    status = *p++;
    STREAM_TO_UINT16 (handle, p);
    p += 8; //role, address type, address
    if(enhanced) {
        p += 12;
    }
    STREAM_TO_UINT16 (interval, p);

    if(status == 0) {
        if(rtk_prof.ispaging){
            rtk_prof.ispaging = 0;
            RtkLogMsg("notify wifi page success end");
            rtk_notify_btoperation_to_wifi(BT_OPCODE_PAGE_SUCCESS_END, 0, NULL);
        }

        hci_conn = find_connection_by_handle(&rtk_prof, handle);
        if(hci_conn == NULL) {
            hci_conn = allocate_connection_by_handle(handle);
            if(hci_conn) {
                add_connection_to_hash(&rtk_prof, hci_conn);
                hci_conn->profile_bitmap = 0;
                memset(hci_conn->profile_refcount, 0, 8);
                hci_conn->type = 2;
                update_profile_connection(hci_conn, profile_hid, TRUE); //for coex, le is the same as hid
                update_hid_active_state(handle, interval);
            } else {
                ALOGE("hci connection allocate fail");
            }
        } else {
            RtkLogMsg("hci connection handle(0x%x) has already exist!", handle);
            hci_conn->profile_bitmap = 0;
            memset(hci_conn->profile_refcount, 0, 8);
            hci_conn->type = 2;
            update_profile_connection(hci_conn, profile_hid, TRUE);
            update_hid_active_state(handle, interval);
        }
    } else if(rtk_prof.ispaging) {
        rtk_prof.ispaging = 0;
        RtkLogMsg("notify wifi page unsuccess end");
        rtk_notify_btoperation_to_wifi(BT_OPCODE_PAGE_UNSUCCESS_END, 0, NULL);
    }
}

static void rtk_handle_le_connection_update_complete_evt(uint8_t* p)
{
    uint16_t handle, interval;
    uint8_t status;

    status = *p++;
    STREAM_TO_UINT16 (handle, p);
    STREAM_TO_UINT16 (interval, p);
    update_hid_active_state(handle, interval);
}

static void rtk_handle_le_meta_evt(uint8_t* p)
{
    uint8_t sub_event = *p++;
    switch (sub_event) {
    case HCI_BLE_CONN_COMPLETE_EVT:
        rtk_handle_le_connection_complete_evt(p, false);
        break;
    case HCI_BLE_ENHANCED_CONN_COMPLETE_EVT:
        rtk_handle_le_connection_complete_evt(p, true);
        break;
    case HCI_BLE_LL_CONN_PARAM_UPD_EVT:
        rtk_handle_le_connection_update_complete_evt(p);
        break;

    default :
        break;
    }
}

static int coex_msg_send(char *tx_msg, int msg_size)
{
    int ret = -1;
    if(rtk_prof.udpsocket > 0) {
        ret = udpsocket_send(tx_msg, msg_size);
    }
    else if(rtk_prof.btcoex_chr > 0) {
        ret = btcoex_chr_send(tx_msg, msg_size);
    }
    return ret;

}

static int coex_msg_recv(uint8_t *recv_msg, uint8_t *msg_size)
{
    int ret = -1;
    if(rtk_prof.udpsocket > 0) {
        ret = udpsocket_recv(recv_msg, msg_size);
    }
    else if(rtk_prof.btcoex_chr > 0) {
        ret = btcoex_chr_recv(recv_msg, msg_size);
    }
    return ret;
}
void rtk_parse_internal_event_intercept(uint8_t *p_msg)
{
    //ALOGE("in rtk_parse_internal_event_intercept, *p= %x", *p);
    uint8_t *p = p_msg;
    uint8_t event_code = *p++;
    uint8_t len = *p++;
    uint16_t opcode, mode_change_handle, mode_interval, subcode;
    uint8_t status, num_hci_cmd_pkts;

    switch (event_code)
    {
        case HCI_INQUIRY_COMP_EVT:
        {
            if(rtk_prof.isinquirying)
            {
                rtk_prof.isinquirying = 0;
                RtkLogMsg("notify wifi inquiry end");
                rtk_notify_btoperation_to_wifi(BT_OPCODE_INQUIRY_END, 0, NULL);
            }
            break;
        }

        case HCI_PIN_CODE_REQUEST_EVT:
        {
            if(!rtk_prof.ispairing)
            {
                rtk_prof.ispairing = 1;
                RtkLogMsg("notify wifi pair start");
                rtk_notify_btoperation_to_wifi(BT_OPCODE_PAIR_START, 0, NULL);
            }
            break;
        }

        case HCI_IO_CAPABILITY_REQUEST_EVT:
        {
            if(!rtk_prof.ispairing)
            {
                rtk_prof.ispairing = 1;
                RtkLogMsg("notify wifi pair start");
                rtk_notify_btoperation_to_wifi(BT_OPCODE_PAIR_START, 0, NULL);
            }
            break;
        }

        case HCI_AUTHENTICATION_COMP_EVT:
        {
            if(rtk_prof.ispairing)
            {
                rtk_prof.ispairing = 0;
                RtkLogMsg("notify wifi pair end");
                rtk_notify_btoperation_to_wifi(BT_OPCODE_PAIR_END, 0, NULL);
            }
            break;
        }

        case HCI_LINK_KEY_NOTIFICATION_EVT:
        {
            if(rtk_prof.ispairing)
            {
                rtk_prof.ispairing = 0;
                RtkLogMsg("notify wifi pair end");
                rtk_notify_btoperation_to_wifi(BT_OPCODE_PAIR_END, 0, NULL);
            }
            break;
        }

        case HCI_MODE_CHANGE_EVT:
        {
            status = *p++;
            STREAM_TO_UINT16(mode_change_handle, p);
            p++;
            STREAM_TO_UINT16(mode_interval, p);
            update_hid_active_state(mode_change_handle, mode_interval);
            break;
        }

        case HCI_COMMAND_COMPLETE_EVT:
            rtk_handle_cmd_complete_evt(p, len);
            break;

        case HCI_COMMAND_STATUS_EVT:
        {
            status = *p++;
            num_hci_cmd_pkts = *p++;
            STREAM_TO_UINT16(opcode, p);
            if((opcode == HCI_INQUIRY) && (status))
            {
                if(rtk_prof.isinquirying)
                {
                    rtk_prof.isinquirying = 0;
                    RtkLogMsg("inquiry start error, notify wifi inquiry stop");
                    rtk_notify_btoperation_to_wifi(BT_OPCODE_INQUIRY_END, 0, NULL);
                }
            }

            if(opcode == HCI_CREATE_CONNECTION)
            {
                if(!status && !rtk_prof.ispaging)
                {
                    rtk_prof.ispaging = 1;
                    RtkLogMsg("notify wifi start page");
                    rtk_notify_btoperation_to_wifi(BT_OPCODE_PAGE_START, 0, NULL);
                }
            }
            break;
        }

        case HCI_CONNECTION_COMP_EVT:
        case HCI_ESCO_CONNECTION_COMP_EVT:
            rtk_handle_connection_complete_evt(p);
            break;

        case HCI_DISCONNECTION_COMP_EVT:
            rtk_handle_disconnect_complete_evt(p);
            break;

        case HCI_VENDOR_SPECIFIC_EVT:
        {
            STREAM_TO_UINT16(subcode, p);
            if(subcode == HCI_VENDOR_PTA_AUTO_REPORT_EVENT)
            {
                RtkLogMsg("notify wifi driver with autoreport data");
                if((len-2) != 8)
                    RtkLogMsg("rtk_parse_internal_event_intercept:HCI_VENDOR_SPECIFIC_EVT:HCI_VENDOR_PTA_AUTO_REPORT_EVENT len=%d", len);
                rtk_notify_info_to_wifi(AUTO_REPORT, (len-2), (uint8_t *)p);
            }
            break;
        }

        case HCI_BLE_EVENT:
            rtk_handle_le_meta_evt(p);
            break;

        default:
            break;
    }
}

void rtk_parse_command(uint8_t *pp)
{
    uint8_t *p = pp;
    uint16_t cmd;
    STREAM_TO_UINT16(cmd, p);

    switch (cmd)
    {
        case HCI_INQUIRY:
        case HCI_PERIODIC_INQUIRY_MODE:
        {
            if(!rtk_prof.isinquirying)
            {
                rtk_prof.isinquirying = 1;
                RtkLogMsg("notify wifi inquiry start");
                rtk_notify_btoperation_to_wifi(BT_OPCODE_INQUIRY_START, 0, NULL);
            }
            break;
        }

        case HCI_INQUIRY_CANCEL:
        case HCI_EXIT_PERIODIC_INQUIRY_MODE:
        {
            if(rtk_prof.isinquirying)
            {
                rtk_prof.isinquirying = 0;
                RtkLogMsg("notify wifi inquiry stop");
                rtk_notify_btoperation_to_wifi(BT_OPCODE_INQUIRY_END, 0, NULL);
            }
            break;
        }

        case HCI_ACCEPT_CONNECTION_REQUEST:
        {
            if(!rtk_prof.ispaging)
            {
                rtk_prof.ispaging = 1;
                RtkLogMsg("notify wifi page start");
                rtk_notify_btoperation_to_wifi(BT_OPCODE_PAGE_START, 0, NULL);
            }
            break;
        }

        default:
            break;
    }
}

void rtk_parse_l2cap_data(uint8_t *pp, uint8_t direction)
{
    uint16_t handle, total_len, pdu_len, channel_ID, command_len, psm, scid, dcid, result, status;
    uint8_t flag, code, identifier;
    STREAM_TO_UINT16 (handle, pp);
    flag = (handle >> HCI_DATA_EVENT_OFFSET) & HCI_DATA_EVENT_MASK;
    handle = handle & 0x0FFF;
    STREAM_TO_UINT16 (total_len, pp);
    STREAM_TO_UINT16 (pdu_len, pp);
    STREAM_TO_UINT16 (channel_ID, pp);

    if(flag != RTK_START_PACKET_BOUNDARY)
      return;

    if(channel_ID == 0x0001)
    {
        code = (uint8_t)(*pp++);
        switch (code)
        {
            case L2CAP_CONNECTION_REQ:
                identifier = (uint8_t)(*pp++);
                STREAM_TO_UINT16 (command_len, pp);
                STREAM_TO_UINT16 (psm, pp);
                STREAM_TO_UINT16 (scid, pp);
                RtkLogMsg("L2CAP_CONNECTION_REQ, handle=%x, PSM=%x, scid=%x", handle, psm, scid);
                handle_l2cap_con_req(handle, psm, scid, direction);
                break;

            case L2CAP_CONNECTION_RSP:
                identifier = (uint8_t)(*pp++);
                STREAM_TO_UINT16 (command_len, pp);
                STREAM_TO_UINT16 (dcid, pp);
                STREAM_TO_UINT16 (scid, pp);
                STREAM_TO_UINT16 (result, pp);
                STREAM_TO_UINT16 (status, pp);
                RtkLogMsg("L2CAP_CONNECTION_RESP, handle=%x, dcid=%x, scid=%x, result=%x", handle, dcid, scid, result);
                //if(result == 0)
                    handle_l2cap_con_rsp(handle, dcid, scid, direction, result);
                break;

            case L2CAP_DISCONNECTION_REQ:
                identifier = (uint8_t)(*pp++);
                STREAM_TO_UINT16 (command_len, pp);
                STREAM_TO_UINT16 (dcid, pp);
                STREAM_TO_UINT16 (scid, pp);
                RtkLogMsg("L2CAP_DISCONNECTION_REQ, handle=%x, dcid=%x, scid=%x",handle, dcid, scid);
                handle_l2cap_discon_req(handle, dcid, scid, direction);
                break;

            case L2CAP_DISCONNECTION_RSP:
                break;

            default:
                break;
        }
    }
    else
    {
        if((flag != 0x01)&&(is_profile_connected(profile_a2dp) || is_profile_connected(profile_pan)))//Do not count the continuous packets
            packets_count(handle, channel_ID, pdu_len, direction, pp);
    }
}

void rtk_add_le_profile(BD_ADDR bdaddr, uint16_t handle, uint8_t profile_map)
{
    RTK_UNUSED(bdaddr);
    RtkLogMsg("rtk_add_le_profile, handle is %x, profile_map is %x", handle, profile_map);

    tRTK_CONN_PROF* hci_conn = find_connection_by_handle(&rtk_prof, handle);
    if(hci_conn)
    {
        if((profile_map & 0x01) || (profile_map & 0x02))//bit0: mouse, bit1:keyboard
            update_profile_connection(hci_conn, profile_hogp, TRUE);

        if(profile_map & 0x04)
            update_profile_connection(hci_conn, profile_voice, TRUE);
    }
    else
    {
        ALOGE("rtk_add_le_profile, connection handle(0x%x) not exist!", handle);
    }
}

void rtk_delete_le_profile(BD_ADDR bdaddr, uint16_t handle, uint8_t profile_map)
{
    RTK_UNUSED(bdaddr);
    RtkLogMsg("rtk_delete_le_profile, handle is %x, profile_map is %x", handle, profile_map);

    pthread_mutex_lock(&rtk_prof.profile_mutex);
    tRTK_CONN_PROF* hci_conn = find_connection_by_handle(&rtk_prof, handle);
    if(hci_conn == NULL)
    {
        ALOGE("rtk_delete_le_profile, hci_conn not exist with handle %x", handle);
    }
    else
    {
        if((profile_map & 0x01) || (profile_map & 0x02))//bit0: mouse, bit1:keyboard
            update_profile_connection(hci_conn, profile_hogp, FALSE);

        if(profile_map & 0x04)
            update_profile_connection(hci_conn, profile_voice, FALSE);
    }
    pthread_mutex_unlock(&rtk_prof.profile_mutex);
}

void rtk_add_le_data_count(uint8_t data_type)
{
    RtkLogMsg("rtk_add_le_data_count, data_type is %x", data_type);

    if((data_type == 1) || (data_type == 2))//1:keyboard, 2:mouse
    {
        rtk_prof.hogp_packet_count++;
        if(!is_profile_busy(profile_hogp))
        {
            RtkLogMsg("hogp idle->busy");
            update_profile_state(profile_hogp, TRUE);
        }
    }

    if(data_type == 3)//voice
    {
        rtk_prof.voice_packet_count ++;
        if(!is_profile_busy(profile_voice))
        {
            RtkLogMsg("voice idle->busy");
            update_profile_state(profile_voice, TRUE);
        }
    }
}

void rtk_set_bt_on(uint8_t bt_on) {
    RtkLogMsg("bt stack is init");
    rtk_prof.bt_on = bt_on;
    uint8_t ttmp[1] = {1};
    rtk_vendor_cmd_to_fw(0xfc1b, 1, ttmp, NULL);
}

static rtk_parse_manager_t parse_interface = {
  rtk_parse_internal_event_intercept,
  rtk_parse_l2cap_data,
  rtk_parse_init,
  rtk_parse_cleanup,
  rtk_parse_command,
  rtk_add_le_profile,
  rtk_delete_le_profile,
  rtk_add_le_data_count,
  rtk_set_bt_on,
};

rtk_parse_manager_t *rtk_parse_manager_get_interface() {
  return &parse_interface;
}


