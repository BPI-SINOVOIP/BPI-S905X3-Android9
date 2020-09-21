/*
 *
 * btusb_dev.c
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

#include <linux/module.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/serial.h>

#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include <linux/jiffies.h>

#include "btusb.h"

#include "hcidefs.h"

#include "gki_int.h"

// delay between the voice samples: 3 ms
#define BTUSB_VOICE_DELAY 3

// local functions declaration
static int btusb_add_voice_channel(tBTUSB_CB *p_dev, unsigned short sco_handle, unsigned char burst);
static int btusb_remove_voice_channel(tBTUSB_CB *p_dev, unsigned short sco_handle);
static int btusb_update_voice_channels(tBTUSB_CB *p_dev);
static void btusb_update_voice_rx(tBTUSB_CB *p_dev);
static void btusb_submit_voice_tx(tBTUSB_CB *p_dev, UINT8 *p_data, int len);
static int btusb_submit_cmd(tBTUSB_CB *p_dev, char *packet, unsigned long length);
static int btusb_submit_acl(tBTUSB_CB *p_dev, char *packet, unsigned long length);
static int btusb_submit_diag(tBTUSB_CB *p_dev, char *packet, unsigned long length);
static int btusb_submit_write_voice(tBTUSB_CB *p_dev, void *p_packet, unsigned long length);

/*******************************************************************************
 **
 ** Function         btusb_alloc_trans
 **
 ** Description      Allocate a transaction in the array of transactions.
 **
 ** Parameters       p_array: array of USB transactions
 **                  size: size of the array of transactions
 **
 ** Returns          Pointer to the next free transaction, NULL if not found
 **
 *******************************************************************************/
static tBTUSB_TRANSACTION *btusb_alloc_trans(tBTUSB_TRANSACTION *p_array, size_t size)
{
    int idx;
    tBTUSB_TRANSACTION *p_trans;

    for (idx = 0; idx < size; idx++)
    {
        p_trans = &p_array[idx];
        if (p_trans->gki_hdr.status == BUF_STATUS_FREE)
        {
            p_trans->gki_hdr.status = BUF_STATUS_UNLINKED;
            return p_trans;
        }
    }
    return NULL;
}

/*******************************************************************************
 **
 ** Function         btusb_dump_data
 **
 ** Description      Print the data into the kernel messages
 **
 ** Parameters       p: pointer to the data to print
 **                  len: length of the data to print
 **                  p_title: title to print before the data
 **
 ** Returns          void
 **
 *******************************************************************************/
void btusb_dump_data(const UINT8 *p, int len, const char *p_title)
{
    int idx;

    if (likely((dbgflags & BTUSB_DBG_MSG) == 0))
    {
        return;
    }

    if (p_title)
    {
        printk("---------------------- %s ----------------------\n", p_title);
    }

    printk("%p: ", p);

    for(idx = 0; idx < len; idx++)
    {
        printk("%02x ", p[idx]);
    }
    printk("\n--------------------------------------------------------------------\n");
}

/*******************************************************************************
 **
 ** Function         btusb_voice_stats
 **
 ** Description
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
void btusb_voice_stats(unsigned long *p_max, unsigned long *p_min,
        struct timeval *p_result, struct timeval *p_last_time)
{
    struct timeval current_time;

    do_gettimeofday(&current_time);
    // perform the carry for the later subtraction by updating y
    if (current_time.tv_usec < p_last_time->tv_usec) {
     int nsec = (p_last_time->tv_usec - current_time.tv_usec) / 1000000 + 1;
     p_last_time->tv_usec -= 1000000 * nsec;
     p_last_time->tv_sec += nsec;
    }
    if (current_time.tv_usec - p_last_time->tv_usec > 1000000) {
     int nsec = (current_time.tv_usec - p_last_time->tv_usec) / 1000000;
     p_last_time->tv_usec += 1000000 * nsec;
     p_last_time->tv_sec -= nsec;
    }
    // compute the time remaining to wait
    p_result->tv_sec = current_time.tv_sec - p_last_time->tv_sec;
    p_result->tv_usec = current_time.tv_usec - p_last_time->tv_usec;

    // update the max except the first time where the calculation is wrong
    // because of the initial value assuming p_last is zero initialized
    if (p_max != NULL)
    {
        if (p_result->tv_usec > *p_max && (p_last_time->tv_sec) && (p_last_time->tv_usec))
        {
           *p_max = p_result->tv_usec;
        }
    }

    // update the min except the first time where the calculation is wrong
    // because of the initial value assuming *p_last and p_min are zero initialized
    if (p_min != NULL)
    {
        if ((p_result->tv_usec < *p_min || (*p_min == 0))
                && (p_last_time->tv_sec) && (p_last_time->tv_usec))
        {
           *p_min = p_result->tv_usec;
        }
    }

    memcpy(p_last_time, &current_time, sizeof(struct timeval));
    BTUSB_DBG("btusb_voice_stats len: %lu\n", p_result->tv_usec);
}

/*******************************************************************************
 **
 ** Function         btusb_tx_task
 **
 ** Description      host to controller tasklet
 **
 ** Parameters       arg:
 **
 ** Returns          void
 **
 *******************************************************************************/
void btusb_tx_task(unsigned long arg)
{
    tBTUSB_CB *p_dev = (tBTUSB_CB *) arg;
    BT_HDR *p_buf;
    UINT8 *p_data;
    UINT8 msg_type;

    // dequeue data packet and submit to transport
    if (unlikely((p_buf = (BT_HDR *) GKI_dequeue(&p_dev->tx_queue)) == NULL))
    {
        BTUSB_DBG("dequeued data = NULL - just return\n");
        goto reschedule_task;
    }

    // bypass the header and retrieve the type
    p_data = (UINT8 *) (p_buf + 1) + p_buf->offset;

    // extract message type
    STREAM_TO_UINT8(msg_type, p_data);
    switch (msg_type)
    {
    case HCIT_TYPE_SCO_DATA:
        btusb_submit_voice_tx(p_dev, p_data, p_buf->len - 4);
        break;
    case HCIT_TYPE_COMMAND:
#ifdef BTUSB_LITE
        /* Filter User's Write operation to catch some commands(VSCs) */
        if (btusb_lite_hci_cmd_filter(p_dev, p_buf))
#endif
        {
            btusb_submit_cmd(p_dev, p_data, p_buf->len - 1);
        }
        break;
    case HCIT_TYPE_ACL_DATA:
        btusb_submit_acl(p_dev, p_data, p_buf->len - 1);
        break;
    case HCIT_TYPE_LM_DIAG:
        btusb_submit_diag(p_dev, p_data, p_buf->len - 1);
        break;
    }
    GKI_freebuf(p_buf);

reschedule_task:
    if (p_dev->tx_queue.count)
        tasklet_schedule(&p_dev->tx_task);
    return;
}

/*******************************************************************************
 **
 ** Function         btusb_enqueue
 **
 ** Description      Add a received USB message into the user queue
 **
 ** Parameters       p_dev: device instance control block
 **                  p_trans: transaction to forward
 **                  hcitype: type of HCI data in the USB transaction
 **
 ** Returns          void
 **
 *******************************************************************************/
void btusb_enqueue(tBTUSB_CB *p_dev, tBTUSB_TRANSACTION *p_trans, UINT8 hcitype)
{
    BT_HDR *p_buf = &p_trans->bt_hdr;
    struct urb *p_urb = p_trans->p_urb;
    UINT8 *p = (UINT8 *)p_urb->transfer_buffer;
    UINT16 len;

    BTUSB_DBG("len = %d\n", p_urb->actual_length);

    switch (hcitype)
    {
    case HCIT_TYPE_ACL_DATA:
        // retrieve the length in the packet header
        p += 2;
        STREAM_TO_UINT16(len, p);
        len += 4;
        break;

    case HCIT_TYPE_EVENT:
        // retrieve the length in the packet header
        p += 1;
        STREAM_TO_UINT8(len, p);
        len += 2;
        break;

    case HCIT_TYPE_LM_DIAG:
        len = HCIT_LM_DIAG_LENGTH;
        break;

    default:
        BTUSB_ERR("Unsupported HCI type: %d\n", hcitype);
        len = p_urb->actual_length;
        break;
    }

    // quirk for the ZLP HW issue
    if (unlikely(((len % 16) == 0) && (p_dev->quirks & BTUSB_QUIRK_ZLP_RX_WA) &&
                 (p_urb->actual_length == (len + 1))))
    {
        BTUSB_DBG("missing ZLP workaround: %d != %d\n", p_urb->actual_length, len);
        p_urb->actual_length = len;
    }
    // check if the length matches that of the ACL packet
    if (unlikely(len != p_urb->actual_length))
    {
        BTUSB_ERR("URB data length does not match packet %d != %d\n", p_urb->actual_length, len);
    }

    // fill up the header of the message
    p_buf->event = hcitype;
    p_buf->len = p_urb->actual_length;
    p_buf->offset = 0;
    p_buf->layer_specific &= ~BTUSB_LS_H4_TYPE_SENT;

    // InQ for user-space to read
    GKI_enqueue(&p_dev->rx_queue, p_buf);

    // if the process is polling, indicate RX event
    wake_up_interruptible(&p_dev->rx_wait_q);
}

/*******************************************************************************
 **
 ** Function         btusb_dequeued
 **
 ** Description      Figure out what to do with a dequeued message
 **
 ** Parameters       p_dev: driver control block
 **                  p_msg: dequeued message
 **
 ** Returns          void
 **
 *******************************************************************************/
void btusb_dequeued(tBTUSB_CB *p_dev, BT_HDR *p_msg)
{
    tBTUSB_TRANSACTION *p_trans = container_of(p_msg, tBTUSB_TRANSACTION, bt_hdr);

    switch (p_msg->event)
    {
    case HCIT_TYPE_ACL_DATA:
        if (unlikely(btusb_submit(p_dev, &p_dev->acl_rx_submitted, p_trans, GFP_KERNEL)))
            p_dev->stats.acl_rx_submit_err++;
        else
            p_dev->stats.acl_rx_submit_ok++;
        break;

    case HCIT_TYPE_EVENT:
#if defined(BTUSB_LITE)
        // when BTUSB_LITE is defined, EVT may be resubmitted from IRQ context
        if (unlikely(btusb_submit(p_dev, &p_dev->event_submitted, p_trans, GFP_ATOMIC)))
#else
        if (unlikely(btusb_submit(p_dev, &p_dev->event_submitted, p_trans, GFP_KERNEL)))
#endif
            p_dev->stats.event_submit_err++;
        else
            p_dev->stats.event_submit_ok++;
        break;

    case HCIT_TYPE_LM_DIAG:
        if (unlikely(btusb_submit(p_dev, &p_dev->diag_rx_submitted, p_trans, GFP_KERNEL)))
            p_dev->stats.diag_rx_submit_err++;
        else
            p_dev->stats.diag_rx_submit_ok++;
        break;

    default:
        GKI_freebuf(p_msg);
        break;
    }
}

/*******************************************************************************
 **
 ** Function         btusb_open
 **
 ** Description      User mode open callback
 **
 *******************************************************************************/
