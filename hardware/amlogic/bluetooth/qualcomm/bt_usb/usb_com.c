/*
 * Copyright (c) 2015, The Linux Foundation. All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY
 * THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/spinlock.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/unistd.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/wait.h>
#include <linux/usb.h>
#include <linux/kthread.h>
#include <asm/bitops.h>

#include "include/linux_usb_extra.h"
#include "include/linux_usb_com.h"
#include "include/linux_usb_dev.h"
#include "include/usb_com.h"

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0) & LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24))

#include <linux/usb_ch9.h>
#endif

#ifndef USB_ISO_ASAP
#define USB_ISO_ASAP URB_ISO_ASAP
#endif

/* Version Information */
#define DRIVER_VERSION          "0.5"
#define DRIVER_AUTHOR           "CSR plc."
#define DRIVER_NAME             "bt_usb"

/* isoc_alt_setting
 * This setting controls which ISOC (SCO) configuration to use.
 * We assume two streams and let the usr_config decide
 * if we use 8bit or 16bit sound. The setting (and others) can be changed
 * dynamically. The possible values are defined by the HCI specification:
 *   1 : packetsize=8,  1 * 8bit
 *   2 : packetsize=17, 1 * 16bit / 2 * 8bit
 *   3 : packetsize=25, 3 * 8bit
 *   4 : packetsize=33, 2 * 16bit
 *   5 : packetsize=49, 3 * 16bit
 */
#define DEFAULT_ISOC_ALT 4

/* Device instance */
csr_dev_t *csr_dev[BT_USB_COUNT];
struct semaphore csr_dev_sem;

/* Default supported USB devices */
struct usb_device_id csr_device_table[] =
{
    { USB_DEVICE(0x0DB0, 0xA871) },
    { USB_DEVICE(CSR_VENDOR_ID, CSR_PRODUCT_ID) },      //{ USB_DEVICE(0x0A12, 0x0001)   },
    { USB_DEVICE(QCOM_VENDOR_ID, QCOM_PRODUCT_ID) },      //QCOM ROME
    { USB_DEVICE(0x0cf3, 0xe300) },
    { USB_DEVICE(0x0cf3, 0xe500) },
    { USB_DEVICE(0x04b3, 0x3107) }, //Broadcom dongle
    { USB_DEVICE(0x0a5c, 0x21ec) }, //Broadcom dongle
    { USB_DEVICE(0x043e, 0x7a86) }, //LGE dongle
    {  }, /* END */
};
MODULE_DEVICE_TABLE(usb, csr_device_table);

#define BTUSB_ATH3012   0x80
struct usb_device_id fw_update_device_table[] =
{
    { USB_DEVICE(QCOM_VENDOR_ID, QCOM_PRODUCT_ID),.driver_info = BTUSB_ATH3012 },      //QCOM ROME
    { USB_DEVICE(0x0cf3, 0xe300),.driver_info = BTUSB_ATH3012 },
    { USB_DEVICE(0x0cf3, 0xe500),.driver_info = BTUSB_ATH3012 },
    { USB_DEVICE(0x043e, 0x7a86),.driver_info = BTUSB_ATH3012 }, //LGE device
    {  }, /* END */
};

/* Global settings with default */
csr_usb_settings_t csr_setup =
{
    .ext_iface                     = false,
    .ext_probe                     = NULL,
    .ext_disconnect                = NULL,
    .ext_start                     = NULL,
    .ext_init                      = NULL,
    .ext_exit                      = NULL,
    .max_isoc_in_frames            = 3,
    .max_isoc_out_frames           = 3,
    .isoc_alt_setting              = DEFAULT_ISOC_ALT,
    .driver_name                   = DRIVER_NAME,
    .usb_devices                   = NULL,
};


/* USB driver table */
struct usb_driver csr_usb_driver =
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,16)
    .owner                        = THIS_MODULE,
#endif
    .name                         = DRIVER_NAME,
    .id_table                     = csr_device_table,
    .probe                        = csrUsbProbe,
    .disconnect                   = csrUsbDisconnect,
#ifdef CONFIG_USB_SUSPEND
    .suspend                      = csrUsbSuspend,
    .resume                       = csrUsbResume,
    .reset_resume                 = csrUsbResetResume,
    .supports_autosuspend         = 1,
#endif
};

/* Local prototypes */
static int16_t usbRxIntr(csr_dev_t *dv);
static int16_t usbRxBulk(csr_dev_t *dv, uint8_t number);

#ifdef CSR_BR_USB_USE_SCO_INTF
static int16_t usbRxIsoc(csr_dev_t *dv, uint8_t number);
#endif

extern void pfree(void *ptr);
extern void *zpmalloc(size_t size);
extern void *pmalloc(size_t size);
extern void btusb_firmware_upgrade(struct usb_device *udev);
/*************************************************************
 * NAME:
 *      csrSlotFromUsb
 *
 * DESCRIPTION:
 *      Find a CSR BT USB device corresponding to a
 *      struct usb_device.  To find an empty slot,
 *      pass a NULL argument.
 *
 *      Grabs the csr_dev_sem lock -- caller MUST release.
 *
 * RETURNS:
 *      Slot number with csr_dev_t struct,
 *      or -1 if not found.
 *
 *************************************************************/

static int csrSlotFromUsb(struct usb_device *uDev)
{
    int slot;

    /* Grab and hold lock. */
    down(&csr_dev_sem);

    for (slot = 0; slot < BT_USB_COUNT; slot++)
    {
        if (uDev != NULL)
        {
            if (csr_dev[slot] && csr_dev[slot]->dev == uDev)
            {
                return slot;
            }
        }
        else /* Looking for free slot */
        {
            if (csr_dev[slot] == NULL)
            {
                return slot;
            }
        }
    }
    return -1;
}

/*************************************************************
 * NAME:
 *      usbCleanUrbs
 *
 * DESCRIPTION:
 *      This function is used to remove any outstanding rx-urbs
 *      which must be done on disconnects.
 *
 * RETURNS:
 *      void
 *
 *************************************************************/
void usbCleanUrbs(csr_dev_t *dv)
{
    int i;

    clear_bit(LISTEN_STARTED, &(dv->flags));

    if(dv->intr_urb != NULL)
    {
        DBG_PRINT("Unlink and free Rx INTR\n");
        URB_UNLINK(dv->intr_urb);
        usb_free_urb(dv->intr_urb);
        dv->intr_urb = NULL;
    }
    for(i=0; i<MAX_RX_BULK_URBS; i++)
    {
        if(dv->bulk_urb[i] != NULL)
        {
            DBG_PRINT("Unlink and free Rx BULK_%i\n", i);
            URB_UNLINK(dv->bulk_urb[i]);
            usb_free_urb(dv->bulk_urb[i]);
            dv->bulk_urb[i] = NULL;
        }
    }
#ifdef CSR_BR_USB_USE_SCO_INTF
    for(i=0; i<MAX_RX_ISOC_URBS; i++)
    {
        if(dv->isoc_urb[i] != NULL)
        {
            DBG_PRINT("Unlink and free Rx ISOC_%i\n", i);
            URB_UNLINK(dv->isoc_urb[i]);
            usb_free_urb(dv->isoc_urb[i]);
            dv->isoc_urb[i] = NULL;
        }
    }
#endif
}


