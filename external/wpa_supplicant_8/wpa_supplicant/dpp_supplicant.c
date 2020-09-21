/*
 * wpa_supplicant - DPP
 * Copyright (c) 2017, Qualcomm Atheros, Inc.
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include "utils/includes.h"

#include "utils/common.h"
#include "utils/eloop.h"
#include "common/dpp.h"
#include "common/gas.h"
#include "common/gas_server.h"
#include "rsn_supp/wpa.h"
#include "rsn_supp/pmksa_cache.h"
#include "wpa_supplicant_i.h"
#include "config.h"
#include "driver_i.h"
#include "offchannel.h"
#include "gas_query.h"
#include "bss.h"
#include "scan.h"
#include "notify.h"
#include "dpp_supplicant.h"


static int wpas_dpp_listen_start(struct wpa_supplicant *wpa_s,
				 unsigned int freq);
static void wpas_dpp_reply_wait_timeout(void *eloop_ctx, void *timeout_ctx);
static void wpas_dpp_auth_success(struct wpa_supplicant *wpa_s, int initiator);
static void wpas_dpp_tx_status(struct wpa_supplicant *wpa_s,
			       unsigned int freq, const u8 *dst,
			       const u8 *src, const u8 *bssid,
			       const u8 *data, size_t data_len,
			       enum offchannel_send_action_result result);

static const u8 broadcast[ETH_ALEN] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

/* Use a hardcoded Transaction ID 1 in Peer Discovery frames since there is only
 * a single transaction in progress at any point in time. */
static const u8 TRANSACTION_ID = 1;


static struct dpp_configurator *
dpp_configurator_get_id(struct wpa_supplicant *wpa_s, unsigned int id)
{
	struct dpp_configurator *conf;

	dl_list_for_each(conf, &wpa_s->dpp_configurator,
			 struct dpp_configurator, list) {
		if (conf->id == id)
			return conf;
	}
	return NULL;
}


static unsigned int wpas_dpp_next_id(struct wpa_supplicant *wpa_s)
{
	struct dpp_bootstrap_info *bi;
	unsigned int max_id = 0;

	dl_list_for_each(bi, &wpa_s->dpp_bootstrap, struct dpp_bootstrap_info,
			 list) {
		if (bi->id > max_id)
			max_id = bi->id;
	}
	return max_id + 1;
}


/**
 * wpas_dpp_qr_code - Parse and add DPP bootstrapping info from a QR Code
 * @wpa_s: Pointer to wpa_supplicant data
 * @cmd: DPP URI read from a QR Code
 * Returns: Identifier of the stored info or -1 on failure
 */
int wpas_dpp_qr_code(struct wpa_supplicant *wpa_s, const char *cmd)
{
	struct dpp_bootstrap_info *bi;
	struct dpp_authentication *auth = wpa_s->dpp_auth;

	bi = dpp_parse_qr_code(cmd);
	if (!bi)
		return -1;

	bi->id = wpas_dpp_next_id(wpa_s);
	dl_list_add(&wpa_s->dpp_bootstrap, &bi->list);

	if (auth && auth->response_pending &&
	    dpp_notify_new_qr_code(auth, bi) == 1) {
		wpa_printf(MSG_DEBUG,
			   "DPP: Sending out pending authentication response");
		offchannel_send_action(wpa_s, auth->curr_freq,
				       auth->peer_mac_addr, wpa_s->own_addr,
				       broadcast,
				       wpabuf_head(auth->resp_msg),
				       wpabuf_len(auth->resp_msg),
				       500, wpas_dpp_tx_status, 0);
	}

	return bi->id;
}


static char * get_param(const char *cmd, const char *param)
{
	const char *pos, *end;
	char *val;
	size_t len;

	pos = os_strstr(cmd, param);
	if (!pos)
		return NULL;

	pos += os_strlen(param);
	end = os_strchr(pos, ' ');
	if (end)
		len = end - pos;
	else
		len = os_strlen(pos);
	val = os_malloc(len + 1);
	if (!val)
		return NULL;
	os_memcpy(val, pos, len);
	val[len] = '\0';
	return val;
}


int wpas_dpp_bootstrap_gen(struct wpa_supplicant *wpa_s, const char *cmd)
{
	char *chan = NULL, *mac = NULL, *info = NULL, *pk = NULL, *curve = NULL;
	char *key = NULL;
	u8 *privkey = NULL;
	size_t privkey_len = 0;
	size_t len;
	int ret = -1;
	struct dpp_bootstrap_info *bi;

	bi = os_zalloc(sizeof(*bi));
	if (!bi)
		goto fail;

	if (os_strstr(cmd, "type=qrcode"))
		bi->type = DPP_BOOTSTRAP_QR_CODE;
	else if (os_strstr(cmd, "type=pkex"))
		bi->type = DPP_BOOTSTRAP_PKEX;
	else
		goto fail;

	chan = get_param(cmd, " chan=");
	mac = get_param(cmd, " mac=");
	info = get_param(cmd, " info=");
	curve = get_param(cmd, " curve=");
	key = get_param(cmd, " key=");

	if (key) {
		privkey_len = os_strlen(key) / 2;
		privkey = os_malloc(privkey_len);
		if (!privkey ||
		    hexstr2bin(key, privkey, privkey_len) < 0)
			goto fail;
	}

	pk = dpp_keygen(bi, curve, privkey, privkey_len);
	if (!pk)
		goto fail;

	len = 4; /* "DPP:" */
	if (chan) {
		if (dpp_parse_uri_chan_list(bi, chan) < 0)
			goto fail;
		len += 3 + os_strlen(chan); /* C:...; */
	}
	if (mac) {
		if (dpp_parse_uri_mac(bi, mac) < 0)
			goto fail;
		len += 3 + os_strlen(mac); /* M:...; */
	}
	if (info) {
		if (dpp_parse_uri_info(bi, info) < 0)
			goto fail;
		len += 3 + os_strlen(info); /* I:...; */
	}
	len += 4 + os_strlen(pk);
	bi->uri = os_malloc(len + 1);
	if (!bi->uri)
		goto fail;
	os_snprintf(bi->uri, len + 1, "DPP:%s%s%s%s%s%s%s%s%sK:%s;;",
		    chan ? "C:" : "", chan ? chan : "", chan ? ";" : "",
		    mac ? "M:" : "", mac ? mac : "", mac ? ";" : "",
		    info ? "I:" : "", info ? info : "", info ? ";" : "",
		    pk);
	bi->id = wpas_dpp_next_id(wpa_s);
	dl_list_add(&wpa_s->dpp_bootstrap, &bi->list);
	ret = bi->id;
	bi = NULL;
fail:
	os_free(curve);
	os_free(pk);
	os_free(chan);
	os_free(mac);
	os_free(info);
	str_clear_free(key);
	bin_clear_free(privkey, privkey_len);
	dpp_bootstrap_info_free(bi);
	return ret;
}


static struct dpp_bootstrap_info *
dpp_bootstrap_get_id(struct wpa_supplicant *wpa_s, unsigned int id)
{
	struct dpp_bootstrap_info *bi;

	dl_list_for_each(bi, &wpa_s->dpp_bootstrap, struct dpp_bootstrap_info,
			 list) {
		if (bi->id == id)
			return bi;
	}
	return NULL;
}


static int dpp_bootstrap_del(struct wpa_supplicant *wpa_s, unsigned int id)
{
	struct dpp_bootstrap_info *bi, *tmp;
	int found = 0;

	dl_list_for_each_safe(bi, tmp, &wpa_s->dpp_bootstrap,
			      struct dpp_bootstrap_info, list) {
		if (id && bi->id != id)
			continue;
		found = 1;
		dl_list_del(&bi->list);
		dpp_bootstrap_info_free(bi);
	}

	if (id == 0)
		return 0; /* flush succeeds regardless of entries found */
	return found ? 0 : -1;
}


int wpas_dpp_bootstrap_remove(struct wpa_supplicant *wpa_s, const char *id)
{
	unsigned int id_val;

	if (os_strcmp(id, "*") == 0) {
		id_val = 0;
	} else {
		id_val = atoi(id);
		if (id_val == 0)
			return -1;
	}

	return dpp_bootstrap_del(wpa_s, id_val);
}


const char * wpas_dpp_bootstrap_get_uri(struct wpa_supplicant *wpa_s,
					unsigned int id)
{
	struct dpp_bootstrap_info *bi;

	bi = dpp_bootstrap_get_id(wpa_s, id);
	if (!bi)
		return NULL;
	return bi->uri;
}


int wpas_dpp_bootstrap_info(struct wpa_supplicant *wpa_s, int id,
			    char *reply, int reply_size)
{
	struct dpp_bootstrap_info *bi;

	bi = dpp_bootstrap_get_id(wpa_s, id);
	if (!bi)
		return -1;
	return os_snprintf(reply, reply_size, "type=%s\n"
			   "mac_addr=" MACSTR "\n"
			   "info=%s\n"
			   "num_freq=%u\n"
			   "curve=%s\n",
			   dpp_bootstrap_type_txt(bi->type),
			   MAC2STR(bi->mac_addr),
			   bi->info ? bi->info : "",
			   bi->num_freq,
			   bi->curve->name);
}


