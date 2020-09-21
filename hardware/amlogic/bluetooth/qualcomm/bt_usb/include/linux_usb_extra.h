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

#ifndef LINUX_USB_EXTRA_H__
#define LINUX_USB_EXTRA_H__
#include "linux_usb_com.h"

#ifdef __cplusplus
extern "C" {
#endif

/*************************************************************
 * NAME:
 *      probeExtraInterface
 *
 * DESCRIPTION:
 *      Is called by the usb probe function when having handled the
 *      default interfaces.
 *
 * RETURNS:
 *      TRUE   when interface has been probed by this function
 *      FALSE  if not probed.
 *
 *************************************************************/
typedef bool (*probeExtraInterface_t)(csr_dev_t *csr_dev,
                                        uint8_t intfNumber,
                                        struct usb_interface *intf,
                                        int altsetting);

/*************************************************************
 * NAME:
 *      setupExtraInterface
 *
 * DESCRIPTION:
 *      Is used to setup information about what bg_int number to
 *      use and which bg_int function to call, when data is received
 *      on the extra interface
 *
 * RETURNS:
 *      void
 *
 *************************************************************/
typedef void (*setupExtraInterface_t)(uint8_t intrNr,
                                      void (*intrFunc)(void));

/*************************************************************
 * NAME:
 *      disconnectExtraInterface
 *
 * DESCRIPTION:
 *      Is called by the general usb_driver whne it disconnects
 *      and therefore some cleanup of the extra interface must
 *      be done.
 *
 * RETURNS:
 *      0 on success, otherwise non-zero for error
 *
 *************************************************************/
typedef int16_t (*disconnectExtraInterface_t)(void);

/*************************************************************
 * NAME:
 *      startDrvExtraInterface
 *
 * DESCRIPTION:
 *      Is called by the general usb_driver when the layers
 *      above calls UsbDrv_Start
 *
 * RETURNS:
 *      void
 *
 *************************************************************/
typedef void (*startDrvExtraInterface_t)(void);

/*************************************************************
 * NAME:
 *      initExtraInterface
 *
 * DESCRIPTION:
 *      Must called from CsrUsbInit in order to init the extra
 *      usb driver
 *
 * RETURNS:
 *      void
 *
 *************************************************************/
typedef void (*initExtraInterface_t)(void);

/*************************************************************
 * NAME:
 *      exitExtraInterface
 *
 * DESCRIPTION:
 *      Must called from CsrUsbExit in order to init the extra
 *      usb driver
 *
 * RETURNS:
 *      void
 *
 *************************************************************/
typedef void (*exitExtraInterface_t)(void);

/*************************************************************
 * NAME:
 *      setupUsb
 *
 * DESCRIPTION:
 *      Must be called if an extra USB interface is to be used.
 *      It setups the function necessary for the extra interface to
 *      work properly.
 *
 * PARAMETERS:
 *      intrNr: The bg_int number to issue when data is received on
 *              the extra interface and must be passed to BCHS
 *      intrFunc: Pointer to the bg_int function to call on reception of data
 *      pEI: Function pointer to probe the extra interface
 *      dEI: Function pointer to disconnect the extra interface
 *      sDEI: Function pointer to start the extra interface
 *      iEI: Function pointer to initialise the extra interface
 *      eEI: Function pointer to exit the extra interface
 *
 * RETURNS:
 *      void
 *
 *************************************************************/
void SetupUsb(uint8_t intrNumber, void (*intrFunc)(void),
              probeExtraInterface_t pEI,
              disconnectExtraInterface_t dEI,
              startDrvExtraInterface_t sDEI,
              initExtraInterface_t iEI,
              setupExtraInterface_t sEI,
              exitExtraInterface_t eEI);

/* Hooks to change the default USB settings */
void SetSupportedUsbDevices(struct usb_device_id *devs);
void SetUsbDriverName(char *name);
void SetUsbIsocAltSetting(int alt);
void SetUsbMaxIsocInFrames(int in);
void SetUsbMaxIsocOutFrames(int out);
bool devExist(uint8_t devno);
/* Structure to hold the static settings (before the driver is initialized) */
typedef struct
{
    bool                       ext_iface;
    probeExtraInterface_t        ext_probe;
    disconnectExtraInterface_t   ext_disconnect;
    startDrvExtraInterface_t     ext_start;
    initExtraInterface_t         ext_init;
    exitExtraInterface_t         ext_exit;
    int                          max_isoc_in_frames;
    int                          max_isoc_out_frames;
    int                          max_isoc_packet_size;
    int                          isoc_alt_setting;
    struct usb_device_id         *usb_devices;
    char                         *driver_name;
} csr_usb_settings_t;

#ifdef __cplusplus
}
#endif

#endif
