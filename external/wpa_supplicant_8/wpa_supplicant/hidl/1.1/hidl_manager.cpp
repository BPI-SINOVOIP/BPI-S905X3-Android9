/*
 * hidl interface for wpa_supplicant daemon
 * Copyright (c) 2004-2016, Jouni Malinen <j@w1.fi>
 * Copyright (c) 2004-2016, Roshan Pius <rpius@google.com>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include <algorithm>
#include <regex>

#include "hidl_manager.h"
#include "misc_utils.h"

extern "C" {
#include "src/eap_common/eap_sim_common.h"
}

namespace {
using android::hardware::hidl_array;

constexpr uint8_t kWfdDeviceInfoLen = 6;
// GSM-AUTH:<RAND1>:<RAND2>[:<RAND3>]
constexpr char kGsmAuthRegex2[] = "GSM-AUTH:([0-9a-f]+):([0-9a-f]+)";
constexpr char kGsmAuthRegex3[] =
    "GSM-AUTH:([0-9a-f]+):([0-9a-f]+):([0-9a-f]+)";
// UMTS-AUTH:<RAND>:<AUTN>
constexpr char kUmtsAuthRegex[] = "UMTS-AUTH:([0-9a-f]+):([0-9a-f]+)";
constexpr size_t kGsmRandLenBytes = GSM_RAND_LEN;
constexpr size_t kUmtsRandLenBytes = EAP_AKA_RAND_LEN;
constexpr size_t kUmtsAutnLenBytes = EAP_AKA_AUTN_LEN;
constexpr u8 kZeroBssid[6] = {0, 0, 0, 0, 0, 0};
/**
 * Check if the provided |wpa_supplicant| structure represents a P2P iface or
 * not.
 */
constexpr bool isP2pIface(const struct wpa_supplicant *wpa_s)
{
	return (wpa_s->global->p2p_init_wpa_s == wpa_s);
}

/**
 * Creates a unique key for the network using the provided |ifname| and
 * |network_id| to be used in the internal map of |ISupplicantNetwork| objects.
 * This is of the form |ifname|_|network_id|. For ex: "wlan0_1".
 *
 * @param ifname Name of the corresponding interface.
 * @param network_id ID of the corresponding network.
 */
const std::string getNetworkObjectMapKey(
    const std::string &ifname, int network_id)
{
	return ifname + "_" + std::to_string(network_id);
}

/**
 * Add callback to the corresponding list after linking to death on the
 * corresponding hidl object reference.
 */
template <class CallbackType>
int registerForDeathAndAddCallbackHidlObjectToList(
    const android::sp<CallbackType> &callback,
    const std::function<void(const android::sp<CallbackType> &)>
	&on_hidl_died_fctor,
    std::vector<android::sp<CallbackType>> &callback_list)
{
#if 0   // TODO(b/31632518): HIDL object death notifications.
	auto death_notifier = new CallbackObjectDeathNotifier<CallbackType>(
	    callback, on_hidl_died_fctor);
	// Use the |callback.get()| as cookie so that we don't need to
	// store a reference to this |CallbackObjectDeathNotifier| instance
	// to use in |unlinkToDeath| later.
	// NOTE: This may cause an immediate callback if the object is already
	// dead, so add it to the list before we register for callback!
	if (android::hardware::IInterface::asBinder(callback)->linkToDeath(
		death_notifier, callback.get()) != android::OK) {
		wpa_printf(
		    MSG_ERROR,
		    "Error registering for death notification for "
		    "supplicant callback object");
		callback_list.erase(
		    std::remove(
			callback_list.begin(), callback_list.end(), callback),
		    callback_list.end());
		return 1;
	}
#endif  // TODO(b/31632518): HIDL object death notifications.
	callback_list.push_back(callback);
	return 0;
}

template <class ObjectType>
int addHidlObjectToMap(
    const std::string &key, const android::sp<ObjectType> object,
    std::map<const std::string, android::sp<ObjectType>> &object_map)
{
	// Return failure if we already have an object for that |key|.
	if (object_map.find(key) != object_map.end())
		return 1;
	object_map[key] = object;
	if (!object_map[key].get())
		return 1;
	return 0;
}

template <class ObjectType>
int removeHidlObjectFromMap(
    const std::string &key,
    std::map<const std::string, android::sp<ObjectType>> &object_map)
{
	// Return failure if we dont have an object for that |key|.
	const auto &object_iter = object_map.find(key);
	if (object_iter == object_map.end())
		return 1;
	object_iter->second->invalidate();
	object_map.erase(object_iter);
	return 0;
}

template <class CallbackType>
int addIfaceCallbackHidlObjectToMap(
    const std::string &ifname, const android::sp<CallbackType> &callback,
    const std::function<void(const android::sp<CallbackType> &)>
	&on_hidl_died_fctor,
    std::map<const std::string, std::vector<android::sp<CallbackType>>>
	&callbacks_map)
{
	if (ifname.empty())
		return 1;

	auto iface_callback_map_iter = callbacks_map.find(ifname);
	if (iface_callback_map_iter == callbacks_map.end())
		return 1;
	auto &iface_callback_list = iface_callback_map_iter->second;

	// Register for death notification before we add it to our list.
	return registerForDeathAndAddCallbackHidlObjectToList<CallbackType>(
	    callback, on_hidl_died_fctor, iface_callback_list);
}

template <class CallbackType>
int addNetworkCallbackHidlObjectToMap(
    const std::string &ifname, int network_id,
    const android::sp<CallbackType> &callback,
    const std::function<void(const android::sp<CallbackType> &)>
	&on_hidl_died_fctor,
    std::map<const std::string, std::vector<android::sp<CallbackType>>>
	&callbacks_map)
{
	if (ifname.empty() || network_id < 0)
		return 1;

	// Generate the key to be used to lookup the network.
	const std::string network_key =
	    getNetworkObjectMapKey(ifname, network_id);
	auto network_callback_map_iter = callbacks_map.find(network_key);
	if (network_callback_map_iter == callbacks_map.end())
		return 1;
	auto &network_callback_list = network_callback_map_iter->second;

	// Register for death notification before we add it to our list.
	return registerForDeathAndAddCallbackHidlObjectToList<CallbackType>(
	    callback, on_hidl_died_fctor, network_callback_list);
}

template <class CallbackType>
int removeAllIfaceCallbackHidlObjectsFromMap(
    const std::string &ifname,
    std::map<const std::string, std::vector<android::sp<CallbackType>>>
	&callbacks_map)
{
	auto iface_callback_map_iter = callbacks_map.find(ifname);
	if (iface_callback_map_iter == callbacks_map.end())
		return 1;
#if 0   // TODO(b/31632518): HIDL object death notifications.
	const auto &iface_callback_list = iface_callback_map_iter->second;
	for (const auto &callback : iface_callback_list) {
		if (android::hardware::IInterface::asBinder(callback)
			->unlinkToDeath(nullptr, callback.get()) !=
		    android::OK) {
			wpa_printf(
			    MSG_ERROR,
			    "Error deregistering for death notification for "
			    "iface callback object");
		}
	}
#endif  // TODO(b/31632518): HIDL object death notifications.
	callbacks_map.erase(iface_callback_map_iter);
	return 0;
}

template <class CallbackType>
int removeAllNetworkCallbackHidlObjectsFromMap(
    const std::string &network_key,
    std::map<const std::string, std::vector<android::sp<CallbackType>>>
	&callbacks_map)
{
	auto network_callback_map_iter = callbacks_map.find(network_key);
	if (network_callback_map_iter == callbacks_map.end())
		return 1;
#if 0   // TODO(b/31632518): HIDL object death notifications.
	const auto &network_callback_list = network_callback_map_iter->second;
	for (const auto &callback : network_callback_list) {
		if (android::hardware::IInterface::asBinder(callback)
			->unlinkToDeath(nullptr, callback.get()) !=
		    android::OK) {
			wpa_printf(
			    MSG_ERROR,
			    "Error deregistering for death "
			    "notification for "
			    "network callback object");
		}
	}
#endif  // TODO(b/31632518): HIDL object death notifications.
	callbacks_map.erase(network_callback_map_iter);
	return 0;
}