/*************************************************************
 * NAME:
 *      csrUsbDisconnect
 *
 * DESCRIPTION:
 *      This function is called by the OS when the USB device is
 *      removed from the system - hence some cleanup must be done.
 *
 * RETURNS:
 *      void
 *
 *************************************************************/
void csrUsbDisconnect(struct usb_interface *intf)
{
    int devno;
    csr_dev_t *dv;

    struct usb_device *uDev = interface_to_usbdev(intf);

    devno = csrSlotFromUsb(uDev);
    if (devno == -1)
    {
        /*
         * This means detachment for the SCO/USB interfaces.  This
         * means we don't have to do anything.
         */
        up(&csr_dev_sem);
        return;
    }
    else
    {
        /* Grab device and mark slot as unused. */
        dv = csr_dev[devno];
        csr_dev[devno] = NULL;
        down(&dv->devlock);
    }

    up(&csr_dev_sem); /* XXX keep? */

    if (test_bit(R_THREAD_RUNNING, &(dv->flags)))
    {
        DBG_PRINT("Stopping the reader thread\n");

        clear_bit(R_THREAD_RUNNING, &(dv->flags));
        wake_up_interruptible(&(dv->queue.wait));

        /* Release lock so reader thread can work. */
        up(&dv->devlock);

        wait_for_completion(&dv->reader_thread_exit);
        down(&dv->devlock);

        /* XXX: Is this whole dance correct? */

        DBG_PRINT("The reader thread has been stopped\n");
    }
    else
    {
        DBG_PRINT("Reader thread was not running\n");
    }

    /* Set the flags indicating that the module must be stopped */
    clear_bit(DEVICE_CONNECTED, &(dv->flags));
    clear_bit(LISTEN_STARTED, &(dv->flags));
    clear_bit(BULK_IN_READY, &(dv->endpoint_present));
    clear_bit(BULK_OUT_READY, &(dv->endpoint_present));
    clear_bit(INTR_IN_READY, &(dv->endpoint_present));
    clear_bit(ISOC_IN_READY, &(dv->endpoint_present));
    clear_bit(ISOC_OUT_READY, &(dv->endpoint_present));
    clear_bit(EXTRA_INTERFACE_READY, &(dv->endpoint_present));
#ifdef CSR_BR_USB_USE_DFU_INTF    
    clear_bit(DFU_READY, &(dv->endpoint_present));
#endif

    /* Let thing settle, before freeing memory */
    set_current_state(TASK_INTERRUPTIBLE);
    schedule_timeout(2*HZ);

    if(csr_setup.ext_iface == true)
    {
        csr_setup.ext_exit();
    }
#ifdef CSR_BR_USB_USE_SCO_INTF
    /* Release reassembly buffers */
    if (dv->isoc_reassembly != NULL)
    {
        pfree(dv->isoc_reassembly);
        dv->isoc_reassembly = NULL;
    }
#endif

    /* Make sure everything has been freed */
    QueueFree(dv);
    usbCleanUrbs(dv);

    device_set_wakeup_capable(&(uDev->dev),false);
    device_set_wakeup_enable(&(uDev->dev),0);
    /* Nothing should be using the device at
     * this point, so it's safe to free it.
     */
    pfree(dv);

    DBG_PRINT("bt_usb%u: device removed\n", devno);
}

/*************************************************************
 * NAME:
 *      usbTxBulkComplete (internal)
 *
 * DESCRIPTION:
 *      This function is called whenever transmission to the
 *      device has completed. It is a call back function,
 *      hence it is not called from this application. If there
 *      are data on the outgoing queue, they will be sent
 *
 * RETURNS:
 *      void
 *
 *************************************************************/
#if ((LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19)))
static void usbTxBulkComplete(struct urb *urb, struct pt_regs *regs)
#else
static void usbTxBulkComplete(struct urb *urb)
#endif
{
    if((urb->status != 0) &&
       (urb->status != -ENOENT) &&
       (urb->status != -ECONNRESET) &&
       (urb->status != -ESHUTDOWN))
    {
        printk(PRNPREFIX "Tx BULK completion error, status: %d\n",
            urb->status);
    }
    else
    {
        DBG_VERBOSE("Tx BULK complete\n");
    }

    /* Free the data no matter what */
    kfree(urb->transfer_buffer);
}

/*************************************************************
 * NAME:
 *      usbTxCtrlComplete (internal)
 *
 * DESCRIPTION:
 *      This function is called whenever transmission to the
 *      device has completed. It is a call back function,
 *      hence it is not called from this application. If there
 *      are data on the outgoing queue, they will be sent
 *
 * RETURNS:
 *      void
 *
 *************************************************************/
#if ((LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19)))
static void usbTxCtrlComplete(struct urb *urb, struct pt_regs *regs)
#else
static void usbTxCtrlComplete(struct urb *urb)
#endif
{
    if((urb->status != 0) &&
       (urb->status != -ENOENT) &&
       (urb->status != -ECONNRESET) &&
       (urb->status != -ESHUTDOWN))
    {
        printk(PRNPREFIX "Tx CTRL completion error, status: %d\n",
            urb->status);
    }
    else
    {
        DBG_VERBOSE("Tx CTRL complete\n");
    }

    /* Free the data no matter what */
    kfree(urb->transfer_buffer);
}

/*************************************************************
 * NAME:
 *      usbTxIsocComplete (internal)
 *
 * DESCRIPTION:
 *      This function is called whenever transmission to the
 *      device has completed. It is a call back function,
 *      hence it is not called from this application. If there
 *      are data on the outgoing queue, they will be sent
 *
 * RETURNS:
 *      void
 *
 *************************************************************/
#ifdef CSR_BR_USB_USE_SCO_INTF
#if ((LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19)))
static void usbTxIsocComplete(struct urb *urb, struct pt_regs *regs)
#else
static void usbTxIsocComplete(struct urb *urb)
#endif
{
    if((urb->status != 0) &&
       (urb->status != -ENOENT) &&
       (urb->status != -ECONNRESET) &&
       (urb->status != -ESHUTDOWN))
    {
        printk(PRNPREFIX "Tx ISOC completion error, status: %d\n",
            urb->status);
    }
    else
    {
        DBG_VERBOSE("Tx ISOC complete\n");
    }

    /* Free the data no matter what */
    kfree(urb->transfer_buffer);
}
#endif

/*************************************************************
 * NAME:
 *      fillIsocDesc
 *
 * DESCRIPTION:
 *      This function fills the ISOC description
 *
 * PARAM:
 *      isocUrb: A pointer to the urb that will have its description filled.
 *      size: Total size of transfer_buffer
 *      mtu: The length of the data to be sent (frame size)
 *      max: The number of frames
 *
 * RETURNS:
 *      void
 *
 *************************************************************/
 #ifdef CSR_BR_USB_USE_SCO_INTF
static void fillIsocDesc(struct urb *isocUrb,
                         int size,
                         int mtu,
                         int max)
{
    uint16_t offset;
    uint16_t i;

    for(i=0, offset = 0;
        (i < max) && (size >= mtu);
        i++, offset += mtu, size -= mtu)
    {
        isocUrb->iso_frame_desc[i].offset = offset;
        isocUrb->iso_frame_desc[i].length = mtu;
    }
    if(i < max && size)
    {
        isocUrb->iso_frame_desc[i].offset = offset;
        isocUrb->iso_frame_desc[i].length = size;
        i++;
    }
    isocUrb->number_of_packets = i;
}

