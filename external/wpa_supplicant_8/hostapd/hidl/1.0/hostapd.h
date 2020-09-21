/*
 * hidl interface for wpa_hostapd daemon
 * Copyright (c) 2004-2018, Jouni Malinen <j@w1.fi>
 * Copyright (c) 2004-2018, Roshan Pius <rpius@google.com>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#ifndef HOSTAPD_HIDL_SUPPLICANT_H
#define HOSTAPD_HIDL_SUPPLICANT_H

#include <string>

#include <android-base/macros.h>

#include <android/hardware/wifi/hostapd/1.0/IHostapd.h>

extern "C"
{
#include "utils/common.h"
#include "utils/includes.h"
#include "utils/wpa_debug.h"
#include "ap/hostapd.h"
}

namespace android {
namespace hardware {
namespace wifi {
namespace hostapd {
namespace V1_0 {
namespace implementation {

/**
 * Implementation of the hostapd hidl object. This hidl
 * object is used core for global control operations on
 * hostapd.
 */
class Hostapd : public V1_0::IHostapd
{
public:
	Hostapd(hapd_interfaces* interfaces);
	~Hostapd() override = default;

	// Hidl methods exposed.
	Return<void> addAccessPoint(
	    const IfaceParams& iface_params, const NetworkParams& nw_params,
	    addAccessPoint_cb _hidl_cb) override;
	Return<void> removeAccessPoint(
	    const hidl_string& iface_name,
	    removeAccessPoint_cb _hidl_cb) override;
	Return<void> terminate() override;

private:
	// Corresponding worker functions for the HIDL methods.
	HostapdStatus addAccessPointInternal(
	    const IfaceParams& iface_params, const NetworkParams& nw_params);
	HostapdStatus removeAccessPointInternal(const std::string& iface_name);

	// Raw pointer to the global structure maintained by the core.
	struct hapd_interfaces* interfaces_;

	DISALLOW_COPY_AND_ASSIGN(Hostapd);
};
}  // namespace implementation
}  // namespace V1_0
}  // namespace hostapd
}  // namespace wifi
}  // namespace hardware
}  // namespace android

#endif  // HOSTAPD_HIDL_SUPPLICANT_H