static void wpas_dpp_tx_status(struct wpa_supplicant *wpa_s,
			       unsigned int freq, const u8 *dst,
			       const u8 *src, const u8 *bssid,
			       const u8 *data, size_t data_len,
			       enum offchannel_send_action_result result)
{
	wpa_printf(MSG_DEBUG, "DPP: TX status: freq=%u dst=" MACSTR
		   " result=%s",
		   freq, MAC2STR(dst),
		   result == OFFCHANNEL_SEND_ACTION_SUCCESS ? "SUCCESS" :
		   (result == OFFCHANNEL_SEND_ACTION_NO_ACK ? "no-ACK" :
		    "FAILED"));

	if (!wpa_s->dpp_auth) {
		wpa_printf(MSG_DEBUG,
			   "DPP: Ignore TX status since there is no ongoing authentication exchange");
		return;
	}

	if (wpa_s->dpp_auth->remove_on_tx_status) {
		wpa_printf(MSG_DEBUG,
			   "DPP: Terminate authentication exchange due to an earlier error");
		eloop_cancel_timeout(wpas_dpp_reply_wait_timeout, wpa_s, NULL);
		offchannel_send_action_done(wpa_s);
		dpp_auth_deinit(wpa_s->dpp_auth);
		wpa_s->dpp_auth = NULL;
		return;
	}

	if (wpa_s->dpp_auth_ok_on_ack)
		wpas_dpp_auth_success(wpa_s, 1);

	if (!is_broadcast_ether_addr(dst) &&
	    result != OFFCHANNEL_SEND_ACTION_SUCCESS) {
		wpa_printf(MSG_DEBUG,
			   "DPP: Unicast DPP Action frame was not ACKed");
		/* TODO: In case of DPP Authentication Request frame, move to
		 * the next channel immediately */
	}
}


static void wpas_dpp_reply_wait_timeout(void *eloop_ctx, void *timeout_ctx)
{
	struct wpa_supplicant *wpa_s = eloop_ctx;

	if (!wpa_s->dpp_auth)
		return;
	wpa_printf(MSG_DEBUG, "DPP: Continue reply wait on channel %u MHz",
		   wpa_s->dpp_auth->curr_freq);
	wpas_dpp_listen_start(wpa_s, wpa_s->dpp_auth->curr_freq);
}


static void wpas_dpp_set_testing_options(struct wpa_supplicant *wpa_s,
					 struct dpp_authentication *auth)
{
#ifdef CONFIG_TESTING_OPTIONS
	if (wpa_s->dpp_config_obj_override)
		auth->config_obj_override =
			os_strdup(wpa_s->dpp_config_obj_override);
	if (wpa_s->dpp_discovery_override)
		auth->discovery_override =
			os_strdup(wpa_s->dpp_discovery_override);
	if (wpa_s->dpp_groups_override)
		auth->groups_override =
			os_strdup(wpa_s->dpp_groups_override);
	auth->ignore_netaccesskey_mismatch =
		wpa_s->dpp_ignore_netaccesskey_mismatch;
#endif /* CONFIG_TESTING_OPTIONS */
}


static void wpas_dpp_set_configurator(struct wpa_supplicant *wpa_s,
				      struct dpp_authentication *auth,
				      const char *cmd)
{
	const char *pos, *end;
	struct dpp_configuration *conf_sta = NULL, *conf_ap = NULL;
	struct dpp_configurator *conf = NULL;
	u8 ssid[32] = { "test" };
	size_t ssid_len = 4;
	char pass[64] = { };
	size_t pass_len = 0;
	u8 psk[PMK_LEN];
	int psk_set = 0;

	if (!cmd)
		return;

	wpa_printf(MSG_DEBUG, "DPP: Set configurator parameters: %s", cmd);
	pos = os_strstr(cmd, " ssid=");
	if (pos) {
		pos += 6;
		end = os_strchr(pos, ' ');
		ssid_len = end ? (size_t) (end - pos) : os_strlen(pos);
		ssid_len /= 2;
		if (ssid_len > sizeof(ssid) ||
		    hexstr2bin(pos, ssid, ssid_len) < 0)
			goto fail;
	}

	pos = os_strstr(cmd, " pass=");
	if (pos) {
		pos += 6;
		end = os_strchr(pos, ' ');
		pass_len = end ? (size_t) (end - pos) : os_strlen(pos);
		pass_len /= 2;
		if (pass_len > sizeof(pass) - 1 || pass_len < 8 ||
		    hexstr2bin(pos, (u8 *) pass, pass_len) < 0)
			goto fail;
	}

	pos = os_strstr(cmd, " psk=");
	if (pos) {
		pos += 5;
		if (hexstr2bin(pos, psk, PMK_LEN) < 0)
			goto fail;
		psk_set = 1;
	}

	if (os_strstr(cmd, " conf=sta-")) {
		conf_sta = os_zalloc(sizeof(struct dpp_configuration));
		if (!conf_sta)
			goto fail;
		os_memcpy(conf_sta->ssid, ssid, ssid_len);
		conf_sta->ssid_len = ssid_len;
		if (os_strstr(cmd, " conf=sta-psk")) {
			conf_sta->dpp = 0;
			if (psk_set) {
				os_memcpy(conf_sta->psk, psk, PMK_LEN);
			} else {
				conf_sta->passphrase = os_strdup(pass);
				if (!conf_sta->passphrase)
					goto fail;
			}
		} else if (os_strstr(cmd, " conf=sta-dpp")) {
			conf_sta->dpp = 1;
		} else {
			goto fail;
		}
	}

	if (os_strstr(cmd, " conf=ap-")) {
		conf_ap = os_zalloc(sizeof(struct dpp_configuration));
		if (!conf_ap)
			goto fail;
		os_memcpy(conf_ap->ssid, ssid, ssid_len);
		conf_ap->ssid_len = ssid_len;
		if (os_strstr(cmd, " conf=ap-psk")) {
			conf_ap->dpp = 0;
			if (psk_set) {
				os_memcpy(conf_ap->psk, psk, PMK_LEN);
			} else {
				conf_ap->passphrase = os_strdup(pass);
				if (!conf_ap->passphrase)
					goto fail;
			}
		} else if (os_strstr(cmd, " conf=ap-dpp")) {
			conf_ap->dpp = 1;
		} else {
			goto fail;
		}
	}

	pos = os_strstr(cmd, " expiry=");
	if (pos) {
		long int val;

		pos += 8;
		val = strtol(pos, NULL, 0);
		if (val <= 0)
			goto fail;
		if (conf_sta)
			conf_sta->netaccesskey_expiry = val;
		if (conf_ap)
			conf_ap->netaccesskey_expiry = val;
	}

	pos = os_strstr(cmd, " configurator=");
	if (pos) {
		pos += 14;
		conf = dpp_configurator_get_id(wpa_s, atoi(pos));
		if (!conf) {
			wpa_printf(MSG_INFO,
				   "DPP: Could not find the specified configurator");
			goto fail;
		}
	}
	auth->conf_sta = conf_sta;
	auth->conf_ap = conf_ap;
	auth->conf = conf;
	return;

fail:
	wpa_printf(MSG_DEBUG, "DPP: Failed to set configurator parameters");
	dpp_configuration_free(conf_sta);
	dpp_configuration_free(conf_ap);
}


int wpas_dpp_auth_init(struct wpa_supplicant *wpa_s, const char *cmd)
{
	const char *pos;
	struct dpp_bootstrap_info *peer_bi, *own_bi = NULL;
	const u8 *dst;
	int res;
	int configurator = 1;
	unsigned int wait_time;

	wpa_s->dpp_gas_client = 0;

	pos = os_strstr(cmd, " peer=");
	if (!pos)
		return -1;
	pos += 6;
	peer_bi = dpp_bootstrap_get_id(wpa_s, atoi(pos));
	if (!peer_bi) {
		wpa_printf(MSG_INFO,
			   "DPP: Could not find bootstrapping info for the identified peer");
		return -1;
	}

	pos = os_strstr(cmd, " own=");
	if (pos) {
		pos += 5;
		own_bi = dpp_bootstrap_get_id(wpa_s, atoi(pos));
		if (!own_bi) {
			wpa_printf(MSG_INFO,
				   "DPP: Could not find bootstrapping info for the identified local entry");
			return -1;
		}

		if (peer_bi->curve != own_bi->curve) {
			wpa_printf(MSG_INFO,
				   "DPP: Mismatching curves in bootstrapping info (peer=%s own=%s)",
				   peer_bi->curve->name, own_bi->curve->name);
			return -1;
		}
	}

	pos = os_strstr(cmd, " role=");
	if (pos) {
		pos += 6;
		if (os_strncmp(pos, "configurator", 12) == 0)
			configurator = 1;
		else if (os_strncmp(pos, "enrollee", 8) == 0)
			configurator = 0;
		else
			goto fail;
	}

	pos = os_strstr(cmd, " netrole=");
	if (pos) {
		pos += 9;
		wpa_s->dpp_netrole_ap = os_strncmp(pos, "ap", 2) == 0;
	}

	if (wpa_s->dpp_auth) {
		eloop_cancel_timeout(wpas_dpp_reply_wait_timeout, wpa_s, NULL);
		offchannel_send_action_done(wpa_s);
		dpp_auth_deinit(wpa_s->dpp_auth);
	}
	wpa_s->dpp_auth = dpp_auth_init(wpa_s, peer_bi, own_bi, configurator);
	if (!wpa_s->dpp_auth)
		goto fail;
	wpas_dpp_set_testing_options(wpa_s, wpa_s->dpp_auth);
	wpas_dpp_set_configurator(wpa_s, wpa_s->dpp_auth, cmd);

	/* TODO: Support iteration over all frequencies and filtering of
	 * frequencies based on locally enabled channels that allow initiation
	 * of transmission. */
	if (peer_bi->num_freq > 0)
		wpa_s->dpp_auth->curr_freq = peer_bi->freq[0];
	else
		wpa_s->dpp_auth->curr_freq = 2412;

	if (is_zero_ether_addr(peer_bi->mac_addr)) {
		dst = broadcast;
	} else {
		dst = peer_bi->mac_addr;
		os_memcpy(wpa_s->dpp_auth->peer_mac_addr, peer_bi->mac_addr,
			  ETH_ALEN);
	}
	wpa_s->dpp_auth_ok_on_ack = 0;
	eloop_cancel_timeout(wpas_dpp_reply_wait_timeout, wpa_s, NULL);
	wait_time = wpa_s->max_remain_on_chan;
	if (wait_time > 2000)
		wait_time = 2000;
	eloop_register_timeout(wait_time / 1000, (wait_time % 1000) * 1000,
			       wpas_dpp_reply_wait_timeout,
			       wpa_s, NULL);
	res = offchannel_send_action(wpa_s, wpa_s->dpp_auth->curr_freq,
				     dst, wpa_s->own_addr, broadcast,
				     wpabuf_head(wpa_s->dpp_auth->req_msg),
				     wpabuf_len(wpa_s->dpp_auth->req_msg),
				     wait_time, wpas_dpp_tx_status, 0);

	return res;
fail:
	return -1;
}


