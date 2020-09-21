/*
 * hidl interface for wpa_supplicant daemon
 * Copyright (c) 2004-2016, Jouni Malinen <j@w1.fi>
 * Copyright (c) 2004-2016, Roshan Pius <rpius@google.com>
 * Copyright (C) 2017 Sony Mobile Communications Inc.
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include "hidl_manager.h"
#include "hidl_return_util.h"
#include "iface_config_utils.h"
#include "misc_utils.h"
#include "p2p_iface.h"

extern "C" {
#include "ap.h"
#include "wps_supplicant.h"
#include "wifi_display.h"
}

namespace {
const char kConfigMethodStrPbc[] = "pbc";
const char kConfigMethodStrDisplay[] = "display";
const char kConfigMethodStrKeypad[] = "keypad";
constexpr char kSetMiracastMode[] = "MIRACAST ";
constexpr uint8_t kWfdDeviceInfoSubelemId = 0;
constexpr char kWfdDeviceInfoSubelemLenHexStr[] = "0006";

using android::hardware::wifi::supplicant::V1_0::ISupplicantP2pIface;
uint8_t convertHidlMiracastModeToInternal(
    ISupplicantP2pIface::MiracastMode mode)
{
	switch (mode) {
	case ISupplicantP2pIface::MiracastMode::DISABLED:
		return 0;
	case ISupplicantP2pIface::MiracastMode::SOURCE:
		return 1;
	case ISupplicantP2pIface::MiracastMode::SINK:
		return 2;
	};
	WPA_ASSERT(false);
}
}  // namespace

namespace android {
namespace hardware {
namespace wifi {
namespace supplicant {
namespace V1_0 {
namespace implementation {
using hidl_return_util::validateAndCall;

P2pIface::P2pIface(struct wpa_global* wpa_global, const char ifname[])
    : wpa_global_(wpa_global), ifname_(ifname), is_valid_(true)
{
}

void P2pIface::invalidate() { is_valid_ = false; }
bool P2pIface::isValid()
{
	return (is_valid_ && (retrieveIfacePtr() != nullptr));
}
Return<void> P2pIface::getName(getName_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &P2pIface::getNameInternal, _hidl_cb);
}

Return<void> P2pIface::getType(getType_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &P2pIface::getTypeInternal, _hidl_cb);
}

Return<void> P2pIface::addNetwork(addNetwork_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &P2pIface::addNetworkInternal, _hidl_cb);
}

Return<void> P2pIface::removeNetwork(
    SupplicantNetworkId id, removeNetwork_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &P2pIface::removeNetworkInternal, _hidl_cb, id);
}

Return<void> P2pIface::getNetwork(
    SupplicantNetworkId id, getNetwork_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &P2pIface::getNetworkInternal, _hidl_cb, id);
}

Return<void> P2pIface::listNetworks(listNetworks_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &P2pIface::listNetworksInternal, _hidl_cb);
}

Return<void> P2pIface::registerCallback(
    const sp<ISupplicantP2pIfaceCallback>& callback,
    registerCallback_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &P2pIface::registerCallbackInternal, _hidl_cb, callback);
}

Return<void> P2pIface::getDeviceAddress(getDeviceAddress_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &P2pIface::getDeviceAddressInternal, _hidl_cb);
}

Return<void> P2pIface::setSsidPostfix(
    const hidl_vec<uint8_t>& postfix, setSsidPostfix_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &P2pIface::setSsidPostfixInternal, _hidl_cb, postfix);
}

Return<void> P2pIface::setGroupIdle(
    const hidl_string& group_ifname, uint32_t timeout_in_sec,
    setGroupIdle_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &P2pIface::setGroupIdleInternal, _hidl_cb, group_ifname,
	    timeout_in_sec);
}

Return<void> P2pIface::setPowerSave(
    const hidl_string& group_ifname, bool enable, setPowerSave_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &P2pIface::setPowerSaveInternal, _hidl_cb, group_ifname, enable);
}

Return<void> P2pIface::find(uint32_t timeout_in_sec, find_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &P2pIface::findInternal, _hidl_cb, timeout_in_sec);
}

Return<void> P2pIface::stopFind(stopFind_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &P2pIface::stopFindInternal, _hidl_cb);
}

Return<void> P2pIface::flush(flush_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &P2pIface::flushInternal, _hidl_cb);
}

Return<void> P2pIface::connect(
    const hidl_array<uint8_t, 6>& peer_address,
    ISupplicantP2pIface::WpsProvisionMethod provision_method,
    const hidl_string& pre_selected_pin, bool join_existing_group,
    bool persistent, uint32_t go_intent, connect_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &P2pIface::connectInternal, _hidl_cb, peer_address,
	    provision_method, pre_selected_pin, join_existing_group, persistent,
	    go_intent);
}

Return<void> P2pIface::cancelConnect(cancelConnect_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &P2pIface::cancelConnectInternal, _hidl_cb);
}

Return<void> P2pIface::provisionDiscovery(
    const hidl_array<uint8_t, 6>& peer_address,
    ISupplicantP2pIface::WpsProvisionMethod provision_method,
    provisionDiscovery_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &P2pIface::provisionDiscoveryInternal, _hidl_cb, peer_address,
	    provision_method);
}

Return<void> P2pIface::addGroup(
    bool persistent, SupplicantNetworkId persistent_network_id,
    addGroup_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &P2pIface::addGroupInternal, _hidl_cb, persistent,
	    persistent_network_id);
}

Return<void> P2pIface::removeGroup(
    const hidl_string& group_ifname, removeGroup_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &P2pIface::removeGroupInternal, _hidl_cb, group_ifname);
}

Return<void> P2pIface::reject(
    const hidl_array<uint8_t, 6>& peer_address, reject_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &P2pIface::rejectInternal, _hidl_cb, peer_address);
}

Return<void> P2pIface::invite(
    const hidl_string& group_ifname,
    const hidl_array<uint8_t, 6>& go_device_address,
    const hidl_array<uint8_t, 6>& peer_address, invite_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &P2pIface::inviteInternal, _hidl_cb, group_ifname,
	    go_device_address, peer_address);
}

Return<void> P2pIface::reinvoke(
    SupplicantNetworkId persistent_network_id,
    const hidl_array<uint8_t, 6>& peer_address, reinvoke_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &P2pIface::reinvokeInternal, _hidl_cb, persistent_network_id,
	    peer_address);
}

Return<void> P2pIface::configureExtListen(
    uint32_t period_in_millis, uint32_t interval_in_millis,
    configureExtListen_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &P2pIface::configureExtListenInternal, _hidl_cb, period_in_millis,
	    interval_in_millis);
}

Return<void> P2pIface::setListenChannel(
    uint32_t channel, uint32_t operating_class, setListenChannel_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &P2pIface::setListenChannelInternal, _hidl_cb, channel,
	    operating_class);
}

Return<void> P2pIface::setDisallowedFrequencies(
    const hidl_vec<FreqRange>& ranges, setDisallowedFrequencies_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &P2pIface::setDisallowedFrequenciesInternal, _hidl_cb, ranges);
}

Return<void> P2pIface::getSsid(
    const hidl_array<uint8_t, 6>& peer_address, getSsid_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &P2pIface::getSsidInternal, _hidl_cb, peer_address);
}

Return<void> P2pIface::getGroupCapability(
    const hidl_array<uint8_t, 6>& peer_address, getGroupCapability_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &P2pIface::getGroupCapabilityInternal, _hidl_cb, peer_address);
}

Return<void> P2pIface::addBonjourService(
    const hidl_vec<uint8_t>& query, const hidl_vec<uint8_t>& response,
    addBonjourService_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &P2pIface::addBonjourServiceInternal, _hidl_cb, query, response);
}

Return<void> P2pIface::removeBonjourService(
    const hidl_vec<uint8_t>& query, removeBonjourService_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &P2pIface::removeBonjourServiceInternal, _hidl_cb, query);
}

Return<void> P2pIface::addUpnpService(
    uint32_t version, const hidl_string& service_name,
    addUpnpService_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &P2pIface::addUpnpServiceInternal, _hidl_cb, version, service_name);
}

Return<void> P2pIface::removeUpnpService(
    uint32_t version, const hidl_string& service_name,
    removeUpnpService_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &P2pIface::removeUpnpServiceInternal, _hidl_cb, version,
	    service_name);
}

Return<void> P2pIface::flushServices(flushServices_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &P2pIface::flushServicesInternal, _hidl_cb);
}

Return<void> P2pIface::requestServiceDiscovery(
    const hidl_array<uint8_t, 6>& peer_address, const hidl_vec<uint8_t>& query,
    requestServiceDiscovery_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &P2pIface::requestServiceDiscoveryInternal, _hidl_cb, peer_address,
	    query);
}

Return<void> P2pIface::cancelServiceDiscovery(
    uint64_t identifier, cancelServiceDiscovery_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &P2pIface::cancelServiceDiscoveryInternal, _hidl_cb, identifier);
}

Return<void> P2pIface::setMiracastMode(
    ISupplicantP2pIface::MiracastMode mode, setMiracastMode_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &P2pIface::setMiracastModeInternal, _hidl_cb, mode);
}

Return<void> P2pIface::startWpsPbc(
    const hidl_string& group_ifname, const hidl_array<uint8_t, 6>& bssid,
    startWpsPbc_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &P2pIface::startWpsPbcInternal, _hidl_cb, group_ifname, bssid);
}

Return<void> P2pIface::startWpsPinKeypad(
    const hidl_string& group_ifname, const hidl_string& pin,
    startWpsPinKeypad_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &P2pIface::startWpsPinKeypadInternal, _hidl_cb, group_ifname, pin);
}

Return<void> P2pIface::startWpsPinDisplay(
    const hidl_string& group_ifname, const hidl_array<uint8_t, 6>& bssid,
    startWpsPinDisplay_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &P2pIface::startWpsPinDisplayInternal, _hidl_cb, group_ifname,
	    bssid);
}

Return<void> P2pIface::cancelWps(
    const hidl_string& group_ifname, cancelWps_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &P2pIface::cancelWpsInternal, _hidl_cb, group_ifname);
}

Return<void> P2pIface::setWpsDeviceName(
    const hidl_string& name, setWpsDeviceName_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &P2pIface::setWpsDeviceNameInternal, _hidl_cb, name);
}

Return<void> P2pIface::setWpsDeviceType(
    const hidl_array<uint8_t, 8>& type, setWpsDeviceType_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &P2pIface::setWpsDeviceTypeInternal, _hidl_cb, type);
}

Return<void> P2pIface::setWpsManufacturer(
    const hidl_string& manufacturer, setWpsManufacturer_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &P2pIface::setWpsManufacturerInternal, _hidl_cb, manufacturer);
}

Return<void> P2pIface::setWpsModelName(
    const hidl_string& model_name, setWpsModelName_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &P2pIface::setWpsModelNameInternal, _hidl_cb, model_name);
}

Return<void> P2pIface::setWpsModelNumber(
    const hidl_string& model_number, setWpsModelNumber_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &P2pIface::setWpsModelNumberInternal, _hidl_cb, model_number);
}

Return<void> P2pIface::setWpsSerialNumber(
    const hidl_string& serial_number, setWpsSerialNumber_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &P2pIface::setWpsSerialNumberInternal, _hidl_cb, serial_number);
}

Return<void> P2pIface::setWpsConfigMethods(
    uint16_t config_methods, setWpsConfigMethods_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &P2pIface::setWpsConfigMethodsInternal, _hidl_cb, config_methods);
}

Return<void> P2pIface::enableWfd(bool enable, enableWfd_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &P2pIface::enableWfdInternal, _hidl_cb, enable);
}

Return<void> P2pIface::setWfdDeviceInfo(
    const hidl_array<uint8_t, 6>& info, setWfdDeviceInfo_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &P2pIface::setWfdDeviceInfoInternal, _hidl_cb, info);
}

Return<void> P2pIface::createNfcHandoverRequestMessage(
    createNfcHandoverRequestMessage_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &P2pIface::createNfcHandoverRequestMessageInternal, _hidl_cb);
}

Return<void> P2pIface::createNfcHandoverSelectMessage(
    createNfcHandoverSelectMessage_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &P2pIface::createNfcHandoverSelectMessageInternal, _hidl_cb);
}

Return<void> P2pIface::reportNfcHandoverResponse(
    const hidl_vec<uint8_t>& request, reportNfcHandoverResponse_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &P2pIface::reportNfcHandoverResponseInternal, _hidl_cb, request);
}

Return<void> P2pIface::reportNfcHandoverInitiation(
    const hidl_vec<uint8_t>& select, reportNfcHandoverInitiation_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &P2pIface::reportNfcHandoverInitiationInternal, _hidl_cb, select);
}

Return<void> P2pIface::saveConfig(saveConfig_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &P2pIface::saveConfigInternal, _hidl_cb);
}

std::pair<SupplicantStatus, std::string> P2pIface::getNameInternal()
{
	return {{SupplicantStatusCode::SUCCESS, ""}, ifname_};
}

std::pair<SupplicantStatus, IfaceType> P2pIface::getTypeInternal()
{
	return {{SupplicantStatusCode::SUCCESS, ""}, IfaceType::P2P};
}

std::pair<SupplicantStatus, sp<ISupplicantP2pNetwork>>
P2pIface::addNetworkInternal()
{
	android::sp<ISupplicantP2pNetwork> network;
	struct wpa_supplicant* wpa_s = retrieveIfacePtr();
	struct wpa_ssid* ssid = wpa_supplicant_add_network(wpa_s);
	if (!ssid) {
		return {{SupplicantStatusCode::FAILURE_UNKNOWN, ""}, network};
	}
	HidlManager* hidl_manager = HidlManager::getInstance();
	if (!hidl_manager ||
	    hidl_manager->getP2pNetworkHidlObjectByIfnameAndNetworkId(
		wpa_s->ifname, ssid->id, &network)) {
		return {{SupplicantStatusCode::FAILURE_UNKNOWN, ""}, network};
	}
	return {{SupplicantStatusCode::SUCCESS, ""}, network};
}

SupplicantStatus P2pIface::removeNetworkInternal(SupplicantNetworkId id)
{
	struct wpa_supplicant* wpa_s = retrieveIfacePtr();
	int result = wpa_supplicant_remove_network(wpa_s, id);
	if (result == -1) {
		return {SupplicantStatusCode::FAILURE_NETWORK_UNKNOWN, ""};
	}
	if (result != 0) {
		return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	}
	return {SupplicantStatusCode::SUCCESS, ""};
}

std::pair<SupplicantStatus, sp<ISupplicantP2pNetwork>>
P2pIface::getNetworkInternal(SupplicantNetworkId id)
{
	android::sp<ISupplicantP2pNetwork> network;
	struct wpa_supplicant* wpa_s = retrieveIfacePtr();
	struct wpa_ssid* ssid = wpa_config_get_network(wpa_s->conf, id);
	if (!ssid) {
		return {{SupplicantStatusCode::FAILURE_NETWORK_UNKNOWN, ""},
			network};
	}
	HidlManager* hidl_manager = HidlManager::getInstance();
	if (!hidl_manager ||
	    hidl_manager->getP2pNetworkHidlObjectByIfnameAndNetworkId(
		wpa_s->ifname, ssid->id, &network)) {
		return {{SupplicantStatusCode::FAILURE_UNKNOWN, ""}, network};
	}
	return {{SupplicantStatusCode::SUCCESS, ""}, network};
}

std::pair<SupplicantStatus, std::vector<SupplicantNetworkId>>
P2pIface::listNetworksInternal()
{
	std::vector<SupplicantNetworkId> network_ids;
	struct wpa_supplicant* wpa_s = retrieveIfacePtr();
	for (struct wpa_ssid* wpa_ssid = wpa_s->conf->ssid; wpa_ssid;
	     wpa_ssid = wpa_ssid->next) {
		network_ids.emplace_back(wpa_ssid->id);
	}
	return {{SupplicantStatusCode::SUCCESS, ""}, std::move(network_ids)};
}

SupplicantStatus P2pIface::registerCallbackInternal(
    const sp<ISupplicantP2pIfaceCallback>& callback)
{
	HidlManager* hidl_manager = HidlManager::getInstance();
	if (!hidl_manager ||
	    hidl_manager->addP2pIfaceCallbackHidlObject(ifname_, callback)) {
		return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	}
	return {SupplicantStatusCode::SUCCESS, ""};
}

std::pair<SupplicantStatus, std::array<uint8_t, 6>>
P2pIface::getDeviceAddressInternal()
{
	struct wpa_supplicant* wpa_s = retrieveIfacePtr();
	std::array<uint8_t, 6> addr;
	static_assert(ETH_ALEN == addr.size(), "Size mismatch");
	os_memcpy(addr.data(), wpa_s->global->p2p_dev_addr, ETH_ALEN);
	return {{SupplicantStatusCode::SUCCESS, ""}, addr};
}

SupplicantStatus P2pIface::setSsidPostfixInternal(
    const std::vector<uint8_t>& postfix)
{
	struct wpa_supplicant* wpa_s = retrieveIfacePtr();
	if (p2p_set_ssid_postfix(
		wpa_s->global->p2p, postfix.data(), postfix.size())) {
		return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	}
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus P2pIface::setGroupIdleInternal(
    const std::string& group_ifname, uint32_t timeout_in_sec)
{
	struct wpa_supplicant* wpa_group_s =
	    retrieveGroupIfacePtr(group_ifname);
	if (!wpa_group_s) {
		return {SupplicantStatusCode::FAILURE_IFACE_UNKNOWN, ""};
	}
	wpa_group_s->conf->p2p_group_idle = timeout_in_sec;
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus P2pIface::setPowerSaveInternal(
    const std::string& group_ifname, bool enable)
{
	struct wpa_supplicant* wpa_group_s =
	    retrieveGroupIfacePtr(group_ifname);
	if (!wpa_group_s) {
		return {SupplicantStatusCode::FAILURE_IFACE_UNKNOWN, ""};
	}
	if (wpa_drv_set_p2p_powersave(wpa_group_s, enable, -1, -1)) {
		return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	}
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus P2pIface::findInternal(uint32_t timeout_in_sec)
{
	struct wpa_supplicant* wpa_s = retrieveIfacePtr();
	if (wpa_s->wpa_state == WPA_INTERFACE_DISABLED) {
		return {SupplicantStatusCode::FAILURE_IFACE_DISABLED, ""};
	}
	uint32_t search_delay = wpas_p2p_search_delay(wpa_s);
	if (wpas_p2p_find(
		wpa_s, timeout_in_sec, P2P_FIND_START_WITH_FULL, 0, nullptr,
		nullptr, search_delay, 0, nullptr, 0)) {
		return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	}
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus P2pIface::stopFindInternal()
{
	struct wpa_supplicant* wpa_s = retrieveIfacePtr();
	if (wpa_s->wpa_state == WPA_INTERFACE_DISABLED) {
		return {SupplicantStatusCode::FAILURE_IFACE_DISABLED, ""};
	}
	wpas_p2p_stop_find(wpa_s);
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus P2pIface::flushInternal()
{
	struct wpa_supplicant* wpa_s = retrieveIfacePtr();
	os_memset(wpa_s->p2p_auth_invite, 0, ETH_ALEN);
	wpa_s->force_long_sd = 0;
	wpas_p2p_stop_find(wpa_s);
	wpa_s->parent->p2ps_method_config_any = 0;
	if (wpa_s->global->p2p)
		p2p_flush(wpa_s->global->p2p);
	return {SupplicantStatusCode::SUCCESS, ""};
}

// This method only implements support for subset (needed by Android framework)
// of parameters that can be specified for connect.
std::pair<SupplicantStatus, std::string> P2pIface::connectInternal(
    const std::array<uint8_t, 6>& peer_address,
    ISupplicantP2pIface::WpsProvisionMethod provision_method,
    const std::string& pre_selected_pin, bool join_existing_group,
    bool persistent, uint32_t go_intent)
{
	struct wpa_supplicant* wpa_s = retrieveIfacePtr();
	if (go_intent > 15) {
		return {{SupplicantStatusCode::FAILURE_ARGS_INVALID, ""}, {}};
	}
	int go_intent_signed = join_existing_group ? -1 : go_intent;
	p2p_wps_method wps_method = {};
	switch (provision_method) {
	case WpsProvisionMethod::PBC:
		wps_method = WPS_PBC;
		break;
	case WpsProvisionMethod::DISPLAY:
		wps_method = WPS_PIN_DISPLAY;
		break;
	case WpsProvisionMethod::KEYPAD:
		wps_method = WPS_PIN_KEYPAD;
		break;
	}
	int vht = wpa_s->conf->p2p_go_vht;
	int ht40 = wpa_s->conf->p2p_go_ht40 || vht;
	const char* pin = pre_selected_pin.length() > 0 ? pre_selected_pin.data() : nullptr;
	int new_pin = wpas_p2p_connect(
	    wpa_s, peer_address.data(), pin, wps_method,
	    persistent, false, join_existing_group, false, go_intent_signed, 0, 0, -1,
	    false, ht40, vht, VHT_CHANWIDTH_USE_HT, nullptr, 0);
	if (new_pin < 0) {
		return {{SupplicantStatusCode::FAILURE_UNKNOWN, ""}, {}};
	}
	std::string pin_ret;
	if (provision_method == WpsProvisionMethod::DISPLAY &&
	    pre_selected_pin.empty()) {
		pin_ret = misc_utils::convertWpsPinToString(new_pin);
	}
	return {{SupplicantStatusCode::SUCCESS, ""}, pin_ret};
}

SupplicantStatus P2pIface::cancelConnectInternal()
{
	struct wpa_supplicant* wpa_s = retrieveIfacePtr();
	if (wpas_p2p_cancel(wpa_s)) {
		return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	}
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus P2pIface::provisionDiscoveryInternal(
    const std::array<uint8_t, 6>& peer_address,
    ISupplicantP2pIface::WpsProvisionMethod provision_method)
{
	struct wpa_supplicant* wpa_s = retrieveIfacePtr();
	p2ps_provision* prov_param;
	const char* config_method_str = nullptr;
	switch (provision_method) {
	case WpsProvisionMethod::PBC:
		config_method_str = kConfigMethodStrPbc;
		break;
	case WpsProvisionMethod::DISPLAY:
		config_method_str = kConfigMethodStrDisplay;
		break;
	case WpsProvisionMethod::KEYPAD:
		config_method_str = kConfigMethodStrKeypad;
		break;
	}
	if (wpas_p2p_prov_disc(
		wpa_s, peer_address.data(), config_method_str,
		WPAS_P2P_PD_FOR_GO_NEG, nullptr)) {
		return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	}
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus P2pIface::addGroupInternal(
    bool persistent, SupplicantNetworkId persistent_network_id)
{
	struct wpa_supplicant* wpa_s = retrieveIfacePtr();
	int vht = wpa_s->conf->p2p_go_vht;
	int ht40 = wpa_s->conf->p2p_go_ht40 || vht;
	struct wpa_ssid* ssid =
	    wpa_config_get_network(wpa_s->conf, persistent_network_id);
	if (ssid == NULL) {
		if (wpas_p2p_group_add(
			wpa_s, persistent, 0, 0, ht40, vht,
			VHT_CHANWIDTH_USE_HT)) {
			return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
		} else {
			return {SupplicantStatusCode::SUCCESS, ""};
		}
	} else if (ssid->disabled == 2) {
		if (wpas_p2p_group_add_persistent(
			wpa_s, ssid, 0, 0, 0, 0, ht40, vht,
			VHT_CHANWIDTH_USE_HT, NULL, 0, 0)) {
			return {SupplicantStatusCode::FAILURE_NETWORK_UNKNOWN,
				""};
		} else {
			return {SupplicantStatusCode::SUCCESS, ""};
		}
	}
	return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
}

SupplicantStatus P2pIface::removeGroupInternal(const std::string& group_ifname)
{
	struct wpa_supplicant* wpa_group_s =
	    retrieveGroupIfacePtr(group_ifname);
	if (!wpa_group_s) {
		return {SupplicantStatusCode::FAILURE_IFACE_UNKNOWN, ""};
	}
	if (wpas_p2p_group_remove(wpa_group_s, group_ifname.c_str())) {
		return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	}
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus P2pIface::rejectInternal(
    const std::array<uint8_t, 6>& peer_address)
{
	struct wpa_supplicant* wpa_s = retrieveIfacePtr();
	if (wpa_s->global->p2p_disabled || wpa_s->global->p2p == NULL) {
		return {SupplicantStatusCode::FAILURE_IFACE_DISABLED, ""};
	}
	if (wpas_p2p_reject(wpa_s, peer_address.data())) {
		return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	}
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus P2pIface::inviteInternal(
    const std::string& group_ifname,
    const std::array<uint8_t, 6>& go_device_address,
    const std::array<uint8_t, 6>& peer_address)
{
	struct wpa_supplicant* wpa_s = retrieveIfacePtr();
	if (wpas_p2p_invite_group(
		wpa_s, group_ifname.c_str(), peer_address.data(),
		go_device_address.data())) {
		return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	}
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus P2pIface::reinvokeInternal(
    SupplicantNetworkId persistent_network_id,
    const std::array<uint8_t, 6>& peer_address)
{
	struct wpa_supplicant* wpa_s = retrieveIfacePtr();
	int vht = wpa_s->conf->p2p_go_vht;
	int ht40 = wpa_s->conf->p2p_go_ht40 || vht;
	struct wpa_ssid* ssid =
	    wpa_config_get_network(wpa_s->conf, persistent_network_id);
	if (ssid == NULL || ssid->disabled != 2) {
		return {SupplicantStatusCode::FAILURE_NETWORK_UNKNOWN, ""};
	}
	if (wpas_p2p_invite(
		wpa_s, peer_address.data(), ssid, NULL, 0, 0, ht40, vht,
		VHT_CHANWIDTH_USE_HT, 0)) {
		return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	}
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus P2pIface::configureExtListenInternal(
    uint32_t period_in_millis, uint32_t interval_in_millis)
{
	struct wpa_supplicant* wpa_s = retrieveIfacePtr();
	if (wpas_p2p_ext_listen(wpa_s, period_in_millis, interval_in_millis)) {
		return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	}
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus P2pIface::setListenChannelInternal(
    uint32_t channel, uint32_t operating_class)
{
	struct wpa_supplicant* wpa_s = retrieveIfacePtr();
	if (p2p_set_listen_channel(
		wpa_s->global->p2p, operating_class, channel, 1)) {
		return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	}
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus P2pIface::setDisallowedFrequenciesInternal(
    const std::vector<FreqRange>& ranges)
{
	struct wpa_supplicant* wpa_s = retrieveIfacePtr();
	using DestT = struct wpa_freq_range_list::wpa_freq_range;
	DestT* freq_ranges = nullptr;
	// Empty ranges is used to enable all frequencies.
	if (ranges.size() != 0) {
		freq_ranges =
		    static_cast<DestT*>(os_malloc(sizeof(DestT) * ranges.size()));
		if (!freq_ranges) {
			return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
		}
		uint32_t i = 0;
		for (const auto& range : ranges) {
			freq_ranges[i].min = range.min;
			freq_ranges[i].max = range.max;
			i++;
		}
	}

	os_free(wpa_s->global->p2p_disallow_freq.range);
	wpa_s->global->p2p_disallow_freq.range = freq_ranges;
	wpa_s->global->p2p_disallow_freq.num = ranges.size();
	wpas_p2p_update_channel_list(wpa_s, WPAS_P2P_CHANNEL_UPDATE_DISALLOW);
	return {SupplicantStatusCode::SUCCESS, ""};
}

std::pair<SupplicantStatus, std::vector<uint8_t>> P2pIface::getSsidInternal(
    const std::array<uint8_t, 6>& peer_address)
{
	struct wpa_supplicant* wpa_s = retrieveIfacePtr();
	const struct p2p_peer_info* info =
	    p2p_get_peer_info(wpa_s->global->p2p, peer_address.data(), 0);
	if (!info) {
		return {{SupplicantStatusCode::FAILURE_UNKNOWN, ""}, {}};
	}
	const struct p2p_device* dev =
	    reinterpret_cast<const struct p2p_device*>(
		(reinterpret_cast<const uint8_t*>(info)) -
		offsetof(struct p2p_device, info));
	std::vector<uint8_t> ssid;
	if (dev && dev->oper_ssid_len) {
		ssid.assign(
		    dev->oper_ssid, dev->oper_ssid + dev->oper_ssid_len);
	}
	return {{SupplicantStatusCode::SUCCESS, ""}, ssid};
}

std::pair<SupplicantStatus, uint32_t> P2pIface::getGroupCapabilityInternal(
    const std::array<uint8_t, 6>& peer_address)
{
	struct wpa_supplicant* wpa_s = retrieveIfacePtr();
	const struct p2p_peer_info* info =
	    p2p_get_peer_info(wpa_s->global->p2p, peer_address.data(), 0);
	if (!info) {
		return {{SupplicantStatusCode::FAILURE_UNKNOWN, ""}, {}};
	}
	return {{SupplicantStatusCode::SUCCESS, ""}, info->group_capab};
}

SupplicantStatus P2pIface::addBonjourServiceInternal(
    const std::vector<uint8_t>& query, const std::vector<uint8_t>& response)
{
	struct wpa_supplicant* wpa_s = retrieveIfacePtr();
	auto query_buf = misc_utils::convertVectorToWpaBuf(query);
	auto response_buf = misc_utils::convertVectorToWpaBuf(response);
	if (!query_buf || !response_buf) {
		return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	}
	if (wpas_p2p_service_add_bonjour(
		wpa_s, query_buf.get(), response_buf.get())) {
		return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	}
	// If successful, the wpabuf is referenced internally and hence should
	// not be freed.
	query_buf.release();
	response_buf.release();
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus P2pIface::removeBonjourServiceInternal(
    const std::vector<uint8_t>& query)
{
	struct wpa_supplicant* wpa_s = retrieveIfacePtr();
	auto query_buf = misc_utils::convertVectorToWpaBuf(query);
	if (!query_buf) {
		return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	}
	if (wpas_p2p_service_del_bonjour(wpa_s, query_buf.get())) {
		return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	}
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus P2pIface::addUpnpServiceInternal(
    uint32_t version, const std::string& service_name)
{
	struct wpa_supplicant* wpa_s = retrieveIfacePtr();
	if (wpas_p2p_service_add_upnp(wpa_s, version, service_name.c_str())) {
		return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	}
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus P2pIface::removeUpnpServiceInternal(
    uint32_t version, const std::string& service_name)
{
	struct wpa_supplicant* wpa_s = retrieveIfacePtr();
	if (wpas_p2p_service_del_upnp(wpa_s, version, service_name.c_str())) {
		return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	}
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus P2pIface::flushServicesInternal()
{
	struct wpa_supplicant* wpa_s = retrieveIfacePtr();
	wpas_p2p_service_flush(wpa_s);
	return {SupplicantStatusCode::SUCCESS, ""};
}

std::pair<SupplicantStatus, uint64_t> P2pIface::requestServiceDiscoveryInternal(
    const std::array<uint8_t, 6>& peer_address,
    const std::vector<uint8_t>& query)
{
	struct wpa_supplicant* wpa_s = retrieveIfacePtr();
	auto query_buf = misc_utils::convertVectorToWpaBuf(query);
	if (!query_buf) {
		return {{SupplicantStatusCode::FAILURE_UNKNOWN, ""}, {}};
	}
	const uint8_t* dst_addr = is_zero_ether_addr(peer_address.data())
				      ? nullptr
				      : peer_address.data();
	uint64_t identifier =
	    wpas_p2p_sd_request(wpa_s, dst_addr, query_buf.get());
	if (identifier == 0) {
		return {{SupplicantStatusCode::FAILURE_UNKNOWN, ""}, {}};
	}
	return {{SupplicantStatusCode::SUCCESS, ""}, identifier};
}

SupplicantStatus P2pIface::cancelServiceDiscoveryInternal(uint64_t identifier)
{
	struct wpa_supplicant* wpa_s = retrieveIfacePtr();
	if (wpas_p2p_sd_cancel_request(wpa_s, identifier)) {
		return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	}
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus P2pIface::setMiracastModeInternal(
    ISupplicantP2pIface::MiracastMode mode)
{
	struct wpa_supplicant* wpa_s = retrieveIfacePtr();
	uint8_t mode_internal = convertHidlMiracastModeToInternal(mode);
	const std::string cmd_str =
	    kSetMiracastMode + std::to_string(mode_internal);
	std::vector<char> cmd(
	    cmd_str.c_str(), cmd_str.c_str() + cmd_str.size() + 1);
	char driver_cmd_reply_buf[4096] = {};
	if (wpa_drv_driver_cmd(
		wpa_s, cmd.data(), driver_cmd_reply_buf,
		sizeof(driver_cmd_reply_buf))) {
		return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	}
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus P2pIface::startWpsPbcInternal(
    const std::string& group_ifname, const std::array<uint8_t, 6>& bssid)
{
	struct wpa_supplicant* wpa_group_s =
	    retrieveGroupIfacePtr(group_ifname);
	if (!wpa_group_s) {
		return {SupplicantStatusCode::FAILURE_IFACE_UNKNOWN, ""};
	}
	const uint8_t* bssid_addr =
	    is_zero_ether_addr(bssid.data()) ? nullptr : bssid.data();
#ifdef CONFIG_AP
	if (wpa_group_s->ap_iface) {
		if (wpa_supplicant_ap_wps_pbc(wpa_group_s, bssid_addr, NULL)) {
			return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
		}
		return {SupplicantStatusCode::SUCCESS, ""};
	}
#endif /* CONFIG_AP */
	if (wpas_wps_start_pbc(wpa_group_s, bssid_addr, 0)) {
		return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	}
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus P2pIface::startWpsPinKeypadInternal(
    const std::string& group_ifname, const std::string& pin)
{
	struct wpa_supplicant* wpa_group_s =
	    retrieveGroupIfacePtr(group_ifname);
	if (!wpa_group_s) {
		return {SupplicantStatusCode::FAILURE_IFACE_UNKNOWN, ""};
	}
#ifdef CONFIG_AP
	if (wpa_group_s->ap_iface) {
		if (wpa_supplicant_ap_wps_pin(
				wpa_group_s, nullptr, pin.c_str(), nullptr, 0, 0) < 0) {
			return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
		}
		return {SupplicantStatusCode::SUCCESS, ""};
	}
#endif /* CONFIG_AP */
	if (wpas_wps_start_pin(
		wpa_group_s, nullptr, pin.c_str(), 0, DEV_PW_DEFAULT)) {
		return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	}
	return {SupplicantStatusCode::SUCCESS, ""};
}

