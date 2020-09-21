/*
 * hidl interface for wpa_supplicant daemon
 * Copyright (c) 2004-2016, Jouni Malinen <j@w1.fi>
 * Copyright (c) 2004-2016, Roshan Pius <rpius@google.com>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include <hwbinder/IPCThreadState.h>

#include <hidl/HidlTransportSupport.h>
#include "hidl_manager.h"

extern "C" {
#include "hidl.h"
#include "hidl_i.h"
#include "utils/common.h"
#include "utils/eloop.h"
#include "utils/includes.h"
}

using android::hardware::configureRpcThreadpool;
using android::hardware::setupTransportPolling;
using android::hardware::handleTransportPoll;
using android::hardware::wifi::supplicant::V1_1::implementation::HidlManager;

void wpas_hidl_sock_handler(
    int sock, void * /* eloop_ctx */, void * /* sock_ctx */)
{
	handleTransportPoll(sock);
}

struct wpas_hidl_priv *wpas_hidl_init(struct wpa_global *global)
{
	struct wpas_hidl_priv *priv;
	HidlManager *hidl_manager;

	priv = (wpas_hidl_priv *)os_zalloc(sizeof(*priv));
	if (!priv)
		return NULL;
	priv->global = global;

	wpa_printf(MSG_DEBUG, "Initing hidl control");

	configureRpcThreadpool(1, true /* callerWillJoin */);
	priv->hidl_fd = setupTransportPolling();
	if (priv->hidl_fd < 0)
		goto err;

	wpa_printf(MSG_INFO, "Processing hidl events on FD %d", priv->hidl_fd);
	// Look for read events from the hidl socket in the eloop.
	if (eloop_register_read_sock(
		priv->hidl_fd, wpas_hidl_sock_handler, global, priv) < 0)
		goto err;

	hidl_manager = HidlManager::getInstance();
	if (!hidl_manager)
		goto err;
	hidl_manager->registerHidlService(global);
	// We may not need to store this hidl manager reference in the
	// global data strucure because we've made it a singleton class.
	priv->hidl_manager = (void *)hidl_manager;

	return priv;
err:
	wpas_hidl_deinit(priv);
	return NULL;
}

void wpas_hidl_deinit(struct wpas_hidl_priv *priv)
{
	if (!priv)
		return;

	wpa_printf(MSG_DEBUG, "Deiniting hidl control");

	HidlManager::destroyInstance();
	eloop_unregister_read_sock(priv->hidl_fd);
	os_free(priv);
}

int wpas_hidl_register_interface(struct wpa_supplicant *wpa_s)
{
	if (!wpa_s || !wpa_s->global->hidl)
		return 1;

	wpa_printf(
	    MSG_DEBUG, "Registering interface to hidl control: %s",
	    wpa_s->ifname);

	HidlManager *hidl_manager = HidlManager::getInstance();
	if (!hidl_manager)
		return 1;

	return hidl_manager->registerInterface(wpa_s);
}

int wpas_hidl_unregister_interface(struct wpa_supplicant *wpa_s)
{
	if (!wpa_s || !wpa_s->global->hidl)
		return 1;

	wpa_printf(
	    MSG_DEBUG, "Deregistering interface from hidl control: %s",
	    wpa_s->ifname);

	HidlManager *hidl_manager = HidlManager::getInstance();
	if (!hidl_manager)
		return 1;

	return hidl_manager->unregisterInterface(wpa_s);
}

int wpas_hidl_register_network(
    struct wpa_supplicant *wpa_s, struct wpa_ssid *ssid)
{
	if (!wpa_s || !wpa_s->global->hidl || !ssid)
		return 1;

	wpa_printf(
	    MSG_DEBUG, "Registering network to hidl control: %d", ssid->id);

	HidlManager *hidl_manager = HidlManager::getInstance();
	if (!hidl_manager)
		return 1;

	return hidl_manager->registerNetwork(wpa_s, ssid);
}

int wpas_hidl_unregister_network(
    struct wpa_supplicant *wpa_s, struct wpa_ssid *ssid)
{
	if (!wpa_s || !wpa_s->global->hidl || !ssid)
		return 1;