struct wpas_dpp_listen_work {
	unsigned int freq;
	unsigned int duration;
	struct wpabuf *probe_resp_ie;
};


static void wpas_dpp_listen_work_free(struct wpas_dpp_listen_work *lwork)
{
	if (!lwork)
		return;
	os_free(lwork);
}


static void wpas_dpp_listen_work_done(struct wpa_supplicant *wpa_s)
{
	struct wpas_dpp_listen_work *lwork;

	if (!wpa_s->dpp_listen_work)
		return;

	lwork = wpa_s->dpp_listen_work->ctx;
	wpas_dpp_listen_work_free(lwork);
	radio_work_done(wpa_s->dpp_listen_work);
	wpa_s->dpp_listen_work = NULL;
}


static void dpp_start_listen_cb(struct wpa_radio_work *work, int deinit)
{
	struct wpa_supplicant *wpa_s = work->wpa_s;
	struct wpas_dpp_listen_work *lwork = work->ctx;

	if (deinit) {
		if (work->started) {
			wpa_s->dpp_listen_work = NULL;
			wpas_dpp_listen_stop(wpa_s);
		}
		wpas_dpp_listen_work_free(lwork);
		return;
	}

	wpa_s->dpp_listen_work = work;

	wpa_s->dpp_pending_listen_freq = lwork->freq;

	if (wpa_drv_remain_on_channel(wpa_s, lwork->freq,
				      wpa_s->max_remain_on_chan) < 0) {
		wpa_printf(MSG_DEBUG,
			   "DPP: Failed to request the driver to remain on channel (%u MHz) for listen",
			   lwork->freq);
		wpas_dpp_listen_work_done(wpa_s);
		wpa_s->dpp_pending_listen_freq = 0;
		return;
	}
	wpa_s->off_channel_freq = 0;
	wpa_s->roc_waiting_drv_freq = lwork->freq;
}


static int wpas_dpp_listen_start(struct wpa_supplicant *wpa_s,
				 unsigned int freq)
{
	struct wpas_dpp_listen_work *lwork;

	if (wpa_s->dpp_listen_work) {
		wpa_printf(MSG_DEBUG,
			   "DPP: Reject start_listen since dpp_listen_work already exists");
		return -1;
	}

	if (wpa_s->dpp_listen_freq)
		wpas_dpp_listen_stop(wpa_s);
	wpa_s->dpp_listen_freq = freq;

	lwork = os_zalloc(sizeof(*lwork));
	if (!lwork)
		return -1;
	lwork->freq = freq;

	if (radio_add_work(wpa_s, freq, "dpp-listen", 0, dpp_start_listen_cb,
			   lwork) < 0) {
		wpas_dpp_listen_work_free(lwork);
		return -1;
	}

	return 0;
}


int wpas_dpp_listen(struct wpa_supplicant *wpa_s, const char *cmd)
{
	int freq;

	freq = atoi(cmd);
	if (freq <= 0)
		return -1;

	if (os_strstr(cmd, " role=configurator"))
		wpa_s->dpp_allowed_roles = DPP_CAPAB_CONFIGURATOR;
	else if (os_strstr(cmd, " role=enrollee"))
		wpa_s->dpp_allowed_roles = DPP_CAPAB_ENROLLEE;
	else
		wpa_s->dpp_allowed_roles = DPP_CAPAB_CONFIGURATOR |
			DPP_CAPAB_ENROLLEE;
	wpa_s->dpp_qr_mutual = os_strstr(cmd, " qr=mutual") != NULL;
	wpa_s->dpp_netrole_ap = os_strstr(cmd, " netrole=ap") != NULL;
	if (wpa_s->dpp_listen_freq == (unsigned int) freq) {
		wpa_printf(MSG_DEBUG, "DPP: Already listening on %u MHz",
			   freq);
		return 0;
	}

	return wpas_dpp_listen_start(wpa_s, freq);
}


void wpas_dpp_listen_stop(struct wpa_supplicant *wpa_s)
{
	if (!wpa_s->dpp_listen_freq)
		return;

	wpa_printf(MSG_DEBUG, "DPP: Stop listen on %u MHz",
		   wpa_s->dpp_listen_freq);
	wpa_drv_cancel_remain_on_channel(wpa_s);
	wpa_s->dpp_listen_freq = 0;
	wpas_dpp_listen_work_done(wpa_s);
}


void wpas_dpp_remain_on_channel_cb(struct wpa_supplicant *wpa_s,
				   unsigned int freq)
{
	if (!wpa_s->dpp_listen_freq && !wpa_s->dpp_pending_listen_freq)
		return;

	wpa_printf(MSG_DEBUG,
		   "DPP: remain-on-channel callback (off_channel_freq=%u dpp_pending_listen_freq=%d roc_waiting_drv_freq=%d freq=%u)",
		   wpa_s->off_channel_freq, wpa_s->dpp_pending_listen_freq,
		   wpa_s->roc_waiting_drv_freq, freq);
	if (wpa_s->off_channel_freq &&
	    wpa_s->off_channel_freq == wpa_s->dpp_pending_listen_freq) {
		wpa_printf(MSG_DEBUG, "DPP: Listen on %u MHz started", freq);
		wpa_s->dpp_pending_listen_freq = 0;
	} else {
		wpa_printf(MSG_DEBUG,
			   "DPP: Ignore remain-on-channel callback (off_channel_freq=%u dpp_pending_listen_freq=%d freq=%u)",
			   wpa_s->off_channel_freq,
			   wpa_s->dpp_pending_listen_freq, freq);
	}
}


void wpas_dpp_cancel_remain_on_channel_cb(struct wpa_supplicant *wpa_s,
					  unsigned int freq)
{
	wpas_dpp_listen_work_done(wpa_s);

	if (wpa_s->dpp_auth && !wpa_s->dpp_gas_client) {
		/* Continue listen with a new remain-on-channel */
		wpa_printf(MSG_DEBUG,
			   "DPP: Continue wait on %u MHz for the ongoing DPP provisioning session",
			   wpa_s->dpp_auth->curr_freq);
		wpas_dpp_listen_start(wpa_s, wpa_s->dpp_auth->curr_freq);
		return;
	}

	if (wpa_s->dpp_listen_freq) {
		/* Continue listen with a new remain-on-channel */
		wpas_dpp_listen_start(wpa_s, wpa_s->dpp_listen_freq);
	}
}


