/*
 *
 * btusb.c
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
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include "btusb.h"
#include "hcidefs.h"

#define BTUSB_VID_BRCM 0x0A5C

// forward reference
struct usb_driver btusb_driver;

// table of devices that work with this driver
static struct usb_device_id btusb_table [] =
{
#if defined(BTUSB_VID) && defined(BTUSB_PID)
    /* If a specific Vid/Pid is specified at compilation time */
    { USB_DEVICE(BTUSB_VID, BTUSB_PID) },
#else
    // all BT devices
    { .match_flags = USB_DEVICE_ID_MATCH_DEV_CLASS | USB_DEVICE_ID_MATCH_DEV_SUBCLASS |
            USB_DEVICE_ID_MATCH_DEV_PROTOCOL,
      .bDeviceClass = USB_CLASS_WIRELESS_CONTROLLER,
      // 0x01 / 0x01 = Bluetooth programming interface
      .bDeviceSubClass = 0x01,
      .bDeviceProtocol = 0x01
    },
    // all BRCM BT interfaces
    { .match_flags = USB_DEVICE_ID_MATCH_VENDOR | USB_DEVICE_ID_MATCH_INT_CLASS | USB_DEVICE_ID_MATCH_INT_SUBCLASS |
            USB_DEVICE_ID_MATCH_INT_PROTOCOL,
      .idVendor = BTUSB_VID_BRCM,
      .bInterfaceClass = USB_CLASS_WIRELESS_CONTROLLER,
      // 0x01 / 0x01 = Bluetooth programming interface
      .bInterfaceSubClass = 0x01,
      .bInterfaceProtocol = 0x01
    },
    // all BRCM vendor specific with Bluetooth programming interface
    { .match_flags = USB_DEVICE_ID_MATCH_VENDOR | USB_DEVICE_ID_MATCH_DEV_CLASS |
            USB_DEVICE_ID_MATCH_DEV_SUBCLASS | USB_DEVICE_ID_MATCH_DEV_PROTOCOL,
      .idVendor = BTUSB_VID_BRCM,
      .bDeviceClass = USB_CLASS_VENDOR_SPEC,
      // 0x01 / 0x01 = Bluetooth programming interface
      .bDeviceSubClass = 0x01,
      .bDeviceProtocol = 0x01
    },
#endif
    { } /* Terminating entry */
};

MODULE_DEVICE_TABLE(usb, btusb_table);

static const struct file_operations btusb_debug_fops =
{
    .open           = btusb_debug_open,
    .read           = seq_read,
    .write          = btusb_debug_write,
    .llseek         = seq_lseek,
    .release        = single_release,
};

static const struct file_operations btusb_scosniff_fops =
{
    .open           = btusb_scosniff_open,
    .read           = seq_read,
    .llseek         = seq_lseek,
    .release        = btusb_scosniff_release,
};

static struct file_operations btusb_fops =
{
    .owner = THIS_MODULE,
    .read = btusb_read,
    .write = btusb_write,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
    .unlocked_ioctl = btusb_ioctl,
#else
    .ioctl = btusb_ioctl,
#endif
    .poll = btusb_poll,
    .open = btusb_open,
    .release = btusb_release,
};

/*
 * usb class driver info in order to get a minor number from the usb core,
 * and to have the device registered with devfs and the driver core
 */
static struct usb_class_driver btusb_class =
{
    .name = "usb/btusb%d",
    .fops = &btusb_fops,
    .minor_base = BTUSB_MINOR_BASE,
};


// static functions
static int btusb_create(tBTUSB_CB *p_dev, struct usb_interface *p_interface, const struct usb_device_id *p_id);

// module parameter to enable suspend/resume with remote wakeup
bool autopm = 0;

// module parameter to enable debug flags
int dbgflags = BTUSB_DBGFLAGS;

/*******************************************************************************
 **
 ** Function         btusb_submit
 **
 ** Description      Submit a BTUSB transaction
 **
 ** Parameters       p_dev: driver control block
 **                  p_anchor: anchor to hook URB to when submitting
 **                  p_trans: transaction to submit
 **                  mem_flags: memory flags to submit the URB with
 **
 ** Returns          0 upon success, error core else.
 **
 *******************************************************************************/
int btusb_submit(tBTUSB_CB *p_dev, struct usb_anchor *p_anchor, tBTUSB_TRANSACTION *p_trans, int mem_flags)
{
    int status;
    struct urb *p_urb = p_trans->p_urb;

    BTUSB_DBG("urb %p b=%p\n", p_urb, p_urb->transfer_buffer);

    if (unlikely(!p_dev->p_main_intf))
    {
        return -ENODEV;
    }

    usb_anchor_urb(p_urb, p_anchor);
    status = usb_submit_urb(p_urb, mem_flags);
    if (unlikely(status))
    {
        BTUSB_ERR("usb_submit_urb failed(%d)\n", status);
        usb_unanchor_urb(p_urb);
        p_dev->stats.urb_submit_err++;
    }
    else
    {
        p_dev->stats.urb_submit_ok++;
    }

    return status;
}


/*******************************************************************************
 **
 ** Function         btusb_acl_read_complete
 **
 ** Description      ACL Bulk pipe completion routine
 **
 ** Parameters       p_urb: URB that completed
 **
 ** Returns          void
 **
 *******************************************************************************/
static void btusb_acl_read_complete(struct urb *p_urb)
{
    tBTUSB_TRANSACTION *p_trans = p_urb->context;
    tBTUSB_CB *p_dev = p_trans->context;
    int count = p_urb->actual_length;

    BTUSB_DBG("urb %p status %d count %d flags x%x\n", p_urb,
        p_urb->status, count, p_urb->transfer_flags);

    // number of ACL URB completed
    p_dev->stats.acl_rx_completed++;

    // check if device is disconnecting, has been unplugged or is closing
    if (unlikely((p_urb->status == -ESHUTDOWN) ||
        (p_urb->status == -ENOENT) ||
        (p_urb->status == -EILSEQ)))
    {
        BTUSB_DBG("not queuing URB\n");
        goto exit;
    }

    // if there was an error or no data, do not forward to upper layers
    if (unlikely(p_urb->status || !count))
    {
        p_dev->stats.acl_rx_resubmit++;
        goto resubmit;
    }

    // increment the number of bytes transferred
    p_dev->stats.acl_rx_bytes += count;

    // forward up the data
    btusb_dump_data(p_urb->transfer_buffer, count, "btusb_acl_read_complete");
    btusb_enqueue(p_dev, p_trans, HCIT_TYPE_ACL_DATA);

    // Do not resubmit since it was forwarded to the user
    goto exit;

resubmit:
    if (unlikely(btusb_submit(p_dev, &p_dev->acl_rx_submitted, p_trans, GFP_ATOMIC)))
        p_dev->stats.acl_rx_submit_err++;
    else
        p_dev->stats.acl_rx_submit_ok++;

exit:
    return;
}

/*******************************************************************************
 **
 ** Function         btusb_diag_read_complete
 **
 ** Description      Diag Bulk pipe completion routine
 **
 ** Parameters       p_urb: URB that completed
 **
 ** Returns          void
 **
 *******************************************************************************/
