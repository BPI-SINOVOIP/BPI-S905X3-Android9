/******************************************************************************
 *
 *  Copyright (C) 2009-2018 Realtek Corporation
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
 *  Filename:      userial_vendor.c
 *
 *  Description:   Contains vendor-specific userial functions
 *
 ******************************************************************************/
#undef NDEBUG
#define LOG_TAG "bt_userial_vendor"

#include <utils/Log.h>
#include <termios.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <sys/eventfd.h>
#include "userial.h"
#include "userial_vendor.h"
#include "rtk_socket.h"

#ifdef CONFIG_SCO_OVER_HCI
#include "sbc.h"
#endif
/******************************************************************************
**  Constants & Macros
******************************************************************************/

#ifndef VNDUSERIAL_DBG
#define VNDUSERIAL_DBG TRUE
#endif

#if (VNDUSERIAL_DBG == TRUE)
#define VNDUSERIALDBG(param, ...) {ALOGD(param, ## __VA_ARGS__);}
#else
#define VNDUSERIALDBG(param, ...) {}
#endif

#define VND_PORT_NAME_MAXLEN    256

#ifndef BT_CHIP_HW_FLOW_CTRL_ON
#define BT_CHIP_HW_FLOW_CTRL_ON TRUE
#endif

/******************************************************************************
**  Extern functions
******************************************************************************/
extern char rtkbt_transtype;
extern void Heartbeat_cleanup();
extern void Heartbeat_init();

/******************************************************************************
**  Local type definitions
******************************************************************************/
#if !defined(EFD_SEMAPHORE)
#  define EFD_SEMAPHORE (1 << 0)
#endif

#define RTK_DATA_RECEIVED 1
#define RTK_DATA_SEND     0
struct rtk_object_t {
  int fd;                              // the file descriptor to monitor for events.
  void *context;                       // a context that's passed back to the *_ready functions..
  pthread_mutex_t lock;                // protects the lifetime of this object and all variables.

  void (*read_ready)(void *context);   // function to call when the file descriptor becomes readable.
  void (*write_ready)(void *context);  // function to call when the file descriptor becomes writeable.
};

/* vendor serial control block */
typedef struct
{
    int fd;                     /* fd to Bluetooth device */
    int uart_fd[2];
    int signal_fd[2];
    int epoll_fd;
    int cpoll_fd;
    int event_fd;
    struct termios termios;     /* serial terminal of BT port */
    char port_name[VND_PORT_NAME_MAXLEN];
    pthread_t thread_socket_id;
    pthread_t thread_uart_id;
    pthread_t thread_coex_id;
    bool thread_running;

    RTB_QUEUE_HEAD *recv_data;
    RTB_QUEUE_HEAD *send_data;
    RTB_QUEUE_HEAD *data_order;
    volatile bool  btdriver_state;
} vnd_userial_cb_t;

#ifdef CONFIG_SCO_OVER_HCI
uint16_t btui_msbc_h2[] = {0x0801,0x3801,0xc801,0xf801};
typedef struct
{
    pthread_mutex_t sco_mutex;
    pthread_cond_t  sco_cond;
    pthread_t thread_socket_sco_id;
    pthread_t thread_recv_sco_id;
    pthread_t thread_send_sco_id;
    uint16_t  sco_handle;
    bool thread_sco_running;
    uint16_t voice_settings;
    RTB_QUEUE_HEAD *recv_sco_data;
    RTB_QUEUE_HEAD *send_sco_data;
    unsigned char enc_data[480];
    unsigned int current_pos;
    uint16_t sco_packet_len;
    bool msbc_used;
    int ctrl_fd, data_fd;
    sbc_t sbc_dec, sbc_enc;
    uint32_t pcm_enc_seq;
}sco_cb_t;
#endif

/******************************************************************************
**  Static functions
******************************************************************************/
static void h5_data_ready_cb(serial_data_type_t type, unsigned int total_length);
static uint16_t h5_int_transmit_data_cb(serial_data_type_t type, uint8_t *data, uint16_t length) ;
extern void RTK_btservice_destroyed();
/******************************************************************************
**  Static variables
******************************************************************************/
#ifdef CONFIG_SCO_OVER_HCI
static sco_cb_t sco_cb;
#endif
static vnd_userial_cb_t vnd_userial;
static const hci_h5_t* h5_int_interface;
static int packet_recv_state = RTKBT_PACKET_IDLE;
static unsigned int packet_bytes_need = 0;
static serial_data_type_t current_type = 0;
static struct rtk_object_t rtk_socket_object;
static struct rtk_object_t rtk_coex_object;
static unsigned char h4_read_buffer[2048] = {0};
static int h4_read_length = 0;

static int coex_packet_recv_state = RTKBT_PACKET_IDLE;
static int coex_packet_bytes_need = 0;
static serial_data_type_t coex_current_type = 0;
static unsigned char coex_resvered_buffer[2048] = {0};
static int coex_resvered_length = 0;

#ifdef RTK_HANDLE_EVENT
static int received_packet_state = RTKBT_PACKET_IDLE;
static int received_packet_bytes_need = 0;
static serial_data_type_t recv_packet_current_type = 0;
static unsigned char received_resvered_header[2048] = {0};
static int received_resvered_length = 0;
static rtkbt_version_t rtkbt_version;
static rtkbt_lescn_t  rtkbt_adv_con;
#endif

static rtk_parse_manager_t * rtk_parse_manager = NULL;

static  hci_h5_callbacks_t h5_int_callbacks = {
    .h5_int_transmit_data_cb = h5_int_transmit_data_cb,
    .h5_data_ready_cb = h5_data_ready_cb,
};

static const uint8_t hci_preamble_sizes[] = {
    COMMAND_PREAMBLE_SIZE,
    ACL_PREAMBLE_SIZE,
    SCO_PREAMBLE_SIZE,
    EVENT_PREAMBLE_SIZE
};

/*****************************************************************************
**   Helper Functions
*****************************************************************************/

/*******************************************************************************
**
** Function        userial_to_tcio_baud
**
** Description     helper function converts USERIAL baud rates into TCIO
**                  conforming baud rates
**
** Returns         TRUE/FALSE
**
*******************************************************************************/
uint8_t userial_to_tcio_baud(uint8_t cfg_baud, uint32_t *baud)
{
    if (cfg_baud == USERIAL_BAUD_115200)
        *baud = B115200;
    else if (cfg_baud == USERIAL_BAUD_4M)
        *baud = B4000000;
    else if (cfg_baud == USERIAL_BAUD_3M)
        *baud = B3000000;
    else if (cfg_baud == USERIAL_BAUD_2M)
        *baud = B2000000;
    else if (cfg_baud == USERIAL_BAUD_1M)
        *baud = B1000000;
    else if (cfg_baud == USERIAL_BAUD_1_5M)
        *baud = B1500000;
    else if (cfg_baud == USERIAL_BAUD_921600)
        *baud = B921600;
    else if (cfg_baud == USERIAL_BAUD_460800)
        *baud = B460800;
    else if (cfg_baud == USERIAL_BAUD_230400)
        *baud = B230400;
    else if (cfg_baud == USERIAL_BAUD_57600)
        *baud = B57600;
    else if (cfg_baud == USERIAL_BAUD_19200)
        *baud = B19200;
    else if (cfg_baud == USERIAL_BAUD_9600)
        *baud = B9600;
    else if (cfg_baud == USERIAL_BAUD_1200)
        *baud = B1200;
    else if (cfg_baud == USERIAL_BAUD_600)
        *baud = B600;
    else
    {
        ALOGE( "userial vendor open: unsupported baud idx %i", cfg_baud);
        *baud = B115200;
        return FALSE;
    }

    return TRUE;
}

#if (BT_WAKE_VIA_USERIAL_IOCTL==TRUE)
/*******************************************************************************
**
** Function        userial_ioctl_init_bt_wake
**
** Description     helper function to set the open state of the bt_wake if ioctl
**                  is used. it should not hurt in the rfkill case but it might
**                  be better to compile it out.
**
** Returns         none
**
*******************************************************************************/
void userial_ioctl_init_bt_wake(int fd)
{
    uint32_t bt_wake_state;

    /* assert BT_WAKE through ioctl */
    ioctl(fd, USERIAL_IOCTL_BT_WAKE_ASSERT, NULL);
    ioctl(fd, USERIAL_IOCTL_BT_WAKE_GET_ST, &bt_wake_state);
    VNDUSERIALDBG("userial_ioctl_init_bt_wake read back BT_WAKE state=%i", \
               bt_wake_state);
}
#endif // (BT_WAKE_VIA_USERIAL_IOCTL==TRUE)


/*****************************************************************************
**   Userial Vendor API Functions
*****************************************************************************/

/*******************************************************************************
**
** Function        userial_vendor_init
**
** Description     Initialize userial vendor-specific control block
**
** Returns         None
**
*******************************************************************************/
void userial_vendor_init(char *bt_device_node)
{
    memset(&rtkbt_adv_con, 0, sizeof(rtkbt_lescn_t));
    vnd_userial.fd = -1;
    char value[100];
    snprintf(vnd_userial.port_name, VND_PORT_NAME_MAXLEN, "%s", \
            bt_device_node);
    if(rtkbt_transtype & RTKBT_TRANS_H5) {
        h5_int_interface = hci_get_h5_int_interface();
        h5_int_interface->h5_int_init(&h5_int_callbacks);
    }
    rtk_parse_manager = NULL;
    property_get("persist.vendor.bluetooth.rtkcoex", value, "true");
    if(strncmp(value, "true", 4) == 0) {
        rtk_parse_manager = rtk_parse_manager_get_interface();
        rtk_parse_manager->rtk_parse_init();
    }
    vnd_userial.data_order = RtbQueueInit();
    vnd_userial.recv_data = RtbQueueInit();
    vnd_userial.send_data = RtbQueueInit();

    //reset coex gloable variables
    coex_packet_recv_state = RTKBT_PACKET_IDLE;
    coex_packet_bytes_need = 0;
    coex_current_type = 0;
    coex_resvered_length = 0;

#ifdef RTK_HANDLE_EVENT
    //reset handle event gloable variables
    received_packet_state = RTKBT_PACKET_IDLE;
    received_packet_bytes_need = 0;
    recv_packet_current_type = 0;
    received_resvered_length = 0;
#endif

#ifdef CONFIG_SCO_OVER_HCI
    sco_cb.recv_sco_data = RtbQueueInit();
    sco_cb.send_sco_data = RtbQueueInit();
    pthread_mutex_init(&sco_cb.sco_mutex, NULL);
    pthread_cond_init(&sco_cb.sco_cond, NULL);
    memset(&sco_cb.sbc_enc, 0, sizeof(sbc_t));
    sbc_init_msbc(&sco_cb.sbc_enc, 0L);
    sco_cb.sbc_enc.endian = SBC_LE;
    memset(&sco_cb.sbc_dec, 0, sizeof(sbc_t));
    sbc_init_msbc(&sco_cb.sbc_dec, 0L);
    sco_cb.sbc_dec.endian = SBC_LE;
#endif
}