static void wpas_dpp_rx_auth_req(struct wpa_supplicant *wpa_s, const u8 *src,
				 const u8 *hdr, const u8 *buf, size_t len,
				 unsigned int freq)
{
	const u8 *r_bootstrap, *i_bootstrap, *wrapped_data;
	u16 r_bootstrap_len, i_bootstrap_len, wrapped_data_len;
	struct dpp_bootstrap_info *bi, *own_bi = NULL, *peer_bi = NULL;

	wpa_printf(MSG_DEBUG, "DPP: Authentication Request from " MACSTR,
		   MAC2STR(src));

	wrapped_data = dpp_get_attr(buf, len, DPP_ATTR_WRAPPED_DATA,
				    &wrapped_data_len);
	if (!wrapped_data) {
		wpa_printf(MSG_DEBUG,
			   "DPP: Missing required Wrapped data attribute");
		return;
	}
	wpa_hexdump(MSG_MSGDUMP, "DPP: Wrapped data",
		    wrapped_data, wrapped_data_len);

	r_bootstrap = dpp_get_attr(buf, len, DPP_ATTR_R_BOOTSTRAP_KEY_HASH,
				   &r_bootstrap_len);
	if (!r_bootstrap || r_bootstrap > wrapped_data ||
	    r_bootstrap_len != SHA256_MAC_LEN) {
		wpa_printf(MSG_DEBUG,
			   "DPP: Missing or invalid required Responder Bootstrapping Key Hash attribute");
		return;
	}
	wpa_hexdump(MSG_MSGDUMP, "DPP: Responder Bootstrapping Key Hash",
		    r_bootstrap, r_bootstrap_len);

	i_bootstrap = dpp_get_attr(buf, len, DPP_ATTR_I_BOOTSTRAP_KEY_HASH,
				   &i_bootstrap_len);
	if (!i_bootstrap || i_bootstrap > wrapped_data ||
	    i_bootstrap_len != SHA256_MAC_LEN) {
		wpa_printf(MSG_DEBUG,
			   "DPP: Missing or invalid required Initiator Bootstrapping Key Hash attribute");
		return;
	}
	wpa_hexdump(MSG_MSGDUMP, "DPP: Initiator Bootstrapping Key Hash",
		    i_bootstrap, i_bootstrap_len);

	/* Try to find own and peer bootstrapping key matches based on the
	 * received hash values */
	dl_list_for_each(bi, &wpa_s->dpp_bootstrap, struct dpp_bootstrap_info,
			 list) {
		if (!own_bi && bi->own &&
		    os_memcmp(bi->pubkey_hash, r_bootstrap,
			      SHA256_MAC_LEN) == 0) {
			wpa_printf(MSG_DEBUG,
				   "DPP: Found matching own bootstrapping information");
			own_bi = bi;
		}

		if (!peer_bi && !bi->own &&
		    os_memcmp(bi->pubkey_hash, i_bootstrap,
			      SHA256_MAC_LEN) == 0) {
			wpa_printf(MSG_DEBUG,
				   "DPP: Found matching peer bootstrapping information");
			peer_bi = bi;
		}

		if (own_bi && peer_bi)
			break;
	}

	if (!own_bi) {
		wpa_printf(MSG_DEBUG,
			   "DPP: No matching own bootstrapping key found - ignore message");
		return;
	}

	if (wpa_s->dpp_auth) {
		wpa_printf(MSG_DEBUG,
			   "DPP: Already in DPP authentication exchange - ignore new one");
		return;
	}

	wpa_s->dpp_gas_client = 0;
	wpa_s->dpp_auth_ok_on_ack = 0;
	wpa_s->dpp_auth = dpp_auth_req_rx(wpa_s, wpa_s->dpp_allowed_roles,
					  wpa_s->dpp_qr_mutual,
					  peer_bi, own_bi, freq, hdr, buf,
					  wrapped_data, wrapped_data_len);
	if (!wpa_s->dpp_auth) {
		wpa_printf(MSG_DEBUG, "DPP: No response generated");
		return;
	}
	wpas_dpp_set_testing_options(wpa_s, wpa_s->dpp_auth);
	wpas_dpp_set_configurator(wpa_s, wpa_s->dpp_auth,
				  wpa_s->dpp_configurator_params);
	os_memcpy(wpa_s->dpp_auth->peer_mac_addr, src, ETH_ALEN);

	offchannel_send_action(wpa_s, wpa_s->dpp_auth->curr_freq,
			       src, wpa_s->own_addr, broadcast,
			       wpabuf_head(wpa_s->dpp_auth->resp_msg),
			       wpabuf_len(wpa_s->dpp_auth->resp_msg),
			       500, wpas_dpp_tx_status, 0);
}


static void wpas_dpp_start_gas_server(struct wpa_supplicant *wpa_s)
{
	/* TODO: stop wait and start ROC */
}


static struct wpa_ssid * wpas_dpp_add_network(struct wpa_supplicant *wpa_s,
					      struct dpp_authentication *auth)
{
	struct wpa_ssid *ssid;

	ssid = wpa_config_add_network(wpa_s->conf);
	if (!ssid)
		return NULL;
	wpas_notify_network_added(wpa_s, ssid);
	wpa_config_set_network_defaults(ssid);
	ssid->disabled = 1;

	ssid->ssid = os_malloc(auth->ssid_len);
	if (!ssid->ssid)
		goto fail;
	os_memcpy(ssid->ssid, auth->ssid, auth->ssid_len);
	ssid->ssid_len = auth->ssid_len;

	if (auth->connector) {
		ssid->key_mgmt = WPA_KEY_MGMT_DPP;
		ssid->dpp_connector = os_strdup(auth->connector);
		if (!ssid->dpp_connector)
			goto fail;
	}

	if (auth->c_sign_key) {
		ssid->dpp_csign = os_malloc(wpabuf_len(auth->c_sign_key));
		if (!ssid->dpp_csign)
			goto fail;
		os_memcpy(ssid->dpp_csign, wpabuf_head(auth->c_sign_key),
			  wpabuf_len(auth->c_sign_key));
		ssid->dpp_csign_len = wpabuf_len(auth->c_sign_key);
	}

	if (auth->net_access_key) {
		ssid->dpp_netaccesskey =
			os_malloc(wpabuf_len(auth->net_access_key));
		if (!ssid->dpp_netaccesskey)
			goto fail;
		os_memcpy(ssid->dpp_netaccesskey,
			  wpabuf_head(auth->net_access_key),
			  wpabuf_len(auth->net_access_key));
		ssid->dpp_netaccesskey_len = wpabuf_len(auth->net_access_key);
		ssid->dpp_netaccesskey_expiry = auth->net_access_key_expiry;
	}

	if (!auth->connector) {
		ssid->key_mgmt = WPA_KEY_MGMT_PSK;
		if (auth->passphrase[0]) {
			if (wpa_config_set_quoted(ssid, "psk",
						  auth->passphrase) < 0)
				goto fail;
			wpa_config_update_psk(ssid);
			ssid->export_keys = 1;
		} else {
			ssid->psk_set = auth->psk_set;
			os_memcpy(ssid->psk, auth->psk, PMK_LEN);
		}
	}

	return ssid;
fail:
	wpas_notify_network_removed(wpa_s, ssid);
	wpa_config_remove_network(wpa_s->conf, ssid->id);
	return NULL;
}


static void wpas_dpp_process_config(struct wpa_supplicant *wpa_s,
				    struct dpp_authentication *auth)
{
	struct wpa_ssid *ssid;

	if (wpa_s->conf->dpp_config_processing < 1)
		return;

	ssid = wpas_dpp_add_network(wpa_s, auth);
	if (!ssid)
		return;

	wpa_msg(wpa_s, MSG_INFO, DPP_EVENT_NETWORK_ID "%d", ssid->id);
	if (wpa_s->conf->dpp_config_processing < 2)
		return;

	wpa_printf(MSG_DEBUG, "DPP: Trying to connect to the new network");
	ssid->disabled = 0;
	wpa_s->disconnected = 0;
	wpa_s->reassociate = 1;
	wpa_s->scan_runs = 0;
	wpa_s->normal_scans = 0;
	wpa_supplicant_cancel_sched_scan(wpa_s);
	wpa_supplicant_req_scan(wpa_s, 0, 0);
}


static void wpas_dpp_handle_config_obj(struct wpa_supplicant *wpa_s,
				       struct dpp_authentication *auth)
{
	wpa_msg(wpa_s, MSG_INFO, DPP_EVENT_CONF_RECEIVED);
	if (auth->ssid_len)
		wpa_msg(wpa_s, MSG_INFO, DPP_EVENT_CONFOBJ_SSID "%s",
			wpa_ssid_txt(auth->ssid, auth->ssid_len));
	if (auth->connector) {
		/* TODO: Save the Connector and consider using a command
		 * to fetch the value instead of sending an event with
		 * it. The Connector could end up being larger than what
		 * most clients are ready to receive as an event
		 * message. */
		wpa_msg(wpa_s, MSG_INFO, DPP_EVENT_CONNECTOR "%s",
			auth->connector);
	}
	if (auth->c_sign_key) {
		char *hex;
		size_t hexlen;

		hexlen = 2 * wpabuf_len(auth->c_sign_key) + 1;
		hex = os_malloc(hexlen);
		if (hex) {
			wpa_snprintf_hex(hex, hexlen,
					 wpabuf_head(auth->c_sign_key),
					 wpabuf_len(auth->c_sign_key));
			wpa_msg(wpa_s, MSG_INFO, DPP_EVENT_C_SIGN_KEY "%s",
				hex);
			os_free(hex);
		}
	}
	if (auth->net_access_key) {
		char *hex;
		size_t hexlen;

		hexlen = 2 * wpabuf_len(auth->net_access_key) + 1;
		hex = os_malloc(hexlen);
		if (hex) {
			wpa_snprintf_hex(hex, hexlen,
					 wpabuf_head(auth->net_access_key),
					 wpabuf_len(auth->net_access_key));
			if (auth->net_access_key_expiry)
				wpa_msg(wpa_s, MSG_INFO,
					DPP_EVENT_NET_ACCESS_KEY "%s %lu", hex,
					(long unsigned)
					auth->net_access_key_expiry);
			else
				wpa_msg(wpa_s, MSG_INFO,
					DPP_EVENT_NET_ACCESS_KEY "%s", hex);
			os_free(hex);
		}
	}

	wpas_dpp_process_config(wpa_s, auth);
}


