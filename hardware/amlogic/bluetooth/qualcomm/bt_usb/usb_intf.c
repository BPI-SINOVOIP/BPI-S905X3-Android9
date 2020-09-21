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
#include <asm/bitops.h>

#include "include/linux_usb_com.h"
#include "include/linux_usb_dev.h"
#include "include/linux_usb_extra.h"
#include "include/usb_com.h"


#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0) & LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24))
#include <linux/usb_ch9.h>
#endif


/* Globals (device, settings and driver) */
extern csr_dev_t *csr_dev[BT_USB_COUNT];
extern struct semaphore csr_dev_sem;

extern csr_usb_settings_t csr_setup;
extern struct usb_driver csr_usb_driver;

extern void pfree(void *ptr);
/*************************************************************
 * NAME:
 *      devLookup
 *
 * DESCRIPTION:
 *      Takes a device number parameter, grabs device
 *      semaphore, and returns pointer to the csr_dev_t
 *      struct, or NULL if not found.
 *      Caller MUST release device semaphore if non-NULL
 *      is returned.
 *
 * RETURNS:
 *      csr_dev_t pointer, or NULL.
 *
 *************************************************************/
csr_dev_t *devLookup(uint8_t devno)
{
    csr_dev_t *dv;

    if (devno > BT_USB_COUNT - 1)
    {
        dv = NULL;
    }
    else
    {
        dv = csr_dev[devno];
	if(dv != NULL)
	{
       	 down(&dv->devlock); /* XXX: _interruptible? */
	}
    }

    return dv;
}

bool devExist(uint8_t devno)
{
    if (devno > BT_USB_COUNT - 1)
    {
        return false;
    }
    else
    {
	if(csr_dev[devno] != NULL)
	{
       	 return true;
	}
    }

return false;
}
/*************************************************************
 * NAME:
 *      readerThread
 *
 * DESCRIPTION:
 *      This function runs as a thread, and is responsible for
 *      calling the bg_interrupt in the Scheduler signalling
 *      that data is ready for being fetched. The thread is signalled
 *      whenever data has arrived from the USB device.
 *
 * RETURNS:
 *      Thread completion code (zero)
 *
 *************************************************************/
int readerThread(void *parm)
{
    csr_dev_t *dv;

    dv = (csr_dev_t *)parm;

    // Removed by victor: deamonize is not supported in kernel
    //daemonize("bt_usb%u", dv->minor);

    set_bit(R_THREAD_RUNNING, &(dv->flags));

    /* Do run while the boss says so */
    while (test_bit(R_THREAD_RUNNING, &(dv->flags)))
    {
        /* Wait for background interrupts */
        while((test_bit(R_THREAD_RUNNING, &(dv->flags))) &&
              (atomic_read(&(dv->await_bg_int)) == 0))
        {
            DBG_VERBOSE("Reader thread entering sleep\n");
            if(wait_event_interruptible(dv->queue.wait,
                                        !test_bit(R_THREAD_RUNNING, &(dv->flags)) ||
                                        atomic_read(&(dv->await_bg_int))!=0))
            {
                break;
            }
        }
        DBG_VERBOSE("Reader thread woke up\n");

        /* Fire the background interrupt */
        if(test_bit(R_THREAD_RUNNING, &(dv->flags)))
        {
            {
                UsbDev_Rx(dv->minor);
            }
            atomic_dec(&(dv->await_bg_int));
        }
    }

    /* Done */
    clear_bit(R_THREAD_RUNNING, &(dv->flags));
    complete_and_exit(&dv->reader_thread_exit, 0);

    return 0;
}

/*************************************************************
 * NAME:
 *      UsbDriverStarted
 *
 * RETURNS:
 *      A bool_t indicating if the driver is started
 *
 *************************************************************/
bool UsbDriverStarted(void)
{
    return true;
}

/*************************************************************
 * NAME:
 *      UsbDeviceProbed
 *
 * RETURNS:
 *      A bool_t indicating if the driver has been probed
 *
 *************************************************************/
bool UsbDeviceProbed(csr_dev_t *dv)
{
    if(test_bit(DEVICE_CONNECTED, &(dv->flags)))
    {
        return true;
    }
    else
    {
        return false;
    }
}

/*************************************************************
 * NAME:
 *      UsbDriverStopRequested
 *
 * RETURNS:
 *      A bool_t indicating if the driver has been requested to stop
 *
 *************************************************************/
bool UsbDriverStopRequested(void)
{
    return false;
}

/*************************************************************
 * NAME:
 *      ExtraUsbInterfaceReady
 *
 * DESCRIPTION
 *      Signal the driver that the extra interface (extension)
 *      is ready for action
 *
 * RETURNS:
 *      void
 *
 *************************************************************/