/*******************************************************************************
**
** Function        userial_vendor_open
**
** Description     Open the serial port with the given configuration
**
** Returns         device fd
**
*******************************************************************************/
int userial_vendor_open(tUSERIAL_CFG *p_cfg)
{
    uint32_t baud;
    uint8_t data_bits;
    uint16_t parity;
    uint8_t stop_bits;

    vnd_userial.fd = -1;

    if (!userial_to_tcio_baud(p_cfg->baud, &baud))
    {
        return -1;
    }

    if(p_cfg->fmt & USERIAL_DATABITS_8)
        data_bits = CS8;
    else if(p_cfg->fmt & USERIAL_DATABITS_7)
        data_bits = CS7;
    else if(p_cfg->fmt & USERIAL_DATABITS_6)
        data_bits = CS6;
    else if(p_cfg->fmt & USERIAL_DATABITS_5)
        data_bits = CS5;
    else
    {
        ALOGE("userial vendor open: unsupported data bits");
        return -1;
    }

    if(p_cfg->fmt & USERIAL_PARITY_NONE)
        parity = 0;
    else if(p_cfg->fmt & USERIAL_PARITY_EVEN)
        parity = PARENB;
    else if(p_cfg->fmt & USERIAL_PARITY_ODD)
        parity = (PARENB | PARODD);
    else
    {
        ALOGE("userial vendor open: unsupported parity bit mode");
        return -1;
    }

    if(p_cfg->fmt & USERIAL_STOPBITS_1)
        stop_bits = 0;
    else if(p_cfg->fmt & USERIAL_STOPBITS_2)
        stop_bits = CSTOPB;
    else
    {
        ALOGE("userial vendor open: unsupported stop bits");
        return -1;
    }

    ALOGI("userial vendor open: opening %s", vnd_userial.port_name);

    if ((vnd_userial.fd = open(vnd_userial.port_name, O_RDWR)) == -1)
    {
        ALOGE("userial vendor open: unable to open %s", vnd_userial.port_name);
        return -1;
    }

    tcflush(vnd_userial.fd, TCIOFLUSH);

    tcgetattr(vnd_userial.fd, &vnd_userial.termios);
    cfmakeraw(&vnd_userial.termios);

    if(p_cfg->hw_fctrl == USERIAL_HW_FLOW_CTRL_ON)
    {
        ALOGI("userial vendor open: with HW flowctrl ON");
        vnd_userial.termios.c_cflag |= (CRTSCTS | stop_bits| parity);
    }
    else
    {
        ALOGI("userial vendor open: with HW flowctrl OFF");
        vnd_userial.termios.c_cflag &= ~CRTSCTS;
        vnd_userial.termios.c_cflag |= (stop_bits| parity);

    }

    tcsetattr(vnd_userial.fd, TCSANOW, &vnd_userial.termios);
    tcflush(vnd_userial.fd, TCIOFLUSH);

    tcsetattr(vnd_userial.fd, TCSANOW, &vnd_userial.termios);
    tcflush(vnd_userial.fd, TCIOFLUSH);
    tcflush(vnd_userial.fd, TCIOFLUSH);

    /* set input/output baudrate */
    cfsetospeed(&vnd_userial.termios, baud);
    cfsetispeed(&vnd_userial.termios, baud);
    tcsetattr(vnd_userial.fd, TCSANOW, &vnd_userial.termios);


#if (BT_WAKE_VIA_USERIAL_IOCTL==TRUE)
    userial_ioctl_init_bt_wake(vnd_userial.fd);
#endif

    vnd_userial.btdriver_state = true;
    ALOGI("device fd = %d open", vnd_userial.fd);

    return vnd_userial.fd;
}

static void userial_socket_close(void)
{
    int result;

    if ((result = close(vnd_userial.uart_fd[0])) < 0)
        ALOGE( "%s (fd:%d) FAILED result:%d", __func__, vnd_userial.uart_fd[0], result);

    if (epoll_ctl(vnd_userial.epoll_fd, EPOLL_CTL_DEL, vnd_userial.uart_fd[1], NULL) == -1)
      ALOGE("%s unable to unregister fd %d from epoll set: %s", __func__, vnd_userial.uart_fd[1], strerror(errno));

    if (epoll_ctl(vnd_userial.epoll_fd, EPOLL_CTL_DEL, vnd_userial.signal_fd[1], NULL) == -1)
      ALOGE("%s unable to unregister signal fd %d from epoll set: %s", __func__, vnd_userial.signal_fd[1], strerror(errno));

    if ((result = close(vnd_userial.uart_fd[1])) < 0)
        ALOGE( "%s (fd:%d) FAILED result:%d", __func__, vnd_userial.uart_fd[1], result);

    pthread_join(vnd_userial.thread_socket_id, NULL);
    close(vnd_userial.epoll_fd);

    if ((result = close(vnd_userial.signal_fd[0])) < 0)
        ALOGE( "%s (signal fd[0]:%d) FAILED result:%d", __func__, vnd_userial.signal_fd[0], result);
    if ((result = close(vnd_userial.signal_fd[1])) < 0)
        ALOGE( "%s (signal fd[1]:%d) FAILED result:%d", __func__, vnd_userial.signal_fd[1], result);

    vnd_userial.epoll_fd = -1;
    vnd_userial.uart_fd[0] = -1;
    vnd_userial.uart_fd[1] = -1;
    vnd_userial.signal_fd[0] = -1;
    vnd_userial.signal_fd[1] = -1;
}

static void userial_uart_close(void)
{
    int result;
    if ((result = close(vnd_userial.fd)) < 0)
        ALOGE( "%s (fd:%d) FAILED result:%d", __func__, vnd_userial.fd, result);
    pthread_join(vnd_userial.thread_uart_id, NULL);
}

static void userial_coex_close(void)
{
    int result;

    if (epoll_ctl(vnd_userial.cpoll_fd, EPOLL_CTL_DEL, vnd_userial.event_fd, NULL) == -1)
      ALOGE("%s unable to unregister fd %d from cpoll set: %s", __func__, vnd_userial.event_fd, strerror(errno));

    if (epoll_ctl(vnd_userial.cpoll_fd, EPOLL_CTL_DEL, vnd_userial.signal_fd[1], NULL) == -1)
      ALOGE("%s unable to unregister fd %d from cpoll set: %s", __func__, vnd_userial.signal_fd[1], strerror(errno));

    if ((result = close(vnd_userial.event_fd)) < 0)
        ALOGE( "%s (fd:%d) FAILED result:%d", __func__, vnd_userial.event_fd, result);

    close(vnd_userial.cpoll_fd);
    pthread_join(vnd_userial.thread_coex_id, NULL);
    vnd_userial.cpoll_fd = -1;
    vnd_userial.event_fd = -1;
}

void userial_send_close_signal(void)
{
    unsigned char close_signal = 1;
    ssize_t ret;
    RTK_NO_INTR(ret = write(vnd_userial.signal_fd[0], &close_signal, 1));
}

/*******************************************************************************
**
** Function        userial_vendor_close
**
** Description     Conduct vendor-specific close work
**
** Returns         None
**
*******************************************************************************/
void userial_vendor_close(void)
{
    if (vnd_userial.fd == -1)
        return;

    if((rtkbt_transtype & RTKBT_TRANS_UART) && (rtkbt_transtype & RTKBT_TRANS_H5)) {
#if (BT_WAKE_VIA_USERIAL_IOCTL==TRUE)
        /* de-assert bt_wake BEFORE closing port */
        ioctl(vnd_userial.fd, USERIAL_IOCTL_BT_WAKE_DEASSERT, NULL);
#endif
        //h5_int_interface->h5_int_cleanup();

    }

    vnd_userial.thread_running = false;
    Heartbeat_cleanup();
    RTK_btservice_destroyed();

    userial_send_close_signal();
    userial_uart_close();
    userial_coex_close();
    userial_socket_close();

    if((rtkbt_transtype & RTKBT_TRANS_UART) && (rtkbt_transtype & RTKBT_TRANS_H5)) {
        h5_int_interface->h5_int_cleanup();
    }

    vnd_userial.fd = -1;
    vnd_userial.btdriver_state = false;
    if(rtk_parse_manager) {
        rtk_parse_manager->rtk_parse_cleanup();
    }
    rtk_parse_manager = NULL;
#ifdef CONFIG_SCO_OVER_HCI
    sbc_finish(&sco_cb.sbc_enc);
    sbc_finish(&sco_cb.sbc_dec);
#endif
    ALOGD( "%s finish", __func__);
}