static void wpas_dpp_gas_resp_cb(void *ctx, const u8 *addr, u8 dialog_token,
				 enum gas_query_result result,
				 const struct wpabuf *adv_proto,
				 const struct wpabuf *resp, u16 status_code)
{
	struct wpa_supplicant *wpa_s = ctx;
	const u8 *pos;
	struct dpp_authentication *auth = wpa_s->dpp_auth;

	if (!auth || !auth->auth_success) {
		wpa_printf(MSG_DEBUG, "DPP: No matching exchange in progress");
		return;
	}
	if (!resp || status_code != WLAN_STATUS_SUCCESS) {
		wpa_printf(MSG_DEBUG, "DPP: GAS query did not succeed");
		goto fail;
	}

	wpa_hexdump_buf(MSG_DEBUG, "DPP: Configuration Response adv_proto",
			adv_proto);
	wpa_hexdump_buf(MSG_DEBUG, "DPP: Configuration Response (GAS response)",
			resp);

	if (wpabuf_len(adv_proto) != 10 ||
	    !(pos = wpabuf_head(adv_proto)) ||
	    pos[0] != WLAN_EID_ADV_PROTO ||
	    pos[1] != 8 ||
	    pos[3] != WLAN_EID_VENDOR_SPECIFIC ||
	    pos[4] != 5 ||
	    WPA_GET_BE24(&pos[5]) != OUI_WFA ||
	    pos[8] != 0x1a ||
	    pos[9] != 1) {
		wpa_printf(MSG_DEBUG,
			   "DPP: Not a DPP Advertisement Protocol ID");
		goto fail;
	}

	if (dpp_conf_resp_rx(auth, resp) < 0) {
		wpa_printf(MSG_DEBUG, "DPP: Configuration attempt failed");
		goto fail;
	}

	wpas_dpp_handle_config_obj(wpa_s, auth);
	dpp_auth_deinit(wpa_s->dpp_auth);
	wpa_s->dpp_auth = NULL;
	return;

fail:
	wpa_msg(wpa_s, MSG_INFO, DPP_EVENT_CONF_FAILED);
	dpp_auth_deinit(wpa_s->dpp_auth);
	wpa_s->dpp_auth = NULL;
}


static void wpas_dpp_start_gas_client(struct wpa_supplicant *wpa_s)
{
	struct dpp_authentication *auth = wpa_s->dpp_auth;
	struct wpabuf *buf, *conf_req;
	char json[100];
	int res;

	wpa_s->dpp_gas_client = 1;
	os_snprintf(json, sizeof(json),
		    "{\"name\":\"Test\","
		    "\"wi-fi_tech\":\"infra\","
		    "\"netRole\":\"%s\"}",
		    wpa_s->dpp_netrole_ap ? "ap" : "sta");
	wpa_printf(MSG_DEBUG, "DPP: GAS Config Attributes: %s", json);

	offchannel_send_action_done(wpa_s);
	wpas_dpp_listen_stop(wpa_s);

	conf_req = dpp_build_conf_req(auth, json);
	if (!conf_req) {
		wpa_printf(MSG_DEBUG,
			   "DPP: No configuration request data available");
		return;
	}

	buf = gas_build_initial_req(0, 10 + 2 + wpabuf_len(conf_req));
	if (!buf) {
		wpabuf_free(conf_req);
		return;
	}

	/* Advertisement Protocol IE */
	wpabuf_put_u8(buf, WLAN_EID_ADV_PROTO);
	wpabuf_put_u8(buf, 8); /* Length */
	wpabuf_put_u8(buf, 0x7f);
	wpabuf_put_u8(buf, WLAN_EID_VENDOR_SPECIFIC);
	wpabuf_put_u8(buf, 5);
	wpabuf_put_be24(buf, OUI_WFA);
	wpabuf_put_u8(buf, DPP_OUI_TYPE);
	wpabuf_put_u8(buf, 0x01);

	/* GAS Query */
	wpabuf_put_le16(buf, wpabuf_len(conf_req));
	wpabuf_put_buf(buf, conf_req);
	wpabuf_free(conf_req);

	wpa_printf(MSG_DEBUG, "DPP: GAS request to " MACSTR " (freq %u MHz)",
		   MAC2STR(auth->peer_mac_addr), auth->curr_freq);

	res = gas_query_req(wpa_s->gas, auth->peer_mac_addr, auth->curr_freq,
			    buf, wpas_dpp_gas_resp_cb, wpa_s);
	if (res < 0) {
		wpa_msg(wpa_s, MSG_DEBUG, "GAS: Failed to send Query Request");
		wpabuf_free(buf);
	} else {
		wpa_printf(MSG_DEBUG,
			   "DPP: GAS query started with dialog token %u", res);
	}
}


static void wpas_dpp_auth_success(struct wpa_supplicant *wpa_s, int initiator)
{
	wpa_printf(MSG_DEBUG, "DPP: Authentication succeeded");
	wpa_msg(wpa_s, MSG_INFO, DPP_EVENT_AUTH_SUCCESS "init=%d", initiator);

	if (wpa_s->dpp_auth->configurator)
		wpas_dpp_start_gas_server(wpa_s);
	else
		wpas_dpp_start_gas_client(wpa_s);
}


static void wpas_dpp_rx_auth_resp(struct wpa_supplicant *wpa_s, const u8 *src,
				  const u8 *hdr, const u8 *buf, size_t len)
{
	struct dpp_authentication *auth = wpa_s->dpp_auth;
	struct wpabuf *msg;

	wpa_printf(MSG_DEBUG, "DPP: Authentication Response from " MACSTR,
		   MAC2STR(src));

	if (!auth) {
		wpa_printf(MSG_DEBUG,
			   "DPP: No DPP Authentication in progress - drop");
		return;
	}

	if (!is_zero_ether_addr(auth->peer_mac_addr) &&
	    os_memcmp(src, auth->peer_mac_addr, ETH_ALEN) != 0) {
		wpa_printf(MSG_DEBUG, "DPP: MAC address mismatch (expected "
			   MACSTR ") - drop", MAC2STR(auth->peer_mac_addr));
		return;
	}

	eloop_cancel_timeout(wpas_dpp_reply_wait_timeout, wpa_s, NULL);

	msg = dpp_auth_resp_rx(auth, hdr, buf, len);
	if (!msg) {
		if (auth->auth_resp_status == DPP_STATUS_RESPONSE_PENDING) {
			wpa_printf(MSG_DEBUG,
				   "DPP: Start wait for full response");
			offchannel_send_action_done(wpa_s);
			wpas_dpp_listen_start(wpa_s, auth->curr_freq);
			return;
		}
		wpa_printf(MSG_DEBUG, "DPP: No confirm generated");
		return;
	}
	os_memcpy(auth->peer_mac_addr, src, ETH_ALEN);

	offchannel_send_action(wpa_s, auth->curr_freq,
			       src, wpa_s->own_addr, broadcast,
			       wpabuf_head(msg), wpabuf_len(msg),
			       500, wpas_dpp_tx_status, 0);
	wpabuf_free(msg);
	wpa_s->dpp_auth_ok_on_ack = 1;
}


static void wpas_dpp_rx_auth_conf(struct wpa_supplicant *wpa_s, const u8 *src,
				  const u8 *hdr, const u8 *buf, size_t len)
{
	struct dpp_authentication *auth = wpa_s->dpp_auth;

	wpa_printf(MSG_DEBUG, "DPP: Authentication Confirmation from " MACSTR,
		   MAC2STR(src));

	if (!auth) {
		wpa_printf(MSG_DEBUG,
			   "DPP: No DPP Authentication in progress - drop");
		return;
	}

	if (os_memcmp(src, auth->peer_mac_addr, ETH_ALEN) != 0) {
		wpa_printf(MSG_DEBUG, "DPP: MAC address mismatch (expected "
			   MACSTR ") - drop", MAC2STR(auth->peer_mac_addr));
		return;
	}

	if (dpp_auth_conf_rx(auth, hdr, buf, len) < 0) {
		wpa_printf(MSG_DEBUG, "DPP: Authentication failed");
		return;
	}

	wpas_dpp_auth_success(wpa_s, 0);
}


