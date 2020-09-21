/*
 * hidl interface for wpa_supplicant daemon
 * Copyright (struct wpa_supplicant* wpa_s, c) 2004-2016, Jouni Malinen
 * <j@w1.fi>
 * Copyright (struct wpa_supplicant* wpa_s, c) 2004-2016, Roshan Pius
 * <rpius@google.com>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include "hidl_manager.h"
#include "hidl_return_util.h"
#include "iface_config_utils.h"

namespace {
constexpr uint32_t kMaxWpsDeviceNameSize = WPS_DEV_NAME_MAX_LEN;
constexpr uint32_t kMaxWpsManufacturerSize = WPS_MANUFACTURER_MAX_LEN;
constexpr uint32_t kMaxWpsModelNameSize = WPS_MODEL_NAME_MAX_LEN;
constexpr uint32_t kMaxWpsModelNumberSize = WPS_MODEL_NUMBER_MAX_LEN;
constexpr uint32_t kMaxWpsSerialNumberSize = WPS_SERIAL_NUMBER_MAX_LEN;

void processConfigUpdate(struct wpa_supplicant* wpa_s, uint32_t changed_param)
{
	wpa_s->conf->changed_parameters |= changed_param;
	wpa_supplicant_update_config(wpa_s);
}

// Free any existing pointer stored in |dst| and store the provided string value
// there.
int freeAndSetStringConfigParam(
    struct wpa_supplicant* wpa_s, const std::string& value, uint32_t max_size,
    uint32_t changed_param, char** dst)
{
	if (value.size() > max_size) {
		return -1;
	}
	WPA_ASSERT(dst);
	os_free(static_cast<void*>(*dst));
	*dst = os_strdup(value.c_str());
	processConfigUpdate(wpa_s, changed_param);
	return 0;
}

std::string convertWpsConfigMethodsMaskToString(uint16_t config_methods)
{
	using WpsConfigMethods =
	    android::hardware::wifi::supplicant::V1_0::WpsConfigMethods;
	std::string config_methods_str;
	for (const auto& flag_and_name :
	     {std::make_pair(WpsConfigMethods::USBA, "usba"),
	      {WpsConfigMethods::ETHERNET, "ethernet"},
	      {WpsConfigMethods::LABEL, "label"},
	      {WpsConfigMethods::DISPLAY, "display"},
	      {WpsConfigMethods::INT_NFC_TOKEN, "int_nfc_token"},
	      {WpsConfigMethods::EXT_NFC_TOKEN, "ext_nfc_token"},
	      {WpsConfigMethods::NFC_INTERFACE, "nfc_interface"},
	      {WpsConfigMethods::PUSHBUTTON, "push_button"},
	      {WpsConfigMethods::KEYPAD, "keypad"},
	      {WpsConfigMethods::VIRT_PUSHBUTTON, "virtual_push_button"},
	      {WpsConfigMethods::PHY_PUSHBUTTON, "physical_push_button"},
	      {WpsConfigMethods::P2PS, "p2ps"},
	      {WpsConfigMethods::VIRT_DISPLAY, "virtual_display"},
	      {WpsConfigMethods::PHY_DISPLAY, "physical_display"}}) {
		const auto flag =
		    static_cast<std::underlying_type<WpsConfigMethods>::type>(
			flag_and_name.first);
		if ((config_methods & flag) == flag) {
			config_methods_str += flag_and_name.second;
			config_methods_str += " ";
		}
	}
	return config_methods_str;
}
}  // namespace

namespace android {
namespace hardware {
namespace wifi {
namespace supplicant {
namespace V1_0 {
namespace implementation {
namespace iface_config_utils {
SupplicantStatus setWpsDeviceName(
    struct wpa_supplicant* wpa_s, const std::string& name)
{
	WPA_ASSERT(wpa_s);
	if (freeAndSetStringConfigParam(
		wpa_s, name, kMaxWpsDeviceNameSize, CFG_CHANGED_DEVICE_NAME,
		&wpa_s->conf->device_name)) {
		return {SupplicantStatusCode::FAILURE_ARGS_INVALID, ""};
	}
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus setWpsDeviceType(
    struct wpa_supplicant* wpa_s, const std::array<uint8_t, 8>& type)
{
	WPA_ASSERT(wpa_s);
	WPA_ASSERT(type.size() == WPS_DEV_TYPE_LEN);
	os_memcpy(wpa_s->conf->device_type, type.data(), WPS_DEV_TYPE_LEN);
	processConfigUpdate(wpa_s, CFG_CHANGED_DEVICE_TYPE);
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus setWpsManufacturer(
    struct wpa_supplicant* wpa_s, const std::string& manufacturer)
{
	WPA_ASSERT(wpa_s);
	if (freeAndSetStringConfigParam(
		wpa_s, manufacturer, kMaxWpsManufacturerSize,
		CFG_CHANGED_WPS_STRING, &wpa_s->conf->manufacturer)) {
		return {SupplicantStatusCode::FAILURE_ARGS_INVALID, ""};
	}
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus setWpsModelName(
    struct wpa_supplicant* wpa_s, const std::string& model_name)
{
	WPA_ASSERT(wpa_s);
	if (freeAndSetStringConfigParam(
		wpa_s, model_name, kMaxWpsModelNameSize, CFG_CHANGED_WPS_STRING,
		&wpa_s->conf->model_name)) {
		return {SupplicantStatusCode::FAILURE_ARGS_INVALID, ""};
	}
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus setWpsModelNumber(
    struct wpa_supplicant* wpa_s, const std::string& model_number)
{
	WPA_ASSERT(wpa_s);
	if (freeAndSetStringConfigParam(
		wpa_s, model_number, kMaxWpsModelNumberSize,
		CFG_CHANGED_WPS_STRING, &wpa_s->conf->model_number)) {
		return {SupplicantStatusCode::FAILURE_ARGS_INVALID, ""};
	}
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus setWpsSerialNumber(
    struct wpa_supplicant* wpa_s, const std::string& serial_number)
{
	WPA_ASSERT(wpa_s);
	if (freeAndSetStringConfigParam(
		wpa_s, serial_number, kMaxWpsSerialNumberSize,
		CFG_CHANGED_WPS_STRING, &wpa_s->conf->serial_number)) {
		return {SupplicantStatusCode::FAILURE_ARGS_INVALID, ""};
	}
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus setWpsConfigMethods(
    struct wpa_supplicant* wpa_s, uint16_t config_methods)
{
	WPA_ASSERT(wpa_s);
	if (freeAndSetStringConfigParam(
		wpa_s, convertWpsConfigMethodsMaskToString(config_methods),
		UINT32_MAX, CFG_CHANGED_CONFIG_METHODS,
		&wpa_s->conf->config_methods)) {
		return {SupplicantStatusCode::FAILURE_ARGS_INVALID, ""};
	}
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus setExternalSim(
    struct wpa_supplicant* wpa_s, bool useExternalSim)
{
	WPA_ASSERT(wpa_s);
	wpa_s->conf->external_sim = useExternalSim ? 1 : 0;
	return {SupplicantStatusCode::SUCCESS, ""};
}
}  // namespace iface_config_utils
}  // namespace implementation
}  // namespace V1_0
}  // namespace wifi
}  // namespace supplicant
}  // namespace hardware
}  // namespace android
