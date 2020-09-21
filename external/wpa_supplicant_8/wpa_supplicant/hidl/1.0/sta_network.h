/*
 * hidl interface for wpa_supplicant daemon
 * Copyright (c) 2004-2016, Jouni Malinen <j@w1.fi>
 * Copyright (c) 2004-2016, Roshan Pius <rpius@google.com>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#ifndef WPA_SUPPLICANT_HIDL_STA_NETWORK_H
#define WPA_SUPPLICANT_HIDL_STA_NETWORK_H

#include <array>
#include <vector>

#include <android-base/macros.h>

#include <android/hardware/wifi/supplicant/1.0/ISupplicantStaNetwork.h>
#include <android/hardware/wifi/supplicant/1.0/ISupplicantStaNetworkCallback.h>

extern "C" {
#include "utils/common.h"
#include "utils/includes.h"
#include "config.h"
#include "wpa_supplicant_i.h"
#include "notify.h"
#include "eapol_supp/eapol_supp_sm.h"
#include "eap_peer/eap.h"
#include "rsn_supp/wpa.h"
}

namespace android {
namespace hardware {
namespace wifi {
namespace supplicant {
namespace V1_0 {
namespace implementation {

/**
 * Implementation of StaNetwork hidl object. Each unique hidl
 * object is used for control operations on a specific network
 * controlled by wpa_supplicant.
 */
class StaNetwork : public ISupplicantStaNetwork
{
public:
	StaNetwork(
	    struct wpa_global* wpa_global, const char ifname[], int network_id);
	~StaNetwork() override = default;
	// Refer to |StaIface::invalidate()|.
	void invalidate();
	bool isValid();

