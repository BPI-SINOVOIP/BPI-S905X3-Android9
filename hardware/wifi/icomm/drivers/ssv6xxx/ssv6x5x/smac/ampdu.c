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

#include <linux/version.h>
#include <ssv6200.h>
#include "dev.h"
#include "ap.h"
#include "sec.h"
#include "ssv_rc_common.h"
#include "ssv_ht_rc.h"
#include "ssv_skb.h"
#include <hal.h>
extern struct ieee80211_ops ssv6200_ops;
#define BA_WAIT_TIMEOUT (100)
#define AMPDU_TID_TO_SC(ampdu_tid) \
    ({ \
    struct AMPDU_TID_st *_ampdu_tid = (ampdu_tid); \
    struct ieee80211_sta *sta = _ampdu_tid->sta; \
    struct ssv_sta_priv_data *ssv_sta_priv = (struct ssv_sta_priv_data *) sta->drv_priv; \
    const struct ssv_sta_info (*sta_info)[] = (const struct ssv_sta_info (*)[])(ssv_sta_priv->sta_info - ssv_sta_priv->sta_idx); \
    struct ssv_softc *sc = container_of(sta_info, struct ssv_softc, sta_info); \
    sc; \
    })
#define get_tid_aggr_len(agg_len,tid_data) \
    ({ \
        u32 agg_max_num = (tid_data)->agg_num_max; \
        u32 to_agg_len = (agg_len); \
        (agg_len >= agg_max_num) ? agg_max_num : to_agg_len; \
    })
#define NEXT_PKT_SN(sn) \
    ({ (sn + 1) % SSV_AMPDU_MAX_SSN; })
#define INC_PKT_SN(sn) \
    ({ \
        sn = NEXT_PKT_SN(sn); \
        sn; \
    })
#ifdef CONFIG_SSV6XXX_DEBUGFS
static ssize_t ampdu_tx_mib_dump (struct ssv_sta_priv_data *ssv_sta_priv,
                                  char *mib_str, ssize_t length);
static int _dump_ba_skb (struct ssv_softc *sc, char *buf, int buf_size, struct sk_buff *ba_skb);
#endif
static struct sk_buff* _aggr_retry_mpdu (struct ssv_softc *sc,
                                         struct AMPDU_TID_st *cur_AMPDU_TID,
                                         struct sk_buff_head *retry_queue,
                                         u32 max_aggr_len);
int ssv6200_dump_BA_notification (char *buf,
                                  struct ampdu_ba_notify_data *ba_notification);
static struct sk_buff *_alloc_ampdu_skb (struct ssv_softc *sc,
                                         struct AMPDU_TID_st *ampdu_tid,
                                         u32 len);
static bool _sync_ampdu_pkt_arr (struct ssv_softc *sc, struct AMPDU_TID_st *ampdu_tid,
                                 struct sk_buff *ampdu_skb, bool retry);
static int _acquire_new_ampdu_ssn_slot(struct ssv_softc *sc, struct sk_buff *ampdu);
static void _put_mpdu_to_ampdu (struct ssv_softc *sc, struct sk_buff *ampdu,
                                struct sk_buff *mpdu);
static u32 _flush_early_ampdu_q (struct ssv_softc *sc,
                                 struct AMPDU_TID_st *ampdu_tid);
static bool _is_skb_q_empty (struct ssv_softc *sc, struct sk_buff *skb);
static void _aggr_ampdu_tx_q (struct ieee80211_hw *hw,
                              struct AMPDU_TID_st *ampdu_tid);
static void _queue_early_ampdu (struct ssv_softc *sc,
                                struct AMPDU_TID_st *ampdu_tid,
                                struct sk_buff *ampdu_skb);
void ssv6xxx_mark_skb_retry (struct ssv_softc *sc, struct SKB_info_st *skb_info,
                            struct sk_buff *skb);
static void _print_BA(struct AMPDU_TID_st *ampdu_tid);
#ifdef CONFIG_DEBUG_SKB_TIMESTAMP
unsigned int cal_duration_of_ampdu(struct sk_buff *ampdu_skb, int stage)
{
 unsigned int timeout;
    SKB_info *mpdu_skb_info;
    u16 ssn = 0;
    struct sk_buff *mpdu = NULL;
    struct ampdu_hdr_st *ampdu_hdr = NULL;
 ktime_t current_ktime;
 ampdu_hdr = (struct ampdu_hdr_st *) ampdu_skb->head;
 ssn = ampdu_hdr->ssn[0];
 mpdu = INDEX_PKT_BY_SSN(ampdu_hdr->ampdu_tid, ssn);
 if (mpdu == NULL)
  return 0;
 mpdu_skb_info = (SKB_info *) (mpdu->head);
 current_ktime = ktime_get();
 timeout = (unsigned int)ktime_to_ms(ktime_sub(current_ktime, mpdu_skb_info->timestamp));
 if (timeout > SKB_DURATION_TIMEOUT_MS) {
  if (stage == SKB_DURATION_STAGE_TO_SDIO)
   printk("*a_to_sdio: %ums\n", timeout);
  else if (stage == SKB_DURATION_STAGE_TX_ENQ)
   printk("*a_to_txenqueue: %ums\n", timeout);
  else
   printk("*a_in_hwq: %ums\n", timeout);
 }
 return timeout;
}
#endif
static u8 _cal_ampdu_delm_half_crc (u8 value)
{
    u32 c32 = value, v32 = value;
    c32 ^= (v32 >> 1) | (v32 << 7);
    c32 ^= (v32 >> 2);
    if (v32 & 2)
        c32 ^= (0xC0);
    c32 ^= ((v32 << 4) & 0x30);
    return (u8) c32;
}
static u8 _cal_ampdu_delm_crc (u8 *pointer)
{
    u8 crc = 0xCF;
    crc ^= _cal_ampdu_delm_half_crc(*pointer++);
    crc = _cal_ampdu_delm_half_crc(crc) ^ _cal_ampdu_delm_half_crc(*pointer);
    return ~crc;
}
static bool ssv6200_ampdu_add_delimiter_and_crc32 (struct sk_buff *mpdu)
{
    p_AMPDU_DELIMITER delimiter_p;
    struct ieee80211_hdr *mpdu_hdr;
    int ret;
    u32 orig_mpdu_len = mpdu->len;
    u32 pad = (4 - (orig_mpdu_len % 4)) % 4;
    struct ieee80211_tx_info *info = IEEE80211_SKB_CB(mpdu);
    struct ssv_vif_priv_data *vif_priv = (struct ssv_vif_priv_data *)info->control.vif->drv_priv;
    mpdu_hdr = (struct ieee80211_hdr*) (mpdu->data);
    mpdu_hdr->duration_id = AMPDU_TX_NAV_MCS_567;
    ret = skb_padto(mpdu, mpdu->len + (AMPDU_FCS_LEN + pad));
    if (ret)
    {
        printk(KERN_ERR "Failed to extand skb for aggregation.");
        return false;
    }
    skb_put(mpdu, AMPDU_FCS_LEN + pad);
    skb_push(mpdu, AMPDU_DELIMITER_LEN);
    delimiter_p = (p_AMPDU_DELIMITER) mpdu->data;
    delimiter_p->reserved = 0;
    delimiter_p->length = orig_mpdu_len + AMPDU_FCS_LEN;
    if (vif_priv->pair_cipher == SSV_CIPHER_CCMP) {
        delimiter_p->length += CCMP_MIC_LEN;
    }
    delimiter_p->signature = AMPDU_SIGNATURE;
    delimiter_p->crc = _cal_ampdu_delm_crc((u8*) (delimiter_p));
    return true;
}
const u16 ampdu_max_transmit_length[RATE_TABLE_SIZE] =
{
    0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    4429, 8860, 13291, 17723, 26586, 35448, 39880, 44311,
    4921, 9844, 14768, 19692, 29539, 39387, 44311, 49234,
    4429, 8860, 13291, 17723, 26586, 35448, 39880, 44311
};
int ssv6200_get_ampdu_max_transmit_length(int rate_idx)
{
 return ampdu_max_transmit_length[rate_idx];
}
static void ssv6200_ampdu_hw_init (struct ieee80211_hw *hw)
{
    struct ssv_softc *sc = hw->priv;
    SSV_AMPDU_AUTO_CRC_EN(sc->sh);
}
bool _sync_ampdu_pkt_arr (struct ssv_softc *sc, struct AMPDU_TID_st *ampdu_tid
    , struct sk_buff *ampdu, bool retry)
{
    struct sk_buff **pp_aggr_pkt;
    struct sk_buff *p_aggr_pkt;
    unsigned long flags;
    struct ampdu_hdr_st *ampdu_hdr = (struct ampdu_hdr_st *) ampdu->head;
    struct sk_buff *mpdu;
    u32 first_ssn = SSV_ILLEGAL_SN;
    u32 old_aggr_pkt_num;
    u32 old_baw_head;
    u32 sync_num = skb_queue_len(&ampdu_hdr->mpdu_q);
    bool ret = true;
    spin_lock_irqsave(&ampdu_tid->pkt_array_lock, flags);
    old_baw_head = ampdu_tid->ssv_baw_head;
    old_aggr_pkt_num = ampdu_tid->aggr_pkt_num;
    ampdu_tid->mib.ampdu_mib_ampdu_counter += 1;
    ampdu_tid->mib.ampdu_mib_dist[sync_num] += 1;
    do
    {
        if (!retry)
        {
            skb_queue_walk(&ampdu_hdr->mpdu_q, mpdu) {
                if (mpdu == NULL)
                {
                    ret = false;
                    goto NoBAWin;
                }
                else
                {
                    u32 ssn = ampdu_skb_ssn(mpdu);
                    if ((ampdu_tid->ssv_baw_head + SSV_AMPDU_BA_WINDOW_SIZE) < ssn)
                        goto NoBAWin;
                    p_aggr_pkt = INDEX_PKT_BY_SSN(ampdu_tid, ssn);
                    if (p_aggr_pkt != NULL)
                    {
                        char msg[256];
                        u32 sn = ampdu_skb_ssn(mpdu);
                        struct ssv_softc *sc = AMPDU_TID_TO_SC(ampdu_tid);
                        skb_queue_walk(&ampdu_hdr->mpdu_q, mpdu)
                        {
                            sn = ampdu_skb_ssn(mpdu);
                            sprintf(msg, " %d", sn);
                        }
                        prn_aggr_err(sc, "ES %d -> %d (%s)\n",
                                     ssn, ampdu_skb_ssn(p_aggr_pkt), msg);
                        ret = false;
                        goto NoBAWin;
                    }
                }
            }
            ampdu_tid->mib.ampdu_mib_mpdu_counter += sync_num;
        }
        else
            ampdu_tid->mib.ampdu_mib_aggr_retry_counter += 1;
        skb_queue_walk(&ampdu_hdr->mpdu_q, mpdu)
        {
            u32 ssn = ampdu_skb_ssn(mpdu);
            SKB_info *mpdu_skb_info = (SKB_info *) (mpdu->head);
            if (first_ssn == SSV_ILLEGAL_SN)
                first_ssn = ssn;
            pp_aggr_pkt = &INDEX_PKT_BY_SSN(ampdu_tid, ssn);
            p_aggr_pkt = *pp_aggr_pkt;
            *pp_aggr_pkt = mpdu;
            if (!retry)
                ampdu_tid->aggr_pkt_num++;
            mpdu_skb_info->ampdu_tx_status = AMPDU_ST_AGGREGATED;
            if (ampdu_tid->ssv_baw_head == SSV_ILLEGAL_SN)
            {
                ampdu_tid->ssv_baw_head = ssn;
            }
            if ((p_aggr_pkt != NULL) && (mpdu != p_aggr_pkt)) {
                prn_aggr_err(AMPDU_TID_TO_SC(ampdu_tid),
                        "%d -> %d (H%d, N%d, Q%d)\n",
                        ssn, ampdu_skb_ssn(p_aggr_pkt), old_baw_head, old_aggr_pkt_num, sync_num);
            }
        }
    } while (0);
NoBAWin:
    spin_unlock_irqrestore(&ampdu_tid->pkt_array_lock, flags);
    #if 1
    {
    u32 page_count = (ampdu->len + SSV6200_ALLOC_RSVD);
    if (page_count & HW_MMU_PAGE_MASK)
        page_count = (page_count >> HW_MMU_PAGE_SHIFT) + 1;
    else page_count = page_count >> HW_MMU_PAGE_SHIFT;
    if (page_count > (sc->sh->tx_info.tx_page_threshold / 2))
        printk(KERN_ERR "AMPDU requires pages %d(%d-%d-%d) exceeds resource limit %d.\n",
               page_count, ampdu->len, ampdu_hdr->max_size, ampdu_hdr->size,
               (sc->sh->tx_info.tx_page_threshold / 2));
    }
    #endif
    return ret;
}
struct sk_buff* _aggr_retry_mpdu (struct ssv_softc *sc,
                                  struct AMPDU_TID_st *ampdu_tid,
                                  struct sk_buff_head *retry_queue,
                                  u32 max_aggr_len)
{
    struct sk_buff *retry_mpdu;
    struct sk_buff *new_ampdu_skb;
    u32 num_retry_mpdu;
    u32 temp_i;
    u32 total_skb_size;
    unsigned long flags;
    struct ampdu_hdr_st *ampdu_hdr;
 u16 head_ssn = ampdu_tid->ssv_baw_head;
    struct ieee80211_tx_info *info;
    struct ssv_vif_priv_data *vif_priv;
#if 0
    if (cur_AMPDU_TID->ssv_baw_head == SSV_ILLEGAL_SN)
    {
        struct sk_buff *skb = skb_peek(ampdu_skb_retry_queue);
        prn_aggr_err(sc, "Rr %d\n", (skb == NULL) ? (-1) : ampdu_skb_ssn(skb));
        return NULL;
    }
#else
    BUG_ON(head_ssn == SSV_ILLEGAL_SN);
#endif
    num_retry_mpdu = skb_queue_len(retry_queue);
    if (num_retry_mpdu == 0)
        return NULL;
    new_ampdu_skb = _alloc_ampdu_skb(sc, ampdu_tid, max_aggr_len);
    if (new_ampdu_skb == 0)
        return NULL;
    ampdu_hdr = (struct ampdu_hdr_st *)new_ampdu_skb->head;
    total_skb_size = 0;
    spin_lock_irqsave(&retry_queue->lock, flags);
    for (temp_i = 0; temp_i < ampdu_tid->agg_num_max; temp_i++)
    {
        u32 new_total_skb_size;
        retry_mpdu = skb_peek(retry_queue);
        if (retry_mpdu == NULL)
        {
            break;
        }
        info = IEEE80211_SKB_CB(retry_mpdu);
        vif_priv = (struct ssv_vif_priv_data *)info->control.vif->drv_priv;
        new_total_skb_size = total_skb_size + retry_mpdu->len;
        if (vif_priv->pair_cipher == SSV_CIPHER_CCMP) {
            new_total_skb_size += CCMP_MIC_LEN;
        }
        if (new_total_skb_size > ampdu_hdr->max_size)
            break;
        total_skb_size = new_total_skb_size;
#if 0
            if (ampdu_skb_retry_queue != NULL)
            prn_aggr_err(sc, "R %d\n", SerialNumber);
#endif
        retry_mpdu = __skb_dequeue(retry_queue);
        _put_mpdu_to_ampdu(sc, new_ampdu_skb, retry_mpdu);
        ampdu_tid->mib.ampdu_mib_retry_counter++;
    }
    ampdu_tid->mib.ampdu_mib_aggr_retry_counter += 1;
    ampdu_tid->mib.ampdu_mib_dist[temp_i] += 1;
    spin_unlock_irqrestore(&retry_queue->lock, flags);
#if 0
    {
    struct ampdu_hdr_st *ampdu_hdr = (struct ampdu_hdr_st *)new_ampdu_skb->head;
    retry_mpdu = skb_peek(&ampdu_hdr->mpdu_q);
    dev_alert(sc->dev, "rA %d - %d\n", ampdu_skb_ssn(retry_mpdu),
              skb_queue_len(&ampdu_hdr->mpdu_q));
    }
#endif
 if (ampdu_hdr->mpdu_num == 0) {
  dev_kfree_skb_any(new_ampdu_skb);
  return NULL;
 }
     return new_ampdu_skb;
}
#ifndef SSV_SUPPORT_HAL
static void _add_ampdu_txinfo (struct ssv_softc *sc, struct sk_buff *ampdu_skb)
{
    struct ssv6200_tx_desc *tx_desc;
    ssv6xxx_add_txinfo(sc, ampdu_skb);
    tx_desc = (struct ssv6200_tx_desc *) ampdu_skb->data;
    tx_desc->tx_report = 1;
#if 0
    tx_desc->len = ampdu_skb->len;
    tx_desc->c_type = M2_TXREQ;
    tx_desc->f80211 = 1;
    tx_desc->ht = 1;
    tx_desc->qos = 1;
    tx_desc->use_4addr = 0;
    tx_desc->security = 0;
    tx_desc->more_data = 0;
    tx_desc->stype_b5b4 = 0;
    tx_desc->extra_info = 0;
    tx_desc->hdr_offset = sc->sh->cfg.txpb_offset;;
    tx_desc->frag = 0;
    tx_desc->unicast = 1;
    tx_desc->hdr_len = 0;
    tx_desc->RSVD_4 = 0;
    tx_desc->tx_burst = 0;
    tx_desc->ack_policy = 1;
    tx_desc->aggregation = 1;
    tx_desc->RSVD_1 = 0;
    tx_desc->do_rts_cts = 0;
    tx_desc->reason = 0;
    tx_desc->payload_offset = tx_desc->hdr_offset + tx_desc->hdr_len;
    tx_desc->RSVD_4 = 0;
    tx_desc->RSVD_2 = 0;
    tx_desc->fCmdIdx = 0;
    tx_desc->wsid = 0;
    tx_desc->txq_idx = 0;
    tx_desc->TxF_ID = 0;
    tx_desc->rts_cts_nav = 0;
    tx_desc->frame_consume_time = 0;
    tx_desc->crate_idx=0;
    tx_desc->drate_idx = 22;
    tx_desc->dl_length = 56;
    tx_desc->RSVD_3 = 0;
#endif
#if 0
    if(ampdu_skb != 0)
    {
        u32 temp_i;
        u8* temp8_p = (u8*)ampdu_skb->data;
        ampdu_db_log("print txinfo.\n");
        for(temp_i=0; temp_i < 24; temp_i++)
        {
            ampdu_db_log_simple("%02x",*(u8*)(temp8_p + temp_i));
            if(((temp_i+1) % 4) == 0)
            {
                ampdu_db_log_simple("\n");
            }
        }
        ampdu_db_log_simple("\n");
    }
#endif
#if 0
    if(ampdu_skb != 0)
    {
        u32 temp_i;
        u8* temp8_p = (u8*)ampdu_skb->data;
        ampdu_db_log("print all skb.\n");
        for(temp_i=0; temp_i < ampdu_skb->len; temp_i++)
        {
            ampdu_db_log_simple("%02x",*(u8*)(temp8_p + temp_i));
            if(((temp_i+1) % 4) == 0)
            {
                ampdu_db_log_simple("\n");
            }
        }
        ampdu_db_log_simple("\n");
    }
#endif
}
#endif
void _send_hci_skb (struct ssv_softc *sc, struct sk_buff *skb, u32 tx_flag)
{
    u32 txq_idx;
    int ret;
    unsigned long flags;
    txq_idx = SSV_GET_TX_DESC_TXQ_IDX(sc->sh, skb);
    spin_lock_irqsave(&sc->tx_pkt_run_no_lock, flags);
    _acquire_new_ampdu_ssn_slot(sc, skb);
    spin_unlock_irqrestore(&sc->tx_pkt_run_no_lock, flags);
    ret = AMPDU_HCI_SEND(sc->sh, skb, txq_idx, tx_flag);
#if 1
    if ((txq_idx > 3) && (ret <= 0))
    {
        prn_aggr_err(sc, "BUG!! %d %d\n", txq_idx, ret);
    }
#else
    BUG_ON(txq_idx>3 && ret<=0);
#endif
}
static void ssv6200_ampdu_add_txinfo_and_send_HCI (struct ssv_softc *sc,
                                                   struct sk_buff *ampdu_skb,
                                                   u32 tx_flag)
{
    SSV_ADD_AMPDU_TXINFO(sc, ampdu_skb);
    _send_hci_skb(sc, ampdu_skb, tx_flag);
}
static void ssv6200_ampdu_change_retry_frame_rate(struct sk_buff_head *ampdu_skb_retry_queue_p)
{
    struct sk_buff *skb;
    struct ieee80211_tx_info *info;
 struct ieee80211_tx_rate *ar;
    int i;
    skb = skb_dequeue(ampdu_skb_retry_queue_p);
    info = IEEE80211_SKB_CB(skb);
    for (i = 0; i < 3; i++) {
     ar = &info->control.rates[i];
        ar->idx = 0;
        ar->count = 4;
        ar->flags = IEEE80211_TX_RC_MCS;
    }
 ar = &info->control.rates[3];
    ar->idx = -1;
    ar->count = 0;
    skb_queue_head(ampdu_skb_retry_queue_p, skb);
}
static void ssv6200_ampdu_send_retry (
        struct ieee80211_hw *hw, AMPDU_TID *cur_ampdu_tid,
        struct sk_buff_head *ampdu_skb_retry_queue_p, bool send_aggr_tx)
{
    struct ssv_softc *sc = hw->priv;
    struct sk_buff *ampdu_retry_skb, *retry_mpdu;
    u32 ampdu_skb_retry_queue_len;
    u32 max_agg_len;
    u16 lowest_rate;
    struct fw_rc_retry_params rates[SSV62XX_TX_MAX_RATES];
    SKB_info *mpdu_skb_info;
    unsigned long flags;
    ampdu_skb_retry_queue_len = skb_queue_len(ampdu_skb_retry_queue_p);
    if (ampdu_skb_retry_queue_len == 0)
        return;
    spin_lock_irqsave(&ampdu_skb_retry_queue_p->lock, flags);
    ampdu_retry_skb = skb_peek(ampdu_skb_retry_queue_p);
    lowest_rate = SSV_HT_RATE_UPDATE(sc, ampdu_retry_skb, rates);
    max_agg_len = SSV_AMPDU_MAX_TRANSMIT_LENGTH(sc, ampdu_retry_skb, lowest_rate);
    if (max_agg_len == 0) {
        ssv6200_ampdu_change_retry_frame_rate(ampdu_skb_retry_queue_p);
        ampdu_retry_skb = skb_peek(ampdu_skb_retry_queue_p);
        max_agg_len = SSV_AMPDU_MAX_TRANSMIT_LENGTH(sc, ampdu_retry_skb, 15);
    }
    spin_unlock_irqrestore(&ampdu_skb_retry_queue_p->lock, flags);
    if (max_agg_len > 0)
    {
        struct ssv_sta_priv_data *ssv_sta_priv = (struct ssv_sta_priv_data *)cur_ampdu_tid->sta->drv_priv;
        u32 cur_ampdu_max_size = ssv_sta_priv->max_ampdu_size;
        if (max_agg_len >= cur_ampdu_max_size)
            max_agg_len = cur_ampdu_max_size;
        while (ampdu_skb_retry_queue_len > 0)
        {
            spin_lock_irqsave(&ampdu_skb_retry_queue_p->lock, flags);
            retry_mpdu = skb_peek(ampdu_skb_retry_queue_p);
            mpdu_skb_info = (SKB_info *)(retry_mpdu->head);
            mpdu_skb_info->lowest_rate = lowest_rate;
            memcpy(mpdu_skb_info->rates, rates, sizeof(rates));
            spin_unlock_irqrestore(&ampdu_skb_retry_queue_p->lock, flags);
            ampdu_retry_skb = _aggr_retry_mpdu(sc, cur_ampdu_tid, ampdu_skb_retry_queue_p,
                                               max_agg_len);
            if (ampdu_retry_skb != NULL)
            {
                _sync_ampdu_pkt_arr(sc, cur_ampdu_tid, ampdu_retry_skb, true);
                ssv6200_ampdu_add_txinfo_and_send_HCI(sc, ampdu_retry_skb,
                                                      AMPDU_HCI_SEND_HEAD_WITHOUT_FLOWCTRL);
            }
            else
            {
                prn_aggr_err(sc, "AMPDU retry failed.\n");
                return;
            }
            ampdu_skb_retry_queue_len = skb_queue_len(ampdu_skb_retry_queue_p);
        }
    }
}
void ssv6200_ampdu_init (struct ieee80211_hw *hw)
{
    struct ssv_softc *sc = hw->priv;
    ssv6200_ampdu_hw_init(hw);
    sc->tx.ampdu_tx_group_id = 0;
#ifdef USE_ENCRYPT_WORK
    INIT_WORK(&sc->ampdu_tx_encry_work, encry_work);
    INIT_WORK(&sc->sync_hwkey_work, sync_hw_key_work);
#endif
}
void ssv6200_ampdu_deinit (struct ieee80211_hw *hw)
{
}
void ssv6200_ampdu_release_skb (struct sk_buff *skb, struct ieee80211_hw *hw)
{
#if LINUX_VERSION_CODE >= 0x030400
    ieee80211_free_txskb(hw, skb);
#else
    dev_kfree_skb_any(skb);
#endif
}
#ifdef CONFIG_SSV6XXX_DEBUGFS
struct mib_dump_data {
    char *prt_buff;
    size_t buff_size;
    size_t prt_len;
};
#define AMPDU_TX_MIB_SUMMARY_BUF_SIZE (4096)
static ssize_t ampdu_tx_mib_summary_read (struct file *file,
                                          char __user *user_buf, size_t count,
                                          loff_t *ppos)
{
    struct ssv_sta_priv_data *ssv_sta_priv =
            (struct ssv_sta_priv_data *) file->private_data;
    char *summary_buf = kzalloc(AMPDU_TX_MIB_SUMMARY_BUF_SIZE, GFP_KERNEL);
    ssize_t summary_size;
    ssize_t ret;
    if (!summary_buf)
        return -ENOMEM;
    summary_size = ampdu_tx_mib_dump(ssv_sta_priv, summary_buf,
                                     AMPDU_TX_MIB_SUMMARY_BUF_SIZE);
    ret = simple_read_from_buffer(user_buf, count, ppos, summary_buf,
                                  summary_size);
    kfree(summary_buf);
    return ret;
}
static int ampdu_tx_mib_summary_open (struct inode *inode, struct file *file)
{
    file->private_data = inode->i_private;
    return 0;
}
static const struct file_operations mib_summary_fops = { .read =
        ampdu_tx_mib_summary_read, .open = ampdu_tx_mib_summary_open, };