int btusb_open(struct inode *inode, struct file *file)
{
    tBTUSB_CB *p_dev;
    struct usb_interface *interface;
    int subminor;
    int retval = 0;
    BT_HDR *p_buf;

    BTUSB_INFO("enter\n");

    // retrieve the minor for this inode
    subminor = iminor(inode);

    // retrieve the USB interface attached to this minor
    interface = usb_find_interface(&btusb_driver, subminor);
    if (!interface)
    {
        BTUSB_ERR("can't find interface for minor %d\n", subminor);
        retval = -ENODEV;
        goto exit;
    }
    BTUSB_INFO("minor %u\n", subminor);

    // retrieve the device driver structure pointer
    p_dev = usb_get_intfdata(interface);
    BTUSB_INFO("p_dev=%p\n", p_dev);
    if (!p_dev)
    {
        BTUSB_ERR("can't find device\n");
        retval = -ENODEV;
        goto exit;
    }

    mutex_lock(&p_dev->open_mutex);
    if (p_dev->opened)
    {
        mutex_unlock(&p_dev->open_mutex);
        BTUSB_ERR("multiple open not allowed %d\n", p_dev->opened);
        retval = -EBUSY;
        goto exit;
    }
    // save our object in the file's private structure
    file->private_data = p_dev;
    p_dev->opened = true;
    mutex_unlock(&p_dev->open_mutex);

    // increment our usage count for the device
    BTUSB_INFO("kref_get -> &p_dev->kref 0x%p\n", &p_dev->kref);
    kref_get(&p_dev->kref);

    // free all pending messages in Rx queue
    BTUSB_DBG("Free Rx Queue count:%d\n", p_dev->rx_queue.count);
    while ((p_buf = (BT_HDR *)GKI_dequeue(&p_dev->rx_queue)) != NULL)
    {
        btusb_dequeued(p_dev, p_buf);
    }

exit:
    return retval;
}

/*******************************************************************************
 **
 ** Function         btusb_release
 **
 ** Description      User mode close callback
 **
 *******************************************************************************/
int btusb_release(struct inode *inode, struct file *file)
{
    tBTUSB_CB *p_dev;
    BT_HDR *p_buf = NULL;
    int idx;

    BTUSB_INFO("enter\n");

    p_dev = (tBTUSB_CB *) file->private_data;
    BTUSB_INFO("p_dev=%p\n", p_dev);
    if (p_dev == NULL)
    {
        BTUSB_ERR("can't find device\n");
        return -ENODEV;
    }

#ifdef BTUSB_LITE
    btusb_lite_stop_all(p_dev);
#endif

    // indicate driver is closed
    mutex_lock(&p_dev->open_mutex);
    p_dev->opened = false;
    mutex_unlock(&p_dev->open_mutex);

    // free all the pending elements
    while ((p_buf = (BT_HDR *) GKI_dequeue(&p_dev->tx_queue)) != NULL)
    {
        GKI_freebuf(p_buf);
    }

    while ((p_buf = (BT_HDR *) GKI_dequeue(&p_dev->rx_queue)) != NULL)
    {
        btusb_dequeued(p_dev, p_buf);
    }

    // ensure there is no process hanging on poll / select
    wake_up_interruptible(&p_dev->rx_wait_q);

    for (idx = 0; idx < ARRAY_SIZE(p_dev->voice_channels); idx++)
    {
        if (p_dev->voice_channels[idx].used)
        {
            if (btusb_remove_voice_channel(p_dev, p_dev->voice_channels[idx].info.sco_handle))
            {
                BTUSB_ERR("btusb_remove_voice_channel failed\n");
            }
        }
    }

    for (idx = 0; idx < BTUSB_NUM_OF_VOICE_TX_BUFFERS; idx++)
    {
        p_dev->p_voicetxIrpList[idx].used = 0;
    }

    // decrement the usage count on our device
    BTUSB_INFO("kref_put -> &p_dev->kref 0x%p\n", &p_dev->kref);
    kref_put(&p_dev->kref, btusb_delete);

    return 0;
}

/*******************************************************************************
 **
 ** Function         btusb_get_rx_packet
 **
 ** Description      Get (extract) next buffer to sent to User Space
 **
 *******************************************************************************/
static BT_HDR *btusb_get_rx_packet(struct btusb_cb *p_dev)
{
    // if there is a pending Rx buffer
    if (p_dev->p_rx_msg)
    {
        return p_dev->p_rx_msg;
    }
#ifdef BTUSB_LITE
    /* If HCI is over IPC */
    if (btusb_lite_is_hci_over_ipc(p_dev))
    {
        return NULL;
    }
#endif

    // dequeue the next buffer and store its reference
    p_dev->p_rx_msg = GKI_dequeue(&p_dev->rx_queue);

    return p_dev->p_rx_msg;
}

/*******************************************************************************
 **
 ** Function         btusb_free_rx_packet
 **
 ** Description      Free buffer sent to User Space
 **
 *******************************************************************************/
static void btusb_free_rx_packet(struct btusb_cb *p_dev, BT_HDR *p_msg)
{
    if (unlikely(p_msg != p_dev->p_rx_msg))
    {
        BTUSB_ERR("Dequeued buffer (%p) does not matched working buffer (%p)\n",
                p_msg, p_dev->p_rx_msg);
    }

    p_dev->p_rx_msg = NULL;

    if (unlikely(p_msg->layer_specific & BTUSB_LS_GKI_BUFFER))
    {
        GKI_freebuf(p_msg);
    }
    else
    {
        // buffer received from USB -> resubmit
        btusb_dequeued(p_dev, p_msg);
    }
}

/*******************************************************************************
 **
 ** Function         btusb_get_rx_packet_buffer
 **
 ** Description      Get pointer address of data to send to User Space
 **
 *******************************************************************************/
static UINT8 *btusb_get_rx_packet_buffer(struct btusb_cb *p_dev, BT_HDR *p_msg)
{
    tBTUSB_TRANSACTION *p_trans;

    switch(p_msg->event)
    {
    case HCIT_TYPE_ACL_DATA:
    case HCIT_TYPE_EVENT:
    case HCIT_TYPE_LM_DIAG:
        if (unlikely(p_msg->layer_specific & BTUSB_LS_GKI_BUFFER))
        {
            return ((char *)(p_msg + 1) + p_msg->offset);
        }
        else
        {
            p_trans = container_of(p_msg, tBTUSB_TRANSACTION, bt_hdr);
            return (p_trans->dma_buffer + p_msg->offset);
        }
    default: // SCO etc...
        return ((char *)(p_msg + 1) + p_msg->offset);
    }
}

/*******************************************************************************
 **
 ** Function         btusb_read
 **
 ** Description      User mode read
 **
 ** Returns          Number of bytes read, error number if negative
 **
 *******************************************************************************/
ssize_t btusb_read(struct file *file, char __user *buffer, size_t count, loff_t *ppos)
{
    tBTUSB_CB *p_dev = (tBTUSB_CB *)file->private_data;
    UINT16 len;
    size_t remainder = count;
    BT_HDR *p_buf = NULL;
    char *p_data;
    char type;
    int retval;

    BTUSB_DBG("p_dev=%p count=%zu\n", p_dev, count);
    if (unlikely(p_dev == NULL))
    {
        BTUSB_ERR("can't find device\n");
        return -ENODEV;
    }

    if (unlikely(!p_dev->p_main_intf))
    {
        BTUSB_ERR("device unplugged\n");
        return -ENODEV;
    }

    if (unlikely(!p_dev->opened))
    {
        BTUSB_ERR("driver is not open\n");
        return -EINVAL;
    }

    // if read is non blocking and there is no data
    if (unlikely((file->f_flags & O_NONBLOCK) && (btusb_get_rx_packet(p_dev) == NULL)))
    {
        BTUSB_INFO("Non blocking read without any data\n");
        return -EAGAIN;
    }

    // wait for an event
    retval = wait_event_interruptible(p_dev->rx_wait_q,
            ((p_buf = btusb_get_rx_packet(p_dev)) != NULL));
    if (unlikely(retval))
    {
        BTUSB_DBG("read wait interrupted\n");
        return retval;
    }
    BTUSB_DBG("buffer=%p len=%u ls=%u\n", p_buf, p_buf->len, p_buf->layer_specific);

    switch(p_buf->event)
    {
    case HCIT_TYPE_ACL_DATA:
    case HCIT_TYPE_EVENT:
    case HCIT_TYPE_LM_DIAG:
        // if this is the first time buffer is picked, add HCI header in user space
        if (likely(!(p_buf->layer_specific & BTUSB_LS_H4_TYPE_SENT)))
        {
            if (likely(remainder >= 1))
            {
                type = p_buf->event;
                // add the H4 packet header
                if (unlikely(copy_to_user(buffer, &type, 1)))
                {
                    BTUSB_ERR("copy to user error\n");
                    return -EINVAL;
                }
                buffer += 1;
                remainder -= 1;
            }
            else
            {
                BTUSB_ERR("Not enough space to copy H4 ACL header\n");
                goto read_end;
            }
        }
        break;

    default:
        break;
    }

    // retrieve address of the next data to send (depends on event)
    p_data = btusb_get_rx_packet_buffer(p_dev, p_buf);

    // take the min of remainder and p_buf length
    if (likely(p_buf->len < remainder))
    {
        len = p_buf->len;
    }
    else
    {
        len = remainder;
    }

    // copy the message data to the available user space
    if (unlikely(copy_to_user(buffer, p_data, len)))
    {
        BTUSB_ERR("copy to user error\n");
        return -EINVAL;
    }
    remainder -= len;
    p_buf->len -= len;
    p_buf->offset += len;
    p_buf->layer_specific |= BTUSB_LS_H4_TYPE_SENT;

    // free the first buffer if it is empty
    if (likely(p_buf->len == 0))
    {
        // free (fake event) or resubmit (real USB buffer) the buffer sent to app
        btusb_free_rx_packet(p_dev, p_buf);
    }

read_end:
    return (count - remainder);
}

/*******************************************************************************
 **
 ** Function         btusb_write
 **
 ** Description      User mode write
 **
 ** Returns          Number of bytes written, error number if negative
 **
 *******************************************************************************/