/*************************************************************
 * NAME:
 *      usbTxIsoc
 *
 * DESCRIPTION:
 *      This function is called whenever a ISOC message (SCO) is
 *      to be sent to the usb device.
 *
 * RETURNS:
 *      0 if successful, otherwise an error message
 *
 *************************************************************/
int16_t usbTxIsoc(csr_dev_t *dv, void *data, uint16_t length)
{
    struct urb *txisoc;
    uint8_t err;

    err = -ENOMEM;

#ifdef CONFIG_ARCH_TEGRA
	/* URB data should be 32-byte aligned */
	if (unlikely((int)data & 0x0000001F))
	{
		printk(PRNPREFIX "Tx ISOC submit URB alignment error\n");
		return -EFAULT;
	}
#endif

    txisoc = URB_ALLOC(csr_setup.max_isoc_out_frames, GFP_KERNEL | GFP_DMA);
    if(txisoc != NULL)
    {
        /* ISOC URBs must be filled manually */
        txisoc->dev = dv->dev;
        txisoc->pipe = usb_sndisocpipe(dv->dev,
                                    dv->isoc_out_ep);
        txisoc->transfer_buffer = data;
        txisoc->complete = usbTxIsocComplete;
        txisoc->transfer_buffer_length = length;
        txisoc->interval = dv->isoc_out_interval;
        txisoc->context = dv;
        txisoc->transfer_flags = URB_ISO_ASAP;

        /* Setup frame size and offset */
        fillIsocDesc(txisoc,
                     length,
                     length / csr_setup.max_isoc_out_frames,
                     csr_setup.max_isoc_out_frames);

        err = URB_SUBMIT(txisoc, GFP_KERNEL);

        if(err !=0 )
        {
            printk(PRNPREFIX "Tx ISOC submit error, status: %d\n",
                   err);

            /* Attempt to resubmit */
            err = URB_SUBMIT(txisoc, GFP_KERNEL);
            if(err !=0 )
            {
                printk(PRNPREFIX "Tx ISOC re-submit error, status: %d\n",
                       err);
            }
        }
        else
        {
            DBG_VERBOSE("Transmitted %i ISOC bytes\n", length);
        }

        /* Free the URB */
        usb_free_urb(txisoc);
    }
    else
    {
        printk(PRNPREFIX "Tx ISOC alloc error\n");
    }
    return err;
}
#endif
/*************************************************************
 * NAME:
 *      usbTxCtrl (internal)
 *
 * DESCRIPTION:
 *      This function is called whenever a control message
 *      (HCI-command) is to be sent to the usb device.
 *
 * RETURNS:
 *      0 if successful, otherwise an error message
 *
 *************************************************************/
int16_t usbTxCtrl(csr_dev_t *dv, void *data, uint16_t length)
{
    int16_t err;
    struct urb *txctrl;

#ifdef CONFIG_ARCH_TEGRA
	/* URB data should be 32-byte aligned */
	if (unlikely((int)data & 0x0000001F))
	{
		printk(PRNPREFIX "Tx CTRL submit URB alignment error\n");
		return -EFAULT;
	}
#endif

    err = -ENOMEM;
    txctrl = URB_ALLOC(0, GFP_KERNEL);
    if(txctrl != NULL)
    {
        /* Setup packet */
        dv->ctrl_setup.wLength = cpu_to_le16(length);
        usb_fill_control_urb(txctrl,
                             dv->dev,
                             usb_sndctrlpipe(dv->dev,
                                             dv->ctrl_ep),
                             (unsigned char*)&dv->ctrl_setup,
                             data,
                             length,
                             usbTxCtrlComplete,
                             dv);
	
        err = URB_SUBMIT(txctrl, GFP_KERNEL);

        if(err != 0)
        {
            printk(PRNPREFIX "Tx CTRL submit error, code %d\n", err);

            /* Attempt to resubmit */
            err = URB_SUBMIT(txctrl, GFP_KERNEL);
            if(err !=0 )
            {
                printk(PRNPREFIX "Tx CTRL re-submit error, status: %d\n",
                       err);
            }
        }
        else
        {
            DBG_VERBOSE("Transmitted %i CTRL bytes\n", length);
        }

        /* Free the URB */
        usb_free_urb(txctrl);
    }
    else
    {
        printk(PRNPREFIX "Tx CTRL alloc error, code %d\n", err);
    }
    return err;
}

/*************************************************************
 * NAME:
 *      usbTxBulk
 *
 * DESCRIPTION:
 *      This function is called whenever a bulk message (ACL) is
 *      to be sent to the usb device.
 *
 * RETURNS:
 *      0 if successful, otherwise an error message
 *
 *************************************************************/
int16_t usbTxBulk(csr_dev_t *dv, void *data, uint16_t length)
{
    int16_t err;
    struct urb *txbulk;

	
#ifdef CONFIG_ARCH_TEGRA
	/* URB data should be 32-byte aligned */
	if (unlikely((int)data & 0x0000001F))
	{
		printk(PRNPREFIX "Tx BULK submit URB alignment error\n");
		return -EFAULT;
	}
#endif

    err = -ENOMEM;
    txbulk = URB_ALLOC(0, GFP_KERNEL);

    if(txbulk != NULL)
    {
        usb_fill_bulk_urb(txbulk,
                          dv->dev,
                          usb_sndbulkpipe(dv->dev,
                                          dv->bulk_out_ep),
                          data,
                          length,
                          usbTxBulkComplete,
                          dv);
        err = URB_SUBMIT(txbulk, GFP_KERNEL);

        if (err!=0)
        {
            printk(PRNPREFIX "Tx BULK submit error, code %d\n", err);

            /* Attempt to resubmit */
            err = URB_SUBMIT(txbulk, GFP_KERNEL);
            if(err !=0 )
            {
                printk(PRNPREFIX "Tx BULK re-submit error, status: %d\n",
                       err);
            }
        }
        else
        {
            DBG_VERBOSE("Transmitted %i BULK bytes\n", length);
        }

        /* Free the URB */
        usb_free_urb(txbulk);
    }
    else
    {
        printk(PRNPREFIX "Tx BULK alloc error, code %d\n", err);
    }
    return err;
}

/*************************************************************
 * NAME:
 *      reassembleIsoc
 *
 * DESCRIPTION:
 *      This function reassembles ISOC frames to SCO packets.
 *      Calling context is FIXME
 *
 * PARAMETERS:
 *      data: A pointer to the data to be reassembled
 *      length: The length of the data block
 *
 * RETURNS:
 *      Error code, zero is success
 *
 *************************************************************/
#ifdef CSR_BR_USB_USE_SCO_INTF
static int32_t reassembleIsoc(csr_dev_t *dv, uint8_t *dat,
                              struct usb_iso_packet_descriptor *frameDesc)
{
    uint8_t len;
    uint32_t length;
    uint8_t *data;

    data = dat + frameDesc->offset;
    length = frameDesc->actual_length;

    /* The frame was not ok, remove possible previously received data to
     * be reassembled with this part */
    if(frameDesc->status != 0)
    {
        if(dv->isoc_reassembly != NULL)
        {
            pfree(dv->isoc_reassembly);
            dv->isoc_reassembly = NULL;
            dv->isoc_total = 0;
            if(length <= dv->isoc_remain)
            {
                dv->isoc_remain -= length;
                return frameDesc->status;
            }
            else
            {
                length -= dv->isoc_remain;
                data += length;
            }
        }
    }

