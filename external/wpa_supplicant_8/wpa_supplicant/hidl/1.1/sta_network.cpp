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
#include "misc_utils.h"
#include "sta_network.h"

extern "C" {
#include "wps_supplicant.h"
}

namespace {
using android::hardware::wifi::supplicant::V1_0::ISupplicantStaNetwork;
using android::hardware::wifi::supplicant::V1_0::SupplicantStatus;

constexpr uint8_t kZeroBssid[6] = {0, 0, 0, 0, 0, 0};

constexpr uint32_t kAllowedKeyMgmtMask =
    (static_cast<uint32_t>(ISupplicantStaNetwork::KeyMgmtMask::NONE) |
     static_cast<uint32_t>(ISupplicantStaNetwork::KeyMgmtMask::WPA_PSK) |
     static_cast<uint32_t>(ISupplicantStaNetwork::KeyMgmtMask::WPA_EAP) |
     static_cast<uint32_t>(ISupplicantStaNetwork::KeyMgmtMask::IEEE8021X) |
     static_cast<uint32_t>(ISupplicantStaNetwork::KeyMgmtMask::FT_EAP) |
     static_cast<uint32_t>(ISupplicantStaNetwork::KeyMgmtMask::FT_PSK) |
     static_cast<uint32_t>(ISupplicantStaNetwork::KeyMgmtMask::OSEN));
constexpr uint32_t kAllowedProtoMask =
    (static_cast<uint32_t>(ISupplicantStaNetwork::ProtoMask::WPA) |
     static_cast<uint32_t>(ISupplicantStaNetwork::ProtoMask::RSN) |
     static_cast<uint32_t>(ISupplicantStaNetwork::ProtoMask::OSEN));
constexpr uint32_t kAllowedAuthAlgMask =
    (static_cast<uint32_t>(ISupplicantStaNetwork::AuthAlgMask::OPEN) |
     static_cast<uint32_t>(ISupplicantStaNetwork::AuthAlgMask::SHARED) |
     static_cast<uint32_t>(ISupplicantStaNetwork::AuthAlgMask::LEAP));
constexpr uint32_t kAllowedGroupCipherMask =
    (static_cast<uint32_t>(ISupplicantStaNetwork::GroupCipherMask::WEP40) |
     static_cast<uint32_t>(ISupplicantStaNetwork::GroupCipherMask::WEP104) |
     static_cast<uint32_t>(ISupplicantStaNetwork::GroupCipherMask::TKIP) |
     static_cast<uint32_t>(ISupplicantStaNetwork::GroupCipherMask::CCMP) |
     static_cast<uint32_t>(
	 ISupplicantStaNetwork::GroupCipherMask::GTK_NOT_USED));
constexpr uint32_t kAllowedPairwisewCipherMask =
    (static_cast<uint32_t>(ISupplicantStaNetwork::PairwiseCipherMask::NONE) |
     static_cast<uint32_t>(ISupplicantStaNetwork::PairwiseCipherMask::TKIP) |
     static_cast<uint32_t>(ISupplicantStaNetwork::PairwiseCipherMask::CCMP));

constexpr uint32_t kEapMethodMax =
    static_cast<uint32_t>(ISupplicantStaNetwork::EapMethod::WFA_UNAUTH_TLS) + 1;
constexpr char const *kEapMethodStrings[kEapMethodMax] = {
    "PEAP", "TLS", "TTLS", "PWD", "SIM", "AKA", "AKA'", "WFA-UNAUTH-TLS"};
constexpr uint32_t kEapPhase2MethodMax =
    static_cast<uint32_t>(ISupplicantStaNetwork::EapPhase2Method::AKA_PRIME) +
    1;
constexpr char const *kEapPhase2MethodStrings[kEapPhase2MethodMax] = {
    "", "PAP", "MSCHAP", "MSCHAPV2", "GTC", "SIM", "AKA", "AKA'"};
constexpr char kEapPhase2AuthPrefix[] = "auth=";
constexpr char kEapPhase2AuthEapPrefix[] = "autheap=";
constexpr char kNetworkEapSimGsmAuthResponse[] = "GSM-AUTH";
constexpr char kNetworkEapSimUmtsAuthResponse[] = "UMTS-AUTH";
constexpr char kNetworkEapSimUmtsAutsResponse[] = "UMTS-AUTS";
constexpr char kNetworkEapSimGsmAuthFailure[] = "GSM-FAIL";
constexpr char kNetworkEapSimUmtsAuthFailure[] = "UMTS-FAIL";
}  // namespace

namespace android {
namespace hardware {
namespace wifi {
namespace supplicant {
namespace V1_1 {
namespace implementation {
using hidl_return_util::validateAndCall;

StaNetwork::StaNetwork(
    struct wpa_global *wpa_global, const char ifname[], int network_id)
    : wpa_global_(wpa_global),
      ifname_(ifname),
      network_id_(network_id),
      is_valid_(true)
{
}

void StaNetwork::invalidate() { is_valid_ = false; }
bool StaNetwork::isValid()
{
	return (is_valid_ && (retrieveNetworkPtr() != nullptr));
}

Return<void> StaNetwork::getId(getId_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaNetwork::getIdInternal, _hidl_cb);
}

Return<void> StaNetwork::getInterfaceName(getInterfaceName_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaNetwork::getInterfaceNameInternal, _hidl_cb);
}

Return<void> StaNetwork::getType(getType_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaNetwork::getTypeInternal, _hidl_cb);
}

Return<void> StaNetwork::registerCallback(
    const sp<ISupplicantStaNetworkCallback> &callback,
    registerCallback_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaNetwork::registerCallbackInternal, _hidl_cb, callback);
}

Return<void> StaNetwork::setSsid(
    const hidl_vec<uint8_t> &ssid, setSsid_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaNetwork::setSsidInternal, _hidl_cb, ssid);
}

Return<void> StaNetwork::setBssid(
    const hidl_array<uint8_t, 6> &bssid, setBssid_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaNetwork::setBssidInternal, _hidl_cb, bssid);
}

Return<void> StaNetwork::setScanSsid(bool enable, setScanSsid_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaNetwork::setScanSsidInternal, _hidl_cb, enable);
}

Return<void> StaNetwork::setKeyMgmt(
    uint32_t key_mgmt_mask, setKeyMgmt_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaNetwork::setKeyMgmtInternal, _hidl_cb, key_mgmt_mask);
}

Return<void> StaNetwork::setProto(uint32_t proto_mask, setProto_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaNetwork::setProtoInternal, _hidl_cb, proto_mask);
}

Return<void> StaNetwork::setAuthAlg(
    uint32_t auth_alg_mask,
    std::function<void(const SupplicantStatus &status)> _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaNetwork::setAuthAlgInternal, _hidl_cb, auth_alg_mask);
}

Return<void> StaNetwork::setGroupCipher(
    uint32_t group_cipher_mask, setGroupCipher_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaNetwork::setGroupCipherInternal, _hidl_cb, group_cipher_mask);
}

Return<void> StaNetwork::setPairwiseCipher(
    uint32_t pairwise_cipher_mask, setPairwiseCipher_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaNetwork::setPairwiseCipherInternal, _hidl_cb,
	    pairwise_cipher_mask);
}

Return<void> StaNetwork::setPskPassphrase(
    const hidl_string &psk, setPskPassphrase_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaNetwork::setPskPassphraseInternal, _hidl_cb, psk);
}

Return<void> StaNetwork::setPsk(
    const hidl_array<uint8_t, 32> &psk, setPsk_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaNetwork::setPskInternal, _hidl_cb, psk);
}

Return<void> StaNetwork::setWepKey(
    uint32_t key_idx, const hidl_vec<uint8_t> &wep_key, setWepKey_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaNetwork::setWepKeyInternal, _hidl_cb, key_idx, wep_key);
}

Return<void> StaNetwork::setWepTxKeyIdx(
    uint32_t key_idx, setWepTxKeyIdx_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaNetwork::setWepTxKeyIdxInternal, _hidl_cb, key_idx);
}

Return<void> StaNetwork::setRequirePmf(bool enable, setRequirePmf_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaNetwork::setRequirePmfInternal, _hidl_cb, enable);
}

Return<void> StaNetwork::setEapMethod(
    ISupplicantStaNetwork::EapMethod method, setEapMethod_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaNetwork::setEapMethodInternal, _hidl_cb, method);
}

Return<void> StaNetwork::setEapPhase2Method(
    ISupplicantStaNetwork::EapPhase2Method method,
    setEapPhase2Method_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaNetwork::setEapPhase2MethodInternal, _hidl_cb, method);
}

Return<void> StaNetwork::setEapIdentity(
    const hidl_vec<uint8_t> &identity, setEapIdentity_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaNetwork::setEapIdentityInternal, _hidl_cb, identity);
}