template <class CallbackType>
void removeIfaceCallbackHidlObjectFromMap(
    const std::string &ifname, const android::sp<CallbackType> &callback,
    std::map<const std::string, std::vector<android::sp<CallbackType>>>
	&callbacks_map)
{
	if (ifname.empty())
		return;

	auto iface_callback_map_iter = callbacks_map.find(ifname);
	if (iface_callback_map_iter == callbacks_map.end())
		return;

	auto &iface_callback_list = iface_callback_map_iter->second;
	iface_callback_list.erase(
	    std::remove(
		iface_callback_list.begin(), iface_callback_list.end(),
		callback),
	    iface_callback_list.end());
}

template <class CallbackType>
void removeNetworkCallbackHidlObjectFromMap(
    const std::string &ifname, int network_id,
    const android::sp<CallbackType> &callback,
    std::map<const std::string, std::vector<android::sp<CallbackType>>>
	&callbacks_map)
{
	if (ifname.empty() || network_id < 0)
		return;

	// Generate the key to be used to lookup the network.
	const std::string network_key =
	    getNetworkObjectMapKey(ifname, network_id);

	auto network_callback_map_iter = callbacks_map.find(network_key);
	if (network_callback_map_iter == callbacks_map.end())
		return;

	auto &network_callback_list = network_callback_map_iter->second;
	network_callback_list.erase(
	    std::remove(
		network_callback_list.begin(), network_callback_list.end(),
		callback),
	    network_callback_list.end());
}

template <class CallbackType>
void callWithEachIfaceCallback(
    const std::string &ifname,
    const std::function<
	android::hardware::Return<void>(android::sp<CallbackType>)> &method,
    const std::map<const std::string, std::vector<android::sp<CallbackType>>>
	&callbacks_map)
{
	if (ifname.empty())
		return;

	auto iface_callback_map_iter = callbacks_map.find(ifname);
	if (iface_callback_map_iter == callbacks_map.end())
		return;
	const auto &iface_callback_list = iface_callback_map_iter->second;
	for (const auto &callback : iface_callback_list) {
		if (!method(callback).isOk()) {
			wpa_printf(
			    MSG_ERROR, "Failed to invoke HIDL iface callback");
		}
	}
}

template <class CallbackTypeV1_0, class CallbackTypeV1_1>
void callWithEachIfaceCallback_1_1(
    const std::string &ifname,
    const std::function<
	android::hardware::Return<void>(android::sp<CallbackTypeV1_1>)> &method,
    const std::map<const std::string, std::vector<android::sp<CallbackTypeV1_0>>>
	&callbacks_map)
{
	if (ifname.empty())
		return;

	auto iface_callback_map_iter = callbacks_map.find(ifname);
	if (iface_callback_map_iter == callbacks_map.end())
		return;
	const auto &iface_callback_list = iface_callback_map_iter->second;
	for (const auto &callback : iface_callback_list) {
		android::sp<CallbackTypeV1_1> callback_1_1 =
		    CallbackTypeV1_1::castFrom(callback);
		if (callback_1_1 == nullptr)
			continue;

		if (!method(callback_1_1).isOk()) {
			wpa_printf(
			    MSG_ERROR, "Failed to invoke HIDL iface callback");
		}
	}
}

template <class CallbackType>
void callWithEachNetworkCallback(
    const std::string &ifname, int network_id,
    const std::function<
	android::hardware::Return<void>(android::sp<CallbackType>)> &method,
    const std::map<const std::string, std::vector<android::sp<CallbackType>>>
	&callbacks_map)
{
	if (ifname.empty() || network_id < 0)
		return;

	// Generate the key to be used to lookup the network.
	const std::string network_key =
	    getNetworkObjectMapKey(ifname, network_id);
	auto network_callback_map_iter = callbacks_map.find(network_key);
	if (network_callback_map_iter == callbacks_map.end())
		return;
	const auto &network_callback_list = network_callback_map_iter->second;
	for (const auto &callback : network_callback_list) {
		if (!method(callback).isOk()) {
			wpa_printf(
			    MSG_ERROR,
			    "Failed to invoke HIDL network callback");
		}
	}
}

int parseGsmAuthNetworkRequest(
    const std::string &params_str,
    std::vector<hidl_array<uint8_t, kGsmRandLenBytes>> *out_rands)
{
	std::smatch matches;
	std::regex params_gsm_regex2(kGsmAuthRegex2);
	std::regex params_gsm_regex3(kGsmAuthRegex3);
	if (!std::regex_match(params_str, matches, params_gsm_regex3) &&
	    !std::regex_match(params_str, matches, params_gsm_regex2)) {
		return 1;
	}
	for (uint32_t i = 1; i < matches.size(); i++) {
		hidl_array<uint8_t, kGsmRandLenBytes> rand;
		const auto &match = matches[i];
		WPA_ASSERT(match.size() >= 2 * rand.size());
		if (hexstr2bin(match.str().c_str(), rand.data(), rand.size())) {
			wpa_printf(
			    MSG_ERROR, "Failed to parse GSM auth params");
			return 1;
		}
		out_rands->push_back(rand);
	}
	return 0;
}

int parseUmtsAuthNetworkRequest(
    const std::string &params_str,
    hidl_array<uint8_t, kUmtsRandLenBytes> *out_rand,
    hidl_array<uint8_t, kUmtsAutnLenBytes> *out_autn)
{
	std::smatch matches;
	std::regex params_umts_regex(kUmtsAuthRegex);
	if (!std::regex_match(params_str, matches, params_umts_regex)) {
		return 1;
	}
	WPA_ASSERT(matches[1].size() >= 2 * out_rand->size());
	if (hexstr2bin(
		matches[1].str().c_str(), out_rand->data(), out_rand->size())) {
		wpa_printf(MSG_ERROR, "Failed to parse UMTS auth params");
		return 1;
	}
	WPA_ASSERT(matches[2].size() >= 2 * out_autn->size());
	if (hexstr2bin(
		matches[2].str().c_str(), out_autn->data(), out_autn->size())) {
		wpa_printf(MSG_ERROR, "Failed to parse UMTS auth params");
		return 1;
	}
	return 0;
}
}  // namespace

