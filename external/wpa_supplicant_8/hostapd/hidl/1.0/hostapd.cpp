/*
 * hidl interface for wpa_hostapd daemon
 * Copyright (c) 2004-2018, Jouni Malinen <j@w1.fi>
 * Copyright (c) 2004-2018, Roshan Pius <rpius@google.com>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include <android-base/file.h>
#include <android-base/stringprintf.h>

#include "hostapd.h"
#include "hidl_return_util.h"

extern "C"
{
#include "utils/eloop.h"
}

// The HIDL implementation for hostapd creates a hostapd.conf dynamically for
// each interface. This file can then be used to hook onto the normal config
// file parsing logic in hostapd code.  Helps us to avoid duplication of code
// in the HIDL interface.
// TOOD(b/71872409): Add unit tests for this.
namespace {
constexpr char kConfFileNameFmt[] = "/data/vendor/wifi/hostapd/hostapd_%s.conf";

using android::base::RemoveFileIfExists;
using android::base::StringPrintf;
using android::base::WriteStringToFile;
using android::hardware::wifi::hostapd::V1_0::IHostapd;

std::string WriteHostapdConfig(
    const std::string& interface_name, const std::string& config)
{
	const std::string file_path =
	    StringPrintf(kConfFileNameFmt, interface_name.c_str());
	if (WriteStringToFile(
		config, file_path, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP,
		getuid(), getgid())) {
		return file_path;
	}
	// Diagnose failure
	int error = errno;
	wpa_printf(
	    MSG_ERROR, "Cannot write hostapd config to %s, error: %s",
	    file_path.c_str(), strerror(error));
	struct stat st;
	int result = stat(file_path.c_str(), &st);
	if (result == 0) {
		wpa_printf(
		    MSG_ERROR, "hostapd config file uid: %d, gid: %d, mode: %d",
		    st.st_uid, st.st_gid, st.st_mode);
	} else {
		wpa_printf(
		    MSG_ERROR,
		    "Error calling stat() on hostapd config file: %s",
		    strerror(errno));
	}
	return "";
}

std::string CreateHostapdConfig(
    const IHostapd::IfaceParams& iface_params,
    const IHostapd::NetworkParams& nw_params)
{
	if (nw_params.ssid.size() >
	    static_cast<uint32_t>(
		IHostapd::ParamSizeLimits::SSID_MAX_LEN_IN_BYTES)) {
		wpa_printf(
		    MSG_ERROR, "Invalid SSID size: %zu", nw_params.ssid.size());
		return "";
	}
	if ((nw_params.encryptionType != IHostapd::EncryptionType::NONE) &&
	    (nw_params.pskPassphrase.size() <
		 static_cast<uint32_t>(
		     IHostapd::ParamSizeLimits::
			 WPA2_PSK_PASSPHRASE_MIN_LEN_IN_BYTES) ||
	     nw_params.pskPassphrase.size() >
		 static_cast<uint32_t>(
		     IHostapd::ParamSizeLimits::
			 WPA2_PSK_PASSPHRASE_MAX_LEN_IN_BYTES))) {
		wpa_printf(
		    MSG_ERROR, "Invalid psk passphrase size: %zu",
		    nw_params.pskPassphrase.size());
		return "";
	}

	// SSID string
	std::stringstream ss;
	ss << std::hex;
	ss << std::setfill('0');
	for (uint8_t b : nw_params.ssid) {
		ss << std::setw(2) << static_cast<unsigned int>(b);
	}
	const std::string ssid_as_string = ss.str();

	// Encryption config string
	std::string encryption_config_as_string;
	switch (nw_params.encryptionType) {
	case IHostapd::EncryptionType::NONE:
		// no security params
		break;
	case IHostapd::EncryptionType::WPA:
		encryption_config_as_string = StringPrintf(
		    "wpa=3\n"
		    "wpa_pairwise=TKIP CCMP\n"
		    "wpa_passphrase=%s",
		    nw_params.pskPassphrase.c_str());
		break;
	case IHostapd::EncryptionType::WPA2:
		encryption_config_as_string = StringPrintf(
		    "wpa=2\n"
		    "rsn_pairwise=CCMP\n"
		    "wpa_passphrase=%s",
		    nw_params.pskPassphrase.c_str());
		break;
	default:
		wpa_printf(MSG_ERROR, "Unknown encryption type");
		return "";
	}

	std::string channel_config_as_string;
	if (iface_params.channelParams.enableAcs) {
		channel_config_as_string = StringPrintf(
		    "channel=0\n"
		    "acs_exclude_dfs=%d",
		    iface_params.channelParams.acsShouldExcludeDfs);
	} else {
		channel_config_as_string = StringPrintf(
		    "channel=%d", iface_params.channelParams.channel);
	}

	// Hw Mode String
	std::string hw_mode_as_string;
	std::string ht_cap_vht_oper_chwidth_as_string;
	switch (iface_params.channelParams.band) {
	case IHostapd::Band::BAND_2_4_GHZ:
		hw_mode_as_string = "hw_mode=g";
		break;
	case IHostapd::Band::BAND_5_GHZ:
		hw_mode_as_string = "hw_mode=a";
		if (iface_params.channelParams.enableAcs) {
			ht_cap_vht_oper_chwidth_as_string =
			    "ht_capab=[HT40+]\n"
			    "vht_oper_chwidth=1";
		}
		break;
	case IHostapd::Band::BAND_ANY:
		hw_mode_as_string = "hw_mode=any";
		if (iface_params.channelParams.enableAcs) {
			ht_cap_vht_oper_chwidth_as_string =
			    "ht_capab=[HT40+]\n"
			    "vht_oper_chwidth=1";
		}
		break;
	default:
		wpa_printf(MSG_ERROR, "Invalid band");
		return "";
	}

	return StringPrintf(
	    "interface=%s\n"
	    "driver=nl80211\n"
	    "ctrl_interface=/data/vendor/wifi/hostapd/ctrl\n"
	    // ssid2 signals to hostapd that the value is not a literal value
	    // for use as a SSID.  In this case, we're giving it a hex
	    // std::string and hostapd needs to expect that.
	    "ssid2=%s\n"
	    "%s\n"
	    "ieee80211n=%d\n"
	    "ieee80211ac=%d\n"
	    "%s\n"
	    "%s\n"
	    "ignore_broadcast_ssid=%d\n"
	    "wowlan_triggers=any\n"
	    "%s\n",
	    iface_params.ifaceName.c_str(), ssid_as_string.c_str(),
	    channel_config_as_string.c_str(),
	    iface_params.hwModeParams.enable80211N ? 1 : 0,
	    iface_params.hwModeParams.enable80211AC ? 1 : 0,
	    hw_mode_as_string.c_str(), ht_cap_vht_oper_chwidth_as_string.c_str(),
	    nw_params.isHidden ? 1 : 0, encryption_config_as_string.c_str());
}
}  // namespace

namespace android {
namespace hardware {
namespace wifi {
namespace hostapd {
namespace V1_0 {
namespace implementation {
using hidl_return_util::call;

Hostapd::Hostapd(struct hapd_interfaces* interfaces) : interfaces_(interfaces)
{}

Return<void> Hostapd::addAccessPoint(
    const IfaceParams& iface_params, const NetworkParams& nw_params,
    addAccessPoint_cb _hidl_cb)
{
	return call(
	    this, &Hostapd::addAccessPointInternal, _hidl_cb, iface_params,
	    nw_params);
}

Return<void> Hostapd::removeAccessPoint(
    const hidl_string& iface_name, removeAccessPoint_cb _hidl_cb)
{
	return call(
	    this, &Hostapd::removeAccessPointInternal, _hidl_cb, iface_name);
}

Return<void> Hostapd::terminate() {
	wpa_printf(MSG_INFO, "Terminating...");
	eloop_terminate();
	return Void();
}

HostapdStatus Hostapd::addAccessPointInternal(
    const IfaceParams& iface_params, const NetworkParams& nw_params)
{
	if (hostapd_get_iface(interfaces_, iface_params.ifaceName.c_str())) {
		wpa_printf(
		    MSG_ERROR, "Interface %s already present",
		    iface_params.ifaceName.c_str());
		return {HostapdStatusCode::FAILURE_IFACE_EXISTS, ""};
	}
	const auto conf_params = CreateHostapdConfig(iface_params, nw_params);
	if (conf_params.empty()) {
		wpa_printf(MSG_ERROR, "Failed to create config params");
		return {HostapdStatusCode::FAILURE_ARGS_INVALID, ""};
	}
	const auto conf_file_path =
	    WriteHostapdConfig(iface_params.ifaceName, conf_params);
	if (conf_file_path.empty()) {
		wpa_printf(MSG_ERROR, "Failed to write config file");
		return {HostapdStatusCode::FAILURE_UNKNOWN, ""};
	}
	std::string add_iface_param_str = StringPrintf(
	    "%s config=%s", iface_params.ifaceName.c_str(),
	    conf_file_path.c_str());
	std::vector<char> add_iface_param_vec(
	    add_iface_param_str.begin(), add_iface_param_str.end() + 1);
	if (hostapd_add_iface(interfaces_, add_iface_param_vec.data()) < 0) {
		wpa_printf(
		    MSG_ERROR, "Adding interface %s failed",
		    add_iface_param_str.c_str());
		return {HostapdStatusCode::FAILURE_UNKNOWN, ""};
	}
	struct hostapd_data* iface_hapd =
	    hostapd_get_iface(interfaces_, iface_params.ifaceName.c_str());
	WPA_ASSERT(iface_hapd != nullptr && iface_hapd->iface != nullptr);
	if (hostapd_enable_iface(iface_hapd->iface) < 0) {
		wpa_printf(
		    MSG_ERROR, "Enabling interface %s failed",
		    iface_params.ifaceName.c_str());
		return {HostapdStatusCode::FAILURE_UNKNOWN, ""};
	}
	return {HostapdStatusCode::SUCCESS, ""};
}

HostapdStatus Hostapd::removeAccessPointInternal(const std::string& iface_name)
{
	std::vector<char> remove_iface_param_vec(
	    iface_name.begin(), iface_name.end() + 1);
	if (hostapd_remove_iface(interfaces_, remove_iface_param_vec.data()) <
	    0) {
		wpa_printf(
		    MSG_ERROR, "Removing interface %s failed",
		    iface_name.c_str());
		return {HostapdStatusCode::FAILURE_UNKNOWN, ""};
	}
	return {HostapdStatusCode::SUCCESS, ""};
}
}  // namespace implementation
}  // namespace V1_0
}  // namespace hostapd
}  // namespace wifi
}  // namespace hardware
}  // namespace android