void ExtraUsbInterfaceReady(csr_dev_t *dv)
{
    set_bit(EXTRA_INTERFACE_READY,&(dv->endpoint_present));
}

/*************************************************************
 * NAME:
 *      SetupUsb
 *
 * DESCRIPTION
 *      Set the extra (extension) interface function pointers.
 *
 * RETURNS:
 *      void
 *
 *************************************************************/
void SetupUsb(uint8_t interfaceNumber,
              void (*interfaceFunc)(void),
              probeExtraInterface_t pEI,
              disconnectExtraInterface_t dEI,
              startDrvExtraInterface_t sDEI,
              initExtraInterface_t iEI,
              setupExtraInterface_t sEI,
              exitExtraInterface_t eEI)
{
    printk(PRNPREFIX "Setup to use extra interface\n");

    csr_setup.ext_iface = true;
    sEI(interfaceNumber, interfaceFunc);
    csr_setup.ext_probe = pEI;
    csr_setup.ext_disconnect = dEI;
    csr_setup.ext_start = sDEI;
    csr_setup.ext_init = iEI;
    csr_setup.ext_exit = eEI;
}

/*************************************************************
 * NAME:
 *      SetSupportedUsbDevices
 *
 * DESCRIPTION
 *      Replace the default list of supported USB devices
 *
 * RETURNS:
 *      void
 *
 *************************************************************/
void SetSupportedUsbDevices(struct usb_device_id *devs)
{
    csr_usb_driver.id_table = devs;
}

/*************************************************************
 * NAME:
 *      SetUsbDriverName
 *
 * DESCRIPTION
 *      Rename default USB driver name
 *
 * RETURNS:
 *      void
 *
 *************************************************************/
void SetUsbDriverName(char *name)
{
    csr_usb_driver.name = name;
}

/*************************************************************
 * NAME:
 *      SetUsbMaxIsocOutFrames
 *
 * DESCRIPTION
 *      Set number of concurrent ISOC-tx frames
 *
 * RETURNS:
 *      void
 *
 *************************************************************/
void SetUsbMaxIsocOutFrames(int out)
{
    csr_setup.max_isoc_out_frames = out;
}

/*************************************************************
 * NAME:
 *      SetUsbMaxIsocInFrames
 *
 * DESCRIPTION
 *      Set number of concurrent ISOC-rx frames
 *
 * RETURNS:
 *      void
 *
 *************************************************************/
void SetUsbMaxIsocInFrames(int in)
{
    csr_setup.max_isoc_in_frames = in;
}

/*************************************************************
 * NAME:
 *      SetUsbIsocAltSetting
 *
 * DESCRIPTION
 *      Set alternate setting for ISOC interface
 *
 * RETURNS:
 *      void
 *
 *************************************************************/
void SetUsbIsocAltSetting(int alt)
{
    csr_setup.isoc_alt_setting = alt;
}

/*************************************************************
 * NAME:
 *      CsrUsbInit
 *
 * DESCRIPTION
 *      Initialize (register) USB driver. When this function
 *      is called, the USB driver is registerd in the kernel
 *      and the other settings can be altered anymore
 *
 * RETURNS:
 *      void
 *
 *************************************************************/
int CsrUsbInit(void)
{
    int err;

    err = usb_register(&csr_usb_driver);
    if(err < 0)
    {
        printk(PRNPREFIX "Failed to register CSR USB driver, code %d\n",err);
    }
    else
    {
        printk(PRNPREFIX "CSR USB driver registered\n");
    }

    return err;
}

/*************************************************************
 * NAME:
 *      CsrUsbExit
 *
 * DESCRIPTION
 *      Shutdown the USB driver and unregister it from the kernel
 *
 * RETURNS:
 *      void
 *
 *************************************************************/
void CsrUsbExit(void)
{
    printk(PRNPREFIX "Shutting down USB driver\n");

    usb_deregister(&csr_usb_driver);

}

/*************************************************************
 * NAME:
 *      UsbDrv_Stop
 *
 * DESCRIPTION
 *      Stop USB device driver. This is not supported!
 *
 * RETURNS:
 *      void
 *
 *************************************************************/
void UsbDrv_Stop(void)
{
    CsrUsbExit();

    return;
}

/*************************************************************
 * NAME:
 *      UsbDrv_Start
 *
 * DESCRIPTION:
 *      This function is called to initialize this module. The
 *      deviceName parameter is not used, but is only present
 *      in order to provide the same interface to BCHS.
 *
 * RETURNS:
 *      TRUE if the device was opened successfully,
 *      otherwise FALSE
 *************************************************************/