static void btusb_diag_read_complete(struct urb *p_urb)
{
    tBTUSB_TRANSACTION *p_trans = p_urb->context;
    tBTUSB_CB *p_dev = p_trans->context;
    int count = p_urb->actual_length;

    BTUSB_DBG("urb %p status %d count %d flags %x\n", p_urb,
        p_urb->status, count, p_urb->transfer_flags);

    p_dev->stats.diag_rx_completed++;

    // check if device is disconnecting, has been unplugged or is closing
    if (unlikely((p_urb->status == -ESHUTDOWN) ||
        (p_urb->status == -ENOENT) ||
        (p_urb->status == -EILSEQ)))
    {
        BTUSB_DBG("not queuing URB\n");
        goto exit;
    }

    // if there was an error or no data or no more submitted diags, do not forward to upper layers
    if (unlikely(p_urb->status || !count || usb_anchor_empty(&p_dev->diag_rx_submitted)))
    {
        p_dev->stats.diag_rx_resubmit++;
        goto resubmit;
    }

    // increment the number of bytes transferred
    p_dev->stats.diag_rx_bytes += count;

    // forward up the data
    btusb_dump_data(p_urb->transfer_buffer, count, "btusb_diag_read_complete");
    btusb_enqueue(p_dev, p_trans, HCIT_TYPE_LM_DIAG);

    goto exit;

resubmit:
    if (unlikely(btusb_submit(p_dev, &p_dev->diag_rx_submitted, p_trans, GFP_ATOMIC)))
        p_dev->stats.diag_rx_submit_err++;
    else
        p_dev->stats.diag_rx_submit_ok++;

exit:
    return;
}


/*******************************************************************************
 **
 ** Function         btusb_event_complete
 **
 ** Description      Interrupt pipe completion routine
 **
 ** Parameters       p_urb: URB that completed
 **
 ** Returns          void
 **
 *******************************************************************************/
static void btusb_event_complete(struct urb *p_urb)
{
    tBTUSB_TRANSACTION *p_trans = p_urb->context;
    tBTUSB_CB *p_dev = p_trans->context;
    int count = p_urb->actual_length;
#if defined (BTUSB_LITE)
    BT_HDR *p_buf;
#endif

    BTUSB_DBG("urb %p status %d count %d flags %x\n", p_urb,
        p_urb->status, count, p_urb->transfer_flags);

    p_dev->stats.event_completed++;

    // check if device is disconnecting, has been unplugged or is closing
    if (unlikely((p_urb->status == -ESHUTDOWN) ||
        (p_urb->status == -ENOENT) ||
        (p_urb->status == -EILSEQ)))
    {
        BTUSB_DBG("not queuing URB\n");
        goto exit;
    }

    // if there was an error or no data, do not forward to upper layers
    if (unlikely(p_urb->status || !count))
    {
        p_dev->stats.event_resubmit++;
        goto resubmit;
    }

#ifdef BTUSB_LITE
    /* Filter Events received from BT Controller*/
    if (btusb_lite_hci_event_filter(p_dev, p_trans->dma_buffer, count) == 0)
    {
        p_buf = &p_trans->bt_hdr;
        p_buf->offset = 0;
        p_buf->event = HCIT_TYPE_EVENT;
        p_buf->len = p_trans->p_urb->actual_length;
        btusb_dequeued(p_dev, p_buf);
    }
    else
#endif
    {
        /* increment the number of bytes transferred */
        p_dev->stats.event_bytes += count;

        /* forward up the event */
        btusb_dump_data(p_urb->transfer_buffer, count, "btusb_event_complete");
        btusb_enqueue(p_dev, p_trans, HCIT_TYPE_EVENT);
    }

    /* Do not resubmit since it was forwarded to the user */
    goto exit;

resubmit:
    if (unlikely(btusb_submit(p_dev, &p_dev->event_submitted, p_trans, GFP_ATOMIC)))
        p_dev->stats.event_submit_err++;
    else
        p_dev->stats.event_submit_ok++;

exit:
    return;
}

/*******************************************************************************
 **
 ** Function         btusb_probe
 **
 ** Description      Device probe callback
 **                  assuming the following USB device configuration :
 **
 **                  NumInterfaces = 4
 **                       Interface 0 Alt settings 0
 **                         3 end point excluding end point 0
 **                            1 interrupt end point -> event
 **                            2 bulk for acl in and out
 **                       Interface 1 Alt settings 0
 **                          2 Iso end point excluding end point 0
 **                       Interface 1 Alt settings 1
 **                          2 Iso end point excluding end point 0
 **                       Interface 1 Alt settings 2
 **                          2 Iso end point excluding end point 0
 **                       Interface 1 Alt settings 3
 **                          2 Iso end point excluding end point 0
 **                       Interface 1 Alt settings 4
 **                          2 Iso end point excluding end point 0
 **                       Interface 1 Alt settings 5
 **                          2 Iso end point excluding end point 0
 **                       Interface 2 Alt settings 0
 **                          2 bulk end point for diags
 **                       Interface 3 Alt settings 0
 **                          0 end point point i.e. only the control endpoint is used
 ** Parameters
 **
 ** Returns          0 upon success, error core else.
 **
 *******************************************************************************/
