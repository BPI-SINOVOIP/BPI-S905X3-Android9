/*
 * Copyright (c) 2015 iComm-semi Ltd.
 *
 * This program is free software: you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License as published by 
 * the Free Software Foundation, either version 3 of the License, or 
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
 * See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef ECLIPSE
#include <ssv_mod_conf.h>
#endif
#include <linux/version.h>
#if ((defined SSV_SUPPORT_HAL) && (defined SSV_SUPPORT_SSV6006))
#include <linux/etherdevice.h>
#include <linux/string.h>
#include <linux/platform_device.h>
#include "ssv6006_cfg.h"
#include "ssv6006_mac.h"
#include <smac/dev.h>
#include <smac/ssv_rc_minstrel.h>
#include <hal.h>
#include "ssv6006_priv.h"
#include <smac/ssv_skb.h>
#include <hci/hctrl.h>
#include <ssvdevice/ssv_cmd.h>
static struct ssv_hw * ssv6006_alloc_hw (void)
{
    struct ssv_hw *sh;
    sh = kzalloc(sizeof(struct ssv_hw), GFP_KERNEL);
    if (sh == NULL)
        goto out;
    memset((void *)sh, 0, sizeof(struct ssv_hw));
    sh->page_count = (u8 *)kzalloc(sizeof(u8) * SSV6006_ID_NUMBER, GFP_KERNEL);
    if (sh->page_count == NULL)
        goto out;
    memset(sh->page_count, 0, sizeof(u8) * SSV6006_ID_NUMBER);
    return sh;
out:
    if (sh->page_count)
        kfree(sh->page_count);
    if (sh)
        kfree(sh);
    return NULL;
}
static bool ssv6006_use_hw_encrypt(int cipher, struct ssv_softc *sc,
        struct ssv_sta_priv_data *sta_priv, struct ssv_vif_priv_data *vif_priv )
{
    return true;
}
static bool ssv6006_if_chk_mac2(struct ssv_hw *sh)
{
    printk(" %s: is not need to check MAC addres 2 for this model \n",__func__);
    return false;
}
static int ssv6006_get_wsid(struct ssv_softc *sc, struct ieee80211_vif *vif,
    struct ieee80211_sta *sta)
{
    struct ssv_vif_priv_data *vif_priv = (struct ssv_vif_priv_data *)vif->drv_priv;
    int s;
    struct ssv_sta_priv_data *sta_priv_dat=NULL;
    struct ssv_sta_info *sta_info;
    for (s = 0; s < SSV_NUM_STA; s++)
    {
        sta_info = &sc->sta_info[s];
        if ((sta_info->s_flags & STA_FLAG_VALID) == 0)
        {
            sta_info->aid = sta->aid;
            sta_info->sta = sta;
            sta_info->vif = vif;
            sta_info->s_flags = STA_FLAG_VALID;
            sta_priv_dat =
                (struct ssv_sta_priv_data *)sta->drv_priv;
            sta_priv_dat->sta_idx = s;
            sta_priv_dat->sta_info = sta_info;
            sta_priv_dat->has_hw_encrypt = false;
            sta_priv_dat->has_hw_decrypt = false;
            sta_priv_dat->need_sw_decrypt = false;
            sta_priv_dat->need_sw_encrypt = false;
            #ifdef USE_LOCAL_CRYPTO
            if (HAL_NEED_SW_CIPHER(sc->sh)){
                sta_priv_dat->crypto_data.ops = NULL;
                sta_priv_dat->crypto_data.priv = NULL;
                #ifdef HAS_CRYPTO_LOCK
                rwlock_init(&sta_priv_dat->crypto_data.lock);
                #endif
            }
            #endif
            if ( (vif_priv->pair_cipher == SSV_CIPHER_WEP40)
                || (vif_priv->pair_cipher == SSV_CIPHER_WEP104))
            {
                #ifdef USE_LOCAL_CRYPTO
                if (vif_priv->crypto_data.ops != NULL)
                {
                    sta_priv_dat->crypto_data.ops = vif_priv->crypto_data.ops;
                    sta_priv_dat->crypto_data.priv = vif_priv->crypto_data.priv;
                }
                #endif
                sta_priv_dat->has_hw_encrypt = vif_priv->has_hw_encrypt;
                sta_priv_dat->has_hw_decrypt = vif_priv->has_hw_decrypt;
                sta_priv_dat->need_sw_encrypt = vif_priv->need_sw_encrypt;
                sta_priv_dat->need_sw_decrypt = vif_priv->need_sw_decrypt;
            }
            list_add_tail(&sta_priv_dat->list, &vif_priv->sta_list);
            break;
        }
    }
    return s;
}
static void ssv6006_add_fw_wsid(struct ssv_softc *sc, struct ssv_vif_priv_data *vif_priv,
    struct ieee80211_sta *sta, struct ssv_sta_info *sta_info)
{
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
     " %s: is not need for this model \n",__func__);
}
static void ssv6006_del_fw_wsid(struct ssv_softc *sc, struct ieee80211_sta *sta,
    struct ssv_sta_info *sta_info)
{
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
     " %s: is not need for this model \n",__func__);
}
static void ssv6006_enable_fw_wsid(struct ssv_softc *sc, struct ieee80211_sta *sta,
    struct ssv_sta_info *sta_info, enum SSV6XXX_WSID_SEC key_type)
{
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
     " %s: is not need for this model \n",__func__);
}
static void ssv6006_disable_fw_wsid(struct ssv_softc *sc, int key_idx,
    struct ssv_sta_priv_data *sta_priv, struct ssv_vif_priv_data *vif_priv)
{
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
     " %s: is not need for this model \n",__func__);
}
static void ssv6006_set_fw_hwwsid_sec_type(struct ssv_softc *sc, struct ieee80211_sta *sta,
        struct ssv_sta_info *sta_info, struct ssv_vif_priv_data *vif_priv)
{
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
     " %s: is not need for this model \n",__func__);
}
static bool ssv6006_wep_use_hw_cipher(struct ssv_softc *sc, struct ssv_vif_priv_data *vif_priv)
{
    bool ret = false;
    if (sc->sh->cfg.use_sw_cipher != 1) {
        ret = true;
    }
    return ret;
}
static bool ssv6006_pairwise_wpa_use_hw_cipher(struct ssv_softc *sc,
    struct ssv_vif_priv_data *vif_priv, enum SSV_CIPHER_E cipher,
    struct ssv_sta_priv_data *sta_priv)
{
    bool ret = false;
    if (sc->sh->cfg.use_sw_cipher != 1) {
        ret = true;
    }
    return ret;
}
static bool ssv6006_group_wpa_use_hw_cipher(struct ssv_softc *sc,
    struct ssv_vif_priv_data *vif_priv, enum SSV_CIPHER_E cipher)
{
    int ret =false;
    if (sc->sh->cfg.use_sw_cipher != 1) {
        ret = true;
    }
    return ret;
}
static bool ssv6006_chk_if_support_hw_bssid(struct ssv_softc *sc,
    int vif_idx)
{
    if ((vif_idx >= 0) && (vif_idx < SSV6006_NUM_HW_BSSID))
        return true;
    printk(" %s: VIF %d doesn't support HW BSSID\n", __func__, vif_idx);
    return false;
}
static void ssv6006_chk_dual_vif_chg_rx_flow(struct ssv_softc *sc,
    struct ssv_vif_priv_data *vif_priv)
{
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
     " %s: is not need for this model \n",__func__);
}
static void ssv6006_restore_rx_flow(struct ssv_softc *sc,
    struct ssv_vif_priv_data *vif_priv, struct ieee80211_sta *sta)
{
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
     " %s: is not need for this model \n",__func__);
}
static int ssv6006_hw_crypto_key_write_wep(struct ssv_softc *sc,
    struct ieee80211_key_conf *keyconf, u8 algorithm,
    struct ssv_vif_info *vif_info)
{
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
     " %s: is not need for this model \n",__func__);
    return 0;
}
static void ssv6006_set_wep_hw_crypto_key(struct ssv_softc *sc,
    struct ssv_sta_info *sta_info, struct ssv_vif_priv_data *vif_priv)
{
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
     " %s: is not need for this model \n",__func__);
}
static bool ssv6006_put_mic_space_for_hw_ccmp_encrypt(struct ssv_softc *sc, struct sk_buff *skb)
{
    struct ieee80211_tx_info *info = IEEE80211_SKB_CB(skb);
    struct ieee80211_key_conf *hw_key = info->control.hw_key;
    struct ieee80211_hdr *hdr = (struct ieee80211_hdr *) skb->data;
    struct SKB_info_st *skb_info = (struct SKB_info_st *)skb->head;
    struct ieee80211_sta *sta = skb_info->sta;
    struct ssv_vif_priv_data *vif_priv = (struct ssv_vif_priv_data *)info->control.vif->drv_priv;
    struct ssv_sta_priv_data *ssv_sta_priv = sta ? (struct ssv_sta_priv_data *)sta->drv_priv : NULL;
    if ( (!ieee80211_is_data_qos(hdr->frame_control)
          && !ieee80211_is_data(hdr->frame_control))
       || !ieee80211_has_protected(hdr->frame_control) )
        return false;
    if (hw_key)
    {
        if(hw_key->cipher != WLAN_CIPHER_SUITE_CCMP)
            return false;
    }
    if (!is_unicast_ether_addr(hdr->addr1))
    {
        if (vif_priv->is_security_valid
            && vif_priv->has_hw_encrypt)
        {
            pskb_expand_head(skb, 0, CCMP_MIC_LEN, GFP_ATOMIC);
            skb_put(skb, CCMP_MIC_LEN);
            return true;
        }
    }
    else if (ssv_sta_priv != NULL)
    {
        if (ssv_sta_priv->has_hw_encrypt)
        {
            pskb_expand_head(skb, 0, CCMP_MIC_LEN, GFP_ATOMIC);
            skb_put(skb, CCMP_MIC_LEN);
            return true;
        }
    }
    return false;
}
static void ssv6006_init_tx_cfg(struct ssv_hw *sh)
{
    u32 dev_type = HCI_DEVICE_TYPE(sh->hci.hci_ctrl);
    if ((sh->cfg.tx_id_threshold == 0) || (sh->cfg.tx_id_threshold > SSV6006_ID_TX_THRESHOLD)) {
        if ((dev_type == SSV_HWIF_INTERFACE_USB) &&
             ( (sh->cfg.usb_hw_resource & USB_HW_RESOURCE_CHK_TXID ) == 0)) {
            sh->tx_info.tx_id_threshold = SSV6006_ID_USB_TX_THRESHOLD;
        } else {
      sh->tx_info.tx_id_threshold = SSV6006_ID_TX_THRESHOLD;
     }
  sh->tx_info.tx_lowthreshold_id_trigger = SSV6006_TX_LOWTHRESHOLD_ID_TRIGGER;
 } else {
  sh->tx_info.tx_id_threshold = sh->cfg.tx_id_threshold;
  sh->tx_info.tx_lowthreshold_id_trigger = (sh->tx_info.tx_id_threshold - 1);
 }
 if ((sh->cfg.tx_page_threshold == 0) || (sh->cfg.tx_page_threshold > SSV6006_PAGE_TX_THRESHOLD)) {
  sh->tx_info.tx_page_threshold = SSV6006_PAGE_TX_THRESHOLD;
  sh->tx_info.tx_lowthreshold_page_trigger = SSV6006_TX_LOWTHRESHOLD_PAGE_TRIGGER;
 } else {
  sh->tx_info.tx_page_threshold = sh->cfg.tx_page_threshold;
  sh->tx_info.tx_lowthreshold_page_trigger =
    (sh->tx_info.tx_page_threshold - (sh->tx_info.tx_page_threshold/SSV6006_AMPDU_DIVIDER));
 }
    sh->tx_page_available = sh->tx_info.tx_page_threshold;
 if (dev_type == SSV_HWIF_INTERFACE_USB) {
        sh->tx_info.tx_page_threshold = SSV6006_PAGE_TX_THRESHOLD - SSV6006_USB_FIFO;
        sh->tx_page_available = sh->tx_info.tx_page_threshold - SSV6006_RESERVED_USB_PAGE;
    }
 sh->tx_info.bk_txq_size = SSV6006_ID_AC_BK_OUT_QUEUE;
 sh->tx_info.be_txq_size = SSV6006_ID_AC_BE_OUT_QUEUE;
 sh->tx_info.vi_txq_size = SSV6006_ID_AC_VI_OUT_QUEUE;
 sh->tx_info.vo_txq_size = SSV6006_ID_AC_VO_OUT_QUEUE;
 sh->tx_info.manage_txq_size = SSV6006_ID_MANAGER_QUEUE;
 sh->ampdu_divider = SSV6006_AMPDU_DIVIDER;
 memcpy(&(sh->hci.hci_ctrl->tx_info), &(sh->tx_info), sizeof(struct ssv6xxx_tx_hw_info));
}
static void ssv6006_init_rx_cfg(struct ssv_hw *sh)
{
    sh->rx_info.rx_id_threshold = SSV6006_TOTAL_ID - (sh->tx_info.tx_id_threshold + SSV6006_ID_TX_USB + SSV6006_ID_SEC);
    sh->rx_info.rx_page_threshold = SSV6006_PAGE_RX_THRESHOLD;
 sh->rx_info.rx_ba_ma_sessions = SSV6006_RX_BA_MAX_SESSIONS;
 memcpy(&(sh->hci.hci_ctrl->rx_info), &(sh->rx_info), sizeof(struct ssv6xxx_rx_hw_info));
}
static u32 ssv6006_ba_map_walker (struct AMPDU_TID_st *ampdu_tid, u32 start_ssn,
    u32 sn_bit_map[2], u32 *p_acked_num, struct aggr_ssn *ampdu_ssn, struct ssv_softc *sc)
{
    int i = 0;
    u32 ssn = ampdu_ssn->ssn[0];
    u32 word_idx = (-1), bit_idx = (-1);
    bool found = ssv6xxx_ssn_to_bit_idx(start_ssn, ssn, &word_idx, &bit_idx);
    bool first_found = found;
    u32 aggr_num = 0;
    u32 acked_num = 0;
    if (found && (word_idx >= 2 || bit_idx >= 32))
        prn_aggr_err(sc, "idx error 1: %d %d %d %d\n",
                     start_ssn, ssn, word_idx, bit_idx);
    while ((i < MAX_AGGR_NUM) && (ssn < SSV_AMPDU_MAX_SSN))
    {
        u32 cur_ssn;
        struct sk_buff *skb = INDEX_PKT_BY_SSN(ampdu_tid, ssn);
        u32 skb_ssn = (skb == NULL) ? (-1) : ampdu_skb_ssn(skb);
        struct SKB_info_st *skb_info;
        aggr_num++;
        if (skb_ssn != ssn){
            prn_aggr_err(sc, "Unmatched SSN packet: %d - %d - %d\n",
                         ssn, skb_ssn, start_ssn);
        } else {
            skb_info = (struct SKB_info_st *) (skb->head);
            if (found && (sn_bit_map[word_idx] & (1 << bit_idx))) {
    if (skb_info->ampdu_tx_status == AMPDU_ST_SENT) {
                 dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_AMPDU_SSN,"%s: mark ssn %d done\n", __func__, ssn);
                 skb_info->ampdu_tx_status = AMPDU_ST_DONE;
    } else {
                 dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_AMPDU_SSN,"%s: Find a MPDU of status %d! ssn %d\n",
      __func__, skb_info->ampdu_tx_status, ssn);
    }
                acked_num++;
            } else {
    if (skb_info->ampdu_tx_status == AMPDU_ST_SENT) {
                 dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_AMPDU_SSN,"%s: mark ssn %d retry\n", __func__, ssn);
                 ssv6xxx_mark_skb_retry(sc, skb_info, skb);
    } else {
                 dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_AMPDU_SSN,"%s: Find a MPDU of status %d! ssn %d\n",
      __func__, skb_info->ampdu_tx_status, ssn);
    }
            }
        }
        cur_ssn = ssn;
        if ((++i >= MAX_AGGR_NUM) || (i >= ampdu_ssn->mpdu_num))
            break;
        ssn = ampdu_ssn->ssn[i];
        if (ssn >= SSV_AMPDU_MAX_SSN)
            break;
        if (first_found) {
            u32 old_word_idx, old_bit_idx;
            found = ssv6xxx_inc_bit_idx(sc, cur_ssn, ssn, &word_idx, &bit_idx);
            old_word_idx = word_idx;
            old_bit_idx = bit_idx;
            if (found && (word_idx >= 2 || bit_idx >= 32)) {
                prn_aggr_err(sc,
                        "idx error 2: %d 0x%08X 0X%08X %d %d (%d %d) (%d %d)\n",
                        start_ssn, sn_bit_map[1], sn_bit_map[0], cur_ssn, ssn, word_idx, bit_idx, old_word_idx, old_bit_idx);
                found = false;
            }else if (!found) {
                int j;
                prn_aggr_err(sc, "SN out-of-order: %d\n", start_ssn);
                for (j = 0; j < ampdu_ssn->mpdu_num; j++)
                   prn_aggr_err(sc, " %d", ampdu_ssn->ssn[j]);
                prn_aggr_err(sc, "\n");
            }
        } else {
            found = ssv6xxx_ssn_to_bit_idx(start_ssn, ssn, &word_idx, &bit_idx);
            first_found = found;
            if (found && (word_idx >= 2 || bit_idx >= 32))
                prn_aggr_err(sc, "idx error 3: %d %d %d %d\n",
                             cur_ssn, ssn, word_idx, bit_idx);
        }
    }
    ssv6xxx_release_frames(ampdu_tid);
    if (p_acked_num != NULL)
        *p_acked_num = acked_num;
    return aggr_num;
}
static void ssv6006_ampdu_ba_notify(struct ssv_softc *sc, struct ieee80211_sta *sta,
  u32 pkt_no, int ampdu_len, int ampdu_ack_len)
{
 struct ssv_sta_priv_data *ssv_sta_priv;
 struct ssv_minstrel_sta_priv *minstrel_sta_priv;
 struct ssv_minstrel_ht_sta *mhs;
 struct ssv_minstrel_ht_rpt ht_rpt[SSV6006RC_MAX_RATE_SERIES];
 struct ssv_minstrel_ampdu_rate_rpt *ampdu_rpt;
 int i, rpt_idx = -1;
 ssv_sta_priv = (struct ssv_sta_priv_data *)sta->drv_priv;
 if((minstrel_sta_priv = (struct ssv_minstrel_sta_priv *)ssv_sta_priv->rc_info) == NULL)
  return;
 if (!minstrel_sta_priv->is_ht)
  return;
 mhs = (struct ssv_minstrel_ht_sta *)&minstrel_sta_priv->ht;
 if (!ssv6006_rc_get_previous_ampdu_rpt(mhs, pkt_no, &rpt_idx))
 {
  ssv6006_rc_add_ampdu_rpt_to_list(sc, mhs, NULL, pkt_no, ampdu_len, ampdu_ack_len);
 } else {
  ampdu_rpt = (struct ssv_minstrel_ampdu_rate_rpt *)&mhs->ampdu_rpt_list[rpt_idx];
  for (i = 0; i < SSV6006RC_MAX_RATE_SERIES; i++) {
   ht_rpt[i].dword = ampdu_rpt->rate_rpt[i].dword;
   ht_rpt[i].count = ampdu_rpt->rate_rpt[i].count;
   ht_rpt[i].success = ampdu_rpt->rate_rpt[i].success;
   ht_rpt[i].last = ampdu_rpt->rate_rpt[i].last;
  }
  ssv_minstrel_ht_tx_status(sc, ssv_sta_priv->rc_info, ht_rpt, SSV6006RC_MAX_RATE_SERIES,
    ampdu_len, ampdu_ack_len, ampdu_rpt->is_sample);
  ampdu_rpt->used = false;
 }
}
int ssv6006_ampdu_rx_start(struct ieee80211_hw *hw, struct ieee80211_vif *vif, struct ieee80211_sta *sta,
        u16 tid, u16 *ssn, u8 buf_size)
{
    struct ssv_softc *sc = hw->priv;
    struct ssv_sta_info *sta_info;
    int i = 0;
    bool find_peer = false;
    for (i = 0; i < SSV_NUM_STA; i++) {
        sta_info = &sc->sta_info[i];
        if ((sta_info->s_flags & STA_FLAG_VALID) && (sta == sta_info->sta)) {
            find_peer = true;
            break;
        }
    }
    if ((find_peer == false) || (sc->rx_ba_session_count > sc->sh->rx_info.rx_ba_ma_sessions))
        return -EBUSY;
    if (sta_info->s_flags & STA_FLAG_AMPDU_RX)
        return 0;
    printk(KERN_ERR "IEEE80211_AMPDU_RX_START %02X:%02X:%02X:%02X:%02X:%02X %d.\n",
        sta->addr[0], sta->addr[1], sta->addr[2], sta->addr[3],
        sta->addr[4], sta->addr[5], tid);
    sc->rx_ba_session_count++;
    sta_info->s_flags |= STA_FLAG_AMPDU_RX;
    return 0;
}
static void ssv6006_ampdu_ba_handler (struct ieee80211_hw *hw, struct sk_buff *skb,
    u32 tx_pkt_run_no)
{
    struct ssv_softc *sc = hw->priv;
    struct ieee80211_hdr *hdr = (struct ieee80211_hdr *) (skb->data
                                                          + sc->sh->rx_desc_len);
    AMPDU_BLOCKACK *BA_frame = (AMPDU_BLOCKACK *) hdr;
    struct ieee80211_sta *sta;
    struct ssv_sta_priv_data *ssv_sta_priv;
    int i;
    u32 ssn, aggr_num = 0, acked_num = 0;
    u8 tid_no;
    u32 sn_bit_map[2];
    sta = ssv6xxx_find_sta_by_rx_skb(sc, skb);
    if (sta == NULL) {
        dev_kfree_skb_any(skb);
        return;
    }
    ssv_sta_priv = (struct ssv_sta_priv_data *) sta->drv_priv;
    ssn = BA_frame->BA_ssn;
    sn_bit_map[0] = BA_frame->BA_sn_bit_map[0];
    sn_bit_map[1] = BA_frame->BA_sn_bit_map[1];
    tid_no = BA_frame->tid_info;
    ssv_sta_priv->ampdu_mib_total_BA_counter++;
    if (ssv_sta_priv->ampdu_tid[tid_no].state == AMPDU_STATE_STOP){
        prn_aggr_err(sc, "%s: state == AMPDU_STATE_STOP.\n", __func__);
        dev_kfree_skb_any(skb);
        return;
    }
    if ( (tx_pkt_run_no < SSV6XXX_PKT_RUN_TYPE_AMPDU_START) ||(tx_pkt_run_no > SSV6XXX_PKT_RUN_TYPE_AMPDU_END)){
        dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_AMPDU_SSN,
            "%s:\n Invalid tx_pkt_run_no %d\n", __func__, tx_pkt_run_no);
        dev_kfree_skb_any(skb);
        return;
    }
    for (i = 0; i < MAX_CONCUR_AMPDU; i ++){
        if (ssv_sta_priv->ampdu_ssn[i].tx_pkt_run_no == tx_pkt_run_no){
            dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_AMPDU_SSN,
                "%s:\n BA tx_pkt_run_no %d: found at slot %d \n", __func__, tx_pkt_run_no, i);
            break;
        }
    }
    if (i == MAX_CONCUR_AMPDU) {
        dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_AMPDU_SSN,"%s:tx_pkt_run_no %d not found on sent list.\n",
    __func__,tx_pkt_run_no );
        dev_kfree_skb_any(skb);
        return;
    }
    if (ssv_sta_priv->ampdu_ssn[i].mpdu_num == 0) {
        dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_AMPDU_SSN,"%s: invalid sent list\n",__func__ );
        dev_kfree_skb_any(skb);
        return;
    }
    ssv_sta_priv->ampdu_tid[tid_no].mib.ampdu_mib_BA_counter++;
    aggr_num = ssv6006_ba_map_walker(&(ssv_sta_priv->ampdu_tid[tid_no]), ssn,
                              sn_bit_map, &acked_num, &ssv_sta_priv->ampdu_ssn[i], sc);
    memset((void*) &ssv_sta_priv->ampdu_ssn[i], 0, sizeof(struct aggr_ssn));
 ssv6006_ampdu_ba_notify(sc, sta, tx_pkt_run_no, aggr_num, acked_num);
    dev_kfree_skb_any(skb);
}
static void ssv6006_adj_config(struct ssv_hw *sh)
{
 int dev_type;
    if (sh->cfg.force_chip_identity)
        sh->cfg.chip_identity = sh->cfg.force_chip_identity;
    switch (sh->cfg.chip_identity) {
        case SV6255P:
        case SV6256P:
                sh->cfg.hw_caps |= SSV6200_HW_CAP_5GHZ;
            break;
        case SV6155P:
        case SV6156P:
        case SV6166P:
        case SV6166F:
        case SV6151P_SV6152P:
                printk("not support 5G for this chip!! \n");
                sh->cfg.hw_caps = sh->cfg.hw_caps & (~(SSV6200_HW_CAP_5GHZ));
            break;
        default:
                printk("unknown chip\n");
            break;
    }
    if (strstr(sh->priv->chip_id, SSV6006D)){
        printk("not support 5G for this chip!! \n");
        sh->cfg.hw_caps = sh->cfg.hw_caps & (~(SSV6200_HW_CAP_5GHZ));
    }
    if (sh->cfg.use_wpa2_only){
        printk("%s: use_wpa2_only set to 1, force it to 0 \n", __func__);
        sh->cfg.use_wpa2_only = 0;
    }
    if (sh->cfg.rx_burstread) {
        printk("%s: rx_burstread set to 1, force it to 0 \n", __func__);
        sh->cfg.rx_burstread = false;
    }
 dev_type = HCI_DEVICE_TYPE(sh->hci.hci_ctrl);
    if (dev_type == SSV_HWIF_INTERFACE_USB){
     if (sh->cfg.hw_caps & SSV6200_HW_CAP_HCI_RX_AGGR){
            printk("%s: clear hci rx aggregation setting \n", __func__);
            sh->cfg.hw_caps = sh->cfg.hw_caps & (~(SSV6200_HW_CAP_HCI_RX_AGGR));
        }
        if ((sh->cfg.crystal_type == SSV6XXX_IQK_CFG_XTAL_26M) || (sh->cfg.crystal_type == SSV6XXX_IQK_CFG_XTAL_24M)) {
            sh->cfg.crystal_type = SSV6XXX_IQK_CFG_XTAL_40M;
            printk("%s: for USB, change iqk config crystal_type to %d \n", __func__, sh->cfg.crystal_type);
        }
 }
    if ((sh->cfg.tx_stuck_detect) && (dev_type == SSV_HWIF_INTERFACE_SDIO)) {
        printk("%s: tx_stuck_detect set to 1, force it to 0 \n", __func__);
        sh->cfg.tx_stuck_detect = false;
    }
    if (strstr(sh->priv->chip_id, SSV6006C)) {
        printk("%s: clear hw beacon \n", __func__);
        sh->cfg.hw_caps &= ~SSV6200_HW_CAP_BEACON;
    }
}
static void ssv6006_get_fw_name(u8 *fw_name)
{
    strcpy(fw_name, "ssv6x5x-sw.bin");
}
static bool ssv6006_need_sw_cipher(struct ssv_hw *sh)
{
    if (sh->cfg.use_sw_cipher){
        return true;
    } else {
        return false;
    }
}
static void ssv6006_send_tx_poll_cmd(struct ssv_hw *sh, u32 type)
{
 struct sk_buff *skb;
 struct cfg_host_cmd *host_cmd;
 int retval = 0;
    if (!sh->cfg.tx_stuck_detect)
        return;
 skb = ssv_skb_alloc(sh->sc, HOST_CMD_HDR_LEN);
 if (!skb) {
  printk("%s(): Fail to alloc cmd buffer.\n", __FUNCTION__);
 }
 skb_put(skb, HOST_CMD_HDR_LEN);
 host_cmd = (struct cfg_host_cmd *)skb->data;
    memset(host_cmd, 0x0, sizeof(struct cfg_host_cmd));
 host_cmd->c_type = HOST_CMD;
 host_cmd->RSVD0 = type;
 host_cmd->h_cmd = (u8)SSV6XXX_HOST_CMD_TX_POLL;
 host_cmd->len = skb->len;
 retval = HCI_SEND_CMD(sh, skb);
    if (retval)
        printk("%s(): Fail to send tx polling cmd\n", __FUNCTION__);
    ssv_skb_free(sh->sc, skb);
}
static void ssv6006_cmd_set_hwq_limit(struct ssv_hw *sh, int argc, char *argv[])
{
    struct ssv_cmd_data *cmd_data = &sh->sc->cmd_data;
    char *endp;
    if ( argc != 3) return;
    if (!strcmp(argv[1], "bk")) {
        sh->cfg.bk_txq_size = simple_strtoul(argv[2], &endp, 0);
    } else if (!strcmp(argv[1], "be")) {
        sh->cfg.be_txq_size = simple_strtoul(argv[2], &endp, 0);
    } else if (!strcmp(argv[1], "vi")) {
        sh->cfg.vi_txq_size = simple_strtoul(argv[2], &endp, 0);
    } else if (!strcmp(argv[1], "vo")) {
        sh->cfg.vo_txq_size = simple_strtoul(argv[2], &endp, 0);
    } else if (!strcmp(argv[1], "mng")) {
        sh->cfg.manage_txq_size = simple_strtoul(argv[2], &endp, 0);
    } else {
        snprintf_res(cmd_data,"\t\t %s is unknown!\n", argv[1]);
    }
}
static void ssv6006_flash_read_all_map(struct ssv_hw *sh)
{
    struct file *fp = (struct file *)NULL;
 int rdlen = 0, i = 0;
    struct ssv6006_flash_layout_table flash_table;
    memset(&flash_table, 0, sizeof(struct ssv6006_flash_layout_table));
    if (sh->cfg.flash_bin_path[0] != 0x00)
        fp = filp_open(sh->cfg.flash_bin_path, O_RDONLY, 0);
    else
        fp = filp_open(DEFAULT_CFG_BIN_NAME, O_RDONLY, 0);
    if (IS_ERR(fp) || fp == NULL)
        fp = filp_open(SEC_CFG_BIN_NAME, O_RDONLY, 0);
    if (IS_ERR(fp) || fp == NULL) {
        printk("flash_file %s not found\n", DEFAULT_CFG_BIN_NAME);
        return;
    }
 rdlen = kernel_read(fp, fp->f_pos, (u8 *)&flash_table, sizeof(struct ssv6006_flash_layout_table));
    filp_close((struct file *)fp, NULL);
    if (rdlen < 0)
        return;
#define SV6155P_IC 0x6155
#define SV6156P_IC 0x6156
#define SV6255P_IC 0x6255
#define SV6256P_IC 0x6256
    if (!((flash_table.ic == SV6155P_IC) ||
          (flash_table.ic == SV6156P_IC) ||
          (flash_table.ic == SV6255P_IC) ||
          (flash_table.ic == SV6256P_IC))) {
        printk("Invalid flash table [ic=0x%04x]\n", flash_table.ic);
        BUG_ON(1);
    }
    sh->sc->thermal_monitor = true;
    sh->flash_config.exist = true;
    sh->flash_config.dcdc = flash_table.dcdc;
    sh->cfg.volt_regulator = ((sh->flash_config.dcdc) ? SSV6XXX_VOLT_DCDC_CONVERT : SSV6XXX_VOLT_LDO_CONVERT);
    sh->flash_config.padpd = flash_table.padpd;
    sh->cfg.disable_dpd = ((sh->flash_config.padpd) ? 0x0 : 0x1f);
    sh->flash_config.xtal_offset_tempe_state = SSV_TEMPERATURE_NORMAL;
    sh->flash_config.xtal_offset_high_boundary = flash_table.xtal_offset_high_boundary;
    sh->flash_config.xtal_offset_low_boundary = flash_table.xtal_offset_low_boundary;
    sh->flash_config.band_gain_tempe_state = SSV_TEMPERATURE_NORMAL;
    sh->flash_config.band_gain_high_boundary = flash_table.band_gain_high_boundary;
    sh->flash_config.band_gain_low_boundary = flash_table.band_gain_low_boundary;
    sh->flash_config.rt_config.xtal_offset = flash_table.xtal_offset_rt_config;
    sh->flash_config.lt_config.xtal_offset = flash_table.xtal_offset_lt_config;
    sh->flash_config.ht_config.xtal_offset = flash_table.xtal_offset_ht_config;
    sh->flash_config.g_band_pa_bias0 = flash_table.g_band_pa_bias0;
    sh->flash_config.g_band_pa_bias1 = flash_table.g_band_pa_bias1;
    sh->flash_config.a_band_pa_bias0 = flash_table.a_band_pa_bias0;
    sh->flash_config.a_band_pa_bias1 = flash_table.a_band_pa_bias1;
#define SIZE_G_BAND_GAIN (sizeof(sh->flash_config.rt_config.g_band_gain) / sizeof((sh->flash_config.rt_config.g_band_gain)[0]))
    for (i = 0; i < SIZE_G_BAND_GAIN; i++) {
        sh->flash_config.rt_config.g_band_gain[i] = flash_table.g_band_gain_rt[i];
        sh->flash_config.lt_config.g_band_gain[i] = flash_table.g_band_gain_lt[i];
        sh->flash_config.ht_config.g_band_gain[i] = flash_table.g_band_gain_ht[i];
    }
#define SIZE_A_BAND_GAIN (sizeof(sh->flash_config.rt_config.a_band_gain) / sizeof((sh->flash_config.rt_config.a_band_gain)[0]))
    for (i = 0; i < SIZE_A_BAND_GAIN; i++) {
        sh->flash_config.rt_config.a_band_gain[i] = flash_table.a_band_gain_rt[i];
        sh->flash_config.lt_config.a_band_gain[i] = flash_table.a_band_gain_lt[i];
        sh->flash_config.ht_config.a_band_gain[i] = flash_table.a_band_gain_ht[i];
    }
#define SIZE_RATE_DELTA (sizeof(sh->flash_config.rate_delta) / sizeof((sh->flash_config.rate_delta)[0]))
    for (i = 0; i < SIZE_RATE_DELTA; i++) {
        sh->flash_config.rate_delta[i] = flash_table.rate_delta[i];
    }
    if (!(sh->sc->log_ctrl & LOG_FLASH_BIN))
        return;
    printk("flash.bin configuration\n");
    printk("xtal_offset_boundary, high = %d, low = %d\n",
            sh->flash_config.xtal_offset_high_boundary, sh->flash_config.xtal_offset_low_boundary);
    printk("band_gain_boundary, high = %d, low = %d\n",
            sh->flash_config.band_gain_high_boundary, sh->flash_config.band_gain_low_boundary);
    printk("xtal_offset, rt = 0x%04x, lt = 0x%04x, ht = 0x%04x\n",
            sh->flash_config.rt_config.xtal_offset,
            sh->flash_config.lt_config.xtal_offset,
            sh->flash_config.ht_config.xtal_offset);
    printk("g_band_pa_bias0 = 0x%08x, g_band_pa_bias1 = 0x%08x\n",
            sh->flash_config.g_band_pa_bias0, sh->flash_config.g_band_pa_bias1);
    printk("a_band_pa_bias0 = 0x%08x, a_band_pa_bias1 = 0x%08x\n",
            sh->flash_config.a_band_pa_bias0, sh->flash_config.a_band_pa_bias1);
    printk("g band gain:\trt\tlt\tht\n");
    for (i = 0; i < SIZE_G_BAND_GAIN; i++) {
        printk("\t\t\t0x%x,\t0x%x,\t0x%x", sh->flash_config.rt_config.g_band_gain[i],
                                           sh->flash_config.lt_config.g_band_gain[i],
                                           sh->flash_config.ht_config.g_band_gain[i]);
    }
    printk("\n");
    printk("a band gain:\trt\tlt\tht\n");
    for (i = 0; i < SIZE_A_BAND_GAIN; i++) {
        printk("\t\t\t0x%x,\t0x%x,\t0x%x", sh->flash_config.rt_config.a_band_gain[i],
                                           sh->flash_config.lt_config.a_band_gain[i],
                                           sh->flash_config.ht_config.a_band_gain[i]);
    }
    printk("\n");
    printk("rate delta: ");
    for (i = 0; i < SIZE_RATE_DELTA; i++)
        printk("%x, ", sh->flash_config.rate_delta[i]);
    return;
}
void ssv_attach_ssv6006_common(struct ssv_hal_ops *hal_ops)
{
    hal_ops->alloc_hw = ssv6006_alloc_hw;
    hal_ops->use_hw_encrypt = ssv6006_use_hw_encrypt;
    hal_ops->if_chk_mac2= ssv6006_if_chk_mac2;
    hal_ops->get_wsid = ssv6006_get_wsid;
    hal_ops->add_fw_wsid = ssv6006_add_fw_wsid;
    hal_ops->del_fw_wsid = ssv6006_del_fw_wsid;
    hal_ops->enable_fw_wsid = ssv6006_enable_fw_wsid;
    hal_ops->disable_fw_wsid = ssv6006_disable_fw_wsid;
    hal_ops->set_fw_hwwsid_sec_type = ssv6006_set_fw_hwwsid_sec_type;
    hal_ops->wep_use_hw_cipher = ssv6006_wep_use_hw_cipher;
    hal_ops->pairwise_wpa_use_hw_cipher = ssv6006_pairwise_wpa_use_hw_cipher;
    hal_ops->group_wpa_use_hw_cipher = ssv6006_group_wpa_use_hw_cipher;
    hal_ops->chk_if_support_hw_bssid = ssv6006_chk_if_support_hw_bssid;
    hal_ops->chk_dual_vif_chg_rx_flow = ssv6006_chk_dual_vif_chg_rx_flow;
    hal_ops->restore_rx_flow = ssv6006_restore_rx_flow;
    hal_ops->hw_crypto_key_write_wep = ssv6006_hw_crypto_key_write_wep;
    hal_ops->set_wep_hw_crypto_key = ssv6006_set_wep_hw_crypto_key;
    hal_ops->put_mic_space_for_hw_ccmp_encrypt = ssv6006_put_mic_space_for_hw_ccmp_encrypt;
    hal_ops->init_tx_cfg = ssv6006_init_tx_cfg;
    hal_ops->init_rx_cfg = ssv6006_init_rx_cfg;
    hal_ops->ampdu_rx_start = ssv6006_ampdu_rx_start;
    hal_ops->ampdu_ba_handler = ssv6006_ampdu_ba_handler;
    hal_ops->adj_config = ssv6006_adj_config;
    hal_ops->get_fw_name = ssv6006_get_fw_name;
    hal_ops->need_sw_cipher = ssv6006_need_sw_cipher;
 hal_ops->send_tx_poll_cmd = ssv6006_send_tx_poll_cmd;
    hal_ops->cmd_hwq_limit = ssv6006_cmd_set_hwq_limit;
    hal_ops->flash_read_all_map = ssv6006_flash_read_all_map;
}
#endif
