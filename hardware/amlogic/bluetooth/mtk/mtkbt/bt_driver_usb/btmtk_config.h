/*
 *  Copyright (c) 2016~2018 MediaTek Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */
#ifndef __BTMTK_CONFIG_H__
#define __BTMTK_CONFIG_H__

#include <linux/usb.h>
#include <linux/version.h>

/**
 * Kernel configuration check
 */
#ifndef CONFIG_PM
	#error "ERROR : CONFIG_PM should be turn on."
#endif

/**
 * Support IC configuration
 */
#define SUPPORT_MT7662 1
#define SUPPORT_MT7668 1
#define SUPPORT_MT7663 1

/**
 * Debug Level Configureation
 */
#define ENABLE_BT_FIFO_THREAD	0

/**
 * BTMTK LOG location, last char must be '/'
 */
/* #define BTMTK_LOG_PATH	"/data/misc/bluedroid/" */

/**
 * Fixed STPBT Major Device Id
 */
#define FIXED_STPBT_MAJOR_DEV_ID 111

/**
 * GPIO PIN configureation
 */
#define BT_DONGLE_RESET_GPIO_PIN	220

/**
 * WoBLE by BLE RC
 */
#define SUPPORT_ANDROID 0 /* Linux build fail due to wake_lock, please set SUPPORT_ANDROID 0 for Linux */
#define SUPPORT_UNIFY_WOBLE 1
#define SUPPORT_LEGACY_WOBLE 0
#define BT_RC_VENDOR_DEFAULT 1
#define BT_RC_VENDOR_S0 0

/**
 * ISOC support
 */
#define ISOC_IF_ALT_MSBC		4
#define ISOC_IF_ALT_CVSD		2
#define ISOC_IF_ALT_DEFAULT		ISOC_IF_ALT_CVSD
#define ISOC_HCI_PKT_SIZE_MSBC		(33 * 3)
#define ISOC_HCI_PKT_SIZE_CVSD		(17 * 3)
#define ISOC_HCI_PKT_SIZE_DEFAULT	ISOC_HCI_PKT_SIZE_CVSD

#endif /* __BTMTK_CONFIG_H__ */