/*******************************************************************************
**
** Function        userial_vendor_set_baud
**
** Description     Set new baud rate
**
** Returns         None
**
*******************************************************************************/
void userial_vendor_set_baud(uint8_t userial_baud)
{
    uint32_t tcio_baud;
    ALOGI("userial_vendor_set_baud");
    userial_to_tcio_baud(userial_baud, &tcio_baud);

    if(cfsetospeed(&vnd_userial.termios, tcio_baud)<0)
        ALOGE("cfsetospeed fail");

    if(cfsetispeed(&vnd_userial.termios, tcio_baud)<0)
        ALOGE("cfsetispeed fail");

    if(tcsetattr(vnd_userial.fd, TCSANOW, &vnd_userial.termios)<0)
        ALOGE("tcsetattr fail ");

    tcflush(vnd_userial.fd, TCIOFLUSH);
}

/*******************************************************************************
**
** Function        userial_vendor_ioctl
**
** Description     ioctl inteface
**
** Returns         None
**
*******************************************************************************/
void userial_vendor_ioctl(userial_vendor_ioctl_op_t op, void *p_data)
{
    RTK_UNUSED(p_data);
    switch(op)
    {
#if (BT_WAKE_VIA_USERIAL_IOCTL==TRUE)
        case USERIAL_OP_ASSERT_BT_WAKE:
            VNDUSERIALDBG("## userial_vendor_ioctl: Asserting BT_Wake ##");
            ioctl(vnd_userial.fd, USERIAL_IOCTL_BT_WAKE_ASSERT, NULL);
            break;

        case USERIAL_OP_DEASSERT_BT_WAKE:
            VNDUSERIALDBG("## userial_vendor_ioctl: De-asserting BT_Wake ##");
            ioctl(vnd_userial.fd, USERIAL_IOCTL_BT_WAKE_DEASSERT, NULL);
            break;

        case USERIAL_OP_GET_BT_WAKE_STATE:
            ioctl(vnd_userial.fd, USERIAL_IOCTL_BT_WAKE_GET_ST, p_data);
            break;
#endif  //  (BT_WAKE_VIA_USERIAL_IOCTL==TRUE)

        default:
            break;
    }
}

/*******************************************************************************
**
** Function        userial_set_port
**
** Description     Configure UART port name
**
** Returns         0 : Success
**                 Otherwise : Fail
**
*******************************************************************************/
int userial_set_port(char *p_conf_name, char *p_conf_value, int param)
{
    RTK_UNUSED(p_conf_name);
    RTK_UNUSED(param);
    strcpy(vnd_userial.port_name, p_conf_value);

    return 0;
}

/*******************************************************************************
**
** Function        userial_vendor_set_hw_fctrl
**
** Description     Conduct vendor-specific close work
**
** Returns         None
**
*******************************************************************************/
void userial_vendor_set_hw_fctrl(uint8_t hw_fctrl)
{
    struct termios termios_old;

    if (vnd_userial.fd == -1)
    {
        ALOGE("vnd_userial.fd is -1");
        return;
    }

    tcgetattr(vnd_userial.fd, &termios_old);
    if(hw_fctrl)
    {
        if(termios_old.c_cflag & CRTSCTS)
        {
            BTVNDDBG("userial_vendor_set_hw_fctrl already hw flowcontrol on");
            return;
        }
        else
        {
            termios_old.c_cflag |= CRTSCTS;
            tcsetattr(vnd_userial.fd, TCSANOW, &termios_old);
            BTVNDDBG("userial_vendor_set_hw_fctrl set hw flowcontrol on");
        }
    }
    else
    {
        if(termios_old.c_cflag & CRTSCTS)
        {
            termios_old.c_cflag &= ~CRTSCTS;
            tcsetattr(vnd_userial.fd, TCSANOW, &termios_old);
            return;
        }
        else
        {
            ALOGI("userial_vendor_set_hw_fctrl set hw flowcontrol off");
            return;
        }
    }
}

static uint16_t h4_int_transmit_data(uint8_t *data, uint16_t total_length) {
    assert(data != NULL);
    assert(total_length > 0);

    uint16_t length = total_length;
    uint16_t transmitted_length = 0;
    while (length > 0 && vnd_userial.btdriver_state) {
        ssize_t ret = write(vnd_userial.fd, data + transmitted_length, length);
        switch (ret) {
            case -1:
            ALOGE("In %s, error writing to the uart serial port: %s", __func__, strerror(errno));
            goto done;
        case 0:
            // If we wrote nothing, don't loop more because we
            // can't go to infinity or beyond, ohterwise H5 can resend data
            ALOGE("%s, ret %zd", __func__, ret);
            goto done;
        default:
            transmitted_length += ret;
            length -= ret;
            break;
        }
    }

done:;
    return transmitted_length;
}

static void userial_enqueue_coex_rawdata(unsigned char * buffer, int length, bool is_recved)
{
    RTK_BUFFER* skb_data = RtbAllocate(length, 0);
    RTK_BUFFER* skb_type = RtbAllocate(1, 0);
    memcpy(skb_data->Data, buffer, length);
    skb_data->Length = length;
    if(is_recved) {
        *skb_type->Data = RTK_DATA_RECEIVED;
        skb_type->Length = 1;
        RtbQueueTail(vnd_userial.recv_data, skb_data);
        RtbQueueTail(vnd_userial.data_order, skb_type);
    }
    else {
        *skb_type->Data = RTK_DATA_SEND;
        skb_type->Length = 1;
        RtbQueueTail(vnd_userial.send_data, skb_data);
        RtbQueueTail(vnd_userial.data_order, skb_type);
    }

    if (eventfd_write(vnd_userial.event_fd, 1) == -1) {
        ALOGE("%s unable to write for coex event fd.", __func__);
    }
}

static int userial_coex_recv_data_handler(unsigned char * recv_buffer, int total_length)
{
    serial_data_type_t type = 0;
    unsigned char * p_data = recv_buffer;
    int length = total_length;
    HC_BT_HDR * p_buf;
    uint8_t boundary_flag;
    uint16_t len, handle, acl_length, l2cap_length;
    switch (coex_packet_recv_state) {
        case RTKBT_PACKET_IDLE:
            coex_packet_bytes_need = 1;
            while(length) {
                type = p_data[0];
                length--;
                p_data++;
                assert((type > DATA_TYPE_COMMAND) && (type <= DATA_TYPE_EVENT));
                if (type < DATA_TYPE_ACL || type > DATA_TYPE_EVENT) {
                    ALOGE("%s invalid data type: %d", __func__, type);
                    if(!length)
                        return total_length;

                    continue;
                }
                break;
            }
            coex_current_type = type;
            coex_packet_recv_state = RTKBT_PACKET_TYPE;
            //fall through

        case RTKBT_PACKET_TYPE:
            if(coex_current_type == DATA_TYPE_ACL) {
                coex_packet_bytes_need = 4;
            }
            else if(coex_current_type == DATA_TYPE_EVENT) {
                coex_packet_bytes_need = 2;
            }
            else {
                coex_packet_bytes_need = 3;
            }
            coex_resvered_length = 0;
            coex_packet_recv_state = RTKBT_PACKET_HEADER;
            //fall through

        case RTKBT_PACKET_HEADER:
            if(length >= coex_packet_bytes_need) {
                memcpy(&coex_resvered_buffer[coex_resvered_length], p_data, coex_packet_bytes_need);
                coex_resvered_length += coex_packet_bytes_need;
                length -= coex_packet_bytes_need;
                p_data += coex_packet_bytes_need;
            }
            else {
                memcpy(&coex_resvered_buffer[coex_resvered_length], p_data, length);
                coex_resvered_length += length;
                coex_packet_bytes_need -= length;
                length = 0;
                return total_length;
            }
            coex_packet_recv_state = RTKBT_PACKET_CONTENT;

            if(coex_current_type == DATA_TYPE_ACL) {
                coex_packet_bytes_need = *(uint16_t *)&coex_resvered_buffer[2];
            }
             else if(coex_current_type == DATA_TYPE_EVENT){
                coex_packet_bytes_need = coex_resvered_buffer[1];
            }
            else {
                coex_packet_bytes_need = coex_resvered_buffer[2];
            }
            //fall through

        case RTKBT_PACKET_CONTENT:
            if(length >= coex_packet_bytes_need) {
                memcpy(&coex_resvered_buffer[coex_resvered_length], p_data, coex_packet_bytes_need);
                length -= coex_packet_bytes_need;
                p_data += coex_packet_bytes_need;
                coex_resvered_length += coex_packet_bytes_need;
                coex_packet_bytes_need = 0;
            }
            else {
                memcpy(&coex_resvered_buffer[coex_resvered_length], p_data, length);
                coex_resvered_length += length;
                coex_packet_bytes_need -= length;
                length = 0;
                return total_length;
            }
            coex_packet_recv_state = RTKBT_PACKET_END;
            //fall through

        case RTKBT_PACKET_END:
            len = BT_HC_HDR_SIZE + coex_resvered_length;
            p_buf = (HC_BT_HDR *) malloc(len);
            p_buf->offset = 0;
            p_buf->layer_specific = 0;
            p_buf->len = coex_resvered_length;
            memcpy((uint8_t *)(p_buf + 1), coex_resvered_buffer, coex_resvered_length);
            switch (coex_current_type) {
                case DATA_TYPE_EVENT:
                    p_buf->event = MSG_HC_TO_STACK_HCI_EVT;
                    if(rtk_parse_manager)
                        rtk_parse_manager->rtk_parse_internal_event_intercept(coex_resvered_buffer);
                break;

                case DATA_TYPE_ACL:
                    p_buf->event = MSG_HC_TO_STACK_HCI_ACL;
                    handle =  *(uint16_t *)coex_resvered_buffer;
                    acl_length = *(uint16_t *)&coex_resvered_buffer[2];
                    l2cap_length = *(uint16_t *)&coex_resvered_buffer[4];
                    boundary_flag = RTK_GET_BOUNDARY_FLAG(handle);
                    if(rtk_parse_manager)
                        rtk_parse_manager->rtk_parse_l2cap_data(coex_resvered_buffer, 0);
                break;

                case DATA_TYPE_SCO:
                    p_buf->event = MSG_HC_TO_STACK_HCI_SCO;
                break;

                default:
                    p_buf->event = MSG_HC_TO_STACK_HCI_ERR;
                break;
            }
            rtk_btsnoop_capture(p_buf, true);
            free(p_buf);
        break;

        default:

        break;
    }

    coex_packet_recv_state = RTKBT_PACKET_IDLE;
    coex_packet_bytes_need = 0;
    coex_current_type = 0;
    coex_resvered_length = 0;

    return (total_length - length);
}