	wpa_printf(
	    MSG_DEBUG, "Deregistering network from hidl control: %d", ssid->id);

	HidlManager *hidl_manager = HidlManager::getInstance();
	if (!hidl_manager)
		return 1;

	return hidl_manager->unregisterNetwork(wpa_s, ssid);
}

int wpas_hidl_notify_state_changed(struct wpa_supplicant *wpa_s)
{
	if (!wpa_s || !wpa_s->global->hidl)
		return 1;

	wpa_printf(
	    MSG_DEBUG, "Notifying state change event to hidl control: %d",
	    wpa_s->wpa_state);

	HidlManager *hidl_manager = HidlManager::getInstance();
	if (!hidl_manager)
		return 1;

	return hidl_manager->notifyStateChange(wpa_s);
}

int wpas_hidl_notify_network_request(
    struct wpa_supplicant *wpa_s, struct wpa_ssid *ssid,
    enum wpa_ctrl_req_type rtype, const char *default_txt)
{
	if (!wpa_s || !wpa_s->global->hidl || !ssid)
		return 1;

	wpa_printf(
	    MSG_DEBUG, "Notifying network request to hidl control: %d",
	    ssid->id);

	HidlManager *hidl_manager = HidlManager::getInstance();
	if (!hidl_manager)
		return 1;

	return hidl_manager->notifyNetworkRequest(
	    wpa_s, ssid, rtype, default_txt);
}

void wpas_hidl_notify_anqp_query_done(
    struct wpa_supplicant *wpa_s, const u8 *bssid, const char *result,
    const struct wpa_bss_anqp *anqp)
{
	if (!wpa_s || !wpa_s->global->hidl || !bssid || !result || !anqp)
		return;

	wpa_printf(
	    MSG_DEBUG,
	    "Notifying ANQP query done to hidl control: " MACSTR "result: %s",
	    MAC2STR(bssid), result);

	HidlManager *hidl_manager = HidlManager::getInstance();
	if (!hidl_manager)
		return;

	hidl_manager->notifyAnqpQueryDone(wpa_s, bssid, result, anqp);
}

void wpas_hidl_notify_hs20_icon_query_done(
    struct wpa_supplicant *wpa_s, const u8 *bssid, const char *file_name,
    const u8 *image, u32 image_length)
{
	if (!wpa_s || !wpa_s->global->hidl || !bssid || !file_name || !image)
		return;

	wpa_printf(
	    MSG_DEBUG, "Notifying HS20 icon query done to hidl control: " MACSTR
		       "file_name: %s",
	    MAC2STR(bssid), file_name);

	HidlManager *hidl_manager = HidlManager::getInstance();
	if (!hidl_manager)
		return;

	hidl_manager->notifyHs20IconQueryDone(
	    wpa_s, bssid, file_name, image, image_length);
}

void wpas_hidl_notify_hs20_rx_subscription_remediation(
    struct wpa_supplicant *wpa_s, const char *url, u8 osu_method)
{
	if (!wpa_s || !wpa_s->global->hidl || !url)
		return;

	wpa_printf(
	    MSG_DEBUG,
	    "Notifying HS20 subscription remediation rx to hidl control: %s",
	    url);

	HidlManager *hidl_manager = HidlManager::getInstance();
	if (!hidl_manager)
		return;

	hidl_manager->notifyHs20RxSubscriptionRemediation(
	    wpa_s, url, osu_method);
}

void wpas_hidl_notify_hs20_rx_deauth_imminent_notice(
    struct wpa_supplicant *wpa_s, u8 code, u16 reauth_delay, const char *url)
{
	if (!wpa_s || !wpa_s->global->hidl || !url)
		return;

	wpa_printf(
	    MSG_DEBUG,
	    "Notifying HS20 deauth imminent notice rx to hidl control: %s",
	    url);

	HidlManager *hidl_manager = HidlManager::getInstance();
	if (!hidl_manager)
		return;

	hidl_manager->notifyHs20RxDeauthImminentNotice(
	    wpa_s, code, reauth_delay, url);
}