ssize_t btusb_write(struct file *file, const char __user *user_buffer, size_t count, loff_t *ppos)
{
    tBTUSB_CB *p_dev;
    BT_HDR *p_msg;
    UINT8 *p_data;
    size_t remainder = count;
    int err;
    UINT16 appended, len;

    p_dev = (tBTUSB_CB *)file->private_data;

    BTUSB_DBG("p_dev=%p count=%zu\n", p_dev, count);

    if (unlikely(p_dev == NULL))
    {
        BTUSB_ERR("can't find device\n");
        return -ENODEV;
    }

    if (unlikely(!p_dev->p_main_intf))
    {
        BTUSB_ERR("device unplugged\n");
        return -ENODEV;
    }

    if (unlikely(!p_dev->opened))
    {
        return -EINVAL;
    }

    if (unlikely(count == 0))
    {
        return 0;
    }

    // check that the incoming data is good
    if (unlikely(!access_ok(VERIFY_READ, (void *)user_buffer, count)))
    {
        BTUSB_ERR("buffer access verification failed\n");
        return -EFAULT;
    }

    while (remainder)
    {
        BTUSB_DBG("remain=%zu p_write_msg=%p\n", remainder, p_dev->p_write_msg);
        // check if there is already an active transmission buffer
        p_msg = p_dev->p_write_msg;
        if (likely(!p_msg))
        {
            // get a buffer from the pool
            p_msg = (BT_HDR *)GKI_getbuf(sizeof(BT_HDR) + BTUSB_H4_MAX_SIZE);
            if (unlikely(!p_msg))
            {
                BTUSB_ERR("unable to get GKI buffer - write failed\n");
                return -EINVAL;
            }
            if (unlikely(dbgflags & BTUSB_GKI_CHK_MSG) &&
                unlikely(GKI_buffer_status(p_msg) != BUF_STATUS_UNLINKED))
            {
                BTUSB_ERR("buffer != BUF_STATUS_UNLINKED 0x%p\n", p_msg);
                return -EINVAL;
            }
            p_dev->p_write_msg = p_msg;

            p_msg->event = 0;
            p_msg->len = 0;
            p_msg->offset = 0;
            p_msg->layer_specific = 0;
        }
        p_data = (UINT8 *)(p_msg + 1) + p_msg->offset;

        // append the entire data to the buffer (not exceeding buffer length)
        if (likely(remainder < (BTUSB_H4_MAX_SIZE - p_msg->len)))
            appended = remainder;
        else
            appended = BTUSB_H4_MAX_SIZE - p_msg->len;
        if (unlikely(copy_from_user(p_data + p_msg->len, (void *)user_buffer, appended)))
        {
            BTUSB_ERR("copy from user error\n");
            return -EINVAL;
        }
        BTUSB_DBG("msg_len=%u appended=%u\n", p_msg->len, appended);

        // update the size of the buffer
        p_msg->len += appended;

        // compute the real HCI packet length (by default 0xFFFF to mark incomplete)
        len = 0xFFFF;
        switch(p_data[0])
        {
            case HCIT_TYPE_SCO_DATA:
            case HCIT_TYPE_COMMAND:
                if (likely(p_msg->len >= 4))
                {
                    // bypass HCI type + opcode/connection handle
                    p_data += 3;
                    STREAM_TO_UINT8(len, p_data);
                    len += 4;
                    BTUSB_DBG("SCO/CMD len=%u cur=%u\n", len, p_msg->len);
                }
                break;
            case HCIT_TYPE_ACL_DATA:
                if (likely(p_msg->len >= 5))
                {
                    // bypass HCI type + connection handle
                    p_data += 3;
                    STREAM_TO_UINT16(len, p_data);
                    len += 5;
                    // sanity check : ACL buffer should not be larger than supported
                    if (unlikely(len > BTUSB_H4_MAX_SIZE))
                    {
                        BTUSB_ERR("ACL packet too long (%u)\n", len);
                        GKI_freebuf(p_dev->p_write_msg);
                        p_dev->p_write_msg = NULL;
                        return -EINVAL;
                    }
                    BTUSB_DBG("ACL len=%u cur=%u\n", len, p_msg->len);
                }
                break;
            case HCIT_TYPE_LM_DIAG:
                // this packet does not have a length, so just send everything
                len = p_msg->len;
                BTUSB_DBG("DIAG len=%u cur=%u\n", len, p_msg->len);
                break;
            default:
                BTUSB_ERR("unsupported packet type\n");
                return count;
                break;
        }
        // check if the buffer length exceeds the packet length
        if (likely(p_msg->len >= len))
        {
            // remove the extra data (belonging to next HCI packet)
            if (unlikely(p_msg->len > len))
            {
                // remove exceeding data
                appended -= p_msg->len - len;
                // set len to computed HCI packet length
                p_msg->len = len;
            }
            if (autopm)
            {
                err = usb_autopm_get_interface(p_dev->p_main_intf);
                if (unlikely(err < 0))
                {
                    BTUSB_ERR("autopm failed\n");
                    autopm = 0;
                }
            }

            // add the incoming data and notify the btusb_tx_task to process
            GKI_enqueue(&p_dev->tx_queue, p_msg);

            // wake up tasklet
            tasklet_schedule(&p_dev->tx_task);

            // new write message
            p_dev->p_write_msg = NULL;
        }
        remainder -= appended;
        user_buffer += appended;
    }

    return count;
}

/*******************************************************************************
 **
 ** Function         btusb_poll
 **
 ** Description      Poll callback (to implement select)
 **
 ** Parameters       file : file structure of the opened instance
 **                  p_pt : poll table to which the local queue must be added
 **
 ** Returns          Mask of the type of polling supported, error number if negative
 **
 *******************************************************************************/
unsigned int btusb_poll(struct file *file, struct poll_table_struct *p_pt)
{
    tBTUSB_CB *p_dev;
    unsigned int mask;

    BTUSB_DBG("enter\n");

    // retrieve the device from the file pointer
    p_dev = (tBTUSB_CB *) file->private_data;
    BTUSB_DBG("p_dev=%p\n", p_dev);
    if (unlikely(p_dev == NULL))
    {
        BTUSB_ERR("can't find device\n");
        return -ENODEV;
    }

    // check if the device was disconnected
    if (unlikely(!p_dev->p_main_intf))
    {
        BTUSB_ERR("device unplugged\n");
        return -ENODEV;
    }

    if (unlikely(!p_dev->opened))
    {
        return -EINVAL;
    }

    // indicate to the system on which queue it should poll (non blocking call)
    poll_wait(file, &p_dev->rx_wait_q, p_pt);

    // enable WRITE (select/poll is not write blocking)
    mask = POLLOUT | POLLWRNORM;

    // enable READ only if data is queued from HCI
    if (btusb_get_rx_packet(p_dev))
    {
        mask |= POLLIN | POLLRDNORM;
    }

    return mask;
}

/*******************************************************************************
 **
 ** Function         btusb_print_voice_stat
 **
 ** Description      SCO print voice stats
 **
 ** Parameters       p_stat buffer holding the statistic to display.
 **
 ** Returns          void.
 **
 *******************************************************************************/
void btusb_print_voice_stat(tBTUSB_STATS * p_stat)
{
    printk("______________btusb_print_voice_stat______________\n");

    printk("Number of voice reqs submitted USB  stack %lu\n", p_stat->voicerx_submitted);
    printk("Number of voice rx submitted in error %lu\n", p_stat->voicerx_submitted_error);
    printk("Number of voice req completions from the USB software stack %lu\n", p_stat->voicerx_completed);
    printk("Number of Voice req completions in error %lu\n", p_stat->voicerx_completed_error);
    printk("Voice frames discarded due to bad status %lu\n", p_stat->voicerx_bad_packets);
    printk("Voice frames discarded due to no headers in data %lu\n", p_stat->voicerx_disc_nohdr);

    printk("Raw bytes received %lu\n",p_stat->voicerx_raw_bytes);
    printk("Skipped bytes because no header matched %lu\n",p_stat->voicerx_skipped_bytes);
    printk("Number of Voice Tx reqs submitted %lu\n",p_stat->voicetx_submitted);
    printk("Number of voice tx submitted in error %lu\n",p_stat->voicetx_submitted_error);
    printk("Number of Voice Tx reqs completed %lu\n",p_stat->voicetx_completed);
    printk("Number of Voice Tx reqs completed in error %lu\n",p_stat->voicetx_completed_error);
    printk("Number of Voice Tx not submitted due to no room on the tx queue %lu\n",
            p_stat->voicetx_disc_nobuf);
    printk("Number of Voice tx not submitted due to too long data %lu\n",
            p_stat->voicetx_disc_toolong);

    printk("voice pending counter %lu, max %lu\n", p_stat->voice_tx_cnt, p_stat->voice_max_tx_cnt);
    p_stat->voice_tx_cnt=0;p_stat->voice_max_tx_cnt=0;

    printk("Delta time between voice tx_done routine in us max: %lu, min: %lu\n", p_stat->voice_max_tx_done_delta_time, p_stat->voice_min_tx_done_delta_time);
    memset(&(p_stat->voice_tx_done_delta_time) , 0, sizeof(struct timeval));
    memset(&(p_stat->voice_last_tx_done_ts) , 0, sizeof(struct timeval));
    p_stat->voice_max_tx_done_delta_time = 0;
    p_stat->voice_min_tx_done_delta_time = 0;

    printk("Delta time between voice tx feeding routine in us max: %lu, min: %lu\n", p_stat->voice_max_rx_feeding_interval, p_stat->voice_min_rx_feeding_interval);
    memset(&(p_stat->voice_rx_feeding_interval) , 0, sizeof(struct timeval));
    memset(&(p_stat->voice_last_rx_feeding_ts) , 0, sizeof(struct timeval));
    p_stat->voice_max_rx_feeding_interval = 0;
    p_stat->voice_min_rx_feeding_interval = 0;

    printk("Delta time between voice rx_rdy routine max in us: %lu, min : %lu\n", p_stat->voice_max_rx_rdy_delta_time, p_stat->voice_min_rx_rdy_delta_time);
    memset(&(p_stat->voice_rx_rdy_delta_time) , 0, sizeof(struct timeval));
    memset(&(p_stat->voice_last_rx_rdy_ts) , 0, sizeof(struct timeval));
    p_stat->voice_max_rx_rdy_delta_time = 0;
    p_stat->voice_min_rx_rdy_delta_time = 0;

    printk("_______________________________________________________\n");
}

#define BTUSB_RETURN_STR(__c) case __c: return #__c
static const char *btusb_ioctl_string(unsigned int cmd)
{
    switch(cmd)
    {
        BTUSB_RETURN_STR(IOCTL_BTWUSB_GET_STATS);
        BTUSB_RETURN_STR(IOCTL_BTWUSB_CLEAR_STATS);
        BTUSB_RETURN_STR(IOCTL_BTWUSB_ADD_VOICE_CHANNEL);
        BTUSB_RETURN_STR(IOCTL_BTWUSB_REMOVE_VOICE_CHANNEL);
        BTUSB_RETURN_STR(TCGETS);
        BTUSB_RETURN_STR(TCSETS);
        BTUSB_RETURN_STR(TCSETSW);
        BTUSB_RETURN_STR(TCSETSF);
        BTUSB_RETURN_STR(TCGETA);
        BTUSB_RETURN_STR(TCSETA);
        BTUSB_RETURN_STR(TCSETAW);
        BTUSB_RETURN_STR(TCSETAF);
        BTUSB_RETURN_STR(TCSBRK);
        BTUSB_RETURN_STR(TCXONC);
        BTUSB_RETURN_STR(TCFLSH);
        BTUSB_RETURN_STR(TIOCGSOFTCAR);
        BTUSB_RETURN_STR(TIOCSSOFTCAR);
        BTUSB_RETURN_STR(TIOCGLCKTRMIOS);
        BTUSB_RETURN_STR(TIOCSLCKTRMIOS);
#ifdef TIOCGETP
        BTUSB_RETURN_STR(TIOCGETP);
        BTUSB_RETURN_STR(TIOCSETP);
        BTUSB_RETURN_STR(TIOCSETN);
#endif
#ifdef TIOCGETC
        BTUSB_RETURN_STR(TIOCGETC);
        BTUSB_RETURN_STR(TIOCSETC);
#endif
#ifdef TIOCGLTC
        BTUSB_RETURN_STR(TIOCGLTC);
        BTUSB_RETURN_STR(TIOCSLTC);
#endif
#ifdef TCGETX
        BTUSB_RETURN_STR(TCGETX);
        BTUSB_RETURN_STR(TCSETX);
        BTUSB_RETURN_STR(TCSETXW);
        BTUSB_RETURN_STR(TCSETXF);
#endif
        BTUSB_RETURN_STR(TIOCMGET);
        BTUSB_RETURN_STR(TIOCMSET);
        BTUSB_RETURN_STR(TIOCGSERIAL);
        BTUSB_RETURN_STR(TIOCMIWAIT);
        BTUSB_RETURN_STR(TIOCMBIC);
        BTUSB_RETURN_STR(TIOCMBIS);
        default:
            return "unknwown";
    }
}