static void wpas_dpp_rx_peer_disc_resp(struct wpa_supplicant *wpa_s,
				       const u8 *src,
				       const u8 *buf, size_t len)
{
	struct wpa_ssid *ssid;
	const u8 *connector, *trans_id;
	u16 connector_len, trans_id_len;
	struct dpp_introduction intro;
	struct rsn_pmksa_cache_entry *entry;
	struct os_time now;
	struct os_reltime rnow;
	os_time_t expiry;
	unsigned int seconds;

	wpa_printf(MSG_DEBUG, "DPP: Peer Discovery Response from " MACSTR,
		   MAC2STR(src));
	if (is_zero_ether_addr(wpa_s->dpp_intro_bssid) ||
	    os_memcmp(src, wpa_s->dpp_intro_bssid, ETH_ALEN) != 0) {
		wpa_printf(MSG_DEBUG, "DPP: Not waiting for response from "
			   MACSTR " - drop", MAC2STR(src));
		return;
	}
	offchannel_send_action_done(wpa_s);

	for (ssid = wpa_s->conf->ssid; ssid; ssid = ssid->next) {
		if (ssid == wpa_s->dpp_intro_network)
			break;
	}
	if (!ssid || !ssid->dpp_connector || !ssid->dpp_netaccesskey ||
	    !ssid->dpp_csign) {
		wpa_printf(MSG_DEBUG,
			   "DPP: Profile not found for network introduction");
		return;
	}

	trans_id = dpp_get_attr(buf, len, DPP_ATTR_TRANSACTION_ID,
			       &trans_id_len);
	if (!trans_id || trans_id_len != 1) {
		wpa_printf(MSG_DEBUG,
			   "DPP: Peer did not include Transaction ID");
		goto fail;
	}
	if (trans_id[0] != TRANSACTION_ID) {
		wpa_printf(MSG_DEBUG,
			   "DPP: Ignore frame with unexpected Transaction ID %u",
			   trans_id[0]);
		goto fail;
	}

	connector = dpp_get_attr(buf, len, DPP_ATTR_CONNECTOR, &connector_len);
	if (!connector) {
		wpa_printf(MSG_DEBUG,
			   "DPP: Peer did not include its Connector");
		return;
	}

	if (dpp_peer_intro(&intro, ssid->dpp_connector,
			   ssid->dpp_netaccesskey,
			   ssid->dpp_netaccesskey_len,
			   ssid->dpp_csign,
			   ssid->dpp_csign_len,
			   connector, connector_len, &expiry) < 0) {
		wpa_printf(MSG_INFO,
			   "DPP: Network Introduction protocol resulted in failure");
		goto fail;
	}

	entry = os_zalloc(sizeof(*entry));
	if (!entry)
		goto fail;
	os_memcpy(entry->aa, src, ETH_ALEN);
	os_memcpy(entry->pmkid, intro.pmkid, PMKID_LEN);
	os_memcpy(entry->pmk, intro.pmk, intro.pmk_len);
	entry->pmk_len = intro.pmk_len;
	entry->akmp = WPA_KEY_MGMT_DPP;
	if (expiry) {
		os_get_time(&now);
		seconds = expiry - now.sec;
	} else {
		seconds = 86400 * 7;
	}
	os_get_reltime(&rnow);
	entry->expiration = rnow.sec + seconds;
	entry->reauth_time = rnow.sec + seconds;
	entry->network_ctx = ssid;
	wpa_sm_pmksa_cache_add_entry(wpa_s->wpa, entry);

	wpa_printf(MSG_DEBUG,
		   "DPP: Try connection again after successful network introduction");
	if (wpa_supplicant_fast_associate(wpa_s) != 1) {
		wpa_supplicant_cancel_sched_scan(wpa_s);
		wpa_supplicant_req_scan(wpa_s, 0, 0);
	}
fail:
	os_memset(&intro, 0, sizeof(intro));
}


static void
wpas_dpp_tx_pkex_status(struct wpa_supplicant *wpa_s,
			unsigned int freq, const u8 *dst,
			const u8 *src, const u8 *bssid,
			const u8 *data, size_t data_len,
			enum offchannel_send_action_result result)
{
	wpa_printf(MSG_DEBUG, "DPP: TX status: freq=%u dst=" MACSTR
		   " result=%s (PKEX)",
		   freq, MAC2STR(dst),
		   result == OFFCHANNEL_SEND_ACTION_SUCCESS ? "SUCCESS" :
		   (result == OFFCHANNEL_SEND_ACTION_NO_ACK ? "no-ACK" :
		    "FAILED"));
	/* TODO: Time out wait for response more quickly in error cases? */
}


static void
wpas_dpp_rx_pkex_exchange_req(struct wpa_supplicant *wpa_s, const u8 *src,
			      const u8 *buf, size_t len, unsigned int freq)
{
	struct wpabuf *msg;
	unsigned int wait_time;

	wpa_printf(MSG_DEBUG, "DPP: PKEX Exchange Request from " MACSTR,
		   MAC2STR(src));

	/* TODO: Support multiple PKEX codes by iterating over all the enabled
	 * values here */

	if (!wpa_s->dpp_pkex_code || !wpa_s->dpp_pkex_bi) {
		wpa_printf(MSG_DEBUG,
			   "DPP: No PKEX code configured - ignore request");
		return;
	}

	if (wpa_s->dpp_pkex) {
		/* TODO: Support parallel operations */
		wpa_printf(MSG_DEBUG,
			   "DPP: Already in PKEX session - ignore new request");
		return;
	}

	wpa_s->dpp_pkex = dpp_pkex_rx_exchange_req(wpa_s->dpp_pkex_bi,
						   wpa_s->own_addr, src,
						   wpa_s->dpp_pkex_identifier,
						   wpa_s->dpp_pkex_code,
						   buf, len);
	if (!wpa_s->dpp_pkex) {
		wpa_printf(MSG_DEBUG,
			   "DPP: Failed to process the request - ignore it");
		return;
	}

	msg = wpa_s->dpp_pkex->exchange_resp;
	wait_time = wpa_s->max_remain_on_chan;
	if (wait_time > 2000)
		wait_time = 2000;
	offchannel_send_action(wpa_s, freq, src, wpa_s->own_addr,
			       broadcast,
			       wpabuf_head(msg), wpabuf_len(msg),
			       wait_time, wpas_dpp_tx_pkex_status, 0);
}


static void
wpas_dpp_rx_pkex_exchange_resp(struct wpa_supplicant *wpa_s, const u8 *src,
			       const u8 *buf, size_t len, unsigned int freq)
{
	struct wpabuf *msg;
	unsigned int wait_time;

	wpa_printf(MSG_DEBUG, "DPP: PKEX Exchange Response from " MACSTR,
		   MAC2STR(src));

	/* TODO: Support multiple PKEX codes by iterating over all the enabled
	 * values here */

	if (!wpa_s->dpp_pkex || !wpa_s->dpp_pkex->initiator ||
	    wpa_s->dpp_pkex->exchange_done) {
		wpa_printf(MSG_DEBUG, "DPP: No matching PKEX session");
		return;
	}

	os_memcpy(wpa_s->dpp_pkex->peer_mac, src, ETH_ALEN);
	msg = dpp_pkex_rx_exchange_resp(wpa_s->dpp_pkex, buf, len);
	if (!msg) {
		wpa_printf(MSG_DEBUG, "DPP: Failed to process the response");
		return;
	}

	wpa_printf(MSG_DEBUG, "DPP: Send PKEX Commit-Reveal Request to " MACSTR,
		   MAC2STR(src));

	wait_time = wpa_s->max_remain_on_chan;
	if (wait_time > 2000)
		wait_time = 2000;
	offchannel_send_action(wpa_s, freq, src, wpa_s->own_addr,
			       broadcast,
			       wpabuf_head(msg), wpabuf_len(msg),
			       wait_time, wpas_dpp_tx_pkex_status, 0);
	wpabuf_free(msg);
}


static void
wpas_dpp_rx_pkex_commit_reveal_req(struct wpa_supplicant *wpa_s, const u8 *src,
				   const u8 *hdr, const u8 *buf, size_t len,
				   unsigned int freq)
{
	struct wpabuf *msg;
	unsigned int wait_time;
	struct dpp_pkex *pkex = wpa_s->dpp_pkex;
	struct dpp_bootstrap_info *bi;

	wpa_printf(MSG_DEBUG, "DPP: PKEX Commit-Reveal Request from " MACSTR,
		   MAC2STR(src));

	if (!pkex || pkex->initiator || !pkex->exchange_done) {
		wpa_printf(MSG_DEBUG, "DPP: No matching PKEX session");
		return;
	}

	msg = dpp_pkex_rx_commit_reveal_req(pkex, hdr, buf, len);
	if (!msg) {
		wpa_printf(MSG_DEBUG, "DPP: Failed to process the request");
		return;
	}

	wpa_printf(MSG_DEBUG, "DPP: Send PKEX Commit-Reveal Response to "
		   MACSTR, MAC2STR(src));

	wait_time = wpa_s->max_remain_on_chan;
	if (wait_time > 2000)
		wait_time = 2000;
	offchannel_send_action(wpa_s, freq, src, wpa_s->own_addr,
			       broadcast,
			       wpabuf_head(msg), wpabuf_len(msg),
			       wait_time, wpas_dpp_tx_pkex_status, 0);
	wpabuf_free(msg);

	bi = os_zalloc(sizeof(*bi));
	if (!bi)
		return;
	bi->id = wpas_dpp_next_id(wpa_s);
	bi->type = DPP_BOOTSTRAP_PKEX;
	os_memcpy(bi->mac_addr, src, ETH_ALEN);
	bi->num_freq = 1;
	bi->freq[0] = freq;
	bi->curve = pkex->own_bi->curve;
	bi->pubkey = pkex->peer_bootstrap_key;
	pkex->peer_bootstrap_key = NULL;
	dpp_pkex_free(pkex);
	wpa_s->dpp_pkex = NULL;
	if (dpp_bootstrap_key_hash(bi) < 0) {
		dpp_bootstrap_info_free(bi);
		return;
	}
	dl_list_add(&wpa_s->dpp_bootstrap, &bi->list);
}