static ssize_t ampdu_tx_tid_window_read (struct file *file,
                                          char __user *user_buf, size_t count,
                                          loff_t *ppos)
{
    struct AMPDU_TID_st *ampdu_tid = (struct AMPDU_TID_st *)file->private_data;
    char *summary_buf = kzalloc(AMPDU_TX_MIB_SUMMARY_BUF_SIZE, GFP_KERNEL);
    ssize_t ret;
    char *prn_ptr = summary_buf;
    int prt_size;
    int buf_size = AMPDU_TX_MIB_SUMMARY_BUF_SIZE;
    int i;
    struct sk_buff *ba_skb, *tmp_ba_skb;
    struct ssv_softc *sc;
    if (!summary_buf)
        return -ENOMEM;
    sc = AMPDU_TID_TO_SC(ampdu_tid);
    prt_size = snprintf(prn_ptr, buf_size, "\nWMM_TID %d:\n"
                        "\tWindow:",
                        ampdu_tid->tidno);
    prn_ptr += prt_size;
    buf_size -= prt_size;
    for (i = 0; i < SSV_AMPDU_BA_WINDOW_SIZE; i++)
    {
        struct sk_buff *skb = ampdu_tid->aggr_pkts[i];
        if ((i % 8) == 0)
        {
            prt_size = snprintf(prn_ptr, buf_size, "\n\t\t");
            prn_ptr += prt_size;
            buf_size -= prt_size;
        }
        if (skb == NULL)
            prt_size = snprintf(prn_ptr, buf_size, " %s", "NULL ");
        else
        {
            struct SKB_info_st *skb_info = (struct SKB_info_st *)(skb->head);
            const char status_symbol[] = {'N',
                                          'A',
                                          'S',
                                          'R',
                                          'P',
                                          'D'};
            prt_size = snprintf(prn_ptr, buf_size, " %4d%c", ampdu_skb_ssn(skb),
                                ( (skb_info->ampdu_tx_status <= AMPDU_ST_DONE)
                                 ? status_symbol[skb_info->ampdu_tx_status]
                                 : 'X'));
        }
        prn_ptr += prt_size;
        buf_size -= prt_size;
    }
    prt_size = snprintf(prn_ptr, buf_size, "\n\tEarly aggregated #: %d\n", ampdu_tid->early_aggr_skb_num);
    prn_ptr += prt_size;
    buf_size -= prt_size;
    prt_size = snprintf(prn_ptr, buf_size, "\tBAW skb #: %d\n", ampdu_tid->aggr_pkt_num);
    prn_ptr += prt_size;
    buf_size -= prt_size;
    prt_size = snprintf(prn_ptr, buf_size, "\tBAW head: %d\n", ampdu_tid->ssv_baw_head);
    prn_ptr += prt_size;
    buf_size -= prt_size;
    prt_size = snprintf(prn_ptr, buf_size, "\tState: %d\n", ampdu_tid->state);
    prn_ptr += prt_size;
    buf_size -= prt_size;
    prt_size = snprintf(prn_ptr, buf_size, "\tBA:\n");
    prn_ptr += prt_size;
    buf_size -= prt_size;
    skb_queue_walk_safe(&ampdu_tid->ba_q, ba_skb, tmp_ba_skb)
    {
        prt_size = _dump_ba_skb(sc, prn_ptr, buf_size, ba_skb);
        prn_ptr += prt_size;
        buf_size -= prt_size;
    }
    buf_size = AMPDU_TX_MIB_SUMMARY_BUF_SIZE - buf_size;
    ret = simple_read_from_buffer(user_buf, count, ppos, summary_buf,
                                  buf_size);
    kfree(summary_buf);
    return ret;
}
static int ampdu_tx_tid_window_open (struct inode *inode, struct file *file)
{
    file->private_data = inode->i_private;
    return 0;
}
static const struct file_operations tid_window_fops = { .read =
        ampdu_tx_tid_window_read, .open = ampdu_tx_tid_window_open, };
static int ampdu_tx_mib_reset_open (struct inode *inode, struct file *file)
{
    file->private_data = inode->i_private;
    return 0;
}
static ssize_t ampdu_tx_mib_reset_read (struct file *file,
                                        char __user *user_buf, size_t count,
                                        loff_t *ppos)
{
    char *reset_buf = kzalloc(64, GFP_KERNEL);
    ssize_t ret;
    u32 reset_size;
    if (!reset_buf)
        return -ENOMEM;
    reset_size = snprintf(reset_buf, 63, "%d", 0);
    ret = simple_read_from_buffer(user_buf, count, ppos, reset_buf,
                                  reset_size);
    kfree(reset_buf);
    return ret;
}
static ssize_t ampdu_tx_mib_reset_write (struct file *file,
                                         const char __user *buffer,
                                         size_t count,
                                         loff_t *pos)
{
    struct AMPDU_TID_st *ampdu_tid = (struct AMPDU_TID_st *)file->private_data;
    memset(&ampdu_tid->mib, 0, sizeof(struct AMPDU_MIB_st));
    return count;
}
static const struct file_operations mib_reset_fops
    = { .read = ampdu_tx_mib_reset_read,
        .open = ampdu_tx_mib_reset_open,
        .write = ampdu_tx_mib_reset_write};
