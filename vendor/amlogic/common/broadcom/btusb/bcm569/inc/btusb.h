/*
 *
 * btusb.h
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

#ifndef BTUSB_H
#define BTUSB_H

#include <linux/version.h>
#include <linux/usb.h>
#include <linux/tty.h>
#include <linux/time.h>

#include "bt_types.h"
#include "gki_int.h"
#include "btusbext.h"

#ifdef BTUSB_LITE
#include "btusb_lite.h"
#endif

/* Linux kernel compatibility abstraction */
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 34)
#define BTUSB_USHRT_MAX USHORT_MAX
#else
#define BTUSB_USHRT_MAX USHRT_MAX
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 10, 0)
#define BTUSB_PDE_DATA(inode) PDE(inode)->data
#else
#define BTUSB_PDE_DATA(inode) PDE_DATA(inode)
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 34)
#define BTUSB_BUFFER_ALLOC usb_buffer_alloc
#define BTUSB_BUFFER_FREE usb_buffer_free
#else
#define BTUSB_BUFFER_ALLOC usb_alloc_coherent
#define BTUSB_BUFFER_FREE usb_free_coherent
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 30)
#define BTUSB_EP_TYPE(ep) (ep->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK)
#define BTUSB_EP_DIR_IN(ep) ((ep->bEndpointAddress & USB_ENDPOINT_DIR_MASK) == USB_DIR_IN)
#define BTUSB_EP_DIR_OUT(ep) ((ep->bEndpointAddress & USB_ENDPOINT_DIR_MASK) == USB_DIR_OUT)
#else
#define BTUSB_EP_TYPE(ep) usb_endpoint_type(ep)
#define BTUSB_EP_DIR_IN(ep) usb_endpoint_dir_in(ep)
#define BTUSB_EP_DIR_OUT(ep) usb_endpoint_dir_out(ep)
#endif

// debug information flags. one-hot encoded:
// bit0 : enable printk(KERN_DEBUG)
// bit1 : enable printk(KERN_INFO)
// bit16 : enable all message dumps in printk
// bit18 : enable GKI buffer check
// bit19 : enable RX ACL size check
#define BTUSB_DBG_MSG           0x0001
#define BTUSB_INFO_MSG          0x0002
#define BTUSB_DUMP_MSG          0x0100
#define BTUSB_GKI_CHK_MSG       0x0400
//#define BTUSB_DBGFLAGS (BTUSB_DBG_MSG | BTUSB_INFO_MSG | BTUSB_DUMP_MSG | BTUSB_GKI_CHK_MSG)
#define BTUSB_DBGFLAGS 0

extern int dbgflags;