static void
wpas_dpp_rx_pkex_commit_reveal_resp(struct wpa_supplicant *wpa_s, const u8 *src,
				    const u8 *hdr, const u8 *buf, size_t len,
				    unsigned int freq)
{
	int res;
	struct dpp_bootstrap_info *bi, *own_bi;
	struct dpp_pkex *pkex = wpa_s->dpp_pkex;
	char cmd[500];

	wpa_printf(MSG_DEBUG, "DPP: PKEX Commit-Reveal Response from " MACSTR,
		   MAC2STR(src));

	if (!pkex || !pkex->initiator || !pkex->exchange_done) {
		wpa_printf(MSG_DEBUG, "DPP: No matching PKEX session");
		return;
	}

	res = dpp_pkex_rx_commit_reveal_resp(pkex, hdr, buf, len);
	if (res < 0) {
		wpa_printf(MSG_DEBUG, "DPP: Failed to process the response");
		return;
	}

	own_bi = pkex->own_bi;

	bi = os_zalloc(sizeof(*bi));
	if (!bi)
		return;
	bi->id = wpas_dpp_next_id(wpa_s);
	bi->type = DPP_BOOTSTRAP_PKEX;
	os_memcpy(bi->mac_addr, src, ETH_ALEN);
	bi->num_freq = 1;
	bi->freq[0] = freq;
	bi->curve = own_bi->curve;
	bi->pubkey = pkex->peer_bootstrap_key;
	pkex->peer_bootstrap_key = NULL;
	dpp_pkex_free(pkex);
	wpa_s->dpp_pkex = NULL;
	if (dpp_bootstrap_key_hash(bi) < 0) {
		dpp_bootstrap_info_free(bi);
		return;
	}
	dl_list_add(&wpa_s->dpp_bootstrap, &bi->list);

	os_snprintf(cmd, sizeof(cmd), " peer=%u %s",
		    bi->id,
		    wpa_s->dpp_pkex_auth_cmd ? wpa_s->dpp_pkex_auth_cmd : "");
	wpa_printf(MSG_DEBUG,
		   "DPP: Start authentication after PKEX with parameters: %s",
		   cmd);
	if (wpas_dpp_auth_init(wpa_s, cmd) < 0) {
		wpa_printf(MSG_DEBUG,
			   "DPP: Authentication initialization failed");
		return;
	}
}


void wpas_dpp_rx_action(struct wpa_supplicant *wpa_s, const u8 *src,
			const u8 *buf, size_t len, unsigned int freq)
{
	u8 crypto_suite;
	enum dpp_public_action_frame_type type;
	const u8 *hdr;

	if (len < DPP_HDR_LEN)
		return;
	if (WPA_GET_BE24(buf) != OUI_WFA || buf[3] != DPP_OUI_TYPE)
		return;
	hdr = buf;
	buf += 4;
	len -= 4;
	crypto_suite = *buf++;
	type = *buf++;
	len -= 2;

	wpa_printf(MSG_DEBUG,
		   "DPP: Received DPP Public Action frame crypto suite %u type %d from "
		   MACSTR " freq=%u",
		   crypto_suite, type, MAC2STR(src), freq);
	if (crypto_suite != 1) {
		wpa_printf(MSG_DEBUG, "DPP: Unsupported crypto suite %u",
			   crypto_suite);
		return;
	}
	wpa_hexdump(MSG_MSGDUMP, "DPP: Received message attributes", buf, len);
	if (dpp_check_attrs(buf, len) < 0)
		return;

	switch (type) {
	case DPP_PA_AUTHENTICATION_REQ:
		wpas_dpp_rx_auth_req(wpa_s, src, hdr, buf, len, freq);
		break;
	case DPP_PA_AUTHENTICATION_RESP:
		wpas_dpp_rx_auth_resp(wpa_s, src, hdr, buf, len);
		break;
	case DPP_PA_AUTHENTICATION_CONF:
		wpas_dpp_rx_auth_conf(wpa_s, src, hdr, buf, len);
		break;
	case DPP_PA_PEER_DISCOVERY_RESP:
		wpas_dpp_rx_peer_disc_resp(wpa_s, src, buf, len);
		break;
	case DPP_PA_PKEX_EXCHANGE_REQ:
		wpas_dpp_rx_pkex_exchange_req(wpa_s, src, buf, len, freq);
		break;
	case DPP_PA_PKEX_EXCHANGE_RESP:
		wpas_dpp_rx_pkex_exchange_resp(wpa_s, src, buf, len, freq);
		break;
	case DPP_PA_PKEX_COMMIT_REVEAL_REQ:
		wpas_dpp_rx_pkex_commit_reveal_req(wpa_s, src, hdr, buf, len,
						   freq);
		break;
	case DPP_PA_PKEX_COMMIT_REVEAL_RESP:
		wpas_dpp_rx_pkex_commit_reveal_resp(wpa_s, src, hdr, buf, len,
						    freq);
		break;
	default:
		wpa_printf(MSG_DEBUG,
			   "DPP: Ignored unsupported frame subtype %d", type);
		break;
	}
}


static struct wpabuf *
wpas_dpp_gas_req_handler(void *ctx, const u8 *sa, const u8 *query,
			 size_t query_len)
{
	struct wpa_supplicant *wpa_s = ctx;
	struct dpp_authentication *auth = wpa_s->dpp_auth;
	struct wpabuf *resp;

	wpa_printf(MSG_DEBUG, "DPP: GAS request from " MACSTR,
		   MAC2STR(sa));
	if (!auth || !auth->auth_success ||
	    os_memcmp(sa, auth->peer_mac_addr, ETH_ALEN) != 0) {
		wpa_printf(MSG_DEBUG, "DPP: No matching exchange in progress");
		return NULL;
	}
	wpa_hexdump(MSG_DEBUG,
		    "DPP: Received Configuration Request (GAS Query Request)",
		    query, query_len);
	resp = dpp_conf_req_rx(auth, query, query_len);
	if (!resp)
		wpa_msg(wpa_s, MSG_INFO, DPP_EVENT_CONF_FAILED);
	return resp;
}


static void
wpas_dpp_gas_status_handler(void *ctx, struct wpabuf *resp, int ok)
{
	struct wpa_supplicant *wpa_s = ctx;
	struct dpp_authentication *auth = wpa_s->dpp_auth;

	if (!auth) {
		wpabuf_free(resp);
		return;
	}

	wpa_printf(MSG_DEBUG, "DPP: Configuration exchange completed (ok=%d)",
		   ok);
	eloop_cancel_timeout(wpas_dpp_reply_wait_timeout, wpa_s, NULL);
	offchannel_send_action_done(wpa_s);
	wpas_dpp_listen_stop(wpa_s);
	if (ok)
		wpa_msg(wpa_s, MSG_INFO, DPP_EVENT_CONF_SENT);
	else
		wpa_msg(wpa_s, MSG_INFO, DPP_EVENT_CONF_FAILED);
	dpp_auth_deinit(wpa_s->dpp_auth);
	wpa_s->dpp_auth = NULL;
	wpabuf_free(resp);
}


static unsigned int wpas_dpp_next_configurator_id(struct wpa_supplicant *wpa_s)
{
	struct dpp_configurator *conf;
	unsigned int max_id = 0;

	dl_list_for_each(conf, &wpa_s->dpp_configurator,
			 struct dpp_configurator, list) {
		if (conf->id > max_id)
			max_id = conf->id;
	}
	return max_id + 1;
}


int wpas_dpp_configurator_add(struct wpa_supplicant *wpa_s, const char *cmd)
{
	char *curve = NULL;
	char *key = NULL;
	u8 *privkey = NULL;
	size_t privkey_len = 0;
	int ret = -1;
	struct dpp_configurator *conf = NULL;

	curve = get_param(cmd, " curve=");
	key = get_param(cmd, " key=");

	if (key) {
		privkey_len = os_strlen(key) / 2;
		privkey = os_malloc(privkey_len);
		if (!privkey ||
		    hexstr2bin(key, privkey, privkey_len) < 0)
			goto fail;
	}

	conf = dpp_keygen_configurator(curve, privkey, privkey_len);
	if (!conf)
		goto fail;

	conf->id = wpas_dpp_next_configurator_id(wpa_s);
	dl_list_add(&wpa_s->dpp_configurator, &conf->list);
	ret = conf->id;
	conf = NULL;
fail:
	os_free(curve);
	str_clear_free(key);
	bin_clear_free(privkey, privkey_len);
	dpp_configurator_free(conf);
	return ret;
}


static int dpp_configurator_del(struct wpa_supplicant *wpa_s, unsigned int id)
{
	struct dpp_configurator *conf, *tmp;
	int found = 0;

	dl_list_for_each_safe(conf, tmp, &wpa_s->dpp_configurator,
			      struct dpp_configurator, list) {
		if (id && conf->id != id)
			continue;
		found = 1;
		dl_list_del(&conf->list);
		dpp_configurator_free(conf);
	}

	if (id == 0)
		return 0; /* flush succeeds regardless of entries found */
	return found ? 0 : -1;
}


int wpas_dpp_configurator_remove(struct wpa_supplicant *wpa_s, const char *id)
{
	unsigned int id_val;

	if (os_strcmp(id, "*") == 0) {
		id_val = 0;
	} else {
		id_val = atoi(id);
		if (id_val == 0)
			return -1;
	}

	return dpp_configurator_del(wpa_s, id_val);
}


int wpas_dpp_configurator_sign(struct wpa_supplicant *wpa_s, const char *cmd)
{
	struct dpp_authentication *auth;
	int ret = -1;
	char *curve = NULL;

	auth = os_zalloc(sizeof(*auth));
	if (!auth)
		return -1;

	curve = get_param(cmd, " curve=");
	wpas_dpp_set_configurator(wpa_s, auth, cmd);

	if (dpp_configurator_own_config(auth, curve) == 0) {
		wpas_dpp_handle_config_obj(wpa_s, auth);
		ret = 0;
	}

	dpp_auth_deinit(auth);
	os_free(curve);

	return ret;
}


static void
wpas_dpp_tx_introduction_status(struct wpa_supplicant *wpa_s,
				unsigned int freq, const u8 *dst,
				const u8 *src, const u8 *bssid,
				const u8 *data, size_t data_len,
				enum offchannel_send_action_result result)
{
	wpa_printf(MSG_DEBUG, "DPP: TX status: freq=%u dst=" MACSTR
		   " result=%s (DPP Peer Discovery Request)",
		   freq, MAC2STR(dst),
		   result == OFFCHANNEL_SEND_ACTION_SUCCESS ? "SUCCESS" :
		   (result == OFFCHANNEL_SEND_ACTION_NO_ACK ? "no-ACK" :
		    "FAILED"));
	/* TODO: Time out wait for response more quickly in error cases? */
}