    /* Status of frame is OK, but the previous received frame to be
     * reassembled with this one was not ok, hence discard this frame
     * as well */
    if ((dv->isoc_reassembly == NULL) &&
       (dv->isoc_remain != 0))
    {
        if(length <= dv->isoc_remain)
        {
            dv->isoc_remain -= length;
            return -1;
        }
        else
        {
            length -= dv->isoc_remain;
            data += length;
        }
    }

    /* Everything is fine - start normal reassembly procedure */
    while(length)
    {
        len = 0;
        /* Beginning of frame */
        if (dv->isoc_reassembly==NULL)
        {
            if(length >= HCI_SCO_HDR_SIZE)
            {
                /* The size is specified in byte 3, therefore "+2" */
                dv->isoc_total = *(uint8_t *)(data+2) + HCI_SCO_HDR_SIZE;
                dv->isoc_remain = dv->isoc_total;
            }
            else
            {
                return -EILSEQ;
            }

            dv->isoc_reassembly = (uint8_t*)pmalloc(dv->isoc_total);
        }
        /* Continuation of packet */
        else
        {
        }

        len = min(dv->isoc_remain, length);
        memcpy(&dv->isoc_reassembly[dv->isoc_total - dv->isoc_remain],
               data,
               len);
        dv->isoc_remain -= len;

        /* The frame is complete */
        if (!dv->isoc_remain)
        {
            /* Let the queue consume the data (don't free it!) */
            QueueAppend(dv, BCSP_CHANNEL_SCO,
                        dv->isoc_reassembly,
                        dv->isoc_total,
                        false);
            dv->isoc_reassembly = NULL;
            atomic_inc(&(dv->await_bg_int));
            wake_up_interruptible(&(dv->queue.wait));
        }
        length -= len;
        data += len;
    }

    return 0;
}
#endif
/*************************************************************
 * NAME:
 *      reassembleIntr
 *
 * DESCRIPTION:
 *      Reassmeble fragmented INTR packets. This functionality
 *      is needed for 2.4.x kernels that does not handle this
 *      automatically.
 *
 * RETURNS:
 *
 *************************************************************/
static int32_t reassembleIntr(struct urb *urb)
{
    csr_dev_t *dv;
    uint16_t length;
    uint32_t len;
    uint8_t  *data;

    dv = (csr_dev_t *)urb->context;

    data = urb->transfer_buffer;
    length = urb->actual_length;

    while(length)
    {
        len = 0;

        /* Beginning of frame */
        if(dv->intr_reassembly==NULL)
        {
            if(length >= HCI_EVENT_HDR_SIZE)
            {
                /* The size is specified in byte 2, therefore "+1" */
                dv->intr_total = *(uint8_t *)(data+1) +
                    HCI_EVENT_HDR_SIZE;
                dv->intr_remain = dv->intr_total;
            }
            else
            {
                return -EILSEQ;
            }

            dv->intr_reassembly = (uint8_t*)pmalloc(dv->intr_total);
        }
        /* Continuation of packet */
        else
        {
        }

        len = min(dv->intr_remain, length);

        memcpy(&dv->intr_reassembly[dv->intr_total - dv->intr_remain],
               data,
               len);

        dv->intr_remain -= len;

        /* The frame is complete */
        if (dv->intr_remain == 0)
        {
            /* Let the queue consume the data (don't free it!) */
            QueueAppend(dv, BCSP_CHANNEL_HCI,
                        dv->intr_reassembly,
                        dv->intr_total,
                        false);	// victor modify to avoid memory leak
            dv->intr_reassembly = NULL;
            atomic_inc(&(dv->await_bg_int));
            wake_up_interruptible(&(dv->queue.wait));
        }
        length -= len;
        data += len;
    }
    return 0;
}

/*************************************************************
 * NAME:
 *      usbRxIntrComplete
 *
 * DESCRIPTION:
 *     This function is called asynchronous whenever hci
 *     events are present on the USB device. The received data is copied
 *     to a buffer structure and the scheduler is informed by issuing a
 *     bg_interrupt.  Finally, the function makes a new data read request
 *     to the device.
 *
 * RETURNS:
 *      void
 *
 *************************************************************/
#if ((LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19)))
static void usbRxIntrComplete(struct urb *urb, struct pt_regs *regs)
#else
static void usbRxIntrComplete(struct urb *urb)
#endif
{
    csr_dev_t *dv;

    dv = (csr_dev_t *)urb->context;


    /* Data is available */
    if((urb->status == 0) && (urb->actual_length > 0))
    {
        if(test_bit(DEVICE_CONNECTED, &(dv->flags)) && !test_bit(DEVICE_SUSPENDED, &(dv->flags)))
        {
            reassembleIntr(urb);
        }
        else
        {
            printk("Received INTR data - not sending up!\n");
        }
    }
    if((urb->status != 0) &&
       (urb->status != -ENOENT) &&
       (urb->status != -ECONNRESET) &&
       (urb->status != -ESHUTDOWN))
    {
        DBG_VERBOSE("Rx INTR complete error, code %d\n", urb->status);
    }
    
    if(test_bit(DEVICE_CONNECTED, &(dv->flags)))
    {
        int16_t err;

		if(test_bit(DEVICE_SUSPENDED, &(dv->flags)))
			return;

	err = URB_SUBMIT(urb, GFP_ATOMIC);
        /* Success or disconnect not errors */
        if((err !=0) && (err != -ENODEV))
        {
            printk(PRNPREFIX "Rx INTR resubmit error, status %d\n",
                   err);
        }
        
    }
}

/*************************************************************
 * NAME:
 *      usbRxBulkComplete
 *
 * DESCRIPTION:
 *      This function is called asynchronous whenever ACL data is
 *      present on the USB device. The received data is copied to a
 *      buffer structure and the scheduler is informed by issuing a
 *      bg_interrupt.  Finally, the function makes a new data read
 *      request to the device.
 *
 * RETURNS:
 *      void
 *
 *************************************************************/
#if ((LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19)))
static void usbRxBulkComplete(struct urb *urb, struct pt_regs *regs)
#else
static void usbRxBulkComplete(struct urb *urb)
#endif
{
    csr_dev_t *dv;
    int16_t err;

    dv = (csr_dev_t *)urb->context;

    /* Data is available */
    if((urb->status == 0) && (urb->actual_length > 0))
    {
        DBG_VERBOSE("Rx BULK complete, %d bytes received\n",
                    urb->actual_length);


        if(test_bit(DEVICE_CONNECTED, &(dv->flags)) && !test_bit(DEVICE_SUSPENDED, &(dv->flags)))
        {

            /* Copy data to queue so URB can be resubmitted */
            QueueAppend(dv, BCSP_CHANNEL_ACL,
                        urb->transfer_buffer,
                        urb->actual_length,
                        true);

            atomic_inc(&(dv->await_bg_int));
            wake_up_interruptible(&(dv->queue.wait));
        }
        else
        {
            DBG_PRINT("BULK data received - not sending up!\n");
        }
    }

    if((urb->status != 0) &&
       (urb->status != -ENOENT) &&
       (urb->status != -ECONNRESET) &&
       (urb->status != -ESHUTDOWN))
    {
        DBG_VERBOSE("Rx BULK complete error, code %d\n", urb->status);
    }
    if(test_bit(DEVICE_CONNECTED, &(dv->flags)))
  	{
		err = URB_SUBMIT(urb, GFP_ATOMIC);
	        /* Success or disconnect not errors */
	        if((err !=0) && (err != -ENODEV))
	        {
	            printk(PRNPREFIX "Rx BULK resubmit error, status %d\n",
	                   err);
	        }
  	}
}

