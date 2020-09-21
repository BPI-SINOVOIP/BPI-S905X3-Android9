/*
 * hidl interface for wpa_supplicant daemon
 * Copyright (c) 2004-2018, Jouni Malinen <j@w1.fi>
 * Copyright (c) 2004-2018, Roshan Pius <rpius@google.com>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include <hwbinder/IPCThreadState.h>
#include <hidl/HidlTransportSupport.h>

#include "hostapd.h"

extern "C"
{
#include "hidl.h"
#include "utils/common.h"
#include "utils/eloop.h"
#include "utils/includes.h"
}

using android::hardware::configureRpcThreadpool;
using android::hardware::IPCThreadState;
using android::hardware::wifi::hostapd::V1_0::IHostapd;
using android::hardware::wifi::hostapd::V1_0::implementation::Hostapd;

// This file is a bridge between the hostapd code written in 'C' and the HIDL
// interface in C++. So, using "C" style static globals here!
static int hidl_fd = -1;
static android::sp<IHostapd> service;

void hostapd_hidl_sock_handler(
    int /* sock */, void * /* eloop_ctx */, void * /* sock_ctx */)
{
	IPCThreadState::self()->handlePolledCommands();
}

int hostapd_hidl_init(struct hapd_interfaces *interfaces)
{
	wpa_printf(MSG_DEBUG, "Initing hidl control");

	IPCThreadState::self()->disableBackgroundScheduling(true);
	IPCThreadState::self()->setupPolling(&hidl_fd);
	if (hidl_fd < 0)
		goto err;

	wpa_printf(MSG_INFO, "Processing hidl events on FD %d", hidl_fd);
	// Look for read events from the hidl socket in the eloop.
	if (eloop_register_read_sock(
		hidl_fd, hostapd_hidl_sock_handler, interfaces, NULL) < 0)
		goto err;
	service = new Hostapd(interfaces);
	if (!service)
		goto err;
	if (service->registerAsService() != android::NO_ERROR)
		goto err;
	return 0;
err:
	hostapd_hidl_deinit(interfaces);
	return -1;
}

void hostapd_hidl_deinit(struct hapd_interfaces *interfaces)
{
	wpa_printf(MSG_DEBUG, "Deiniting hidl control");
	eloop_unregister_read_sock(hidl_fd);
	IPCThreadState::shutdown();
	hidl_fd = -1;
	service.clear();
}