static void userial_coex_send_data_handler(unsigned char * send_buffer, int total_length)
{
    serial_data_type_t type = 0;
    type = send_buffer[0];
    int length = total_length;
    HC_BT_HDR * p_buf;
    uint8_t boundary_flag;
    uint16_t len, handle, acl_length, l2cap_length;

    len = BT_HC_HDR_SIZE + (length - 1);
    p_buf = (HC_BT_HDR *) malloc(len);
    p_buf->offset = 0;
    p_buf->layer_specific = 0;
    p_buf->len = total_length -1;
    memcpy((uint8_t *)(p_buf + 1), &send_buffer[1], length - 1);

    switch (type) {
        case DATA_TYPE_COMMAND:
            p_buf->event = MSG_STACK_TO_HC_HCI_CMD;
            if(rtk_parse_manager)
                rtk_parse_manager->rtk_parse_command(&send_buffer[1]);
        break;

        case DATA_TYPE_ACL:
            p_buf->event = MSG_STACK_TO_HC_HCI_ACL;
            handle =  *(uint16_t *)&send_buffer[1];
            acl_length = *(uint16_t *)&send_buffer[3];
            l2cap_length = *(uint16_t *)&send_buffer[5];
            boundary_flag = RTK_GET_BOUNDARY_FLAG(handle);
            if(rtk_parse_manager)
                rtk_parse_manager->rtk_parse_l2cap_data(&send_buffer[1], 1);

        break;

        case DATA_TYPE_SCO:
            p_buf->event = MSG_STACK_TO_HC_HCI_SCO;
        break;
        default:
            p_buf->event = 0;
            ALOGE("%s invalid data type: %d", __func__, type);
        break;
    }
    rtk_btsnoop_capture(p_buf, false);
    free(p_buf);
}

static void userial_coex_handler(void *context)
{
    RTK_UNUSED(context);
    RTK_BUFFER* skb_data;
    RTK_BUFFER* skb_type;
    eventfd_t value;
    unsigned int read_length = 0;
    eventfd_read(vnd_userial.event_fd, &value);
    if(!value && !vnd_userial.thread_running) {
        return;
    }

    while(!RtbQueueIsEmpty(vnd_userial.data_order)) {
        read_length = 0;
        skb_type = RtbDequeueHead(vnd_userial.data_order);
        if(skb_type) {
            if(*(skb_type->Data) == RTK_DATA_RECEIVED) {
                skb_data = RtbDequeueHead(vnd_userial.recv_data);
                if(skb_data) {
                    do {
                        read_length += userial_coex_recv_data_handler((skb_data->Data + read_length), (skb_data->Length - read_length));
                    }while(read_length < skb_data->Length);
                    RtbFree(skb_data);
                }
            }
            else {
                skb_data = RtbDequeueHead(vnd_userial.send_data);
                if(skb_data) {
                    userial_coex_send_data_handler(skb_data->Data, skb_data->Length);
                    RtbFree(skb_data);
                }

            }

            RtbFree(skb_type);
        }
    }
}

#ifdef CONFIG_SCO_OVER_HCI
//receive sco encode data over hci, we need to decode msbc data to pcm, and send it to sco audio hal
static void* userial_recv_sco_thread(void *arg)
{
    RTK_UNUSED(arg);
    RTK_BUFFER* skb_sco_data;
    unsigned char dec_data[480];
    unsigned char pcm_data[960];
    int index = 0;
    //uint16_t sco_packet_len = 60;
    uint8_t * p_data = NULL;
    int res = 0;
    size_t writen = 0;
    prctl(PR_SET_NAME, (unsigned long)"userial_recv_sco_thread", 0, 0, 0);
    /*
    FILE *file;
    unsigned char enc_data[60];
    file = fopen("/data/misc/bluedroid/sco_capture.raw", "rb");
    FILE *file2;
    file2 = fopen("/data/misc/bluedroid/sco_capture.pcm", "wb");
    if (!file) {
        ALOGE("Unable to create file");
        return NULL;
    }
    */
    RtbEmptyQueue(sco_cb.recv_sco_data);
    ALOGE("userial_recv_sco_thread start");
    while(sco_cb.thread_sco_running) {
        pthread_mutex_lock(&sco_cb.sco_mutex);
        while(RtbQueueIsEmpty(sco_cb.recv_sco_data) && sco_cb.thread_sco_running) {
            pthread_cond_wait(&sco_cb.sco_cond, &sco_cb.sco_mutex);
        }
        pthread_mutex_unlock(&sco_cb.sco_mutex);
        skb_sco_data = RtbDequeueHead(sco_cb.recv_sco_data);
        if(!skb_sco_data)
          continue;
        p_data = skb_sco_data->Data;
        //if (fwrite(skb_sco_data->Data, 1, 60, file) != 60) {
            //ALOGE("Error capturing sample");
        //}
        /*
        if(fread(enc_data, 1, 60, file) > 0) {
            ALOGE("userial_recv_sco_thread, fread data");
            res = sbc_decode(&sco_cb.sbc_dec, &enc_data[2], 58, dec_data, 240, &writen);
        }
        else {
            fseek(file, 0L, SEEK_SET);
            if(fread(enc_data, 1, 60, file) > 0) {
                res = sbc_decode(&sco_cb.sbc_dec, &enc_data[2], 58, dec_data, 240, &writen);
            }
        }
        */
        res = sbc_decode(&sco_cb.sbc_dec, (p_data+2), 58, dec_data, 240, &writen);
        if(res > 0) {
            memcpy(&pcm_data[240 * index], dec_data, 240);
            //if (fwrite(dec_data, 240, 1, file2) != 240) {
                    //ALOGE("Error capturing sample");
            //}
            index = (index + 1) % 4;
            if(index == 0) {
                Skt_Send_noblock(sco_cb.data_fd, pcm_data, 960);
            }
        }
        else {
            ALOGE("msbc decode fail!");
        }
        RtbFree(skb_sco_data);
    }
    ALOGE("userial_recv_sco_thread exit");
    RtbEmptyQueue(sco_cb.recv_sco_data);
    return NULL;
}

static void* userial_send_sco_thread(void *arg)
{
    RTK_UNUSED(arg);
    unsigned char enc_data[240];
    unsigned char pcm_data[960];
    unsigned char send_data[100];
    int writen = 0;
    int num_read;
    prctl(PR_SET_NAME, (unsigned long)"userial_send_sco_thread", 0, 0, 0);
    sco_cb.pcm_enc_seq = 0;
    int i;

    /*
    FILE *file;
    file = fopen("/data/misc/bluedroid/sco_playback.raw", "rb");
    if (!file) {
        ALOGE("Unable to create file");
        return NULL;
    }
    */
    ALOGE("userial_send_sco_thread start");
    while(sco_cb.thread_sco_running) {
        num_read = Skt_Read(sco_cb.data_fd, pcm_data, 960 * 2);
        /*
        for(i = 0; i < 5; i ++) {
            if(fread(&enc_data[4], 1, 48, file) > 0) {
                enc_data[0] = DATA_TYPE_SCO;
                enc_data[3] = 48;
                *(uint16_t *)&enc_data[1] = sco_cb.sco_handle;
                h4_int_transmit_data(enc_data, 52);
            }
            else {
            fseek(file, 0L, SEEK_SET);
            }
        }
        userial_enqueue_coex_rawdata(enc_data,52, false);
        //usleep(7500);
        continue;
        */
        for(i = 0; i < 4; i++) {
            if(sbc_encode(&sco_cb.sbc_enc, &pcm_data[240*i], 240, &enc_data[i*60 +2], 58, (ssize_t *)&writen) <= 0) {
                ALOGE("sbc encode error!");
            }
            else {
                *(uint16_t*)(&(enc_data[i*60])) = btui_msbc_h2[sco_cb.pcm_enc_seq % 4];
                sco_cb.pcm_enc_seq++;
                enc_data[i*60 + 59] = 0x00;    //padding
            }
        }
        for(i = 0; i < 5; i++) {
            send_data[0] = DATA_TYPE_SCO;
            send_data[3] = 48;
            *(uint16_t *)&send_data[1] = sco_cb.sco_handle;
            memcpy(&send_data[4], &enc_data[i*48], 48);
            h4_int_transmit_data(send_data, 52);
            userial_enqueue_coex_rawdata(enc_data, 52, false);
        }
    }
    ALOGE("userial_send_sco_thread exit");
    return NULL;
}

static void userial_sco_socket_stop()
{
    sco_cb.thread_sco_running = false;
    pthread_mutex_lock(&sco_cb.sco_mutex);
    pthread_cond_signal(&sco_cb.sco_cond);
    pthread_mutex_unlock(&sco_cb.sco_mutex);
    pthread_join(sco_cb.thread_socket_sco_id, NULL);
    pthread_join(sco_cb.thread_recv_sco_id, NULL);
    pthread_join(sco_cb.thread_send_sco_id, NULL);
}