/*************************************************************
 * NAME:
 *      usbRxIsocComplete
 *
 * DESCRIPTION:
 *      This function is called asynchronous whenever sco data
 *      are present on the USB device. The received data is copied to a
 *      buffer structure and the scheduler is informed by issuing a
 *      bg_interrupt.  Finally, the function makes a new data read request
 *      to the device.
 *
 * RETURNS:
 *      void
 *
 *************************************************************/
#ifdef CSR_BR_USB_USE_SCO_INTF
#if ((LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19)))
static void usbRxIsocComplete(struct urb *urb, struct pt_regs *regs)
#else
static void usbRxIsocComplete(struct urb *urb)
#endif
{
    csr_dev_t *dv;
    int16_t err, i;

    dv = (csr_dev_t *)urb->context;

    /* Data is available */
    err = 0;
    if((urb->status == 0) && (urb->actual_length > 0))
    {
        if(test_bit(DEVICE_CONNECTED, &(dv->flags)) && !test_bit(DEVICE_SUSPENDED, &(dv->flags)))
        {
            for(i=0; i < urb->number_of_packets; i++)
            {
                reassembleIsoc(dv, (uint8_t *)urb->transfer_buffer,
                               &urb->iso_frame_desc[i]);
            }
        }
        else
        {
            DBG_PRINT("ISOC data received - not sending up!\n");
        }
    }
    if((urb->status != 0) &&
       (urb->status != -ENOENT) &&
       (urb->status != -ECONNRESET) &&
       (urb->status != -ESHUTDOWN))
    {
        DBG_VERBOSE("Rx ISOC complete error, code %d\n", urb->status);
    }

    if(test_bit(DEVICE_CONNECTED, &(dv->flags)))
    {
		if(test_bit(DEVICE_SUSPENDED, &(dv->flags)))
			return;

        /* Resubmit the URB */
        urb->dev = dv->dev;
        err = URB_SUBMIT(urb, GFP_ATOMIC);

        /* Success or disconnect not errors */
        if((err != 0) && (err != -ENODEV))
        {
            printk(PRNPREFIX "Rx ISOC resubmit error, code %d\n",
                   err);
        }
    }
}
#endif
/*************************************************************
 * NAME:
 *      usbRxIntr (internal)
 *
 * DESCRIPTION:
 *      This function is called whenever a read for HCI events
 *      is wanted. The request will result in a asynchronous
 *      call of the usbRxComplete function, when data has been
 *      read from the device.
 *
 * RETURNS:
 *      0 on success, otherwise a number indicating the error
 *
 *************************************************************/
static int16_t usbRxIntr(csr_dev_t *dv)
{
    int16_t err;
    void *buf;
    struct urb *rxintr;

	if(test_bit(DEVICE_SUSPENDED, &(dv->flags)))
		return -ENOENT;


    err = -ENOMEM;
    rxintr = URB_ALLOC(0, GFP_KERNEL);
    buf = kmalloc(MAX_HCI_EVENT_SIZE, GFP_KERNEL | GFP_DMA);
#ifdef CONFIG_ARCH_TEGRA
	/* URB data should be 32-byte aligned */
	if (unlikely((int)buf & 0x0000001F))
	{
		printk(PRNPREFIX "Rx INTR submit URB alignment error\n");
		kfree(buf);
		kfree(rxintr);
		return -EFAULT;
	}
#endif
    if((rxintr != NULL) && (buf != NULL))
    {
        usb_fill_int_urb(rxintr,
                         dv->dev,
                         usb_rcvintpipe(dv->dev,
                                        dv->intr_ep),
                         buf,
                         dv->intr_size,
                         usbRxIntrComplete,
                         dv,
                         dv->intr_interval);


        err = URB_SUBMIT(rxintr, GFP_KERNEL);
		
        if(err != 0)
        {
            usb_free_urb(rxintr);
            dv->intr_urb = NULL;
            printk(PRNPREFIX "Rx INTR submit error, code %d\n", err);
        }
        else
        {
            dv->intr_urb = rxintr;
        }
    }
    else
    {
        printk(PRNPREFIX "Rx INTR alloc error, code %d\n", err);
        if(rxintr)
        {
            pfree(rxintr);
        }
        if(buf)
        {
            pfree(buf);
        }
    }
   
    return err;
}

/*************************************************************
 * NAME:
 *      usbRxBulk (internal)
 *
 * DESCRIPTION:
 *      This function is called whenever a read for BULK data
 *      is wanted. The request will result in a asynchronous
 *      call of the usbRxComplete function, when data has been
 *      read from the device.
 *
 * PARAMS:
 *      number: is the urb number in the bulk urb array
 *
 * RETURNS:
 *      0 on success, otherwise a number indicating the error
 *
 *************************************************************/
static int16_t usbRxBulk(csr_dev_t *dv, uint8_t number)
{
    int16_t err;
    void *buf;
    struct urb *rxbulk;

	if(test_bit(DEVICE_SUSPENDED, &(dv->flags)))
		return -ENOENT;

    err = -ENOMEM;
    rxbulk = URB_ALLOC(0, GFP_KERNEL);
    buf = kmalloc(MAX_HCI_ACL_SIZE, GFP_KERNEL | GFP_DMA);

#ifdef CONFIG_ARCH_TEGRA
	/* URB data should be 32-byte aligned */
	if (unlikely((int)buf & 0x0000001F))
	{
		printk(PRNPREFIX "Rx BULK submit URB alignment error\n");
		kfree(buf);
		kfree(rxbulk);
		return -EFAULT;
	}
#endif

    if((rxbulk != NULL) && (buf != NULL))
    {
        usb_fill_bulk_urb(rxbulk,
                          dv->dev,
                          usb_rcvbulkpipe(dv->dev,
                                          dv->bulk_in_ep),
                          buf,
                          MAX_HCI_ACL_SIZE,
                          usbRxBulkComplete,
                          dv);

        err = URB_SUBMIT(rxbulk, GFP_KERNEL);

        if(err != 0)
        {
            usb_free_urb(rxbulk);
            dv->bulk_urb[number] = NULL;
            printk(PRNPREFIX "Rx BULK submit error, code %d\n",
                   err);
        }
        else
        {
            dv->bulk_urb[number] = rxbulk;
        }
    }
    else
    {
        printk(PRNPREFIX "Rx BULK alloc error, code %d\n", err);
        if(rxbulk)
        {
            pfree(rxbulk);
        }
        if(buf)
        {
            pfree(buf);
        }
    }
    
    return err;
}

/*************************************************************
 * NAME:
 *      usbRxIsoc
 *
 * DESCRIPTION:
 *      This function is called whenever a read for ISOC data
 *      is wanted. The request will result in an asynchronous
 *      call of the usbRxComplete function, when data has been
 *      read from the device.
 *
 * RETURNS:
 *      0 on success, otherwise a number indicating the error
 *
 *************************************************************/
 #ifdef CSR_BR_USB_USE_SCO_INTF