/*******************************************************************************
 **
 ** Function         btusb_ioctl
 **
 ** Description      User mode ioctl
 **
 ** Parameters
 **
 ** Returns          0 upon success or an error code.
 **
 *******************************************************************************/
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
long btusb_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
#else
int btusb_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
#endif
{
    const char *s_cmd;
    tBTUSB_CB *p_dev;
    tBTUSB_SCO_INFO sco_info;
    int retval = 0;

    s_cmd = btusb_ioctl_string(cmd);

    BTUSB_INFO("cmd=%s\n", s_cmd);

    p_dev = (tBTUSB_CB *) file->private_data;
    BTUSB_INFO("p_dev=%p\n", p_dev);
    if (p_dev == NULL)
    {
        BTUSB_ERR("can't find device\n");
        return -ENODEV;
    }

    // check if the device was disconnected
    if (unlikely(!p_dev->p_main_intf))
    {
        BTUSB_ERR("device unplugged\n");
        return -ENODEV;
    }

    switch (cmd)
    {
        case IOCTL_BTWUSB_GET_STATS:
            if (copy_to_user((void *) arg, &p_dev->stats, sizeof(tBTUSB_STATS)))
            {
                retval = -EFAULT;
                goto err_out;
            }
            break;

        case IOCTL_BTWUSB_CLEAR_STATS:
            memset(&p_dev->stats, 0, sizeof(tBTUSB_STATS));
            break;

        case IOCTL_BTWUSB_ADD_VOICE_CHANNEL:
            if (copy_from_user(&sco_info, (void *) arg, sizeof(sco_info)))
            {
                BTUSB_ERR("BTUSB_IOCTL_ADD_VOICE_CHANNEL failed getting 1st par\n");
                retval = -EFAULT;
                goto err_out;
            }
            return btusb_add_voice_channel(p_dev, sco_info.sco_handle, sco_info.burst);

        case IOCTL_BTWUSB_REMOVE_VOICE_CHANNEL:
            if (copy_from_user(&sco_info, (void *) arg, sizeof(sco_info)))
            {
                BTUSB_ERR("BTUSB_IOCTL_ADD_VOICE_CHANNEL failed getting 1st par\n");
                retval = -EFAULT;
                goto err_out;
            }
            return btusb_remove_voice_channel(p_dev, sco_info.sco_handle);

        case TCGETS:
            /* dummy support of TTY IOCTLs */
            if (!arg)
            {
                retval = -EFAULT;
                goto err_out;
            }
#ifndef TCGETS2
            if (kernel_termios_to_user_termios((struct termios __user *)arg, &p_dev->kterm))
#else
            if (kernel_termios_to_user_termios_1((struct termios __user *)arg, &p_dev->kterm))
#endif
            {
                BTUSB_ERR("failure copying termios\n");
                retval = -EFAULT;
                goto err_out;
            }
            break;

        case TCSETSW:
        case TCSETS:
            if (!arg)
            {
                retval = -EFAULT;
                goto err_out;
            }
#ifndef TCGETS2
            if (user_termios_to_kernel_termios(&p_dev->kterm, (struct termios __user *)arg))
#else
            if (user_termios_to_kernel_termios_1(&p_dev->kterm, (struct termios __user *)arg))
#endif
            {
                retval = -EFAULT;
                goto err_out;
            }
            break;

        case TCSETSF:
        case TCGETA:
        case TCSETA:
        case TCSETAW:
        case TCSETAF:
        case TCSBRK:
        case TCXONC:
        case TCFLSH:
        case TIOCGSOFTCAR:
        case TIOCSSOFTCAR:
        case TIOCGLCKTRMIOS:
        case TIOCSLCKTRMIOS:
#ifdef TIOCGETP
        case TIOCGETP:
        case TIOCSETP:
        case TIOCSETN:
#endif
#ifdef TIOCGETC
        case TIOCGETC:
        case TIOCSETC:
#endif
#ifdef TIOCGLTC
        case TIOCGLTC:
        case TIOCSLTC:
#endif
#ifdef TCGETX
        case TCGETX:
        case TCSETX:
        case TCSETXW:
        case TCSETXF:
#endif
        case TIOCMSET:
        case TIOCMBIC:
        case TIOCMBIS:
            /* dummy support of TTY IOCTLs */
            BTUSB_DBG("TTY ioctl not implemented\n");
            break;

        case TIOCGSERIAL:
        {
            struct serial_struct tmp;
            if (!arg)
            {
                retval = -EFAULT;
                goto err_out;
            }
            memset(&tmp, 0, sizeof(tmp));
            tmp.type = 0;
            tmp.line = 0;
            tmp.port = 0;
            tmp.irq = 0;
            tmp.flags = ASYNC_SKIP_TEST | ASYNC_AUTO_IRQ;
            tmp.xmit_fifo_size = 4096;
            tmp.baud_base = 115200;
            tmp.close_delay = 5*HZ;
            tmp.closing_wait = 30*HZ;
            tmp.custom_divisor = 0;
            tmp.hub6 = 0;
            tmp.io_type = 0;
            if (copy_to_user((void __user *)arg, &tmp, sizeof(tmp)))
            {
                retval = -EFAULT;
                goto err_out;
            }
            retval = 0;
            break;
        }

        case TIOCMGET:
        {
            int tiocm = TIOCM_RTS | TIOCM_CTS;
            if (!arg)
            {
                retval = -EFAULT;
                goto err_out;
            }

            if (copy_to_user((void __user *)arg, &tiocm, sizeof(tiocm)))
            {
                retval = -EFAULT;
                goto err_out;
            }
            retval = 0;
            break;
        }

        case TIOCMIWAIT:
        {
            DECLARE_WAITQUEUE(wait, current);

            BTUSB_DBG("arg = %lu\n", arg);
            while (1)
            {
                BTUSB_DBG("adding task to wait list\n");
                add_wait_queue(&p_dev->rx_wait_q, &wait);
                set_current_state(TASK_INTERRUPTIBLE);
                schedule();
                BTUSB_DBG("removing task from wait list\n");
                remove_wait_queue(&p_dev->rx_wait_q, &wait);
                /* see if a signal woke us up */
                if (signal_pending(current))
                {
                    BTUSB_ERR("signal was pending\n");
                    retval = -ERESTARTSYS;
                    goto err_out;
                }
                // do not check the expected signals
                retval = 0;
                break;
            }
            break;
        }
        default:
            BTUSB_ERR("unknown cmd %u\n", cmd);
            retval = -ENOIOCTLCMD;
            break;
    }

err_out:
    BTUSB_DBG("returning %d\n", retval);
    return retval;
}

/*******************************************************************************
 **
 ** Function         btusb_write_complete
 **
 ** Description      Data write (bulk pipe) completion routine
 **
 ** Parameters       urb pointer
 **
 ** Returns          void
 **
 *******************************************************************************/
void btusb_write_complete(struct urb *p_urb)
{
    tBTUSB_TRANSACTION *p_trans = p_urb->context;
    tBTUSB_CB *p_dev = p_trans->context;

    BTUSB_DBG("status %d length %u flags %x\n", p_urb->status,
            p_urb->transfer_buffer_length, p_urb->transfer_flags);

    p_trans->gki_hdr.status = BUF_STATUS_FREE;

    if (unlikely(p_urb->status))
    {
        p_dev->stats.writes_completed_error++;
        printk(KERN_ERR "BTUSB btusb_write_complete failure %d\n", p_urb->status);
    }
    else
    {
        p_dev->stats.writes_completed++;
    }

    if(autopm)
    {
        usb_autopm_put_interface(p_dev->p_main_intf);
    }

    return;
}

/*******************************************************************************
 **
 ** Function         btusb_strip_off_acl_handle
 **
 ** Description      Strip off the ACL handle if this is firmware download packet
 **
 ** Parameters       
 **
 ** Returns          void
 **
 *******************************************************************************/
void btusb_strip_off_acl_handle(char* *packet, unsigned long *length)
{
    UINT8* p = (UINT8 *) (*packet);

    BTUSB_DBG("first bytes are %02X %02X\n", *p, *(p+1));
    
    // check special handle for firmware download packet
    if ((*p == 0xEF) && (*(p+1) == 0xBE))
    {
        *packet = (char *) (p + 4);
        *length -= 4;  
    }
}

/*******************************************************************************
 **
 ** Function         btusb_submit_acl
 **
 ** Description      ACL write submission to the controller
 **
 ** Parameters
 **
 ** Returns          0 upon success, negative value if error
 **
 *******************************************************************************/
int btusb_submit_acl(tBTUSB_CB *p_dev, char *packet, unsigned long length)
{
    int retval;
    tBTUSB_TRANSACTION *p_trans;

    BTUSB_DBG("%p[%lu]\n", packet, length);

    // strip off the ACL handle if this is a firmware download packet
    btusb_strip_off_acl_handle (&packet, &length);
    BTUSB_DBG("after adjusting %p[%lu]\n", packet, length);

    btusb_dump_data(packet, length, "OUTGOING DATA");



    p_trans = btusb_alloc_trans(p_dev->acl_tx_array, ARRAY_SIZE(p_dev->acl_tx_array));
    if (unlikely(p_trans == NULL))
    {
        return -ENOMEM;
    }

    if (unlikely(length > BTUSB_HCI_MAX_ACL_SIZE))
    {
        retval = -E2BIG;
        goto error;
    }

#if 0
    // if this is called directly from write call
    if (copy_from_user(p_trans->dma_buffer, user_buffer, length))
    {
            retval = -EFAULT;
            goto error;
    }
#else
    memcpy(p_trans->dma_buffer, packet, length);
#endif
    p_trans->p_urb->transfer_buffer_length = length;

    retval = btusb_submit(p_dev, &p_dev->acl_tx_submitted, p_trans, GFP_ATOMIC);
    if (unlikely(retval))
    {
        goto error;
    }
    return retval;

error:
    BTUSB_ERR("failed : %d\n", retval);
    p_trans->gki_hdr.status = BUF_STATUS_FREE;
    return retval;
#if 0
    return btusb_submit_bulk(p_dev, packet, length, p_dev->p_acl_out, &p_dev->acl_tx_submitted);
#endif
}