Return<void> StaNetwork::setEapEncryptedImsiIdentity(
    const EapSimEncryptedIdentity &identity, setEapEncryptedImsiIdentity_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaNetwork::setEapEncryptedImsiIdentityInternal, _hidl_cb, identity);
}

Return<void> StaNetwork::setEapAnonymousIdentity(
    const hidl_vec<uint8_t> &identity, setEapAnonymousIdentity_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaNetwork::setEapAnonymousIdentityInternal, _hidl_cb, identity);
}

Return<void> StaNetwork::setEapPassword(
    const hidl_vec<uint8_t> &password, setEapPassword_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaNetwork::setEapPasswordInternal, _hidl_cb, password);
}

Return<void> StaNetwork::setEapCACert(
    const hidl_string &path, setEapCACert_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaNetwork::setEapCACertInternal, _hidl_cb, path);
}

Return<void> StaNetwork::setEapCAPath(
    const hidl_string &path, setEapCAPath_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaNetwork::setEapCAPathInternal, _hidl_cb, path);
}

Return<void> StaNetwork::setEapClientCert(
    const hidl_string &path, setEapClientCert_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaNetwork::setEapClientCertInternal, _hidl_cb, path);
}

Return<void> StaNetwork::setEapPrivateKeyId(
    const hidl_string &id, setEapPrivateKeyId_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaNetwork::setEapPrivateKeyIdInternal, _hidl_cb, id);
}

Return<void> StaNetwork::setEapSubjectMatch(
    const hidl_string &match, setEapSubjectMatch_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaNetwork::setEapSubjectMatchInternal, _hidl_cb, match);
}

Return<void> StaNetwork::setEapAltSubjectMatch(
    const hidl_string &match, setEapAltSubjectMatch_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaNetwork::setEapAltSubjectMatchInternal, _hidl_cb, match);
}

Return<void> StaNetwork::setEapEngine(bool enable, setEapEngine_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaNetwork::setEapEngineInternal, _hidl_cb, enable);
}

Return<void> StaNetwork::setEapEngineID(
    const hidl_string &id, setEapEngineID_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaNetwork::setEapEngineIDInternal, _hidl_cb, id);
}

Return<void> StaNetwork::setEapDomainSuffixMatch(
    const hidl_string &match, setEapDomainSuffixMatch_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaNetwork::setEapDomainSuffixMatchInternal, _hidl_cb, match);
}

Return<void> StaNetwork::setProactiveKeyCaching(
    bool enable, setProactiveKeyCaching_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaNetwork::setProactiveKeyCachingInternal, _hidl_cb, enable);
}

Return<void> StaNetwork::setIdStr(
    const hidl_string &id_str, setIdStr_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaNetwork::setIdStrInternal, _hidl_cb, id_str);
}

Return<void> StaNetwork::setUpdateIdentifier(
    uint32_t id, setUpdateIdentifier_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaNetwork::setUpdateIdentifierInternal, _hidl_cb, id);
}

Return<void> StaNetwork::getSsid(getSsid_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaNetwork::getSsidInternal, _hidl_cb);
}

Return<void> StaNetwork::getBssid(getBssid_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaNetwork::getBssidInternal, _hidl_cb);
}

Return<void> StaNetwork::getScanSsid(getScanSsid_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaNetwork::getScanSsidInternal, _hidl_cb);
}

Return<void> StaNetwork::getKeyMgmt(getKeyMgmt_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaNetwork::getKeyMgmtInternal, _hidl_cb);
}

Return<void> StaNetwork::getProto(getProto_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaNetwork::getProtoInternal, _hidl_cb);
}

Return<void> StaNetwork::getAuthAlg(getAuthAlg_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaNetwork::getAuthAlgInternal, _hidl_cb);
}

Return<void> StaNetwork::getGroupCipher(getGroupCipher_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaNetwork::getGroupCipherInternal, _hidl_cb);
}

Return<void> StaNetwork::getPairwiseCipher(getPairwiseCipher_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaNetwork::getPairwiseCipherInternal, _hidl_cb);
}

Return<void> StaNetwork::getPskPassphrase(getPskPassphrase_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaNetwork::getPskPassphraseInternal, _hidl_cb);
}

Return<void> StaNetwork::getPsk(getPsk_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaNetwork::getPskInternal, _hidl_cb);
}

Return<void> StaNetwork::getWepKey(uint32_t key_idx, getWepKey_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaNetwork::getWepKeyInternal, _hidl_cb, key_idx);
}

Return<void> StaNetwork::getWepTxKeyIdx(getWepTxKeyIdx_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaNetwork::getWepTxKeyIdxInternal, _hidl_cb);
}

Return<void> StaNetwork::getRequirePmf(getRequirePmf_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaNetwork::getRequirePmfInternal, _hidl_cb);
}

Return<void> StaNetwork::getEapMethod(getEapMethod_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaNetwork::getEapMethodInternal, _hidl_cb);
}

Return<void> StaNetwork::getEapPhase2Method(getEapPhase2Method_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaNetwork::getEapPhase2MethodInternal, _hidl_cb);
}

Return<void> StaNetwork::getEapIdentity(getEapIdentity_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaNetwork::getEapIdentityInternal, _hidl_cb);
}

Return<void> StaNetwork::getEapAnonymousIdentity(
    getEapAnonymousIdentity_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaNetwork::getEapAnonymousIdentityInternal, _hidl_cb);
}

Return<void> StaNetwork::getEapPassword(getEapPassword_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaNetwork::getEapPasswordInternal, _hidl_cb);
}

Return<void> StaNetwork::getEapCACert(getEapCACert_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaNetwork::getEapCACertInternal, _hidl_cb);
}

Return<void> StaNetwork::getEapCAPath(getEapCAPath_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaNetwork::getEapCAPathInternal, _hidl_cb);
}

Return<void> StaNetwork::getEapClientCert(getEapClientCert_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaNetwork::getEapClientCertInternal, _hidl_cb);
}

Return<void> StaNetwork::getEapPrivateKeyId(getEapPrivateKeyId_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaNetwork::getEapPrivateKeyIdInternal, _hidl_cb);
}

Return<void> StaNetwork::getEapSubjectMatch(getEapSubjectMatch_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaNetwork::getEapSubjectMatchInternal, _hidl_cb);
}

Return<void> StaNetwork::getEapAltSubjectMatch(
    getEapAltSubjectMatch_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaNetwork::getEapAltSubjectMatchInternal, _hidl_cb);
}

Return<void> StaNetwork::getEapEngine(getEapEngine_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaNetwork::getEapEngineInternal, _hidl_cb);
}

Return<void> StaNetwork::getEapEngineID(getEapEngineID_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaNetwork::getEapEngineIDInternal, _hidl_cb);
}

Return<void> StaNetwork::getEapDomainSuffixMatch(
    getEapDomainSuffixMatch_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaNetwork::getEapDomainSuffixMatchInternal, _hidl_cb);
}

Return<void> StaNetwork::getIdStr(getIdStr_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaNetwork::getIdStrInternal, _hidl_cb);
}

Return<void> StaNetwork::getWpsNfcConfigurationToken(
    getWpsNfcConfigurationToken_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaNetwork::getWpsNfcConfigurationTokenInternal, _hidl_cb);
}

Return<void> StaNetwork::enable(bool no_connect, enable_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaNetwork::enableInternal, _hidl_cb, no_connect);
}

Return<void> StaNetwork::disable(disable_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaNetwork::disableInternal, _hidl_cb);
}

Return<void> StaNetwork::select(select_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaNetwork::selectInternal, _hidl_cb);
}

Return<void> StaNetwork::sendNetworkEapSimGsmAuthResponse(
    const hidl_vec<ISupplicantStaNetwork::NetworkResponseEapSimGsmAuthParams>
	&vec_params,
    sendNetworkEapSimGsmAuthResponse_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaNetwork::sendNetworkEapSimGsmAuthResponseInternal, _hidl_cb,
	    vec_params);
}

Return<void> StaNetwork::sendNetworkEapSimGsmAuthFailure(
    sendNetworkEapSimGsmAuthFailure_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaNetwork::sendNetworkEapSimGsmAuthFailureInternal, _hidl_cb);
}

Return<void> StaNetwork::sendNetworkEapSimUmtsAuthResponse(
    const ISupplicantStaNetwork::NetworkResponseEapSimUmtsAuthParams &params,
    sendNetworkEapSimUmtsAuthResponse_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaNetwork::sendNetworkEapSimUmtsAuthResponseInternal, _hidl_cb,
	    params);
}

Return<void> StaNetwork::sendNetworkEapSimUmtsAutsResponse(
    const hidl_array<uint8_t, 14> &auts,
    sendNetworkEapSimUmtsAutsResponse_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaNetwork::sendNetworkEapSimUmtsAutsResponseInternal, _hidl_cb,
	    auts);
}