static int16_t usbRxIsoc(csr_dev_t *dv, uint8_t number)
{
    int16_t err;
    uint32_t size;
    void *buf;
    struct urb *rxisoc;

	if(test_bit(DEVICE_SUSPENDED, &(dv->flags)))
		return -ENOENT;

    err = -ENOMEM;
    size = dv->isoc_in_size * csr_setup.max_isoc_in_frames;
    rxisoc = URB_ALLOC(csr_setup.max_isoc_in_frames, GFP_ATOMIC);
    buf = kmalloc(size, GFP_ATOMIC | GFP_DMA);

#ifdef CONFIG_ARCH_TEGRA
	/* URB data should be 32-byte aligned */
	if (unlikely((int)buf & 0x0000001F))
	{
		printk(PRNPREFIX "Rx ISOC submit URB alignment error\n");
		kfree(buf);
		kfree(rxisoc);
		return -EFAULT;
	}
#endif

	if((rxisoc != NULL) && (buf != NULL))
    {
        rxisoc->dev                     = dv->dev;
        rxisoc->context                 = dv;
        rxisoc->pipe                    = usb_rcvisocpipe(dv->dev,
                                                          dv->isoc_in_ep);
        rxisoc->complete                = usbRxIsocComplete;
        rxisoc->interval                = dv->isoc_in_interval;
        rxisoc->transfer_buffer         = buf;
        rxisoc->transfer_buffer_length  = size;
        rxisoc->transfer_flags          = URB_ISO_ASAP;

        fillIsocDesc(rxisoc,
                     size,
                     size / csr_setup.max_isoc_in_frames,
                     csr_setup.max_isoc_in_frames);

        err = URB_SUBMIT(rxisoc, GFP_ATOMIC | GFP_DMA);
        if(err!=0)
        {
            usb_free_urb(rxisoc);
            dv->isoc_urb[number] = NULL;

            printk(PRNPREFIX "Rx ISOC submit error, code %d\n",
                   err);
        }
        else
        {
            dv->isoc_urb[number] = rxisoc;
        }
    }
    else
    {
        printk(PRNPREFIX "Rx ISOC alloc error, code %d\n",
               err);
        if(rxisoc)
        {
            pfree(rxisoc);
        }
        if(buf)
        {
            pfree(buf);
        }
    }
    return err;
}
#endif
/*************************************************************
 * NAME:
 *      startListen
 *
 * DESCRIPTION:
 *      This functions transmits the first receive URBs.
 *      It is called when the probe function has found all interfaces.
 *
 * RETURNS:
 *      void
 *
 *************************************************************/
static void startListen(csr_dev_t *dv)
{
    int res;
    int i;

    set_bit(LISTEN_STARTED, &(dv->flags));

    /* Start listening for ACL and HCI events */
    res = usbRxIntr(dv);
    if(res != 0)
    {
        printk(PRNPREFIX "Error starting Rx INTR, code %d\n", res);
    }
    else
    {
        for(i=0; (i < MAX_RX_BULK_URBS) && (res == 0); i++)
        {
            res = usbRxBulk(dv, i);
            if(res != 0)
            {
                printk(PRNPREFIX "Error starting Rx BULK, code %d\n", res);
            }
        }
#ifdef CSR_BR_USB_USE_SCO_INTF
        for(i = 0; (i < MAX_RX_ISOC_URBS) && (res == 0); i++)
        {
           res = usbRxIsoc(dv, i);
            if(res != 0)
            {
                printk(PRNPREFIX "Error starting Rx ISOC, code %d\n",res);
            }
        }
#endif        
    }

    DBG_PRINT("Listen loop started, code %i\n", res);
}

/*************************************************************
 * NAME:
 *      handleIsocInEndpoint
 *
 * DESCRIPTION:
 *      Probe helper for ISOC-IN endpoints
 *
 * RETURNS:
 *      void
 *
 *************************************************************/
static void handleIsocInEndpoint(csr_dev_t *dv, int alt,
                                 struct usb_endpoint_descriptor *endpoint,
                                 int ifnum)
{
    if(alt == csr_setup.isoc_alt_setting)
    {
        dv->isoc_reassembly = NULL;
        if(usb_set_interface(dv->dev, ifnum,
                             csr_setup.isoc_alt_setting))
        {
            printk("bt_usb%u: Cannot set ISOC interface alternate settings\n",
                dv->minor);
        }
        else
        {
            /*
             * The interval of the USB descriptor of the isochronous
             * interface is expressed as 2**(N-1), where N is the value to
             * read from the descriptor.
             * When setting up the isochronous interface we must specify the
             * interval as number of (micro)frames, hence we must convert
             * N to nr of frames.
             */
            dv->isoc_in_interval = 1 << (endpoint->bInterval-1);
            dv->isoc_in_size = endpoint->wMaxPacketSize;
            dv->isoc_in_ep = endpoint->bEndpointAddress;

            set_bit(ISOC_IN_READY, &dv->endpoint_present);

            printk("bt_usb%u: ISOC in int %u mps %u\n",
                dv->minor, dv->isoc_in_interval, dv->isoc_in_size);
        }
    }
}

/*************************************************************
 * NAME:
 *      handleIsocOutEndpoint
 *
 * DESCRIPTION:
 *      Probe helper for ISOC-OUT endpoints
 *
 * RETURNS:
 *      void
 *
 *************************************************************/
static void handleIsocOutEndpoint(csr_dev_t *dv, int alt,
                                  struct usb_endpoint_descriptor *endpoint)
{
    if(alt == csr_setup.isoc_alt_setting)
    {
        dv->isoc_out_size = endpoint->wMaxPacketSize;
        dv->isoc_out_interval = 1 << (endpoint->bInterval-1);
        dv->isoc_out_ep = endpoint->bEndpointAddress;

        set_bit(ISOC_OUT_READY, &dv->endpoint_present);

        printk("bt_usb%u: ISOC out int %u mps %u\n",
            dv->minor, dv->isoc_out_interval, dv->isoc_out_size);
    }
}
/*************************************************************
 * NAME:
 *      handleBulkInEndpoint
 *
 * DESCRIPTION:
 *      Probe helper for BULK-IN endpoints
 *
 * RETURNS:
 *      void
 *
 *************************************************************/
static void handleBulkInEndpoint(csr_dev_t *dv, int alt,
                                 struct usb_endpoint_descriptor *endpoint)
{
    dv->bulk_in_ep = endpoint->bEndpointAddress;

    set_bit(BULK_IN_READY, &dv->endpoint_present);

    printk("bt_usb%u: BULK in int %u mps %u ep = %d\n",
        dv->minor, endpoint->bInterval, endpoint->wMaxPacketSize, dv->bulk_in_ep);
}

/*************************************************************
 * NAME:
 *      handleBulkOutEndpoint
 *
 * DESCRIPTION:
 *      Probe helper for BULK-OUT endpoints
 *
 * RETURNS:
 *      void
 *
 *************************************************************/
static void handleBulkOutEndpoint(csr_dev_t *dv, int alt,
                                  struct usb_endpoint_descriptor *endpoint)
{
    dv->bulk_out_ep = endpoint->bEndpointAddress;

    set_bit(BULK_OUT_READY, &dv->endpoint_present);
    printk("bt_usb%u: BULK out int %u mps %u\n",
        dv->minor, endpoint->bInterval, endpoint->wMaxPacketSize);
}

/*************************************************************
 * NAME:
 *      handleIntrInEndpoint
 *
 * DESCRIPTION:
 *      Probe helper for INTR-IN endpoints
 *
 * RETURNS:
 *      void
 *
 *************************************************************/