/*******************************************************************************
 **
 ** Function         btusb_submit_diag
 **
 ** Description      Diag write submission to the controller
 **
 ** Parameters
 **
 ** Returns          0 upon success, negative value if error
 **
 *******************************************************************************/
int btusb_submit_diag(tBTUSB_CB *p_dev, char *packet, unsigned long length)
{
    int retval;
    tBTUSB_TRANSACTION *p_trans;

    BTUSB_DBG("%p[%lu]\n", packet, length);

    btusb_dump_data(packet, length, "OUTGOING DIAG");
    if (unlikely(!p_dev->p_diag_out))
    {
        return -ENODEV;
    }

    p_trans = btusb_alloc_trans(p_dev->diag_tx_array, ARRAY_SIZE(p_dev->diag_tx_array));
    if (unlikely(p_trans == NULL))
    {
        return -ENOMEM;
    }

    if (unlikely(length > BTUSB_HCI_MAX_ACL_SIZE))
    {
        retval = -E2BIG;
        goto error;
    }

#if 0
    // if this is called directly from write call
    if (copy_from_user(p_trans->dma_buffer, user_buffer, length))
    {
            retval = -EFAULT;
            goto error;
    }
#else
    memcpy(p_trans->dma_buffer, packet, length);
#endif
    p_trans->p_urb->transfer_buffer_length = length;

    retval = btusb_submit(p_dev, &p_dev->diag_tx_submitted, p_trans, GFP_ATOMIC);
    if (unlikely(retval))
    {
        goto error;
    }
    return retval;

error:
    BTUSB_ERR("failed : %d\n", retval);
    p_trans->gki_hdr.status = BUF_STATUS_FREE;
    return retval;
}


/*******************************************************************************
 **
 ** Function         btusb_cmd_complete
 **
 ** Description      Command (control pipe) completion routine
 **
 ** Parameters
 **
 ** Returns          void.
 **
 *******************************************************************************/
void btusb_cmd_complete(struct urb *p_urb)
{
    tBTUSB_TRANSACTION *p_trans = p_urb->context;
    tBTUSB_CB *p_dev = p_trans->context;

    BTUSB_DBG("status %d %u length flags %x\n", p_urb->status,
            p_urb->transfer_buffer_length, p_urb->transfer_flags);

    p_trans->gki_hdr.status = BUF_STATUS_FREE;

    if (unlikely(p_urb->status))
    {
        BTUSB_ERR("failure %d\n", p_urb->status);
        p_dev->stats.commands_completed_error++;
    }
    else
    {
        p_dev->stats.commands_completed++;
    }

    if(autopm)
        usb_autopm_put_interface(p_dev->p_main_intf);

}

/*******************************************************************************
 **
 ** Function         btusb_submit_cmd
 **
 ** Description
 **
 ** Parameters
 **
 ** Returns          0 upon success, negative value if error
 **
 *******************************************************************************/
int btusb_submit_cmd(tBTUSB_CB *p_dev, char *packet, unsigned long length)
{
    int retval;
    tBTUSB_TRANSACTION *p_trans;
    struct usb_ctrlrequest *p_dr = NULL;

    BTUSB_DBG("%p[%lu]\n", packet, length);

    btusb_dump_data(packet, length, "OUTGOING CMD");

    p_trans = btusb_alloc_trans(p_dev->cmd_array, ARRAY_SIZE(p_dev->cmd_array));
    if (unlikely(p_trans == NULL))
    {
        return -ENOMEM;
    }

    if (unlikely(length > BTUSB_HCI_MAX_CMD_SIZE))
    {
        retval = -E2BIG;
        goto error;
    }

#if 0
    // if this is called directly from write call
    if (copy_from_user(p_trans->dma_buffer, packet, length))
    {
        retval = -EFAULT;
        goto error;
    }
#else
    memcpy(p_trans->dma_buffer, packet, length);
#endif
    p_dr = (void *)p_trans->p_urb->setup_packet;
    p_dr->wLength = __cpu_to_le16(length);
    p_trans->p_urb->transfer_buffer_length = length;

    retval = btusb_submit(p_dev, &p_dev->cmd_submitted, p_trans, GFP_ATOMIC);
    if (unlikely(retval))
    {
        p_dev->stats.commands_submitted_error++;
        goto error;
    }
    return retval;

error:
    p_trans->gki_hdr.status = BUF_STATUS_FREE;
    return retval;
}

/*******************************************************************************
 **
 ** Function         btusb_voicetx_complete
 **
 ** Description      Voice write (iso pipe) completion routine.
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
static void btusb_voicetx_complete(struct urb *p_urb)
{
    tBTUSB_ISO_ELEMENT *pIsoXfer = p_urb->context;
    tBTUSB_CB *p_dev = (void *) pIsoXfer->dev;

    btusb_voice_stats(&(p_dev->stats.voice_max_tx_done_delta_time), &(p_dev->stats.voice_min_tx_done_delta_time),
            &(p_dev->stats.voice_tx_done_delta_time), &(p_dev->stats.voice_last_tx_done_ts));

    // keep track on the number of Tx buffer pending
    p_dev->stats.voice_tx_cnt--;

    if (unlikely(!p_dev->p_main_intf))
        goto exit;

    pIsoXfer->used = 0;

    if (unlikely(p_urb->status))
    {
        BTUSB_ERR("btusb_voicetx_complete failure %d\n", p_urb->status);
        p_dev->stats.voicetx_completed_error++;
    }
    else
    {
        p_dev->stats.voicetx_completed++;
    }

exit:
    return;
}

/*******************************************************************************
 **
 ** Function         btusb_fill_iso_pkts
 **
 ** Description      Useful voice utility.
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
static void btusb_fill_iso_pkts(struct urb *p_urb, int len, int pkt_size)
{
    int off = 0, i;

    for (i = 0; len >= pkt_size; i++, off += pkt_size, len -= pkt_size)
    {
        p_urb->iso_frame_desc[i].offset = off;
        p_urb->iso_frame_desc[i].length = pkt_size;
    }

    // remainder?
    if (len)
    {
        p_urb->iso_frame_desc[i].offset = off;
        p_urb->iso_frame_desc[i].length = len;
        i++;
    }

    p_urb->number_of_packets = i;
    return;
}

/*******************************************************************************
 **
 ** Function         btusb_fill_iso_rx_pkts
 **
 ** Description      Useful voice utility.
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
static void btusb_fill_iso_rx_pkts(struct urb *p_urb, int len, int pkt_size)
{
    int off = 0, i;

    for (i = 0; len >= pkt_size; i++, off += ALIGN(pkt_size, 4), len -= ALIGN(pkt_size, 4))
    {
        p_urb->iso_frame_desc[i].offset = off;
        p_urb->iso_frame_desc[i].length = pkt_size;
        BTUSB_DBG("off=%d pkt_size=%d\n", off, pkt_size);
    }
    p_urb->number_of_packets = i;
    return;
}

/*******************************************************************************
 **
 ** Function         btusb_submit_voice_rx
 **
 ** Description      Resubmit an already filled URB request
 **
 ** Parameters
 **
 ** Returns
 **
 *******************************************************************************/
void btusb_submit_voice_rx(tBTUSB_CB *p_dev, tBTUSB_TRANSACTION *p_trans, int mem_flags)
{
    if (unlikely(btusb_submit(p_dev, &p_dev->voice_rx_submitted, p_trans, mem_flags)))
    {
        p_dev->stats.voicerx_submitted_error++;
    }
    else
    {
        p_dev->stats.voicerx_submitted++;
    }

}

/*******************************************************************************
 **
 ** Function         btusb_update_voice_rx
 **
 ** Description      Finish filling the voice URB and submit them.  At this point
 **                  the URBs should NOT be submitted.
 **
 ** Parameters
 **
 ** Returns
 **
 *******************************************************************************/
static void btusb_update_voice_rx(tBTUSB_CB *p_dev)
{
    unsigned int idx;
    tBTUSB_TRANSACTION *p_trans;
    struct urb *p_urb;
    int packet_size;

    BTUSB_DBG("enter\n");

    if (!p_dev->p_main_intf || !p_dev->p_voice_intf || !p_dev->p_voice_in)
    {
        BTUSB_ERR("failing (p_dev removed or no voice intf)\n");
        return;
    }

    packet_size = le16_to_cpu(p_dev->p_voice_in->desc.wMaxPacketSize);
    if (!packet_size)
    {
        BTUSB_ERR("failing, 0 packet size)\n");
        return;
    }
    BTUSB_DBG("packet_size=%d\n", packet_size);

    BTUSB_ARRAY_FOR_EACH_TRANS(p_dev->voice_rx_array)
    {
        p_urb = p_trans->p_urb;
        p_urb->pipe = usb_rcvisocpipe(p_dev->p_udev, p_dev->p_voice_in->desc.bEndpointAddress);
        BTUSB_DBG("ep=%d\n", p_dev->p_voice_in->desc.bEndpointAddress);
        p_urb->interval = p_dev->p_voice_in->desc.bInterval;
        BTUSB_DBG("interval=%d\n", p_urb->interval);
        p_urb->transfer_buffer_length = ALIGN(packet_size, 4) * BTUSB_VOICE_FRAMES_PER_URB;
        BTUSB_DBG("transfer_buffer_length=%d\n", p_urb->transfer_buffer_length);
        if (p_urb->transfer_buffer_length > BTUSB_VOICE_BUFFER_MAXSIZE)
        {
            BTUSB_ERR("Required buffer size larger than allocated size\n");
            p_urb->transfer_buffer_length = BTUSB_VOICE_BUFFER_MAXSIZE;
        }
        btusb_fill_iso_rx_pkts(p_urb, p_urb->transfer_buffer_length, packet_size);

        btusb_submit_voice_rx(p_dev, p_trans, GFP_KERNEL);
    }
}


/*******************************************************************************
 **
 ** Function         btusb_submit_voice_tx
 **
 ** Description      Voice write submission
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
static void btusb_submit_voice_tx(tBTUSB_CB *p_dev, UINT8 *p_data, int len)
{
    int to_send = 0;

    btusb_voice_stats(&(p_dev->stats.voice_max_rx_feeding_interval), &(p_dev->stats.voice_min_rx_feeding_interval),
            &(p_dev->stats.voice_rx_feeding_interval), &(p_dev->stats.voice_last_rx_feeding_ts));

    while (len > 0)
    {
        if (len >= BTUSB_VOICE_BURST_SIZE)
        {
            to_send = BTUSB_VOICE_BURST_SIZE;
        }
        else
        {
            BTUSB_DBG("Sending partial sco data len: %d\n", len);
        }
        p_data[2] = to_send;
        len -= to_send;
        btusb_submit_write_voice(p_dev, p_data, to_send + 3);

        if (len)
        {
            // copy the header for the next chunk
            memcpy(p_data + BTUSB_VOICE_BURST_SIZE, p_data , 3);
            p_data += BTUSB_VOICE_BURST_SIZE;
        }
    }

    // we should always have at least 30 ms of audio ready to send
    // if not fill up with silence
    if (p_dev->stats.voice_tx_cnt<10)
    {
        BTUSB_DBG("less than 10 buffer in the Tx Q %lu insert silence \n", p_dev->stats.voice_tx_cnt);
        while(p_dev->stats.voice_tx_cnt < 30)
        {
            memset(p_data + 3, 0, BTUSB_VOICE_BURST_SIZE );
            btusb_submit_write_voice(p_dev, p_data, BTUSB_VOICE_BURST_SIZE + 3);
        }
    }
}

/*******************************************************************************
 **
 ** Function         btusb_submit_write_voice
 **
 ** Description      Voice write submission, length should be
 **                  smaller than or equal to VOICE_SAMPLE_SIZE
 **
 ** Parameters
 **
 ** Returns          A zero value upon success.
 **
 *******************************************************************************/
