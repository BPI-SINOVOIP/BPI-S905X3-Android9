/*
 * hidl interface for wpa_supplicant daemon
 * Copyright (c) 2004-2016, Jouni Malinen <j@w1.fi>
 * Copyright (c) 2004-2016, Roshan Pius <rpius@google.com>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#ifndef HIDL_I_H
#define HIDL_I_H

#ifdef _cplusplus
extern "C" {
#endif  // _cplusplus

struct wpas_hidl_priv
{
	int hidl_fd;
	struct wpa_global *global;
	void *hidl_manager;
};

#ifdef _cplusplus
}
#endif  // _cplusplus

#endif  // HIDL_I_H