namespace android {
namespace hardware {
namespace wifi {
namespace supplicant {
namespace V1_1 {
namespace implementation {

using namespace android::hardware::wifi::supplicant::V1_0;
using namespace android::hardware::wifi::supplicant::V1_1;
using V1_0::ISupplicantStaIfaceCallback;

HidlManager *HidlManager::instance_ = NULL;

HidlManager *HidlManager::getInstance()
{
	if (!instance_)
		instance_ = new HidlManager();
	return instance_;
}

void HidlManager::destroyInstance()
{
	if (instance_)
		delete instance_;
	instance_ = NULL;
}

int HidlManager::registerHidlService(struct wpa_global *global)
{
	// Create the main hidl service object and register it.
	supplicant_object_ = new Supplicant(global);
	if (supplicant_object_->registerAsService() != android::NO_ERROR) {
		return 1;
	}
	return 0;
}

/**
 * Register an interface to hidl manager.
 *
 * @param wpa_s |wpa_supplicant| struct corresponding to the interface.
 *
 * @return 0 on success, 1 on failure.
 */
int HidlManager::registerInterface(struct wpa_supplicant *wpa_s)
{
	if (!wpa_s)
		return 1;

	if (isP2pIface(wpa_s)) {
		if (addHidlObjectToMap<P2pIface>(
			wpa_s->ifname,
			new P2pIface(wpa_s->global, wpa_s->ifname),
			p2p_iface_object_map_)) {
			wpa_printf(
			    MSG_ERROR,
			    "Failed to register P2P interface with HIDL "
			    "control: %s",
			    wpa_s->ifname);
			return 1;
		}
		p2p_iface_callbacks_map_[wpa_s->ifname] =
		    std::vector<android::sp<ISupplicantP2pIfaceCallback>>();
	} else {
		if (addHidlObjectToMap<StaIface>(
			wpa_s->ifname,
			new StaIface(wpa_s->global, wpa_s->ifname),
			sta_iface_object_map_)) {
			wpa_printf(
			    MSG_ERROR,
			    "Failed to register STA interface with HIDL "
			    "control: %s",
			    wpa_s->ifname);
			return 1;
		}
		sta_iface_callbacks_map_[wpa_s->ifname] =
		    std::vector<android::sp<ISupplicantStaIfaceCallback>>();
	}

	// Invoke the |onInterfaceCreated| method on all registered callbacks.
	callWithEachSupplicantCallback(std::bind(
	    &ISupplicantCallback::onInterfaceCreated, std::placeholders::_1,
	    wpa_s->ifname));
	return 0;
}

/**
 * Unregister an interface from hidl manager.
 *
 * @param wpa_s |wpa_supplicant| struct corresponding to the interface.
 *
 * @return 0 on success, 1 on failure.
 */
int HidlManager::unregisterInterface(struct wpa_supplicant *wpa_s)
{
	if (!wpa_s)
		return 1;

	// Check if this interface is present in P2P map first, else check in
	// STA map.
	// Note: We can't use isP2pIface() here because interface
	// pointers (wpa_s->global->p2p_init_wpa_s == wpa_s) used by the helper
	// function is cleared by the core before notifying the HIDL interface.
	bool success =
	    !removeHidlObjectFromMap(wpa_s->ifname, p2p_iface_object_map_);
	if (success) {  // assumed to be P2P
		success = !removeAllIfaceCallbackHidlObjectsFromMap(
		    wpa_s->ifname, p2p_iface_callbacks_map_);
	} else {  // assumed to be STA
		success = !removeHidlObjectFromMap(
		    wpa_s->ifname, sta_iface_object_map_);
		if (success) {
			success = !removeAllIfaceCallbackHidlObjectsFromMap(
			    wpa_s->ifname, sta_iface_callbacks_map_);
		}
	}
	if (!success) {
		wpa_printf(
		    MSG_ERROR,
		    "Failed to unregister interface with HIDL "
		    "control: %s",
		    wpa_s->ifname);
		return 1;
	}

	// Invoke the |onInterfaceRemoved| method on all registered callbacks.
	callWithEachSupplicantCallback(std::bind(
	    &ISupplicantCallback::onInterfaceRemoved, std::placeholders::_1,
	    wpa_s->ifname));
	return 0;
}

/**
 * Register a network to hidl manager.
 *
 * @param wpa_s |wpa_supplicant| struct corresponding to the interface on which
 * the network is added.
 * @param ssid |wpa_ssid| struct corresponding to the network being added.
 *
 * @return 0 on success, 1 on failure.
 */
int HidlManager::registerNetwork(
    struct wpa_supplicant *wpa_s, struct wpa_ssid *ssid)
{
	if (!wpa_s || !ssid)
		return 1;

	// Generate the key to be used to lookup the network.
	const std::string network_key =
	    getNetworkObjectMapKey(wpa_s->ifname, ssid->id);

	if (isP2pIface(wpa_s)) {
		if (addHidlObjectToMap<P2pNetwork>(
			network_key,
			new P2pNetwork(wpa_s->global, wpa_s->ifname, ssid->id),
			p2p_network_object_map_)) {
			wpa_printf(
			    MSG_ERROR,
			    "Failed to register P2P network with HIDL "
			    "control: %d",
			    ssid->id);
			return 1;
		}
		p2p_network_callbacks_map_[network_key] =
		    std::vector<android::sp<ISupplicantP2pNetworkCallback>>();
		// Invoke the |onNetworkAdded| method on all registered
		// callbacks.
		callWithEachP2pIfaceCallback(
		    wpa_s->ifname,
		    std::bind(
			&ISupplicantP2pIfaceCallback::onNetworkAdded,
			std::placeholders::_1, ssid->id));
	} else {
		if (addHidlObjectToMap<StaNetwork>(
			network_key,
			new StaNetwork(wpa_s->global, wpa_s->ifname, ssid->id),
			sta_network_object_map_)) {
			wpa_printf(
			    MSG_ERROR,
			    "Failed to register STA network with HIDL "
			    "control: %d",
			    ssid->id);
			return 1;
		}
		sta_network_callbacks_map_[network_key] =
		    std::vector<android::sp<ISupplicantStaNetworkCallback>>();
		// Invoke the |onNetworkAdded| method on all registered
		// callbacks.
		callWithEachStaIfaceCallback(
		    wpa_s->ifname,
		    std::bind(
			&ISupplicantStaIfaceCallback::onNetworkAdded,
			std::placeholders::_1, ssid->id));
	}
	return 0;
}

/**
 * Unregister a network from hidl manager.
 *
 * @param wpa_s |wpa_supplicant| struct corresponding to the interface on which
 * the network is added.
 * @param ssid |wpa_ssid| struct corresponding to the network being added.
 *
 * @return 0 on success, 1 on failure.
 */
int HidlManager::unregisterNetwork(
    struct wpa_supplicant *wpa_s, struct wpa_ssid *ssid)
{
	if (!wpa_s || !ssid)
		return 1;

	// Generate the key to be used to lookup the network.
	const std::string network_key =
	    getNetworkObjectMapKey(wpa_s->ifname, ssid->id);