int btusb_submit_write_voice(tBTUSB_CB *p_dev, void *p_packet, unsigned long length)
{
    tBTUSB_ISO_ELEMENT *pIsoXfer;
    struct urb *p_urb;
    int status, pipe;
    int packet_size, cur_frame_number;

    if (!p_dev->p_main_intf || !p_dev->p_voice_intf || !p_dev->p_voice_out)
    {
        BTUSB_DBG(" failing (p_dev removed or no voice intf)\n");
        return -1;
    }

    packet_size = le16_to_cpu(p_dev->p_voice_out->desc.wMaxPacketSize);
    if ((length > BTUSB_MAXIMUM_TX_VOICE_SIZE) || !packet_size)
    {
        BTUSB_ERR("failing (%lu bytes - too many for %d packet size)\n", length, packet_size);
        p_dev->stats.voicetx_disc_toolong++;
        return -11;
    }

    // advance to the next voice tx IRP
    if (++p_dev->voicetxIrpIndex == BTUSB_NUM_OF_VOICE_TX_BUFFERS)
        p_dev->voicetxIrpIndex = 0;

    // keep track on the number of tx buffer for stats debug and optimization
    p_dev->stats.voice_tx_cnt++;
    if (p_dev->stats.voice_tx_cnt>p_dev->stats.voice_max_tx_cnt)
    {
        p_dev->stats.voice_max_tx_cnt = p_dev->stats.voice_tx_cnt;
    }

    pIsoXfer = &p_dev->p_voicetxIrpList[p_dev->voicetxIrpIndex];

    // buffer available?
    if (pIsoXfer->used)
    {
        BTUSB_ERR("failed, no buf!\n");
        p_dev->stats.voicetx_disc_nobuf++;
        return -111;
    }

    p_urb = &pIsoXfer->urb;
    pIsoXfer->index = p_dev->voicetxIrpIndex;
    pIsoXfer->dev = p_dev;
    memcpy(pIsoXfer->packet, p_packet, length);
    pipe = usb_sndisocpipe(p_dev->p_udev, p_dev->p_voice_out->desc.bEndpointAddress);
    p_urb->complete = btusb_voicetx_complete;
    p_urb->context = pIsoXfer;
    p_urb->dev = p_dev->p_udev;
    p_urb->pipe = pipe;
    p_urb->interval = p_dev->p_voice_out->desc.bInterval;
    p_urb->transfer_buffer = pIsoXfer->packet;
    p_urb->transfer_buffer_length = length;
    btusb_fill_iso_pkts(p_urb, length, packet_size);
    // p_urb->transfer_flags = 0;
    p_urb->transfer_flags = URB_ISO_ASAP;
    cur_frame_number = usb_get_current_frame_number(p_dev->p_udev);

    BTUSB_INFO("len %lu idx %lu frm %u\n", length, pIsoXfer->index, cur_frame_number);
    pIsoXfer->used = 1;
    status = usb_submit_urb(p_urb, GFP_ATOMIC);
    if (status)
    {
        BTUSB_ERR("usb_submit_urb failed %d\n", status);
        p_dev->stats.voicetx_submitted_error++;
        pIsoXfer->used = 0;
    }
    else
    {
        p_dev->stats.voicetx_submitted++;
        BTUSB_DBG("success\n");
    }

    return(status);
}

/*******************************************************************************
 **
 ** Function         btusb_set_voice
 **
 ** Description      Change voice interface setting processor
 **                  NOTE: Must be called at low execution level
 **
 ** Parameters       p_dev: pointer to the device control block
 **                  setting_number: new voice interface setting number
 **                  submit_urb: true if the URBs must be submitted
 **
 ** Returns          0 upon success, error code otherwise
 **
 *******************************************************************************/
static int btusb_set_voice(tBTUSB_CB *p_dev, unsigned char setting_number,
        bool submit_urb)
{
    int idx;
    struct usb_host_interface *p_host_intf;
    struct usb_endpoint_descriptor *p_ep_desc;

    BTUSB_DBG("setting_number=%d submit_urb=%u\n", setting_number, submit_urb);

    // cancel all voice requests before switching buffers
    p_dev->p_voice_in = NULL;
    p_dev->p_voice_out = NULL;
    btusb_cancel_voice(p_dev);

    // configure the voice interface to the proper setting
    if (usb_set_interface(p_dev->p_udev, 1, setting_number))
    {
        BTUSB_ERR("failed to set iso intf to %u\n", setting_number);
        return -EFAULT;
    }

    // find the endpoints
    p_host_intf = p_dev->p_voice_intf->cur_altsetting;

    for (idx = 0; idx < p_host_intf->desc.bNumEndpoints; idx++)
    {
        p_ep_desc = &p_host_intf->endpoint[idx].desc;
        if (usb_endpoint_type(p_ep_desc) == USB_ENDPOINT_XFER_ISOC)
        {
            if (usb_endpoint_dir_in(p_ep_desc))
            {
                p_dev->p_voice_in = &p_host_intf->endpoint[idx];
            }
            else
            {
                p_dev->p_voice_out = &p_host_intf->endpoint[idx];
            }
        }
    }

    if (!p_dev->p_voice_in || !p_dev->p_voice_out)
    {
        BTUSB_ERR("no iso pipes found!\n");
        return -EFAULT;
    }

    if (submit_urb)
    {
        btusb_update_voice_rx(p_dev);
    }

    return 0;
}

/*******************************************************************************
 **
 ** Function         btusb_add_voice_channel
 **
 ** Description      Add a voice channel in the list of current channels
 **
 ** Parameters       p_dev: pointer to the device control structure
 **                  sco_handle: handle of the synchronous connection carrying voice
 **                  burst: size of the voice bursts
 **
 ** Returns          Return 0 upon success, error code otherwise
 **
 *******************************************************************************/
static int btusb_add_voice_channel(tBTUSB_CB *p_dev, unsigned short sco_handle, unsigned char burst)
{
    int idx;
    tBTUSB_VOICE_CHANNEL *p_chan;

    if (!p_dev->p_voice_intf)
    {
        BTUSB_ERR("No voice interface detected\n");
        return -EOPNOTSUPP;
    }

    // kludge to support the backward compatibility with older BTKRNLs
    // not supplying the packet size...
    if (burst == 0)
    {
        BTUSB_INFO("fixing legacy req to 48\n");
        burst = BTUSB_VOICE_BURST_SIZE;
    }

    // look for an available voice channel entry
    for (idx = 0; idx < ARRAY_SIZE(p_dev->voice_channels); idx++)
    {
        p_chan = &p_dev->voice_channels[idx];
        if (!p_chan->used)
        {
            p_chan->info.sco_handle = sco_handle;
            p_chan->info.burst = burst;
            p_chan->used = 1;
            goto found;
        }
    }
    BTUSB_ERR("Could not find empty slot in internal tables\n");
    return -EMLINK;

found:
    if (btusb_update_voice_channels(p_dev))
    {
        BTUSB_ERR("Failed adding voice channel\n");
        // failure, remove the channel just added
        btusb_remove_voice_channel(p_dev, sco_handle);
        return -ENOMEM;
    }
    return 0;
}

/*******************************************************************************
 **
 ** Function         btusb_remove_voice_channel
 **
 ** Description      Remove a voice channel from the list of current channels
 **
 ** Parameters       p_dev: pointer to the device control structure
 **                  sco_handle: handle of the synchronous connection carrying voice
 **
 ** Returns          Return 0 upon success, error code otherwise
 **
 *******************************************************************************/
static int btusb_remove_voice_channel(tBTUSB_CB *p_dev, unsigned short sco_handle)
{
    int idx;
    tBTUSB_VOICE_CHANNEL *p_chan;

    if (!p_dev->p_voice_intf)
    {
        BTUSB_ERR("No voice interface detected\n");
        return -EOPNOTSUPP;
    }

    // find the channel to be removed
    for (idx = 0; idx < ARRAY_SIZE(p_dev->voice_channels); idx++)
    {
        p_chan = &p_dev->voice_channels[idx];
        if (p_chan->used && (p_chan->info.sco_handle == sco_handle))
        {
            p_chan->used = 0;
            goto found;
        }
    }
    BTUSB_ERR("Could not find SCO handle in internal tables\n");
    return -ENOENT;

found:
    return btusb_update_voice_channels(p_dev);
}

/*******************************************************************************
 **
 ** Function         btusb_update_voice_channels
 **
 ** Description      Voice channels just updated, reconfigure
 **
 ** Parameters       p_dev: pointer to the device control structure
 **
 ** Returns          Return 0 upon success, error code otherwise
 **
 *******************************************************************************/
