/*
 * hidl interface for wpa_supplicant daemon
 * Copyright (c) 2004-2016, Jouni Malinen <j@w1.fi>
 * Copyright (c) 2004-2016, Roshan Pius <rpius@google.com>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#ifndef HIDL_RETURN_UTIL_H_
#define HIDL_RETURN_UTIL_H_

namespace android {
namespace hardware {
namespace wifi {
namespace supplicant {
namespace V1_0 {
namespace implementation {
namespace hidl_return_util {

/**
 * These utility functions are used to invoke a method on the provided
 * HIDL interface object.
 * These functions checks if the provided HIDL interface object is valid.
 * a) if valid, Invokes the corresponding internal implementation function of
 * the HIDL method. It then invokes the HIDL continuation callback with
 * the status and any returned values.
 * b) if invalid, invokes the HIDL continuation callback with the
 * provided error status and default values.
 */
// Use for HIDL methods which return only an instance of SupplicantStatus.
template <typename ObjT, typename WorkFuncT, typename... Args>
Return<void> validateAndCall(
    ObjT* obj, SupplicantStatusCode status_code_if_invalid, WorkFuncT&& work,
    const std::function<void(const SupplicantStatus&)>& hidl_cb, Args&&... args)
{
	if (obj->isValid()) {
		hidl_cb((obj->*work)(std::forward<Args>(args)...));
	} else {
		hidl_cb({status_code_if_invalid, ""});
	}
	return Void();
}

// Use for HIDL methods which return instance of SupplicantStatus and a single
// return value.
template <typename ObjT, typename WorkFuncT, typename ReturnT, typename... Args>
Return<void> validateAndCall(
    ObjT* obj, SupplicantStatusCode status_code_if_invalid, WorkFuncT&& work,
    const std::function<void(const SupplicantStatus&, ReturnT)>& hidl_cb,
    Args&&... args)
{
	if (obj->isValid()) {
		const auto& ret_pair =
		    (obj->*work)(std::forward<Args>(args)...);
		const SupplicantStatus& status = std::get<0>(ret_pair);
		const auto& ret_value = std::get<1>(ret_pair);
		hidl_cb(status, ret_value);
	} else {
		hidl_cb(
		    {status_code_if_invalid, ""},
		    typename std::remove_reference<ReturnT>::type());
	}
	return Void();
}

// Use for HIDL methods which return instance of SupplicantStatus and 2 return
// values.
template <
    typename ObjT, typename WorkFuncT, typename ReturnT1, typename ReturnT2,
    typename... Args>
Return<void> validateAndCall(
    ObjT* obj, SupplicantStatusCode status_code_if_invalid, WorkFuncT&& work,
    const std::function<void(const SupplicantStatus&, ReturnT1, ReturnT2)>&
	hidl_cb,
    Args&&... args)
{
	if (obj->isValid()) {
		const auto& ret_tuple =
		    (obj->*work)(std::forward<Args>(args)...);
		const SupplicantStatus& status = std::get<0>(ret_tuple);
		const auto& ret_value1 = std::get<1>(ret_tuple);
		const auto& ret_value2 = std::get<2>(ret_tuple);
		hidl_cb(status, ret_value1, ret_value2);
	} else {
		hidl_cb(
		    {status_code_if_invalid, ""},
		    typename std::remove_reference<ReturnT1>::type(),
		    typename std::remove_reference<ReturnT2>::type());
	}
	return Void();
}

}  // namespace hidl_util
}  // namespace implementation
}  // namespace V1_0
}  // namespace supplicant
}  // namespace wifi
}  // namespace hardware
}  // namespace android
#endif  // HIDL_RETURN_UTIL_H_