	if (isP2pIface(wpa_s)) {
		if (removeHidlObjectFromMap(
			network_key, p2p_network_object_map_)) {
			wpa_printf(
			    MSG_ERROR,
			    "Failed to unregister P2P network with HIDL "
			    "control: %d",
			    ssid->id);
			return 1;
		}
		if (removeAllNetworkCallbackHidlObjectsFromMap(
			network_key, p2p_network_callbacks_map_))
			return 1;

		// Invoke the |onNetworkRemoved| method on all registered
		// callbacks.
		callWithEachP2pIfaceCallback(
		    wpa_s->ifname,
		    std::bind(
			&ISupplicantP2pIfaceCallback::onNetworkRemoved,
			std::placeholders::_1, ssid->id));
	} else {
		if (removeHidlObjectFromMap(
			network_key, sta_network_object_map_)) {
			wpa_printf(
			    MSG_ERROR,
			    "Failed to unregister STA network with HIDL "
			    "control: %d",
			    ssid->id);
			return 1;
		}
		if (removeAllNetworkCallbackHidlObjectsFromMap(
			network_key, sta_network_callbacks_map_))
			return 1;

		// Invoke the |onNetworkRemoved| method on all registered
		// callbacks.
		callWithEachStaIfaceCallback(
		    wpa_s->ifname,
		    std::bind(
			&ISupplicantStaIfaceCallback::onNetworkRemoved,
			std::placeholders::_1, ssid->id));
	}
	return 0;
}

/**
 * Notify all listeners about any state changes on a particular interface.
 *
 * @param wpa_s |wpa_supplicant| struct corresponding to the interface on which
 * the state change event occured.
 */
int HidlManager::notifyStateChange(struct wpa_supplicant *wpa_s)
{
	if (!wpa_s)
		return 1;

	if (sta_iface_object_map_.find(wpa_s->ifname) ==
	    sta_iface_object_map_.end())
		return 1;

	// Invoke the |onStateChanged| method on all registered callbacks.
	uint32_t hidl_network_id = UINT32_MAX;
	std::vector<uint8_t> hidl_ssid;
	if (wpa_s->current_ssid) {
		hidl_network_id = wpa_s->current_ssid->id;
		hidl_ssid.assign(
		    wpa_s->current_ssid->ssid,
		    wpa_s->current_ssid->ssid + wpa_s->current_ssid->ssid_len);
	}
	uint8_t *bssid;
	// wpa_supplicant sets the |pending_bssid| field when it starts a
	// connection. Only after association state does it update the |bssid|
	// field. So, in the HIDL callback send the appropriate bssid.
	if (wpa_s->wpa_state <= WPA_ASSOCIATED) {
		bssid = wpa_s->pending_bssid;
	} else {
		bssid = wpa_s->bssid;
	}
	callWithEachStaIfaceCallback(
	    wpa_s->ifname, std::bind(
			       &ISupplicantStaIfaceCallback::onStateChanged,
			       std::placeholders::_1,
			       static_cast<ISupplicantStaIfaceCallback::State>(
				   wpa_s->wpa_state),
			       bssid, hidl_network_id, hidl_ssid));
	return 0;
}

/**
 * Notify all listeners about a request on a particular network.
 *
 * @param wpa_s |wpa_supplicant| struct corresponding to the interface on which
 * the network is present.
 * @param ssid |wpa_ssid| struct corresponding to the network.
 * @param type type of request.
 * @param param addition params associated with the request.
 */
int HidlManager::notifyNetworkRequest(
    struct wpa_supplicant *wpa_s, struct wpa_ssid *ssid, int type,
    const char *param)
{
	if (!wpa_s || !ssid)
		return 1;

	const std::string network_key =
	    getNetworkObjectMapKey(wpa_s->ifname, ssid->id);
	if (sta_network_object_map_.find(network_key) ==
	    sta_network_object_map_.end())
		return 1;

	if (type == WPA_CTRL_REQ_EAP_IDENTITY) {
		callWithEachStaNetworkCallback(
		    wpa_s->ifname, ssid->id,
		    std::bind(
			&ISupplicantStaNetworkCallback::
			    onNetworkEapIdentityRequest,
			std::placeholders::_1));
		return 0;
	}
	if (type == WPA_CTRL_REQ_SIM) {
		std::vector<hidl_array<uint8_t, 16>> gsm_rands;
		hidl_array<uint8_t, 16> umts_rand;
		hidl_array<uint8_t, 16> umts_autn;
		if (!parseGsmAuthNetworkRequest(param, &gsm_rands)) {
			ISupplicantStaNetworkCallback::
			    NetworkRequestEapSimGsmAuthParams hidl_params;
			hidl_params.rands = gsm_rands;
			callWithEachStaNetworkCallback(
			    wpa_s->ifname, ssid->id,
			    std::bind(
				&ISupplicantStaNetworkCallback::
				    onNetworkEapSimGsmAuthRequest,
				std::placeholders::_1, hidl_params));
			return 0;
		}
		if (!parseUmtsAuthNetworkRequest(
			param, &umts_rand, &umts_autn)) {
			ISupplicantStaNetworkCallback::
			    NetworkRequestEapSimUmtsAuthParams hidl_params;
			hidl_params.rand = umts_rand;
			hidl_params.autn = umts_autn;
			callWithEachStaNetworkCallback(
			    wpa_s->ifname, ssid->id,
			    std::bind(
				&ISupplicantStaNetworkCallback::
				    onNetworkEapSimUmtsAuthRequest,
				std::placeholders::_1, hidl_params));
			return 0;
		}
	}
	return 1;
}

/**
 * Notify all listeners about the end of an ANQP query.
 *
 * @param wpa_s |wpa_supplicant| struct corresponding to the interface.
 * @param bssid BSSID of the access point.
 * @param result Result of the operation ("SUCCESS" or "FAILURE").
 * @param anqp |wpa_bss_anqp| ANQP data fetched.
 */
void HidlManager::notifyAnqpQueryDone(
    struct wpa_supplicant *wpa_s, const u8 *bssid, const char *result,
    const struct wpa_bss_anqp *anqp)
{
	if (!wpa_s || !bssid || !result || !anqp)
		return;

	if (sta_iface_object_map_.find(wpa_s->ifname) ==
	    sta_iface_object_map_.end())
		return;

	ISupplicantStaIfaceCallback::AnqpData hidl_anqp_data;
	ISupplicantStaIfaceCallback::Hs20AnqpData hidl_hs20_anqp_data;
	if (std::string(result) == "SUCCESS") {
		hidl_anqp_data.venueName =
		    misc_utils::convertWpaBufToVector(anqp->venue_name);
		hidl_anqp_data.roamingConsortium =
		    misc_utils::convertWpaBufToVector(anqp->roaming_consortium);
		hidl_anqp_data.ipAddrTypeAvailability =
		    misc_utils::convertWpaBufToVector(
			anqp->ip_addr_type_availability);
		hidl_anqp_data.naiRealm =
		    misc_utils::convertWpaBufToVector(anqp->nai_realm);
		hidl_anqp_data.anqp3gppCellularNetwork =
		    misc_utils::convertWpaBufToVector(anqp->anqp_3gpp);
		hidl_anqp_data.domainName =
		    misc_utils::convertWpaBufToVector(anqp->domain_name);

		hidl_hs20_anqp_data.operatorFriendlyName =
		    misc_utils::convertWpaBufToVector(
			anqp->hs20_operator_friendly_name);
		hidl_hs20_anqp_data.wanMetrics =
		    misc_utils::convertWpaBufToVector(anqp->hs20_wan_metrics);
		hidl_hs20_anqp_data.connectionCapability =
		    misc_utils::convertWpaBufToVector(
			anqp->hs20_connection_capability);
		hidl_hs20_anqp_data.osuProvidersList =
		    misc_utils::convertWpaBufToVector(
			anqp->hs20_osu_providers_list);
	}

	callWithEachStaIfaceCallback(
	    wpa_s->ifname, std::bind(
			       &ISupplicantStaIfaceCallback::onAnqpQueryDone,
			       std::placeholders::_1, bssid, hidl_anqp_data,
			       hidl_hs20_anqp_data));
}

/**
 * Notify all listeners about the end of an HS20 icon query.
 *
 * @param wpa_s |wpa_supplicant| struct corresponding to the interface.
 * @param bssid BSSID of the access point.
 * @param file_name Name of the icon file.
 * @param image Raw bytes of the icon file.
 * @param image_length Size of the the icon file.
 */
void HidlManager::notifyHs20IconQueryDone(
    struct wpa_supplicant *wpa_s, const u8 *bssid, const char *file_name,
    const u8 *image, u32 image_length)
{
	if (!wpa_s || !bssid || !file_name || !image)
		return;

	if (sta_iface_object_map_.find(wpa_s->ifname) ==
	    sta_iface_object_map_.end())
		return;

	callWithEachStaIfaceCallback(
	    wpa_s->ifname,
	    std::bind(
		&ISupplicantStaIfaceCallback::onHs20IconQueryDone,
		std::placeholders::_1, bssid, file_name,
		std::vector<uint8_t>(image, image + image_length)));
}

/**
 * Notify all listeners about the reception of HS20 subscription
 * remediation notification from the server.
 *
 * @param wpa_s |wpa_supplicant| struct corresponding to the interface.
 * @param url URL of the server.
 * @param osu_method OSU method (OMA_DM or SOAP_XML_SPP).
 */
void HidlManager::notifyHs20RxSubscriptionRemediation(
    struct wpa_supplicant *wpa_s, const char *url, u8 osu_method)
{
	if (!wpa_s || !url)
		return;

	if (sta_iface_object_map_.find(wpa_s->ifname) ==
	    sta_iface_object_map_.end())
		return;

	ISupplicantStaIfaceCallback::OsuMethod hidl_osu_method = {};
	if (osu_method & 0x1) {
		hidl_osu_method =
		    ISupplicantStaIfaceCallback::OsuMethod::OMA_DM;
	} else if (osu_method & 0x2) {
		hidl_osu_method =
		    ISupplicantStaIfaceCallback::OsuMethod::SOAP_XML_SPP;
	}
	callWithEachStaIfaceCallback(
	    wpa_s->ifname,
	    std::bind(
		&ISupplicantStaIfaceCallback::onHs20SubscriptionRemediation,
		std::placeholders::_1, wpa_s->bssid, hidl_osu_method, url));
}

/**
 * Notify all listeners about the reception of HS20 immient deauth
 * notification from the server.
 *
 * @param wpa_s |wpa_supplicant| struct corresponding to the interface.
 * @param code Deauth reason code sent from server.
 * @param reauth_delay Reauthentication delay in seconds sent from server.
 * @param url URL of the server.
 */
void HidlManager::notifyHs20RxDeauthImminentNotice(
    struct wpa_supplicant *wpa_s, u8 code, u16 reauth_delay, const char *url)
{
	if (!wpa_s || !url)
		return;

	if (sta_iface_object_map_.find(wpa_s->ifname) ==
	    sta_iface_object_map_.end())
		return;