static void userial_sco_ctrl_skt_handle()
{
    uint8_t cmd = 0, ack = 0;;
    int result = Skt_Read(sco_cb.ctrl_fd, &cmd, 1);

    if(result == 0) {
        userial_sco_socket_stop();
        return;
    }

    ALOGE("%s, cmd = %d, msbc_used = %d", __func__, cmd, sco_cb.msbc_used);
    switch (cmd) {
        case SCO_CTRL_CMD_CHECK_READY:

        break;

        case SCO_CTRL_CMD_OUT_START:
        {
            pthread_attr_t thread_attr;
            pthread_attr_init(&thread_attr);
            pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_JOINABLE);
            if(pthread_create(&sco_cb.thread_send_sco_id, &thread_attr, userial_send_sco_thread, NULL)!= 0 )
            {
                ALOGE("pthread_create : %s", strerror(errno));
            }
        }
        break;

        case SCO_CTRL_CMD_IN_START:
        {
            pthread_attr_t thread_attr;
            pthread_attr_init(&thread_attr);
            pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_JOINABLE);
            if(pthread_create(&sco_cb.thread_recv_sco_id, &thread_attr, userial_recv_sco_thread, NULL)!= 0 )
            {
                ALOGE("pthread_create : %s", strerror(errno));
            }
        }
        break;

        case SCO_CTRL_CMD_OUT_STOP:

        break;

        case SCO_CTRL_CMD_SUSPEND:

        break;

        case SCO_CTRL_GET_AUDIO_CONFIG:
            if(sco_cb.msbc_used) {
                ack = 2;
                Skt_Send(sco_cb.ctrl_fd, &ack, 1);
            }
            else {
                ack = 1;
                Skt_Send(sco_cb.ctrl_fd, &ack, 1);
            }
        break;

        default:

        break;
    }
}

static void* userial_socket_sco_thread(void *arg)
{
    RTK_UNUSED(arg);
    struct sockaddr_un addr, remote;
    socklen_t alen, len = sizeof(struct sockaddr_un);
    fd_set read_set, active_set;
    int result, max_fd;
    int s_ctrl = socket(AF_LOCAL, SOCK_STREAM, 0);
    int s_data = socket(AF_LOCAL, SOCK_STREAM, 0);
    prctl(PR_SET_NAME, (unsigned long)"userial_socket_sco_thread", 0, 0, 0);

    //bind sco ctrl socket
    unlink(SCO_CTRL_PATH);
    memset(&addr, 0, sizeof(addr));
    strcpy(addr.sun_path, SCO_CTRL_PATH);
    addr.sun_family = AF_LOCAL;
    alen = strlen(addr.sun_path) + offsetof(struct sockaddr_un, sun_path);
    if (bind(s_ctrl, (struct sockaddr *)&addr, alen) < 0) {
        ALOGE("userial_socket_sco_thread, bind ctrl socket error : %s", strerror(errno));
        return NULL;
    }

    if(listen(s_ctrl, 5) < 0) {
        ALOGE("userial_socket_sco_thread, listen ctrl socket error : %s", strerror(errno));
        return NULL;
    }

    chmod(SCO_CTRL_PATH, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
    //bind sco data socket
    unlink(SCO_DATA_PATH);
    memset(&addr, 0, sizeof(addr));
    strcpy(addr.sun_path, SCO_DATA_PATH);
    addr.sun_family = AF_LOCAL;
    alen = strlen(addr.sun_path) + offsetof(struct sockaddr_un, sun_path);
    if (bind(s_data, (struct sockaddr *)&addr, alen) < 0) {
        ALOGE("userial_socket_sco_thread, bind data socket error : %s", strerror(errno));
        return NULL;
    }

    if(listen(s_data, 5) < 0) {
        ALOGE("userial_socket_sco_thread, listen data socket error : %s", strerror(errno));
        return NULL;
    }
    chmod(SCO_DATA_PATH, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);

    ALOGE("userial_socket_sco_thread");
    FD_ZERO(&read_set);
    FD_ZERO(&active_set);
    FD_SET(s_ctrl, &active_set);
    FD_SET(s_data, &active_set);
    max_fd = (MAX(s_ctrl, s_data)) + 1;

    while(sco_cb.thread_sco_running) {
        read_set = active_set;
        result = select(max_fd, &read_set, NULL, NULL, NULL);
        if (result == 0) {
            ALOGE("select timeout");
            continue;
        }
        if (result < 0) {
            if (errno != EINTR)
                ALOGE("select failed %s", strerror(errno));
            continue;
        }
        if(FD_ISSET(s_ctrl, &read_set)) {
            RTK_NO_INTR(sco_cb.ctrl_fd = accept(s_ctrl, (struct sockaddr *)&remote, &len));
            if (sco_cb.ctrl_fd == -1) {
                ALOGE("sock accept failed (%s)", strerror(errno));
                return NULL;
            }
            const int size = (512);
            setsockopt(sco_cb.ctrl_fd, SOL_SOCKET, SO_RCVBUF, (char*)&size, (int)sizeof(size));
            FD_SET(sco_cb.ctrl_fd, &active_set);
            max_fd = (MAX(max_fd, sco_cb.ctrl_fd)) + 1;
        }

        if(FD_ISSET(s_data, &read_set)) {
            RTK_NO_INTR(sco_cb.data_fd = accept(s_data, (struct sockaddr *)&remote, &len));
            if (sco_cb.data_fd == -1) {
                ALOGE("sock accept failed (%s)", strerror(errno));
                return NULL;
            }
            const int size = (30 * 960);
            int ret = setsockopt(sco_cb.data_fd, SOL_SOCKET, SO_RCVBUF, (char*)&size, (int)sizeof(size));
            ret = setsockopt(sco_cb.data_fd, SOL_SOCKET, SO_SNDBUF, (char*)&size, (int)sizeof(size));
        }

        if(FD_ISSET(sco_cb.ctrl_fd, &read_set)) {
            userial_sco_ctrl_skt_handle();
        }
    }
    close(s_ctrl);
    close(s_data);
    return NULL;
}

#endif

#ifdef RTK_HANDLE_CMD
static void userial_send_personal_cmd(unsigned char * recv_buffer, int total_length)
{
    if(rtkbt_transtype & RTKBT_TRANS_H4) {
        h4_int_transmit_data(recv_buffer, total_length);
    }
    else {
        h5_int_interface->h5_send_cmd(DATA_TYPE_COMMAND, &recv_buffer[1], (total_length - 1));
    }
    userial_enqueue_coex_rawdata(recv_buffer, total_length, false);
}

static void userial_handle_cmd(unsigned char * recv_buffer, int total_length)
{
    RTK_UNUSED(total_length);
    uint16_t opcode = *(uint16_t*)recv_buffer;
    uint16_t scan_int, scan_win;
    static uint16_t voice_settings;
    char prop_value[100];
    switch (opcode) {
        case HCI_BLE_WRITE_SCAN_PARAMS :
            scan_int = *(uint16_t*)&recv_buffer[4];
            scan_win = *(uint16_t*)&recv_buffer[6];
            if(scan_win > 20){
                if((scan_int/scan_win) > 2) {
                  *(uint16_t*)&recv_buffer[4] = (scan_int * 20) / scan_win;
                  *(uint16_t*)&recv_buffer[6] = 20;
                }
                else {
                  *(uint16_t*)&recv_buffer[4] = 40;
                  *(uint16_t*)&recv_buffer[6] = 20;
                }
            }
            else if(scan_win == scan_int) {
              *(uint16_t*)&recv_buffer[4] = (scan_int * 5) & 0xFE;
            }
            else if((scan_int/scan_win) <= 2) {
              *(uint16_t*)&recv_buffer[4] = (scan_int * 3) & 0xFE;
            }
        break;

        case HCI_LE_SET_EXTENDED_SCAN_PARAMETERS:
            scan_int = *(uint16_t*)&recv_buffer[7];
            scan_win = *(uint16_t*)&recv_buffer[9];
            if(scan_win > 20){
                if((scan_int/scan_win) > 2) {
                    *(uint16_t*)&recv_buffer[7] = (scan_int * 20) / scan_win;
                    *(uint16_t*)&recv_buffer[9] = 20;
                }
                else {
                    *(uint16_t*)&recv_buffer[7] = 40;
                    *(uint16_t*)&recv_buffer[9] = 20;
                }
            }
            else if(scan_win == scan_int) {
              *(uint16_t*)&recv_buffer[7] = (scan_int * 5) & 0xFE;
            }
            else if((scan_int/scan_win) <= 2) {
              *(uint16_t*)&recv_buffer[9] = (scan_int * 3) & 0xFE;
            }

        break;

        case HCI_WRITE_VOICE_SETTINGS :
            voice_settings = *(uint16_t*)&recv_buffer[3];
            userial_vendor_usb_ioctl(SET_ISO_CFG, &voice_settings);
#ifdef CONFIG_SCO_OVER_HCI
            sco_cb.voice_settings = voice_settings;
#endif
        break;
#ifdef CONFIG_SCO_OVER_HCI
        case HCI_SETUP_ESCO_CONNECTION :
            sco_cb.voice_settings = *(uint16_t*)&recv_buffer[15];
            pthread_attr_t thread_attr;
            pthread_attr_init(&thread_attr);
            pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_JOINABLE);
            sco_cb.thread_sco_running = true;
            if(pthread_create(&sco_cb.thread_socket_sco_id, &thread_attr, userial_socket_sco_thread, NULL)!= 0 )
            {
                ALOGE("pthread_create : %s", strerror(errno));
            }
        break;
#endif
        case HCI_SET_EVENT_MASK:
          ALOGE("set event mask, it should bt stack init, set coex bt on");
          if(rtk_parse_manager) {
              rtk_parse_manager->rtk_set_bt_on(1);
          }
          Heartbeat_init();
        break;

        case HCI_ACCEPT_CONNECTION_REQUEST:
          property_get("persist.bluetooth.prefferedrole", prop_value, "none");
          if(strcmp(prop_value, "none") != 0) {
              int role = recv_buffer[9];
              if(role == 0x01 && (strcmp(prop_value, "master") == 0))
                recv_buffer[9] = 0x00;
              else if(role == 0x00 && (strcmp(prop_value, "slave") == 0))
                recv_buffer[9] = 0x01;
          }
        break;

        case HCI_BLE_WRITE_ADV_PARAMS:
        {
            if(rtkbt_version.hci_version> HCI_PROTO_VERSION_4_2) {
                break;
            }
            rtkbt_adv_con.adverting_type = recv_buffer[7];
        }
        break;
        case HCI_BLE_WRITE_ADV_ENABLE:
        {
            if(rtkbt_version.hci_version > HCI_PROTO_VERSION_4_2) {
                break;
            }
            if(recv_buffer[2] == 0x01) {
                rtkbt_adv_con.adverting_start = TRUE;
            }
            else if(recv_buffer[2] == 0x00) {
                rtkbt_adv_con.adverting_type = 0;
                rtkbt_adv_con.adverting_enable = FALSE;
                rtkbt_adv_con.adverting_start = FALSE;
            }
        }
        break;
        case HCI_BLE_CREATE_LL_CONN:
          if(rtkbt_version.hci_version > HCI_PROTO_VERSION_4_2) {
              break;
          }
          if(rtkbt_adv_con.adverting_enable &&
            ((rtkbt_adv_con.adverting_type == 0x00) ||
            (rtkbt_adv_con.adverting_type == 0x01) ||
            (rtkbt_adv_con.adverting_type == 0x04))) {
              uint8_t disable_adv_cmd[5] = {0x01, 0x0A, 0x20, 0x01, 0x00};
              rtkbt_adv_con.adverting_enable = FALSE;
              userial_send_personal_cmd(disable_adv_cmd, 5);
          }
        break;
        default:
        break;
    }
}
#endif