static int btusb_probe(struct usb_interface *p_interface, const struct usb_device_id *p_id)
{
    tBTUSB_CB *p_dev;
    struct usb_ctrlrequest *p_dr;
    struct usb_host_interface *p_host_intf;
    struct usb_endpoint_descriptor *p_ep_desc;
    struct urb *p_urb;
    int idx, jdx;
    tBTUSB_TRANSACTION *p_trans;
    int retval = -ENOMEM;
    char procname[64];

    BTUSB_INFO("p_interface=%p, p_id=%p\n", p_interface, p_id);
    BTUSB_INFO("p_interface->cur_altsetting->desc.bInterfaceNumber=%d\n", p_interface->cur_altsetting->desc.bInterfaceNumber);
    BTUSB_DBG("match=0x%x VID=0x%x PID=0x%x class=0x%x subclass=0x%x protocol=0x%x\n",
        p_id->match_flags, p_id->idVendor, p_id->idProduct, p_id->bDeviceClass, p_id->bDeviceSubClass,
        p_id->bDeviceProtocol);

    // Bluetooth core Interface number must be 0 (hardcoded in the spec)
    if (p_interface->cur_altsetting->desc.bInterfaceNumber != 0)
    {
        BTUSB_INFO("InterfaceNumber is not 0. This is not a Bluetooth Interface\n");
        return -ENODEV;
    }

    // allocate memory for our device
    p_dev = kzalloc(sizeof(tBTUSB_CB), GFP_KERNEL);
    if (p_dev == NULL)
    {
        dev_err(&p_interface->dev, "Out of memory\n");
        goto error;
    }
    BTUSB_DBG("allocated p_dev=%p\n", p_dev);

    if (btusb_create(p_dev, p_interface, p_id))
    {
        goto error;
    }

    // set up the endpoint information
    // use only the first bulk-in and bulk-out endpoints
    p_host_intf = p_interface->cur_altsetting;
    BTUSB_DBG("%u endpoints\n", p_host_intf->desc.bNumEndpoints);

    for (idx = 0; idx < p_host_intf->desc.bNumEndpoints; ++idx)
    {
        p_ep_desc = &p_host_intf->endpoint[idx].desc;
        BTUSB_DBG("endpoint addr 0x%x attr 0x%x\n",
                p_ep_desc->bEndpointAddress, p_ep_desc->bmAttributes);
        if ((!p_dev->p_acl_in) &&
            BTUSB_EP_DIR_IN(p_ep_desc) &&
            (BTUSB_EP_TYPE(p_ep_desc) == USB_ENDPOINT_XFER_BULK))
        {
            // we found a bulk in endpoint
            p_dev->p_acl_in = &p_host_intf->endpoint[idx];
        }

        if ((!p_dev->p_event_in) &&
            BTUSB_EP_DIR_IN(p_ep_desc) &&
            (BTUSB_EP_TYPE(p_ep_desc) == USB_ENDPOINT_XFER_INT))
        {
            // we found an interrupt in end point
            p_dev->p_event_in = &p_host_intf->endpoint[idx];
        }

        if ((!p_dev->p_acl_out) &&
            BTUSB_EP_DIR_OUT(p_ep_desc) &&
            (BTUSB_EP_TYPE(p_ep_desc) == USB_ENDPOINT_XFER_BULK))
        {
            // we found a bulk out end point
            p_dev->p_acl_out = &p_host_intf->endpoint[idx];
        }
    }

    // now start looking at the voice interface (check if it a BT interface)
    p_dev->p_voice_intf = usb_ifnum_to_if(p_dev->p_udev, 1);
    if (p_dev->p_voice_intf)
    {
        p_host_intf = &p_dev->p_voice_intf->altsetting[0];
        if (((p_host_intf->desc.bInterfaceClass == USB_CLASS_WIRELESS_CONTROLLER) ||
             (p_host_intf->desc.bInterfaceClass == USB_CLASS_VENDOR_SPEC)) &&
            (p_host_intf->desc.bInterfaceSubClass == 1) &&
            (p_host_intf->desc.bInterfaceProtocol == 1))
        {
            // can claim here, but prefer to check there are ISO endpoints
        }
        else
        {
            BTUSB_INFO("Interface 1 is not VOICE\n");
            goto no_voice;
        }
        for (idx = 0; idx < p_dev->p_voice_intf->num_altsetting; idx++)
        {
            p_host_intf = &p_dev->p_voice_intf->altsetting[idx];
            for (jdx = 0; jdx < p_host_intf->desc.bNumEndpoints; jdx++)
            {
                p_ep_desc = &p_host_intf->endpoint[jdx].desc;
                if (BTUSB_EP_TYPE(p_ep_desc) == USB_ENDPOINT_XFER_ISOC)
                {
                    // found an ISO endpoint, claim interface and stop here
                    if (usb_driver_claim_interface(&btusb_driver, p_dev->p_voice_intf, p_dev) != 0)
                    {
                        BTUSB_ERR("failed claiming iso interface\n");
                        // reset it to prevent releasing it
                        p_dev->p_voice_intf = NULL;
                        goto error_claim;
                    }
                    BTUSB_DBG("claimed iso interface\n");

                    // set it into a disabled state
                    if (usb_set_interface(p_dev->p_udev, 1, 0))
                    {
                        BTUSB_ERR("failed to set iso intf to 0\n");
                    }
                    goto found_voice;
                }
            }
        }
    }
no_voice:
    p_dev->p_voice_intf = NULL;
found_voice:

    // grab other interfaces if present
    p_dev->p_diag_intf = usb_ifnum_to_if(p_dev->p_udev, 2);
    if (p_dev->p_diag_intf)
    {
        p_host_intf = &p_dev->p_diag_intf->altsetting[0];
        if ((p_host_intf->desc.bInterfaceClass == USB_CLASS_VENDOR_SPEC) &&
            (p_host_intf->desc.bInterfaceSubClass == 255) &&
            (p_host_intf->desc.bInterfaceProtocol == 255))
        {
            if (usb_driver_claim_interface(&btusb_driver, p_dev->p_diag_intf, p_dev) != 0)
            {
                BTUSB_ERR("failed claiming diag interface\n");
                p_dev->p_diag_intf = NULL;
                goto no_diag;
            }
        }
        else
        {
            BTUSB_INFO("Interface 2 is not DIAG\n");
            p_dev->p_diag_intf = NULL;
            goto no_diag;
        }

        BTUSB_DBG("claimed diag interface bNumEndpoints %d\n", p_host_intf->desc.bNumEndpoints);
        for (idx = 0; idx < p_host_intf->desc.bNumEndpoints; ++idx)
        {
            p_ep_desc = &p_host_intf->endpoint[idx].desc;
            BTUSB_DBG("diag endpoint addr 0x%x attr 0x%x\n",
                    p_ep_desc->bEndpointAddress, p_ep_desc->bmAttributes);
            if ((!p_dev->p_diag_in) &&
                BTUSB_EP_DIR_IN(p_ep_desc) &&
                (BTUSB_EP_TYPE(p_ep_desc) == USB_ENDPOINT_XFER_BULK))
            {
                // we found a bulk in end point
                p_dev->p_diag_in = &p_host_intf->endpoint[idx];
            }
            if ((!p_dev->p_diag_out) &&
                BTUSB_EP_DIR_OUT(p_ep_desc) &&
                (BTUSB_EP_TYPE(p_ep_desc) == USB_ENDPOINT_XFER_BULK))
            {
                // we found a bulk out end point
                p_dev->p_diag_out = &p_host_intf->endpoint[idx];
            }
        }
    }
no_diag:

    // try to get the DFU interface
    p_dev->p_dfu_intf = usb_ifnum_to_if(p_dev->p_udev, 3);
    if (p_dev->p_dfu_intf)
    {
        p_host_intf = &p_dev->p_dfu_intf->altsetting[0];
        if ((p_host_intf->desc.bInterfaceClass == USB_CLASS_APP_SPEC) &&
            (p_host_intf->desc.bInterfaceSubClass == 1))
        {
            if (usb_driver_claim_interface(&btusb_driver, p_dev->p_dfu_intf, p_dev) != 0)
            {
                BTUSB_ERR("failed claiming dfu interface\n");
                p_dev->p_dfu_intf = NULL;
                goto no_dfu;
            }
            BTUSB_DBG("claimed DFU interface bNumEndpoints %d\n", p_dev->p_dfu_intf->cur_altsetting->desc.bNumEndpoints);
        }
        else
        {
            BTUSB_INFO("Interface 3 is not DFU\n");
            p_dev->p_dfu_intf = NULL;
            goto no_dfu;
        }
    }
no_dfu:

    BTUSB_ARRAY_FOR_EACH_TRANS(p_dev->cmd_array)
    {
        p_trans->gki_hdr.status = BUF_STATUS_FREE;
#if (GKI_ENABLE_BUF_CORRUPTION_CHECK == TRUE)
        // write the magic number to allow checking corruption
        p_trans->gki_hdr.q_id = GKI_NUM_TOTAL_BUF_POOLS;
        p_trans->magic = MAGIC_NO;
#endif
        p_trans->context = p_dev;
        p_trans->dma_buffer = BTUSB_BUFFER_ALLOC(p_dev->p_udev,
                        BTUSB_HCI_MAX_CMD_SIZE, GFP_KERNEL, &p_trans->dma);
        p_urb = p_trans->p_urb = usb_alloc_urb(0, GFP_KERNEL);
        if (!p_trans->dma_buffer || !p_urb)
        {
            BTUSB_ERR("transaction allocation failed\n");
            goto error_claim;
        }
        BTUSB_DBG("cmd_array[%d]: b=%p\n", idx, p_trans->dma_buffer);
        p_dr = &p_dev->cmd_req_array[idx];
        p_dr->bRequestType = USB_TYPE_CLASS;
        p_dr->bRequest = 0;
        p_dr->wIndex = 0;
        p_dr->wValue = 0;
        p_dr->wLength = 0;

        usb_fill_control_urb(p_urb,
                p_dev->p_udev,
                usb_sndctrlpipe(p_dev->p_udev, 0),
                (void *)p_dr,
                p_trans->dma_buffer,
                BTUSB_HCI_MAX_CMD_SIZE,
                btusb_cmd_complete,
                p_trans);
        p_urb->transfer_dma = p_trans->dma;
        p_urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;
    }

    // initialize the USB data paths
    BTUSB_ARRAY_FOR_EACH_TRANS(p_dev->event_array)
    {
        p_trans->gki_hdr.status = BUF_STATUS_UNLINKED;
#if (GKI_ENABLE_BUF_CORRUPTION_CHECK == TRUE)
        // write the magic number to allow checking corruption
        p_trans->gki_hdr.q_id = GKI_NUM_TOTAL_BUF_POOLS;
        p_trans->magic = MAGIC_NO;
#endif
        p_trans->context = p_dev;
        p_trans->dma_buffer = BTUSB_BUFFER_ALLOC(p_dev->p_udev,
                BTUSB_HCI_MAX_EVT_SIZE, GFP_KERNEL, &p_trans->dma);
        p_urb = p_trans->p_urb = usb_alloc_urb(0, GFP_KERNEL);
        if (!p_trans->dma_buffer || !p_urb)
        {
            BTUSB_ERR("transaction allocation failed\n");
            goto error_claim;
        }
        BTUSB_DBG("event_array[%d]: b=%p\n", idx, p_trans->dma_buffer);
        usb_fill_int_urb(p_urb,
                p_dev->p_udev,
                usb_rcvintpipe(p_dev->p_udev, p_dev->p_event_in->desc.bEndpointAddress),
                p_trans->dma_buffer,
                BTUSB_HCI_MAX_EVT_SIZE,
                btusb_event_complete,
                p_trans,
                p_dev->p_event_in->desc.bInterval);
        p_urb->transfer_dma = p_trans->dma;
        p_urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;
    }

    BTUSB_ARRAY_FOR_EACH_TRANS(p_dev->acl_rx_array)
    {
        p_trans->gki_hdr.status = BUF_STATUS_UNLINKED;
#if (GKI_ENABLE_BUF_CORRUPTION_CHECK == TRUE)
        // write the magic number to allow checking corruption
        p_trans->gki_hdr.q_id = GKI_NUM_TOTAL_BUF_POOLS;
        p_trans->magic = MAGIC_NO;
#endif
        p_trans->context = p_dev;
        p_trans->dma_buffer = BTUSB_BUFFER_ALLOC(p_dev->p_udev,
                        BTUSB_HCI_MAX_ACL_SIZE, GFP_KERNEL, &p_trans->dma);
        p_urb = p_trans->p_urb = usb_alloc_urb(0, GFP_KERNEL);
        if (!p_trans->dma_buffer || !p_urb)
        {
            BTUSB_ERR("transaction allocation failed\n");
            goto error_claim;
        }
        BTUSB_DBG("acl_rx_array[%d]: b=%p\n", idx, p_trans->dma_buffer);
        usb_fill_bulk_urb(p_urb,
                p_dev->p_udev,
                usb_rcvbulkpipe(p_dev->p_udev, p_dev->p_acl_in->desc.bEndpointAddress),
                p_trans->dma_buffer,
                BTUSB_HCI_MAX_ACL_SIZE,
                btusb_acl_read_complete,
                p_trans);
        p_urb->transfer_dma = p_trans->dma;
        p_urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;
    }

    BTUSB_ARRAY_FOR_EACH_TRANS(p_dev->acl_tx_array)
    {
        p_trans->gki_hdr.status = BUF_STATUS_FREE;
#if (GKI_ENABLE_BUF_CORRUPTION_CHECK == TRUE)
        // write the magic number to allow checking corruption
        p_trans->gki_hdr.q_id = GKI_NUM_TOTAL_BUF_POOLS;
        p_trans->magic = MAGIC_NO;
#endif
        p_trans->context = p_dev;
        p_trans->dma_buffer = BTUSB_BUFFER_ALLOC(p_dev->p_udev,
                        BTUSB_HCI_MAX_ACL_SIZE, GFP_KERNEL, &p_trans->dma);
        p_urb = p_trans->p_urb = usb_alloc_urb(0, GFP_KERNEL);
        if (!p_trans->dma_buffer || !p_urb)
        {
            BTUSB_ERR("transaction allocation failed\n");
            goto error_claim;
        }
        BTUSB_DBG("acl_tx_array[%d]: b=%p\n", idx, p_trans->dma_buffer);
        usb_fill_bulk_urb(p_urb,
                p_dev->p_udev,
                usb_sndbulkpipe(p_dev->p_udev, p_dev->p_acl_out->desc.bEndpointAddress),
                p_trans->dma_buffer,
                BTUSB_HCI_MAX_ACL_SIZE,
                btusb_write_complete,
                p_trans);
        p_urb->transfer_dma = p_trans->dma;
        p_urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;
        // if it is a composite device, BULK out transfers must be ZERO packet terminated
        if (p_dev->quirks & BTUSB_QUIRK_ZLP_TX_REQ)
        {
            BTUSB_DBG("acl_tx_array[%d]: add ZERO_PACKET\n", idx);
            p_urb->transfer_flags |= URB_ZERO_PACKET;
        }
    }

    if (p_dev->p_diag_in)
    {
        BTUSB_ARRAY_FOR_EACH_TRANS(p_dev->diag_rx_array)
        {
            p_trans->gki_hdr.status = BUF_STATUS_UNLINKED;
#if (GKI_ENABLE_BUF_CORRUPTION_CHECK == TRUE)
            // write the magic number to allow checking corruption
            p_trans->gki_hdr.q_id = GKI_NUM_TOTAL_BUF_POOLS;
            p_trans->magic = MAGIC_NO;
#endif
            p_trans->context = p_dev;
            p_trans->dma_buffer = BTUSB_BUFFER_ALLOC(p_dev->p_udev,
                                BTUSB_HCI_MAX_ACL_SIZE, GFP_KERNEL, &p_trans->dma);
            p_urb = p_trans->p_urb = usb_alloc_urb(0, GFP_KERNEL);
            if (!p_trans->dma_buffer || !p_urb)
            {
                BTUSB_ERR("transaction allocation failed\n");
                goto error_claim;
            }
            BTUSB_DBG("diag_rx_array[%d]: b=%p\n", idx, p_trans->dma_buffer);
            usb_fill_bulk_urb(p_urb,
                    p_dev->p_udev,
                    usb_rcvbulkpipe(p_dev->p_udev, p_dev->p_diag_in->desc.bEndpointAddress),
                    p_trans->dma_buffer,
                    BTUSB_HCI_MAX_ACL_SIZE,
                    btusb_diag_read_complete,
                    p_trans);
            p_urb->transfer_dma = p_trans->dma;
            p_urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;
        }
    }

    if (p_dev->p_diag_out)
    {
        BTUSB_ARRAY_FOR_EACH_TRANS(p_dev->diag_tx_array)
        {
            p_trans->gki_hdr.status = BUF_STATUS_FREE;
#if (GKI_ENABLE_BUF_CORRUPTION_CHECK == TRUE)
            // write the magic number to allow checking corruption
            p_trans->gki_hdr.q_id = GKI_NUM_TOTAL_BUF_POOLS;
            p_trans->magic = MAGIC_NO;
#endif
            p_trans->context = p_dev;
            p_trans->dma_buffer = BTUSB_BUFFER_ALLOC(p_dev->p_udev,
                                BTUSB_HCI_MAX_ACL_SIZE, GFP_KERNEL, &p_trans->dma);
            p_urb = p_trans->p_urb = usb_alloc_urb(0, GFP_KERNEL);
            if (!p_trans->dma_buffer || !p_urb)
            {
                BTUSB_ERR("transaction allocation failed\n");
                goto error_claim;
            }
            BTUSB_DBG("diag_tx_array[%d]: b=%p\n", idx, p_trans->dma_buffer);
            usb_fill_bulk_urb(p_urb,
                    p_dev->p_udev,
                    usb_sndbulkpipe(p_dev->p_udev, p_dev->p_diag_out->desc.bEndpointAddress),
                    p_trans->dma_buffer,
                    BTUSB_HCI_MAX_ACL_SIZE,
                    btusb_write_complete,
                    p_trans);
            p_urb->transfer_dma = p_trans->dma;
            p_urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;
        }
    }

    BTUSB_ARRAY_FOR_EACH_TRANS(p_dev->voice_rx_array)
    {
        p_trans->gki_hdr.status = BUF_STATUS_FREE;
#if (GKI_ENABLE_BUF_CORRUPTION_CHECK == TRUE)
        // write the magic number to allow checking corruption
        p_trans->gki_hdr.q_id = GKI_NUM_TOTAL_BUF_POOLS;
        p_trans->magic = MAGIC_NO;
#endif
        p_trans->context = p_dev;
        p_trans->dma_buffer = BTUSB_BUFFER_ALLOC(p_dev->p_udev,
                BTUSB_VOICE_BUFFER_MAXSIZE, GFP_KERNEL, &p_trans->dma);
        p_urb = p_trans->p_urb = usb_alloc_urb(BTUSB_VOICE_FRAMES_PER_URB, GFP_KERNEL);
        if (!p_trans->dma_buffer || !p_urb)
        {
            BTUSB_ERR("transaction allocation failed\n");
            goto error_claim;
        }
        BTUSB_DBG("voice_rx_array[%d]: b=%p\n", idx, p_trans->dma_buffer);

        p_urb->dev = p_dev->p_udev;
        p_urb->transfer_buffer = p_trans->dma_buffer;
        p_urb->transfer_buffer_length = BTUSB_VOICE_BUFFER_MAXSIZE;
        p_urb->complete = btusb_voicerx_complete;
        p_urb->context = p_trans;
        p_urb->transfer_dma = p_trans->dma;
        p_urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP | URB_ISO_ASAP;
    }

    for (idx = 0; idx < BTUSB_NUM_OF_VOICE_TX_BUFFERS; idx++)
    {
        usb_init_urb(&p_dev->p_voicetxIrpList[idx].urb);
    }

    // we can register the device now, as it is ready
    usb_set_intfdata(p_interface, p_dev);
    retval = usb_register_dev(p_interface, &btusb_class);
    if (retval)
    {
        // something prevented us from registering this driver
        BTUSB_ERR("Not able to get a minor for this device\n");
        goto error_claim;
    }

    // start sending IN tokens
    BTUSB_ARRAY_FOR_EACH_TRANS(p_dev->event_array)
    {
        if (btusb_submit(p_dev, &p_dev->event_submitted, p_trans, GFP_KERNEL))
        {
            p_dev->stats.event_submit_err++;
            BTUSB_ERR("btusb_submit(event) failed\n");
            goto error_bt_submit;
        }
        p_dev->stats.event_submit_ok++;
    }
    BTUSB_ARRAY_FOR_EACH_TRANS(p_dev->acl_rx_array)
    {
        if (btusb_submit(p_dev, &p_dev->acl_rx_submitted, p_trans, GFP_KERNEL))
        {
            p_dev->stats.acl_rx_submit_err++;
            BTUSB_ERR("btusb_submit(acl_rx) failed\n");
            goto error_bt_submit;
        }
        p_dev->stats.acl_rx_submit_ok++;
    }
    if (p_dev->p_diag_in)
    {
        BTUSB_ARRAY_FOR_EACH_TRANS(p_dev->diag_rx_array)
        {
            if (btusb_submit(p_dev, &p_dev->diag_rx_submitted, p_trans, GFP_KERNEL))
            {
                p_dev->stats.diag_rx_submit_err++;
                BTUSB_ERR("btusb_submit(diag) failed\n");
                goto error_bt_submit;
            }
            p_dev->stats.diag_rx_submit_ok++;
        }
    }

    /* Create the proc filesystem entry.  To make it easy to retrieve, the
     * entry created is called /proc/driver/btusbN where btusbN matches the
     * file entry in /dev.
     * The name of the device in /dev comes from the device created when
     * usb_register_dev was called (in the current fn).  The name was built
     * from the btusb_class structure.  usb_register_dev calls device_create
     * which creates the VFS entry.  This can be found in online linux code.
     */
    scnprintf(procname, sizeof(procname), "driver/%s", p_interface->usb_dev->kobj.name);
    p_dev->p_debug_pde = proc_create_data(procname, S_IRUGO | S_IWUGO, NULL, &btusb_debug_fops, p_dev);
    if (p_dev->p_debug_pde == NULL)
    {
        BTUSB_ERR("Couldn't create proc entry\n");
    }
    else
    {
        BTUSB_DBG("created /proc/%s\n", procname);
    }

    scnprintf(procname, sizeof(procname), "driver/%s-scosniff", p_interface->usb_dev->kobj.name);
    p_dev->p_scosniff_pde = proc_create_data(procname, S_IRUGO, NULL, &btusb_scosniff_fops, p_dev);
    if (p_dev->p_scosniff_pde == NULL)
    {
        BTUSB_ERR("Couldn't create proc SCO sniff entry\n");
    }
    else
    {
        BTUSB_DBG("created /proc/%s\n", procname);
        INIT_LIST_HEAD(&p_dev->scosniff_list);
        init_completion(&p_dev->scosniff_completion);
    }

#ifdef BTUSB_LITE
    btusb_lite_create(p_dev, p_interface);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 34)
    if(autopm)
    {
        BTUSB_INFO("Enabling for remote wakeup\n");
        p_interface->needs_remote_wakeup = 1;

        BTUSB_INFO("Setting power/wakeup to enabled\n");
        device_set_wakeup_enable(&p_dev->p_udev->dev, 1);

        BTUSB_INFO("Enabling for autosuspend\n");
        usb_enable_autosuspend(p_dev->p_udev);
    }