std::pair<SupplicantStatus, std::string> P2pIface::startWpsPinDisplayInternal(
    const std::string& group_ifname, const std::array<uint8_t, 6>& bssid)
{
	struct wpa_supplicant* wpa_group_s =
	    retrieveGroupIfacePtr(group_ifname);
	if (!wpa_group_s) {
		return {{SupplicantStatusCode::FAILURE_IFACE_UNKNOWN, ""}, ""};
	}
	const uint8_t* bssid_addr =
	    is_zero_ether_addr(bssid.data()) ? nullptr : bssid.data();
	int pin = wpas_wps_start_pin(
	    wpa_group_s, bssid_addr, nullptr, 0, DEV_PW_DEFAULT);
	if (pin < 0) {
		return {{SupplicantStatusCode::FAILURE_UNKNOWN, ""}, ""};
	}
	return {{SupplicantStatusCode::SUCCESS, ""},
		misc_utils::convertWpsPinToString(pin)};
}

SupplicantStatus P2pIface::cancelWpsInternal(const std::string& group_ifname)
{
	struct wpa_supplicant* wpa_group_s =
	    retrieveGroupIfacePtr(group_ifname);
	if (!wpa_group_s) {
		return {SupplicantStatusCode::FAILURE_IFACE_UNKNOWN, ""};
	}
	if (wpas_wps_cancel(wpa_group_s)) {
		return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	}
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus P2pIface::setWpsDeviceNameInternal(const std::string& name)
{
	return iface_config_utils::setWpsDeviceName(retrieveIfacePtr(), name);
}

SupplicantStatus P2pIface::setWpsDeviceTypeInternal(
    const std::array<uint8_t, 8>& type)
{
	return iface_config_utils::setWpsDeviceType(retrieveIfacePtr(), type);
}

SupplicantStatus P2pIface::setWpsManufacturerInternal(
    const std::string& manufacturer)
{
	return iface_config_utils::setWpsManufacturer(
	    retrieveIfacePtr(), manufacturer);
}

SupplicantStatus P2pIface::setWpsModelNameInternal(
    const std::string& model_name)
{
	return iface_config_utils::setWpsModelName(
	    retrieveIfacePtr(), model_name);
}

SupplicantStatus P2pIface::setWpsModelNumberInternal(
    const std::string& model_number)
{
	return iface_config_utils::setWpsModelNumber(
	    retrieveIfacePtr(), model_number);
}

SupplicantStatus P2pIface::setWpsSerialNumberInternal(
    const std::string& serial_number)
{
	return iface_config_utils::setWpsSerialNumber(
	    retrieveIfacePtr(), serial_number);
}

SupplicantStatus P2pIface::setWpsConfigMethodsInternal(uint16_t config_methods)
{
	return iface_config_utils::setWpsConfigMethods(
	    retrieveIfacePtr(), config_methods);
}

SupplicantStatus P2pIface::enableWfdInternal(bool enable)
{
	struct wpa_supplicant* wpa_s = retrieveIfacePtr();
	wifi_display_enable(wpa_s->global, enable);
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus P2pIface::setWfdDeviceInfoInternal(
    const std::array<uint8_t, 6>& info)
{
	struct wpa_supplicant* wpa_s = retrieveIfacePtr();
	std::vector<char> wfd_device_info_hex(info.size() * 2 + 1);
	wpa_snprintf_hex(
	    wfd_device_info_hex.data(), wfd_device_info_hex.size(), info.data(),
	    info.size());
	// |wifi_display_subelem_set| expects the first 2 bytes
	// to hold the lenght of the subelement. In this case it's
	// fixed to 6, so prepend that.
	std::string wfd_device_info_set_cmd_str =
	    std::to_string(kWfdDeviceInfoSubelemId) + " " +
	    kWfdDeviceInfoSubelemLenHexStr + wfd_device_info_hex.data();
	std::vector<char> wfd_device_info_set_cmd(
	    wfd_device_info_set_cmd_str.c_str(),
	    wfd_device_info_set_cmd_str.c_str() +
		wfd_device_info_set_cmd_str.size() + 1);
	if (wifi_display_subelem_set(
		wpa_s->global, wfd_device_info_set_cmd.data())) {
		return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	}
	return {SupplicantStatusCode::SUCCESS, ""};
}

std::pair<SupplicantStatus, std::vector<uint8_t>>
P2pIface::createNfcHandoverRequestMessageInternal()
{
	struct wpa_supplicant* wpa_s = retrieveIfacePtr();
	auto buf = misc_utils::createWpaBufUniquePtr(
	    wpas_p2p_nfc_handover_req(wpa_s, 1));
	if (!buf) {
		return {{SupplicantStatusCode::FAILURE_UNKNOWN, ""}, {}};
	}
	return {{SupplicantStatusCode::SUCCESS, ""},
		misc_utils::convertWpaBufToVector(buf.get())};
}

std::pair<SupplicantStatus, std::vector<uint8_t>>
P2pIface::createNfcHandoverSelectMessageInternal()
{
	struct wpa_supplicant* wpa_s = retrieveIfacePtr();
	auto buf = misc_utils::createWpaBufUniquePtr(
	    wpas_p2p_nfc_handover_sel(wpa_s, 1, 0));
	if (!buf) {
		return {{SupplicantStatusCode::FAILURE_UNKNOWN, ""}, {}};
	}
	return {{SupplicantStatusCode::SUCCESS, ""},
		misc_utils::convertWpaBufToVector(buf.get())};
}

SupplicantStatus P2pIface::reportNfcHandoverResponseInternal(
    const std::vector<uint8_t>& request)
{
	struct wpa_supplicant* wpa_s = retrieveIfacePtr();
	auto req = misc_utils::convertVectorToWpaBuf(request);
	auto sel = misc_utils::convertVectorToWpaBuf(std::vector<uint8_t>{0});
	if (!req || !sel) {
		return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	}

	if (wpas_p2p_nfc_report_handover(wpa_s, 0, req.get(), sel.get(), 0)) {
		return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	}
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus P2pIface::reportNfcHandoverInitiationInternal(
    const std::vector<uint8_t>& select)
{
	struct wpa_supplicant* wpa_s = retrieveIfacePtr();
	auto req = misc_utils::convertVectorToWpaBuf(std::vector<uint8_t>{0});
	auto sel = misc_utils::convertVectorToWpaBuf(select);
	if (!req || !sel) {
		return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	}

	if (wpas_p2p_nfc_report_handover(wpa_s, 1, req.get(), sel.get(), 0)) {
		return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	}
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus P2pIface::saveConfigInternal()
{
	struct wpa_supplicant* wpa_s = retrieveIfacePtr();
	if (!wpa_s->conf->update_config) {
		return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	}
	if (wpa_config_write(wpa_s->confname, wpa_s->conf)) {
		return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	}
	return {SupplicantStatusCode::SUCCESS, ""};
}

/**
 * Retrieve the underlying |wpa_supplicant| struct
 * pointer for this iface.
 * If the underlying iface is removed, then all RPC method calls on this object
 * will return failure.
 */
wpa_supplicant* P2pIface::retrieveIfacePtr()
{
	return wpa_supplicant_get_iface(wpa_global_, ifname_.c_str());
}

/**
 * Retrieve the underlying |wpa_supplicant| struct
 * pointer for this group iface.
 */
wpa_supplicant* P2pIface::retrieveGroupIfacePtr(const std::string& group_ifname)
{
	return wpa_supplicant_get_iface(wpa_global_, group_ifname.c_str());
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace wifi
}  // namespace supplicant
}  // namespace hardware
}  // namespace android