static void handleIntrInEndpoint(csr_dev_t *dv, int alt,
                                 struct usb_endpoint_descriptor *endpoint)
{
    dv->intr_ep = endpoint->bEndpointAddress;
    dv->intr_interval = endpoint->bInterval;
	dv->intr_size = endpoint->wMaxPacketSize;

    set_bit(INTR_IN_READY, &dv->endpoint_present);

    printk("bt_usb%u: INTR in int %u mps %u EP = %d\n",
        dv->minor, endpoint->bInterval, dv->intr_size, dv->intr_ep);
}
#ifdef CSR_BR_USB_USE_DFU_INTF
/*************************************************************
 * NAME:
 *      handleDfuInterface
 *
 * DESCRIPTION:
 *      Probe helper for DFU interfaces
 *
 * RETURNS:
 *      void
 *
 *************************************************************/
static void handleDfuInterface(csr_dev_t *dv, int ifnum)
{
    dv->dfu_iface = ifnum;
    set_bit(DFU_READY, &dv->endpoint_present);
    printk("bt_usb%u: DFU interface %u\n", dv->minor, ifnum);
}
#endif
/*************************************************************
 * NAME:
 *      csrUsbProbe
 *
 * DESCRIPTION:
 *      Is called whenever a USB device is inserted, which
 *      is believed to be of interest for this driver.
 *
 * RETURNS:
 *
 *
 *************************************************************/
int csrUsbProbe(struct usb_interface *intf,
                const struct usb_device_id *id)
{
    char buf[16];
    struct usb_endpoint_descriptor *endpoint;
    struct usb_interface *sco_intf;
#ifdef CSR_BR_USB_USE_DFU_INTF    
    struct usb_interface *dfu_intf;
#endif    
    csr_dev_t *dv;
    int alt;
    int devno;
    bool res;
    const struct usb_device_id *pDevMatch;

    struct usb_device *uDev = interface_to_usbdev(intf);
    unsigned int ifnum;
    printk(PRNPREFIX "enter csrUsbProbe.\n");

    /* Look up a free slot. */
    devno = csrSlotFromUsb(NULL);
    if (devno == -1)
    {
        up(&csr_dev_sem);
        printk(PRNPREFIX "no more free slots, exiting.\n");
        return 0;
    }

    /* Do FW upgrade only for QCOM chips */
    if ( !(id->driver_info) )
    {
        pDevMatch = usb_match_id(intf, fw_update_device_table);

        if( pDevMatch )
        {
            if( pDevMatch->driver_info & BTUSB_ATH3012 )
            {
                btusb_firmware_upgrade(uDev);
            }
        }
    }

    
    dv = csr_dev[devno] = zpmalloc(sizeof(*dv));

    /* Lock device and release device list lock. */
    init_MUTEX_LOCKED(&dv->devlock);
    dv->dev = uDev;
    up(&csr_dev_sem); /* XXX: pbly needs to hold this longer */

    dv->minor = devno;
    init_completion(&dv->reader_thread_exit);
    atomic_set(&dv->await_bg_int, 0);

    if (uDev->descriptor.iSerialNumber)
    {
        if (usb_string(uDev, uDev->descriptor.iSerialNumber,
            buf, sizeof(buf)) > 0)
        {
            /* The format is ``lap-uap-nap''.  Substitute
             * '\0' for '-' in the string for easy printing.
             */
            buf[6] = 0;
            buf[9] = 0;
            printk("bt_usb%u: %s:%s:%s\n", devno,
                &buf[10], &buf[7], &buf[0]);
        }
    }
    printk("bt_usb%u: probing interfaces\n", devno);

    /* SCO interface is mandatory */
    sco_intf = usb_ifnum_to_if(uDev, 1);
    if (sco_intf == NULL)
    {
        printk("bt_usb%u: SCO interface missing, exiting.\n", devno);
        return 0;
    }
    else if (usb_driver_claim_interface(&csr_usb_driver, sco_intf,
        NULL) != 0)
    {
        printk("bt_usb%u: couldn't claim SCO interface, exiting.\n", devno);
        return 0;
    }
#ifdef CSR_BR_USB_USE_DFU_INTF
    dfu_intf = usb_ifnum_to_if(uDev, 2);
    if (dfu_intf == NULL)
    {
        printk("bt_usb%u: warning: DFU interface missing.\n", devno);
    }
    else if (usb_driver_claim_interface(&csr_usb_driver, dfu_intf,
        NULL) != 0)
    {
        printk("bt_usb%u: couldn't claim DFU interface, exiting.\n", devno);
        return 0;
    }
#endif /*CSR_BR_USB_USE_DFU_INTF*/
    res = false;

    /* Initialize pending queue */
    QueuePrepare(dv);

    dv->isoc_reassembly = NULL;

    /* Set values for the control request header stuff */
    dv->ctrl_setup.bRequestType = BT_CTRL_REQUEST;
    dv->ctrl_setup.bRequest = 0;
    dv->ctrl_setup.wValue = 0;
    dv->ctrl_setup.wIndex = 0;


    /* The control endpoint is mandatory for all usb devices,
       and can therefore be setup statically */
    dv->ctrl_ep = USB_ENDPOINT_XFER_CONTROL;
        printk("bt_usb%u: Control endpoint set \n", devno);

    /* Scan HCI/ACL interface for endpoints */
    for (alt = 0; alt < intf->num_altsetting; alt++)
    {
        uint8_t i, num_ep;
        struct usb_interface_descriptor *ifaceDesc;

        struct usb_host_interface *host;
        host = &(intf->altsetting[alt]);
        ifaceDesc = &(host->desc);
        num_ep = ifaceDesc->bNumEndpoints;
        ifnum = ifaceDesc->bInterfaceNumber;

        /* Scan endpoints */
        for (i = 0; i < num_ep; i++)
        {
            endpoint = &host->endpoint[i].desc;
            /* BULK in */
            if ((endpoint->bEndpointAddress & USB_DIR_IN) &&
               ((endpoint->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK)
                == USB_ENDPOINT_XFER_BULK))
            {
                if (csr_setup.ext_iface)
                {
                    res = csr_setup.ext_probe(dv, ifnum, intf, alt);
                }
                if ((res == false) &&
                    !test_bit(BULK_IN_READY, &dv->endpoint_present))
                {
                    handleBulkInEndpoint(dv, alt, endpoint);
                }
            }

            /* BULK out */
            else if (!(endpoint->bEndpointAddress & USB_DIR_IN) &&
                    ((endpoint->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK)
                     == USB_ENDPOINT_XFER_BULK))
            {
                if (csr_setup.ext_iface)
                {
                    res = csr_setup.ext_probe(dv, ifnum, intf, alt);
                }
                if ((res == false) &&
                    !test_bit(BULK_OUT_READY, &dv->endpoint_present))
                {
                    handleBulkOutEndpoint(dv, alt, endpoint);
                }
            }

            /* INTR in */
            else if ((endpoint->bEndpointAddress & USB_DIR_IN) &&
                    ((endpoint->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK)
                     == USB_ENDPOINT_XFER_INT))
            {
                if (csr_setup.ext_iface)
                {
                    res = csr_setup.ext_probe(dv, ifnum, intf, alt);
                }
                if ((res == false) &&
                   !test_bit(INTR_IN_READY, &dv->endpoint_present))
                {
                    handleIntrInEndpoint(dv, alt, endpoint);
                }
            }
            else
            {
                printk("bt_usb%u: Unknown endpoint\n", devno);
            }
        }
    }
    /* Scan SCO interface for endpoints */
    for (alt = 0; alt < sco_intf->num_altsetting; alt++)
    {
        uint8_t i, num_ep;
        struct usb_interface_descriptor *ifaceDesc;

        struct usb_host_interface *host;
        host = &(sco_intf->altsetting[alt]);
        ifaceDesc = &(host->desc);
        num_ep = ifaceDesc->bNumEndpoints;
        ifnum = ifaceDesc->bInterfaceNumber;

        /* Scan endpoints */
        for (i = 0; i < num_ep; i++)
        {
            endpoint = &host->endpoint[i].desc;

            /* ISOC in */
            if ((endpoint->bEndpointAddress & USB_DIR_IN) &&
                    ((endpoint->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK)
                     == USB_ENDPOINT_XFER_ISOC))
            {
                if (csr_setup.ext_iface)
                {
                    res = csr_setup.ext_probe(dv, ifnum, sco_intf, alt);
                }
                if ((res == false) &&
                   !test_bit(ISOC_IN_READY, &dv->endpoint_present))
                {
                    handleIsocInEndpoint(dv, alt, endpoint, ifnum);
                }
            }

            /* ISOC out */
            else if (!(endpoint->bEndpointAddress & USB_DIR_IN) &&
                    ((endpoint->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK)
                     == USB_ENDPOINT_XFER_ISOC))
            {
                if (csr_setup.ext_iface)
                {
                    res = csr_setup.ext_probe(dv, ifnum,
                                              sco_intf, alt);
                }
                if ((res == false) &&
                   !test_bit(ISOC_OUT_READY, &dv->endpoint_present))
                {
                    handleIsocOutEndpoint(dv, alt,endpoint);
                }
            }
            else
            {
                printk("bt_usb%u: Unknown endpoint\n", devno);
            }
        }
    }
#ifdef CSR_BR_USB_USE_DFU_INTF
    if(dfu_intf != NULL)
    {
        /* Scan DFU interface for endpoints */
        for (alt = 0; alt < dfu_intf->num_altsetting; alt++)
        {
            struct usb_interface_descriptor *ifaceDesc;
            struct usb_host_interface *host;

            host = &(dfu_intf->altsetting[alt]);
            ifaceDesc = &(host->desc);
            ifnum = ifaceDesc->bInterfaceNumber;

            /* Detect DFU interface */
            if ((ifaceDesc->bNumEndpoints == 0) &&
               (ifaceDesc->bInterfaceClass == DFU_IFACE_CLASS) &&
               (ifaceDesc->bInterfaceSubClass == DFU_IFACE_SUB_CLASS) &&
               (ifaceDesc->bInterfaceProtocol == DFU_IFACE_PROTOCOL))
            {
                if (!test_bit(DFU_READY, &dv->endpoint_present))
                {
                    handleDfuInterface(dv, ifnum);
                }
            }
            else
            {
                printk("bt_usb%u: no valid DFU interface found\n", devno);
            }
        }
    }
#endif /*CSR_BR_USB_USE_DFU_INTF*/	
#ifdef CONFIG_USB_SUSPEND
    //usb_enable_autosuspend(uDev);
    //intf->needs_remote_wakeup = 1;
#endif

    if (test_bit(BULK_IN_READY, &dv->endpoint_present) &&
       test_bit(BULK_OUT_READY, &dv->endpoint_present) &&
       test_bit(ISOC_OUT_READY, &dv->endpoint_present) &&
       test_bit(ISOC_IN_READY, &dv->endpoint_present) &&
       test_bit(INTR_IN_READY, &dv->endpoint_present) &&
       !test_bit(LISTEN_STARTED, &(dv->flags)))
    {
        DBG_PRINT("All required endpoints found, starting loop\n");
       set_bit(DEVICE_CONNECTED, &(dv->flags)); 
       startListen(dv);

        DBG_PRINT("creating readerThread\n");
        /* Start the reader thread */
        // victor change: kernel_thread is not supported in kernel
        //kernel_thread(readerThread, dv, 0);
        kthread_run(readerThread, dv, "bt_usb%u", dv->minor);
    }

    /*
     * Let go of device lock so the
     * reader thread can start.
     */
    up(&dv->devlock);
    device_init_wakeup(&(uDev->dev),1);
    printk("bt_usb%u: device_init_wakeup is done\n",devno);
    return 0;
}