#else
    autopm = 0;
    BTUSB_INFO("USB suspend/resume not supported for this kernel version\n");
#endif

    // let the user know what node this device is now attached to
    BTUSB_DBG("device now attached to minor %d\n", p_interface->minor);

    return 0;

error_bt_submit:
    usb_kill_anchored_urbs(&p_dev->acl_rx_submitted);
    usb_kill_anchored_urbs(&p_dev->acl_tx_submitted);
    usb_kill_anchored_urbs(&p_dev->diag_rx_submitted);
    usb_kill_anchored_urbs(&p_dev->diag_tx_submitted);
    usb_kill_anchored_urbs(&p_dev->event_submitted);
    usb_kill_anchored_urbs(&p_dev->cmd_submitted);
    usb_kill_anchored_urbs(&p_dev->voice_rx_submitted);

    usb_deregister_dev(p_interface, &btusb_class);

error_claim:
    usb_set_intfdata(p_interface, NULL);
    if (p_dev->p_diag_intf)
    {
        usb_driver_release_interface(&btusb_driver, p_dev->p_diag_intf);
        p_dev->p_diag_intf = NULL;
        BTUSB_DBG("released diag interface\n");
    }
    if (p_dev->p_dfu_intf)
    {
        usb_driver_release_interface(&btusb_driver, p_dev->p_dfu_intf);
        p_dev->p_dfu_intf = NULL;
        BTUSB_DBG("released dfu interface\n");
    }
    if (p_dev->p_voice_intf)
    {
        usb_driver_release_interface(&btusb_driver, p_dev->p_voice_intf);
        p_dev->p_voice_intf = NULL;
        BTUSB_DBG("released iso interface\n");
    }

