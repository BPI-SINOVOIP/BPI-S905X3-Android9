/*
 * hidl interface for wpa_supplicant daemon
 * Copyright (c) 2004-2016, Jouni Malinen <j@w1.fi>
 * Copyright (c) 2004-2016, Roshan Pius <rpius@google.com>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include "hidl_manager.h"
#include "hidl_return_util.h"
#include "p2p_network.h"

extern "C" {
#include "config_ssid.h"
}

namespace android {
namespace hardware {
namespace wifi {
namespace supplicant {
namespace V1_0 {
namespace implementation {
using hidl_return_util::validateAndCall;

P2pNetwork::P2pNetwork(
    struct wpa_global *wpa_global, const char ifname[], int network_id)
    : wpa_global_(wpa_global),
      ifname_(ifname),
      network_id_(network_id),
      is_valid_(true)
{
}

void P2pNetwork::invalidate() { is_valid_ = false; }
bool P2pNetwork::isValid()
{
	return (is_valid_ && (retrieveNetworkPtr() != nullptr));
}

Return<void> P2pNetwork::getId(getId_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &P2pNetwork::getIdInternal, _hidl_cb);
}

Return<void> P2pNetwork::getInterfaceName(getInterfaceName_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &P2pNetwork::getInterfaceNameInternal, _hidl_cb);
}

Return<void> P2pNetwork::getType(getType_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &P2pNetwork::getTypeInternal, _hidl_cb);
}

Return<void> P2pNetwork::registerCallback(
    const sp<ISupplicantP2pNetworkCallback> &callback,
    registerCallback_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &P2pNetwork::registerCallbackInternal, _hidl_cb, callback);
}

Return<void> P2pNetwork::getSsid(getSsid_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &P2pNetwork::getSsidInternal, _hidl_cb);
}

Return<void> P2pNetwork::getBssid(getBssid_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &P2pNetwork::getBssidInternal, _hidl_cb);
}

Return<void> P2pNetwork::isCurrent(isCurrent_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &P2pNetwork::isCurrentInternal, _hidl_cb);
}

Return<void> P2pNetwork::isPersistent(isPersistent_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &P2pNetwork::isPersistentInternal, _hidl_cb);
}

Return<void> P2pNetwork::isGo(isGo_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &P2pNetwork::isGoInternal, _hidl_cb);
}

Return<void> P2pNetwork::setClientList(
    const hidl_vec<hidl_array<uint8_t, 6>> &clients, setClientList_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &P2pNetwork::setClientListInternal, _hidl_cb, clients);
}

Return<void> P2pNetwork::getClientList(getClientList_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &P2pNetwork::getClientListInternal, _hidl_cb);
}

std::pair<SupplicantStatus, uint32_t> P2pNetwork::getIdInternal()
{
	return {{SupplicantStatusCode::SUCCESS, ""}, network_id_};
}

std::pair<SupplicantStatus, std::string> P2pNetwork::getInterfaceNameInternal()
{
	return {{SupplicantStatusCode::SUCCESS, ""}, ifname_};
}

std::pair<SupplicantStatus, IfaceType> P2pNetwork::getTypeInternal()
{
	return {{SupplicantStatusCode::SUCCESS, ""}, IfaceType::P2P};
}

SupplicantStatus P2pNetwork::registerCallbackInternal(
    const sp<ISupplicantP2pNetworkCallback> &callback)
{
	HidlManager *hidl_manager = HidlManager::getInstance();
	if (!hidl_manager ||
	    hidl_manager->addP2pNetworkCallbackHidlObject(
		ifname_, network_id_, callback)) {
		return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	}
	return {SupplicantStatusCode::SUCCESS, ""};
}

std::pair<SupplicantStatus, std::vector<uint8_t>> P2pNetwork::getSsidInternal()
{
	struct wpa_ssid *wpa_ssid = retrieveNetworkPtr();
	return {{SupplicantStatusCode::SUCCESS, ""},
		{wpa_ssid->ssid, wpa_ssid->ssid + wpa_ssid->ssid_len}};
}

std::pair<SupplicantStatus, std::array<uint8_t, 6>>
P2pNetwork::getBssidInternal()
{
	struct wpa_ssid *wpa_ssid = retrieveNetworkPtr();
	std::array<uint8_t, 6> bssid{};
	if (wpa_ssid->bssid_set) {
		os_memcpy(bssid.data(), wpa_ssid->bssid, ETH_ALEN);
	}
	return {{SupplicantStatusCode::SUCCESS, ""}, bssid};
}

std::pair<SupplicantStatus, bool> P2pNetwork::isCurrentInternal()
{
	struct wpa_supplicant *wpa_s = retrieveIfacePtr();
	struct wpa_ssid *wpa_ssid = retrieveNetworkPtr();
	return {{SupplicantStatusCode::SUCCESS, ""},
		(wpa_s->current_ssid == wpa_ssid)};
}

std::pair<SupplicantStatus, bool> P2pNetwork::isPersistentInternal()
{
	struct wpa_ssid *wpa_ssid = retrieveNetworkPtr();
	return {{SupplicantStatusCode::SUCCESS, ""}, (wpa_ssid->disabled == 2)};
}

std::pair<SupplicantStatus, bool> P2pNetwork::isGoInternal()
{
	struct wpa_ssid *wpa_ssid = retrieveNetworkPtr();
	return {{SupplicantStatusCode::SUCCESS, ""},
		(wpa_ssid->mode == wpa_ssid::wpas_mode::WPAS_MODE_P2P_GO)};
}

SupplicantStatus P2pNetwork::setClientListInternal(
    const std::vector<hidl_array<uint8_t, 6>> &clients)
{
	struct wpa_ssid *wpa_ssid = retrieveNetworkPtr();
	os_free(wpa_ssid->p2p_client_list);
	// Internal representation uses a generic MAC addr/mask storage format
	// (even though the mask is always 0xFF'ed for p2p_client_list). So, the
	// first 6 bytes holds the client MAC address and the next 6 bytes are
	// OxFF'ed.
	wpa_ssid->p2p_client_list =
	    (u8 *)os_malloc(ETH_ALEN * 2 * clients.size());
	if (!wpa_ssid->p2p_client_list) {
		return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	}
	u8 *list = wpa_ssid->p2p_client_list;
	for (const auto &client : clients) {
		os_memcpy(list, client.data(), ETH_ALEN);
		list += ETH_ALEN;
		os_memset(list, 0xFF, ETH_ALEN);
		list += ETH_ALEN;
	}
	wpa_ssid->num_p2p_clients = clients.size();
	return {SupplicantStatusCode::SUCCESS, ""};
}

std::pair<SupplicantStatus, std::vector<hidl_array<uint8_t, 6>>>
P2pNetwork::getClientListInternal()
{
	struct wpa_ssid *wpa_ssid = retrieveNetworkPtr();
	if (!wpa_ssid->p2p_client_list) {
		return {{SupplicantStatusCode::FAILURE_UNKNOWN, ""}, {}};
	}
	std::vector<hidl_array<uint8_t, 6>> clients;
	u8 *list = wpa_ssid->p2p_client_list;
	for (size_t i = 0; i < wpa_ssid->num_p2p_clients; i++) {
		clients.emplace_back(list);
		list += 2 * ETH_ALEN;
	}
	return {{SupplicantStatusCode::SUCCESS, ""}, clients};
}

/**
 * Retrieve the underlying |wpa_ssid| struct pointer for
 * this network.
 * If the underlying network is removed or the interface
 * this network belong to is removed, all RPC method calls
 * on this object will return failure.
 */
struct wpa_ssid *P2pNetwork::retrieveNetworkPtr()
{
	wpa_supplicant *wpa_s = retrieveIfacePtr();
	if (!wpa_s)
		return nullptr;
	return wpa_config_get_network(wpa_s->conf, network_id_);
}

/**
 * Retrieve the underlying |wpa_supplicant| struct
 * pointer for this network.
 */
struct wpa_supplicant *P2pNetwork::retrieveIfacePtr()
{
	return wpa_supplicant_get_iface(
	    (struct wpa_global *)wpa_global_, ifname_.c_str());
}
}  // namespace implementation
}  // namespace V1_0
}  // namespace wifi
}  // namespace supplicant
}  // namespace hardware
}  // namespace android