Return<void> StaNetwork::sendNetworkEapSimUmtsAuthFailure(
    sendNetworkEapSimUmtsAuthFailure_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaNetwork::sendNetworkEapSimUmtsAuthFailureInternal, _hidl_cb);
}

Return<void> StaNetwork::sendNetworkEapIdentityResponse(
    const hidl_vec<uint8_t> &identity,
    sendNetworkEapIdentityResponse_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaNetwork::sendNetworkEapIdentityResponseInternal, _hidl_cb,
	    identity);
}

Return<void> StaNetwork::sendNetworkEapIdentityResponse_1_1(
    const EapSimIdentity &identity,
    const EapSimEncryptedIdentity &encrypted_imsi_identity,
    sendNetworkEapIdentityResponse_1_1_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaNetwork::sendNetworkEapIdentityResponseInternal_1_1, _hidl_cb,
	    identity, encrypted_imsi_identity);
}

std::pair<SupplicantStatus, uint32_t> StaNetwork::getIdInternal()
{
	return {{SupplicantStatusCode::SUCCESS, ""}, network_id_};
}

std::pair<SupplicantStatus, std::string> StaNetwork::getInterfaceNameInternal()
{
	return {{SupplicantStatusCode::SUCCESS, ""}, ifname_};
}

std::pair<SupplicantStatus, IfaceType> StaNetwork::getTypeInternal()
{
	return {{SupplicantStatusCode::SUCCESS, ""}, IfaceType::STA};
}