void wpas_hidl_notify_disconnect_reason(struct wpa_supplicant *wpa_s)
{
	if (!wpa_s)
		return;

	wpa_printf(
	    MSG_DEBUG, "Notifying disconnect reason to hidl control: %d",
	    wpa_s->disconnect_reason);

	HidlManager *hidl_manager = HidlManager::getInstance();
	if (!hidl_manager)
		return;

	hidl_manager->notifyDisconnectReason(wpa_s);
}

void wpas_hidl_notify_assoc_reject(struct wpa_supplicant *wpa_s)
{
	if (!wpa_s)
		return;

	wpa_printf(
	    MSG_DEBUG, "Notifying assoc reject to hidl control: %d",
	    wpa_s->assoc_status_code);

	HidlManager *hidl_manager = HidlManager::getInstance();
	if (!hidl_manager)
		return;

	hidl_manager->notifyAssocReject(wpa_s);
}

void wpas_hidl_notify_auth_timeout(struct wpa_supplicant *wpa_s)
{
	if (!wpa_s)
		return;

	wpa_printf(MSG_DEBUG, "Notifying auth timeout to hidl control");

	HidlManager *hidl_manager = HidlManager::getInstance();
	if (!hidl_manager)
		return;

	hidl_manager->notifyAuthTimeout(wpa_s);
}

void wpas_hidl_notify_bssid_changed(struct wpa_supplicant *wpa_s)
{
	if (!wpa_s)
		return;

	wpa_printf(MSG_DEBUG, "Notifying bssid changed to hidl control");

	HidlManager *hidl_manager = HidlManager::getInstance();
	if (!hidl_manager)
		return;

	hidl_manager->notifyBssidChanged(wpa_s);
}

void wpas_hidl_notify_wps_event_fail(
    struct wpa_supplicant *wpa_s, uint8_t *peer_macaddr, uint16_t config_error,
    uint16_t error_indication)
{
	if (!wpa_s || !peer_macaddr)
		return;

	wpa_printf(
	    MSG_DEBUG, "Notifying Wps event fail to hidl control: %d, %d",
	    config_error, error_indication);

	HidlManager *hidl_manager = HidlManager::getInstance();
	if (!hidl_manager)
		return;

	hidl_manager->notifyWpsEventFail(
	    wpa_s, peer_macaddr, config_error, error_indication);
}

void wpas_hidl_notify_wps_event_success(struct wpa_supplicant *wpa_s)
{
	if (!wpa_s)
		return;

	wpa_printf(MSG_DEBUG, "Notifying Wps event success to hidl control");

	HidlManager *hidl_manager = HidlManager::getInstance();
	if (!hidl_manager)
		return;

	hidl_manager->notifyWpsEventSuccess(wpa_s);
}

void wpas_hidl_notify_wps_event_pbc_overlap(struct wpa_supplicant *wpa_s)
{
	if (!wpa_s)
		return;

	wpa_printf(
	    MSG_DEBUG, "Notifying Wps event PBC overlap to hidl control");

	HidlManager *hidl_manager = HidlManager::getInstance();
	if (!hidl_manager)
		return;

	hidl_manager->notifyWpsEventPbcOverlap(wpa_s);
}

void wpas_hidl_notify_p2p_device_found(
    struct wpa_supplicant *wpa_s, const u8 *addr,
    const struct p2p_peer_info *info, const u8 *peer_wfd_device_info,
    u8 peer_wfd_device_info_len)
{
	if (!wpa_s || !addr || !info)
		return;

	wpa_printf(
	    MSG_DEBUG, "Notifying P2P device found to hidl control " MACSTR,
	    MAC2STR(info->p2p_device_addr));

	HidlManager *hidl_manager = HidlManager::getInstance();
	if (!hidl_manager)
		return;

	hidl_manager->notifyP2pDeviceFound(
	    wpa_s, addr, info, peer_wfd_device_info, peer_wfd_device_info_len);
}