	// Hidl methods exposed.
	Return<void> getId(getId_cb _hidl_cb) override;
	Return<void> getInterfaceName(getInterfaceName_cb _hidl_cb) override;
	Return<void> getType(getType_cb _hidl_cb) override;
	Return<void> registerCallback(
	    const sp<ISupplicantStaNetworkCallback>& callback,
	    registerCallback_cb _hidl_cb) override;
	Return<void> setSsid(
	    const hidl_vec<uint8_t>& ssid, setSsid_cb _hidl_cb) override;
	Return<void> setBssid(
	    const hidl_array<uint8_t, 6>& bssid, setBssid_cb _hidl_cb) override;
	Return<void> setScanSsid(bool enable, setScanSsid_cb _hidl_cb) override;
	Return<void> setKeyMgmt(
	    uint32_t key_mgmt_mask, setKeyMgmt_cb _hidl_cb) override;
	Return<void> setProto(
	    uint32_t proto_mask, setProto_cb _hidl_cb) override;
	Return<void> setAuthAlg(
	    uint32_t auth_alg_mask, setAuthAlg_cb _hidl_cb) override;
	Return<void> setGroupCipher(
	    uint32_t group_cipher_mask, setGroupCipher_cb _hidl_cb) override;
	Return<void> setPairwiseCipher(
	    uint32_t pairwise_cipher_mask,
	    setPairwiseCipher_cb _hidl_cb) override;
	Return<void> setPskPassphrase(
	    const hidl_string& psk, setPskPassphrase_cb _hidl_cb) override;
	Return<void> setPsk(
	    const hidl_array<uint8_t, 32>& psk, setPsk_cb _hidl_cb) override;
	Return<void> setWepKey(
	    uint32_t key_idx, const hidl_vec<uint8_t>& wep_key,
	    setWepKey_cb _hidl_cb) override;
	Return<void> setWepTxKeyIdx(
	    uint32_t key_idx, setWepTxKeyIdx_cb _hidl_cb) override;
	Return<void> setRequirePmf(
	    bool enable, setRequirePmf_cb _hidl_cb) override;
	Return<void> setEapMethod(
	    ISupplicantStaNetwork::EapMethod method,
	    setEapMethod_cb _hidl_cb) override;
	Return<void> setEapPhase2Method(
	    ISupplicantStaNetwork::EapPhase2Method method,
	    setEapPhase2Method_cb _hidl_cb) override;
	Return<void> setEapIdentity(
	    const hidl_vec<uint8_t>& identity,
	    setEapIdentity_cb _hidl_cb) override;
	Return<void> setEapAnonymousIdentity(
	    const hidl_vec<uint8_t>& identity,
	    setEapAnonymousIdentity_cb _hidl_cb) override;
	Return<void> setEapPassword(
	    const hidl_vec<uint8_t>& password,
	    setEapPassword_cb _hidl_cb) override;
	Return<void> setEapCACert(
	    const hidl_string& path, setEapCACert_cb _hidl_cb) override;
	Return<void> setEapCAPath(
	    const hidl_string& path, setEapCAPath_cb _hidl_cb) override;
	Return<void> setEapClientCert(
	    const hidl_string& path, setEapClientCert_cb _hidl_cb) override;
	Return<void> setEapPrivateKeyId(
	    const hidl_string& id, setEapPrivateKeyId_cb _hidl_cb) override;
	Return<void> setEapSubjectMatch(
	    const hidl_string& match, setEapSubjectMatch_cb _hidl_cb) override;
	Return<void> setEapAltSubjectMatch(
	    const hidl_string& match,
	    setEapAltSubjectMatch_cb _hidl_cb) override;
	Return<void> setEapEngine(
	    bool enable, setEapEngine_cb _hidl_cb) override;
	Return<void> setEapEngineID(
	    const hidl_string& id, setEapEngineID_cb _hidl_cb) override;
	Return<void> setEapDomainSuffixMatch(
	    const hidl_string& match,
	    setEapDomainSuffixMatch_cb _hidl_cb) override;
	Return<void> setProactiveKeyCaching(
	    bool enable, setProactiveKeyCaching_cb _hidl_cb) override;
	Return<void> setIdStr(
	    const hidl_string& id_str, setIdStr_cb _hidl_cb) override;
	Return<void> setUpdateIdentifier(
	    uint32_t id, setUpdateIdentifier_cb _hidl_cb) override;
	Return<void> getSsid(getSsid_cb _hidl_cb) override;
	Return<void> getBssid(getBssid_cb _hidl_cb) override;
	Return<void> getScanSsid(getScanSsid_cb _hidl_cb) override;
	Return<void> getKeyMgmt(getKeyMgmt_cb _hidl_cb) override;
	Return<void> getProto(getProto_cb _hidl_cb) override;
	Return<void> getAuthAlg(getAuthAlg_cb _hidl_cb) override;
	Return<void> getGroupCipher(getGroupCipher_cb _hidl_cb) override;
	Return<void> getPairwiseCipher(getPairwiseCipher_cb _hidl_cb) override;
	Return<void> getPskPassphrase(getPskPassphrase_cb _hidl_cb) override;
	Return<void> getPsk(getPsk_cb _hidl_cb) override;
	Return<void> getWepKey(
	    uint32_t key_idx, getWepKey_cb _hidl_cb) override;
	Return<void> getWepTxKeyIdx(getWepTxKeyIdx_cb _hidl_cb) override;
	Return<void> getRequirePmf(getRequirePmf_cb _hidl_cb) override;
	Return<void> getEapMethod(getEapMethod_cb _hidl_cb) override;
	Return<void> getEapPhase2Method(
	    getEapPhase2Method_cb _hidl_cb) override;
	Return<void> getEapIdentity(getEapIdentity_cb _hidl_cb) override;
	Return<void> getEapAnonymousIdentity(
	    getEapAnonymousIdentity_cb _hidl_cb) override;
	Return<void> getEapPassword(getEapPassword_cb _hidl_cb) override;
	Return<void> getEapCACert(getEapCACert_cb _hidl_cb) override;
	Return<void> getEapCAPath(getEapCAPath_cb _hidl_cb) override;
	Return<void> getEapClientCert(getEapClientCert_cb _hidl_cb) override;
	Return<void> getEapPrivateKeyId(
	    getEapPrivateKeyId_cb _hidl_cb) override;
	Return<void> getEapSubjectMatch(
	    getEapSubjectMatch_cb _hidl_cb) override;
	Return<void> getEapAltSubjectMatch(
	    getEapAltSubjectMatch_cb _hidl_cb) override;
	Return<void> getEapEngine(getEapEngine_cb _hidl_cb) override;
	Return<void> getEapEngineID(getEapEngineID_cb _hidl_cb) override;
	Return<void> getEapDomainSuffixMatch(
	    getEapDomainSuffixMatch_cb _hidl_cb) override;
	Return<void> getIdStr(getIdStr_cb _hidl_cb) override;
	Return<void> getWpsNfcConfigurationToken(
	    getWpsNfcConfigurationToken_cb _hidl_cb) override;
	Return<void> enable(bool no_connect, enable_cb _hidl_cb) override;
	Return<void> disable(disable_cb _hidl_cb) override;
	Return<void> select(select_cb _hidl_cb) override;
	Return<void> sendNetworkEapSimGsmAuthResponse(
	    const hidl_vec<
		ISupplicantStaNetwork::NetworkResponseEapSimGsmAuthParams>&
		vec_params,
	    sendNetworkEapSimGsmAuthResponse_cb _hidl_cb) override;
	Return<void> sendNetworkEapSimGsmAuthFailure(
	    sendNetworkEapSimGsmAuthFailure_cb _hidl_cb) override;
	Return<void> sendNetworkEapSimUmtsAuthResponse(
	    const ISupplicantStaNetwork::NetworkResponseEapSimUmtsAuthParams&
		params,
	    sendNetworkEapSimUmtsAuthResponse_cb _hidl_cb) override;
	Return<void> sendNetworkEapSimUmtsAutsResponse(
	    const hidl_array<uint8_t, 14>& auts,
	    sendNetworkEapSimUmtsAutsResponse_cb _hidl_cb) override;
	Return<void> sendNetworkEapSimUmtsAuthFailure(
	    sendNetworkEapSimUmtsAuthFailure_cb _hidl_cb) override;
	Return<void> sendNetworkEapIdentityResponse(
	    const hidl_vec<uint8_t>& identity,
	    sendNetworkEapIdentityResponse_cb _hidl_cb) override;

private:
	// Corresponding worker functions for the HIDL methods.
	std::pair<SupplicantStatus, uint32_t> getIdInternal();
	std::pair<SupplicantStatus, std::string> getInterfaceNameInternal();
	std::pair<SupplicantStatus, IfaceType> getTypeInternal();
	SupplicantStatus registerCallbackInternal(
	    const sp<ISupplicantStaNetworkCallback>& callback);
	SupplicantStatus setSsidInternal(const std::vector<uint8_t>& ssid);
	SupplicantStatus setBssidInternal(const std::array<uint8_t, 6>& bssid);
	SupplicantStatus setScanSsidInternal(bool enable);
	SupplicantStatus setKeyMgmtInternal(uint32_t key_mgmt_mask);
	SupplicantStatus setProtoInternal(uint32_t proto_mask);
	SupplicantStatus setAuthAlgInternal(uint32_t auth_alg_mask);
	SupplicantStatus setGroupCipherInternal(uint32_t group_cipher_mask);
	SupplicantStatus setPairwiseCipherInternal(
	    uint32_t pairwise_cipher_mask);
	SupplicantStatus setPskPassphraseInternal(const std::string& psk);
	SupplicantStatus setPskInternal(const std::array<uint8_t, 32>& psk);
	SupplicantStatus setWepKeyInternal(
	    uint32_t key_idx, const std::vector<uint8_t>& wep_key);
	SupplicantStatus setWepTxKeyIdxInternal(uint32_t key_idx);
	SupplicantStatus setRequirePmfInternal(bool enable);
	SupplicantStatus setEapMethodInternal(
	    ISupplicantStaNetwork::EapMethod method);
	SupplicantStatus setEapPhase2MethodInternal(
	    ISupplicantStaNetwork::EapPhase2Method method);
	SupplicantStatus setEapIdentityInternal(
	    const std::vector<uint8_t>& identity);
	SupplicantStatus setEapAnonymousIdentityInternal(
	    const std::vector<uint8_t>& identity);
	SupplicantStatus setEapPasswordInternal(
	    const std::vector<uint8_t>& password);
	SupplicantStatus setEapCACertInternal(const std::string& path);
	SupplicantStatus setEapCAPathInternal(const std::string& path);
	SupplicantStatus setEapClientCertInternal(const std::string& path);
	SupplicantStatus setEapPrivateKeyIdInternal(const std::string& id);
	SupplicantStatus setEapSubjectMatchInternal(const std::string& match);
	SupplicantStatus setEapAltSubjectMatchInternal(
	    const std::string& match);
	SupplicantStatus setEapEngineInternal(bool enable);
	SupplicantStatus setEapEngineIDInternal(const std::string& id);
	SupplicantStatus setEapDomainSuffixMatchInternal(
	    const std::string& match);
	SupplicantStatus setProactiveKeyCachingInternal(bool enable);
	SupplicantStatus setIdStrInternal(const std::string& id_str);
	SupplicantStatus setUpdateIdentifierInternal(uint32_t id);
	std::pair<SupplicantStatus, std::vector<uint8_t>> getSsidInternal();
	std::pair<SupplicantStatus, std::array<uint8_t, 6>> getBssidInternal();
	std::pair<SupplicantStatus, bool> getScanSsidInternal();
	std::pair<SupplicantStatus, uint32_t> getKeyMgmtInternal();
	std::pair<SupplicantStatus, uint32_t> getProtoInternal();
	std::pair<SupplicantStatus, uint32_t> getAuthAlgInternal();
	std::pair<SupplicantStatus, uint32_t> getGroupCipherInternal();
	std::pair<SupplicantStatus, uint32_t> getPairwiseCipherInternal();
	std::pair<SupplicantStatus, std::string> getPskPassphraseInternal();
	std::pair<SupplicantStatus, std::array<uint8_t, 32>> getPskInternal();
	std::pair<SupplicantStatus, std::vector<uint8_t>> getWepKeyInternal(
	    uint32_t key_idx);
	std::pair<SupplicantStatus, uint32_t> getWepTxKeyIdxInternal();
	std::pair<SupplicantStatus, bool> getRequirePmfInternal();
	std::pair<SupplicantStatus, ISupplicantStaNetwork::EapMethod>
	getEapMethodInternal();
	std::pair<SupplicantStatus, ISupplicantStaNetwork::EapPhase2Method>
	getEapPhase2MethodInternal();
	std::pair<SupplicantStatus, std::vector<uint8_t>>
	getEapIdentityInternal();
	std::pair<SupplicantStatus, std::vector<uint8_t>>
	getEapAnonymousIdentityInternal();
	std::pair<SupplicantStatus, std::vector<uint8_t>>
	getEapPasswordInternal();
	std::pair<SupplicantStatus, std::string> getEapCACertInternal();
	std::pair<SupplicantStatus, std::string> getEapCAPathInternal();
	std::pair<SupplicantStatus, std::string> getEapClientCertInternal();
	std::pair<SupplicantStatus, std::string> getEapPrivateKeyIdInternal();
	std::pair<SupplicantStatus, std::string> getEapSubjectMatchInternal();
	std::pair<SupplicantStatus, std::string>
	getEapAltSubjectMatchInternal();
	std::pair<SupplicantStatus, bool> getEapEngineInternal();
	std::pair<SupplicantStatus, std::string> getEapEngineIDInternal();
	std::pair<SupplicantStatus, std::string>
	getEapDomainSuffixMatchInternal();
	std::pair<SupplicantStatus, std::string> getIdStrInternal();
	std::pair<SupplicantStatus, std::vector<uint8_t>>
	getWpsNfcConfigurationTokenInternal();
	SupplicantStatus enableInternal(bool no_connect);
	SupplicantStatus disableInternal();
	SupplicantStatus selectInternal();
	SupplicantStatus sendNetworkEapSimGsmAuthResponseInternal(
	    const std::vector<
		ISupplicantStaNetwork::NetworkResponseEapSimGsmAuthParams>&
		vec_params);
	SupplicantStatus sendNetworkEapSimGsmAuthFailureInternal();
	SupplicantStatus sendNetworkEapSimUmtsAuthResponseInternal(
	    const ISupplicantStaNetwork::NetworkResponseEapSimUmtsAuthParams&
		params);
	SupplicantStatus sendNetworkEapSimUmtsAutsResponseInternal(
	    const std::array<uint8_t, 14>& auts);
	SupplicantStatus sendNetworkEapSimUmtsAuthFailureInternal();
	SupplicantStatus sendNetworkEapIdentityResponseInternal(
	    const std::vector<uint8_t>& identity);