static void ssv6200_ampdu_tx_init_debugfs (
        struct ssv_softc *sc, struct ssv_sta_priv_data *ssv_sta_priv)
{
    struct ssv_sta_info *sta_info = ssv_sta_priv->sta_info;
    int i;
    struct dentry *sta_debugfs_dir = sta_info->debugfs_dir;
    dev_info(sc->dev, "Creating AMPDU TX debugfs.\n");
    if (sta_debugfs_dir == NULL)
    {
        dev_err(sc->dev, "No STA debugfs.\n");
        return;
    }
    debugfs_create_file("ampdu_tx_summary", 00444, sta_debugfs_dir,
                        ssv_sta_priv, &mib_summary_fops);
    debugfs_create_u32("total_BA", 00644, sta_debugfs_dir,
                       &ssv_sta_priv->ampdu_mib_total_BA_counter);
    for (i = 0; i < WMM_TID_NUM; i++)
    {
        char debugfs_name[20];
        struct dentry *ampdu_tx_debugfs_dir;
        int j;
        struct AMPDU_TID_st *ampdu_tid = &ssv_sta_priv->ampdu_tid[i];
        struct AMPDU_MIB_st *ampdu_mib = &ampdu_tid->mib;
        snprintf(debugfs_name, sizeof(debugfs_name), "ampdu_tx_%d", i);
        ampdu_tx_debugfs_dir = debugfs_create_dir(debugfs_name,
                                                  sta_debugfs_dir);
        if (ampdu_tx_debugfs_dir == NULL)
        {
            dev_err(sc->dev,
                    "Failed to create debugfs for AMPDU TX TID %d: %s\n", i,
                    debugfs_name);
            continue;
        }
        ssv_sta_priv->ampdu_tid[i].debugfs_dir = ampdu_tx_debugfs_dir;
        debugfs_create_file("baw_status", 00444, ampdu_tx_debugfs_dir,
                            ampdu_tid, &tid_window_fops);
        debugfs_create_file("reset", 00644, ampdu_tx_debugfs_dir,
                            ampdu_tid, &mib_reset_fops);
        debugfs_create_u32("total", 00444, ampdu_tx_debugfs_dir,
                           &ampdu_mib->ampdu_mib_ampdu_counter);
        debugfs_create_u32("retry", 00444, ampdu_tx_debugfs_dir,
                           &ampdu_mib->ampdu_mib_retry_counter);
        debugfs_create_u32("aggr_retry", 00444, ampdu_tx_debugfs_dir,
                           &ampdu_mib->ampdu_mib_aggr_retry_counter);
        debugfs_create_u32("BAR", 00444, ampdu_tx_debugfs_dir,
                           &ampdu_mib->ampdu_mib_bar_counter);
        debugfs_create_u32("Discarded", 00444, ampdu_tx_debugfs_dir,
                           &ampdu_mib->ampdu_mib_discard_counter);
        debugfs_create_u32("BA", 00444, ampdu_tx_debugfs_dir,
                           &ampdu_mib->ampdu_mib_BA_counter);
        debugfs_create_u32("Pass", 00444, ampdu_tx_debugfs_dir,
                           &ampdu_mib->ampdu_mib_pass_counter);
        for (j = 0; j <= SSV_AMPDU_aggr_num_max; j++)
        {
            char dist_dbg_name[10];
            snprintf(dist_dbg_name, sizeof(dist_dbg_name), "aggr_%d", j);
            debugfs_create_u32(dist_dbg_name, 00444, ampdu_tx_debugfs_dir,
                               &ampdu_mib->ampdu_mib_dist[j]);
        }
        skb_queue_head_init(&ssv_sta_priv->ampdu_tid[i].ba_q);
    }
}
#endif
void ssv6200_ampdu_tx_add_sta (struct ieee80211_hw *hw,
                               struct ieee80211_sta *sta)
{
    struct ssv_sta_priv_data *ssv_sta_priv;
    struct ssv_softc *sc;
    u32 temp_i;
    ssv_sta_priv = (struct ssv_sta_priv_data *) sta->drv_priv;
    sc = (struct ssv_softc *) hw->priv;
    for (temp_i = 0; temp_i < WMM_TID_NUM; temp_i++)
    {
        ssv_sta_priv->ampdu_tid[temp_i].sta = sta;
        ssv_sta_priv->ampdu_tid[temp_i].state = AMPDU_STATE_STOP;
        spin_lock_init(
                &ssv_sta_priv->ampdu_tid[temp_i].ampdu_skb_tx_queue_lock);
        spin_lock_init(&ssv_sta_priv->ampdu_tid[temp_i].pkt_array_lock);
    }
#ifdef CONFIG_SSV6XXX_DEBUGFS
    ssv6200_ampdu_tx_init_debugfs(sc, ssv_sta_priv);
#endif
}
int ssv6200_ampdu_rx_start(struct ieee80211_hw *hw, struct ieee80211_vif *vif, struct ieee80211_sta *sta,
        u16 tid, u16 *ssn, u8 buf_size)
{
    struct ssv_softc *sc = hw->priv;
#ifdef WIFI_CERTIFIED
    if (sc->rx_ba_session_count >= sc->sh->rx_info.rx_ba_ma_sessions) {
#if LINUX_VERSION_CODE > KERNEL_VERSION(3,1,0)
        ieee80211_stop_rx_ba_session(vif, (1<<(sc->ba_tid)), sc->ba_ra_addr);
#endif
        sc->rx_ba_session_count--;
    }
#else
    if ((sc->rx_ba_session_count >= sc->sh->rx_info.rx_ba_ma_sessions) && (sc->rx_ba_sta != sta)) {
        return -EBUSY;
    }
    else if ((sc->rx_ba_session_count >= sc->sh->rx_info.rx_ba_ma_sessions) && (sc->rx_ba_sta == sta)) {
#if LINUX_VERSION_CODE > KERNEL_VERSION(3,1,0)
        ieee80211_stop_rx_ba_session(vif,(1<<(sc->ba_tid)),sc->ba_ra_addr);
#endif
        sc->rx_ba_session_count--;
    }
#endif
    printk(KERN_ERR "IEEE80211_AMPDU_RX_START %02X:%02X:%02X:%02X:%02X:%02X %d.\n",
        sta->addr[0], sta->addr[1], sta->addr[2], sta->addr[3],
        sta->addr[4], sta->addr[5], tid);
    sc->rx_ba_session_count++;
    sc->rx_ba_sta = sta;
    sc->ba_tid = tid;
    sc->ba_ssn = *ssn;
    memcpy(sc->ba_ra_addr, sta->addr, ETH_ALEN);
    return 0;
}
void ssv6200_ampdu_tx_start (u16 tid, struct ieee80211_sta *sta,
                             struct ieee80211_hw *hw, u16 *ssn)
{
    struct ssv_softc *sc = hw->priv;
    struct ssv_sta_priv_data *ssv_sta_priv;
    struct AMPDU_TID_st *ampdu_tid;
    int i;
    ssv_sta_priv = (struct ssv_sta_priv_data *) sta->drv_priv;
    ampdu_tid = &ssv_sta_priv->ampdu_tid[tid];
    if (ampdu_tid->state == AMPDU_STATE_START)
        return;
    ampdu_tid->ssv_baw_head = SSV_ILLEGAL_SN;
#if 0
    if (list_empty(&sc->tx.ampdu_tx_que))
    {
        sc->ampdu_rekey_pause = AMPDU_REKEY_PAUSE_STOP;
    }
#endif
#ifdef DEBUG_AMPDU_FLUSH
    printk(KERN_ERR "Adding %02X-%02X-%02X-%02X-%02X-%02X TID %d (%p).\n",
            sta->addr[0], sta->addr[1], sta->addr[2],
            sta->addr[3], sta->addr[4], sta->addr[5],
            ampdu_tid->tidno, ampdu_tid);
    {
        int j;
        for (j = 0; j <= MAX_TID; j++)
        {
            if (sc->tid[j] == 0)
                break;
        }
        if (j == MAX_TID)
        {
            printk(KERN_ERR "No room for new TID.\n");
        }
        else
            sc->tid[j] = ampdu_tid;
}
#endif
    list_add_tail_rcu(&ampdu_tid->list, &sc->tx.ampdu_tx_que);
    skb_queue_head_init(&ampdu_tid->ampdu_skb_tx_queue);
#ifdef ENABLE_INCREMENTAL_AGGREGATION
    skb_queue_head_init(&ampdu_tid->early_aggr_ampdu_q);
    ampdu_tid->early_aggr_skb_num = 0;
#endif
    skb_queue_head_init(&ampdu_tid->ampdu_skb_wait_encry_queue);
    skb_queue_head_init(&ampdu_tid->retry_queue);
    skb_queue_head_init(&ampdu_tid->release_queue);
    for (i = 0;
            i < (sizeof(ampdu_tid->aggr_pkts) / sizeof(ampdu_tid->aggr_pkts[0]));
            i++)
        ampdu_tid->aggr_pkts[i] = 0;
    ampdu_tid->aggr_pkt_num = 0;
#ifdef ENABLE_AGGREGATE_IN_TIME
    ampdu_tid->cur_ampdu_pkt = _alloc_ampdu_skb(sc, ampdu_tid, 0);
#endif
#ifdef AMPDU_CHECK_SKB_SEQNO
    ssv_sta_priv->ampdu_tid[tid].last_seqno = (-1);
#endif
    ssv_sta_priv->ampdu_mib_total_BA_counter = 0;
    memset(&ssv_sta_priv->ampdu_tid[tid].mib, 0, sizeof(struct AMPDU_MIB_st));
    ssv_sta_priv->ampdu_tid[tid].state = AMPDU_STATE_START;
    #ifdef CONFIG_SSV6XXX_DEBUGFS
    skb_queue_head_init(&ssv_sta_priv->ampdu_tid[tid].ba_q);
    #endif
}
void ssv6200_ampdu_tx_operation (u16 tid, struct ieee80211_sta *sta,
                                 struct ieee80211_hw *hw, u8 buffer_size)
{
    struct ssv_sta_priv_data *ssv_sta_priv;
    ssv_sta_priv = (struct ssv_sta_priv_data *) sta->drv_priv;
    ssv_sta_priv->ampdu_tid[tid].tidno = tid;
    ssv_sta_priv->ampdu_tid[tid].sta = sta;
    ssv_sta_priv->ampdu_tid[tid].agg_num_max = MAX_AGGR_NUM;
#if 1
    if (buffer_size > IEEE80211_MAX_AMPDU_BUF)
    {
        buffer_size = IEEE80211_MAX_AMPDU_BUF;
    }
    printk("ssv6200_ampdu_tx_operation:buffer_size=%d\n", buffer_size);
    ssv_sta_priv->ampdu_tid[tid].ssv_baw_size = SSV_AMPDU_WINDOW_SIZE;
#else
    ssv_sta_priv->ampdu_tid[tid].ssv_baw_size = IEEE80211_MIN_AMPDU_BUF << sta->ht_cap.ampdu_factor;
    if(buffer_size > IEEE80211_MAX_AMPDU_BUF)
    {
        buffer_size = IEEE80211_MAX_AMPDU_BUF;
    }
    if(ssv_sta_priv->ampdu_tid[tid].ssv_baw_size > buffer_size)
    {
        ssv_sta_priv->ampdu_tid[tid].ssv_baw_size = buffer_size;
    }
#endif
    ssv_sta_priv->ampdu_tid[tid].state = AMPDU_STATE_OPERATION;
}
static void _clear_mpdu_q (struct ieee80211_hw *hw, struct sk_buff_head *q,
                           bool aggregated_mpdu)
{
    struct ssv_softc *sc = hw->priv;
    struct sk_buff *skb;
    struct SKB_info_st *mpdu_skb_info_p = NULL;
    while (1)
    {
        skb = skb_dequeue(q);
        if (!skb)
            break;
        mpdu_skb_info_p = (SKB_info *) (skb->head);
        if (aggregated_mpdu)
            skb_pull(skb, AMPDU_DELIMITER_LEN);
        if (mpdu_skb_info_p->directly_ack)
            dev_kfree_skb_any(skb);
        else {
            ieee80211_tx_status(hw, skb);
  }
        atomic_dec(&sc->ampdu_tx_frame);
    }
}
void ssv6200_ampdu_tx_stop (u16 tid, struct ieee80211_sta *sta,
                            struct ieee80211_hw *hw)
{
    struct ssv_softc *sc = hw->priv;
    struct ssv_sta_priv_data *ssv_sta_priv;
    struct sk_buff *ampdu_skb;
    struct ampdu_hdr_st *ampdu_hdr;
    struct SKB_info_st *skb_info;
    int i = 0;
    bool collect_ba_window_frame = false;
    ssv_sta_priv = (struct ssv_sta_priv_data *) sta->drv_priv;
    if (ssv_sta_priv->ampdu_tid[tid].state == AMPDU_STATE_STOP)
        return;
    ssv_sta_priv->ampdu_tid[tid].state = AMPDU_STATE_STOP;
    printk("ssv6200_ampdu_tx_stop\n");
    if (!list_empty(&sc->tx.ampdu_tx_que))
    {
#ifdef DEBUG_AMPDU_FLUSH
        {
            int j;
            struct AMPDU_TID_st *ampdu_tid = &ssv_sta_priv->ampdu_tid[tid];
            for (j = 0; j <= MAX_TID; j++)
            {
                if (sc->tid[j] == ampdu_tid)
                    break;
            }
            if (j == MAX_TID)
            {
                printk(KERN_ERR "No TID found when deleting it.\n");
            }
            else
                sc->tid[j] = NULL;
            printk(KERN_ERR "Deleting %02X-%02X-%02X-%02X-%02X-%02X TID %d (%p).\n",
                   sta->addr[0], sta->addr[1], sta->addr[2],
                   sta->addr[3], sta->addr[4], sta->addr[5],
                   ampdu_tid->tidno, ampdu_tid);
        }
#endif
        list_del_rcu(&ssv_sta_priv->ampdu_tid[tid].list);
    }
    for (i = 0; i < SSV_AMPDU_BA_WINDOW_SIZE; i++) {
        struct sk_buff *skb = ssv_sta_priv->ampdu_tid[tid].aggr_pkts[i];
        if (skb != NULL) {
            skb_info = (struct SKB_info_st *) (skb->head);
            skb_info->mpdu_retry_counter = SSV_AMPDU_retry_counter_max;
            if(skb_info->ampdu_tx_status != AMPDU_ST_RETRY_Q){
            	skb_info->ampdu_tx_status = AMPDU_ST_DROPPED;
			}
            collect_ba_window_frame = true;
        }
    }
    if (collect_ba_window_frame) {
        printk("collect ba sindow frame to release queue\n");
        ssv6xxx_release_frames(&ssv_sta_priv->ampdu_tid[tid]);
    }
    printk("clear tx q len=%d\n",
           skb_queue_len(&ssv_sta_priv->ampdu_tid[tid].ampdu_skb_tx_queue));
    _clear_mpdu_q(sc->hw, &ssv_sta_priv->ampdu_tid[tid].ampdu_skb_tx_queue,
                  true);
    printk("clear retry q len=%d\n",
           skb_queue_len(&ssv_sta_priv->ampdu_tid[tid].retry_queue));
    _clear_mpdu_q(sc->hw, &ssv_sta_priv->ampdu_tid[tid].retry_queue, true);
#ifdef USE_ENCRYPT_WORK
    printk("clear encrypt q len=%d\n",skb_queue_len(&ssv_sta_priv->ampdu_tid[tid].ampdu_skb_wait_encry_queue));
    _clear_mpdu_q(sc->hw, &ssv_sta_priv->ampdu_tid[tid].ampdu_skb_wait_encry_queue, false);
#endif
    printk("clear release q len=%d\n",
           skb_queue_len(&ssv_sta_priv->ampdu_tid[tid].release_queue));
    _clear_mpdu_q(sc->hw, &ssv_sta_priv->ampdu_tid[tid].release_queue, true);
    printk("clear early aggr ampdu q len=%d\n",
           skb_queue_len(&ssv_sta_priv->ampdu_tid[tid].early_aggr_ampdu_q));
    while (skb_queue_len(&ssv_sta_priv->ampdu_tid[tid].early_aggr_ampdu_q)) {
        ampdu_skb = skb_dequeue(&ssv_sta_priv->ampdu_tid[tid].early_aggr_ampdu_q);
        ampdu_hdr = (struct ampdu_hdr_st *) ampdu_skb->head;
        _clear_mpdu_q(sc->hw, &ampdu_hdr->mpdu_q, true);
     ssv6200_ampdu_release_skb(ampdu_skb, sc->hw);
    }
#ifdef ENABLE_AGGREGATE_IN_TIME
    if (ssv_sta_priv->ampdu_tid[tid].cur_ampdu_pkt != NULL)
    {
        dev_kfree_skb_any(ssv_sta_priv->ampdu_tid[tid].cur_ampdu_pkt);
        ssv_sta_priv->ampdu_tid[tid].cur_ampdu_pkt = NULL;
    }
#endif
    ssv6200_tx_flow_control((void *) sc,
                            sc->tx.hw_txqid[ssv_sta_priv->ampdu_tid[tid].ac],
                            false, 1000);
}
void ssv6xxx_ampdu2mpdu(struct ieee80211_hw *hw, struct ieee80211_vif *vif, struct sk_buff *skb)
{
    struct ssv_softc *sc = hw->priv;
    struct ieee80211_hdr *hdr = (struct ieee80211_hdr *)skb->data;
    struct SKB_info_st *skb_info = (struct SKB_info_st *)skb->head;
    struct ieee80211_sta *sta = skb_info->sta;
    struct ssv_sta_priv_data *ssv_sta_priv = (struct ssv_sta_priv_data *) sta->drv_priv;
    struct AMPDU_TID_st *ampdu_tid;
    u16 tidno = 0;
    if (!sta)
        return;
    if (!SSV_RC_HT_STA_CURRENT_RATE_IS_CCK(sc, sta))
        return;
    ssv6xxx_ampdu_flush(sc->hw);
    if (!ieee80211_is_data(hdr->frame_control))
        return;
    tidno = ieee80211_get_qos_ctl(hdr)[0] & IEEE80211_QOS_CTL_TID_MASK;
    ampdu_tid = &ssv_sta_priv->ampdu_tid[tidno];
    if (ampdu_tid->state == AMPDU_STATE_STOP)
        return;
    if ((skb_queue_len(&ampdu_tid->early_aggr_ampdu_q) != 0) || (skb_queue_len(&ampdu_tid->retry_queue) != 0))
        return;
    if (ampdu_tid->aggr_pkt_num != 0)
        return;
    ieee80211_stop_tx_ba_session(sta, tidno);
}
static void ssv6200_ampdu_tx_state_stop_func (
        struct ssv_softc *sc, struct ieee80211_sta *sta, struct sk_buff *skb,
        struct AMPDU_TID_st *cur_AMPDU_TID)
{
    struct ieee80211_hdr *hdr = (struct ieee80211_hdr *) skb->data;
    u8 *skb_qos_ctl = ieee80211_get_qos_ctl(hdr);
    u8 tid_no = skb_qos_ctl[0] & 0xf;
    if ((sta->ht_cap.ht_supported == true)
        && (!!(sc->sh->cfg.hw_caps & SSV6200_HW_CAP_AMPDU_TX)))
    {
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,32)
        ieee80211_start_tx_ba_session(sc->hw, (u8*)(sta->addr), (u16)tid_no);
#elif LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,37)
        ieee80211_start_tx_ba_session(sta, tid_no);
#else
        ieee80211_start_tx_ba_session(sta, tid_no, 0);
#endif
        ampdu_db_log("start ampdu_tx(rc) : tid_no = %d\n", tid_no);
    }
}
static void ssv6200_ampdu_tx_state_operation_func (
        struct ssv_softc *sc, struct ieee80211_sta *sta, struct sk_buff *skb,
        struct AMPDU_TID_st *cur_AMPDU_TID)
{
#if 0
    else if (sc->ampdu_rekey_pause == AMPDU_REKEY_PAUSE_ONGOING)
    {
        pause_ampdu = true;
        printk("!!!ampdu_rekey_pause\n");
    }
#endif
}
void ssv6200_ampdu_tx_update_state (void *priv, struct ieee80211_sta *sta,
                                    struct sk_buff *skb)
{
    struct ssv_softc *sc = (struct ssv_softc *) priv;
    struct ssv_sta_priv_data *ssv_sta_priv =
            (struct ssv_sta_priv_data *) sta->drv_priv;
    struct ieee80211_hdr *hdr = (struct ieee80211_hdr *) skb->data;
    u8 *skb_qos_ctl;
    u8 tid_no;
    {
        skb_qos_ctl = ieee80211_get_qos_ctl(hdr);
        tid_no = skb_qos_ctl[0] & 0xf;
        switch (ssv_sta_priv->ampdu_tid[tid_no].state)
        {
            case AMPDU_STATE_STOP:
                ssv6200_ampdu_tx_state_stop_func(
                        sc, sta, skb, &(ssv_sta_priv->ampdu_tid[tid_no]));
                break;
            case AMPDU_STATE_START:
                break;
            case AMPDU_STATE_OPERATION:
                ssv6200_ampdu_tx_state_operation_func(
                        sc, sta, skb, &(ssv_sta_priv->ampdu_tid[tid_no]));
                break;
            default:
                break;
        }
    }
}
static int _acquire_new_ampdu_ssn_slot(struct ssv_softc *sc, struct sk_buff *ampdu)
{
    struct ssv_sta_priv_data *ssv_sta_priv;
    int i, find_ampdu_slot = -1;
    struct ampdu_hdr_st *ampdu_hdr = (struct ampdu_hdr_st *) ampdu->head;
    struct ieee80211_hdr *hdr = (struct ieee80211_hdr *)(ampdu->data + TXPB_OFFSET + AMPDU_DELIMITER_LEN);
    struct ieee80211_sta *sta = NULL;
    int j = 0, k;
    struct AMPDU_TID_st *ampdu_tid;
    sc->tx_pkt_run_no ++;
    sc->tx_pkt_run_no &= SSV6XXX_PKT_RUN_TYPE_AMPDU_END;
    if (sc->tx_pkt_run_no == SSV6XXX_PKT_RUN_TYPE_NOTUSED)
        sc->tx_pkt_run_no ++;
    SSV_UPDATE_AMPDU_TXINFO(sc, ampdu);
    sta = ampdu_hdr->ampdu_tid->sta;
    ssv_sta_priv = (struct ssv_sta_priv_data *)sta->drv_priv;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_AMPDU_SSN,
        "%s: current tx_pkt_run_no %d\n", __func__, sc->tx_pkt_run_no);
 for (i = 0; i < MAX_CONCUR_AMPDU; i++) {
        if (ssv_sta_priv->ampdu_ssn[i].tx_pkt_run_no == sc->tx_pkt_run_no) {
   dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_AMPDU_SSN,
                "Find the duplicated tx_pkt_run_no[%d]:ssn at slot %d", sc->tx_pkt_run_no, i);
   ampdu_tid = &(ssv_sta_priv->ampdu_tid[ssv_sta_priv->ampdu_ssn[i].tid_no]);
   for (k = 0; k < ssv_sta_priv->ampdu_ssn[i].mpdu_num; k++) {
    u16 ssn = ssv_sta_priv->ampdu_ssn[i].ssn[k];
    struct sk_buff *mpdu;
    u32 skb_ssn;
    struct SKB_info_st *skb_info;
                dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_AMPDU_SSN," %d", ssn);
    mpdu = INDEX_PKT_BY_SSN(ampdu_tid, ssn);
    skb_ssn = (mpdu == NULL) ? (-1) : ampdu_skb_ssn(mpdu);
    if (skb_ssn != ssn)
     continue;
    skb_info = (struct SKB_info_st *) (mpdu->head);
    skb_info->ampdu_tx_status = AMPDU_ST_RETRY;
   }
   dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_AMPDU_SSN,"\n current slot content");
            for (j =0 ;j < MAX_CONCUR_AMPDU; j++){
                if (j %16 == 0)
                    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_AMPDU_SSN,"\n");
                dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_AMPDU_SSN,"%d: %d ", j, ssv_sta_priv->ampdu_ssn[j].tx_pkt_run_no);
            }
            dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_AMPDU_SSN,"\n");
   memset((void*)&ssv_sta_priv->ampdu_ssn[i], 0, sizeof(struct aggr_ssn));
  }
  if ((find_ampdu_slot == -1) && (ssv_sta_priv->ampdu_ssn[i].tx_pkt_run_no == SSV6XXX_PKT_RUN_TYPE_NOTUSED)) {
   find_ampdu_slot = i;
  }
 }
 if (find_ampdu_slot != -1) {
     dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_AMPDU_SSN,
         "%s: tx_pkt_run_no %d: slot %d ssn", __func__, sc->tx_pkt_run_no, find_ampdu_slot);
        for (k = 0; k < ampdu_hdr->mpdu_num; k++){
            ssv_sta_priv->ampdu_ssn[find_ampdu_slot].ssn[k] = ampdu_hdr->ssn[k];
            dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_AMPDU_SSN," %d,", ampdu_hdr->ssn[k]);
        }
        dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_AMPDU_SSN,"\n");
        ssv_sta_priv->ampdu_ssn[find_ampdu_slot].tx_pkt_run_no = sc->tx_pkt_run_no;
        ssv_sta_priv->ampdu_ssn[find_ampdu_slot].mpdu_num = ampdu_hdr->mpdu_num;
  ssv_sta_priv->ampdu_ssn[find_ampdu_slot].tid_no = ieee80211_get_qos_ctl(hdr)[0] & IEEE80211_QOS_CTL_TID_MASK;
 }
    return find_ampdu_slot;
}
void _put_mpdu_to_ampdu (struct ssv_softc *sc, struct sk_buff *ampdu, struct sk_buff *mpdu)
{
    bool is_empty_ampdu = (ampdu->len == 0);
    unsigned char *data_dest;
    struct ampdu_hdr_st *ampdu_hdr = (struct ampdu_hdr_st *) ampdu->head;
    struct ieee80211_tx_info *info = IEEE80211_SKB_CB(mpdu);
    struct ssv_vif_priv_data *vif_priv = (struct ssv_vif_priv_data *)info->control.vif->drv_priv;
    BUG_ON(skb_tailroom(ampdu) < mpdu->len);
    data_dest = skb_tail_pointer(ampdu);
    skb_put(ampdu, mpdu->len);
    if (vif_priv->pair_cipher == SSV_CIPHER_CCMP) {
       skb_put(ampdu, CCMP_MIC_LEN);
    }
    if (is_empty_ampdu)
    {
        struct ieee80211_tx_info *ampdu_info = IEEE80211_SKB_CB(ampdu);
        struct ieee80211_tx_info *mpdu_info = IEEE80211_SKB_CB(mpdu);
        SKB_info *mpdu_skb_info = (SKB_info *)(mpdu->head);
        u32 max_size_for_rate = SSV_AMPDU_MAX_TRANSMIT_LENGTH(sc, mpdu, mpdu_skb_info->lowest_rate);
        BUG_ON(max_size_for_rate == 0);
        memcpy(ampdu_info, mpdu_info, sizeof(struct ieee80211_tx_info));
        skb_set_queue_mapping(ampdu, skb_get_queue_mapping(mpdu));
        ampdu_hdr->first_sn = ampdu_skb_ssn(mpdu);
        ampdu_hdr->sta = ((struct SKB_info_st *)mpdu->head)->sta;
        if (ampdu_hdr->max_size > max_size_for_rate)
            ampdu_hdr->max_size = max_size_for_rate;
        memcpy(ampdu_hdr->rates, mpdu_skb_info->rates, sizeof(ampdu_hdr->rates));
    }
    memcpy(data_dest, mpdu->data, mpdu->len);
    __skb_queue_tail(&ampdu_hdr->mpdu_q, mpdu);
    ampdu_hdr->ssn[ampdu_hdr->mpdu_num++] = ampdu_skb_ssn(mpdu);
    ampdu_hdr->size += mpdu->len;
    if (vif_priv->pair_cipher == SSV_CIPHER_CCMP) {
       ampdu_hdr->size += CCMP_MIC_LEN;
    }
    BUG_ON(ampdu_hdr->size > ampdu_hdr->max_size);
}
u32 _flush_early_ampdu_q (struct ssv_softc *sc, struct AMPDU_TID_st *ampdu_tid)
{
    u32 flushed_ampdu = 0;
    unsigned long flags;
    struct sk_buff_head *early_aggr_ampdu_q = &ampdu_tid->early_aggr_ampdu_q;
    spin_lock_irqsave(&early_aggr_ampdu_q->lock, flags);
    while (skb_queue_len(early_aggr_ampdu_q))
    {
        struct sk_buff *head_ampdu;
        struct ampdu_hdr_st *head_ampdu_hdr;
        u32 ampdu_aggr_num;
        head_ampdu = skb_peek(early_aggr_ampdu_q);
        head_ampdu_hdr = (struct ampdu_hdr_st *) head_ampdu->head;
        ampdu_aggr_num = skb_queue_len(&head_ampdu_hdr->mpdu_q);
  if (ampdu_aggr_num == 0) {
            head_ampdu = __skb_dequeue(early_aggr_ampdu_q);
   ssv6200_ampdu_release_skb(head_ampdu, sc->hw);
   continue;
  }
        if ((SSV_AMPDU_BA_WINDOW_SIZE - ampdu_tid->aggr_pkt_num)
            < ampdu_aggr_num)
            break;
        if (_sync_ampdu_pkt_arr(sc, ampdu_tid, head_ampdu, false))
        {
            head_ampdu = __skb_dequeue(early_aggr_ampdu_q);
            ampdu_tid->early_aggr_skb_num -= ampdu_aggr_num;
#ifdef SSV_AMPDU_FLOW_CONTROL
            if (ampdu_tid->early_aggr_skb_num
                <= SSV_AMPDU_FLOW_CONTROL_LOWER_BOUND)
            {
                ssv6200_tx_flow_control((void *) sc,
                                        sc->tx.hw_txqid[ampdu_tid->ac], false, 1000);
            }
#endif
            if ((skb_queue_len(early_aggr_ampdu_q) == 0)
                && (ampdu_tid->early_aggr_skb_num > 0))
            {
                printk(KERN_ERR "Empty early Q w. %d.\n", ampdu_tid->early_aggr_skb_num);
            }
            spin_unlock_irqrestore(&early_aggr_ampdu_q->lock, flags);
            _send_hci_skb(sc, head_ampdu,
                          AMPDU_HCI_SEND_TAIL_WITHOUT_FLOWCTRL);
            spin_lock_irqsave(&early_aggr_ampdu_q->lock, flags);
            flushed_ampdu++;
        }
        else
            break;
    }
    spin_unlock_irqrestore(&early_aggr_ampdu_q->lock, flags);
    return flushed_ampdu;
}
void _aggr_ampdu_tx_q (struct ieee80211_hw *hw, struct AMPDU_TID_st *ampdu_tid)
{
    struct ssv_softc *sc = hw->priv;
    struct sk_buff *ampdu_skb = ampdu_tid->cur_ampdu_pkt;
    struct ieee80211_tx_info *info;
    struct ssv_vif_priv_data *vif_priv;
    while (skb_queue_len(&ampdu_tid->ampdu_skb_tx_queue))
    {
        u32 aggr_len;
        struct sk_buff *mpdu_skb;
        struct ampdu_hdr_st *ampdu_hdr;
        bool is_aggr_full = false;
        if (ampdu_skb == NULL)
        {
            ampdu_skb = _alloc_ampdu_skb(sc, ampdu_tid, 0);
            if (ampdu_skb == NULL)
                break;
            ampdu_tid->cur_ampdu_pkt = ampdu_skb;
        }
        ampdu_hdr = (struct ampdu_hdr_st *) ampdu_skb->head;
        aggr_len = skb_queue_len(&ampdu_hdr->mpdu_q);
        do
        {
            struct sk_buff_head *tx_q = &ampdu_tid->ampdu_skb_tx_queue;
            unsigned long flags;
            u32 new_total_skb_size;
            spin_lock_irqsave(&tx_q->lock, flags);
            mpdu_skb = skb_peek(&ampdu_tid->ampdu_skb_tx_queue);
            if (mpdu_skb == NULL)
            {
                spin_unlock_irqrestore(&tx_q->lock, flags);
                break;
            }
            info = IEEE80211_SKB_CB(mpdu_skb);
            vif_priv = (struct ssv_vif_priv_data *)info->control.vif->drv_priv;
            new_total_skb_size = mpdu_skb->len + ampdu_hdr->size;
            if (vif_priv->pair_cipher == SSV_CIPHER_CCMP) {
                new_total_skb_size += CCMP_MIC_LEN;
            }
            if ( new_total_skb_size > ampdu_hdr->max_size)
            {
                is_aggr_full = true;
                spin_unlock_irqrestore(&tx_q->lock, flags);
                break;
            }
            mpdu_skb = __skb_dequeue(&ampdu_tid->ampdu_skb_tx_queue);
            spin_unlock_irqrestore(&tx_q->lock, flags);
            _put_mpdu_to_ampdu(sc, ampdu_skb, mpdu_skb);
        } while (++aggr_len < MAX_AGGR_NUM );
        if ( (is_aggr_full || (aggr_len >= MAX_AGGR_NUM))
            || ( (aggr_len > 0)
                && (skb_queue_len(&ampdu_tid->early_aggr_ampdu_q) == 0)
                && (ampdu_tid->ssv_baw_head == SSV_ILLEGAL_SN)
                && _is_skb_q_empty(sc, ampdu_skb)))
        {
            SSV_ADD_AMPDU_TXINFO(sc, ampdu_skb);
            _queue_early_ampdu(sc, ampdu_tid, ampdu_skb);
            ampdu_tid->cur_ampdu_pkt = ampdu_skb = NULL;
        }
        _flush_early_ampdu_q(sc, ampdu_tid);
    }
}
void _queue_early_ampdu (struct ssv_softc *sc, struct AMPDU_TID_st *ampdu_tid,
                         struct sk_buff *ampdu_skb)
{
    unsigned long flags;
    struct ampdu_hdr_st *ampdu_hdr = (struct ampdu_hdr_st *) ampdu_skb->head;
    spin_lock_irqsave(&ampdu_tid->early_aggr_ampdu_q.lock, flags);
    __skb_queue_tail(&ampdu_tid->early_aggr_ampdu_q, ampdu_skb);
    ampdu_tid->early_aggr_skb_num += skb_queue_len(&ampdu_hdr->mpdu_q);
#ifdef SSV_AMPDU_FLOW_CONTROL
    if (ampdu_tid->early_aggr_skb_num >= SSV_AMPDU_FLOW_CONTROL_UPPER_BOUND)
    {
        ssv6200_tx_flow_control((void *) sc, sc->tx.hw_txqid[ampdu_tid->ac],
                                true, 1000);
    }
#endif
    spin_unlock_irqrestore(&ampdu_tid->early_aggr_ampdu_q.lock, flags);
}
void _flush_mpdu (struct ssv_softc *sc, struct ieee80211_sta *sta)
{
    unsigned long flags;
    struct ssv_sta_priv_data *ssv_sta_priv = (struct ssv_sta_priv_data *) sta->drv_priv;
    int i;
    for (i = 0; i < (sizeof(ssv_sta_priv->ampdu_tid) / sizeof(ssv_sta_priv->ampdu_tid[0])); i++)
    {
        struct AMPDU_TID_st *ampdu_tid = &ssv_sta_priv->ampdu_tid[i];
        struct sk_buff_head *early_aggr_ampdu_q;
        struct sk_buff *ampdu;
        struct ampdu_hdr_st *ampdu_hdr;
        struct sk_buff_head *mpdu_q;
        struct sk_buff *mpdu;
        if (ampdu_tid->state != AMPDU_STATE_OPERATION)
            continue;
        early_aggr_ampdu_q = &ampdu_tid->early_aggr_ampdu_q;
        spin_lock_irqsave(&early_aggr_ampdu_q->lock, flags);
        while ((ampdu = __skb_dequeue(early_aggr_ampdu_q)) != NULL)
        {
            ampdu_hdr = (struct ampdu_hdr_st *)ampdu->head;
            mpdu_q = &ampdu_hdr->mpdu_q;
            spin_unlock_irqrestore(&early_aggr_ampdu_q->lock, flags);
            while ((mpdu = __skb_dequeue(mpdu_q)) != NULL)
            {
                _send_hci_skb(sc, mpdu, AMPDU_HCI_SEND_TAIL_WITHOUT_FLOWCTRL);
            }
            ssv6200_ampdu_release_skb(ampdu, sc->hw);
            spin_lock_irqsave(&early_aggr_ampdu_q->lock, flags);
        }
        if (ampdu_tid->cur_ampdu_pkt != NULL)
        {
            ampdu_hdr = (struct ampdu_hdr_st *)ampdu_tid->cur_ampdu_pkt->head;
            mpdu_q = &ampdu_hdr->mpdu_q;
            spin_unlock_irqrestore(&early_aggr_ampdu_q->lock, flags);
            while ((mpdu = __skb_dequeue(mpdu_q)) != NULL)
            {
                _send_hci_skb(sc, mpdu, AMPDU_HCI_SEND_TAIL_WITHOUT_FLOWCTRL);
            }
            ssv6200_ampdu_release_skb(ampdu_tid->cur_ampdu_pkt, sc->hw);
            spin_lock_irqsave(&early_aggr_ampdu_q->lock, flags);
            ampdu_tid->cur_ampdu_pkt = NULL;
        }
        spin_unlock_irqrestore(&early_aggr_ampdu_q->lock, flags);
    }
}
bool ssv6200_ampdu_tx_handler (struct ieee80211_hw *hw, struct sk_buff *skb)
{
    struct ssv_softc *sc = hw->priv;
    struct ieee80211_hdr *hdr = (struct ieee80211_hdr *) skb->data;
    struct ieee80211_tx_info *info = IEEE80211_SKB_CB(skb);
    struct sk_buff *tx_skb = skb;
    struct sk_buff *copy_skb = NULL;
    struct SKB_info_st *mpdu_skb_info_p = (SKB_info *) (skb->head);
    struct SKB_info_st *copy_skb_info_p = NULL;
    struct ieee80211_sta *sta = mpdu_skb_info_p->sta;
    struct ssv_sta_priv_data *ssv_sta_priv =
            (struct ssv_sta_priv_data *) sta->drv_priv;
    u8 tidno;
    struct AMPDU_TID_st *ampdu_tid;
    int ampdu_tx_frame = 0;
    if (sta == NULL)
    {
        WARN_ON(1);
        return false;
    }
    tidno = ieee80211_get_qos_ctl(hdr)[0] & IEEE80211_QOS_CTL_TID_MASK;
    ampdu_db_log("tidno = %d\n", tidno);
    ampdu_tid = &ssv_sta_priv->ampdu_tid[tidno];
    if (ampdu_tid->state != AMPDU_STATE_OPERATION)
        return false;
#ifdef AMPDU_CHECK_SKB_SEQNO
    {
        u32 skb_seqno = ((struct ieee80211_hdr*) (skb->data))->seq_ctrl
                        >> SSV_SEQ_NUM_SHIFT;
        u32 tid_seqno = ampdu_tid->last_seqno;
        if ((tid_seqno != (-1)) && (skb_seqno != NEXT_PKT_SN(tid_seqno)))
        {
            prn_aggr_err(sc, "Non continueous seq no: %d - %d\n", tid_seqno, skb_seqno);
            return false;
        }
        ampdu_tid->last_seqno = skb_seqno;
    }
#endif
#if 1
    mpdu_skb_info_p->lowest_rate = SSV_HT_RATE_UPDATE(sc, skb, mpdu_skb_info_p->rates);
    if (SSV_AMPDU_MAX_TRANSMIT_LENGTH(sc, skb, mpdu_skb_info_p->lowest_rate) == 0)
    {
        return false;
    }
#endif
#if 0
    if ((ampdu_tid->ssv_baw_head == SSV_ILLEGAL_SN)
        && (skb_queue_len(&ampdu_tid->ampdu_skb_tx_queue) == 0)
        && (skb_queue_len(&ampdu_tid->early_aggr_ampdu_q) == 0)
        && ((ampdu_tid->cur_ampdu_pkt == NULL)
            || (skb_queue_len(
                    &((struct ampdu_hdr_st *) (ampdu_tid->cur_ampdu_pkt->head))->mpdu_q)
                == 0))
        && _is_skb_q_empty(sc, skb))
    {
        ampdu_tid->mib.ampdu_mib_pass_counter++;
#if 0
        prn_aggr_err(sc,
                "pass %d\n",
                ((struct ieee80211_hdr*)(skb->data))->seq_ctrl >> SSV_SEQ_NUM_SHIFT);
#else
        if ((ssv_sta_priv->ampdu_tid[tidno].mib.ampdu_mib_pass_counter % 1024) == 0)
            prn_aggr_err(sc, "STA %d TID %d passed %d\n", ssv_sta_priv->sta_idx,
                         ampdu_tid->tidno,
                         ampdu_tid->mib.ampdu_mib_pass_counter);
#endif
        return false;
    }
#endif
    mpdu_skb_info_p = (SKB_info *) (skb->head);
    mpdu_skb_info_p->mpdu_retry_counter = 0;
    mpdu_skb_info_p->ampdu_tx_status = AMPDU_ST_NON_AMPDU;
    mpdu_skb_info_p->ampdu_tx_final_retry_count = 0;
    mpdu_skb_info_p->directly_ack = false;
    ssv_sta_priv->ampdu_tid[tidno].ac = skb_get_queue_mapping(skb);
    ampdu_tx_frame = atomic_read(&sc->ampdu_tx_frame);
    if ((sc->force_disable_directly_ack_tx != true) &&
        (sc->sc_flags & SC_OP_DIRECTLY_ACK) &&
        (ampdu_tx_frame < sc->directly_ack_high_threshold)) {
        info = IEEE80211_SKB_CB(skb);
        tx_skb = skb;
        info->flags |= IEEE80211_TX_STAT_ACK;
        copy_skb = skb_copy(tx_skb, GFP_ATOMIC);
        if (!copy_skb) {
            printk("create TX skb copy failed!\n");
            return false;
        }
        ieee80211_tx_status(sc->hw, tx_skb);
        skb = copy_skb;
        copy_skb_info_p = (SKB_info *)(skb->head);
        copy_skb_info_p->directly_ack = true;
    } else {
        sc->sc_flags &= ~SC_OP_DIRECTLY_ACK;
    }
    {
        bool ret;
        ret = ssv6200_ampdu_add_delimiter_and_crc32(skb);
        if (ret == false)
        {
            ssv6200_ampdu_release_skb(skb, hw);
            return false;
        }
        skb_queue_tail(&ssv_sta_priv->ampdu_tid[tidno].ampdu_skb_tx_queue, skb);
        atomic_inc(&sc->ampdu_tx_frame);
        ssv_sta_priv->ampdu_tid[tidno].timestamp = jiffies;
    }
    _aggr_ampdu_tx_q(hw, &ssv_sta_priv->ampdu_tid[tidno]);
    return true;
}
u32 ssv6xxx_ampdu_flush (struct ieee80211_hw *hw)
{
    struct ssv_softc *sc = hw->priv;
    struct AMPDU_TID_st *cur_AMPDU_TID;
    u32 flushed_ampdu = 0;
    u32 tid_idx = 0;
    if (!list_empty(&sc->tx.ampdu_tx_que))
    {
        list_for_each_entry_rcu(cur_AMPDU_TID, &sc->tx.ampdu_tx_que, list)
        {
            tid_idx++;
#ifdef DEBUG_AMPDU_FLUSH
            {
                int i = 0;
                for (i = 0; i < MAX_TID; i++)
                    if (sc->tid[i] == cur_AMPDU_TID)
                        break;
                if (i == MAX_TID)
                {
                    printk(KERN_ERR "No matching TID (%d) found! %p\n", tid_idx, cur_AMPDU_TID);
                    continue;
                }
            }
#endif
            if (cur_AMPDU_TID->state != AMPDU_STATE_OPERATION)
            {
                struct ieee80211_sta *sta = cur_AMPDU_TID->sta;
                struct ssv_sta_priv_data *sta_priv = (struct ssv_sta_priv_data *)sta->drv_priv;
                dev_dbg(sc->dev, "STA %d TID %d is @%d\n",
                        sta_priv->sta_idx, cur_AMPDU_TID->tidno,
                        cur_AMPDU_TID->state);
                continue;
            }
            if ((cur_AMPDU_TID->state == AMPDU_STATE_OPERATION)
                && (skb_queue_len(&cur_AMPDU_TID->early_aggr_ampdu_q) == 0)
                && (cur_AMPDU_TID->cur_ampdu_pkt != NULL))
            {
                struct ampdu_hdr_st *ampdu_hdr =
                        (struct ampdu_hdr_st *) (cur_AMPDU_TID->cur_ampdu_pkt->head);
                u32 aggr_len = skb_queue_len(&ampdu_hdr->mpdu_q);
                if (aggr_len)
                {
                    struct sk_buff *ampdu_skb = cur_AMPDU_TID->cur_ampdu_pkt;
                    cur_AMPDU_TID->cur_ampdu_pkt = NULL;
                    SSV_ADD_AMPDU_TXINFO(sc, ampdu_skb);
                    _queue_early_ampdu(sc, cur_AMPDU_TID, ampdu_skb);
                    #if 0
                    prn_aggr_err(sc, "A%c %d %d\n", sc->tx_q_empty ? 'e' : 't',
                                ampdu_hdr->first_sn, aggr_len);
                    #endif
                }
            }
            if (skb_queue_len(&cur_AMPDU_TID->early_aggr_ampdu_q) > 0)
                flushed_ampdu += _flush_early_ampdu_q(sc, cur_AMPDU_TID);
        }
    }
    return flushed_ampdu;
}
int ssv6200_dump_BA_notification (char *buf,
                            struct ampdu_ba_notify_data *ba_notification)
{
    int i;
    char *orig_buf = buf;
    for (i = 0; i < MAX_AGGR_NUM; i++)
    {
        if (ba_notification->seq_no[i] == (u16) (-1))
            break;
        buf += sprintf(buf, " %d", ba_notification->seq_no[i]);
    }
    return ((size_t)buf - (size_t)orig_buf);
}
int _dump_ba_skb (struct ssv_softc *sc, char *buf, int buf_size, struct sk_buff *ba_skb)
{
    struct ieee80211_hdr *hdr = (struct ieee80211_hdr *) (ba_skb->data
                                                          + sc->sh->rx_desc_len);
    AMPDU_BLOCKACK *BA_frame = (AMPDU_BLOCKACK *) hdr;
    u32 ssn = BA_frame->BA_ssn;
    struct ampdu_ba_notify_data *ba_notification =
            (struct ampdu_ba_notify_data *) (ba_skb->data + ba_skb->len
                                             - sizeof(struct ampdu_ba_notify_data));
    int prt_size;
    prt_size = snprintf(buf, buf_size, "\n\t\t%04d %08X %08X -",
                        ssn, BA_frame->BA_sn_bit_map[0], BA_frame->BA_sn_bit_map[1]);
    buf_size -= prt_size;
    buf += prt_size;
    prt_size = prt_size + ssv6200_dump_BA_notification(buf, ba_notification);
    return prt_size;
}
bool ssv6xxx_ssn_to_bit_idx (u32 start_ssn, u32 mpdu_ssn, u32 *word_idx,
                             u32 *bit_idx)
{
    u32 ret_bit_idx, ret_word_idx = 0;
    s32 diff = mpdu_ssn - start_ssn;
    if (diff >= 0)
    {
        if (diff >= SSV_AMPDU_BA_WINDOW_SIZE)
        {
            return false;
        }
        ret_bit_idx = diff;
    }
    else
    {
        diff = -diff;
        if (diff <= (SSV_AMPDU_MAX_SSN - SSV_AMPDU_BA_WINDOW_SIZE))
        {
            *word_idx = 0;
            *bit_idx = 0;
            return false;
        }
        ret_bit_idx = SSV_AMPDU_MAX_SSN - diff;
    }
    if (ret_bit_idx >= 32)
    {
        ret_bit_idx -= 32;
        ret_word_idx = 1;
    }
    *bit_idx = ret_bit_idx;
    *word_idx = ret_word_idx;
    return true;
}
bool ssv6xxx_inc_bit_idx (struct ssv_softc *sc, u32 ssn_1st, u32 ssn_next,
                          u32 *word_idx, u32 *bit_idx)
{
    u32 ret_word_idx = *word_idx, ret_bit_idx = *bit_idx;
    s32 diff = (s32) ssn_1st - (s32) ssn_next;
    if (diff > 0)
    {
        if (diff < (SSV_AMPDU_MAX_SSN - SSV_AMPDU_BA_WINDOW_SIZE))
        {
            prn_aggr_err(sc, "Irrational SN distance in AMPDU: %d %d.\n",
                         ssn_1st, ssn_next);
            return false;
        }
        diff = SSV_AMPDU_MAX_SSN - diff;
    }
    else
    {
        diff = -diff;
    }
    if (diff > SSV_AMPDU_MAX_SSN)
        prn_aggr_err(sc, "DF %d - %d = %d\n", ssn_1st, ssn_next, diff);
    ret_bit_idx += diff;
    if (ret_bit_idx >= 32)
    {
        ret_bit_idx -= 32;
        ret_word_idx++;
    }
    *word_idx = ret_word_idx;
    *bit_idx = ret_bit_idx;
    return true;
}
void ssv6xxx_release_frames (struct AMPDU_TID_st *ampdu_tid)
{
    u32 head_ssn, head_ssn_before, last_ssn;
    struct sk_buff **skb;
    struct SKB_info_st *skb_info;
    struct ssv_softc *sc = AMPDU_TID_TO_SC(ampdu_tid);
    unsigned long flags;
    spin_lock_irqsave(&ampdu_tid->pkt_array_lock, flags);
    head_ssn_before = ampdu_tid->ssv_baw_head;
#if 1
    if (head_ssn_before >= SSV_AMPDU_MAX_SSN)
    {
        spin_unlock_irqrestore(&ampdu_tid->pkt_array_lock, flags);
        prn_aggr_err(sc, "l x.x %d\n", head_ssn_before);
        return;
    }
#else
    BUG_ON(head_ssn_before >= SSV_AMPDU_MAX_SSN);
#endif
    head_ssn = ampdu_tid->ssv_baw_head;
    last_ssn = head_ssn;
    do
    {
        skb = &INDEX_PKT_BY_SSN(ampdu_tid, head_ssn);
        if (*skb == NULL)
        {
            head_ssn = SSV_ILLEGAL_SN;
            {
                int i;
                char sn_str[66 * 5] = "";
                char *str = sn_str;
                for (i = 0; i < SSV_AMPDU_BA_WINDOW_SIZE; i++)
                    if (ampdu_tid->aggr_pkts[i] != NULL)
                    {
                        str += sprintf(str, "%d ",
                                       ampdu_skb_ssn(ampdu_tid->aggr_pkts[i]));
                    }
                *str = 0;
                if (str == sn_str)
                {
                }
                else
                    prn_aggr_err(sc,
                            "ILL %d %d - %d (%s)\n",
                            head_ssn_before, last_ssn, ampdu_tid->aggr_pkt_num, sn_str);
            }
            break;
        }
        skb_info = (struct SKB_info_st *) ((*skb)->head);
        if ((skb_info->ampdu_tx_status == AMPDU_ST_DONE)
            || (skb_info->ampdu_tx_status == AMPDU_ST_DROPPED))
        {
            __skb_queue_tail(&ampdu_tid->release_queue, *skb);
            *skb = NULL;
            last_ssn = head_ssn;
            INC_PKT_SN(head_ssn);
            ampdu_tid->aggr_pkt_num--;
            if (skb_info->ampdu_tx_status == AMPDU_ST_DROPPED)
                ampdu_tid->mib.ampdu_mib_discard_counter++;
        }
        else
        {
            break;
        }
    } while (1);
    ampdu_tid->ssv_baw_head = head_ssn;
#if 0
    if (head_ssn == SSV_ILLEGAL_SN)
    {
        u32 i = head_ssn_before;
        do
        {
            skb = &INDEX_PKT_BY_SSN(ampdu_tid, i);
            if (*skb != NULL)
            prn_aggr_err(sc, "O.o %d: %d - %d\n", head_ssn_before, i, ampdu_skb_ssn(*skb));
            INC_PKT_SN(i);
        }while (i != head_ssn_before);
    }
#endif
    spin_unlock_irqrestore(&ampdu_tid->pkt_array_lock, flags);
#if 0
    if (head_ssn_before != head_ssn)
    {
        prn_aggr_err(sc, "H %d -> %d (%d - %d)\n", head_ssn_before, head_ssn,
                ampdu_tid->aggr_pkt_num, skb_queue_len(&ampdu_tid->ampdu_skb_tx_queue));
    }
#endif
}
static int _collect_retry_frames (struct AMPDU_TID_st *ampdu_tid)
{
    u16 ssn, head_ssn, end_ssn, diff;
    int num_retry = 0;
    int timeout_check = 1;
    unsigned long check_jiffies = jiffies;
    struct ssv_softc *sc = AMPDU_TID_TO_SC(ampdu_tid);
 head_ssn = ampdu_tid->ssv_baw_head;
    ssn = head_ssn;
    if (ssn == SSV_ILLEGAL_SN)
        return 0;
    end_ssn = (head_ssn + SSV_AMPDU_BA_WINDOW_SIZE) % SSV_AMPDU_MAX_SSN;
    do
    {
        struct sk_buff *skb = INDEX_PKT_BY_SSN(ampdu_tid, ssn);
        struct SKB_info_st *skb_info;
        int timeout_retry = 0;
        if (skb == NULL){
            INC_PKT_SN(ssn);
            continue;
        }
        skb_info = (SKB_info *) (skb->head);
        if ( timeout_check
            && (skb_info->ampdu_tx_status == AMPDU_ST_SENT))
        {
            unsigned long cur_jiffies = jiffies;
            unsigned long timeout_jiffies = skb_info->aggr_timestamp
                                            + msecs_to_jiffies(BA_WAIT_TIMEOUT);
            u32 delta_ms;
            if (time_before(cur_jiffies, timeout_jiffies))
            {
                timeout_check = 0;
                continue;
            }
            ssv6xxx_mark_skb_retry(sc, skb_info, skb);
            delta_ms = jiffies_to_msecs((long)(cur_jiffies - skb_info->aggr_timestamp));
            prn_aggr_err(sc, "t S%d-T%d-%d (%u)\n",
                         ((struct ssv_sta_priv_data *)skb_info->sta->drv_priv)->sta_idx,
                         ampdu_tid->tidno, ssn,
                         delta_ms);
            if (delta_ms > 1000)
            {
                prn_aggr_err(sc, "Last checktime %lu - %lu = %u, cur jiffies %lu \n",
                             check_jiffies, skb_info->aggr_timestamp,
                             jiffies_to_msecs((long)(check_jiffies - skb_info->aggr_timestamp)), cur_jiffies);
            }
            timeout_retry = 1;
        }
        diff = SSV_AMPDU_SN_a_minus_b(head_ssn, ssn);
  if (diff > ampdu_tid->ssv_baw_size) {
            prn_aggr_dbg(sc, "Z. release skb (s %d, h %d, d %d)\n", ssn, head_ssn, diff);
   skb_info->ampdu_tx_status = AMPDU_ST_DROPPED;
  }
        if (skb_info->ampdu_tx_status == AMPDU_ST_RETRY)
        {
#if 0
            if (!timeout_retry)
                prn_aggr_dbg(sc, "r %d - %d\n", ssn, ampdu_skb_ssn(skb));
#endif
            skb_info->ampdu_tx_status = AMPDU_ST_RETRY_Q;
            skb_queue_tail(&ampdu_tid->retry_queue, skb);
            ampdu_tid->mib.ampdu_mib_retry_counter++;
            num_retry++;
        }
        INC_PKT_SN(ssn);
    } while (ssn != end_ssn);
    ampdu_tid->timestamp = check_jiffies;
    return num_retry;
}
void ssv6xxx_mark_skb_retry (struct ssv_softc *sc, struct SKB_info_st *skb_info, struct sk_buff *skb)
{
    struct SKB_info_st *mpdu_skb_info_p = (SKB_info *) (skb->head);
    struct ieee80211_sta *sta = mpdu_skb_info_p->sta;
    struct ssv_sta_priv_data *ssv_sta_priv = (struct ssv_sta_priv_data *) sta->drv_priv;
    if (skb_info->ampdu_tx_status == AMPDU_ST_SENT){
        if (skb_info->mpdu_retry_counter == 0)
        {
            struct ieee80211_hdr *skb_hdr = ampdu_skb_hdr(skb);
            skb_hdr->frame_control |= cpu_to_le16(IEEE80211_FCTL_RETRY);
        }
        skb_info->ampdu_tx_status = AMPDU_ST_RETRY;
        ssv_sta_priv->retry_samples[skb_info->mpdu_retry_counter]++;
        skb_info->mpdu_retry_counter++;
        if (skb_info->mpdu_retry_counter >= SSV_AMPDU_retry_counter_max){
            skb_info->ampdu_tx_status = AMPDU_ST_DROPPED;
        }
    }
}
void ssv6xxx_find_txpktrun_no_from_ssn(struct ssv_softc *sc, u32 ssn,
    struct ssv_sta_priv_data *ssv_sta_priv)
 {
    u32 i , j;
    u8 slot;
    for (i = 0; i < MAX_CONCUR_AMPDU; i ++){
        if (ssv_sta_priv->ampdu_ssn[i].mpdu_num != 0) {
            for (j = 0 ; j < ssv_sta_priv->ampdu_ssn[i].mpdu_num; j++) {
                if (ssv_sta_priv->ampdu_ssn[i].ssn[j] == ssn) {
                    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_AMPDU_SSN,
                        "==>%s: found new acked ssn %d, tx_pkt_run_no %d at slot %d \n", __func__
                        , ssn,ssv_sta_priv->ampdu_ssn[i].tx_pkt_run_no, i);
                    slot = i;
                    goto out;
                }
            }
        }
    }