void wpas_hidl_notify_p2p_device_lost(
    struct wpa_supplicant *wpa_s, const u8 *p2p_device_addr)
{
	if (!wpa_s || !p2p_device_addr)
		return;

	wpa_printf(
	    MSG_DEBUG, "Notifying P2P device lost to hidl control " MACSTR,
	    MAC2STR(p2p_device_addr));

	HidlManager *hidl_manager = HidlManager::getInstance();
	if (!hidl_manager)
		return;

	hidl_manager->notifyP2pDeviceLost(wpa_s, p2p_device_addr);
}

void wpas_hidl_notify_p2p_find_stopped(struct wpa_supplicant *wpa_s)
{
	if (!wpa_s)
		return;

	wpa_printf(MSG_DEBUG, "Notifying P2P find stop to hidl control");

	HidlManager *hidl_manager = HidlManager::getInstance();
	if (!hidl_manager)
		return;

	hidl_manager->notifyP2pFindStopped(wpa_s);
}

void wpas_hidl_notify_p2p_go_neg_req(
    struct wpa_supplicant *wpa_s, const u8 *src_addr, u16 dev_passwd_id,
    u8 go_intent)
{
	if (!wpa_s || !src_addr)
		return;

	wpa_printf(
	    MSG_DEBUG,
	    "Notifying P2P GO negotiation request to hidl control " MACSTR,
	    MAC2STR(src_addr));

	HidlManager *hidl_manager = HidlManager::getInstance();
	if (!hidl_manager)
		return;

	hidl_manager->notifyP2pGoNegReq(
	    wpa_s, src_addr, dev_passwd_id, go_intent);
}

void wpas_hidl_notify_p2p_go_neg_completed(
    struct wpa_supplicant *wpa_s, const struct p2p_go_neg_results *res)
{
	if (!wpa_s || !res)
		return;

	wpa_printf(
	    MSG_DEBUG,
	    "Notifying P2P GO negotiation completed to hidl control: %d",
	    res->status);

	HidlManager *hidl_manager = HidlManager::getInstance();
	if (!hidl_manager)
		return;

	hidl_manager->notifyP2pGoNegCompleted(wpa_s, res);
}

void wpas_hidl_notify_p2p_group_formation_failure(
    struct wpa_supplicant *wpa_s, const char *reason)
{
	if (!wpa_s || !reason)
		return;

	wpa_printf(
	    MSG_DEBUG,
	    "Notifying P2P Group formation failure to hidl control: %s",
	    reason);

	HidlManager *hidl_manager = HidlManager::getInstance();
	if (!hidl_manager)
		return;

	hidl_manager->notifyP2pGroupFormationFailure(wpa_s, reason);
}

void wpas_hidl_notify_p2p_group_started(
    struct wpa_supplicant *wpa_s, const struct wpa_ssid *ssid, int persistent,
    int client)
{
	if (!wpa_s || !ssid)
		return;

	wpa_printf(
	    MSG_DEBUG, "Notifying P2P Group start to hidl control: %d",
	    ssid->id);

	HidlManager *hidl_manager = HidlManager::getInstance();
	if (!hidl_manager)
		return;

	hidl_manager->notifyP2pGroupStarted(wpa_s, ssid, persistent, client);
}

void wpas_hidl_notify_p2p_group_removed(
    struct wpa_supplicant *wpa_s, const struct wpa_ssid *ssid, const char *role)
{
	if (!wpa_s || !ssid || !role)
		return;

	wpa_printf(
	    MSG_DEBUG, "Notifying P2P Group removed to hidl control: %d",
	    ssid->id);

	HidlManager *hidl_manager = HidlManager::getInstance();
	if (!hidl_manager)
		return;

	hidl_manager->notifyP2pGroupRemoved(wpa_s, ssid, role);
}