error:
    if (p_dev)
    {
        kref_put(&p_dev->kref, btusb_delete);
    }
    return retval;
}


/*******************************************************************************
 **
 ** Function         btusb_cancel_voice
 **
 ** Description      Cancel all pending voice requests and wait for their cancellations
 **
 ** Parameters       p_dev: device instance control block
 **
 ** Returns          void.
 **
 *******************************************************************************/
void btusb_cancel_voice(tBTUSB_CB *p_dev)
{
    int idx;
    tBTUSB_VOICE_CHANNEL *p_chan;

    BTUSB_DBG("enter\n");

    // cancel rx
    BTUSB_DBG("Removing SCO RX anchored URBs\n");
    usb_kill_anchored_urbs(&p_dev->voice_rx_submitted);

    // free any SCO HCI message being consolidated
    for (idx = 0; idx < ARRAY_SIZE(p_dev->voice_channels); idx++)
    {
        p_chan = &p_dev->voice_channels[idx];
        if (p_chan->p_msg != NULL)
        {
            GKI_freebuf(p_chan->p_msg);
            p_chan->p_msg = NULL;
        }
    }
    // clear active RX processing
    p_dev->pending_bytes = 0;
    p_dev->pp_pending_msg = NULL;
    p_dev->pending_hdr_size = 0;


    // cancel tx
    for (idx = 0; idx < BTUSB_NUM_OF_VOICE_TX_BUFFERS; idx++)
    {
        if (p_dev->p_voicetxIrpList[idx].used)
        {
            BTUSB_DBG("canceling tx URB %d\n", idx);
            usb_kill_urb(&p_dev->p_voicetxIrpList[idx].urb);
            p_dev->p_voicetxIrpList[idx].used = 0;
            BTUSB_DBG("tx URB %d canceled\n", idx);
        }
    }

}