//This recv data from bt process. The data type only have ACL/SCO/COMMAND
// direction  BT HOST ----> CONTROLLER
static void userial_recv_H4_rawdata(void *context)
{
    RTK_UNUSED(context);
    serial_data_type_t type = 0;
    ssize_t bytes_read;
    uint16_t opcode;
    uint16_t transmitted_length = 0;
    //unsigned char *buffer = NULL;

    switch (packet_recv_state) {
        case RTKBT_PACKET_IDLE:
            packet_bytes_need = 1;
            do {
                RTK_NO_INTR(bytes_read = read(vnd_userial.uart_fd[1], &type, 1));
                if(bytes_read == -1) {
                    ALOGE("%s, state = %d, read error %s", __func__, packet_recv_state, strerror(errno));
                    return;
                }
                if(!bytes_read && packet_bytes_need) {
                    ALOGE("%s, state = %d, bytes_read 0", __func__, packet_recv_state);
                    return;
                }

                if (type < DATA_TYPE_COMMAND || type > DATA_TYPE_SCO) {
                    ALOGE("%s invalid data type: %d", __func__, type);
                    assert((type >= DATA_TYPE_COMMAND) && (type <= DATA_TYPE_SCO));
                }
                else {
                    packet_bytes_need -= bytes_read;
                    packet_recv_state = RTKBT_PACKET_TYPE;
                    current_type = type;
                    h4_read_buffer[0] = type;
                }
            }while(packet_bytes_need);
            //fall through

        case RTKBT_PACKET_TYPE:
            packet_bytes_need = hci_preamble_sizes[HCI_PACKET_TYPE_TO_INDEX(current_type)];
            h4_read_length = 0;
            packet_recv_state = RTKBT_PACKET_HEADER;
            //fall through

        case RTKBT_PACKET_HEADER:
            do {
                RTK_NO_INTR(bytes_read = read(vnd_userial.uart_fd[1], &h4_read_buffer[h4_read_length + 1], packet_bytes_need));
                if(bytes_read == -1) {
                    ALOGE("%s, state = %d, read error %s", __func__, packet_recv_state, strerror(errno));
                    return;
                }
                if(!bytes_read && packet_bytes_need) {
                    ALOGE("%s, state = %d, bytes_read 0, type : %d", __func__, packet_recv_state, current_type);
                    return;
                }
                packet_bytes_need -= bytes_read;
                h4_read_length += bytes_read;
            }while(packet_bytes_need);
            packet_recv_state = RTKBT_PACKET_CONTENT;

            if(current_type == DATA_TYPE_ACL) {
                packet_bytes_need = *(uint16_t *)&h4_read_buffer[COMMON_DATA_LENGTH_INDEX];
            } else if(current_type == DATA_TYPE_EVENT) {
                packet_bytes_need = h4_read_buffer[EVENT_DATA_LENGTH_INDEX];
            } else {
                packet_bytes_need = h4_read_buffer[COMMON_DATA_LENGTH_INDEX];
            }
            //fall through

        case RTKBT_PACKET_CONTENT:
            while(packet_bytes_need) {
                RTK_NO_INTR(bytes_read = read(vnd_userial.uart_fd[1], &h4_read_buffer[h4_read_length + 1], packet_bytes_need));
                if(bytes_read == -1) {
                    ALOGE("%s, state = %d, read error %s", __func__, packet_recv_state, strerror(errno));
                    return;
                }
                if(!bytes_read) {
                    ALOGE("%s, state = %d, bytes_read 0", __func__, packet_recv_state);
                    return;
                }

                packet_bytes_need -= bytes_read;
                h4_read_length += bytes_read;
            }
            packet_recv_state = RTKBT_PACKET_END;
            //fall through

        case RTKBT_PACKET_END:
            switch (current_type) {
                case DATA_TYPE_COMMAND:
#ifdef RTK_HANDLE_CMD
                    userial_handle_cmd(&h4_read_buffer[1], h4_read_length);
#endif
                    if(rtkbt_transtype & RTKBT_TRANS_H4) {
                        h4_int_transmit_data(h4_read_buffer, (h4_read_length + 1));
                    }
                    else {
                        opcode = *(uint16_t *)&h4_read_buffer[1];
                        if(opcode == HCI_VSC_H5_INIT) {
                            h5_int_interface->h5_send_sync_cmd(opcode, NULL, h4_read_length);
                        }
                        else {
                            transmitted_length = h5_int_interface->h5_send_cmd(type, &h4_read_buffer[1], h4_read_length);
                        }
                    }
                break;

                case DATA_TYPE_ACL:
                    if(rtkbt_transtype & RTKBT_TRANS_H4) {
                        h4_int_transmit_data(h4_read_buffer, (h4_read_length + 1));
                    }
                    else {
                        transmitted_length = h5_int_interface->h5_send_acl_data(type, &h4_read_buffer[1], h4_read_length);
                    }
                break;

                case DATA_TYPE_SCO:
                    if(rtkbt_transtype & RTKBT_TRANS_H4) {
                        h4_int_transmit_data(h4_read_buffer, (h4_read_length + 1));
                    }
                    else {
                        transmitted_length = h5_int_interface->h5_send_sco_data(type, &h4_read_buffer[1], h4_read_length);
                    }
                break;
                default:
                    ALOGE("%s invalid data type: %d", __func__, current_type);
                break;
            }

            userial_enqueue_coex_rawdata(h4_read_buffer,(h4_read_length + 1), false);
        break;

        default:

        break;
    }

    packet_recv_state = RTKBT_PACKET_IDLE;
    packet_bytes_need = 0;
    current_type = 0;
    h4_read_length = 0;
}

static uint16_t h5_int_transmit_data_cb(serial_data_type_t type, uint8_t *data, uint16_t length) {
    assert(data != NULL);
    assert(length > 0);

    if (type != DATA_TYPE_H5) {
        ALOGE("%s invalid data type: %d", __func__, type);
        return 0;
    }

    uint16_t transmitted_length = 0;
    while (length > 0 && vnd_userial.btdriver_state) {
        ssize_t ret = write(vnd_userial.fd, data + transmitted_length, length);
        switch (ret) {
            case -1:
            ALOGE("In %s, error writing to the uart serial port: %s", __func__, strerror(errno));
            goto done;
        case 0:
            // If we wrote nothing, don't loop more because we
            // can't go to infinity or beyond, ohterwise H5 can resend data
            ALOGE("%s, ret %zd", __func__, ret);
            goto done;
        default:
            transmitted_length += ret;
            length -= ret;
            break;
        }
    }

done:;
    return transmitted_length;

}

#ifdef RTK_HANDLE_EVENT
static void userial_handle_event(unsigned char * recv_buffer, int total_length)
{
    RTK_UNUSED(total_length);
    uint8_t event;
    uint8_t *p_data = recv_buffer;
    event = p_data[0];
    switch (event) {
    case HCI_COMMAND_COMPLETE_EVT:
    {
        uint16_t opcode = *((uint16_t*)&p_data[3]);
        uint8_t* stream = &p_data[6];
        if(opcode == HCI_READ_LOCAL_VERSION_INFO) {
            STREAM_TO_UINT8(rtkbt_version.hci_version, stream);
            STREAM_TO_UINT16(rtkbt_version.hci_revision, stream);
            STREAM_TO_UINT8(rtkbt_version.lmp_version, stream);
            STREAM_TO_UINT16(rtkbt_version.manufacturer, stream);
            STREAM_TO_UINT16(rtkbt_version.lmp_subversion, stream);
        }
        else if(opcode == HCI_BLE_WRITE_ADV_ENABLE){
            if(rtkbt_version.hci_version > HCI_PROTO_VERSION_4_2) {
                break;
            }
            if(rtkbt_adv_con.adverting_start &&(p_data[5] == HCI_SUCCESS)) {
                rtkbt_adv_con.adverting_enable = TRUE;
                rtkbt_adv_con.adverting_start = FALSE;
            }
        }
    }
    break;
#ifdef CONFIG_SCO_OVER_HCI
    case HCI_ESCO_CONNECTION_COMP_EVT: {
        if(p_data[2] != 0) {
            sco_cb.thread_sco_running = false;
            pthread_join(sco_cb.thread_recv_sco_id, NULL);
            pthread_join(sco_cb.thread_send_sco_id, NULL);
        }
        else {
          sco_cb.sco_handle = *((uint16_t *)&p_data[3]);
          if(!(sco_cb.voice_settings & 0x0003)) {
              sco_cb.sco_packet_len = 240;
              sco_cb.msbc_used = false;
          }
          else {
              sco_cb.sco_packet_len = 60;
              sco_cb.msbc_used = true;
          }
          sco_cb.current_pos = 0;
        }

        ALOGE("userial_handle_event sco_handle: %d",sco_cb.sco_handle);
    }
    break;

    case HCI_DISCONNECTION_COMP_EVT: {
        if((*((uint16_t *)&p_data[3])) == sco_cb.sco_handle) {
            sco_cb.sco_handle = 0;
            sco_cb.msbc_used = false;
        }
    }
    break;
#endif
    default :
    break;
  }
}

