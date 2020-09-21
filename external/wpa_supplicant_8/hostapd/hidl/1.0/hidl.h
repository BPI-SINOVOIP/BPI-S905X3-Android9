/*
 * hidl interface for wpa_supplicant daemon
 * Copyright (c) 2004-2018, Jouni Malinen <j@w1.fi>
 * Copyright (c) 2004-2018, Roshan Pius <rpius@google.com>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#ifndef HOSTAPD_HIDL_HIDL_H
#define HOSTAPD_HIDL_HIDL_H

#ifdef __cplusplus
extern "C"
{
#endif  // _cplusplus
#include "ap/hostapd.h"

/**
 * This is the hidl RPC interface entry point to the hostapd core.
 * This initializes the hidl driver & IHostapd instance.
 */
int hostapd_hidl_init(struct hapd_interfaces *interfaces);
void hostapd_hidl_deinit(struct hapd_interfaces *interfaces);

#ifdef __cplusplus
}
#endif  // _cplusplus

#endif  // HOSTAPD_HIDL_HIDL_H
