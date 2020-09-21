/*
 * hidl interface for wpa_supplicant daemon
 * Copyright (c) 2004-2016, Jouni Malinen <j@w1.fi>
 * Copyright (c) 2004-2016, Roshan Pius <rpius@google.com>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#ifndef WPA_SUPPLICANT_HIDL_P2P_NETWORK_H
#define WPA_SUPPLICANT_HIDL_P2P_NETWORK_H

#include <android-base/macros.h>

#include <android/hardware/wifi/supplicant/1.0/ISupplicantP2pNetwork.h>
#include <android/hardware/wifi/supplicant/1.0/ISupplicantP2pNetworkCallback.h>

extern "C" {
#include "utils/common.h"
#include "utils/includes.h"
#include "wpa_supplicant_i.h"
}

namespace android {
namespace hardware {
namespace wifi {
namespace supplicant {
namespace V1_0 {
namespace implementation {

/**
 * Implementation of P2pNetwork hidl object. Each unique hidl
 * object is used for control operations on a specific network
 * controlled by wpa_supplicant.
 */
class P2pNetwork : public ISupplicantP2pNetwork
{
public:
	P2pNetwork(
	    struct wpa_global* wpa_global, const char ifname[], int network_id);
	~P2pNetwork() override = default;
	// Refer to |StaIface::invalidate()|.
	void invalidate();
	bool isValid();

	// Hidl methods exposed.
	Return<void> getId(getId_cb _hidl_cb) override;
	Return<void> getInterfaceName(getInterfaceName_cb _hidl_cb) override;
	Return<void> getType(getType_cb _hidl_cb) override;
	Return<void> registerCallback(
	    const sp<ISupplicantP2pNetworkCallback>& callback,
	    registerCallback_cb _hidl_cb) override;
	Return<void> getSsid(getSsid_cb _hidl_cb) override;
	Return<void> getBssid(getBssid_cb _hidl_cb) override;
	Return<void> isCurrent(isCurrent_cb _hidl_cb) override;
	Return<void> isPersistent(isPersistent_cb _hidl_cb) override;
	Return<void> isGo(isGo_cb _hidl_cb) override;
	Return<void> setClientList(
	    const hidl_vec<hidl_array<uint8_t, 6>>& clients,
	    setClientList_cb _hidl_cb) override;
	Return<void> getClientList(getClientList_cb _hidl_cb) override;

private:
	// Corresponding worker functions for the HIDL methods.
	std::pair<SupplicantStatus, uint32_t> getIdInternal();
	std::pair<SupplicantStatus, std::string> getInterfaceNameInternal();
	std::pair<SupplicantStatus, IfaceType> getTypeInternal();
	SupplicantStatus registerCallbackInternal(
	    const sp<ISupplicantP2pNetworkCallback>& callback);
	std::pair<SupplicantStatus, std::vector<uint8_t>> getSsidInternal();
	std::pair<SupplicantStatus, std::array<uint8_t, 6>> getBssidInternal();
	std::pair<SupplicantStatus, bool> isCurrentInternal();
	std::pair<SupplicantStatus, bool> isPersistentInternal();
	std::pair<SupplicantStatus, bool> isGoInternal();
	SupplicantStatus setClientListInternal(
	    const std::vector<hidl_array<uint8_t, 6>>& clients);
	std::pair<SupplicantStatus, std::vector<hidl_array<uint8_t, 6>>>
	getClientListInternal();

	struct wpa_ssid* retrieveNetworkPtr();
	struct wpa_supplicant* retrieveIfacePtr();

	// Reference to the global wpa_struct. This is assumed to be valid
	// for the lifetime of the process.
	const struct wpa_global* wpa_global_;
	// Name of the iface this network belongs to.
	const std::string ifname_;
	// Id of the network this hidl object controls.
	const int network_id_;
	bool is_valid_;

	DISALLOW_COPY_AND_ASSIGN(P2pNetwork);
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace wifi
}  // namespace supplicant
}  // namespace hardware
}  // namespace android

#endif  // WPA_SUPPLICANT_HIDL_P2P_NETWORK_H