#ifdef CONFIG_SCO_OVER_HCI
static void userial_enqueue_sco_data(unsigned char * recv_buffer, int total_length)
{
    RTK_UNUSED(total_length);
    uint16_t sco_handle;
    uint8_t sco_length;
    uint8_t *p_data = recv_buffer;
    RTK_BUFFER* skb_sco_data;
    int i;
    sco_handle = *((uint16_t *)p_data);
    uint16_t current_pos = sco_cb.current_pos;
    uint16_t sco_packet_len = sco_cb.sco_packet_len;

    if(sco_handle == sco_cb.sco_handle) {
        sco_length = p_data[SCO_PREAMBLE_SIZE - 1];
        p_data += SCO_PREAMBLE_SIZE;
        if(current_pos) {
            if((sco_packet_len - current_pos) <= sco_length) {
                memcpy(&sco_cb.enc_data[current_pos], p_data, (sco_packet_len - current_pos));
                skb_sco_data = RtbAllocate(sco_packet_len, 0);
                memcpy(skb_sco_data->Data, sco_cb.enc_data, sco_packet_len);
                pthread_mutex_lock(&sco_cb.sco_mutex);
                RtbQueueTail(sco_cb.recv_sco_data, skb_sco_data);
                pthread_cond_signal(&sco_cb.sco_cond);
                pthread_mutex_unlock(&sco_cb.sco_mutex);

                sco_cb.current_pos = 0;
                p_data += (sco_packet_len - current_pos);
                sco_length -= (sco_packet_len - current_pos);
            }
            else {
                memcpy(&sco_cb.enc_data[current_pos], p_data, sco_length);
                sco_cb.current_pos += sco_length;
                return;
            }
        }

        if(!sco_cb.msbc_used) {
            for(i = 0; i < (sco_length/sco_packet_len); i++) {
                skb_sco_data = RtbAllocate(sco_packet_len, 0);
                memcpy(skb_sco_data->Data, p_data + i*sco_packet_len, sco_packet_len);
                RtbQueueTail(sco_cb.recv_sco_data, skb_sco_data);
            }
            if((sco_length/sco_packet_len)) {
                pthread_mutex_lock(&sco_cb.sco_mutex);
                pthread_cond_signal(&sco_cb.sco_cond);
                pthread_mutex_unlock(&sco_cb.sco_mutex);
            }

            i = (sco_length % sco_packet_len);
            current_pos = sco_length - i;
            if(i) {
                memcpy(sco_cb.enc_data, p_data + current_pos, i);
                sco_cb.current_pos = i;
            }
            return;
        }
        for(i = 0; i < sco_length; i++) {
            if((p_data[i] == 0x01) && ((p_data[i+1] & 0x0f) == 0x08) && (p_data[i+2] == 0xAD)) {
              if((sco_length - i) < sco_packet_len) {
                  memcpy(sco_cb.enc_data, &p_data[i], (sco_length - i));
                  sco_cb.current_pos = sco_length - i;
                  return;
              }
              else {
                  memcpy(sco_cb.enc_data, &p_data[i], sco_packet_len);   //complete msbc data
                  skb_sco_data = RtbAllocate(sco_packet_len, 0);
                  memcpy(skb_sco_data->Data, sco_cb.enc_data, sco_packet_len);
                  pthread_mutex_lock(&sco_cb.sco_mutex);
                  RtbQueueTail(sco_cb.recv_sco_data, skb_sco_data);
                  pthread_cond_signal(&sco_cb.sco_cond);
                  pthread_mutex_unlock(&sco_cb.sco_mutex);

                  sco_cb.current_pos = 0;
                  i += (sco_packet_len - 1);
              }
            }
        }
    }
}
#endif

static int userial_handle_recv_data(unsigned char * recv_buffer, int total_length)
{
    serial_data_type_t type = 0;
    unsigned char * p_data = recv_buffer;
    int length = total_length;
    uint8_t event;

    switch (received_packet_state) {
        case RTKBT_PACKET_IDLE:
            received_packet_bytes_need = 1;
            while(length) {
                type = p_data[0];
                length--;
                p_data++;
                if (type < DATA_TYPE_ACL || type > DATA_TYPE_EVENT) {
                    ALOGE("%s invalid data type: %d", __func__, type);
                    assert((type > DATA_TYPE_COMMAND) && (type <= DATA_TYPE_EVENT));
                    if(!length)
                        return total_length;

                    continue;
                }
                break;
            }
            recv_packet_current_type = type;
            received_packet_state = RTKBT_PACKET_TYPE;
            //fall through

        case RTKBT_PACKET_TYPE:
            received_packet_bytes_need = hci_preamble_sizes[HCI_PACKET_TYPE_TO_INDEX(recv_packet_current_type)];
            received_resvered_length = 0;
            received_packet_state = RTKBT_PACKET_HEADER;
            //fall through

        case RTKBT_PACKET_HEADER:
            if(length >= received_packet_bytes_need) {
                memcpy(&received_resvered_header[received_resvered_length], p_data, received_packet_bytes_need);
                received_resvered_length += received_packet_bytes_need;
                length -= received_packet_bytes_need;
                p_data += received_packet_bytes_need;
            }
            else {
                memcpy(&received_resvered_header[received_resvered_length], p_data, length);
                received_resvered_length += length;
                received_packet_bytes_need -= length;
                length = 0;
                return total_length;
            }
            received_packet_state = RTKBT_PACKET_CONTENT;

            if(recv_packet_current_type == DATA_TYPE_ACL) {
                received_packet_bytes_need = *(uint16_t *)&received_resvered_header[2];
            }
             else if(recv_packet_current_type == DATA_TYPE_EVENT){
                received_packet_bytes_need = received_resvered_header[1];
            }
            else {
                received_packet_bytes_need = received_resvered_header[2];
            }
            //fall through

        case RTKBT_PACKET_CONTENT:
            if(recv_packet_current_type == DATA_TYPE_EVENT) {
                event = received_resvered_header[0];

                if(event == HCI_COMMAND_COMPLETE_EVT) {
                    if(received_resvered_length == 2) {
                      if(length >= 1) {
                          *p_data = 1;
                      }
                    }
                }
                else if(event == HCI_COMMAND_STATUS_EVT) {
                    if(received_resvered_length < 4) {
                      int act_len = 4 - received_resvered_length;
                      if(length >= act_len) {
                          *(p_data + act_len -1) = 1;
                      }
                    }
                }
            }
            if(length >= received_packet_bytes_need) {
                memcpy(&received_resvered_header[received_resvered_length], p_data, received_packet_bytes_need);
                length -= received_packet_bytes_need;
                p_data += received_packet_bytes_need;
                received_resvered_length += received_packet_bytes_need;
                received_packet_bytes_need = 0;
            }
            else {
                memcpy(&received_resvered_header[received_resvered_length], p_data, length);
                received_resvered_length += length;
                received_packet_bytes_need -= length;
                length = 0;
                return total_length;
            }
            received_packet_state = RTKBT_PACKET_END;
            //fall through

        case RTKBT_PACKET_END:
            switch (recv_packet_current_type) {
                case DATA_TYPE_EVENT :
                    userial_handle_event(received_resvered_header, received_resvered_length);
                break;
#ifdef CONFIG_SCO_OVER_HCI
                case DATA_TYPE_SCO :
                    userial_enqueue_sco_data(received_resvered_header, received_resvered_length);
                break;
#endif
                default :

                break;
            }
        break;

        default:

        break;
    }

    received_packet_state = RTKBT_PACKET_IDLE;
    received_packet_bytes_need = 0;
    recv_packet_current_type = 0;
    received_resvered_length = 0;

    return (total_length - length);
}
#endif

static void h5_data_ready_cb(serial_data_type_t type, unsigned int total_length)
{
    unsigned char buffer[1028] = {0};
    int length = 0;
    length = h5_int_interface->h5_int_read_data(&buffer[1], total_length);
    if(length == -1) {
        ALOGE("%s, error read length", __func__);
        assert(length != -1);
    }
    buffer[0] = type;
    length++;
    uint16_t transmitted_length = 0;
    unsigned int real_length = length;
#ifdef RTK_HANDLE_EVENT
    unsigned int read_length = 0;
    do {
        read_length += userial_handle_recv_data(buffer + read_length, real_length - read_length);
    }while(vnd_userial.thread_running && read_length < total_length);
#endif

    while (length > 0) {
        ssize_t ret;
        RTK_NO_INTR(ret = write(vnd_userial.uart_fd[1], buffer + transmitted_length, length));
        switch (ret) {
        case -1:
            ALOGE("In %s, error writing to the uart serial port: %s", __func__, strerror(errno));
            goto done;
        case 0:
            // If we wrote nothing, don't loop more because we
            // can't go to infinity or beyond
            goto done;
        default:
            transmitted_length += ret;
            length -= ret;
            break;
        }
    }
done:;
    if(real_length)
        userial_enqueue_coex_rawdata(buffer, real_length, true);
    return;
}

//This recv data from driver which is sent or recv by the controller. The data type have ACL/SCO/EVENT
// direction CONTROLLER -----> BT HOST
static void userial_recv_uart_rawdata(unsigned char *buffer, unsigned int total_length)
{
    unsigned int length = total_length;
    uint16_t transmitted_length = 0;
#ifdef RTK_HANDLE_EVENT
    unsigned int read_length = 0;
    do {
        read_length += userial_handle_recv_data(buffer + read_length, total_length - read_length);

    }while(read_length < total_length);
#endif
    while (length > 0) {
        ssize_t ret;
        RTK_NO_INTR(ret = write(vnd_userial.uart_fd[1], buffer + transmitted_length, length));
        switch (ret) {
        case -1:
            ALOGE("In %s, error writing to the uart serial port: %s", __func__, strerror(errno));
            goto done;
        case 0:
            // If we wrote nothing, don't loop more because we
            // can't go to infinity or beyond
            goto done;
        default:
            transmitted_length += ret;
            length -= ret;
            break;
        }
    }
done:;
    if(total_length)
        userial_enqueue_coex_rawdata(buffer, total_length, true);
    return;
}