#ifdef CONFIG_USB_SUSPEND
int csrUsbSuspend(struct usb_interface *intf, pm_message_t state)
{
    int devno;
    csr_dev_t *dv;

    struct usb_device *uDev = interface_to_usbdev(intf);

    devno = csrSlotFromUsb(uDev);
    if (devno == -1)
    {
        /*
         * This means detachment for the SCO/USB interfaces.  This
         * means we don't have to do anything.
         */
        up(&csr_dev_sem);
        return 0;
    }
    else
    {
        /* Grab device and mark slot as unused. */
        dv = csr_dev[devno];
    }

    up(&csr_dev_sem); /* XXX keep? */

    if(intf->pm_usage_cnt.counter > 0)
    {
        printk("bt_usb%u: attempt to suspend (PM message: 0x%04x) - busy\n", devno, state.event);
        return -EBUSY;
    }

    if(test_bit(DEVICE_SUSPENDED, &(dv->flags)))
    {
        printk("bt_usb%u: already suspended\n", devno);
        return 0;
    }

    set_bit(DEVICE_SUSPENDED, &(dv->flags));

    usbCleanUrbs(dv);

    printk("bt_usb%u: went to suspend (PM message: 0x%04x)\n", devno, state.event);

    return 0;
}

int csrUsbResume(struct usb_interface *intf)
{
    int devno;
    csr_dev_t *dv;

    struct usb_device *uDev = interface_to_usbdev(intf);

    devno = csrSlotFromUsb(uDev);
    if (devno == -1)
    {
        /*
         * This means detachment for the SCO/USB interfaces.  This
         * means we don't have to do anything.
         */
        up(&csr_dev_sem);
        return 0;
    }
    else
    {
        /* Grab device and mark slot as unused. */
        dv = csr_dev[devno];
    }

    up(&csr_dev_sem);

    if(!test_bit(DEVICE_SUSPENDED, &(dv->flags)))
    {
        printk("bt_usb%u: not in suspend mode\n", devno);
        return 0;
    }

    clear_bit(DEVICE_SUSPENDED, &(dv->flags));

    startListen(dv);

    printk("bt_usb%u: resume\n", devno);

    return 0;
}

int csrUsbResetResume(struct usb_interface *intf)
{
    int devno;
    csr_dev_t *dv;

    struct usb_device *uDev = interface_to_usbdev(intf);

    devno = csrSlotFromUsb(uDev);
    if (devno == -1)
    {
        /*
         * This means detachment for the SCO/USB interfaces.  This
         * means we don't have to do anything.
         */
        up(&csr_dev_sem);
        return 0;
    }
    else
    {
        /* Grab device and mark slot as unused. */
        dv = csr_dev[devno];
    }

    up(&csr_dev_sem);

    clear_bit(DEVICE_SUSPENDED, &(dv->flags));

    startListen(dv);

    printk("bt_usb%u: resume after reset\n", devno);

    return 0;
}
#endif