	struct wpa_ssid* retrieveNetworkPtr();
	struct wpa_supplicant* retrieveIfacePtr();
	int isPskPassphraseValid(const std::string& psk);
	void resetInternalStateAfterParamsUpdate();
	int setStringFieldAndResetState(
	    const char* value, uint8_t** to_update_field,
	    const char* hexdump_prefix);
	int setStringFieldAndResetState(
	    const char* value, char** to_update_field,
	    const char* hexdump_prefix);
	int setStringKeyFieldAndResetState(
	    const char* value, char** to_update_field,
	    const char* hexdump_prefix);
	int setByteArrayFieldAndResetState(
	    const uint8_t* value, const size_t value_len,
	    uint8_t** to_update_field, size_t* to_update_field_len,
	    const char* hexdump_prefix);
	int setByteArrayKeyFieldAndResetState(
	    const uint8_t* value, const size_t value_len,
	    uint8_t** to_update_field, size_t* to_update_field_len,
	    const char* hexdump_prefix);

	// Reference to the global wpa_struct. This is assumed to be valid
	// for the lifetime of the process.
	struct wpa_global* wpa_global_;
	// Name of the iface this network belongs to.
	const std::string ifname_;
	// Id of the network this hidl object controls.
	const int network_id_;
	bool is_valid_;

	DISALLOW_COPY_AND_ASSIGN(StaNetwork);
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace wifi
}  // namespace supplicant
}  // namespace hardware
}  // namespace android

#endif  // WPA_SUPPLICANT_HIDL_STA_NETWORK_H