/*******************************************************************************
 **
 ** Function         btusb_cancel_urbs
 **
 ** Description      Cancel all pending non-voice requests and wait for their cancellations
 **
 ** Parameters       pointer on usb_bt struck that contains all the btusb info
 **
 ** Returns          void.
 **
 *******************************************************************************/
void btusb_cancel_urbs(tBTUSB_CB *p_dev)
{
    BTUSB_DBG("enter\n");

    // stop reading data
    BTUSB_DBG("Removing ACL RX anchored URBs\n");
    usb_kill_anchored_urbs(&p_dev->acl_rx_submitted);

    // stop reading events
    BTUSB_DBG("Removing EVENT anchored URBs\n");
    usb_kill_anchored_urbs(&p_dev->event_submitted);

    // stop reading diags
    BTUSB_DBG("Removing DIAG anchored URBs\n");
    usb_kill_anchored_urbs(&p_dev->diag_rx_submitted);

    // cancel ACL writes
    BTUSB_DBG("Removing ACL TX anchored URBs\n");
    usb_kill_anchored_urbs(&p_dev->acl_tx_submitted);

    // cancel commands
    BTUSB_DBG("Removing CMD anchored URBs\n");
    usb_kill_anchored_urbs(&p_dev->cmd_submitted);

    // cancel diags
    BTUSB_DBG("Removing DIAG TX anchored URBs\n");
    usb_kill_anchored_urbs(&p_dev->diag_tx_submitted);

}

/*******************************************************************************
 **
 ** Function         btusb_disconnect
 **
 ** Description
 **
 ** Parameters       p_interface: first interface being disconnected
 **
 ** Returns          void.
 **
 *******************************************************************************/
static void btusb_disconnect(struct usb_interface *p_interface)
{
    tBTUSB_CB *p_dev = usb_get_intfdata(p_interface);
    int idx;
    char procname[64];

    BTUSB_INFO("p_dev=%p\n", p_dev);
    if (p_dev == NULL)
    {
        BTUSB_ERR("p_dev == NULL\n");
        return;
    }

    if (p_interface != p_dev->p_main_intf)
    {
        BTUSB_DBG("not the main interface\n");
        return;
    }

    // clear the interface data information
    usb_set_intfdata(p_interface, NULL);

    scnprintf(procname, sizeof(procname), "driver/%s", p_interface->usb_dev->kobj.name);
    remove_proc_entry(procname, NULL);

    scnprintf(procname, sizeof(procname), "driver/%s-scosniff", p_interface->usb_dev->kobj.name);
    remove_proc_entry(procname, NULL);

#ifdef BTUSB_LITE
    btusb_lite_stop_all(p_dev);
    btusb_lite_delete(p_dev, p_interface);
#endif

    BTUSB_DBG("shutting down HCI intf\n");

    /* prevent more I/O from starting */
    spin_lock_bh(&p_dev->tasklet_lock);
    p_dev->p_main_intf = NULL;
    spin_unlock_bh(&p_dev->tasklet_lock);

    // cancel pending requests
    btusb_cancel_voice(p_dev);
    btusb_cancel_urbs(p_dev);

    // give back our minor
    BTUSB_DBG("unregistering #%d\n", p_interface->minor);
    usb_deregister_dev(p_interface, &btusb_class);

    // release interfaces
    if (p_dev->p_diag_intf)
    {
        usb_driver_release_interface(&btusb_driver, p_dev->p_diag_intf);
        p_dev->p_diag_intf = NULL;
        BTUSB_DBG("released diag interface\n");
    }
    if (p_dev->p_dfu_intf)
    {
        usb_driver_release_interface(&btusb_driver, p_dev->p_dfu_intf);
        p_dev->p_dfu_intf = NULL;
        BTUSB_DBG("released dfu interface\n");
    }
    if (p_dev->p_voice_intf)
    {
        usb_driver_release_interface(&btusb_driver, p_dev->p_voice_intf);
        p_dev->p_voice_intf = NULL;
        BTUSB_DBG("released iso interface\n");
    }

    for (idx = 0; idx < BTUSB_NUM_OF_VOICE_TX_BUFFERS; idx++)
    {
        p_dev->p_voicetxIrpList[idx].used = 0;
    }

    if(autopm)
    {
        BTUSB_INFO("Disabling for remote wakeup\n");
        p_dev->p_main_intf->needs_remote_wakeup = 0;

        BTUSB_INFO("Setting power/wakeup to disabled\n");
        device_set_wakeup_enable(&p_dev->p_udev->dev, 0);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 34)
        BTUSB_INFO("Disabling autosuspend\n");
        usb_disable_autosuspend(p_dev->p_udev);
#endif
    }

    // decrement the reference counter
    BTUSB_DBG("kref_put -> &p_dev->kref 0x%p \n", &p_dev->kref);
    kref_put(&p_dev->kref, btusb_delete);
}

/*******************************************************************************
 **
 ** Function         btusb_suspend
 **
 ** Description
 **
 ** Parameters       p_interface: first interface being suspended
 **
 ** Returns          void.
 **
 *******************************************************************************/
static int btusb_suspend(struct usb_interface *p_interface, pm_message_t message)
{
    tBTUSB_CB *p_dev = usb_get_intfdata(p_interface);

    if (unlikely(!p_dev))
        return 0;

    if (p_dev->p_main_intf == p_interface)
    {
        BTUSB_INFO("Suspending USB\n");
        BTUSB_DBG("main interface (%p)\n",p_dev->p_main_intf);

        // stop reading data
        BTUSB_DBG("Removing ACL RX anchored URBs\n");
        usb_kill_anchored_urbs(&p_dev->acl_rx_submitted);

        // stop reading events
        BTUSB_DBG("Removing EVENT anchored URBs\n");
        usb_kill_anchored_urbs(&p_dev->event_submitted);

        // cancel ACL writes
        BTUSB_DBG("Removing ACL TX anchored URBs\n");
        usb_kill_anchored_urbs(&p_dev->acl_tx_submitted);

        // cancel commands
        BTUSB_DBG("Removing CMD anchored URBs\n");
        usb_kill_anchored_urbs(&p_dev->cmd_submitted);

    }
    if (p_dev->p_voice_intf == p_interface)
    {
        BTUSB_DBG("voice interface (%p)\n",p_dev->p_voice_intf);
    }
    if (p_dev->p_dfu_intf == p_interface)
    {
        BTUSB_DBG("dfu interface (%p)\n",p_dev->p_dfu_intf);
    }
    if (p_dev->p_diag_intf == p_interface)
    {
        BTUSB_DBG("diag interface (%p)\n",p_dev->p_diag_intf);

        // stop reading diags
        BTUSB_DBG("Removing DIAG RX anchored URBs\n");
        usb_kill_anchored_urbs(&p_dev->diag_rx_submitted);

        // cancel diags
        BTUSB_DBG("Removing DIAG TX anchored URBs\n");
        usb_kill_anchored_urbs(&p_dev->diag_tx_submitted);
    }

    return 0;
}

