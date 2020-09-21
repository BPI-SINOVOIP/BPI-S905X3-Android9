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

#ifndef LINUX_USB_IOCTL_H__
#define LINUX_USB_IOCTL_H__
#ifdef __linux__

#include <linux/ioctl.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BT_USB_IOC_MAGIC     'c'
#define BT_USB_IOC_ENTER_DFU			_IO(BT_USB_IOC_MAGIC, 0)
#define BT_USB_IOC_RESET				_IO(BT_USB_IOC_MAGIC, 1)
#ifdef CONFIG_USB_SUSPEND
#	define BT_USB_IOC_ENABLE_SUSPEND	_IO(BT_USB_IOC_MAGIC, 2)
#	define BT_USB_IOC_DISABLE_SUSPEND	_IO(BT_USB_IOC_MAGIC, 3)
#	define BT_USB_IOC_MAX_NO    (4)
#else
#	define BT_USB_IOC_MAX_NO    (2)
#endif

#ifdef __cplusplus
}
#endif

#endif

#endif