	callWithEachStaIfaceCallback(
	    wpa_s->ifname,
	    std::bind(
		&ISupplicantStaIfaceCallback::onHs20DeauthImminentNotice,
		std::placeholders::_1, wpa_s->bssid, code, reauth_delay, url));
}

/**
 * Notify all listeners about the reason code for disconnection from the
 * currently connected network.
 *
 * @param wpa_s |wpa_supplicant| struct corresponding to the interface on which
 * the network is present.
 */
void HidlManager::notifyDisconnectReason(struct wpa_supplicant *wpa_s)
{
	if (!wpa_s)
		return;

	if (sta_iface_object_map_.find(wpa_s->ifname) ==
	    sta_iface_object_map_.end())
		return;

	const u8 *bssid = wpa_s->bssid;
	if (is_zero_ether_addr(bssid)) {
		bssid = wpa_s->pending_bssid;
	}

	callWithEachStaIfaceCallback(
	    wpa_s->ifname,
	    std::bind(
		&ISupplicantStaIfaceCallback::onDisconnected,
		std::placeholders::_1, bssid, wpa_s->disconnect_reason < 0,
		static_cast<ISupplicantStaIfaceCallback::ReasonCode>(
		    abs(wpa_s->disconnect_reason))));
}

/**
 * Notify all listeners about association reject from the access point to which
 * we are attempting to connect.
 *
 * @param wpa_s |wpa_supplicant| struct corresponding to the interface on which
 * the network is present.
 */
void HidlManager::notifyAssocReject(struct wpa_supplicant *wpa_s)
{
	if (!wpa_s)
		return;

	if (sta_iface_object_map_.find(wpa_s->ifname) ==
	    sta_iface_object_map_.end())
		return;

	const u8 *bssid = wpa_s->bssid;
	if (is_zero_ether_addr(bssid)) {
		bssid = wpa_s->pending_bssid;
	}

	callWithEachStaIfaceCallback(
	    wpa_s->ifname,
	    std::bind(
		&ISupplicantStaIfaceCallback::onAssociationRejected,
		std::placeholders::_1, bssid,
		static_cast<ISupplicantStaIfaceCallback::StatusCode>(
		    wpa_s->assoc_status_code),
		wpa_s->assoc_timed_out == 1));
}

void HidlManager::notifyAuthTimeout(struct wpa_supplicant *wpa_s)
{
	if (!wpa_s)
		return;

	const std::string ifname(wpa_s->ifname);
	if (sta_iface_object_map_.find(ifname) == sta_iface_object_map_.end())
		return;

	const u8 *bssid = wpa_s->bssid;
	if (is_zero_ether_addr(bssid)) {
		bssid = wpa_s->pending_bssid;
	}
	callWithEachStaIfaceCallback(
	    wpa_s->ifname,
	    std::bind(
		&ISupplicantStaIfaceCallback::onAuthenticationTimeout,
		std::placeholders::_1, bssid));
}

void HidlManager::notifyBssidChanged(struct wpa_supplicant *wpa_s)
{
	if (!wpa_s)
		return;

	const std::string ifname(wpa_s->ifname);
	if (sta_iface_object_map_.find(ifname) == sta_iface_object_map_.end())
		return;

	// wpa_supplicant does not explicitly give us the reason for bssid
	// change, but we figure that out from what is set out of |wpa_s->bssid|
	// & |wpa_s->pending_bssid|.
	const u8 *bssid;
	ISupplicantStaIfaceCallback::BssidChangeReason reason;
	if (is_zero_ether_addr(wpa_s->bssid) &&
	    !is_zero_ether_addr(wpa_s->pending_bssid)) {
		bssid = wpa_s->pending_bssid;
		reason =
		    ISupplicantStaIfaceCallback::BssidChangeReason::ASSOC_START;
	} else if (
	    !is_zero_ether_addr(wpa_s->bssid) &&
	    is_zero_ether_addr(wpa_s->pending_bssid)) {
		bssid = wpa_s->bssid;
		reason = ISupplicantStaIfaceCallback::BssidChangeReason::
		    ASSOC_COMPLETE;
	} else if (
	    is_zero_ether_addr(wpa_s->bssid) &&
	    is_zero_ether_addr(wpa_s->pending_bssid)) {
		bssid = wpa_s->pending_bssid;
		reason =
		    ISupplicantStaIfaceCallback::BssidChangeReason::DISASSOC;
	} else {
		wpa_printf(MSG_ERROR, "Unknown bssid change reason");
		return;
	}

	callWithEachStaIfaceCallback(
	    wpa_s->ifname, std::bind(
			       &ISupplicantStaIfaceCallback::onBssidChanged,
			       std::placeholders::_1, reason, bssid));
}

void HidlManager::notifyWpsEventFail(
    struct wpa_supplicant *wpa_s, uint8_t *peer_macaddr, uint16_t config_error,
    uint16_t error_indication)
{
	if (!wpa_s || !peer_macaddr)
		return;

	if (sta_iface_object_map_.find(wpa_s->ifname) ==
	    sta_iface_object_map_.end())
		return;

	callWithEachStaIfaceCallback(
	    wpa_s->ifname,
	    std::bind(
		&ISupplicantStaIfaceCallback::onWpsEventFail,
		std::placeholders::_1, peer_macaddr,
		static_cast<ISupplicantStaIfaceCallback::WpsConfigError>(
		    config_error),
		static_cast<ISupplicantStaIfaceCallback::WpsErrorIndication>(
		    error_indication)));
}

void HidlManager::notifyWpsEventSuccess(struct wpa_supplicant *wpa_s)
{
	if (!wpa_s)
		return;

	if (sta_iface_object_map_.find(wpa_s->ifname) ==
	    sta_iface_object_map_.end())
		return;

	callWithEachStaIfaceCallback(
	    wpa_s->ifname, std::bind(
			       &ISupplicantStaIfaceCallback::onWpsEventSuccess,
			       std::placeholders::_1));
}

void HidlManager::notifyWpsEventPbcOverlap(struct wpa_supplicant *wpa_s)
{
	if (!wpa_s)
		return;

	if (sta_iface_object_map_.find(wpa_s->ifname) ==
	    sta_iface_object_map_.end())
		return;

	callWithEachStaIfaceCallback(
	    wpa_s->ifname,
	    std::bind(
		&ISupplicantStaIfaceCallback::onWpsEventPbcOverlap,
		std::placeholders::_1));
}

void HidlManager::notifyP2pDeviceFound(
    struct wpa_supplicant *wpa_s, const u8 *addr,
    const struct p2p_peer_info *info, const u8 *peer_wfd_device_info,
    u8 peer_wfd_device_info_len)
{
	if (!wpa_s || !addr || !info)
		return;

	if (p2p_iface_object_map_.find(wpa_s->ifname) ==
	    p2p_iface_object_map_.end())
		return;

	std::array<uint8_t, kWfdDeviceInfoLen> hidl_peer_wfd_device_info{};
	if (peer_wfd_device_info) {
		if (peer_wfd_device_info_len != kWfdDeviceInfoLen) {
			wpa_printf(
			    MSG_ERROR, "Unexpected WFD device info len: %d",
			    peer_wfd_device_info_len);
		} else {
			os_memcpy(
			    hidl_peer_wfd_device_info.data(),
			    peer_wfd_device_info, kWfdDeviceInfoLen);
		}
	}

	callWithEachP2pIfaceCallback(
	    wpa_s->ifname,
	    std::bind(
		&ISupplicantP2pIfaceCallback::onDeviceFound,
		std::placeholders::_1, addr, info->p2p_device_addr,
		info->pri_dev_type, info->device_name, info->config_methods,
		info->dev_capab, info->group_capab, hidl_peer_wfd_device_info));
}

void HidlManager::notifyP2pDeviceLost(
    struct wpa_supplicant *wpa_s, const u8 *p2p_device_addr)
{
	if (!wpa_s || !p2p_device_addr)
		return;

	if (p2p_iface_object_map_.find(wpa_s->ifname) ==
	    p2p_iface_object_map_.end())
		return;