SupplicantStatus StaNetwork::registerCallbackInternal(
    const sp<ISupplicantStaNetworkCallback> &callback)
{
	HidlManager *hidl_manager = HidlManager::getInstance();
	if (!hidl_manager ||
	    hidl_manager->addStaNetworkCallbackHidlObject(
		ifname_, network_id_, callback)) {
		return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	}
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus StaNetwork::setSsidInternal(const std::vector<uint8_t> &ssid)
{
	struct wpa_ssid *wpa_ssid = retrieveNetworkPtr();
	if (ssid.size() == 0 ||
	    ssid.size() >
		static_cast<uint32_t>(ISupplicantStaNetwork::ParamSizeLimits::
					  SSID_MAX_LEN_IN_BYTES)) {
		return {SupplicantStatusCode::FAILURE_ARGS_INVALID, ""};
	}
	if (setByteArrayFieldAndResetState(
		ssid.data(), ssid.size(), &(wpa_ssid->ssid),
		&(wpa_ssid->ssid_len), "ssid")) {
		return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	}
	if (wpa_ssid->passphrase) {
		wpa_config_update_psk(wpa_ssid);
	}
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus StaNetwork::setBssidInternal(
    const std::array<uint8_t, 6> &bssid)
{
	struct wpa_ssid *wpa_ssid = retrieveNetworkPtr();
	int prev_bssid_set = wpa_ssid->bssid_set;
	u8 prev_bssid[ETH_ALEN];
	os_memcpy(prev_bssid, wpa_ssid->bssid, ETH_ALEN);
	// Zero'ed array is used to clear out the BSSID value.
	if (os_memcmp(bssid.data(), kZeroBssid, ETH_ALEN) == 0) {
		wpa_ssid->bssid_set = 0;
		wpa_printf(MSG_MSGDUMP, "BSSID any");
	} else {
		os_memcpy(wpa_ssid->bssid, bssid.data(), ETH_ALEN);
		wpa_ssid->bssid_set = 1;
		wpa_hexdump(MSG_MSGDUMP, "BSSID", wpa_ssid->bssid, ETH_ALEN);
	}
	struct wpa_supplicant *wpa_s = retrieveIfacePtr();
	if ((wpa_ssid->bssid_set != prev_bssid_set ||
	     os_memcmp(wpa_ssid->bssid, prev_bssid, ETH_ALEN) != 0)) {
		wpas_notify_network_bssid_set_changed(wpa_s, wpa_ssid);
	}
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus StaNetwork::setScanSsidInternal(bool enable)
{
	struct wpa_ssid *wpa_ssid = retrieveNetworkPtr();
	wpa_ssid->scan_ssid = enable ? 1 : 0;
	resetInternalStateAfterParamsUpdate();
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus StaNetwork::setKeyMgmtInternal(uint32_t key_mgmt_mask)
{
	struct wpa_ssid *wpa_ssid = retrieveNetworkPtr();
	if (key_mgmt_mask & ~kAllowedKeyMgmtMask) {
		return {SupplicantStatusCode::FAILURE_ARGS_INVALID, ""};
	}
	wpa_ssid->key_mgmt = key_mgmt_mask;
	wpa_printf(MSG_MSGDUMP, "key_mgmt: 0x%x", wpa_ssid->key_mgmt);
	resetInternalStateAfterParamsUpdate();
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus StaNetwork::setProtoInternal(uint32_t proto_mask)
{
	struct wpa_ssid *wpa_ssid = retrieveNetworkPtr();
	if (proto_mask & ~kAllowedProtoMask) {
		return {SupplicantStatusCode::FAILURE_ARGS_INVALID, ""};
	}
	wpa_ssid->proto = proto_mask;
	wpa_printf(MSG_MSGDUMP, "proto: 0x%x", wpa_ssid->proto);
	resetInternalStateAfterParamsUpdate();
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus StaNetwork::setAuthAlgInternal(uint32_t auth_alg_mask)
{
	struct wpa_ssid *wpa_ssid = retrieveNetworkPtr();
	if (auth_alg_mask & ~kAllowedAuthAlgMask) {
		return {SupplicantStatusCode::FAILURE_ARGS_INVALID, ""};
	}
	wpa_ssid->auth_alg = auth_alg_mask;
	wpa_printf(MSG_MSGDUMP, "auth_alg: 0x%x", wpa_ssid->auth_alg);
	resetInternalStateAfterParamsUpdate();
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus StaNetwork::setGroupCipherInternal(uint32_t group_cipher_mask)
{
	struct wpa_ssid *wpa_ssid = retrieveNetworkPtr();
	if (group_cipher_mask & ~kAllowedGroupCipherMask) {
		return {SupplicantStatusCode::FAILURE_ARGS_INVALID, ""};
	}
	wpa_ssid->group_cipher = group_cipher_mask;
	wpa_printf(MSG_MSGDUMP, "group_cipher: 0x%x", wpa_ssid->group_cipher);
	resetInternalStateAfterParamsUpdate();
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus StaNetwork::setPairwiseCipherInternal(
    uint32_t pairwise_cipher_mask)
{
	struct wpa_ssid *wpa_ssid = retrieveNetworkPtr();
	if (pairwise_cipher_mask & ~kAllowedPairwisewCipherMask) {
		return {SupplicantStatusCode::FAILURE_ARGS_INVALID, ""};
	}
	wpa_ssid->pairwise_cipher = pairwise_cipher_mask;
	wpa_printf(
	    MSG_MSGDUMP, "pairwise_cipher: 0x%x", wpa_ssid->pairwise_cipher);
	resetInternalStateAfterParamsUpdate();
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus StaNetwork::setPskPassphraseInternal(const std::string &psk)
{
	struct wpa_ssid *wpa_ssid = retrieveNetworkPtr();
	if (isPskPassphraseValid(psk)) {
		return {SupplicantStatusCode::FAILURE_ARGS_INVALID, ""};
	}
	if (wpa_ssid->passphrase &&
	    os_strlen(wpa_ssid->passphrase) == psk.size() &&
	    os_memcmp(wpa_ssid->passphrase, psk.c_str(), psk.size()) == 0) {
		return {SupplicantStatusCode::SUCCESS, ""};
	}
	// Flag to indicate if raw psk is calculated or not using
	// |wpa_config_update_psk|. Deferred if ssid not already set.
	wpa_ssid->psk_set = 0;
	if (setStringKeyFieldAndResetState(
		psk.c_str(), &(wpa_ssid->passphrase), "psk passphrase")) {
		return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	}
	if (wpa_ssid->ssid_len) {
		wpa_config_update_psk(wpa_ssid);
	}
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus StaNetwork::setPskInternal(const std::array<uint8_t, 32> &psk)
{
	struct wpa_ssid *wpa_ssid = retrieveNetworkPtr();
	WPA_ASSERT(psk.size() == sizeof(wpa_ssid->psk));
	str_clear_free(wpa_ssid->passphrase);
	wpa_ssid->passphrase = nullptr;
	os_memcpy(wpa_ssid->psk, psk.data(), sizeof(wpa_ssid->psk));
	wpa_ssid->psk_set = 1;
	resetInternalStateAfterParamsUpdate();
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus StaNetwork::setWepKeyInternal(
    uint32_t key_idx, const std::vector<uint8_t> &wep_key)
{
	struct wpa_ssid *wpa_ssid = retrieveNetworkPtr();
	if (key_idx >=
	    static_cast<uint32_t>(
		ISupplicantStaNetwork::ParamSizeLimits::WEP_KEYS_MAX_NUM)) {
		return {SupplicantStatusCode::FAILURE_ARGS_INVALID, ""};
	}
	if (wep_key.size() !=
		static_cast<uint32_t>(ISupplicantStaNetwork::ParamSizeLimits::
					  WEP40_KEY_LEN_IN_BYTES) &&
	    wep_key.size() !=
		static_cast<uint32_t>(ISupplicantStaNetwork::ParamSizeLimits::
					  WEP104_KEY_LEN_IN_BYTES)) {
		return {SupplicantStatusCode::FAILURE_ARGS_INVALID, ""};
	}
	os_memcpy(wpa_ssid->wep_key[key_idx], wep_key.data(), wep_key.size());
	wpa_ssid->wep_key_len[key_idx] = wep_key.size();
	std::string msg_dump_title("wep_key" + std::to_string(key_idx));
	wpa_hexdump_key(
	    MSG_MSGDUMP, msg_dump_title.c_str(), wpa_ssid->wep_key[key_idx],
	    wpa_ssid->wep_key_len[key_idx]);
	resetInternalStateAfterParamsUpdate();
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus StaNetwork::setWepTxKeyIdxInternal(uint32_t key_idx)
{
	struct wpa_ssid *wpa_ssid = retrieveNetworkPtr();
	if (key_idx >=
	    static_cast<uint32_t>(
		ISupplicantStaNetwork::ParamSizeLimits::WEP_KEYS_MAX_NUM)) {
		return {SupplicantStatusCode::FAILURE_ARGS_INVALID, ""};
	}
	wpa_ssid->wep_tx_keyidx = key_idx;
	resetInternalStateAfterParamsUpdate();
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus StaNetwork::setRequirePmfInternal(bool enable)
{
	struct wpa_ssid *wpa_ssid = retrieveNetworkPtr();
	wpa_ssid->ieee80211w =
	    enable ? MGMT_FRAME_PROTECTION_REQUIRED : NO_MGMT_FRAME_PROTECTION;
	resetInternalStateAfterParamsUpdate();
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus StaNetwork::setEapMethodInternal(
    ISupplicantStaNetwork::EapMethod method)
{
	uint32_t eap_method_idx = static_cast<
	    std::underlying_type<ISupplicantStaNetwork::EapMethod>::type>(
	    method);
	if (eap_method_idx >= kEapMethodMax) {
		return {SupplicantStatusCode::FAILURE_ARGS_INVALID, ""};
	}

	struct wpa_ssid *wpa_ssid = retrieveNetworkPtr();
	int retrieved_vendor, retrieved_method;
	const char *method_str = kEapMethodStrings[eap_method_idx];
	// This string lookup is needed to check if the device supports the
	// corresponding EAP type.
	retrieved_method = eap_peer_get_type(method_str, &retrieved_vendor);
	if (retrieved_vendor == EAP_VENDOR_IETF &&
	    retrieved_method == EAP_TYPE_NONE) {
		return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	}
	if (wpa_ssid->eap.eap_methods) {
		os_free(wpa_ssid->eap.eap_methods);
	}
	// wpa_supplicant can support setting multiple eap methods for each
	// network. But, this is not really used by Android. So, just adding
	// support for setting one EAP method for each network. The additional
	// |eap_method_type| member in the array is used to indicate the end
	// of list.
	wpa_ssid->eap.eap_methods =
	    (eap_method_type *)os_malloc(sizeof(eap_method_type) * 2);
	if (!wpa_ssid->eap.eap_methods) {
		return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	}
	wpa_ssid->eap.eap_methods[0].vendor = retrieved_vendor;
	wpa_ssid->eap.eap_methods[0].method = retrieved_method;
	wpa_ssid->eap.eap_methods[1].vendor = EAP_VENDOR_IETF;
	wpa_ssid->eap.eap_methods[1].method = EAP_TYPE_NONE;

	wpa_ssid->leap = 0;
	wpa_ssid->non_leap = 0;
	if (retrieved_vendor == EAP_VENDOR_IETF &&
	    retrieved_method == EAP_TYPE_LEAP) {
		wpa_ssid->leap++;
	} else {
		wpa_ssid->non_leap++;
	}
	wpa_hexdump(
	    MSG_MSGDUMP, "eap methods", (u8 *)wpa_ssid->eap.eap_methods,
	    sizeof(eap_method_type) * 2);
	resetInternalStateAfterParamsUpdate();
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus StaNetwork::setEapPhase2MethodInternal(
    ISupplicantStaNetwork::EapPhase2Method method)
{
	uint32_t eap_phase2_method_idx = static_cast<
	    std::underlying_type<ISupplicantStaNetwork::EapPhase2Method>::type>(
	    method);
	if (eap_phase2_method_idx >= kEapPhase2MethodMax) {
		return {SupplicantStatusCode::FAILURE_ARGS_INVALID, ""};
	}

	struct wpa_ssid *wpa_ssid = retrieveNetworkPtr();
	// EAP method needs to be set for us to construct the eap
	// phase 2 method string.
	SupplicantStatus status;
	ISupplicantStaNetwork::EapMethod eap_method;
	std::tie(status, eap_method) = getEapMethodInternal();
	if (status.code != SupplicantStatusCode::SUCCESS) {
		return {SupplicantStatusCode::FAILURE_UNKNOWN,
			"EAP method not set"};
	}
	std::string eap_phase2_str;
	if (method == ISupplicantStaNetwork::EapPhase2Method::NONE) {
		eap_phase2_str = "";
	} else if (
	    eap_method == ISupplicantStaNetwork::EapMethod::TTLS &&
	    method == ISupplicantStaNetwork::EapPhase2Method::GTC) {
		eap_phase2_str = kEapPhase2AuthEapPrefix;
	} else {
		eap_phase2_str = kEapPhase2AuthPrefix;
	}
	eap_phase2_str += kEapPhase2MethodStrings[eap_phase2_method_idx];
	if (setStringFieldAndResetState(
		eap_phase2_str.c_str(), &(wpa_ssid->eap.phase2),
		"eap phase2")) {
		return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	}
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus StaNetwork::setEapIdentityInternal(
    const std::vector<uint8_t> &identity)
{
	struct wpa_ssid *wpa_ssid = retrieveNetworkPtr();
	if (setByteArrayFieldAndResetState(
		identity.data(), identity.size(), &(wpa_ssid->eap.identity),
		&(wpa_ssid->eap.identity_len), "eap identity")) {
		return { SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	}
	// plain IMSI identity
	if (setByteArrayFieldAndResetState(
		identity.data(), identity.size(), &(wpa_ssid->eap.imsi_identity),
		&(wpa_ssid->eap.imsi_identity_len), "eap imsi identity")) {
		return { SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	}
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus StaNetwork::setEapEncryptedImsiIdentityInternal(
    const std::vector<uint8_t> &identity)
{
	struct wpa_ssid *wpa_ssid = retrieveNetworkPtr();
	// encrypted IMSI identity
	if (setByteArrayFieldAndResetState(
		identity.data(), identity.size(), &(wpa_ssid->eap.identity),
		&(wpa_ssid->eap.identity_len), "eap encrypted imsi identity")) {
		return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	}
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus StaNetwork::setEapAnonymousIdentityInternal(
    const std::vector<uint8_t> &identity)
{
	struct wpa_ssid *wpa_ssid = retrieveNetworkPtr();
	if (setByteArrayFieldAndResetState(
		identity.data(), identity.size(),
		&(wpa_ssid->eap.anonymous_identity),
		&(wpa_ssid->eap.anonymous_identity_len),
		"eap anonymous_identity")) {
		return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	}
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus StaNetwork::setEapPasswordInternal(
    const std::vector<uint8_t> &password)
{
	struct wpa_ssid *wpa_ssid = retrieveNetworkPtr();
	if (setByteArrayKeyFieldAndResetState(
		password.data(), password.size(), &(wpa_ssid->eap.password),
		&(wpa_ssid->eap.password_len), "eap password")) {
		return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	}
	wpa_ssid->eap.flags &= ~EAP_CONFIG_FLAGS_PASSWORD_NTHASH;
	wpa_ssid->eap.flags &= ~EAP_CONFIG_FLAGS_EXT_PASSWORD;
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus StaNetwork::setEapCACertInternal(const std::string &path)
{
	struct wpa_ssid *wpa_ssid = retrieveNetworkPtr();
	if (setStringFieldAndResetState(
		path.c_str(), &(wpa_ssid->eap.ca_cert), "eap ca_cert")) {
		return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	}
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus StaNetwork::setEapCAPathInternal(const std::string &path)
{
	struct wpa_ssid *wpa_ssid = retrieveNetworkPtr();
	if (setStringFieldAndResetState(
		path.c_str(), &(wpa_ssid->eap.ca_path), "eap ca_path")) {
		return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	}
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus StaNetwork::setEapClientCertInternal(const std::string &path)
{
	struct wpa_ssid *wpa_ssid = retrieveNetworkPtr();
	if (setStringFieldAndResetState(
		path.c_str(), &(wpa_ssid->eap.client_cert),
		"eap client_cert")) {
		return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	}
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus StaNetwork::setEapPrivateKeyIdInternal(const std::string &id)
{
	struct wpa_ssid *wpa_ssid = retrieveNetworkPtr();
	if (setStringFieldAndResetState(
		id.c_str(), &(wpa_ssid->eap.key_id), "eap key_id")) {
		return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	}
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus StaNetwork::setEapSubjectMatchInternal(
    const std::string &match)
{
	struct wpa_ssid *wpa_ssid = retrieveNetworkPtr();
	if (setStringFieldAndResetState(
		match.c_str(), &(wpa_ssid->eap.subject_match),
		"eap subject_match")) {
		return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	}
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus StaNetwork::setEapAltSubjectMatchInternal(
    const std::string &match)
{
	struct wpa_ssid *wpa_ssid = retrieveNetworkPtr();
	if (setStringFieldAndResetState(
		match.c_str(), &(wpa_ssid->eap.altsubject_match),
		"eap altsubject_match")) {
		return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	}
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus StaNetwork::setEapEngineInternal(bool enable)
{
	struct wpa_ssid *wpa_ssid = retrieveNetworkPtr();
	wpa_ssid->eap.engine = enable ? 1 : 0;
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus StaNetwork::setEapEngineIDInternal(const std::string &id)
{
	struct wpa_ssid *wpa_ssid = retrieveNetworkPtr();
	if (setStringFieldAndResetState(
		id.c_str(), &(wpa_ssid->eap.engine_id), "eap engine_id")) {
		return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	}
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus StaNetwork::setEapDomainSuffixMatchInternal(
    const std::string &match)
{
	struct wpa_ssid *wpa_ssid = retrieveNetworkPtr();
	if (setStringFieldAndResetState(
		match.c_str(), &(wpa_ssid->eap.domain_suffix_match),
		"eap domain_suffix_match")) {
		return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	}
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus StaNetwork::setProactiveKeyCachingInternal(bool enable)
{
	struct wpa_ssid *wpa_ssid = retrieveNetworkPtr();
	wpa_ssid->proactive_key_caching = enable ? 1 : 0;
	resetInternalStateAfterParamsUpdate();
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus StaNetwork::setIdStrInternal(const std::string &id_str)
{
	struct wpa_ssid *wpa_ssid = retrieveNetworkPtr();
	if (setStringFieldAndResetState(
		id_str.c_str(), &(wpa_ssid->id_str), "id_str")) {
		return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	}
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus StaNetwork::setUpdateIdentifierInternal(uint32_t id)
{
	struct wpa_ssid *wpa_ssid = retrieveNetworkPtr();
	wpa_ssid->update_identifier = id;
	wpa_printf(
	    MSG_MSGDUMP, "update_identifier: %d", wpa_ssid->update_identifier);
	resetInternalStateAfterParamsUpdate();
	return {SupplicantStatusCode::SUCCESS, ""};
}

std::pair<SupplicantStatus, std::vector<uint8_t>> StaNetwork::getSsidInternal()
{
	struct wpa_ssid *wpa_ssid = retrieveNetworkPtr();
	std::vector<uint8_t> ssid;
	ssid.assign(wpa_ssid->ssid, wpa_ssid->ssid + wpa_ssid->ssid_len);
	return {{SupplicantStatusCode::SUCCESS, ""}, std::move(ssid)};
}

std::pair<SupplicantStatus, std::array<uint8_t, 6>>
StaNetwork::getBssidInternal()
{
	struct wpa_ssid *wpa_ssid = retrieveNetworkPtr();
	std::array<uint8_t, 6> bssid{};
	os_memcpy(bssid.data(), kZeroBssid, ETH_ALEN);
	if (wpa_ssid->bssid_set) {
		os_memcpy(bssid.data(), wpa_ssid->bssid, ETH_ALEN);
	}
	return {{SupplicantStatusCode::SUCCESS, ""}, std::move(bssid)};
}

std::pair<SupplicantStatus, bool> StaNetwork::getScanSsidInternal()
{
	struct wpa_ssid *wpa_ssid = retrieveNetworkPtr();
	return {{SupplicantStatusCode::SUCCESS, ""},
		(wpa_ssid->scan_ssid == 1)};
}

std::pair<SupplicantStatus, uint32_t> StaNetwork::getKeyMgmtInternal()
{
	struct wpa_ssid *wpa_ssid = retrieveNetworkPtr();
	return {{SupplicantStatusCode::SUCCESS, ""},
		wpa_ssid->key_mgmt & kAllowedKeyMgmtMask};
}

std::pair<SupplicantStatus, uint32_t> StaNetwork::getProtoInternal()
{
	struct wpa_ssid *wpa_ssid = retrieveNetworkPtr();
	return {{SupplicantStatusCode::SUCCESS, ""},
		wpa_ssid->proto & kAllowedProtoMask};
}

std::pair<SupplicantStatus, uint32_t> StaNetwork::getAuthAlgInternal()
{
	struct wpa_ssid *wpa_ssid = retrieveNetworkPtr();
	return {{SupplicantStatusCode::SUCCESS, ""},
		wpa_ssid->auth_alg & kAllowedAuthAlgMask};
}

std::pair<SupplicantStatus, uint32_t> StaNetwork::getGroupCipherInternal()
{
	struct wpa_ssid *wpa_ssid = retrieveNetworkPtr();
	return {{SupplicantStatusCode::SUCCESS, ""},
		wpa_ssid->group_cipher & kAllowedGroupCipherMask};
}

std::pair<SupplicantStatus, uint32_t> StaNetwork::getPairwiseCipherInternal()
{
	struct wpa_ssid *wpa_ssid = retrieveNetworkPtr();
	return {{SupplicantStatusCode::SUCCESS, ""},
		wpa_ssid->pairwise_cipher & kAllowedPairwisewCipherMask};
}

std::pair<SupplicantStatus, std::string> StaNetwork::getPskPassphraseInternal()
{
	struct wpa_ssid *wpa_ssid = retrieveNetworkPtr();
	if (!wpa_ssid->passphrase) {
		return {{SupplicantStatusCode::FAILURE_UNKNOWN, ""}, {}};
	}
	return {{SupplicantStatusCode::SUCCESS, ""}, wpa_ssid->passphrase};
}

std::pair<SupplicantStatus, std::array<uint8_t, 32>>
StaNetwork::getPskInternal()
{
	struct wpa_ssid *wpa_ssid = retrieveNetworkPtr();
	WPA_ASSERT(psk.size() == sizeof(wpa_ssid->psk));
	if (!wpa_ssid->psk_set) {
		return {{SupplicantStatusCode::FAILURE_UNKNOWN, ""}, {}};
	}
	std::array<uint8_t, 32> psk;
	os_memcpy(psk.data(), wpa_ssid->psk, psk.size());
	return {{SupplicantStatusCode::SUCCESS, ""}, psk};
}

std::pair<SupplicantStatus, std::vector<uint8_t>> StaNetwork::getWepKeyInternal(
    uint32_t key_idx)
{
	std::vector<uint8_t> wep_key;
	struct wpa_ssid *wpa_ssid = retrieveNetworkPtr();
	if (key_idx >=
	    static_cast<uint32_t>(
		ISupplicantStaNetwork::ParamSizeLimits::WEP_KEYS_MAX_NUM)) {
		return {{SupplicantStatusCode::FAILURE_ARGS_INVALID, ""},
			wep_key};
	}
	wep_key.assign(
	    wpa_ssid->wep_key[key_idx],
	    wpa_ssid->wep_key[key_idx] + wpa_ssid->wep_key_len[key_idx]);
	return {{SupplicantStatusCode::SUCCESS, ""}, std::move(wep_key)};
}

std::pair<SupplicantStatus, uint32_t> StaNetwork::getWepTxKeyIdxInternal()
{
	struct wpa_ssid *wpa_ssid = retrieveNetworkPtr();
	return {{SupplicantStatusCode::SUCCESS, ""}, wpa_ssid->wep_tx_keyidx};
}

std::pair<SupplicantStatus, bool> StaNetwork::getRequirePmfInternal()
{
	struct wpa_ssid *wpa_ssid = retrieveNetworkPtr();
	return {{SupplicantStatusCode::SUCCESS, ""},
		(wpa_ssid->ieee80211w == MGMT_FRAME_PROTECTION_REQUIRED)};
}

std::pair<SupplicantStatus, ISupplicantStaNetwork::EapMethod>
StaNetwork::getEapMethodInternal()
{
	struct wpa_ssid *wpa_ssid = retrieveNetworkPtr();
	if (!wpa_ssid->eap.eap_methods) {
		return {{SupplicantStatusCode::FAILURE_UNKNOWN, ""}, {}};
	}
	// wpa_supplicant can support setting multiple eap methods for each
	// network. But, this is not really used by Android. So, just reading
	// the first EAP method for each network.
	const std::string eap_method_str = eap_get_name(
	    wpa_ssid->eap.eap_methods[0].vendor,
	    static_cast<EapType>(wpa_ssid->eap.eap_methods[0].method));
	size_t eap_method_idx =
	    std::find(
		std::begin(kEapMethodStrings), std::end(kEapMethodStrings),
		eap_method_str) -
	    std::begin(kEapMethodStrings);
	if (eap_method_idx >= kEapMethodMax) {
		return {{SupplicantStatusCode::FAILURE_UNKNOWN, ""}, {}};
	}
	return {{SupplicantStatusCode::SUCCESS, ""},
		static_cast<ISupplicantStaNetwork::EapMethod>(eap_method_idx)};
}

std::pair<SupplicantStatus, ISupplicantStaNetwork::EapPhase2Method>
StaNetwork::getEapPhase2MethodInternal()
{
	struct wpa_ssid *wpa_ssid = retrieveNetworkPtr();
	if (!wpa_ssid->eap.phase2) {
		return {{SupplicantStatusCode::FAILURE_UNKNOWN, ""}, {}};
	}
	const std::string eap_phase2_method_str_with_prefix =
	    wpa_ssid->eap.phase2;
	std::string eap_phase2_method_str;
	// Strip out the phase 2 method prefix before doing a reverse lookup
	// of phase 2 string to the Eap Phase 2 type.
	if (eap_phase2_method_str_with_prefix.find(kEapPhase2AuthPrefix) == 0) {
		eap_phase2_method_str =
		    eap_phase2_method_str_with_prefix.substr(
			strlen(kEapPhase2AuthPrefix),
			eap_phase2_method_str_with_prefix.size());
	} else if (
	    eap_phase2_method_str_with_prefix.find(kEapPhase2AuthEapPrefix) ==
	    0) {
		eap_phase2_method_str =
		    eap_phase2_method_str_with_prefix.substr(
			strlen(kEapPhase2AuthEapPrefix),
			eap_phase2_method_str_with_prefix.size());
	}
	size_t eap_phase2_method_idx =
	    std::find(
		std::begin(kEapPhase2MethodStrings),
		std::end(kEapPhase2MethodStrings), eap_phase2_method_str) -
	    std::begin(kEapPhase2MethodStrings);
	if (eap_phase2_method_idx >= kEapPhase2MethodMax) {
		return {{SupplicantStatusCode::FAILURE_UNKNOWN, ""}, {}};
	}
	return {{SupplicantStatusCode::SUCCESS, ""},
		static_cast<ISupplicantStaNetwork::EapPhase2Method>(
		    eap_phase2_method_idx)};
}

std::pair<SupplicantStatus, std::vector<uint8_t>>
StaNetwork::getEapIdentityInternal()
{
	struct wpa_ssid *wpa_ssid = retrieveNetworkPtr();
	if (!wpa_ssid->eap.identity) {
		return {{SupplicantStatusCode::FAILURE_UNKNOWN, ""}, {}};
	}
	return {{SupplicantStatusCode::SUCCESS, ""},
		std::vector<uint8_t>(
		    wpa_ssid->eap.identity,
		    wpa_ssid->eap.identity + wpa_ssid->eap.identity_len)};
}

std::pair<SupplicantStatus, std::vector<uint8_t>>
StaNetwork::getEapAnonymousIdentityInternal()
{
	struct wpa_ssid *wpa_ssid = retrieveNetworkPtr();
	if (!wpa_ssid->eap.anonymous_identity) {
		return {{SupplicantStatusCode::FAILURE_UNKNOWN, ""}, {}};
	}
	return {{SupplicantStatusCode::SUCCESS, ""},
		std::vector<uint8_t>(
		    wpa_ssid->eap.anonymous_identity,
		    wpa_ssid->eap.anonymous_identity +
			wpa_ssid->eap.anonymous_identity_len)};
}

std::pair<SupplicantStatus, std::vector<uint8_t>>
StaNetwork::getEapPasswordInternal()
{
	struct wpa_ssid *wpa_ssid = retrieveNetworkPtr();
	if (!wpa_ssid->eap.password) {
		return {{SupplicantStatusCode::FAILURE_UNKNOWN, ""}, {}};
	}
	return {{SupplicantStatusCode::SUCCESS, ""},
		std::vector<uint8_t>(
		    wpa_ssid->eap.password,
		    wpa_ssid->eap.password + wpa_ssid->eap.password_len)};
}

std::pair<SupplicantStatus, std::string> StaNetwork::getEapCACertInternal()
{
	struct wpa_ssid *wpa_ssid = retrieveNetworkPtr();
	if (!wpa_ssid->eap.ca_cert) {
		return {{SupplicantStatusCode::FAILURE_UNKNOWN, ""}, {}};
	}
	return {{SupplicantStatusCode::SUCCESS, ""},
		reinterpret_cast<char *>(wpa_ssid->eap.ca_cert)};
}

std::pair<SupplicantStatus, std::string> StaNetwork::getEapCAPathInternal()
{
	struct wpa_ssid *wpa_ssid = retrieveNetworkPtr();
	if (!wpa_ssid->eap.ca_path) {
		return {{SupplicantStatusCode::FAILURE_UNKNOWN, ""}, {}};
	}
	return {{SupplicantStatusCode::SUCCESS, ""},
		reinterpret_cast<char *>(wpa_ssid->eap.ca_path)};
}

std::pair<SupplicantStatus, std::string> StaNetwork::getEapClientCertInternal()
{
	struct wpa_ssid *wpa_ssid = retrieveNetworkPtr();
	if (!wpa_ssid->eap.client_cert) {
		return {{SupplicantStatusCode::FAILURE_UNKNOWN, ""}, {}};
	}
	return {{SupplicantStatusCode::SUCCESS, ""},
		reinterpret_cast<char *>(wpa_ssid->eap.client_cert)};
}

std::pair<SupplicantStatus, std::string>
StaNetwork::getEapPrivateKeyIdInternal()
{
	struct wpa_ssid *wpa_ssid = retrieveNetworkPtr();
	if (!wpa_ssid->eap.key_id) {
		return {{SupplicantStatusCode::FAILURE_UNKNOWN, ""}, {}};
	}
	return {{SupplicantStatusCode::SUCCESS, ""}, wpa_ssid->eap.key_id};
}

std::pair<SupplicantStatus, std::string>
StaNetwork::getEapSubjectMatchInternal()
{
	struct wpa_ssid *wpa_ssid = retrieveNetworkPtr();
	if (!wpa_ssid->eap.subject_match) {
		return {{SupplicantStatusCode::FAILURE_UNKNOWN, ""}, {}};
	}
	return {{SupplicantStatusCode::SUCCESS, ""},
		reinterpret_cast<char *>(wpa_ssid->eap.subject_match)};
}

std::pair<SupplicantStatus, std::string>
StaNetwork::getEapAltSubjectMatchInternal()
{
	struct wpa_ssid *wpa_ssid = retrieveNetworkPtr();
	if (!wpa_ssid->eap.altsubject_match) {
		return {{SupplicantStatusCode::FAILURE_UNKNOWN, ""}, {}};
	}
	return {{SupplicantStatusCode::SUCCESS, ""},
		reinterpret_cast<char *>(wpa_ssid->eap.altsubject_match)};
}

std::pair<SupplicantStatus, bool> StaNetwork::getEapEngineInternal()
{
	struct wpa_ssid *wpa_ssid = retrieveNetworkPtr();
	return {{SupplicantStatusCode::SUCCESS, ""}, wpa_ssid->eap.engine == 1};
}

std::pair<SupplicantStatus, std::string> StaNetwork::getEapEngineIDInternal()
{
	struct wpa_ssid *wpa_ssid = retrieveNetworkPtr();
	if (!wpa_ssid->eap.engine_id) {
		return {{SupplicantStatusCode::FAILURE_UNKNOWN, ""}, {}};
	}
	return {{SupplicantStatusCode::SUCCESS, ""}, {wpa_ssid->eap.engine_id}};
}

std::pair<SupplicantStatus, std::string>
StaNetwork::getEapDomainSuffixMatchInternal()
{
	struct wpa_ssid *wpa_ssid = retrieveNetworkPtr();
	if (!wpa_ssid->eap.domain_suffix_match) {
		return {{SupplicantStatusCode::FAILURE_UNKNOWN, ""}, {}};
	}
	return {{SupplicantStatusCode::SUCCESS, ""},
		{wpa_ssid->eap.domain_suffix_match}};
}

std::pair<SupplicantStatus, std::string> StaNetwork::getIdStrInternal()
{
	struct wpa_ssid *wpa_ssid = retrieveNetworkPtr();
	if (!wpa_ssid->id_str) {
		return {{SupplicantStatusCode::FAILURE_UNKNOWN, ""}, {}};
	}
	return {{SupplicantStatusCode::SUCCESS, ""}, {wpa_ssid->id_str}};
}

std::pair<SupplicantStatus, std::vector<uint8_t>>
StaNetwork::getWpsNfcConfigurationTokenInternal()
{
	struct wpa_supplicant *wpa_s = retrieveIfacePtr();
	struct wpa_ssid *wpa_ssid = retrieveNetworkPtr();
	auto token_buf = misc_utils::createWpaBufUniquePtr(
	    wpas_wps_network_config_token(wpa_s, 0, wpa_ssid));
	if (!token_buf) {
		return {{SupplicantStatusCode::FAILURE_UNKNOWN, ""}, {}};
	}
	return {{SupplicantStatusCode::SUCCESS, ""},
		misc_utils::convertWpaBufToVector(token_buf.get())};
}

SupplicantStatus StaNetwork::enableInternal(bool no_connect)
{
	struct wpa_ssid *wpa_ssid = retrieveNetworkPtr();
	if (wpa_ssid->disabled == 2) {
		return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	}
	struct wpa_supplicant *wpa_s = retrieveIfacePtr();
	if (no_connect) {
		wpa_ssid->disabled = 0;
	} else {
		wpa_s->scan_min_time.sec = 0;
		wpa_s->scan_min_time.usec = 0;
		wpa_supplicant_enable_network(wpa_s, wpa_ssid);
	}
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus StaNetwork::disableInternal()
{
	struct wpa_ssid *wpa_ssid = retrieveNetworkPtr();
	if (wpa_ssid->disabled == 2) {
		return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	}
	struct wpa_supplicant *wpa_s = retrieveIfacePtr();
	wpa_supplicant_disable_network(wpa_s, wpa_ssid);
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus StaNetwork::selectInternal()
{
	struct wpa_ssid *wpa_ssid = retrieveNetworkPtr();
	if (wpa_ssid->disabled == 2) {
		return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	}
	struct wpa_supplicant *wpa_s = retrieveIfacePtr();
	wpa_s->scan_min_time.sec = 0;
	wpa_s->scan_min_time.usec = 0;
	// Make sure that the supplicant is updated to the latest
	// MAC address, which might have changed due to MAC randomization.
	wpa_supplicant_update_mac_addr(wpa_s);
	wpa_supplicant_select_network(wpa_s, wpa_ssid);
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus StaNetwork::sendNetworkEapSimGsmAuthResponseInternal(
    const std::vector<ISupplicantStaNetwork::NetworkResponseEapSimGsmAuthParams>
	&vec_params)
{
	struct wpa_ssid *wpa_ssid = retrieveNetworkPtr();
	// Convert the incoming parameters to a string to pass to
	// wpa_supplicant.
	std::string ctrl_rsp_param = std::string(kNetworkEapSimGsmAuthResponse);
	for (const auto &params : vec_params) {
		uint32_t kc_hex_len = params.kc.size() * 2 + 1;
		std::vector<char> kc_hex(kc_hex_len);
		uint32_t sres_hex_len = params.sres.size() * 2 + 1;
		std::vector<char> sres_hex(sres_hex_len);
		wpa_snprintf_hex(
		    kc_hex.data(), kc_hex.size(), params.kc.data(),
		    params.kc.size());
		wpa_snprintf_hex(
		    sres_hex.data(), sres_hex.size(), params.sres.data(),
		    params.sres.size());
		ctrl_rsp_param += ":" + std::string(kc_hex.data()) + ":" +
				  std::string(sres_hex.data());
	}
	enum wpa_ctrl_req_type rtype = WPA_CTRL_REQ_SIM;
	struct wpa_supplicant *wpa_s = retrieveIfacePtr();
	if (wpa_supplicant_ctrl_rsp_handle(
		wpa_s, wpa_ssid, rtype, ctrl_rsp_param.c_str(),
		ctrl_rsp_param.size())) {
		return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	}
	eapol_sm_notify_ctrl_response(wpa_s->eapol);
	wpa_hexdump_ascii_key(
	    MSG_DEBUG, "network sim gsm auth response param",
	    (const u8 *)ctrl_rsp_param.c_str(), ctrl_rsp_param.size());
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus StaNetwork::sendNetworkEapSimGsmAuthFailureInternal()
{
	struct wpa_ssid *wpa_ssid = retrieveNetworkPtr();
	struct wpa_supplicant *wpa_s = retrieveIfacePtr();
	enum wpa_ctrl_req_type rtype = WPA_CTRL_REQ_SIM;
	if (wpa_supplicant_ctrl_rsp_handle(
		wpa_s, wpa_ssid, rtype, kNetworkEapSimGsmAuthFailure,
		strlen(kNetworkEapSimGsmAuthFailure))) {
		return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	}
	eapol_sm_notify_ctrl_response(wpa_s->eapol);
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus StaNetwork::sendNetworkEapSimUmtsAuthResponseInternal(
    const ISupplicantStaNetwork::NetworkResponseEapSimUmtsAuthParams &params)
{
	struct wpa_ssid *wpa_ssid = retrieveNetworkPtr();
	// Convert the incoming parameters to a string to pass to
	// wpa_supplicant.
	uint32_t ik_hex_len = params.ik.size() * 2 + 1;
	std::vector<char> ik_hex(ik_hex_len);
	uint32_t ck_hex_len = params.ck.size() * 2 + 1;
	std::vector<char> ck_hex(ck_hex_len);
	uint32_t res_hex_len = params.res.size() * 2 + 1;
	std::vector<char> res_hex(res_hex_len);
	wpa_snprintf_hex(
	    ik_hex.data(), ik_hex.size(), params.ik.data(), params.ik.size());
	wpa_snprintf_hex(
	    ck_hex.data(), ck_hex.size(), params.ck.data(), params.ck.size());
	wpa_snprintf_hex(
	    res_hex.data(), res_hex.size(), params.res.data(),
	    params.res.size());
	std::string ctrl_rsp_param =
	    std::string(kNetworkEapSimUmtsAuthResponse) + ":" +
	    std::string(ik_hex.data()) + ":" + std::string(ck_hex.data()) +
	    ":" + std::string(res_hex.data());
	enum wpa_ctrl_req_type rtype = WPA_CTRL_REQ_SIM;
	struct wpa_supplicant *wpa_s = retrieveIfacePtr();
	if (wpa_supplicant_ctrl_rsp_handle(
		wpa_s, wpa_ssid, rtype, ctrl_rsp_param.c_str(),
		ctrl_rsp_param.size())) {
		return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	}
	eapol_sm_notify_ctrl_response(wpa_s->eapol);
	wpa_hexdump_ascii_key(
	    MSG_DEBUG, "network sim umts auth response param",
	    (const u8 *)ctrl_rsp_param.c_str(), ctrl_rsp_param.size());
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus StaNetwork::sendNetworkEapSimUmtsAutsResponseInternal(
    const std::array<uint8_t, 14> &auts)
{
	struct wpa_ssid *wpa_ssid = retrieveNetworkPtr();
	uint32_t auts_hex_len = auts.size() * 2 + 1;
	std::vector<char> auts_hex(auts_hex_len);
	wpa_snprintf_hex(
	    auts_hex.data(), auts_hex.size(), auts.data(), auts.size());
	std::string ctrl_rsp_param =
	    std::string(kNetworkEapSimUmtsAutsResponse) + ":" +
	    std::string(auts_hex.data());
	enum wpa_ctrl_req_type rtype = WPA_CTRL_REQ_SIM;
	struct wpa_supplicant *wpa_s = retrieveIfacePtr();
	if (wpa_supplicant_ctrl_rsp_handle(
		wpa_s, wpa_ssid, rtype, ctrl_rsp_param.c_str(),
		ctrl_rsp_param.size())) {
		return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	}
	eapol_sm_notify_ctrl_response(wpa_s->eapol);
	wpa_hexdump_ascii_key(
	    MSG_DEBUG, "network sim umts auts response param",
	    (const u8 *)ctrl_rsp_param.c_str(), ctrl_rsp_param.size());
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus StaNetwork::sendNetworkEapSimUmtsAuthFailureInternal()
{
	struct wpa_ssid *wpa_ssid = retrieveNetworkPtr();
	struct wpa_supplicant *wpa_s = retrieveIfacePtr();
	enum wpa_ctrl_req_type rtype = WPA_CTRL_REQ_SIM;
	if (wpa_supplicant_ctrl_rsp_handle(
		wpa_s, wpa_ssid, rtype, kNetworkEapSimUmtsAuthFailure,
		strlen(kNetworkEapSimUmtsAuthFailure))) {
		return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	}
	eapol_sm_notify_ctrl_response(wpa_s->eapol);
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus StaNetwork::sendNetworkEapIdentityResponseInternal(
    const std::vector<uint8_t> &identity)
{
	struct wpa_ssid *wpa_ssid = retrieveNetworkPtr();
	std::string ctrl_rsp_param(identity.begin(), identity.end());
	enum wpa_ctrl_req_type rtype = WPA_CTRL_REQ_EAP_IDENTITY;
	struct wpa_supplicant *wpa_s = retrieveIfacePtr();
	if (wpa_supplicant_ctrl_rsp_handle(
		wpa_s, wpa_ssid, rtype, ctrl_rsp_param.c_str(),
		ctrl_rsp_param.size())) {
		return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	}
	eapol_sm_notify_ctrl_response(wpa_s->eapol);
	wpa_hexdump_ascii_key(
	    MSG_DEBUG, "network identity response param",
	    (const u8 *)ctrl_rsp_param.c_str(), ctrl_rsp_param.size());
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus StaNetwork::sendNetworkEapIdentityResponseInternal_1_1(
    const std::vector<uint8_t> &identity, const std::vector<uint8_t> &encrypted_imsi_identity)
{
	struct wpa_ssid *wpa_ssid = retrieveNetworkPtr();
	// format: plain identity + ":" + encrypted identity(encrypted_imsi_identity)
	std::string ctrl_rsp_param =
		std::string(identity.begin(), identity.end()) + ":" +
		std::string(encrypted_imsi_identity.begin(), encrypted_imsi_identity.end());
	enum wpa_ctrl_req_type rtype = WPA_CTRL_REQ_EAP_IDENTITY;
	struct wpa_supplicant *wpa_s = retrieveIfacePtr();
	if (wpa_supplicant_ctrl_rsp_handle(
		wpa_s, wpa_ssid, rtype, ctrl_rsp_param.c_str(),
		ctrl_rsp_param.size())) {
		return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	}
	eapol_sm_notify_ctrl_response(wpa_s->eapol);
	wpa_hexdump_ascii_key(
	    MSG_DEBUG, "network identity response param",
	    (const u8 *)ctrl_rsp_param.c_str(), ctrl_rsp_param.size());
	return {SupplicantStatusCode::SUCCESS, ""};
}

/**
 * Retrieve the underlying |wpa_ssid| struct pointer for
 * this network.
 * If the underlying network is removed or the interface
 * this network belong to
 * is removed, all RPC method calls on this object will
 * return failure.
 */
struct wpa_ssid *StaNetwork::retrieveNetworkPtr()
{
	wpa_supplicant *wpa_s = retrieveIfacePtr();
	if (!wpa_s)
		return nullptr;
	return wpa_config_get_network(wpa_s->conf, network_id_);
}

/**
 * Retrieve the underlying |wpa_supplicant| struct
 * pointer for
 * this network.
 */
struct wpa_supplicant *StaNetwork::retrieveIfacePtr()
{
	return wpa_supplicant_get_iface(wpa_global_, ifname_.c_str());
}

/**
 * Check if the provided psk passhrase is valid or not.
 *
 * Returns 0 if valid, 1 otherwise.
 */
int StaNetwork::isPskPassphraseValid(const std::string &psk)
{
	if (psk.size() <
		static_cast<uint32_t>(ISupplicantStaNetwork::ParamSizeLimits::
					  PSK_PASSPHRASE_MIN_LEN_IN_BYTES) ||
	    psk.size() >
		static_cast<uint32_t>(ISupplicantStaNetwork::ParamSizeLimits::
					  PSK_PASSPHRASE_MAX_LEN_IN_BYTES)) {
		return 1;
	}
	if (has_ctrl_char((u8 *)psk.c_str(), psk.size())) {
		return 1;
	}
	return 0;
}

/**
 * Reset internal wpa_supplicant state machine state
 * after params update (except
 * bssid).
 */
void StaNetwork::resetInternalStateAfterParamsUpdate()
{
	struct wpa_supplicant *wpa_s = retrieveIfacePtr();
	struct wpa_ssid *wpa_ssid = retrieveNetworkPtr();

	wpa_sm_pmksa_cache_flush(wpa_s->wpa, wpa_ssid);

	if (wpa_s->current_ssid == wpa_ssid || wpa_s->current_ssid == NULL) {
		/*
		 * Invalidate the EAP session cache if
		 * anything in the
		 * current or previously used
		 * configuration changes.
		 */
		eapol_sm_invalidate_cached_session(wpa_s->eapol);
	}
}

/**
 * Helper function to set value in a string field in |wpa_ssid| structue
 * instance for this network.
 * This function frees any existing data in these fields.
 */
int StaNetwork::setStringFieldAndResetState(
    const char *value, uint8_t **to_update_field, const char *hexdump_prefix)
{
	return setStringFieldAndResetState(
	    value, (char **)to_update_field, hexdump_prefix);
}

/**
 * Helper function to set value in a string field in |wpa_ssid| structue
 * instance for this network.
 * This function frees any existing data in these fields.
 */
int StaNetwork::setStringFieldAndResetState(
    const char *value, char **to_update_field, const char *hexdump_prefix)
{
	int value_len = strlen(value);
	if (*to_update_field) {
		os_free(*to_update_field);
	}
	*to_update_field = dup_binstr(value, value_len);
	if (!(*to_update_field)) {
		return 1;
	}
	wpa_hexdump_ascii(
	    MSG_MSGDUMP, hexdump_prefix, *to_update_field, value_len);
	resetInternalStateAfterParamsUpdate();
	return 0;
}

/**
 * Helper function to set value in a string key field in |wpa_ssid| structue
 * instance for this network.
 * This function frees any existing data in these fields.
 */
int StaNetwork::setStringKeyFieldAndResetState(
    const char *value, char **to_update_field, const char *hexdump_prefix)
{
	int value_len = strlen(value);
	if (*to_update_field) {
		str_clear_free(*to_update_field);
	}
	*to_update_field = dup_binstr(value, value_len);
	if (!(*to_update_field)) {
		return 1;
	}
	wpa_hexdump_ascii_key(
	    MSG_MSGDUMP, hexdump_prefix, *to_update_field, value_len);
	resetInternalStateAfterParamsUpdate();
	return 0;
}

/**
 * Helper function to set value in a string field with a corresponding length
 * field in |wpa_ssid| structue instance for this network.
 * This function frees any existing data in these fields.
 */
int StaNetwork::setByteArrayFieldAndResetState(
    const uint8_t *value, const size_t value_len, uint8_t **to_update_field,
    size_t *to_update_field_len, const char *hexdump_prefix)
{
	if (*to_update_field) {
		os_free(*to_update_field);
	}
	*to_update_field = (uint8_t *)os_malloc(value_len);
	if (!(*to_update_field)) {
		return 1;
	}
	os_memcpy(*to_update_field, value, value_len);
	*to_update_field_len = value_len;

	wpa_hexdump_ascii(
	    MSG_MSGDUMP, hexdump_prefix, *to_update_field,
	    *to_update_field_len);
	resetInternalStateAfterParamsUpdate();
	return 0;
}

/**
 * Helper function to set value in a string key field with a corresponding
 * length field in |wpa_ssid| structue instance for this network.
 * This function frees any existing data in these fields.
 */
int StaNetwork::setByteArrayKeyFieldAndResetState(
    const uint8_t *value, const size_t value_len, uint8_t **to_update_field,
    size_t *to_update_field_len, const char *hexdump_prefix)
{
	if (*to_update_field) {
		bin_clear_free(*to_update_field, *to_update_field_len);
	}
	*to_update_field = (uint8_t *)os_malloc(value_len);
	if (!(*to_update_field)) {
		return 1;
	}
	os_memcpy(*to_update_field, value, value_len);
	*to_update_field_len = value_len;

	wpa_hexdump_ascii_key(
	    MSG_MSGDUMP, hexdump_prefix, *to_update_field,
	    *to_update_field_len);
	resetInternalStateAfterParamsUpdate();
	return 0;
}

}  // namespace implementation
}  // namespace V1_1
}  // namespace wifi
}  // namespace supplicant
}  // namespace hardware
}  // namespace android