void userial_recv_rawdata_hook(unsigned char *buffer, unsigned int total_length)
{
      uint16_t transmitted_length = 0;
      unsigned int real_length = total_length;

      while (vnd_userial.thread_running && (total_length > 0)) {
          ssize_t ret;
          RTK_NO_INTR(ret = write(vnd_userial.uart_fd[1], buffer + transmitted_length, total_length));
          switch (ret) {
          case -1:
              ALOGE("In %s, error writing to the uart serial port: %s", __func__, strerror(errno));
              goto done;
          case 0:
              // If we wrote nothing, don't loop more because we
              // can't go to infinity or beyond
              goto done;
          default:
              transmitted_length += ret;
              total_length -= ret;
              break;
          }
      }
  done:;
      if(real_length && vnd_userial.thread_running)
          userial_enqueue_coex_rawdata(buffer, real_length, true);
      return;

}

static void* userial_recv_socket_thread(void *arg)
{
    RTK_UNUSED(arg);
    struct epoll_event events[64];
    int j;
    while(vnd_userial.thread_running) {
        int ret;
        do{
            ret = epoll_wait(vnd_userial.epoll_fd, events, 32, 500);
        }while(vnd_userial.thread_running && ret == -1 && errno == EINTR);

        if (ret == -1) {
            ALOGE("%s error in epoll_wait: %s", __func__, strerror(errno));
        }
        for (j = 0; j < ret; ++j) {
            struct rtk_object_t *object = (struct rtk_object_t *)events[j].data.ptr;
            if (events[j].data.ptr == NULL)
                continue;
            else {
                if (events[j].events & (EPOLLIN | EPOLLHUP | EPOLLRDHUP | EPOLLERR) && object->read_ready)
                    object->read_ready(object->context);
                if (events[j].events & EPOLLOUT && object->write_ready)
                    object->write_ready(object->context);
            }
        }
    }
    ALOGD("%s exit", __func__);
    return NULL;
}

static void* userial_recv_uart_thread(void *arg)
{
    RTK_UNUSED(arg);
    struct pollfd pfd[2];
    pfd[0].events = POLLIN|POLLHUP|POLLERR|POLLRDHUP;
    pfd[0].fd = vnd_userial.signal_fd[1];
    pfd[1].events = POLLIN|POLLHUP|POLLERR|POLLRDHUP;
    pfd[1].fd = vnd_userial.fd;
    int ret;
    unsigned char read_buffer[2056] = {0};
    ssize_t bytes_read;
    while(vnd_userial.thread_running) {
        do{
            ret = poll(pfd, 2, 500);
        }while(ret == -1 && errno == EINTR && vnd_userial.thread_running);

        //exit signal is always at first index
        if(pfd[0].revents && !vnd_userial.thread_running) {
            ALOGE("receive exit signal and stop thread ");
            return NULL;
        }

        if (pfd[1].revents & POLLIN) {
            RTK_NO_INTR(bytes_read = read(vnd_userial.fd, read_buffer, sizeof(read_buffer)));
            if(!bytes_read)
                continue;

            if(rtkbt_transtype & RTKBT_TRANS_H5) {
                h5_int_interface->h5_recv_msg(read_buffer, bytes_read);
            }
            else {
                userial_recv_uart_rawdata(read_buffer, bytes_read);
            }
        }

        if (pfd[1].revents & (POLLERR|POLLHUP)) {
            ALOGE("%s poll error, fd : %d", __func__, vnd_userial.fd);
            vnd_userial.btdriver_state = false;
            close(vnd_userial.fd);
            return NULL;
        }
        if (ret < 0)
        {
            ALOGE("%s : error (%d)", __func__, ret);
            continue;
        }
    }
    ALOGD("%s exit", __func__);
    return NULL;
}

static void* userial_coex_thread(void *arg)
{
    RTK_UNUSED(arg);
    struct epoll_event events[64];
    int j;
    while(vnd_userial.thread_running) {
        int ret;
        do{
            ret = epoll_wait(vnd_userial.cpoll_fd, events, 64, 500);
        }while(ret == -1 && errno == EINTR && vnd_userial.thread_running);
        if (ret == -1) {
            ALOGE("%s error in epoll_wait: %s", __func__, strerror(errno));
        }
        for (j = 0; j < ret; ++j) {
            struct rtk_object_t *object = (struct rtk_object_t *)events[j].data.ptr;
            if (events[j].data.ptr == NULL)
                continue;
            else {
                if (events[j].events & (EPOLLIN | EPOLLHUP | EPOLLRDHUP | EPOLLERR) && object->read_ready)
                    object->read_ready(object->context);
                if (events[j].events & EPOLLOUT && object->write_ready)
                    object->write_ready(object->context);
            }
        }
    }
    ALOGD("%s exit", __func__);
    return NULL;
}

int userial_socket_open()
{
    int ret = 0;
    struct epoll_event event;
    if((ret = socketpair(AF_UNIX, SOCK_STREAM, 0, vnd_userial.uart_fd)) < 0) {
        ALOGE("%s, errno : %s", __func__, strerror(errno));
        return ret;
    }

    if((ret = socketpair(AF_UNIX, SOCK_STREAM, 0, vnd_userial.signal_fd)) < 0) {
        ALOGE("%s, errno : %s", __func__, strerror(errno));
        return ret;
    }

    vnd_userial.epoll_fd = epoll_create(64);
    if (vnd_userial.epoll_fd == -1) {
        ALOGE("%s unable to create epoll instance: %s", __func__, strerror(errno));
        return -1;
    }

    rtk_socket_object.fd = vnd_userial.uart_fd[1];
    rtk_socket_object.read_ready = userial_recv_H4_rawdata;
    memset(&event, 0, sizeof(event));
    event.events |= EPOLLIN | EPOLLHUP | EPOLLRDHUP | EPOLLERR;
    event.data.ptr = (void *)&rtk_socket_object;
    if (epoll_ctl(vnd_userial.epoll_fd, EPOLL_CTL_ADD, vnd_userial.uart_fd[1], &event) == -1) {
        ALOGE("%s unable to register fd %d to epoll set: %s", __func__, vnd_userial.uart_fd[1], strerror(errno));
        close(vnd_userial.epoll_fd);
        return -1;
    }

    event.data.ptr = NULL;
    if (epoll_ctl(vnd_userial.epoll_fd, EPOLL_CTL_ADD, vnd_userial.signal_fd[1], &event) == -1) {
        ALOGE("%s unable to register signal fd %d to epoll set: %s", __func__, vnd_userial.signal_fd[1], strerror(errno));
        close(vnd_userial.epoll_fd);
        return -1;
    }
    pthread_attr_t thread_attr;
    pthread_attr_init(&thread_attr);
    pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_JOINABLE);
    vnd_userial.thread_running = true;
    if (pthread_create(&vnd_userial.thread_socket_id, &thread_attr, userial_recv_socket_thread, NULL)!=0 )
    {
        ALOGE("pthread_create : %s", strerror(errno));
        close(vnd_userial.epoll_fd);
        return -1;
    }


    if (pthread_create(&vnd_userial.thread_uart_id, &thread_attr, userial_recv_uart_thread, NULL)!=0 )
    {
        ALOGE("pthread_create : %s", strerror(errno));
        close(vnd_userial.epoll_fd);
        vnd_userial.thread_running = false;
        pthread_join(vnd_userial.thread_socket_id, NULL);
        return -1;
    }

    vnd_userial.cpoll_fd = epoll_create(64);
    assert (vnd_userial.cpoll_fd != -1);

    vnd_userial.event_fd = eventfd(10, EFD_NONBLOCK);
    assert(vnd_userial.event_fd != -1);
    if(vnd_userial.event_fd != -1) {
        rtk_coex_object.fd = vnd_userial.event_fd;
        rtk_coex_object.read_ready = userial_coex_handler;
        memset(&event, 0, sizeof(event));
        event.events |= EPOLLIN | EPOLLHUP | EPOLLRDHUP | EPOLLERR;
        event.data.ptr = (void *)&rtk_coex_object;
        if (epoll_ctl(vnd_userial.cpoll_fd, EPOLL_CTL_ADD, vnd_userial.event_fd, &event) == -1) {
            ALOGE("%s unable to register fd %d to cpoll set: %s", __func__, vnd_userial.event_fd, strerror(errno));
            assert(false);
        }

        event.data.ptr = NULL;
        if (epoll_ctl(vnd_userial.cpoll_fd, EPOLL_CTL_ADD, vnd_userial.signal_fd[1], &event) == -1) {
            ALOGE("%s unable to register fd %d to cpoll set: %s", __func__, vnd_userial.signal_fd[1], strerror(errno));
            assert(false);
        }

        if (pthread_create(&vnd_userial.thread_coex_id, &thread_attr, userial_coex_thread, NULL) !=0 )
        {
            ALOGE("pthread create  coex : %s", strerror(errno));
            assert(false);
        }
    }

    ret = vnd_userial.uart_fd[0];
    return ret;
}

int userial_vendor_usb_ioctl(int operation, void* param)
{
    int retval;
    retval = ioctl(vnd_userial.fd, operation, param);
    return retval;
}

int userial_vendor_usb_open(void)
{
    if ((vnd_userial.fd = open(vnd_userial.port_name, O_RDWR)) == -1)
    {
        ALOGE("%s: unable to open %s: %s", __func__, vnd_userial.port_name, strerror(errno));
        return -1;
    }

    vnd_userial.btdriver_state = true;
    ALOGI("device fd = %d open", vnd_userial.fd);

    return vnd_userial.fd;
}