void wpas_hidl_notify_p2p_invitation_received(
    struct wpa_supplicant *wpa_s, const u8 *sa, const u8 *go_dev_addr,
    const u8 *bssid, int id, int op_freq)
{
	if (!wpa_s || !sa || !go_dev_addr || !bssid)
		return;

	wpa_printf(
	    MSG_DEBUG,
	    "Notifying P2P invitation received to hidl control: %d " MACSTR, id,
	    MAC2STR(bssid));

	HidlManager *hidl_manager = HidlManager::getInstance();
	if (!hidl_manager)
		return;

	hidl_manager->notifyP2pInvitationReceived(
	    wpa_s, sa, go_dev_addr, bssid, id, op_freq);
}

void wpas_hidl_notify_p2p_invitation_result(
    struct wpa_supplicant *wpa_s, int status, const u8 *bssid)
{
	if (!wpa_s)
		return;
	if (bssid) {
		wpa_printf(
			MSG_DEBUG,
			"Notifying P2P invitation result to hidl control: " MACSTR,
			MAC2STR(bssid));
	} else {
		wpa_printf(
			MSG_DEBUG,
			"Notifying P2P invitation result to hidl control: NULL bssid");
	}

	HidlManager *hidl_manager = HidlManager::getInstance();
	if (!hidl_manager)
		return;

	hidl_manager->notifyP2pInvitationResult(wpa_s, status, bssid);
}

void wpas_hidl_notify_p2p_provision_discovery(
    struct wpa_supplicant *wpa_s, const u8 *dev_addr, int request,
    enum p2p_prov_disc_status status, u16 config_methods,
    unsigned int generated_pin)
{
	if (!wpa_s || !dev_addr)
		return;

	wpa_printf(
	    MSG_DEBUG,
	    "Notifying P2P provision discovery to hidl control " MACSTR,
	    MAC2STR(dev_addr));

	HidlManager *hidl_manager = HidlManager::getInstance();
	if (!hidl_manager)
		return;

	hidl_manager->notifyP2pProvisionDiscovery(
	    wpa_s, dev_addr, request, status, config_methods, generated_pin);
}

void wpas_hidl_notify_p2p_sd_response(
    struct wpa_supplicant *wpa_s, const u8 *sa, u16 update_indic,
    const u8 *tlvs, size_t tlvs_len)
{
	if (!wpa_s || !sa || !tlvs)
		return;

	wpa_printf(
	    MSG_DEBUG,
	    "Notifying P2P service discovery response to hidl control " MACSTR,
	    MAC2STR(sa));

	HidlManager *hidl_manager = HidlManager::getInstance();
	if (!hidl_manager)
		return;

	hidl_manager->notifyP2pSdResponse(
	    wpa_s, sa, update_indic, tlvs, tlvs_len);
}

void wpas_hidl_notify_ap_sta_authorized(
    struct wpa_supplicant *wpa_s, const u8 *sta, const u8 *p2p_dev_addr)
{
	if (!wpa_s || !sta)
		return;

	wpa_printf(
	    MSG_DEBUG,
	    "Notifying P2P AP STA authorized to hidl control " MACSTR,
	    MAC2STR(sta));

	HidlManager *hidl_manager = HidlManager::getInstance();
	if (!hidl_manager)
		return;

	hidl_manager->notifyApStaAuthorized(wpa_s, sta, p2p_dev_addr);
}

void wpas_hidl_notify_ap_sta_deauthorized(
    struct wpa_supplicant *wpa_s, const u8 *sta, const u8 *p2p_dev_addr)
{
	if (!wpa_s || !sta)
		return;

	wpa_printf(
	    MSG_DEBUG,
	    "Notifying P2P AP STA deauthorized to hidl control " MACSTR,
	    MAC2STR(sta));

	HidlManager *hidl_manager = HidlManager::getInstance();
	if (!hidl_manager)
		return;

	hidl_manager->notifyApStaDeauthorized(wpa_s, sta, p2p_dev_addr);
}

void wpas_hidl_notify_eap_error(
    struct wpa_supplicant *wpa_s, int error_code)
{
	if (!wpa_s)
		return;

	wpa_printf(
	    MSG_DEBUG,
            "Notifying EAP Error: %d ", error_code);

	HidlManager *hidl_manager = HidlManager::getInstance();
	if (!hidl_manager)
		return;

	hidl_manager->notifyEapError(wpa_s, error_code);
}

