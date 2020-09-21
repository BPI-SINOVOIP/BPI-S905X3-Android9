/*
 * hidl interface for wpa_supplicant daemon
 * Copyright (c) 2004-2016, Jouni Malinen <j@w1.fi>
 * Copyright (c) 2004-2016, Roshan Pius <rpius@google.com>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#ifndef WPA_SUPPLICANT_HIDL_HIDL_H
#define WPA_SUPPLICANT_HIDL_HIDL_H

#ifdef _cplusplus
extern "C" {
#endif  // _cplusplus

/**
 * This is the hidl RPC interface entry point to the wpa_supplicant core.
 * This initializes the hidl driver & HidlManager instance and then forwards
 * all the notifcations from the supplicant core to the HidlManager.
 */
struct wpas_hidl_priv;
struct wpa_global;

struct wpas_hidl_priv *wpas_hidl_init(struct wpa_global *global);
void wpas_hidl_deinit(struct wpas_hidl_priv *priv);

#ifdef CONFIG_CTRL_IFACE_HIDL
int wpas_hidl_register_interface(struct wpa_supplicant *wpa_s);
int wpas_hidl_unregister_interface(struct wpa_supplicant *wpa_s);
int wpas_hidl_register_network(
    struct wpa_supplicant *wpa_s, struct wpa_ssid *ssid);
int wpas_hidl_unregister_network(
    struct wpa_supplicant *wpa_s, struct wpa_ssid *ssid);
int wpas_hidl_notify_state_changed(struct wpa_supplicant *wpa_s);
int wpas_hidl_notify_network_request(
    struct wpa_supplicant *wpa_s, struct wpa_ssid *ssid,
    enum wpa_ctrl_req_type rtype, const char *default_txt);
void wpas_hidl_notify_anqp_query_done(
    struct wpa_supplicant *wpa_s, const u8 *bssid, const char *result,
    const struct wpa_bss_anqp *anqp);
void wpas_hidl_notify_hs20_icon_query_done(
    struct wpa_supplicant *wpa_s, const u8 *bssid, const char *file_name,
    const u8 *image, u32 image_length);
void wpas_hidl_notify_hs20_rx_subscription_remediation(
    struct wpa_supplicant *wpa_s, const char *url, u8 osu_method);
void wpas_hidl_notify_hs20_rx_deauth_imminent_notice(
    struct wpa_supplicant *wpa_s, u8 code, u16 reauth_delay, const char *url);
void wpas_hidl_notify_disconnect_reason(struct wpa_supplicant *wpa_s);
void wpas_hidl_notify_assoc_reject(struct wpa_supplicant *wpa_s);
void wpas_hidl_notify_auth_timeout(struct wpa_supplicant *wpa_s);
void wpas_hidl_notify_bssid_changed(struct wpa_supplicant *wpa_s);
void wpas_hidl_notify_wps_event_fail(
    struct wpa_supplicant *wpa_s, uint8_t *peer_macaddr, uint16_t config_error,
    uint16_t error_indication);
void wpas_hidl_notify_wps_event_success(struct wpa_supplicant *wpa_s);
void wpas_hidl_notify_wps_event_pbc_overlap(struct wpa_supplicant *wpa_s);
void wpas_hidl_notify_p2p_device_found(
    struct wpa_supplicant *wpa_s, const u8 *addr,
    const struct p2p_peer_info *info, const u8 *peer_wfd_device_info,
    u8 peer_wfd_device_info_len);
void wpas_hidl_notify_p2p_device_lost(
    struct wpa_supplicant *wpa_s, const u8 *p2p_device_addr);
void wpas_hidl_notify_p2p_find_stopped(struct wpa_supplicant *wpa_s);
void wpas_hidl_notify_p2p_go_neg_req(
    struct wpa_supplicant *wpa_s, const u8 *src_addr, u16 dev_passwd_id,
    u8 go_intent);
void wpas_hidl_notify_p2p_go_neg_completed(
    struct wpa_supplicant *wpa_s, const struct p2p_go_neg_results *res);
void wpas_hidl_notify_p2p_group_formation_failure(
    struct wpa_supplicant *wpa_s, const char *reason);
void wpas_hidl_notify_p2p_group_started(
    struct wpa_supplicant *wpa_s, const struct wpa_ssid *ssid, int persistent,
    int client);
void wpas_hidl_notify_p2p_group_removed(
    struct wpa_supplicant *wpa_s, const struct wpa_ssid *ssid,
    const char *role);
void wpas_hidl_notify_p2p_invitation_received(
    struct wpa_supplicant *wpa_s, const u8 *sa, const u8 *go_dev_addr,
    const u8 *bssid, int id, int op_freq);
void wpas_hidl_notify_p2p_invitation_result(
    struct wpa_supplicant *wpa_s, int status, const u8 *bssid);
void wpas_hidl_notify_p2p_provision_discovery(
    struct wpa_supplicant *wpa_s, const u8 *dev_addr, int request,
    enum p2p_prov_disc_status status, u16 config_methods,
    unsigned int generated_pin);
void wpas_hidl_notify_p2p_sd_response(
    struct wpa_supplicant *wpa_s, const u8 *sa, u16 update_indic,
    const u8 *tlvs, size_t tlvs_len);
void wpas_hidl_notify_ap_sta_authorized(
    struct wpa_supplicant *wpa_s, const u8 *sta, const u8 *p2p_dev_addr);