	callWithEachP2pIfaceCallback(
	    wpa_s->ifname, std::bind(
			       &ISupplicantP2pIfaceCallback::onDeviceLost,
			       std::placeholders::_1, p2p_device_addr));
}

void HidlManager::notifyP2pFindStopped(struct wpa_supplicant *wpa_s)
{
	if (!wpa_s)
		return;

	if (p2p_iface_object_map_.find(wpa_s->ifname) ==
	    p2p_iface_object_map_.end())
		return;

	callWithEachP2pIfaceCallback(
	    wpa_s->ifname, std::bind(
			       &ISupplicantP2pIfaceCallback::onFindStopped,
			       std::placeholders::_1));
}

void HidlManager::notifyP2pGoNegReq(
    struct wpa_supplicant *wpa_s, const u8 *src_addr, u16 dev_passwd_id,
    u8 /* go_intent */)
{
	if (!wpa_s || !src_addr)
		return;

	if (p2p_iface_object_map_.find(wpa_s->ifname) ==
	    p2p_iface_object_map_.end())
		return;

	callWithEachP2pIfaceCallback(
	    wpa_s->ifname,
	    std::bind(
		&ISupplicantP2pIfaceCallback::onGoNegotiationRequest,
		std::placeholders::_1, src_addr,
		static_cast<ISupplicantP2pIfaceCallback::WpsDevPasswordId>(
		    dev_passwd_id)));
}

void HidlManager::notifyP2pGoNegCompleted(
    struct wpa_supplicant *wpa_s, const struct p2p_go_neg_results *res)
{
	if (!wpa_s || !res)
		return;

	if (p2p_iface_object_map_.find(wpa_s->ifname) ==
	    p2p_iface_object_map_.end())
		return;

	callWithEachP2pIfaceCallback(
	    wpa_s->ifname,
	    std::bind(
		&ISupplicantP2pIfaceCallback::onGoNegotiationCompleted,
		std::placeholders::_1,
		static_cast<ISupplicantP2pIfaceCallback::P2pStatusCode>(
		    res->status)));
}

void HidlManager::notifyP2pGroupFormationFailure(
    struct wpa_supplicant *wpa_s, const char *reason)
{
	if (!wpa_s || !reason)
		return;

	if (p2p_iface_object_map_.find(wpa_s->ifname) ==
	    p2p_iface_object_map_.end())
		return;

	callWithEachP2pIfaceCallback(
	    wpa_s->ifname,
	    std::bind(
		&ISupplicantP2pIfaceCallback::onGroupFormationFailure,
		std::placeholders::_1, reason));
}

void HidlManager::notifyP2pGroupStarted(
    struct wpa_supplicant *wpa_group_s, const struct wpa_ssid *ssid, int persistent, int client)
{
	if (!wpa_group_s || !wpa_group_s->parent || !ssid)
		return;

	// For group notifications, need to use the parent iface for callbacks.
	struct wpa_supplicant *wpa_s = wpa_group_s->parent;
	if (p2p_iface_object_map_.find(wpa_s->ifname) ==
	    p2p_iface_object_map_.end())
		return;

	uint32_t hidl_freq = wpa_group_s->current_bss
				 ? wpa_group_s->current_bss->freq
				 : wpa_group_s->assoc_freq;
	std::array<uint8_t, 32> hidl_psk;
	if (ssid->psk_set) {
		os_memcpy(hidl_psk.data(), ssid->psk, 32);
	}
	bool hidl_is_go = (client == 0 ? true : false);
	bool hidl_is_persistent = (persistent == 1 ? true : false);

	callWithEachP2pIfaceCallback(
	    wpa_s->ifname,
	    std::bind(
		&ISupplicantP2pIfaceCallback::onGroupStarted,
		std::placeholders::_1, wpa_group_s->ifname, hidl_is_go,
		std::vector<uint8_t>{ssid->ssid, ssid->ssid + ssid->ssid_len},
		hidl_freq, hidl_psk, ssid->passphrase, wpa_group_s->go_dev_addr,
		hidl_is_persistent));
}

void HidlManager::notifyP2pGroupRemoved(
    struct wpa_supplicant *wpa_group_s, const struct wpa_ssid *ssid,
    const char *role)
{
	if (!wpa_group_s || !wpa_group_s->parent || !ssid || !role)
		return;

	// For group notifications, need to use the parent iface for callbacks.
	struct wpa_supplicant *wpa_s = wpa_group_s->parent;
	if (p2p_iface_object_map_.find(wpa_s->ifname) ==
	    p2p_iface_object_map_.end())
		return;

	bool hidl_is_go = (std::string(role) == "GO");

	callWithEachP2pIfaceCallback(
	    wpa_s->ifname,
	    std::bind(
		&ISupplicantP2pIfaceCallback::onGroupRemoved,
		std::placeholders::_1, wpa_group_s->ifname, hidl_is_go));
}

void HidlManager::notifyP2pInvitationReceived(
    struct wpa_supplicant *wpa_s, const u8 *sa, const u8 *go_dev_addr,
    const u8 *bssid, int id, int op_freq)
{
	if (!wpa_s || !sa || !go_dev_addr || !bssid)
		return;

	if (p2p_iface_object_map_.find(wpa_s->ifname) ==
	    p2p_iface_object_map_.end())
		return;

	SupplicantNetworkId hidl_network_id;
	if (id < 0) {
		hidl_network_id = UINT32_MAX;
	}
	hidl_network_id = id;

	callWithEachP2pIfaceCallback(
	    wpa_s->ifname,
	    std::bind(
		&ISupplicantP2pIfaceCallback::onInvitationReceived,
		std::placeholders::_1, sa, go_dev_addr, bssid, hidl_network_id,
		op_freq));
}

void HidlManager::notifyP2pInvitationResult(
    struct wpa_supplicant *wpa_s, int status, const u8 *bssid)
{
	if (!wpa_s)
		return;

	if (p2p_iface_object_map_.find(wpa_s->ifname) ==
	    p2p_iface_object_map_.end())
		return;

	callWithEachP2pIfaceCallback(
	    wpa_s->ifname,
	    std::bind(
		&ISupplicantP2pIfaceCallback::onInvitationResult,
		std::placeholders::_1, bssid ? bssid : kZeroBssid,
		static_cast<ISupplicantP2pIfaceCallback::P2pStatusCode>(
		    status)));
}

void HidlManager::notifyP2pProvisionDiscovery(
    struct wpa_supplicant *wpa_s, const u8 *dev_addr, int request,
    enum p2p_prov_disc_status status, u16 config_methods,
    unsigned int generated_pin)
{
	if (!wpa_s || !dev_addr)
		return;

	if (p2p_iface_object_map_.find(wpa_s->ifname) ==
	    p2p_iface_object_map_.end())
		return;

	std::string hidl_generated_pin;
	if (generated_pin > 0) {
		hidl_generated_pin =
		    misc_utils::convertWpsPinToString(generated_pin);
	}
	bool hidl_is_request = (request == 1 ? true : false);

	callWithEachP2pIfaceCallback(
	    wpa_s->ifname,
	    std::bind(
		&ISupplicantP2pIfaceCallback::onProvisionDiscoveryCompleted,
		std::placeholders::_1, dev_addr, hidl_is_request,
		static_cast<ISupplicantP2pIfaceCallback::P2pProvDiscStatusCode>(
		    status),
		config_methods, hidl_generated_pin));
}

void HidlManager::notifyP2pSdResponse(
    struct wpa_supplicant *wpa_s, const u8 *sa, u16 update_indic,
    const u8 *tlvs, size_t tlvs_len)
{
	if (!wpa_s || !sa || !tlvs)
		return;

	if (p2p_iface_object_map_.find(wpa_s->ifname) ==
	    p2p_iface_object_map_.end())
		return;

	callWithEachP2pIfaceCallback(
	    wpa_s->ifname,
	    std::bind(
		&ISupplicantP2pIfaceCallback::onServiceDiscoveryResponse,
		std::placeholders::_1, sa, update_indic,
		std::vector<uint8_t>{tlvs, tlvs + tlvs_len}));
}

void HidlManager::notifyApStaAuthorized(
    struct wpa_supplicant *wpa_s, const u8 *sta, const u8 *p2p_dev_addr)
{
	if (!wpa_s || !wpa_s->parent || !sta)
		return;
	if (p2p_iface_object_map_.find(wpa_s->parent->ifname) ==
	    p2p_iface_object_map_.end())
		return;
	callWithEachP2pIfaceCallback(
	    wpa_s->parent->ifname, std::bind(
			       &ISupplicantP2pIfaceCallback::onStaAuthorized,
			       std::placeholders::_1, sta, p2p_dev_addr ? p2p_dev_addr : kZeroBssid));
}

void HidlManager::notifyApStaDeauthorized(
    struct wpa_supplicant *wpa_s, const u8 *sta, const u8 *p2p_dev_addr)
{
	if (!wpa_s || !wpa_s->parent || !sta)
		return;
	if (p2p_iface_object_map_.find(wpa_s->parent->ifname) ==
	    p2p_iface_object_map_.end())
		return;