static int btusb_update_voice_channels(tBTUSB_CB *p_dev)
{
    int idx, jdx;
    unsigned char min_burst, max_burst, num_voice_chan, voice_setting;
    unsigned short desired_packet_size, packet_size;
    tBTUSB_VOICE_CHANNEL *p_chan;
    struct usb_host_interface *p_host_intf;
    struct usb_endpoint_descriptor *p_ep_desc;

    BTUSB_DBG("\n");

    // get the number of voice channels and the size information
    num_voice_chan = 0;
    min_burst = 0xFF;
    max_burst = 0;
    for (idx = 0; idx < ARRAY_SIZE(p_dev->voice_channels); idx++)
    {
        p_chan = &p_dev->voice_channels[idx];
        if (p_chan->used)
        {
            num_voice_chan++;
            min_burst = min(min_burst, p_chan->info.burst);
            max_burst = max(max_burst, p_chan->info.burst);
        }
    }

    BTUSB_DBG("num_voice_chan=%d\n", num_voice_chan);
    // now calculate a desired_packet_size
    switch (num_voice_chan)
    {
    case 0:
        desired_packet_size = 0;
        break;

    case 1:
        // single channel: we just need a third of the length (rounded up so we add 2 before dividing)
        desired_packet_size = ((max_burst + BTUSB_VOICE_HEADER_SIZE) + 2) / 3;
        break;

    case 2:
        // two channels: we need the smaller one to fit in completely
        // and the larger one to fit in into two...
        packet_size = (max_burst + BTUSB_VOICE_HEADER_SIZE + 1) / 2;

        desired_packet_size = min_burst + BTUSB_VOICE_HEADER_SIZE;
        if (packet_size > desired_packet_size)
            desired_packet_size = packet_size;
        break;

    case 3:
        // three channels - we need all of them to fit into a single packet
        desired_packet_size = max_burst + BTUSB_VOICE_HEADER_SIZE;
        break;

    default:
        // this can not happen
        BTUSB_ERR("invalid # (%d) of channels, failing...\n", num_voice_chan);
        return 0;
    }

    BTUSB_DBG("desired packet size is %u\n", desired_packet_size);

    // now convert the desired_packet_size into the interface setting number
    packet_size = BTUSB_USHRT_MAX;
    voice_setting = 0;
    for (idx = 0; idx < p_dev->p_voice_intf->num_altsetting; idx++)
    {
        p_host_intf = &p_dev->p_voice_intf->altsetting[idx];
        for (jdx = 0; jdx < p_host_intf->desc.bNumEndpoints; jdx++)
        {
            p_ep_desc = &p_host_intf->endpoint[jdx].desc;
            if ((usb_endpoint_type(p_ep_desc) == USB_ENDPOINT_XFER_ISOC) &&
                usb_endpoint_dir_in(p_ep_desc))
            {
                // if the MaxPacketSize is large enough and if it is smaller
                // than the current setting
                if ((desired_packet_size <= le16_to_cpu(p_ep_desc->wMaxPacketSize)) &&
                    (le16_to_cpu(p_ep_desc->wMaxPacketSize) < packet_size))
                {
                    packet_size = le16_to_cpu(p_ep_desc->wMaxPacketSize);
                    voice_setting = p_host_intf->desc.bAlternateSetting;
                }
            }
        }
    }
    if (packet_size == BTUSB_USHRT_MAX)
    {
        BTUSB_ERR("no appropriate ISO interface setting found, failing...\n");
        return -ERANGE;
    }

    BTUSB_DBG("desired_packet_size=%d voice_setting=%d\n", desired_packet_size, voice_setting);
    // store the desired packet size
    p_dev->desired_packet_size = desired_packet_size;

    // set the voice setting and only submit the URBs if there is a channel
    return btusb_set_voice(p_dev, voice_setting, num_voice_chan != 0);
}

/*******************************************************************************
 **
 ** Function         btusb_debug_show
 **
 ** Description      Function called when reading the /proc file created by our driver
 **
 *******************************************************************************/
#define BTUSB_STATS(__n) \
    seq_printf(s, "  - " #__n " = %lu\n", p_dev->stats.__n)

static int btusb_debug_show(struct seq_file *s, void *unused)
{
    tBTUSB_CB *p_dev = s->private;
    struct usb_device *udev = p_dev->p_udev;
    struct usb_host_interface *p_host_intf;
    struct usb_endpoint_descriptor *p_ep_desc;
    tBTUSB_VOICE_CHANNEL *p_chan;
    int idx, jdx;

    seq_printf(s, "USB device :\n");
    seq_printf(s, "  - Match info:\n");
    if (p_dev->p_id->match_flags & USB_DEVICE_ID_MATCH_VENDOR)
        seq_printf(s, "    * USB_DEVICE_ID_MATCH_VENDOR (0x%02X)\n", p_dev->p_id->idVendor);
    if (p_dev->p_id->match_flags & USB_DEVICE_ID_MATCH_PRODUCT)
        seq_printf(s, "    * USB_DEVICE_ID_MATCH_PRODUCT 0x%02X)\n", p_dev->p_id->idProduct);
    if (p_dev->p_id->match_flags & USB_DEVICE_ID_MATCH_DEV_LO)
        seq_printf(s, "    * USB_DEVICE_ID_MATCH_DEV_LO (%u)\n", p_dev->p_id->bcdDevice_lo);
    if (p_dev->p_id->match_flags & USB_DEVICE_ID_MATCH_DEV_HI)
        seq_printf(s, "    * USB_DEVICE_ID_MATCH_DEV_HI (%u)\n", p_dev->p_id->bcdDevice_hi);
    if (p_dev->p_id->match_flags & USB_DEVICE_ID_MATCH_DEV_CLASS)
        seq_printf(s, "    * USB_DEVICE_ID_MATCH_DEV_CLASS (0x%02X)\n", p_dev->p_id->bDeviceClass);
    if (p_dev->p_id->match_flags & USB_DEVICE_ID_MATCH_DEV_SUBCLASS)
        seq_printf(s, "    * USB_DEVICE_ID_MATCH_DEV_SUBCLASS (0x%02X)\n", p_dev->p_id->bDeviceSubClass);
    if (p_dev->p_id->match_flags & USB_DEVICE_ID_MATCH_DEV_PROTOCOL)
        seq_printf(s, "    * USB_DEVICE_ID_MATCH_DEV_PROTOCOL (0x%02X)\n", p_dev->p_id->bDeviceProtocol);
    if (p_dev->p_id->match_flags & USB_DEVICE_ID_MATCH_INT_CLASS)
        seq_printf(s, "    * USB_DEVICE_ID_MATCH_INT_CLASS (0x%02X)\n", p_dev->p_id->bInterfaceClass);
    if (p_dev->p_id->match_flags & USB_DEVICE_ID_MATCH_INT_SUBCLASS)
        seq_printf(s, "    * USB_DEVICE_ID_MATCH_INT_SUBCLASS (0x%02X)\n", p_dev->p_id->bInterfaceSubClass);
    if (p_dev->p_id->match_flags & USB_DEVICE_ID_MATCH_INT_PROTOCOL)
        seq_printf(s, "    * USB_DEVICE_ID_MATCH_INT_PROTOCOL (0x%02X)\n", p_dev->p_id->bInterfaceProtocol);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 6, 0)
    if (p_dev->p_id->match_flags & USB_DEVICE_ID_MATCH_INT_NUMBER)
        seq_printf(s, "    * USB_DEVICE_ID_MATCH_INT_NUMBER (%u)\n", p_dev->p_id->bInterfaceNumber);
#endif

    seq_printf(s, "  - Address = %d\n", udev->devnum);
    seq_printf(s, "  - VendorId = %04x\n", le16_to_cpu(udev->descriptor.idVendor));
    seq_printf(s, "  - ProductId = %04x\n", le16_to_cpu(udev->descriptor.idProduct));
    seq_printf(s, "  - Manufacturer String = %s\n", udev->manufacturer);
    seq_printf(s, "  - Product String = %s\n", udev->product);
    seq_printf(s, "  - USB bus number = %d\n", udev->bus->busnum);
    seq_printf(s, "  - USB devpath = %s\n", udev->devpath);
    seq_printf(s, "  - USB devnum = %d\n", udev->devnum);
    seq_printf(s, "  - USB ttport = %d\n", udev->ttport);
    seq_printf(s, "  - Interfaces :\n");
    seq_printf(s, "    * MAIN : ");
    if (p_dev->p_main_intf)
    {
        seq_printf(s, "intf = %d (nb alt settings = %d, ",
                p_dev->p_main_intf->cur_altsetting->desc.bInterfaceNumber,
                p_dev->p_main_intf->num_altsetting);
        seq_printf(s, "cur alt setting = %d)\n", p_dev->p_main_intf->cur_altsetting->desc.bAlternateSetting);
        seq_printf(s, "      * HCI EVENT : ");
        if (p_dev->p_event_in)
        {
            seq_printf(s, "ep = 0x%02x\n", p_dev->p_event_in->desc.bEndpointAddress);
        }
        else
        {
            seq_printf(s, "ERROR (endpoint not found)\n");
        }
        seq_printf(s, "      * ACL RX : ");
        if (p_dev->p_acl_in)
        {
            seq_printf(s, "ep = 0x%02x\n", p_dev->p_event_in->desc.bEndpointAddress);
        }
        else
        {
            seq_printf(s, "ERROR (endpoint not found)\n");
        }
        seq_printf(s, "      * ACL TX : ");
        if (p_dev->p_acl_out)
        {
            seq_printf(s, "ep = 0x%02x\n", p_dev->p_acl_out->desc.bEndpointAddress);
        }
        else
        {
            seq_printf(s, "ERROR (endpoint not found)\n");
        }
    }
    else
    {

    }
    seq_printf(s, "    * VOICE :");
    if (p_dev->p_voice_intf)
    {
        seq_printf(s, " intf = %d (nb alt setting = %d, ", p_dev->p_voice_intf->cur_altsetting->desc.bInterfaceNumber, p_dev->p_voice_intf->num_altsetting);
        seq_printf(s, "cur alt setting = %d)\n", p_dev->p_voice_intf->cur_altsetting->desc.bAlternateSetting);
        for (idx = 0; idx < p_dev->p_voice_intf->num_altsetting; idx++)
        {
            p_host_intf = &p_dev->p_voice_intf->altsetting[idx];
            seq_printf(s, "      * alt setting %d (idx %d) : %d enpoints\n",
                    p_host_intf->desc.bAlternateSetting, idx, p_host_intf->desc.bNumEndpoints);
            for (jdx = 0; jdx < p_host_intf->desc.bNumEndpoints; jdx++)
            {
                p_ep_desc = &p_host_intf->endpoint[jdx].desc;
                seq_printf(s, "        *  ep = 0x%02x : ", p_ep_desc->bEndpointAddress);
                if (usb_endpoint_type(p_ep_desc) == USB_ENDPOINT_XFER_ISOC)
                {
                    seq_printf(s, "Isoch ");
                    if (usb_endpoint_dir_out(p_ep_desc))
                    {
                        seq_printf(s, "(OUT) ");
                    }
                    else
                    {
                        seq_printf(s, "(IN)  ");
                    }
                    seq_printf(s, "wMaxPacketSize = %d\n", le16_to_cpu(p_ep_desc->wMaxPacketSize));
                }
                else
                {
                    seq_printf(s, "not isochronous endpoint\n");
                }
            }
        }
    }
    else
    {
        seq_printf(s, "Not present\n");
    }
    seq_printf(s, "    * DIAG RX : ");
    if (p_dev->p_diag_in)
    {
        seq_printf(s, "intf = %d ep = 0x%02x\n", p_dev->p_diag_intf->cur_altsetting->desc.bInterfaceNumber, p_dev->p_diag_in->desc.bEndpointAddress);
    }
    else
    {
        seq_printf(s, "Not present\n");
    }
    seq_printf(s, "    * DIAG TX : ");
    if (p_dev->p_diag_out)
    {
        seq_printf(s, "intf = %d ep = 0x%02x\n", p_dev->p_diag_intf->cur_altsetting->desc.bInterfaceNumber, p_dev->p_diag_out->desc.bEndpointAddress);
    }
    else
    {
        seq_printf(s, "Not present\n");
    }
    seq_printf(s, "    * DFU : ");
    if (p_dev->p_dfu_intf)
    {
        seq_printf(s, "intf = %d\n", p_dev->p_dfu_intf->cur_altsetting->desc.bInterfaceNumber);
    }
    else
    {
        seq_printf(s, "Not present\n");
    }

    seq_printf(s, "Memory usage :\n");
    seq_printf(s, "  - p_dev = %p\n", p_dev);
    seq_printf(s, "  - size = %zd\n", sizeof(*p_dev));
    seq_printf(s, "    * CMD = off:%zd/size=%zd\n", offsetof(tBTUSB_CB, cmd_array), sizeof(p_dev->cmd_array));
    seq_printf(s, "    * EVENT = off:%zd/size=%zd\n", offsetof(tBTUSB_CB, event_array), sizeof(p_dev->event_array));
    seq_printf(s, "    * ACL RX = off:%zd/size=%zd\n", offsetof(tBTUSB_CB, acl_rx_array), sizeof(p_dev->acl_rx_array));
    seq_printf(s, "    * ACL TX = off:%zd/size=%zd\n", offsetof(tBTUSB_CB, acl_tx_array), sizeof(p_dev->acl_tx_array));
    seq_printf(s, "    * DIAG RX = off:%zd/size=%zd\n", offsetof(tBTUSB_CB, diag_rx_array), sizeof(p_dev->diag_rx_array));
    seq_printf(s, "    * DIAG TX = off:%zd/size=%zd\n", offsetof(tBTUSB_CB, diag_tx_array), sizeof(p_dev->diag_tx_array));
    seq_printf(s, "    * VOICE = off:%zd/size=%zd\n", offsetof(tBTUSB_CB, voice_channels), offsetof(tBTUSB_CB, rx_queue)-offsetof(tBTUSB_CB, voice_channels));

    seq_printf(s, "Status :\n");
    seq_printf(s, "  - issharedusb = %u\n", p_dev->issharedusb);
    seq_printf(s, "  - quirks = %u\n", p_dev->quirks);
    seq_printf(s, "  - opened = %d\n", p_dev->opened);
    seq_printf(s, "Voice :\n");
    // calculate number of voice sample per timer expiration
    // HZ could  be 100, 250 or 1000
    if ((1000 / HZ) <= BTUSB_VOICE_DELAY)
    {
        seq_printf(s, "  - number of voice samples per timer expiration = 1\n");
    }
    else
    {
        seq_printf(s, "  - number of voice samples per timer expiration = %d\n",
                (1000 / (HZ * BTUSB_VOICE_DELAY)));
    }
    jdx = 0;
    for (idx = 0; idx < ARRAY_SIZE(p_dev->voice_channels); idx++)
    {
        p_chan = &p_dev->voice_channels[idx];
        if (p_chan->used)
        {
            seq_printf(s, "  - channel %d : SCO handle = %d(0x%02x) burst = %d\n", idx,
                    p_chan->info.sco_handle, p_chan->info.sco_handle, p_chan->info.burst);
            jdx = 1;
        }
    }
    if (jdx)
    {
        seq_printf(s, "  - desired_packet_size = %d\n", p_dev->desired_packet_size);
    }
    else
    {
        seq_printf(s, "  - No active channels\n");
    }
    seq_printf(s, "Statistics :\n");
    BTUSB_STATS(urb_submit_ok);
    BTUSB_STATS(urb_submit_err);
    BTUSB_STATS(acl_rx_submit_ok);
    BTUSB_STATS(acl_rx_submit_err);
    BTUSB_STATS(acl_rx_completed);
    BTUSB_STATS(acl_rx_resubmit);
    BTUSB_STATS(acl_rx_bytes);
    BTUSB_STATS(event_submit_ok);
    BTUSB_STATS(event_submit_err);
    BTUSB_STATS(event_completed);
    BTUSB_STATS(event_resubmit);
    BTUSB_STATS(event_bytes);
    BTUSB_STATS(diag_rx_submit_ok);
    BTUSB_STATS(diag_rx_submit_err);
    BTUSB_STATS(diag_rx_completed);
    BTUSB_STATS(diag_rx_resubmit);
    BTUSB_STATS(diag_rx_bytes);
    BTUSB_STATS(diag_rx_bytes);
    BTUSB_STATS(writes_submitted);
    BTUSB_STATS(writes_submitted_error);
    BTUSB_STATS(writes_completed);
    BTUSB_STATS(writes_completed_error);
    BTUSB_STATS(commands_submitted);
    BTUSB_STATS(commands_submitted_error);
    BTUSB_STATS(commands_completed);
    BTUSB_STATS(commands_completed_error);
    BTUSB_STATS(voicerx_submitted);
    BTUSB_STATS(voicerx_submitted_error);
    BTUSB_STATS(voicerx_completed);
    BTUSB_STATS(voicerx_completed_error);
    BTUSB_STATS(voicerx_bad_packets);
    BTUSB_STATS(voicerx_disc_nohdr);
    BTUSB_STATS(voicerx_split_hdr);
    BTUSB_STATS(voicerx_raw_bytes);
    BTUSB_STATS(voicerx_skipped_bytes);
    BTUSB_STATS(voicetx_submitted);
    BTUSB_STATS(voicetx_submitted_error);
    BTUSB_STATS(voicetx_completed);
    BTUSB_STATS(voicetx_completed_error);
    BTUSB_STATS(voicetx_disc_nobuf);
    BTUSB_STATS(voicetx_disc_toolong);
    BTUSB_STATS(voice_tx_cnt);
    BTUSB_STATS(voice_max_tx_cnt);

    {
        struct btusb_scosniff *p, *n;
        seq_printf(s, "SCO sniffing:\n");
        seq_printf(s, "  - list: %p\n", &p_dev->scosniff_list);
        seq_printf(s, "  - list.next: %p\n", p_dev->scosniff_list.next);
        seq_printf(s, "  - list.prev: %p\n", p_dev->scosniff_list.prev);
        seq_printf(s, "  - whole list:\n");
        list_for_each_entry_safe(p, n, &p_dev->scosniff_list, lh)
        {
            seq_printf(s, "    >  %p\n", p);
        }
    }

    return 0;
}