int wpas_dpp_check_connect(struct wpa_supplicant *wpa_s, struct wpa_ssid *ssid,
			   struct wpa_bss *bss)
{
	struct os_time now;
	struct wpabuf *msg;
	unsigned int wait_time;

	if (!(ssid->key_mgmt & WPA_KEY_MGMT_DPP) || !bss)
		return 0; /* Not using DPP AKM - continue */
	if (wpa_sm_pmksa_exists(wpa_s->wpa, bss->bssid, ssid))
		return 0; /* PMKSA exists for DPP AKM - continue */

	if (!ssid->dpp_connector || !ssid->dpp_netaccesskey ||
	    !ssid->dpp_csign) {
		wpa_msg(wpa_s, MSG_INFO, DPP_EVENT_MISSING_CONNECTOR
			"missing %s",
			!ssid->dpp_connector ? "Connector" :
			(!ssid->dpp_netaccesskey ? "netAccessKey" :
			 "C-sign-key"));
		return -1;
	}

	os_get_time(&now);

	if (ssid->dpp_netaccesskey_expiry &&
	    ssid->dpp_netaccesskey_expiry < now.sec) {
		wpa_msg(wpa_s, MSG_INFO, DPP_EVENT_MISSING_CONNECTOR
			"netAccessKey expired");
		return -1;
	}

	wpa_printf(MSG_DEBUG,
		   "DPP: Starting network introduction protocol to derive PMKSA for "
		   MACSTR, MAC2STR(bss->bssid));

	msg = dpp_alloc_msg(DPP_PA_PEER_DISCOVERY_REQ,
			    5 + 4 + os_strlen(ssid->dpp_connector));
	if (!msg)
		return -1;

	/* Transaction ID */
	wpabuf_put_le16(msg, DPP_ATTR_TRANSACTION_ID);
	wpabuf_put_le16(msg, 1);
	wpabuf_put_u8(msg, TRANSACTION_ID);

	/* DPP Connector */
	wpabuf_put_le16(msg, DPP_ATTR_CONNECTOR);
	wpabuf_put_le16(msg, os_strlen(ssid->dpp_connector));
	wpabuf_put_str(msg, ssid->dpp_connector);

	/* TODO: Timeout on AP response */
	wait_time = wpa_s->max_remain_on_chan;
	if (wait_time > 2000)
		wait_time = 2000;
	offchannel_send_action(wpa_s, bss->freq, bss->bssid, wpa_s->own_addr,
			       broadcast,
			       wpabuf_head(msg), wpabuf_len(msg),
			       wait_time, wpas_dpp_tx_introduction_status, 0);
	wpabuf_free(msg);

	/* Request this connection attempt to terminate - new one will be
	 * started when network introduction protocol completes */
	os_memcpy(wpa_s->dpp_intro_bssid, bss->bssid, ETH_ALEN);
	wpa_s->dpp_intro_network = ssid;
	return 1;
}


int wpas_dpp_pkex_add(struct wpa_supplicant *wpa_s, const char *cmd)
{
	struct dpp_bootstrap_info *own_bi;
	const char *pos, *end;
	unsigned int wait_time;

	pos = os_strstr(cmd, " own=");
	if (!pos)
		return -1;
	pos += 5;
	own_bi = dpp_bootstrap_get_id(wpa_s, atoi(pos));
	if (!own_bi) {
		wpa_printf(MSG_DEBUG,
			   "DPP: Identified bootstrap info not found");
		return -1;
	}
	if (own_bi->type != DPP_BOOTSTRAP_PKEX) {
		wpa_printf(MSG_DEBUG,
			   "DPP: Identified bootstrap info not for PKEX");
		return -1;
	}
	wpa_s->dpp_pkex_bi = own_bi;

	os_free(wpa_s->dpp_pkex_identifier);
	wpa_s->dpp_pkex_identifier = NULL;
	pos = os_strstr(cmd, " identifier=");
	if (pos) {
		pos += 12;
		end = os_strchr(pos, ' ');
		if (!end)
			return -1;
		wpa_s->dpp_pkex_identifier = os_malloc(end - pos + 1);
		if (!wpa_s->dpp_pkex_identifier)
			return -1;
		os_memcpy(wpa_s->dpp_pkex_identifier, pos, end - pos);
		wpa_s->dpp_pkex_identifier[end - pos] = '\0';
	}

	pos = os_strstr(cmd, " code=");
	if (!pos)
		return -1;
	os_free(wpa_s->dpp_pkex_code);
	wpa_s->dpp_pkex_code = os_strdup(pos + 6);
	if (!wpa_s->dpp_pkex_code)
		return -1;

	if (os_strstr(cmd, " init=1")) {
		struct wpabuf *msg;

		wpa_printf(MSG_DEBUG, "DPP: Initiating PKEX");
		dpp_pkex_free(wpa_s->dpp_pkex);
		wpa_s->dpp_pkex = dpp_pkex_init(own_bi, wpa_s->own_addr,
						wpa_s->dpp_pkex_identifier,
						wpa_s->dpp_pkex_code);
		if (!wpa_s->dpp_pkex)
			return -1;

		msg = wpa_s->dpp_pkex->exchange_req;
		wait_time = wpa_s->max_remain_on_chan;
		if (wait_time > 2000)
			wait_time = 2000;
		/* TODO: Which channel to use? */
		offchannel_send_action(wpa_s, 2437, broadcast, wpa_s->own_addr,
				       broadcast,
				       wpabuf_head(msg), wpabuf_len(msg),
				       wait_time, wpas_dpp_tx_pkex_status, 0);
	}

	/* TODO: Support multiple PKEX info entries */

	os_free(wpa_s->dpp_pkex_auth_cmd);
	wpa_s->dpp_pkex_auth_cmd = os_strdup(cmd);

	return 1;
}


int wpas_dpp_pkex_remove(struct wpa_supplicant *wpa_s, const char *id)
{
	unsigned int id_val;

	if (os_strcmp(id, "*") == 0) {
		id_val = 0;
	} else {
		id_val = atoi(id);
		if (id_val == 0)
			return -1;
	}

	if ((id_val != 0 && id_val != 1) || !wpa_s->dpp_pkex_code)
		return -1;

	/* TODO: Support multiple PKEX entries */
	os_free(wpa_s->dpp_pkex_code);
	wpa_s->dpp_pkex_code = NULL;
	os_free(wpa_s->dpp_pkex_identifier);
	wpa_s->dpp_pkex_identifier = NULL;
	os_free(wpa_s->dpp_pkex_auth_cmd);
	wpa_s->dpp_pkex_auth_cmd = NULL;
	wpa_s->dpp_pkex_bi = NULL;
	/* TODO: Remove dpp_pkex only if it is for the identified PKEX code */
	dpp_pkex_free(wpa_s->dpp_pkex);
	wpa_s->dpp_pkex = NULL;
	return 0;
}


int wpas_dpp_init(struct wpa_supplicant *wpa_s)
{
	u8 adv_proto_id[7];

	adv_proto_id[0] = WLAN_EID_VENDOR_SPECIFIC;
	adv_proto_id[1] = 5;
	WPA_PUT_BE24(&adv_proto_id[2], OUI_WFA);
	adv_proto_id[5] = DPP_OUI_TYPE;
	adv_proto_id[6] = 0x01;

	if (gas_server_register(wpa_s->gas_server, adv_proto_id,
				sizeof(adv_proto_id), wpas_dpp_gas_req_handler,
				wpas_dpp_gas_status_handler, wpa_s) < 0)
		return -1;
	dl_list_init(&wpa_s->dpp_bootstrap);
	dl_list_init(&wpa_s->dpp_configurator);
	wpa_s->dpp_init_done = 1;
	return 0;
}


void wpas_dpp_deinit(struct wpa_supplicant *wpa_s)
{
#ifdef CONFIG_TESTING_OPTIONS
	os_free(wpa_s->dpp_config_obj_override);
	wpa_s->dpp_config_obj_override = NULL;
	os_free(wpa_s->dpp_discovery_override);
	wpa_s->dpp_discovery_override = NULL;
	os_free(wpa_s->dpp_groups_override);
	wpa_s->dpp_groups_override = NULL;
	wpa_s->dpp_ignore_netaccesskey_mismatch = 0;
#endif /* CONFIG_TESTING_OPTIONS */
	if (!wpa_s->dpp_init_done)
		return;
	eloop_cancel_timeout(wpas_dpp_reply_wait_timeout, wpa_s, NULL);
	offchannel_send_action_done(wpa_s);
	wpas_dpp_listen_stop(wpa_s);
	dpp_bootstrap_del(wpa_s, 0);
	dpp_configurator_del(wpa_s, 0);
	dpp_auth_deinit(wpa_s->dpp_auth);
	wpa_s->dpp_auth = NULL;
	wpas_dpp_pkex_remove(wpa_s, "*");
	wpa_s->dpp_pkex = NULL;
	os_memset(wpa_s->dpp_intro_bssid, 0, ETH_ALEN);
	os_free(wpa_s->dpp_configurator_params);
	wpa_s->dpp_configurator_params = NULL;
}
