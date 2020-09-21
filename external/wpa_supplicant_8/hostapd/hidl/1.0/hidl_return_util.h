/*
 * hidl interface for wpa_hostapd daemon
 * Copyright (c) 2004-2018, Jouni Malinen <j@w1.fi>
 * Copyright (c) 2004-2018, Roshan Pius <rpius@google.com>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#ifndef HIDL_RETURN_UTIL_H_
#define HIDL_RETURN_UTIL_H_

#include <functional>

namespace android {
namespace hardware {
namespace wifi {
namespace hostapd {
namespace V1_0 {
namespace implementation {
namespace hidl_return_util {

/**
 * These utility functions are used to invoke a method on the provided
 * HIDL interface object.
 */
// Use for HIDL methods which return only an instance of HostapdStatus.
template <typename ObjT, typename WorkFuncT, typename... Args>
Return<void> call(
    ObjT* obj, WorkFuncT&& work,
    const std::function<void(const HostapdStatus&)>& hidl_cb, Args&&... args)
{
	hidl_cb((obj->*work)(std::forward<Args>(args)...));
	return Void();
}

}  // namespace hidl_return_util
}  // namespace implementation
}  // namespace V1_0
}  // namespace hostapd
}  // namespace wifi
}  // namespace hardware
}  // namespace android
#endif  // HIDL_RETURN_UTIL_H_