/*******************************************************************************
 **
 ** Function         btusb_debug_write
 **
 ** Description      Write handler of the debug /proc interface
 **
 *******************************************************************************/
ssize_t btusb_debug_write(struct file *file, const char *buf,
        size_t count, loff_t *pos)
{
    struct seq_file *s = file->private_data;
    tBTUSB_CB *p_dev = s->private;
    unsigned char cmd;

    // copy the first byte from the data written
    if (copy_from_user(&cmd, buf, 1))
    {
        return -EFAULT;
    }

    // unconditional print on purpose
    BTUSB_INFO("'%c'\n", cmd);

    switch (cmd)
    {
    case '0':
        // reset the stats
        memset(&p_dev->stats, 0, sizeof(p_dev->stats));
        break;
    case '1':
        btusb_add_voice_channel(p_dev, 6, BTUSB_VOICE_BURST_SIZE);
        break;

    case '2':
        btusb_remove_voice_channel(p_dev, 6);
        break;

    case '3':
        if (p_dev->scosniff_active)
        {
            struct btusb_scosniff *bs;
            bs = kmalloc(sizeof(*bs), GFP_ATOMIC);
            if (bs)
            {

                BTUSB_INFO("SCOSNIFF: adding %p\n", bs);
                bs->n = 0;
                list_add_tail(&bs->lh, &p_dev->scosniff_list);
                complete(&p_dev->scosniff_completion);
            }
        }
        break;

#ifdef BTUSB_LITE
    case '4':
        BTUSB_INFO("Mute PCM0\n");
        pcm0_mute = 1;
        break;

    case '5':
        BTUSB_INFO("Unmute PCM0\n");
        pcm0_mute = 0;
        break;
#endif

    default:
        break;
    }

    return count;
}

/*******************************************************************************
 **
 ** Function         btusb_debug_open
 **
 ** Description      Open handler of the debug /proc interface
 **
 *******************************************************************************/
int btusb_debug_open(struct inode *inode, struct file *file)
{
    return single_open(file, btusb_debug_show, BTUSB_PDE_DATA(inode));
}

static void * btusb_scosniff_start(struct seq_file *s, loff_t *pos)
{
    tBTUSB_CB *p_dev = s->private;
    struct btusb_scosniff *bs;
    int rv;

    BTUSB_INFO("waiting %p\n", p_dev);
    rv = wait_for_completion_interruptible(&p_dev->scosniff_completion);
    if (rv < 0)
        return NULL;

    BTUSB_INFO("triggered\n");

    if (!list_empty(&p_dev->scosniff_list))
    {
        bs = list_first_entry(&p_dev->scosniff_list, struct btusb_scosniff, lh);

        // remove the element from the list
        list_del(&bs->lh);
        BTUSB_INFO("receiving %p\n", bs);

        return bs;
    }
    return NULL;
}

static void btusb_scosniff_stop(struct seq_file *s, void *v)
{
    BTUSB_INFO("stop\n");
}

static void * btusb_scosniff_next(struct seq_file *s, void *v, loff_t *pos)
{
    struct btusb_scosniff *bs = v;
    BTUSB_INFO("next\n");

    kfree(bs);

    (*pos)++;

    /* if you do not want to buffer the data, just return NULL, otherwise, call start
     * again in order to use as much of the allocated PAGE in seq_read
     *
     * optional:
     *     return btusb_scosniff_start(s, pos);
     */
    return NULL;
}

 static int btusb_scosniff_show(struct seq_file *s, void *v)
{
    struct btusb_scosniff *bs = v;
    unsigned int i, j;
    unsigned char *p_buf, *p_c;
    unsigned char c;
    struct usb_iso_packet_descriptor *p_uipd;
    const char hexdigit[16] = {'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};

    seq_printf(s, "%u %u %d \n", bs->n, bs->l, bs->s);
    p_buf = (unsigned char *)&bs->d[bs->n];
    for (i = 0; i < bs->n; i++)
    {
        p_uipd = &bs->d[i];
        seq_printf(s, "  %d %u %u %u\n", p_uipd->status, p_uipd->actual_length, p_uipd->length, p_uipd->offset);
        for (j = 0, p_c = &p_buf[p_uipd->offset]; j < p_uipd->actual_length; j++, p_c++)
        {
            c = *p_c;
            seq_putc(s, hexdigit[c >> 4]);
            seq_putc(s, hexdigit[c & 0xF]);
            seq_putc(s, ' ');
        }
        seq_putc(s, '\n');
    }

    return 0;
}

static const struct seq_operations btusb_scosniff_seq_ops = {
    .start = btusb_scosniff_start,
    .next  = btusb_scosniff_next,
    .stop  = btusb_scosniff_stop,
    .show  = btusb_scosniff_show,
};

int btusb_scosniff_open(struct inode *inode, struct file *file)
{
    int rv;
    tBTUSB_CB *p_dev = BTUSB_PDE_DATA(inode);

    rv = seq_open(file, &btusb_scosniff_seq_ops);
    if (!rv)
    {
        p_dev->scosniff_active = true;
        ((struct seq_file *)file->private_data)->private = p_dev;
    }
    return rv;
}

int btusb_scosniff_release(struct inode *inode, struct file *file)
{
    struct btusb_scosniff *p, *n;
    tBTUSB_CB *p_dev = BTUSB_PDE_DATA(inode);

    p_dev->scosniff_active = false;

    list_for_each_entry_safe(p, n, &p_dev->scosniff_list, lh)
    {
        list_del(&p->lh);
        kfree(p);
    }

    return seq_release(inode, file);
}