out:
    if ( i != MAX_CONCUR_AMPDU) {
        memset((void*)&ssv_sta_priv->ampdu_ssn[slot], 0, sizeof(struct aggr_ssn));
        dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_AMPDU_SSN,"%s: slot %d released!\n", __func__, slot);
    } else {
        dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_AMPDU_SSN,"%s: not found tx_pkt_run_no for this ssn %s\n", __func__, ssn);
    }
}
u32 ssv6200_ba_map_walker (struct AMPDU_TID_st *ampdu_tid, u32 start_ssn,
                           u32 sn_bit_map[2],
                           struct ampdu_ba_notify_data *ba_notify_data,
                           u32 *p_acked_num)
{
    int i = 0;
    u32 ssn = ba_notify_data->seq_no[0];
    u32 word_idx = (-1), bit_idx = (-1);
    bool found = ssv6xxx_ssn_to_bit_idx(start_ssn, ssn, &word_idx, &bit_idx);
    bool first_found = found;
    u32 aggr_num = 0;
    u32 acked_num = 0;
    struct ssv_softc *sc = AMPDU_TID_TO_SC(ampdu_tid);
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
        if (skb_ssn != ssn)
        {
            prn_aggr_err(sc, "Unmatched SSN packet: %d - %d - %d\n",
                         ssn, skb_ssn, start_ssn);
        }
        else
        {
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
        if (++i >= MAX_AGGR_NUM)
            break;
        ssn = ba_notify_data->seq_no[i];
        if (ssn >= SSV_AMPDU_MAX_SSN)
            break;
        if (first_found)
        {
            u32 old_word_idx, old_bit_idx;
            old_word_idx = word_idx;
            old_bit_idx = bit_idx;
            found = ssv6xxx_inc_bit_idx(sc, cur_ssn, ssn, &word_idx, &bit_idx);
            if (found && (word_idx >= 2 || bit_idx >= 32))
            {
                prn_aggr_err(sc,
                        "idx error 2: %d 0x%08X 0X%08X %d %d (%d %d) (%d %d)\n",
                        start_ssn, sn_bit_map[1], sn_bit_map[0], cur_ssn, ssn, word_idx, bit_idx, old_word_idx, old_bit_idx);
                found = false;
            }
            else if (!found)
            {
                char strbuf[256];
                ssv6200_dump_BA_notification(strbuf, ba_notify_data);
                prn_aggr_err(sc, "SN out-of-order: %d\n%s\n", start_ssn, strbuf);
            }
        }
        else
        {
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
static void _flush_release_queue (struct ieee80211_hw *hw,
                                  struct sk_buff_head *release_queue)
{
    struct ssv_softc *sc = hw->priv;
    int ampdu_tx_frame = 0;
    do
    {
        struct sk_buff *ampdu_skb = __skb_dequeue(release_queue);
        struct ieee80211_tx_info *tx_info;
        struct SKB_info_st *skb_info;
        if (ampdu_skb == NULL)
            break;
        skb_info = (struct SKB_info_st *) (ampdu_skb->head);
        skb_pull(ampdu_skb, AMPDU_DELIMITER_LEN);
        tx_info = IEEE80211_SKB_CB(ampdu_skb);
        ieee80211_tx_info_clear_status(tx_info);
        tx_info->flags |= IEEE80211_TX_STAT_AMPDU;
        if (skb_info->ampdu_tx_status == AMPDU_ST_DONE)
            tx_info->flags |= IEEE80211_TX_STAT_ACK;
        tx_info->status.ampdu_len = 1;
        tx_info->status.ampdu_ack_len = 1;
        if (skb_info->directly_ack) {
            dev_kfree_skb_any(ampdu_skb);
        } else {
            #if defined(USE_THREAD_RX) && !defined(IRQ_PROC_TX_DATA)
            ieee80211_tx_status(hw, ampdu_skb);
            #else
            ieee80211_tx_status_irqsafe(hw, ampdu_skb);
            #endif
        }
        ampdu_tx_frame = atomic_sub_return(1, &sc->ampdu_tx_frame);
        if (ampdu_tx_frame < sc->directly_ack_low_threshold)
            sc->sc_flags |= SC_OP_DIRECTLY_ACK;
    } while (1);
}
#if 0
static u16 _get_BA_notification_hits(u16 ssn,u32 *bit_map,struct ampdu_ba_notify_data *ba_notification,u16 *max_continue_hits,u16 *aggr_num)
{
    int i;
    u16 hits=0,continue_hits=0;
    u64 bitMap=0;
    if(bit_map)
    bitMap = ((u64)bit_map[1]<<32) | (u64)(bit_map[0]);
    for (i = 0; i < MAX_AGGR_NUM; i++)
    {
        if (ba_notification->seq_no[i] == (u16)(-1))
        break;
        if(ssn <= ba_notification->seq_no[i])
        {
            if((bitMap>>(ba_notification->seq_no[i]-ssn))&0x1)
            {
                hits++;
                continue_hits++;
                if(*max_continue_hits<=continue_hits)
                *max_continue_hits = continue_hits;
            }
            else
            {
                continue_hits=0;
            }
        }
    }
    *aggr_num = i;
    return hits;
}
#endif
void ssv6200_ampdu_no_BA_handler (struct ieee80211_hw *hw, struct sk_buff *skb)
{
    struct cfg_host_event *host_event = (struct cfg_host_event *) skb->data;
    struct ampdu_ba_notify_data *ba_notification =
            (struct ampdu_ba_notify_data *) &host_event->dat[0];
    struct ieee80211_hdr *hdr = (struct ieee80211_hdr *) (ba_notification + 1);
    struct ssv_softc *sc = hw->priv;
    struct ieee80211_sta *sta = ssv6xxx_find_sta_by_addr(sc, hdr->addr1);
    u8 tidno = ieee80211_get_qos_ctl(hdr)[0] & IEEE80211_QOS_CTL_TID_MASK;
    struct ssv_sta_priv_data *ssv_sta_priv;
    char seq_str[256];
    struct AMPDU_TID_st *ampdu_tid;
    int i;
    u16 aggr_num = 0;
    struct firmware_rate_control_report_data *report_data;
    if (sta == NULL)
    {
        prn_aggr_err(sc,
                "NO BA for %d to unmatched STA %02X-%02X-%02X-%02X-%02X-%02X: %s\n",
                tidno, hdr->addr1[0], hdr->addr1[1], hdr->addr1[2], hdr->addr1[3], hdr->addr1[4], hdr->addr1[5], seq_str);
        dev_kfree_skb_any(skb);
        return;
    }
    ssv_sta_priv = (struct ssv_sta_priv_data *) sta->drv_priv;
    down_read(&sc->sta_info_sem);
    if ((ssv_sta_priv->sta_info->s_flags & STA_FLAG_VALID) == 0) {
        up_read(&sc->sta_info_sem);
        prn_aggr_err(sc, "%s(): sta_info is gone.\n", __func__);
        dev_kfree_skb_any(skb);
        return;
    }
    ssv6200_dump_BA_notification(seq_str, ba_notification);
    ssv6xxx_find_txpktrun_no_from_ssn(sc, ba_notification->seq_no[0], ssv_sta_priv);
    prn_aggr_err(sc,
            "NO BA for %d to %02X-%02X-%02X-%02X-%02X-%02X: %s\n",
            tidno, sta->addr[0], sta->addr[1], sta->addr[2], sta->addr[3], sta->addr[4], sta->addr[5], seq_str);
    ampdu_tid = &ssv_sta_priv->ampdu_tid[tidno];
    if (ampdu_tid->state != AMPDU_STATE_OPERATION)
    {
        up_read(&sc->sta_info_sem);
        dev_kfree_skb_any(skb);
        return;
    }
    for (i = 0; i < MAX_AGGR_NUM; i++)
    {
        u32 ssn = ba_notification->seq_no[i];
        struct sk_buff *skb;
        u32 skb_ssn;
        struct SKB_info_st *skb_info;
        if (ssn >= (4096))
            break;
        aggr_num++;
        skb = INDEX_PKT_BY_SSN(ampdu_tid, ssn);
        skb_ssn = (skb == NULL) ? (-1) : ampdu_skb_ssn(skb);
        if (skb_ssn != ssn)
        {
            prn_aggr_err(sc, "Unmatched SSN packet: %d - %d\n", ssn, skb_ssn);
            continue;
        }
        skb_info = (struct SKB_info_st *) (skb->head);
        if (skb_info->ampdu_tx_status == AMPDU_ST_SENT)
        {
            if (skb_info->mpdu_retry_counter < SSV_AMPDU_retry_counter_max)
            {
                if (skb_info->mpdu_retry_counter == 0)
                {
                    struct ieee80211_hdr *skb_hdr = ampdu_skb_hdr(skb);
                    skb_hdr->frame_control |= cpu_to_le16(IEEE80211_FCTL_RETRY);
                }
                skb_info->ampdu_tx_status = AMPDU_ST_RETRY;
                ssv_sta_priv->retry_samples[skb_info->mpdu_retry_counter]++;
                skb_info->mpdu_retry_counter++;
            }
            else
            {
                skb_info->ampdu_tx_status = AMPDU_ST_DROPPED;
                prn_aggr_err(sc, "p %d\n", skb_ssn);
            }
        }
        else
        {
            prn_aggr_err(sc, "S %d %d\n", skb_ssn, skb_info->ampdu_tx_status);
        }
    }
    ssv6xxx_release_frames(ampdu_tid);
    host_event->h_event = SOC_EVT_RC_AMPDU_REPORT;
    report_data =
            (struct firmware_rate_control_report_data *) &host_event->dat[0];
    report_data->ampdu_len = aggr_num;
    report_data->ampdu_ack_len = 0;
    report_data->wsid = ssv_sta_priv->sta_info->hw_wsid;
#if 0
    printk("AMPDU report NO BA!!wsid[%d]didx[%d]F[%d]R[%d]S[%d]\n",report_data->wsid,report_data->data_rate,report_data->mpduFrames,report_data->mpduFrameRetry,report_data->mpduFrameSuccess);
#endif
    skb_queue_tail(&sc->rc_report_queue, skb);
    if (sc->rc_report_sechedule == 0)
        queue_work(sc->rc_report_workqueue, &sc->rc_report_work);
    up_read(&sc->sta_info_sem);
}
#ifndef SSV_SUPPORT_HAL
void ssv6200_ampdu_BA_handler (struct ieee80211_hw *hw, struct sk_buff *skb)
{
    struct ssv_softc *sc = hw->priv;
    struct ieee80211_hdr *hdr = (struct ieee80211_hdr *) (skb->data
                                                          + sc->sh->rx_desc_len);
    AMPDU_BLOCKACK *BA_frame = (AMPDU_BLOCKACK *) hdr;
    struct ieee80211_sta *sta;
    struct ssv_sta_priv_data *ssv_sta_priv;
    struct ampdu_ba_notify_data *ba_notification;
    u32 ssn, aggr_num = 0, acked_num = 0;
    u8 tid_no;
    u32 sn_bit_map[2];
    struct firmware_rate_control_report_data *report_data;
    HDR_HostEvent *host_evt;
    sta = ssv6xxx_find_sta_by_rx_skb(sc, skb);
    if (sta == NULL)
    {
        if (skb->len > AMPDU_BA_FRAME_LEN)
        {
            char strbuf[256];
            struct ampdu_ba_notify_data *ba_notification =
                    (struct ampdu_ba_notify_data *) (skb->data + skb->len
                                                     - sizeof(struct ampdu_ba_notify_data));
            ssv6200_dump_BA_notification(strbuf, ba_notification);
            prn_aggr_err(sc, "BA from not connected STA (%02X-%02X-%02X-%02X-%02X-%02X) (%s)\n",
                    BA_frame->ta_addr[0], BA_frame->ta_addr[1], BA_frame->ta_addr[2],
                    BA_frame->ta_addr[3], BA_frame->ta_addr[4], BA_frame->ta_addr[5], strbuf);
        }
        dev_kfree_skb_any(skb);
        return;
    }
    ssv_sta_priv = (struct ssv_sta_priv_data *) sta->drv_priv;
    ssn = BA_frame->BA_ssn;
    sn_bit_map[0] = BA_frame->BA_sn_bit_map[0];
    sn_bit_map[1] = BA_frame->BA_sn_bit_map[1];
    tid_no = BA_frame->tid_info;
    ssv_sta_priv->ampdu_mib_total_BA_counter++;
    if (ssv_sta_priv->ampdu_tid[tid_no].state == AMPDU_STATE_STOP)
    {
        prn_aggr_err(sc, "ssv6200_ampdu_BA_handler state == AMPDU_STATE_STOP.\n");
        dev_kfree_skb_any(skb);
        return;
    }
    ssv_sta_priv->ampdu_tid[tid_no].mib.ampdu_mib_BA_counter++;
    if (skb->len <= AMPDU_BA_FRAME_LEN)
    {
        prn_aggr_err(sc, "b %d\n", ssn);
        dev_kfree_skb_any(skb);
        return;
    }
    ba_notification =
            (struct ampdu_ba_notify_data *) (skb->data + skb->len
                                             - sizeof(struct ampdu_ba_notify_data));
    ssv6xxx_find_txpktrun_no_from_ssn(sc, ba_notification->seq_no[0], ssv_sta_priv);
#if 0
    if (1)
    {
        char strbuf[256];
        ssv6200_dump_BA_notification(strbuf, ba_notification);
        prn_aggr_err(sc, "B %d %08X %08X: %s\n", ssn, sn_bit_map[0], sn_bit_map[1], strbuf);
    }
#endif
    aggr_num = ssv6200_ba_map_walker(&(ssv_sta_priv->ampdu_tid[tid_no]), ssn,
                              sn_bit_map, ba_notification, &acked_num);
    #ifdef CONFIG_SSV6XXX_DEBUGFS
    if (ssv_sta_priv->ampdu_tid[tid_no].debugfs_dir)
    {
        struct sk_buff *dup_skb;
        if (skb_queue_len(&ssv_sta_priv->ampdu_tid[tid_no].ba_q) > 24)
        {
            struct sk_buff *ba_skb = skb_dequeue(&ssv_sta_priv->ampdu_tid[tid_no].ba_q);
            if (ba_skb)
                dev_kfree_skb_any(ba_skb);
        }
        dup_skb = skb_clone(skb, GFP_ATOMIC);
        if (dup_skb)
            skb_queue_tail(&ssv_sta_priv->ampdu_tid[tid_no].ba_q, dup_skb);
    }
    #endif
    skb_trim(skb, skb->len - sizeof(struct ampdu_ba_notify_data));
#if 0
    total_debug_count++;
    if (ba_notification_hits != ba_notification_aggr_num)
    printk("rate[%d] firmware retry [%d] agg nums[%d] hits[%d] continue_hits[%d] \n",ba_notification.data_rate,ba_notification.retry_count,ba_notification_aggr_num,ba_notification_hits,ba_notification_continue_hits);
    else
    {
        if (ba_notification.retry_count==0)
        total_perfect_debug_count++;
        else
        total_perfect_debug_count_but_firmware_retry++;
    }
    if ((total_debug_count % 2000) == 0)
    {
        printk("Percentage %d/2000\n",total_perfect_debug_count);
        printk("firmware retry [%d] no BA[%d]\n",total_perfect_debug_count_but_firmware_retry,no_ba_debug_count);
        total_debug_count = 0;
        total_perfect_debug_count_but_firmware_retry=0;
        total_perfect_debug_count = 0;
        no_ba_debug_count = 0;
    }
#endif
    host_evt = (HDR_HostEvent *) skb->data;
    host_evt->h_event = SOC_EVT_RC_AMPDU_REPORT;
    report_data =
            (struct firmware_rate_control_report_data *) &host_evt->dat[0];
    memcpy(report_data, ba_notification,
           sizeof(struct firmware_rate_control_report_data));
    report_data->ampdu_len = aggr_num;
    report_data->ampdu_ack_len = acked_num;
#ifdef RATE_CONTROL_HT_PERCENTAGE_TRACE
    if((acked_num) && (acked_num != aggr_num))
    {
        int i;
        for (i = 0; i < SSV62XX_TX_MAX_RATES ; i++) {
            if(report_data->rates[i].data_rate == -1)
                break;
            if(report_data->rates[i].count == 0) {
                    printk("*********************************\n");
                    printk("       Illegal HT report         \n");
                    printk("*********************************\n");
            }
            printk("        i=[%d] rate[%d] count[%d]\n",i,report_data->rates[i].data_rate,report_data->rates[i].count);
        }
        printk("AMPDU percentage = %d%% \n",acked_num*100/aggr_num);
    }
    else if(acked_num == 0)
    {
        printk("AMPDU percentage = 0%% aggr_num=%d acked_num=%d\n",aggr_num,acked_num);
    }
#endif
    skb_queue_tail(&sc->rc_report_queue, skb);
    if (sc->rc_report_sechedule == 0)
        queue_work(sc->rc_report_workqueue, &sc->rc_report_work);
}
#endif
static void _postprocess_BA (struct ssv_softc *sc, struct ssv_sta_info *sta_info, void *param)
{
    int j;
    struct ssv_sta_priv_data *ssv_sta_priv;
    if ( (sta_info->sta == NULL)
        || ((sta_info->s_flags & STA_FLAG_VALID) == 0))
        return;
    ssv_sta_priv = (struct ssv_sta_priv_data *) sta_info->sta->drv_priv;
    for (j = 0; j < WMM_TID_NUM; j++)
    {
        AMPDU_TID *ampdu_tid = &ssv_sta_priv->ampdu_tid[j];
        if (ampdu_tid->state != AMPDU_STATE_OPERATION)
            continue;
        _collect_retry_frames(ampdu_tid);
        ssv6200_ampdu_send_retry(sc->hw, ampdu_tid, &ampdu_tid->retry_queue, true);
        _flush_early_ampdu_q(sc, ampdu_tid);
        _flush_release_queue(sc->hw, &ampdu_tid->release_queue);
    }
}
void ssv6xxx_ampdu_postprocess_BA (struct ieee80211_hw *hw)
{
    struct ssv_softc *sc = hw->priv;
    ssv6xxx_foreach_sta(sc, _postprocess_BA, NULL);
}
#ifndef SSV_SUPPORT_HAL
static void ssv6200_hw_set_rx_ba_session (struct ssv_hw *sh, bool on, u8 *ta,
                                          u16 tid, u16 ssn, u8 buf_size)
{
    if (on)
    {
        u32 u32ta;
        u32ta = 0;
        u32ta |= (ta[0] & 0xff) << (8 * 0);
        u32ta |= (ta[1] & 0xff) << (8 * 1);
        u32ta |= (ta[2] & 0xff) << (8 * 2);
        u32ta |= (ta[3] & 0xff) << (8 * 3);
        SMAC_REG_WRITE(sh, ADR_BA_TA_0, u32ta);
        u32ta = 0;
        u32ta |= (ta[4] & 0xff) << (8 * 0);
        u32ta |= (ta[5] & 0xff) << (8 * 1);
        SMAC_REG_WRITE(sh, ADR_BA_TA_1, u32ta);
        SMAC_REG_WRITE(sh, ADR_BA_TID, tid);
        SMAC_REG_WRITE(sh, ADR_BA_ST_SEQ, ssn);
        SMAC_REG_WRITE(sh, ADR_BA_SB0, 0);
        SMAC_REG_WRITE(sh, ADR_BA_SB1, 0);
        SMAC_REG_WRITE(sh, ADR_BA_CTRL,
            (1 << BA_AGRE_EN_SFT)| (3 << BA_CTRL_SFT));
    } else {
        SMAC_REG_WRITE(sh, ADR_BA_CTRL, 0x0);
    }
}
#endif
void ssv6xxx_set_ampdu_rx_add_work (struct work_struct *work)
{
    struct ssv_softc
    *sc = container_of(work, struct ssv_softc, set_ampdu_rx_add_work);
    SSV_SET_RX_BA(sc->sh, true, sc->ba_ra_addr, sc->ba_tid, sc->ba_ssn, 64);
}
void ssv6xxx_set_ampdu_rx_del_work (struct work_struct *work)
{
    struct ssv_softc*sc = container_of(work, struct ssv_softc,
                                       set_ampdu_rx_del_work);
    u8 addr[6] = { 0 };
    SSV_SET_RX_BA(sc->sh, false, addr, 0, 0, 0);
}
#ifdef CONFIG_SSV6XXX_DEBUGFS
static void _reset_ampdu_mib (struct ssv_softc *sc, struct ssv_sta_info *sta_info, void *param)
{
    struct ieee80211_sta *sta = sta_info->sta;
    struct ssv_sta_priv_data *ssv_sta_priv;
    int i;
    ssv_sta_priv = (struct ssv_sta_priv_data *)sta->drv_priv;
    for (i = 0; i < WMM_TID_NUM; i++)
    {
        ssv_sta_priv->ampdu_tid[i].ampdu_mib_reset = 1;
    }
}
void ssv6xxx_ampdu_mib_reset (struct ieee80211_hw *hw)
{
    struct ssv_softc *sc = hw->priv;
    if (sc == NULL)
        return;
    ssv6xxx_foreach_sta(sc, _reset_ampdu_mib, NULL);
}
ssize_t ampdu_tx_mib_dump (struct ssv_sta_priv_data *ssv_sta_priv,
                           char *mib_str, ssize_t length)
{
    ssize_t buf_size = length;
    ssize_t prt_size;
    int j;
    struct ssv_softc *sc;
    struct ssv_sta_info *ssv_sta = ssv_sta_priv->sta_info, *first_ssv_sta;
    first_ssv_sta = ssv_sta - ssv_sta_priv->sta_idx;
    sc = container_of(first_ssv_sta, struct ssv_softc, sta_info[0]);
    down_read(&sc->sta_info_sem);
    if ((ssv_sta->s_flags & STA_FLAG_VALID) == 0) {
        goto mib_dump_exit;
    }
    if (ssv_sta->sta == NULL)
    {
        prt_size = snprintf(mib_str, buf_size, "\n    NULL STA.\n");
        mib_str += prt_size;
        buf_size -= prt_size;
        goto mib_dump_exit;
    }
    for (j = 0; j < WMM_TID_NUM; j++)
    {
        int k;
        struct AMPDU_TID_st *ampdu_tid = &ssv_sta_priv->ampdu_tid[j];
        struct AMPDU_MIB_st *ampdu_mib = &ampdu_tid->mib;
        prt_size = snprintf(mib_str, buf_size, "\n    WMM_TID %d@%d\n", j,
                            ampdu_tid->state);
        mib_str += prt_size;
        buf_size -= prt_size;
        if (ampdu_tid->state != AMPDU_STATE_OPERATION)
            continue;
        prt_size = snprintf(mib_str, buf_size, "        BA window size: %d\n",
                            ampdu_tid->ssv_baw_size);
        mib_str += prt_size;
        buf_size -= prt_size;
        prt_size = snprintf(mib_str, buf_size, "        BA window head: %d\n",
                            ampdu_tid->ssv_baw_head);
        mib_str += prt_size;
        buf_size -= prt_size;
        prt_size = snprintf(mib_str, buf_size,
                            "        Sending aggregated #: %d\n",
                            ampdu_tid->aggr_pkt_num);
        mib_str += prt_size;
        buf_size -= prt_size;
        prt_size = snprintf(
                mib_str, buf_size, "        Waiting #: %d\n",
                skb_queue_len(&ampdu_tid->ampdu_skb_tx_queue));
        mib_str += prt_size;
        buf_size -= prt_size;
        prt_size = snprintf(mib_str, buf_size, "        Early aggregated %d\n",
                            ampdu_tid->early_aggr_skb_num);
        mib_str += prt_size;
        buf_size -= prt_size;
        prt_size = snprintf(mib_str, buf_size,
                            "        MPDU: %d\n",
                            ampdu_mib->ampdu_mib_mpdu_counter);
        mib_str += prt_size;
        buf_size -= prt_size;
        prt_size = snprintf(mib_str, buf_size,
                            "        Passed: %d\n", ampdu_mib->ampdu_mib_pass_counter);
        mib_str += prt_size;
        buf_size -= prt_size;
        prt_size = snprintf(mib_str, buf_size,
                            "        Retry: %d\n",
                            ampdu_mib->ampdu_mib_retry_counter);
        mib_str += prt_size;
        buf_size -= prt_size;
        prt_size = snprintf(mib_str, buf_size,
                            "        AMPDU: %d\n",
                            ampdu_mib->ampdu_mib_ampdu_counter);
        mib_str += prt_size;
        buf_size -= prt_size;
        prt_size = snprintf(mib_str, buf_size,
                            "        Retry AMPDU: %d\n",
                            ampdu_mib->ampdu_mib_aggr_retry_counter);
        mib_str += prt_size;
        buf_size -= prt_size;
        prt_size = snprintf(mib_str, buf_size,
                            "        BAR count: %d\n",
                            ampdu_mib->ampdu_mib_bar_counter);
        mib_str += prt_size;
        buf_size -= prt_size;
        prt_size = snprintf(mib_str, buf_size,
                            "        Discard count: %d\n",
                            ampdu_mib->ampdu_mib_discard_counter);
        mib_str += prt_size;
        buf_size -= prt_size;
        prt_size = snprintf(mib_str, buf_size,
                            "        BA count: %d\n",
                            ampdu_mib->ampdu_mib_BA_counter);
        mib_str += prt_size;
        buf_size -= prt_size;
        prt_size = snprintf(mib_str, buf_size, "        Total BA count: %d\n",
                            ssv_sta_priv->ampdu_mib_total_BA_counter);
        mib_str += prt_size;
        buf_size -= prt_size;
        prt_size = snprintf(mib_str, buf_size, "        Aggr # count:\n");
        mib_str += prt_size;
        buf_size -= prt_size;
        for (k = 0; k <= SSV_AMPDU_aggr_num_max; k++)
        {
            prt_size = snprintf(mib_str, buf_size, "            %d: %d\n", k,
                                ampdu_mib->ampdu_mib_dist[k]);
            mib_str += prt_size;
            buf_size -= prt_size;
        }
    }
mib_dump_exit:
    up_read(&sc->sta_info_sem);
    return (length - buf_size);
}
#endif
#if 0
static void _dump_ampdu_mib (struct ssv_softc *sc, struct ssv_sta_info *sta_info, void *param)
{
    struct mib_dump_data *dump_data = (struct mib_dump_data *)param;
    struct ieee80211_sta *sta;
    struct ssv_sta_priv_data *ssv_sta_priv;
    ssize_t buf_size;
    ssize_t prt_size;
    char *mib_str = dump_data->prt_buff;
    if (param == NULL)
        return;
    buf_size = dump_data->buff_size - 1;
    sta = sta_info->sta;
    if ((sta == NULL) || ((sta_info->s_flags & STA_FLAG_VALID) == 0))
        return;
    prt_size = snprintf(mib_str, buf_size,
                        "STA: %02X-%02X-%02X-%02X-%02X-%02X:\n",
                        sta->addr[0], sta->addr[1], sta->addr[2],
                        sta->addr[3], sta->addr[4], sta->addr[5]);
    mib_str += prt_size;
    buf_size -= prt_size;
    ssv_sta_priv = (struct ssv_sta_priv_data *) sta->drv_priv;
    prt_size = ampdu_tx_mib_dump(ssv_sta_priv, mib_str, buf_size);
    mib_str += prt_size;
    buf_size -= prt_size;
    dump_data->prt_len = (dump_data->buff_size - 1 - buf_size);
    dump_data->prt_buff = mib_str;
    dump_data->buff_size = buf_size;
}
ssize_t ssv6xxx_ampdu_mib_dump (struct ieee80211_hw *hw, char *mib_str,
                                ssize_t length)
{
    struct ssv_softc *sc = hw->priv;
    ssize_t buf_size = length - 1;
    struct mib_dump_data dump_data = {mib_str, buf_size, 0};
    if (sc == NULL)
        return 0;
    ssv6xxx_foreach_sta(sc, _dump_ampdu_mib, &dump_data);
    return dump_data.prt_len;
}
#endif
struct sk_buff *_alloc_ampdu_skb (struct ssv_softc *sc, struct AMPDU_TID_st *ampdu_tid, u32 len)
{
    unsigned char *payload_addr;
    u32 headroom = sc->hw->extra_tx_headroom;
    u32 offset;
    struct ssv_sta_priv_data *ssv_sta_priv = (struct ssv_sta_priv_data *)ampdu_tid->sta->drv_priv;
    u32 cur_max_ampdu_size = ssv_sta_priv->max_ampdu_size;
    u32 extra_room = sc->sh->tx_desc_len * 2 + 48;
    u32 max_physical_len = (len && ((len + extra_room) < cur_max_ampdu_size))
                           ? (len + extra_room)
                           : cur_max_ampdu_size;
    u32 skb_len = max_physical_len + headroom + 3;
    struct sk_buff *ampdu_skb = __dev_alloc_skb(skb_len, GFP_KERNEL);
    struct ampdu_hdr_st *ampdu_hdr;
    if (ampdu_skb == NULL)
    {
        dev_err(sc->dev, "AMPDU allocation of size %d(%d) failed\n", len, skb_len);
        return NULL;
    }
    payload_addr = ampdu_skb->data + headroom - sc->sh->tx_desc_len;
    offset = ((size_t) payload_addr) % 4U;
    if (offset)
    {
        printk(KERN_ERR "Align AMPDU data %d\n", offset);
        skb_reserve(ampdu_skb, headroom + 4 - offset);
    }
    else
        skb_reserve(ampdu_skb, headroom);
    ampdu_hdr = (struct ampdu_hdr_st *) ampdu_skb->head;
    skb_queue_head_init(&ampdu_hdr->mpdu_q);
    ampdu_hdr->max_size = max_physical_len - extra_room;
    ampdu_hdr->size = 0;
    ampdu_hdr->ampdu_tid = ampdu_tid;
    memset(ampdu_hdr->ssn, 0xFF, sizeof(ampdu_hdr->ssn));
    ampdu_hdr->mpdu_num = 0;
    return ampdu_skb;
}
bool _is_skb_q_empty (struct ssv_softc *sc, struct sk_buff *skb)
{
    u32 ac = skb_get_queue_mapping(skb);
    u32 hw_txqid = sc->tx.hw_txqid[ac];
    return AMPDU_HCI_Q_EMPTY(sc->sh, hw_txqid);
}
static void _print_BA(struct AMPDU_TID_st *ampdu_tid)
{
    u16 i, head_ssn, ssn;
    struct sk_buff *skb;
    struct SKB_info_st *skb_info;
    head_ssn = ampdu_tid->ssv_baw_head;
    printk(" BA for tid %d:", ampdu_tid->tidno);
    for (i = 0; i < SSV_AMPDU_BA_WINDOW_SIZE; i ++){
        ssn = (head_ssn + i)% SSV_AMPDU_MAX_SSN;
        skb = INDEX_PKT_BY_SSN(ampdu_tid, ssn);
        if ((i % 8) == 0) {
            printk("\n");
        }
        if (skb == NULL){
            printk(" NULL   ");
            continue;
        }
        skb_info = (SKB_info *) (skb->head);
        switch (skb_info->ampdu_tx_status){
            case AMPDU_ST_NON_AMPDU:
                printk(" %4d Non", ssn);
                break;
            case AMPDU_ST_AGGREGATED:
                printk(" %4d Agg", ssn);
                break;
            case AMPDU_ST_SENT:
                printk(" %4d S  ", ssn);
                break;
            case AMPDU_ST_RETRY:
                printk(" %4d R  ", ssn);
                break;
            case AMPDU_ST_RETRY_Q:
                printk(" %4d Rq ", ssn);
                break;
            case AMPDU_ST_DROPPED:
                printk(" %4d Dr ", ssn);
                break;
            case AMPDU_ST_DONE:
                printk(" %4d Do ", ssn);
                break;
            default:
                break;
        }
    }
    printk("\n");
}
static u32 _check_timeout (struct AMPDU_TID_st *ampdu_tid, u32 *has_drop)
{
    u16 ssn, head_ssn, end_ssn;
    unsigned long check_jiffies = jiffies;
    u32 has_retry = 0;
    struct ssv_softc *sc = AMPDU_TID_TO_SC(ampdu_tid);
    bool check_BA = false;
    head_ssn = ampdu_tid->ssv_baw_head;
 ssn = head_ssn;
    if (ssn == SSV_ILLEGAL_SN)
        return 0;
    end_ssn = (head_ssn + SSV_AMPDU_BA_WINDOW_SIZE)
              % SSV_AMPDU_MAX_SSN;
    do {
        struct sk_buff *skb = INDEX_PKT_BY_SSN(ampdu_tid, ssn);
        struct SKB_info_st *skb_info;
        unsigned long cur_jiffies;
        unsigned long timeout_jiffies;
        u32 delta_ms;
        if (skb == NULL){
            INC_PKT_SN(ssn);
            continue;
        }
        skb_info = (SKB_info *) (skb->head);
        if (skb_info->ampdu_tx_status != AMPDU_ST_AGGREGATED){
            cur_jiffies = jiffies;
            timeout_jiffies = skb_info->aggr_timestamp + msecs_to_jiffies(BA_WAIT_TIMEOUT);
            if ( time_before(cur_jiffies, timeout_jiffies))
                break;
            delta_ms = jiffies_to_msecs((long)(cur_jiffies - skb_info->aggr_timestamp));
            prn_aggr_err(sc, "rt S%d-T%d-%d (%u) current state %d\n",
                         ((struct ssv_sta_priv_data *)skb_info->sta->drv_priv)->sta_idx,
                         ampdu_tid->tidno, ssn,
                         delta_ms, skb_info->ampdu_tx_status);
            if (delta_ms > 1000)
            {
                prn_aggr_err(sc, "Last checktime %lu - %lu = %u, cur_jiffies %lu\n",
                             check_jiffies, skb_info->aggr_timestamp,
                             jiffies_to_msecs((long)(check_jiffies - skb_info->aggr_timestamp)), cur_jiffies);
                check_BA = true;
            }
            ssv6xxx_mark_skb_retry(sc, skb_info, skb);
      if (skb_info->ampdu_tx_status == AMPDU_ST_RETRY)
                has_retry++;;
            if (skb_info->ampdu_tx_status == AMPDU_ST_DROPPED)
       (*has_drop)++;
     }
        INC_PKT_SN(ssn);
    } while (ssn != end_ssn);
    if ((check_BA) && (sc->log_ctrl & LOG_AMPDU_ERR))
        _print_BA(ampdu_tid);
    ampdu_tid->timestamp = check_jiffies;
    return has_retry;
}
void ssv6xxx_ampdu_check_timeout (struct ieee80211_hw *hw)
{
    struct ssv_softc *sc = hw->priv;
    struct AMPDU_TID_st *cur_AMPDU_TID;
    u32 has_retry = 0, has_drop = 0;
 bool tx_q_empty = true;
 int i;
    if (!list_empty(&sc->tx.ampdu_tx_que))
    {
        list_for_each_entry_rcu(cur_AMPDU_TID, &sc->tx.ampdu_tx_que, list)
        {
            has_retry = 0;
   has_drop = 0;
            if (cur_AMPDU_TID == NULL)
                return;
            if (cur_AMPDU_TID->state != AMPDU_STATE_OPERATION)
                continue;
            has_retry = _check_timeout(cur_AMPDU_TID, &has_drop);
            if (has_retry)
            {
                _collect_retry_frames(cur_AMPDU_TID);
                ssv6200_ampdu_send_retry(sc->hw, cur_AMPDU_TID, &cur_AMPDU_TID->retry_queue,
                                         true);
            }
   if (has_drop){
       ssv6xxx_release_frames(cur_AMPDU_TID);
      }
        }
    }
 for (i = 0; i < WMM_NUM_AC; i++) {
  if (AMPDU_HCI_Q_EMPTY(sc->sh, i)) {
   tx_q_empty = false;
   break;
  }
 }
 if (tx_q_empty == true)
  ssv6xxx_ampdu_flush(sc->hw);
}
void ssv6xxx_ampdu_sent(struct ieee80211_hw *hw, struct sk_buff *ampdu)
{
    struct ssv_softc *sc = hw->priv;
    struct ampdu_hdr_st *ampdu_hdr = (struct ampdu_hdr_st *) ampdu->head;
    struct sk_buff *mpdu;
    unsigned long cur_jiffies = jiffies;
    int i;
    SKB_info *mpdu_skb_info;
    u16 ssn;
    unsigned long flags;
    if (ampdu_hdr->ampdu_tid->state != AMPDU_STATE_OPERATION)
        return;
    spin_lock_irqsave(&ampdu_hdr->ampdu_tid->pkt_array_lock, flags);
    for (i = 0; i < ampdu_hdr->mpdu_num; i++)
    {
        ssn = ampdu_hdr->ssn[i];
        mpdu = INDEX_PKT_BY_SSN(ampdu_hdr->ampdu_tid, ssn);
        if (mpdu == NULL)
        {
            dev_err(sc->dev, "T%d-%d is a NULL MPDU.\n",
                    ampdu_hdr->ampdu_tid->tidno, ssn);
            continue;
        }
        if (ampdu_skb_ssn(mpdu) != ssn)
        {
            dev_err(sc->dev, "T%d-%d does not match %d MPDU.\n",
                    ampdu_hdr->ampdu_tid->tidno, ssn, ampdu_skb_ssn(mpdu));
            continue;
        }
        mpdu_skb_info = (SKB_info *) (mpdu->head);
        mpdu_skb_info->aggr_timestamp = cur_jiffies;
        if ((mpdu_skb_info->ampdu_tx_status != AMPDU_ST_DROPPED) ||
    (mpdu_skb_info->ampdu_tx_status != AMPDU_ST_DONE)){
   mpdu_skb_info->ampdu_tx_status = AMPDU_ST_SENT;
     }
    }
    spin_unlock_irqrestore(&ampdu_hdr->ampdu_tid->pkt_array_lock, flags);
}