/*******************************************************************************
 **
 ** Function         btusb_resume
 **
 ** Description
 **
 ** Parameters       p_interface: first interface being resumed
 **
 ** Returns          void.
 **
 *******************************************************************************/
static int btusb_resume(struct usb_interface *p_interface)
{
    tBTUSB_CB *p_dev = usb_get_intfdata(p_interface);
    int idx;
    int retval = -ENOMEM;
    tBTUSB_TRANSACTION *p_trans;

    BTUSB_INFO("p_dev=%p\n", p_dev);
    if (unlikely(p_dev == NULL))
    {
        BTUSB_ERR("p_dev == NULL\n");
        return retval;
    }

    if (p_dev->p_main_intf == p_interface)
    {
        BTUSB_DBG("main interface (%p)\n",p_dev->p_main_intf);
        BTUSB_ARRAY_FOR_EACH_TRANS(p_dev->event_array)
        {
            if (likely(p_trans->dma_buffer != NULL))
            {
                if (unlikely(btusb_submit(p_dev, &p_dev->event_submitted, p_trans, GFP_KERNEL)))
                {
                    p_dev->stats.event_submit_err++;
                    BTUSB_ERR("btusb_submit(event) failed\n");
                    goto error_bt_submit;
                }
                p_dev->stats.event_submit_ok++;
            }
        }
        BTUSB_ARRAY_FOR_EACH_TRANS(p_dev->acl_rx_array)
        {
            if (likely(p_trans->dma_buffer != NULL))
            {
                if (unlikely(btusb_submit(p_dev, &p_dev->acl_rx_submitted, p_trans, GFP_KERNEL)))
                {
                    p_dev->stats.acl_rx_submit_err++;
                    BTUSB_ERR("btusb_submit(acl_rx) failed\n");
                    goto error_bt_submit;
                }
                p_dev->stats.acl_rx_submit_ok++;
            }
        }
    }
    if (p_dev->p_voice_intf == p_interface)
    {
        BTUSB_DBG("voice interface (%p)\n",p_dev->p_voice_intf);
    }
    if (p_dev->p_dfu_intf == p_interface)
    {
        BTUSB_DBG("dfu interface (%p)\n",p_dev->p_dfu_intf);
    }
    if (p_dev->p_diag_intf == p_interface)
    {
        BTUSB_DBG("diag interface (%p)\n",p_dev->p_diag_intf);
        BTUSB_ARRAY_FOR_EACH_TRANS(p_dev->diag_rx_array)
        {
            if (likely(p_trans->dma_buffer != NULL))
            {
                if (unlikely(btusb_submit(p_dev, &p_dev->diag_rx_submitted, p_trans, GFP_KERNEL)))
                {
                    p_dev->stats.diag_rx_submit_err++;
                    BTUSB_ERR("btusb_submit(diag) failed\n");
                    goto error_bt_submit;
                }
                p_dev->stats.diag_rx_submit_ok++;
            }
        }
    }

    return 0;

error_bt_submit:
    usb_kill_anchored_urbs(&p_dev->acl_rx_submitted);
    usb_kill_anchored_urbs(&p_dev->diag_rx_submitted);
    usb_kill_anchored_urbs(&p_dev->event_submitted);

    usb_deregister_dev(p_interface, &btusb_class);
    return retval;
}


/*******************************************************************************
 **
 ** Function         btusb_reset_resume
 **
 ** Description
 **
 ** Parameters       p_interface: first interface being resumed
 **
 ** Returns          void.
 **
 *******************************************************************************/
static int btusb_reset_resume(struct usb_interface *p_interface)
{
    BTUSB_DBG("enter\n");

    return btusb_resume(p_interface);
}


struct usb_driver btusb_driver =
{
    .name = "btusb",
    .id_table = btusb_table,
    .probe = btusb_probe,
    .disconnect = btusb_disconnect,
    .suspend = btusb_suspend,
    .resume = btusb_resume,
    .reset_resume = btusb_reset_resume,
    .supports_autosuspend = 1
};

/*******************************************************************************
 **
 ** Function         btusb_create
 **
 ** Description      Initialize the control block
 **
 ** Returns          0 upon success, error core else.
 **
 *******************************************************************************/
static int btusb_create(tBTUSB_CB *p_dev, struct usb_interface *p_interface, const struct usb_device_id *p_id)
{
    unsigned idx;
    struct usb_host_config *p_config;
    struct usb_host_interface *p_host_intf;

    // initialize the elements of the device structure
    mutex_init(&p_dev->open_mutex);
    spin_lock_init(&p_dev->tasklet_lock);
    kref_init(&p_dev->kref);
    GKI_init_q(&p_dev->tx_queue);
    GKI_init_q(&p_dev->rx_queue);
    init_waitqueue_head(&p_dev->rx_wait_q);
    init_usb_anchor(&p_dev->acl_rx_submitted);
    init_usb_anchor(&p_dev->event_submitted);
    init_usb_anchor(&p_dev->diag_rx_submitted);
    init_usb_anchor(&p_dev->acl_tx_submitted);
    init_usb_anchor(&p_dev->cmd_submitted);
    init_usb_anchor(&p_dev->diag_tx_submitted);
    init_usb_anchor(&p_dev->voice_rx_submitted);

    // save the USB information
    p_dev->p_udev = usb_get_dev(interface_to_usbdev(p_interface));
    p_dev->p_id = p_id;
    p_dev->p_main_intf = p_interface;

    // register tx task so we can start sending data to transport
    tasklet_init(&p_dev->tx_task, btusb_tx_task, (unsigned long)p_dev);
    BTUSB_DBG("tasklet_init complete\n");

    BTUSB_DBG("sizeof(tBTUSB_ISO_ELEMENT)*BTUSB_NUM_OF_VOICE_TX_BUFFERS=%zu\n",
            sizeof(tBTUSB_ISO_ELEMENT)*BTUSB_NUM_OF_VOICE_TX_BUFFERS);
    p_dev->p_voicetxIrpList = kzalloc(sizeof(tBTUSB_ISO_ELEMENT)*BTUSB_NUM_OF_VOICE_TX_BUFFERS, GFP_KERNEL);
    if (p_dev->p_voicetxIrpList == NULL)
    {
        dev_err(&p_interface->dev, "Out of memory for voice TX\n");
        return -ENOMEM;
    }

    for (idx = 0; idx < BTUSB_NUM_OF_VOICE_TX_BUFFERS; idx++)
    {
        p_dev->p_voicetxIrpList[idx].packet = kmalloc(BTUSB_MAXIMUM_TX_VOICE_SIZE, GFP_KERNEL);
        if (p_dev->p_voicetxIrpList[idx].packet == NULL)
            return -ENOMEM;
    }

    p_dev->kterm.c_line = N_HCI;
    p_dev->kterm.c_iflag = IGNPAR | ICRNL;
    p_dev->kterm.c_oflag = 0;
    p_dev->kterm.c_cflag = B115200 | CRTSCTS | CS8 | CLOCAL | CREAD;
    p_dev->kterm.c_lflag = ICANON;
    p_dev->kterm.c_ispeed = p_dev->kterm.c_ospeed = 115200;
    p_dev->kterm.c_cc[VEOF] = 4;
    p_dev->kterm.c_cc[VMIN] = 1;

    // check if there is a WiFi interface in the same device
    p_config = p_dev->p_udev->actconfig;
    for (idx = 1; idx < p_config->desc.bNumInterfaces; idx++)
    {
        p_host_intf = &p_config->interface[idx]->altsetting[0];
        if ((p_host_intf->desc.bInterfaceClass == USB_CLASS_VENDOR_SPEC) &&
            (p_host_intf->desc.bInterfaceSubClass == 2) &&
            (p_host_intf->desc.bInterfaceProtocol == 255))
        {
            p_dev->issharedusb = true;
        }
    }

    // set quirks
    if (p_dev->issharedusb)
    {
        // all shared USB devices need a ZLP to be sent in TX
        p_dev->quirks |= BTUSB_QUIRK_ZLP_TX_REQ;

        if ((le16_to_cpu(p_dev->p_udev->descriptor.idVendor) == 0x0A5C) &&
            (le16_to_cpu(p_dev->p_udev->descriptor.idProduct) == 0x0BDC))
        {
            // the 43242 has a specific bug that needs to append 1 byte because it
            // can not send ZLP
            p_dev->quirks |= BTUSB_QUIRK_ZLP_RX_WA;
        }
    }
    return 0;
}