bool UsbDrv_Start(char *deviceName)
{
    memset(csr_dev, 0, sizeof(csr_dev));
    init_MUTEX(&csr_dev_sem);

    /* Register USB driver */
    CsrUsbInit();

    DBG_PRINT("The BCHS USB driver has been started\n");

    if(csr_setup.ext_iface == true)
    {
        csr_setup.ext_start();
    }

    return true;
}
/*************************************************************
 * NAME:
 *      UsbDev_Rx
 *
 * DESCRIPTION:
 *      This function is called by a background interrupt each
 *      time a packet (ACL, SCO or HCI) is received from the
 *      USB device. The function passes the data to the higher
 *      layers.
 *
 * RETURNS:
 *      void
 *
 *************************************************************/
void UsbDev_Rx(uint8_t devno)
{
    csr_dev_t *dv;
    char *tmpbuf;
    struct usb_qe *qe;

    dv = devLookup(devno);

    if((atomic_read(&(dv->queue.count)) > 0 ) &&
       test_bit(DEVICE_CONNECTED, &(dv->flags)))
    {
        do
        {
            qe = QueuePop(dv);
            if(qe != NULL)
            {
                DBG_VERBOSE("Sent %i bytes to stack\n",
                            qe->msg->buflen);

                /* Send to stack, which consumes the message buffer */
                tmpbuf = qe->msg->buf;

                hcCom_ReceiveMsg(qe->msg, qe->chan,
                    dv->minor);

                /* Free the queue-element */
                pfree(qe);
            }
        }
        while (qe != NULL);
    }

    up(&dv->devlock);

    return;
}
/*************************************************************
 * NAME:
 *      UsbDev_Tx
 *
 * DESCRIPTION:
 *      Interface to the above protocol stack, when data is to
 *      be transmitted over the USB interface. Both ACL, SCO
 *      and HCI commands are supported. The type of data is
 *      specified by the channel parameter. A pointer to the
 *      data must be specified in the data parameter. Finally,
 *      the size parameter specifies the size of the data.
 *
 * RETURNS:
 *      TRUE if the data can be buffered for transmission,
 *      otherwise FALSE
 *************************************************************/
bool UsbDev_Tx(uint8_t devno, uint8_t channel, uint8_t *data, uint16_t size)
{
    csr_dev_t *dv;
    bool res;

    DBG_VERBOSE("Have received %i bytes from stack\n",size);
    dv = devLookup(devno);

    if(dv == NULL)
   {
   	printk("bt_usb: Device not initialized\n");
	return false;
   }
    res = false;
    if((size > 0) &&
       test_bit(DEVICE_CONNECTED, &(dv->flags)))
    {
    
        if(channel == BCSP_CHANNEL_HCI)
        {
            usbTxCtrl(dv, data, size);
            res = true;
        }
        else if(channel == BCSP_CHANNEL_ACL)
        {
            usbTxBulk(dv, data, size);
            res = true;
        }
#ifdef CSR_BR_USB_USE_SCO_INTF 
        else if(channel == BCSP_CHANNEL_SCO)
        {
            usbTxIsoc(dv, data, size);
            res = true;
        }
#endif  
    }
    up(&dv->devlock);

    return res;
}
/*************************************************************
 * NAME:
 *      UsbDev_Reset
 *
 * DESCRIPTION
 *      Perform an USB bus reset of the device
 *
 * RETURNS:
 *      void
 *
 *************************************************************/
void UsbDev_Reset(uint8_t devno)
{
    csr_dev_t *dv;

    printk(PRNPREFIX "Resetting USB device..\n");

    dv = devLookup(devno);
    usb_reset_device(dv->dev);

    up(&dv->devlock);
}


#ifdef CONFIG_USB_SUSPEND
void UsbDev_PmDisable(uint8_t devno)
{
#if 0
    csr_dev_t *dv;

	struct usb_interface* usb_intf;

    dv = devLookup(devno);

    if (dv == NULL)
    {
    	return;
    }
	up(&dv->devlock);
	usb_intf = usb_find_interface(&csr_usb_driver, devno);
	
    	usb_autopm_disable(usb_intf);
	printk(PRNPREFIX "bt_usb%u: disabling power-save mode (cnt: %d)\n", devno, usb_intf->pm_usage_cnt);
#endif	
}

void UsbDev_PmEnable(uint8_t devno)
{
#if 0
    csr_dev_t *dv;

	struct usb_interface* usb_intf;

    dv = devLookup(devno);
    if (dv == NULL)
    {
    	return;
    }
    
	up(&dv->devlock);
	usb_intf = usb_find_interface(&csr_usb_driver, devno);

    usb_autopm_enable(usb_intf);
	printk(PRNPREFIX "bt_usb%u: enabling power-save mode (cnt: %d)\n", devno, usb_intf->pm_usage_cnt);
#endif
}
#endif

