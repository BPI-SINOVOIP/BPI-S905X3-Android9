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

#ifndef USB_COM_H__
#define USB_COM_H__
#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*
Public Defines
*============================================================================*/
/* Start of backwards compatibility section */

/* Send primitive on a unreliable channel */
#define BCSP_UNRELIABLE_CHANNEL        0

/* Send primitive on a reliable channel */
#define BCSP_RELIABLE_CHANNEL          1

#define BCSP_CHANNEL_BCCMD             2
#define BCSP_CHANNEL_HQ                3
#define BCSP_CHANNEL_DM                4
#define BCSP_CHANNEL_HCI               5
#define BCSP_CHANNEL_ACL               6
#define BCSP_CHANNEL_SCO               7
#define BCSP_CHANNEL_L2CAP             8
#define BCSP_CHANNEL_RFCOMM            9
#define BCSP_CHANNEL_SDP               10
#define BCSP_CHANNEL_DFU               12
#define BCSP_CHANNEL_VM                13

#define init_MUTEX(sem)		sema_init(sem, 1)
#define init_MUTEX_LOCKED(sem)	sema_init(sem, 0)

/* For backwards compatibility */
/*----------------------------------------------------------------------------*
 *  NAME
 *      hcCom_ReceiveMsg
 *
 *  DESCRIPTION
 *      This function is called when a message is received from abcsp.
 *
 *  RETURNS
 *
 *----------------------------------------------------------------------------*/
void hcCom_ReceiveMsg(void *msg, uint8_t bcspChannel, uint8_t rel);

bool  UsbDrv_Start(char *deviceName);
void    UsbDrv_Stop(void);

#ifdef __linux__
#ifdef __KERNEL__

#ifdef CSR_BR_USB_USE_DFU_INTF
int     UsbDrv_EnterDFU(void);
#endif

/*************************************************************
 * NAME:
 *      CsrUsbInit
 *
 * DESCRIPTION:
 *      This function is called by the core, when the module is
 *      loaded - hence the function will register this driver.
 *
 * RETURNS:
 *      -
 *************************************************************/
int CsrUsbInit(void);

/*************************************************************
 * NAME:
 *      CsrUsbExit
 *
 * DESCRIPTION:
 *      This function is called by the core, when the module is
 *      unloaded - hence the driver must be unregistered
 *
 * RETURNS:
 *      -
 *************************************************************/
void CsrUsbExit(void);
#endif
#endif

#ifdef __cplusplus
}
#endif

#endif /* USB_COM_H_ */