void wpas_hidl_notify_ap_sta_deauthorized(
    struct wpa_supplicant *wpa_s, const u8 *sta, const u8 *p2p_dev_addr);
#else   // CONFIG_CTRL_IFACE_HIDL
static inline int wpas_hidl_register_interface(struct wpa_supplicant *wpa_s)
{
	return 0;
}
static inline int wpas_hidl_unregister_interface(struct wpa_supplicant *wpa_s)
{
	return 0;
}
static inline int wpas_hidl_register_network(
    struct wpa_supplicant *wpa_s, struct wpa_ssid *ssid)
{
	return 0;
}
static inline int wpas_hidl_unregister_network(
    struct wpa_supplicant *wpa_s, struct wpa_ssid *ssid)
{
	return 0;
}
static inline int wpas_hidl_notify_state_changed(struct wpa_supplicant *wpa_s)
{
	return 0;
}
static inline int wpas_hidl_notify_network_request(
    struct wpa_supplicant *wpa_s, struct wpa_ssid *ssid,
    enum wpa_ctrl_req_type rtype, const char *default_txt)
{
	return 0;
}
static void wpas_hidl_notify_anqp_query_done(
    struct wpa_supplicant *wpa_s, const u8 *bssid, const char *result,
    const struct wpa_bss_anqp *anqp)
{
}
static void wpas_hidl_notify_hs20_icon_query_done(
    struct wpa_supplicant *wpa_s, const u8 *bssid, const char *file_name,
    const u8 *image, u32 image_length)
{
}
static void wpas_hidl_notify_hs20_rx_subscription_remediation(
    struct wpa_supplicant *wpa_s, const char *url, u8 osu_method)
{
}
static void wpas_hidl_notify_hs20_rx_deauth_imminent_notice(
    struct wpa_supplicant *wpa_s, u8 code, u16 reauth_delay, const char *url)
{
}
static void wpas_hidl_notify_disconnect_reason(struct wpa_supplicant *wpa_s) {}
static void wpas_hidl_notify_assoc_reject(struct wpa_supplicant *wpa_s) {}
static void wpas_hidl_notify_auth_timeout(struct wpa_supplicant *wpa_s) {}
static void wpas_hidl_notify_wps_event_fail(
    struct wpa_supplicant *wpa_s, uint8_t *peer_macaddr, uint16_t config_error,
    uint16_t error_indication)
{
}
static void wpas_hidl_notify_bssid_changed(struct wpa_supplicant *wpa_s) {}
static void wpas_hidl_notify_wps_event_success(struct wpa_supplicant *wpa_s) {}
static void wpas_hidl_notify_wps_event_pbc_overlap(struct wpa_supplicant *wpa_s)
{
}
static void wpas_hidl_notify_p2p_device_found(
    struct wpa_supplicant *wpa_s, const u8 *addr,
    const struct p2p_peer_info *info, const u8 *peer_wfd_device_info,
    u8 peer_wfd_device_info_len);
{
}
static void wpas_hidl_notify_p2p_device_lost(
    struct wpa_supplicant *wpa_s, const u8 *p2p_device_addr)
{
}
static void wpas_hidl_notify_p2p_find_stopped(struct wpa_supplicant *wpa_s) {}
static void wpas_hidl_notify_p2p_go_neg_req(
    struct wpa_supplicant *wpa_s, const u8 *src_addr, u16 dev_passwd_id,
    u8 go_intent)
{
}
static void wpas_hidl_notify_p2p_go_neg_completed(
    struct wpa_supplicant *wpa_s, const struct p2p_go_neg_results *res)
{
}
static void wpas_hidl_notify_p2p_group_formation_failure(
    struct wpa_supplicant *wpa_s, const char *reason)
{
}
static void wpas_hidl_notify_p2p_group_started(
    struct wpa_supplicant *wpa_s, const struct wpa_ssid *ssid, int persistent,
    int client)
{
}
static void wpas_hidl_notify_p2p_group_removed(
    struct wpa_supplicant *wpa_s, const struct wpa_ssid *ssid, const char *role)
{
}
static void wpas_hidl_notify_p2p_invitation_received(
    struct wpa_supplicant *wpa_s, const u8 *sa, const u8 *go_dev_addr,
    const u8 *bssid, int id, int op_freq)
{
}
static void wpas_hidl_notify_p2p_invitation_result(
    struct wpa_supplicant *wpa_s, int status, const u8 *bssid)
{
}
static void wpas_hidl_notify_p2p_provision_discovery(
    struct wpa_supplicant *wpa_s, const u8 *dev_addr, int request,
    enum p2p_prov_disc_status status, u16 config_methods,
    unsigned int generated_pin)
{
}
static void wpas_hidl_notify_p2p_sd_response(
    struct wpa_supplicant *wpa_s, const u8 *sa, u16 update_indic,
    const u8 *tlvs, size_t tlvs_len)
{
}
static void wpas_hidl_notify_ap_sta_authorized(
    struct wpa_supplicant *wpa_s, const u8 *sta, const u8 *p2p_dev_addr)
{
}
static void wpas_hidl_notify_ap_sta_deauthorized(
    struct wpa_supplicant *wpa_s, const u8 *sta, const u8 *p2p_dev_addr)
{
}
#endif  // CONFIG_CTRL_IFACE_HIDL

#ifdef _cplusplus
}
#endif  // _cplusplus

#endif  // WPA_SUPPLICANT_HIDL_HIDL_H
