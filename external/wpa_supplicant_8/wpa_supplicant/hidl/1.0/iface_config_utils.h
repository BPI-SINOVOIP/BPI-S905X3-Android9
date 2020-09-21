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

#ifndef WPA_SUPPLICANT_HIDL_IFACE_CONFIG_UTILS_H
#define WPA_SUPPLICANT_HIDL_IFACE_CONFIG_UTILS_H

#include <android-base/macros.h>

extern "C" {
#include "utils/common.h"
#include "utils/includes.h"
#include "wpa_supplicant_i.h"
#include "config.h"
}

/**
 * Utility functions to set various config parameters of an iface via HIDL
 * methods.
 */
namespace android {
namespace hardware {
namespace wifi {
namespace supplicant {
namespace V1_0 {
namespace implementation {
namespace iface_config_utils {
SupplicantStatus setWpsDeviceName(
    struct wpa_supplicant* wpa_s, const std::string& name);
SupplicantStatus setWpsDeviceType(
    struct wpa_supplicant* wpa_s, const std::array<uint8_t, 8>& type);
SupplicantStatus setWpsManufacturer(
    struct wpa_supplicant* wpa_s, const std::string& manufacturer);
SupplicantStatus setWpsModelName(
    struct wpa_supplicant* wpa_s, const std::string& model_name);
SupplicantStatus setWpsModelNumber(
    struct wpa_supplicant* wpa_s, const std::string& model_number);
SupplicantStatus setWpsSerialNumber(
    struct wpa_supplicant* wpa_s, const std::string& serial_number);
SupplicantStatus setWpsConfigMethods(
    struct wpa_supplicant* wpa_s, uint16_t config_methods);
SupplicantStatus setExternalSim(
    struct wpa_supplicant* wpa_s, bool useExternalSim);
}  // namespace iface_config_utils
}  // namespace implementation
}  // namespace V1_0
}  // namespace wifi
}  // namespace supplicant
}  // namespace hardware
}  // namespace android

#endif  // WPA_SUPPLICANT_HIDL_IFACE_CONFIG_UTILS_H