/*******************************************************************************
 **
 ** Function         btusb_delete
 **
 ** Description      Fully remove the elements for a device
 **
 ** Parameters       kref: reference counter
 **
 ** Returns          None
 **
 *******************************************************************************/
void btusb_delete(struct kref *kref)
{
    tBTUSB_CB *p_dev = container_of(kref, tBTUSB_CB, kref);
    tBTUSB_TRANSACTION *p_trans;
    BT_HDR *p_buf = NULL;
    int idx;

    BTUSB_DBG("enter\n");

#ifdef BTUSB_LITE
    btusb_lite_stop_all(p_dev);
#endif

    // stop tx_task and then remove any data packets from tx_q
    tasklet_kill(&p_dev->tx_task);

    BTUSB_DBG("tasklet_kill complete\n");
    while ((p_buf = (BT_HDR *) GKI_dequeue(&p_dev->tx_queue)) != NULL)
    {
        GKI_freebuf(p_buf);
    }

    BTUSB_DBG("dequeued all remaining buffers from TX Q\n");

    while ((p_buf = (BT_HDR *) GKI_dequeue(&p_dev->rx_queue)) != NULL)
    {
        btusb_dequeued(p_dev, p_buf);
    }

    if (p_dev->p_write_msg)
    {
        GKI_freebuf(p_dev->p_write_msg);
    }

    BTUSB_DBG("dequeued all remaining buffers from RX Q\n");

    // ensure that the process is not hanging on select()/poll()
    wake_up_interruptible(&p_dev->rx_wait_q);

    // free allocated USB DMA space
    BTUSB_ARRAY_FOR_EACH_TRANS(p_dev->cmd_array)
    {
        BTUSB_BUFFER_FREE(p_dev->p_udev, BTUSB_HCI_MAX_CMD_SIZE,
                p_trans->dma_buffer,
                p_trans->dma);
        usb_free_urb(p_trans->p_urb);
    }
    BTUSB_ARRAY_FOR_EACH_TRANS(p_dev->event_array)
    {
        BTUSB_BUFFER_FREE(p_dev->p_udev, BTUSB_HCI_MAX_EVT_SIZE,
                p_trans->dma_buffer,
                p_trans->dma);
        usb_free_urb(p_trans->p_urb);
    }
    BTUSB_ARRAY_FOR_EACH_TRANS(p_dev->acl_rx_array)
    {
        BTUSB_BUFFER_FREE(p_dev->p_udev, BTUSB_HCI_MAX_ACL_SIZE,
                p_trans->dma_buffer,
                p_trans->dma);
        usb_free_urb(p_trans->p_urb);
    }
    BTUSB_ARRAY_FOR_EACH_TRANS(p_dev->acl_tx_array)
    {
        BTUSB_BUFFER_FREE(p_dev->p_udev, BTUSB_HCI_MAX_ACL_SIZE,
                p_trans->dma_buffer,
                p_trans->dma);
        usb_free_urb(p_trans->p_urb);
    }
    BTUSB_ARRAY_FOR_EACH_TRANS(p_dev->diag_rx_array)
    {
        BTUSB_BUFFER_FREE(p_dev->p_udev, BTUSB_HCI_MAX_ACL_SIZE,
                p_trans->dma_buffer,
                p_trans->dma);
        usb_free_urb(p_trans->p_urb);
    }
    BTUSB_ARRAY_FOR_EACH_TRANS(p_dev->diag_tx_array)
    {
        BTUSB_BUFFER_FREE(p_dev->p_udev, BTUSB_HCI_MAX_ACL_SIZE,
                p_trans->dma_buffer,
                p_trans->dma);
        usb_free_urb(p_trans->p_urb);
    }
    BTUSB_ARRAY_FOR_EACH_TRANS(p_dev->voice_rx_array)
    {
        BTUSB_BUFFER_FREE(p_dev->p_udev, BTUSB_VOICE_BUFFER_MAXSIZE,
                p_trans->dma_buffer,
                p_trans->dma);
        usb_free_urb(p_trans->p_urb);
    }
    usb_put_dev(p_dev->p_udev);

    for (idx = 0; idx < BTUSB_NUM_OF_VOICE_TX_BUFFERS; idx++)
    {
        if (p_dev->p_voicetxIrpList[idx].packet)
        {
            kfree(p_dev->p_voicetxIrpList[idx].packet);
        }
    }

    if (p_dev->p_voicetxIrpList)
    {
        kfree(p_dev->p_voicetxIrpList);
    }
    kfree(p_dev);
}


/*******************************************************************************
 **
 ** Function         btusb_init
 **
 ** Description      Module init routine
 **
 ** Parameters       None
 **
 ** Returns          None
 **
 *******************************************************************************/
static int __init btusb_init(void)
{
    int result;

    BTUSB_DBG("built %s,%s\n", __DATE__, __TIME__);

    // initialize the GKI
    GKI_init();

    // register this driver with the USB subsystem
    result = usb_register(&btusb_driver);
    if (result)
    {
        BTUSB_ERR("usb_register failed (%d)\n", result);
    }

    return result;
}

/*******************************************************************************
 **
 ** Function         btusb_exit
 **
 ** Description      Module exit routine
 **
 ** Parameters       None
 **
 ** Returns          None
 **
 *******************************************************************************/
static void __exit btusb_exit(void)
{
    BTUSB_DBG("enter\n");

    // shutdown the GKI
    GKI_shutdown();

    // deregister this driver from the USB subsystem
    usb_deregister(&btusb_driver);
}

module_init(btusb_init);
module_exit(btusb_exit);

module_param(autopm, bool, 0644);
MODULE_PARM_DESC(autopm, "Enable suspend/resume with remote wakeup");
module_param(dbgflags, int, 0644);
MODULE_PARM_DESC(dbgflags, "Debug flags");

MODULE_DESCRIPTION("Broadcom Bluetooth USB driver");
MODULE_LICENSE("GPL");