#define BTUSB_DBG(fmt, ...) if (unlikely(dbgflags & BTUSB_DBG_MSG)) \
    printk(KERN_DEBUG "BTUSB %s: " fmt, __FUNCTION__, ##__VA_ARGS__)

#define BTUSB_INFO(fmt, ...) if (unlikely(dbgflags & BTUSB_INFO_MSG))\
    printk(KERN_INFO "BTUSB %s: " fmt, __FUNCTION__, ##__VA_ARGS__)

#define BTUSB_ERR(fmt, ...) \
    printk(KERN_ERR "BTUSB %s: " fmt, __FUNCTION__, ##__VA_ARGS__)

// TBD: how do we assign the minor range?
#define BTUSB_MINOR_BASE 194

// 1025 = size(con_hdl) + size(acl_len) + size(3-dh5) = 2 + 2 + 1021
#define BTUSB_HCI_MAX_ACL_SIZE 1025
// Maximum size of command and events packets (events = 255 + 2, commands = 255 + 3)
#define BTUSB_HCI_MAX_CMD_SIZE 258
#define BTUSB_HCI_MAX_EVT_SIZE 258

// Maximum HCI H4 size = HCI type + ACL packet
#define BTUSB_H4_MAX_SIZE (1 + BTUSB_HCI_MAX_ACL_SIZE)

#define BTUSB_NUM_OF_ACL_RX_BUFFERS   12
#define BTUSB_NUM_OF_ACL_TX_BUFFERS   12
#define BTUSB_NUM_OF_DIAG_RX_BUFFERS   2 // must not be less than 2
#define BTUSB_NUM_OF_DIAG_TX_BUFFERS   2
#define BTUSB_NUM_OF_EVENT_BUFFERS     8 // should not be less than 2
#define BTUSB_NUM_OF_CMD_BUFFERS       8
#define BTUSB_MAXIMUM_TX_VOICE_SIZE  192
#define BTUSB_NUM_OF_VOICE_RX_BUFFERS  2 // for now should match 2 urbs
#define BTUSB_NUM_OF_VOICE_TX_BUFFERS 32

#define SCO_RX_BUFF_SIZE 360
#define SCO_RX_MAX_LEN   240

#define BTUSB_VOICE_BURST_SIZE 48
#define BTUSB_VOICE_HEADER_SIZE 3
#define BTUSB_VOICE_FRAMES_PER_URB 9
#define BTUSB_VOICE_BUFFER_MAXSIZE (BTUSB_VOICE_FRAMES_PER_URB * \
        ALIGN(BTUSB_VOICE_BURST_SIZE + BTUSB_VOICE_HEADER_SIZE, 4))


#ifndef UINT8_TO_STREAM
#define UINT8_TO_STREAM(p, u8)   {*(p)++ = (UINT8)(u8);}
#define UINT16_TO_STREAM(p, u16) {*(p)++ = (UINT8)(u16); *(p)++ = (UINT8)((u16) >> 8);}
#define BDADDR_TO_STREAM(p, a)   {register int ijk; for (ijk = 0; ijk < BD_ADDR_LEN;  ijk++) *(p)++ = (UINT8) a[BD_ADDR_LEN - 1 - ijk];}
#define ARRAY_TO_STREAM(p, a, len) {register int ijk; for (ijk = 0; ijk < len; ijk++) *(p)++ = (UINT8) a[ijk];}

#define STREAM_TO_UINT8(u8, p)   {u8 = (UINT8)(*(p)); (p) += 1;}
#define STREAM_TO_UINT16(u16, p) {u16 = (UINT16)((*(p)) + ((*((p) + 1)) << 8)); (p) += 2;}
#define STREAM_TO_UINT32(u32, p) {u32 = (((UINT32)(*(p))) + ((((UINT32)(*((p) + 1)))) << 8) + ((((UINT32)(*((p) + 2)))) << 16) + ((((UINT32)(*((p) + 3)))) << 24)); (p) += 4;}
#define STREAM_TO_BDADDR(a, p)   {register int ijk; register UINT8 *pbda = (UINT8 *)a + BD_ADDR_LEN - 1; for (ijk = 0; ijk < BD_ADDR_LEN; ijk++) *pbda-- = *p++;}
#define STREAM_TO_ARRAY(a, p, len) {register int ijk; for (ijk = 0; ijk < len; ijk++) ((UINT8 *) a)[ijk] = *p++;}
#endif

#define UINT32_TO_BE_STREAM(p, u32) {*(p)++ = (UINT8)((u32) >> 24);  *(p)++ = (UINT8)((u32) >> 16); *(p)++ = (UINT8)((u32) >> 8); *(p)++ = (UINT8)(u32); }
#define UINT24_TO_BE_STREAM(p, u24) {*(p)++ = (UINT8)((u24) >> 16); *(p)++ = (UINT8)((u24) >> 8); *(p)++ = (UINT8)(u24);}
#define UINT16_TO_BE_STREAM(p, u16) {*(p)++ = (UINT8)((u16) >> 8); *(p)++ = (UINT8)(u16);}
#define UINT8_TO_BE_STREAM(p, u8)   {*(p)++ = (UINT8)(u8);}

// macro that helps parsing arrays
#define BTUSB_ARRAY_FOR_EACH_TRANS(__a) \
    for (idx = 0, p_trans = &__a[0]; idx < ARRAY_SIZE(__a); idx++, p_trans = &__a[idx])

/* Layer Specific field: Used to send packet to User Space */
#define BTUSB_LS_H4_TYPE_SENT   (1<<0)  /* H4 HCI Type already sent */
#define BTUSB_LS_GKI_BUFFER     (1<<1)  /* Locally allocated buffer-not to resubmit */

//
// Container used to copy the URB to sniff the SCO
//
struct btusb_scosniff
{
    struct list_head lh;                    // to add element to a list
    int s;                                  // start frame
    unsigned int n;                         // number of descriptors
    unsigned int l;                         // buffer length
    struct usb_iso_packet_descriptor d[0];  // descriptors
};

//
// This data structure is used for isochronous transfers
//
typedef struct
{
    // These two must go in sequence
    void *dev;
    unsigned char *packet;
    int length;
    unsigned long index;
    int used;

    // URB & related info must be the last fields
    struct urb urb;
    // enough to cover the longest request
    struct usb_iso_packet_descriptor IsoPacket[(BTUSB_MAXIMUM_TX_VOICE_SIZE /9) + 1];
} tBTUSB_ISO_ELEMENT;

// BTUSB transaction
typedef struct
{
    /* This is mapped to a GKI buffer to allow queuing */
    BUFFER_HDR_T gki_hdr;
    /* Sharing queue with other packets -> needs BT header to multiplex */
    BT_HDR bt_hdr;
    /* Pointer to the location where the USB data is received */
    UINT8 *dma_buffer;
    /* DMA structure */
    dma_addr_t dma;
    /* URB for this transaction */
    struct urb *p_urb;
    void *context;
#if (GKI_ENABLE_BUF_CORRUPTION_CHECK == TRUE)
    /* Magic number */
    UINT32 magic;
#endif
} tBTUSB_TRANSACTION;

// Voice channel descriptor
typedef struct
{
    unsigned short sco_handle;
    unsigned char burst;
} tBTUSB_SCO_INFO;

typedef struct
{
    int used;
    BT_HDR *p_msg;
    tBTUSB_SCO_INFO info;
} tBTUSB_VOICE_CHANNEL;

#define BTUSB_QUIRK_ZLP_TX_REQ (1 << 0)
#define BTUSB_QUIRK_ZLP_RX_WA  (1 << 1)

// Define the main structure
typedef struct btusb_cb
{
    struct usb_device *p_udev;              // the usb device for this device
    const struct usb_device_id *p_id;       // Device Id from probe
    struct usb_interface *p_main_intf;      // Main Interface reference
    struct usb_interface *p_voice_intf;     // VOICE Interface reference
    struct usb_interface *p_dfu_intf;       // DFU Interface reference
    struct usb_interface *p_diag_intf;      // Diag Interface reference

    struct usb_host_endpoint *p_acl_in;     // the acl bulk in endpoint
    struct usb_host_endpoint *p_acl_out;    // the acl bulk out endpoint
    struct usb_host_endpoint *p_diag_in;    // the diag bulk in endpoint
    struct usb_host_endpoint *p_diag_out;   // the diag bulk out endpoint
    struct usb_host_endpoint *p_event_in;   // the interrupt in endpoint
    struct usb_host_endpoint *p_voice_out;  // iso out endpoint
    struct usb_host_endpoint *p_voice_in;   // voice in endpoint
    struct ktermios kterm;                  // TTY emulation
    struct mutex open_mutex;                // protect concurrent open accesses
    spinlock_t tasklet_lock;
    struct kref kref;
    tBTUSB_STATS stats;
    bool opened;
    bool issharedusb;
    unsigned int quirks;

    // reception queue
    wait_queue_head_t rx_wait_q;

    // tx tasklet
    struct tasklet_struct tx_task;

    // proc filesystem entry to retrieve info from driver environment
    struct proc_dir_entry *p_debug_pde;
    bool scosniff_active;
    struct proc_dir_entry *p_scosniff_pde;
    struct list_head scosniff_list;
    struct completion scosniff_completion;

    // Command transmit path
    tBTUSB_TRANSACTION cmd_array[BTUSB_NUM_OF_CMD_BUFFERS];
    struct usb_ctrlrequest cmd_req_array[BTUSB_NUM_OF_CMD_BUFFERS];
    struct usb_anchor cmd_submitted;

    // Event receive path
    tBTUSB_TRANSACTION event_array[BTUSB_NUM_OF_EVENT_BUFFERS];
    struct usb_anchor event_submitted;

    // ACL receive path
    tBTUSB_TRANSACTION acl_rx_array[BTUSB_NUM_OF_ACL_RX_BUFFERS];
    struct usb_anchor acl_rx_submitted;

    // ACL transmit path
    tBTUSB_TRANSACTION acl_tx_array[BTUSB_NUM_OF_ACL_TX_BUFFERS];
    struct usb_anchor acl_tx_submitted;

    // Diagnostics receive path
    tBTUSB_TRANSACTION diag_rx_array[BTUSB_NUM_OF_DIAG_RX_BUFFERS];
    struct usb_anchor diag_rx_submitted;

    // Diagnostics transmit path
    tBTUSB_TRANSACTION diag_tx_array[BTUSB_NUM_OF_DIAG_TX_BUFFERS];
    struct usb_anchor diag_tx_submitted;

    // Voice
    tBTUSB_TRANSACTION voice_rx_array[BTUSB_NUM_OF_VOICE_RX_BUFFERS];
    struct usb_anchor voice_rx_submitted;

    tBTUSB_VOICE_CHANNEL voice_channels[3];
    unsigned short desired_packet_size;
    unsigned int pending_bytes;
    BT_HDR **pp_pending_msg;
    unsigned char pending_hdr[BTUSB_VOICE_HEADER_SIZE];
    unsigned int pending_hdr_size;

    BT_HDR *p_write_msg;

    unsigned long room_for_device_object; // must be here before the pending buffer
    tBTUSB_ISO_ELEMENT *p_voicetxIrpList;
    unsigned long voicetxIrpIndex;

    BUFFER_Q rx_queue;
    BT_HDR *p_rx_msg;
    BUFFER_Q tx_queue;

#ifdef BTUSB_LITE
    struct btusb_lite_cb lite_cb;
#endif
} tBTUSB_CB;


//
// Function prototypes
//
void btusb_delete(struct kref *kref);

void btusb_tx_task(unsigned long arg);

void btusb_cancel_voice(tBTUSB_CB *p_dev);
void btusb_cancel_urbs(tBTUSB_CB *p_dev);

void btusb_voice_stats(unsigned long *p_max, unsigned long *p_min,
        struct timeval *p_result, struct timeval *p_last_time);

/* URB submit */
int btusb_submit(tBTUSB_CB *p_dev, struct usb_anchor *p_anchor, tBTUSB_TRANSACTION *p_trans, int mem_flags);
void btusb_submit_voice_rx(tBTUSB_CB *p_dev, tBTUSB_TRANSACTION *p_trans, int mem_flags);

/* bt controller to host routines */
void btusb_enqueue(tBTUSB_CB *p_dev, tBTUSB_TRANSACTION *p_trans, UINT8 hcitype);
void btusb_dequeued(tBTUSB_CB *p_dev, BT_HDR *p_msg);

void btusb_dump_data(const UINT8 *p, int len, const char *p_title);

void btusb_cmd_complete(struct urb *p_urb);
void btusb_write_complete(struct urb *p_urb);
void btusb_voicerx_complete(struct urb *p_urb);

// USB device interface
int btusb_open(struct inode *inode, struct file *file);
int btusb_release(struct inode *inode, struct file *file);
ssize_t btusb_read(struct file *file, char __user *buffer, size_t count, loff_t *ppos);
ssize_t btusb_write(struct file *file, const char __user *user_buffer, size_t count, loff_t *ppos);
unsigned int btusb_poll(struct file *file, struct poll_table_struct *p_pt);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
long btusb_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
#else
int btusb_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg);
#endif

// proc standard interface
int btusb_debug_open(struct inode *inode, struct file *file);
ssize_t btusb_debug_write(struct file *file, const char *buf, size_t count, loff_t *pos);

ssize_t btusb_scosniff_read(struct file *file, char __user *buf, size_t size, loff_t *ppos);
int btusb_scosniff_open(struct inode *inode, struct file *file);
int btusb_scosniff_release(struct inode *inode, struct file *file);

//
// Globals
//
extern struct usb_driver btusb_driver;
extern bool autopm;

#endif // BTUSB_H