	callWithEachP2pIfaceCallback(
	    wpa_s->parent->ifname, std::bind(
			       &ISupplicantP2pIfaceCallback::onStaDeauthorized,
			       std::placeholders::_1, sta, p2p_dev_addr ? p2p_dev_addr : kZeroBssid));
}

void HidlManager::notifyExtRadioWorkStart(
    struct wpa_supplicant *wpa_s, uint32_t id)
{
	if (!wpa_s)
		return;

	if (sta_iface_object_map_.find(wpa_s->ifname) ==
	    sta_iface_object_map_.end())
		return;

	callWithEachStaIfaceCallback(
	    wpa_s->ifname,
	    std::bind(
		&ISupplicantStaIfaceCallback::onExtRadioWorkStart,
		std::placeholders::_1, id));
}

void HidlManager::notifyExtRadioWorkTimeout(
    struct wpa_supplicant *wpa_s, uint32_t id)
{
	if (!wpa_s)
		return;

	if (sta_iface_object_map_.find(wpa_s->ifname) ==
	    sta_iface_object_map_.end())
		return;

	callWithEachStaIfaceCallback(
	    wpa_s->ifname,
	    std::bind(
		&ISupplicantStaIfaceCallback::onExtRadioWorkTimeout,
		std::placeholders::_1, id));
}

void HidlManager::notifyEapError(struct wpa_supplicant *wpa_s, int error_code)
{
	typedef V1_1::ISupplicantStaIfaceCallback::EapErrorCode EapErrorCode;

	if (!wpa_s)
		return;

	switch (static_cast<EapErrorCode>(error_code)) {
		case EapErrorCode::SIM_GENERAL_FAILURE_AFTER_AUTH:
		case EapErrorCode::SIM_TEMPORARILY_DENIED:
		case EapErrorCode::SIM_NOT_SUBSCRIBED:
		case EapErrorCode::SIM_GENERAL_FAILURE_BEFORE_AUTH:
		case EapErrorCode::SIM_VENDOR_SPECIFIC_EXPIRED_CERT:
			break;
		default:
			return;
	}

	callWithEachStaIfaceCallback_1_1(
	    wpa_s->ifname,
	    std::bind(
		&V1_1::ISupplicantStaIfaceCallback::onEapFailure_1_1,
		std::placeholders::_1,
		static_cast<EapErrorCode>(error_code)));
}

/**
 * Retrieve the |ISupplicantP2pIface| hidl object reference using the provided
 * ifname.
 *
 * @param ifname Name of the corresponding interface.
 * @param iface_object Hidl reference corresponding to the iface.
 *
 * @return 0 on success, 1 on failure.
 */
int HidlManager::getP2pIfaceHidlObjectByIfname(
    const std::string &ifname, android::sp<ISupplicantP2pIface> *iface_object)
{
	if (ifname.empty() || !iface_object)
		return 1;

	auto iface_object_iter = p2p_iface_object_map_.find(ifname);
	if (iface_object_iter == p2p_iface_object_map_.end())
		return 1;

	*iface_object = iface_object_iter->second;
	return 0;
}

/**
 * Retrieve the |ISupplicantStaIface| hidl object reference using the provided
 * ifname.
 *
 * @param ifname Name of the corresponding interface.
 * @param iface_object Hidl reference corresponding to the iface.
 *
 * @return 0 on success, 1 on failure.
 */
int HidlManager::getStaIfaceHidlObjectByIfname(
    const std::string &ifname, android::sp<ISupplicantStaIface> *iface_object)
{
	if (ifname.empty() || !iface_object)
		return 1;

	auto iface_object_iter = sta_iface_object_map_.find(ifname);
	if (iface_object_iter == sta_iface_object_map_.end())
		return 1;

	*iface_object = iface_object_iter->second;
	return 0;
}

/**
 * Retrieve the |ISupplicantP2pNetwork| hidl object reference using the provided
 * ifname and network_id.
 *
 * @param ifname Name of the corresponding interface.
 * @param network_id ID of the corresponding network.
 * @param network_object Hidl reference corresponding to the network.
 *
 * @return 0 on success, 1 on failure.
 */
int HidlManager::getP2pNetworkHidlObjectByIfnameAndNetworkId(
    const std::string &ifname, int network_id,
    android::sp<ISupplicantP2pNetwork> *network_object)
{
	if (ifname.empty() || network_id < 0 || !network_object)
		return 1;

	// Generate the key to be used to lookup the network.
	const std::string network_key =
	    getNetworkObjectMapKey(ifname, network_id);

	auto network_object_iter = p2p_network_object_map_.find(network_key);
	if (network_object_iter == p2p_network_object_map_.end())
		return 1;

	*network_object = network_object_iter->second;
	return 0;
}

/**
 * Retrieve the |ISupplicantStaNetwork| hidl object reference using the provided
 * ifname and network_id.
 *
 * @param ifname Name of the corresponding interface.
 * @param network_id ID of the corresponding network.
 * @param network_object Hidl reference corresponding to the network.
 *
 * @return 0 on success, 1 on failure.
 */
int HidlManager::getStaNetworkHidlObjectByIfnameAndNetworkId(
    const std::string &ifname, int network_id,
    android::sp<ISupplicantStaNetwork> *network_object)
{
	if (ifname.empty() || network_id < 0 || !network_object)
		return 1;

	// Generate the key to be used to lookup the network.
	const std::string network_key =
	    getNetworkObjectMapKey(ifname, network_id);

	auto network_object_iter = sta_network_object_map_.find(network_key);
	if (network_object_iter == sta_network_object_map_.end())
		return 1;

	*network_object = network_object_iter->second;
	return 0;
}

/**
 * Add a new |ISupplicantCallback| hidl object reference to our
 * global callback list.
 *
 * @param callback Hidl reference of the |ISupplicantCallback| object.
 *
 * @return 0 on success, 1 on failure.
 */
int HidlManager::addSupplicantCallbackHidlObject(
    const android::sp<ISupplicantCallback> &callback)
{
	// Register for death notification before we add it to our list.
	auto on_hidl_died_fctor = std::bind(
	    &HidlManager::removeSupplicantCallbackHidlObject, this,
	    std::placeholders::_1);
	return registerForDeathAndAddCallbackHidlObjectToList<
	    ISupplicantCallback>(
	    callback, on_hidl_died_fctor, supplicant_callbacks_);
}

/**
 * Add a new iface callback hidl object reference to our
 * interface callback list.
 *
 * @param ifname Name of the corresponding interface.
 * @param callback Hidl reference of the callback object.
 *
 * @return 0 on success, 1 on failure.
 */
int HidlManager::addP2pIfaceCallbackHidlObject(
    const std::string &ifname,
    const android::sp<ISupplicantP2pIfaceCallback> &callback)
{
	const std::function<void(
	    const android::sp<ISupplicantP2pIfaceCallback> &)>
	    on_hidl_died_fctor = std::bind(
		&HidlManager::removeP2pIfaceCallbackHidlObject, this, ifname,
		std::placeholders::_1);
	return addIfaceCallbackHidlObjectToMap(
	    ifname, callback, on_hidl_died_fctor, p2p_iface_callbacks_map_);
}

/**
 * Add a new iface callback hidl object reference to our
 * interface callback list.
 *
 * @param ifname Name of the corresponding interface.
 * @param callback Hidl reference of the callback object.
 *
 * @return 0 on success, 1 on failure.
 */
int HidlManager::addStaIfaceCallbackHidlObject(
    const std::string &ifname,
    const android::sp<ISupplicantStaIfaceCallback> &callback)
{
	const std::function<void(
	    const android::sp<ISupplicantStaIfaceCallback> &)>
	    on_hidl_died_fctor = std::bind(
		&HidlManager::removeStaIfaceCallbackHidlObject, this, ifname,
		std::placeholders::_1);
	return addIfaceCallbackHidlObjectToMap(
	    ifname, callback, on_hidl_died_fctor, sta_iface_callbacks_map_);
}

/**
 * Add a new network callback hidl object reference to our network callback
 * list.
 *
 * @param ifname Name of the corresponding interface.
 * @param network_id ID of the corresponding network.
 * @param callback Hidl reference of the callback object.
 *
 * @return 0 on success, 1 on failure.
 */
int HidlManager::addP2pNetworkCallbackHidlObject(
    const std::string &ifname, int network_id,
    const android::sp<ISupplicantP2pNetworkCallback> &callback)
{
	const std::function<void(
	    const android::sp<ISupplicantP2pNetworkCallback> &)>
	    on_hidl_died_fctor = std::bind(
		&HidlManager::removeP2pNetworkCallbackHidlObject, this, ifname,
		network_id, std::placeholders::_1);
	return addNetworkCallbackHidlObjectToMap(
	    ifname, network_id, callback, on_hidl_died_fctor,
	    p2p_network_callbacks_map_);
}

/**
 * Add a new network callback hidl object reference to our network callback
 * list.
 *
 * @param ifname Name of the corresponding interface.
 * @param network_id ID of the corresponding network.
 * @param callback Hidl reference of the callback object.
 *
 * @return 0 on success, 1 on failure.
 */
int HidlManager::addStaNetworkCallbackHidlObject(
    const std::string &ifname, int network_id,
    const android::sp<ISupplicantStaNetworkCallback> &callback)
{
	const std::function<void(
	    const android::sp<ISupplicantStaNetworkCallback> &)>
	    on_hidl_died_fctor = std::bind(
		&HidlManager::removeStaNetworkCallbackHidlObject, this, ifname,
		network_id, std::placeholders::_1);
	return addNetworkCallbackHidlObjectToMap(
	    ifname, network_id, callback, on_hidl_died_fctor,
	    sta_network_callbacks_map_);
}

/**
 * Removes the provided |ISupplicantCallback| hidl object reference
 * from our global callback list.
 *
 * @param callback Hidl reference of the |ISupplicantCallback| object.
 */
void HidlManager::removeSupplicantCallbackHidlObject(
    const android::sp<ISupplicantCallback> &callback)
{
	supplicant_callbacks_.erase(
	    std::remove(
		supplicant_callbacks_.begin(), supplicant_callbacks_.end(),
		callback),
	    supplicant_callbacks_.end());
}

/**
 * Removes the provided iface callback hidl object reference from
 * our interface callback list.
 *
 * @param ifname Name of the corresponding interface.
 * @param callback Hidl reference of the callback object.
 */
void HidlManager::removeP2pIfaceCallbackHidlObject(
    const std::string &ifname,
    const android::sp<ISupplicantP2pIfaceCallback> &callback)
{
	return removeIfaceCallbackHidlObjectFromMap(
	    ifname, callback, p2p_iface_callbacks_map_);
}

/**
 * Removes the provided iface callback hidl object reference from
 * our interface callback list.
 *
 * @param ifname Name of the corresponding interface.
 * @param callback Hidl reference of the callback object.
 */
void HidlManager::removeStaIfaceCallbackHidlObject(
    const std::string &ifname,
    const android::sp<ISupplicantStaIfaceCallback> &callback)
{
	return removeIfaceCallbackHidlObjectFromMap(
	    ifname, callback, sta_iface_callbacks_map_);
}

/**
 * Removes the provided network callback hidl object reference from
 * our network callback list.
 *
 * @param ifname Name of the corresponding interface.
 * @param network_id ID of the corresponding network.
 * @param callback Hidl reference of the callback object.
 */
void HidlManager::removeP2pNetworkCallbackHidlObject(
    const std::string &ifname, int network_id,
    const android::sp<ISupplicantP2pNetworkCallback> &callback)
{
	return removeNetworkCallbackHidlObjectFromMap(
	    ifname, network_id, callback, p2p_network_callbacks_map_);
}

/**
 * Removes the provided network callback hidl object reference from
 * our network callback list.
 *
 * @param ifname Name of the corresponding interface.
 * @param network_id ID of the corresponding network.
 * @param callback Hidl reference of the callback object.
 */
void HidlManager::removeStaNetworkCallbackHidlObject(
    const std::string &ifname, int network_id,
    const android::sp<ISupplicantStaNetworkCallback> &callback)
{
	return removeNetworkCallbackHidlObjectFromMap(
	    ifname, network_id, callback, sta_network_callbacks_map_);
}

/**
 * Helper function to invoke the provided callback method on all the
 * registered |ISupplicantCallback| callback hidl objects.
 *
 * @param method Pointer to the required hidl method from
 * |ISupplicantCallback|.
 */
void HidlManager::callWithEachSupplicantCallback(
    const std::function<Return<void>(android::sp<ISupplicantCallback>)> &method)
{
	for (const auto &callback : supplicant_callbacks_) {
		if (!method(callback).isOk()) {
			wpa_printf(MSG_ERROR, "Failed to invoke HIDL callback");
		}
	}
}

/**
 * Helper fucntion to invoke the provided callback method on all the
 * registered iface callback hidl objects for the specified
 * |ifname|.
 *
 * @param ifname Name of the corresponding interface.
 * @param method Pointer to the required hidl method from
 * |ISupplicantIfaceCallback|.
 */
void HidlManager::callWithEachP2pIfaceCallback(
    const std::string &ifname,
    const std::function<Return<void>(android::sp<ISupplicantP2pIfaceCallback>)>
	&method)
{
	callWithEachIfaceCallback(ifname, method, p2p_iface_callbacks_map_);
}

/**
 * Helper fucntion to invoke the provided callback method on all the
 * registered V1.1 iface callback hidl objects for the specified
 * |ifname|.
 *
 * @param ifname Name of the corresponding interface.
 * @param method Pointer to the required hidl method from
 * |V1_1::ISupplicantIfaceCallback|.
 */
void HidlManager::callWithEachStaIfaceCallback_1_1(
    const std::string &ifname,
    const std::function<Return<void>
	(android::sp<V1_1::ISupplicantStaIfaceCallback>)> &method)
{
	callWithEachIfaceCallback_1_1(ifname, method, sta_iface_callbacks_map_);
}

/**
 * Helper fucntion to invoke the provided callback method on all the
 * registered iface callback hidl objects for the specified
 * |ifname|.
 *
 * @param ifname Name of the corresponding interface.
 * @param method Pointer to the required hidl method from
 * |ISupplicantIfaceCallback|.
 */
void HidlManager::callWithEachStaIfaceCallback(
    const std::string &ifname,
    const std::function<Return<void>(android::sp<ISupplicantStaIfaceCallback>)>
	&method)
{
	callWithEachIfaceCallback(ifname, method, sta_iface_callbacks_map_);
}

/**
 * Helper function to invoke the provided callback method on all the
 * registered network callback hidl objects for the specified
 * |ifname| & |network_id|.
 *
 * @param ifname Name of the corresponding interface.
 * @param network_id ID of the corresponding network.
 * @param method Pointer to the required hidl method from
 * |ISupplicantP2pNetworkCallback| or |ISupplicantStaNetworkCallback| .
 */
void HidlManager::callWithEachP2pNetworkCallback(
    const std::string &ifname, int network_id,
    const std::function<
	Return<void>(android::sp<ISupplicantP2pNetworkCallback>)> &method)
{
	callWithEachNetworkCallback(
	    ifname, network_id, method, p2p_network_callbacks_map_);
}

/**
 * Helper function to invoke the provided callback method on all the
 * registered network callback hidl objects for the specified
 * |ifname| & |network_id|.
 *
 * @param ifname Name of the corresponding interface.
 * @param network_id ID of the corresponding network.
 * @param method Pointer to the required hidl method from
 * |ISupplicantP2pNetworkCallback| or |ISupplicantStaNetworkCallback| .
 */
void HidlManager::callWithEachStaNetworkCallback(
    const std::string &ifname, int network_id,
    const std::function<
	Return<void>(android::sp<ISupplicantStaNetworkCallback>)> &method)
{
	callWithEachNetworkCallback(
	    ifname, network_id, method, sta_network_callbacks_map_);
}
}  // namespace implementation
}  // namespace V1_1
}  // namespace wifi
}  // namespace supplicant
}  // namespace hardware
}  // namespace android
