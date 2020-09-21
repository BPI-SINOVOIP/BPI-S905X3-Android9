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

#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/debugfs.h>
#include <ssv6200.h>
#include "dev.h"
#include "ssv_rc_minstrel.h"
#include <hal.h>
#include <linux_80211.h>
#define SSV_MINSTREL_ACK_LEN 39
#define SAMPLE_COLUMNS 10
#define SAMPLE_TBL(_sta_priv,_idx,_col) \
  _sta_priv->sample_table[(_idx * SAMPLE_COLUMNS) + _col]
int minstrel_ewma(int old, int new, int weight)
{
 return (new * (100 - weight) + old * weight) / 100;
}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,7,0)
int ieee80211_frame_duration(enum nl80211_band band, size_t len,
   int rate, int erp, int short_preamble)
#else
int ieee80211_frame_duration(enum ieee80211_band band, size_t len,
   int rate, int erp, int short_preamble)
#endif
{
 int dur;
 if (band == INDEX_80211_BAND_5GHZ || erp) {
  dur = 16;
  dur += 16;
  dur += 4;
  dur += 4 * DIV_ROUND_UP((16 + 8 * (len + 4) + 6) * 10, 4 * rate);
 } else {
  dur = 10;
  dur += short_preamble ? (72 + 24) : (144 + 48);
  dur += DIV_ROUND_UP(8 * (len + 4) * 10, rate);
 }
 return dur;
}
static void ssv_minstrel_update_stats(struct ssv_softc *sc, struct ssv_minstrel_priv *smp,
  struct ssv_minstrel_sta_priv *minstrel_sta_priv)
{
 struct ssv_minstrel_sta_info *smi = (struct ssv_minstrel_sta_info *)&minstrel_sta_priv->legacy;
 u32 max_tp = 0, index_max_tp = 0, index_max_tp2 = 0;
 u32 max_prob = 0, index_max_prob = 0;
 u32 usecs;
 int i;
 bool no_update = false;
 struct rc_setting *rc_setting = &sc->sh->cfg.rc_setting;
    for (i = 0; i < smi->n_rates; i++) {
        struct ssv_minstrel_rate *mr = &minstrel_sta_priv->ratelist[i];
        if (mr->attempts)
            break;
    }
    if (i == smi->n_rates){
        no_update = true;
    }
 smi->stats_update = jiffies;
 if (no_update == false){
     for (i = 0; i < smi->n_rates; i++) {
      struct ssv_minstrel_rate *mr = &minstrel_sta_priv->ratelist[i];
      usecs = mr->perfect_tx_time;
      if (!usecs)
       usecs = 1000000;
      if (mr->attempts) {
       mr->succ_hist += mr->success;
       mr->att_hist += mr->attempts;
       mr->cur_prob = MINSTREL_FRAC(mr->success, mr->attempts);
       if ((!mr->att_hist) || (mr->probability < MINSTREL_FRAC(10, 100)))
           mr->probability = mr->cur_prob;
          else
           mr->probability = minstrel_ewma(mr->probability, mr->cur_prob, EWMA_LEVEL);
       mr->cur_tp = mr->probability * (1000000 / usecs);
       mr->last_jiffies = jiffies;
            } else {
          if (time_after(jiffies, mr->last_jiffies + msecs_to_jiffies(rc_setting->aging_period))){
              if (sc->bScanning == false) {
                  mr->probability = minstrel_ewma(mr->probability, 0, EWMA_LEVEL);
              }
              mr->last_jiffies = jiffies;
                }
         }
      mr->last_success = mr->success;
      mr->last_attempts = mr->attempts;
      mr->success = 0;
      mr->attempts = 0;
      if ((mr->probability > MINSTREL_FRAC(95, 100)) || (mr->probability < MINSTREL_FRAC(10, 100))) {
       mr->adjusted_retry_count = mr->retry_count >> 1;
       if (mr->adjusted_retry_count > 2)
        mr->adjusted_retry_count = 2;
       mr->sample_limit = 4;
      } else {
       mr->sample_limit = -1;
       mr->adjusted_retry_count = mr->retry_count;
      }
      if (!mr->adjusted_retry_count)
       mr->adjusted_retry_count = 2;
     }
 }
 for (i = 0; i < smi->n_rates; i++) {
  struct ssv_minstrel_rate *mr = &minstrel_sta_priv->ratelist[i];
        if (mr->bitrate == 20)
            continue;
        if (max_tp < mr->cur_tp) {
   index_max_tp = i;
   max_tp = mr->cur_tp;
  }
  if (max_prob < mr->probability) {
   index_max_prob = i;
   max_prob = mr->probability;
  }
 }
 max_tp = 0;
 for (i = 0; i < smi->n_rates; i++) {
  struct ssv_minstrel_rate *mr = &minstrel_sta_priv->ratelist[i];
        if (mr->bitrate == 20)
            continue;
  if (i == index_max_tp)
   continue;
  if (max_tp < mr->cur_tp) {
   index_max_tp2 = i;
   max_tp = mr->cur_tp;
  }
 }
 smi->max_tp_rate = index_max_tp;
    if (smi->max_tp_rate > 1) {
        smi->max_tp_rate2 = smi->max_tp_rate - 1;
    } else {
        smi->max_tp_rate2 = 0;
   }
    if (smi->max_tp_rate2 > index_max_prob) {
     smi->max_prob_rate = index_max_prob;
    } else {
        if (smi->max_tp_rate2 > 1) {
            smi->max_prob_rate = smi->max_tp_rate2 - 1;
        } else {
            smi->max_prob_rate = 0;
     }
    }
 if (sc->sh->cfg.rc_log) {
  printk("%s():\n", __FUNCTION__);
  for (i = 0; i < smi->n_rates; i++) {
   struct ssv_minstrel_rate *mr = &minstrel_sta_priv->ratelist[i];
   printk("bitrate[%d], succ_hist=%llu, att_hist=%llu, cur_prob=%d, probability=%d, cur_tp=%d\n",
    mr->bitrate, mr->succ_hist, mr->att_hist, mr->cur_prob, mr->probability, mr->cur_tp);
  }
  printk("\n\n");
  printk("max_tp_rate=%d, max_tp_rate2=%d, max_prob_rate=%d\n\n",
   smi->max_tp_rate, smi->max_tp_rate2, smi->max_prob_rate);
 }
}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,7,0)
static void ssv_calc_rate_durations(enum nl80211_band band, struct ssv_minstrel_rate *d,
   struct ieee80211_rate *rate, int use_short_preamble)
#else
static void ssv_calc_rate_durations(enum ieee80211_band band, struct ssv_minstrel_rate *d,
   struct ieee80211_rate *rate, int use_short_preamble)
#endif
{
 int erp = !!(rate->flags & IEEE80211_RATE_ERP_G);
 int short_preamble = !!(rate->flags & IEEE80211_RATE_SHORT_PREAMBLE);
 d->perfect_tx_time = ieee80211_frame_duration(band, 1200, rate->bitrate,
    erp, (use_short_preamble && short_preamble));
 d->ack_time = ieee80211_frame_duration(band, SSV_MINSTREL_ACK_LEN, rate->bitrate,
    erp, (use_short_preamble && short_preamble));
}
static inline unsigned int minstrel_get_retry_count(struct ssv_minstrel_rate *mr,
    struct ieee80211_tx_info *info)
{
 unsigned int retry = mr->adjusted_retry_count;
 if (info->control.rates[0].flags & IEEE80211_TX_RC_USE_RTS_CTS)
  retry = max(2U, min(mr->retry_count_rtscts, retry));
 else if (info->control.rates[0].flags & IEEE80211_TX_RC_USE_CTS_PROTECT)
  retry = max(2U, min(mr->retry_count_cts, retry));
 return retry;
}
static int minstrel_get_next_sample(struct ssv_minstrel_sta_priv *minstrel_sta_priv)
{
 struct ssv_minstrel_sta_info *smi = (struct ssv_minstrel_sta_info *)&minstrel_sta_priv->legacy;
 unsigned int sample_ndx;
 sample_ndx = SAMPLE_TBL(minstrel_sta_priv, smi->sample_idx, smi->sample_column);
 smi->sample_idx++;
 if ((int) smi->sample_idx > (smi->n_rates - 2)) {
  smi->sample_idx = 0;
  smi->sample_column++;
  if (smi->sample_column >= SAMPLE_COLUMNS)
   smi->sample_column = 0;
 }
 return sample_ndx;
}
static void ssv_init_sample_table(struct ssv_minstrel_sta_priv *minstrel_sta_priv)
{
 struct ssv_minstrel_sta_info *smi = (struct ssv_minstrel_sta_info *)&minstrel_sta_priv->legacy;
 unsigned int i, col, new_idx;
 unsigned int n_srates = smi->n_rates - 1;
 u8 rnd[8];
 smi->sample_column = 0;
 smi->sample_idx = 0;
 memset(minstrel_sta_priv->sample_table, 0, SAMPLE_COLUMNS * smi->n_rates);
 for (col = 0; col < SAMPLE_COLUMNS; col++) {
  for (i = 0; i < n_srates; i++) {
   get_random_bytes(rnd, sizeof(rnd));
   new_idx = (i + rnd[i & 7]) % n_srates;
   while (SAMPLE_TBL(minstrel_sta_priv, new_idx, col) != 0)
    new_idx = (new_idx + 1) % n_srates;
   SAMPLE_TBL(minstrel_sta_priv, new_idx, col) = i + 1;
  }
 }
}
static void ssv_minstrel_tx_status(void *priv, struct ieee80211_supported_band *sband,
                   struct ieee80211_sta *sta, void *priv_sta,
     struct sk_buff *skb)
{
 struct ssv_softc *sc;
 struct ieee80211_hdr *hdr;
 struct ssv_minstrel_sta_priv *minstrel_sta_priv;
    struct ieee80211_tx_info *info = IEEE80211_SKB_CB(skb);
 __le16 fc;
 hdr = (struct ieee80211_hdr *)skb->data;
 fc = hdr->frame_control;
 if (!priv_sta || !ieee80211_is_data_qos(fc))
  return;
    minstrel_sta_priv = (struct ssv_minstrel_sta_priv *)priv_sta;
    if ((!minstrel_sta_priv->is_ht) ||
        (info->flags & IEEE80211_TX_CTL_AMPDU) ||
        (!minstrel_sta_priv->update_aggr_check))
        return;
 sc = (struct ssv_softc *)priv;
 if ( conf_is_ht(&sc->hw->conf)
  && (!(skb->protocol == cpu_to_be16(ETH_P_PAE))))
 {
  if (skb_get_queue_mapping(skb) != IEEE80211_AC_VO)
   ssv6200_ampdu_tx_update_state(priv, sta, skb);
 }
    minstrel_sta_priv->update_aggr_check = false;
 return;
}
void ssv_minstrel_set_fix_data_rate(struct ssv_softc *sc,
        struct ssv_minstrel_sta_priv *minstrel_sta_priv, struct ieee80211_tx_rate *ar)
{
    int i = 0;
    struct ssv_hw *sh = sc->sh;
    struct ieee80211_channel *curchan;
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,7,0)
    curchan = sc->hw->conf.channel;
#else
    curchan = sc->hw->conf.chandef.chan;
#endif
    for (i = 0; i < sc->hw->max_rates; i++) {
        ar[i].count = (sh->cfg.rc_retry_set >> i*4) & 0xF;
        switch (sh->cfg.rc_phy_mode) {
            case 3:
                minstrel_sta_priv->update_aggr_check = true;
                ar[i].idx = (sh->cfg.rc_rate_idx_set >> i* 4) & 0x7;
                ar[i].flags = IEEE80211_TX_RC_MCS;
                if (sh->cfg.rc_long_short)
                    ar[i].flags |= IEEE80211_TX_RC_SHORT_GI;
                if (sh->cfg.rc_ht40)
                    ar[i].flags |= IEEE80211_TX_RC_40_MHZ_WIDTH;
             if (sh->cfg.rc_mf)
                    ar[i].flags |= IEEE80211_TX_RC_GREEN_FIELD;
                break;
            case 2:
                if (curchan->band == INDEX_80211_BAND_2GHZ)
                    ar[i].idx = 4 + ((sh->cfg.rc_rate_idx_set >> i* 4) & 0x7);
                else
                    ar[i].idx = ((sh->cfg.rc_rate_idx_set >> i* 4) & 0x7);
                break;
            case 0:
                ar[i].idx = ((sh->cfg.rc_rate_idx_set >> i* 4) & 0x3);
                break;
            default:
                ar[i].idx = 0;
                break;
        }
    }
}
static void ssv_minstrel_get_rate(void *priv, struct ieee80211_sta *sta,
    void *priv_sta, struct ieee80211_tx_rate_control *txrc)
{
 struct sk_buff *skb = txrc->skb;
 struct ieee80211_tx_info *info = IEEE80211_SKB_CB(skb);
 struct ssv_minstrel_sta_priv *minstrel_sta_priv = priv_sta;
 struct ssv_minstrel_sta_info *smi;
 struct ssv_softc *sc = priv;
 struct ssv_minstrel_priv *smp = (struct ssv_minstrel_priv *)sc->rc;
 struct ieee80211_tx_rate *ar = info->control.rates;
 unsigned int ndx, sample_ndx = 0;
 struct ssv_minstrel_rate *msr;
 bool mrr;
 bool sample_slower = false;
 bool sample = false;
 int i;
 int mrr_ndx[3];
 int sample_rate;
 u64 delta;
 struct rc_setting *rc_setting = &sc->sh->cfg.rc_setting;
 int force_sample_pr;
 if (rate_control_send_low(sta, priv_sta, txrc))
  return;
    if (sc->sh->cfg.auto_rate_enable == false) {
        ssv_minstrel_set_fix_data_rate(sc, minstrel_sta_priv, ar);
        return;
    }
 if (minstrel_sta_priv && minstrel_sta_priv->is_ht) {
  ssv_minstrel_ht_get_rate(priv, sta, priv_sta, txrc);
  return;
 }
 smi = (struct ssv_minstrel_sta_info *)&minstrel_sta_priv->legacy;
 mrr = smp->has_mrr;
 if (time_after(jiffies, smi->stats_update + (smp->update_interval * HZ) / 1000))
  ssv_minstrel_update_stats(sc, smp, minstrel_sta_priv);
 ndx = smi->max_tp_rate;
 if (mrr)
  sample_rate = smp->lookaround_rate_mrr;
 else
  sample_rate = smp->lookaround_rate;
 smi->packet_count++;
 delta = div_u64(smi->packet_count * sample_rate, 100) -
   div_u64(smi->sample_count + smi->sample_deferred, 2);
 if ((delta > 0) && (mrr || !smi->prev_sample)) {
  if (smi->packet_count >= 10000) {
   smi->sample_deferred = 0;
   smi->sample_count = 0;
   smi->packet_count = 0;
  } else if (delta > smi->n_rates * 2) {
   smi->sample_count += (delta - smi->n_rates * 2);
  }
  sample_ndx = minstrel_get_next_sample(minstrel_sta_priv);
  msr = &minstrel_sta_priv->ratelist[sample_ndx];
  sample = true;
  sample_slower = mrr && (msr->perfect_tx_time > minstrel_sta_priv->ratelist[ndx].perfect_tx_time);
  if (!sample_slower) {
   if (msr->sample_limit != 0) {
    ndx = sample_ndx;
    smi->sample_count++;
    if (msr->sample_limit > 0)
     msr->sample_limit--;
   }
   else
    sample = false;
  } else {
   info->flags |= IEEE80211_TX_CTL_RATE_CTRL_PROBE;
   smi->sample_deferred++;
  }
 }
 smi->prev_sample = sample;
    if (sc->primary_edca_mib > 10)
        force_sample_pr = rc_setting->force_sample_pr;
    else
        force_sample_pr = 10;
 if ((sample) && ((sample_slower) ||
     ((minstrel_sta_priv->ratelist[sample_ndx].probability > MINSTREL_FRAC(force_sample_pr, 100)) &&
     (minstrel_sta_priv->ratelist[sample_ndx].probability < MINSTREL_FRAC(rc_setting->up_pr, 100)))) )
  ndx = smi->max_tp_rate;
    if ((sample)&&
       ((time_before(jiffies,
        minstrel_sta_priv->ratelist[sample_ndx].last_jiffies + msecs_to_jiffies(500)) &&
        minstrel_sta_priv->ratelist[sample_ndx].probability < MINSTREL_FRAC(force_sample_pr, 100))))
        ndx = smi->max_tp_rate;
 ar[0].idx = minstrel_sta_priv->ratelist[ndx].rix;
 ar[0].count = minstrel_get_retry_count(&minstrel_sta_priv->ratelist[ndx], info);
#if 0
 if (sample) {
  if (sample_slower)
   mrr_ndx[0] = sample_ndx;
  else
   mrr_ndx[0] = smi->max_tp_rate;
 } else {
  mrr_ndx[0] = smi->max_tp_rate2;
 }
#endif
 mrr_ndx[0] = smi->max_tp_rate2;
 mrr_ndx[1] = smi->max_prob_rate;
 mrr_ndx[2] = 0;
 for (i = 1; i < 4; i++) {
  ar[i].idx = minstrel_sta_priv->ratelist[mrr_ndx[i - 1]].rix;
  ar[i].count = minstrel_sta_priv->ratelist[mrr_ndx[i - 1]].adjusted_retry_count;
 }
 dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_RATE_CONTROL,
     "%s():\nar[0].idx=%02x, ar[0].flags=%02x, ar[0].count=%d\n"
  "ar[1].idx=%02x, ar[1].flags=%02x, ar[1].count=%d\n"
  "ar[2].idx=%02x, ar[2].flags=%02x, ar[2].count=%d\n"
  "ar[3].idx=%02x, ar[3].flags=%02x, ar[3].count=%d\n",
  __FUNCTION__, ar[0].idx, ar[0].flags, ar[0].count, ar[1].idx, ar[1].flags, ar[1].count,
  ar[2].idx, ar[2].flags, ar[2].count, ar[3].idx, ar[3].flags, ar[3].count);
}
static void ssv6xxx_rate_update_minstrel_type(void *priv, struct ieee80211_supported_band *sband,
    struct ieee80211_sta *sta, void *priv_sta, enum nl80211_channel_type oper_chan_type)
{
 struct ssv_minstrel_sta_priv *minstrel_sta_priv = priv_sta;
 struct ssv_minstrel_sta_info *smi;
 struct ssv_softc *sc = priv;
 struct ssv_minstrel_priv *smp = (struct ssv_minstrel_priv *)sc->rc;
 struct ieee80211_rate *ctl_rate;
 unsigned int i, t_slot, n = 0;
 int use_short_preamble = 0;
 u8 drate_desc = 0;
 bool supportted_11m = false;
 minstrel_sta_priv->sta = sta;
 minstrel_sta_priv->is_ht = sta->ht_cap.ht_supported;
 if (minstrel_sta_priv->is_ht) {
  ssv_minstrel_ht_update_caps(priv, sband, sta, priv_sta, oper_chan_type);
  return;
 }
 smi = (struct ssv_minstrel_sta_info *)&minstrel_sta_priv->legacy;
 if (sc->sc_flags & SC_OP_SHORT_PREAMBLE)
  use_short_preamble = 1;
 smi->lowest_rix = rate_lowest_index(sband, sta);
 ctl_rate = &sband->bitrates[smi->lowest_rix];
 smi->sp_ack_dur = ieee80211_frame_duration(sband->band, SSV_MINSTREL_ACK_LEN, ctl_rate->bitrate,
     !!(ctl_rate->flags & IEEE80211_RATE_ERP_G),
     (use_short_preamble && (ctl_rate->flags & IEEE80211_RATE_SHORT_PREAMBLE)));
 smi->g_rates_offset = 0;
 for (i = 0; i < sband->n_bitrates; i++) {
  struct ssv_minstrel_rate *mr = &minstrel_sta_priv->ratelist[n];
  unsigned int tx_time = 0, tx_time_cts = 0, tx_time_rtscts = 0;
  unsigned int tx_time_single;
  unsigned int cw = smp->cw_min;
  if (!rate_supported(sta, sband->band, i))
   continue;
  if (supportted_11m) {
      if ((sband->bitrates[i].bitrate == 60) || (sband->bitrates[i].bitrate == 90))
          continue;
        }
  n++;
  memset(mr, 0, sizeof(*mr));
  mr->rix = i;
  mr->bitrate = sband->bitrates[i].bitrate;
  mr->flags = sband->bitrates[i].flags;
  SSV_RC_LEGACY_BITRATE_TO_RATE_DESC(sc, mr->bitrate, &drate_desc);
  mr->hw_rate_desc = drate_desc;
  if ((mr->bitrate == 10) || (mr->bitrate == 20) || (mr->bitrate == 55) || (mr->bitrate == 110))
   smi->g_rates_offset++;
        if (mr->bitrate == 110)
            supportted_11m = true;
  ssv_calc_rate_durations(sband->band, mr, &sband->bitrates[i], use_short_preamble);
  if (use_short_preamble && (mr->flags & IEEE80211_RATE_SHORT_PREAMBLE))
   t_slot = 9;
  else
   t_slot = 20;
  mr->sample_limit = -1;
  mr->retry_count = 1;
  mr->retry_count_cts = 1;
  mr->retry_count_rtscts = 1;
  tx_time = mr->perfect_tx_time + smi->sp_ack_dur;
  do {
   tx_time_single = mr->ack_time + mr->perfect_tx_time;
   tx_time_single += (t_slot * cw) >> 1;
   cw = min((cw << 1) | 1, smp->cw_max);
   tx_time += tx_time_single;
   tx_time_cts += tx_time_single + smi->sp_ack_dur;
   tx_time_rtscts += tx_time_single + 2 * smi->sp_ack_dur;
   if ((tx_time_cts < smp->segment_size) && (mr->retry_count_cts < smp->max_retry))
    mr->retry_count_cts++;
   if ((tx_time_rtscts < smp->segment_size) && (mr->retry_count_rtscts < smp->max_retry))
    mr->retry_count_rtscts++;
  } while ((tx_time < smp->segment_size) && (++mr->retry_count < smp->max_retry));
  mr->adjusted_retry_count = mr->retry_count;
 }
 for (i = n; i < sband->n_bitrates; i++) {
  struct ssv_minstrel_rate *mr = &minstrel_sta_priv->ratelist[i];
  mr->rix = -1;
 }
 smi->n_rates = n;
 smi->stats_update = jiffies;
 ssv_init_sample_table(minstrel_sta_priv);
}
#if LINUX_VERSION_CODE > 0x030500
static void ssv_minstrel_rate_update(void *priv, struct ieee80211_supported_band *sband,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,13,0)
                struct cfg80211_chan_def *chandef,
#endif
    struct ieee80211_sta *sta, void *priv_sta,
    u32 changed)
#else
static void ssv_minstrel_rate_update(void *priv, struct ieee80211_supported_band *sband,
    struct ieee80211_sta *sta, void *priv_sta,
    u32 changed, enum nl80211_channel_type oper_chan_type)
#endif
{
#if LINUX_VERSION_CODE > 0x030500
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,13,0)
 ssv6xxx_rate_update_minstrel_type(priv, sband, sta, priv_sta, cfg80211_get_chandef_type(chandef));
#else
 struct ssv_softc *sc = priv;
 struct ieee80211_hw *hw = sc->hw;
 ssv6xxx_rate_update_minstrel_type(priv, sband, sta, priv_sta, cfg80211_get_chandef_type(&(hw->conf.chandef)));
#endif
#else
 ssv6xxx_rate_update_minstrel_type(priv, sband, sta, priv_sta, oper_chan_type);
#endif
}
static void ssv_minstrel_rate_init(void *priv, struct ieee80211_supported_band *sband,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,13,0)
                struct cfg80211_chan_def *chandef,
#endif
    struct ieee80211_sta *sta, void *priv_sta)
{
#if LINUX_VERSION_CODE > 0x030500
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,13,0)
 ssv6xxx_rate_update_minstrel_type(priv, sband, sta, priv_sta, cfg80211_get_chandef_type(chandef));
#else
 struct ssv_softc *sc = priv;
 struct ieee80211_hw *hw = sc->hw;
 ssv6xxx_rate_update_minstrel_type(priv, sband, sta, priv_sta, cfg80211_get_chandef_type(&(hw->conf.chandef)));
#endif
#else
 struct ssv_softc *sc = priv;
 struct ieee80211_hw *hw = sc->hw;
 ssv6xxx_rate_update_minstrel_type(priv, sband, sta, priv_sta, hw->conf.channel_type);
#endif
}
static void *ssv_minstrel_alloc_sta(void *priv, struct ieee80211_sta *sta, gfp_t gfp)
{
 struct ieee80211_supported_band *sband;
 struct ssv_minstrel_sta_priv *minstrel_sta_priv;
 struct ssv_sta_priv_data *sta_priv;
 struct ssv_softc *sc = priv;
 struct ieee80211_hw *hw = sc->hw;
 int max_rates = 0;
 int i;
 minstrel_sta_priv = kzalloc(sizeof(struct ssv_minstrel_sta_priv), gfp);
 if (!minstrel_sta_priv)
  return NULL;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,7,0)
 for (i = 0; i < NUM_NL80211_BANDS; i++) {
#else
 for (i = 0; i < IEEE80211_NUM_BANDS; i++) {
#endif
  sband = hw->wiphy->bands[i];
  if (sband && sband->n_bitrates > max_rates)
   max_rates = sband->n_bitrates;
 }
 minstrel_sta_priv->ratelist = kzalloc(sizeof(struct ssv_minstrel_rate) * max_rates, gfp);
 if (!minstrel_sta_priv->ratelist)
  goto error;
 minstrel_sta_priv->sample_table = kmalloc(SAMPLE_COLUMNS * max_rates, gfp);
 if (!minstrel_sta_priv->sample_table)
  goto error1;
 sta_priv = (struct ssv_sta_priv_data *)sta->drv_priv;
 sta_priv->rc_info = minstrel_sta_priv;
 return minstrel_sta_priv;
error1:
 kfree(minstrel_sta_priv);
error:
 return NULL;
}
static void ssv_minstrel_free_sta(void *priv, struct ieee80211_sta *sta, void *priv_sta)
{
 struct ssv_minstrel_sta_priv *minstrel_sta_priv = priv_sta;
 kfree(minstrel_sta_priv->sample_table);
 kfree(minstrel_sta_priv->ratelist);
 kfree(minstrel_sta_priv);
}
static void *ssv_minstrel_alloc(struct ieee80211_hw *hw, struct dentry *debugfsdir)
{
 struct ssv_softc *sc = hw->priv;
 struct ssv_minstrel_priv *smp;
 sc->rc = kzalloc(sizeof(struct ssv_minstrel_priv), GFP_ATOMIC);
 if (!sc->rc)
  return NULL;
 memset(sc->rc, 0, sizeof(struct ssv_minstrel_priv));
 smp = (struct ssv_minstrel_priv *)sc->rc;
 smp->cw_min = 15;
 smp->cw_max = 1023;
 smp->lookaround_rate = 5;
 smp->lookaround_rate_mrr = 10;
 smp->segment_size = 6000;
 smp->max_retry = hw->max_rate_tries;
 smp->max_rates = hw->max_rates;
 smp->has_mrr = true;
 smp->update_interval = 100;
 return hw->priv;
}
static void ssv_minstrel_free(void *priv)
{
 struct ssv_softc *sc = priv;
 if (sc->rc) {
  kfree(sc->rc);
  sc->rc = NULL;
 }
}
#ifdef CONFIG_MAC80211_DEBUGFS
static int ssv_minstrel_stats_open(struct inode *inode, struct file *file)
{
 struct ssv_minstrel_sta_priv *minstrel_sta_priv = inode->i_private;
 struct ssv_minstrel_sta_info *legacy;
 struct ssv_minstrel_ht_sta *ht;
 struct ssv_minstrel_ht_mcs_group_data *mg;
 struct ssv_minstrel_debugfs_info *ms;
 unsigned int i, j, tp, prob, eprob;
 char *p;
    unsigned int max_mcs = MINSTREL_MAX_STREAMS * MINSTREL_STREAM_GROUPS;
    int bitrates[4] = { 10, 20, 55, 110 };
    int max_ht_groups = 0;
 ms = kmalloc(sizeof(*ms) + 4096, GFP_KERNEL);
 if (!ms)
  return -ENOMEM;
 file->private_data = ms;
 p = ms->buf;
 if (!minstrel_sta_priv->is_ht) {
  legacy = (struct ssv_minstrel_sta_info *)&minstrel_sta_priv->legacy;
  p += sprintf(p, "rate     throughput  ewma prob   this prob  "
    "this succ/attempt   success    attempts\n");
  for (i = 0; i < legacy->n_rates; i++) {
   struct ssv_minstrel_rate *mr = &minstrel_sta_priv->ratelist[i];
   *(p++) = (i == legacy->max_tp_rate) ? 'T' : ' ';
   *(p++) = (i == legacy->max_tp_rate2) ? 't' : ' ';
   *(p++) = (i == legacy->max_prob_rate) ? 'P' : ' ';
   p += sprintf(p, "%3u%s", mr->bitrate / 10, (mr->bitrate & 1 ? ".5" : "  "));
   tp = mr->cur_tp/(1024*1024);
   prob = MINSTREL_TRUNC(mr->cur_prob * 1000);
   eprob = MINSTREL_TRUNC(mr->probability * 1000);
   p += sprintf(p, "  %6u.%1u   %6u.%1u   %6u.%1u        "
     "%3u(%3u)   %8llu    %8llu\n",
     tp / 10, tp % 10,
     eprob / 10, eprob % 10,
     prob / 10, prob % 10,
     mr->last_success,
     mr->last_attempts,
     (unsigned long long)mr->succ_hist,
     (unsigned long long)mr->att_hist);
  }
  p += sprintf(p, "\nTotal packet count::    total %llu      lookaround %llu\n\n",
    legacy->packet_count, legacy->sample_count);
 } else {
  ht = (struct ssv_minstrel_ht_sta *)&minstrel_sta_priv->ht;
  p += sprintf(p, "%4s %8s %12s %12s %12s %20s %10s %10s\n",
                "type", "rate", "throughput", "ewma prob", "this prob", "this succ/attempt", "success", "attempts");
        max_ht_groups = MINSTREL_MAX_STREAMS * MINSTREL_STREAM_GROUPS + 1;
  for (i = 0; i < max_ht_groups; i++) {
   mg = &ht->groups[i];
   if (!mg->supported)
    continue;
            for (j = 0; j < MCS_GROUP_RATES; j++) {
    struct ssv_minstrel_ht_rate_stats *mhr = &ht->groups[i].rates[j];
    int idx = i * MCS_GROUP_RATES + j;
    if (!(ht->groups[i].supported & BIT(j)))
     continue;
    *(p++) = ' ';
    *(p++) = (idx == ht->max_tp_rate) ? 'T' : ' ';
    *(p++) = (idx == ht->max_tp_rate2) ? 't' : ' ';
    *(p++) = (idx == ht->max_prob_rate) ? 'P' : ' ';
                if (i == max_mcs) {
                    int r = bitrates[j % 4];
                    p += sprintf(p, "  %2u.%1uM/%c", r / 10, r % 10, (ht->cck_supported_short ? 'S' : 'L'));
                } else
                    p += sprintf(p, "    MCS%-2u", j);
    tp = mhr->cur_tp / 10;
    prob = MINSTREL_TRUNC(mhr->cur_prob * 1000);
    eprob = MINSTREL_TRUNC(mhr->probability * 1000);
    p += sprintf(p, "     %6u.%1u     %6u.%1u     %6u.%1u            %3u(%3u)    %8llu   %8llu\n",
      tp / 10, tp % 10,
      eprob / 10, eprob % 10,
      prob / 10, prob % 10,
      mhr->last_success,
      mhr->last_attempts,
      (unsigned long long)mhr->succ_hist,
      (unsigned long long)mhr->att_hist);
   }
  }
  p += sprintf(p, "\nTotal packet count::    total %llu      lookaround %llu, short GI state = %d\n",
    ht->total_packets, ht->sample_packets, ht->sgi_state);
 }
 ms->len = p - ms->buf;
 return 0;
}
static ssize_t ssv_minstrel_stats_read(struct file *file, char __user *buf, size_t len, loff_t *ppos)
{
 struct ssv_minstrel_debugfs_info *ms;
 ms = file->private_data;
 return simple_read_from_buffer(buf, len, ppos, ms->buf, ms->len);
}
static int ssv_minstrel_stats_release(struct inode *inode, struct file *file)
{
 kfree(file->private_data);
 return 0;
}
static const struct file_operations ssv_minstrel_stat_fops = {
 .open = ssv_minstrel_stats_open,
 .read = ssv_minstrel_stats_read,
 .release = ssv_minstrel_stats_release,
 .llseek = no_llseek,
};
static void ssv_minstrel_add_sta_debugfs(void *priv, void *priv_sta, struct dentry *dir)
{
 struct ssv_minstrel_sta_priv *minstrel_sta_priv = priv_sta;
 minstrel_sta_priv->dbg_stats = debugfs_create_file("rc_stats", S_IRUGO, dir,
   minstrel_sta_priv, &ssv_minstrel_stat_fops);
}
static void ssv_minstrel_remove_sta_debugfs(void *priv, void *priv_sta)
{
 struct ssv_minstrel_sta_priv *minstrel_sta_priv = priv_sta;
 debugfs_remove(minstrel_sta_priv->dbg_stats);
}
#endif
struct rate_control_ops ssv_minstrel = {
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,15,0)
 .module = NULL,
#endif
 .name = "ssv_minstrel",
 .tx_status = ssv_minstrel_tx_status,
 .get_rate = ssv_minstrel_get_rate,
 .rate_update = ssv_minstrel_rate_update,
 .rate_init = ssv_minstrel_rate_init,
 .alloc = ssv_minstrel_alloc,
 .free = ssv_minstrel_free,
 .alloc_sta = ssv_minstrel_alloc_sta,
 .free_sta = ssv_minstrel_free_sta,
#ifdef CONFIG_MAC80211_DEBUGFS
 .add_sta_debugfs = ssv_minstrel_add_sta_debugfs,
 .remove_sta_debugfs = ssv_minstrel_remove_sta_debugfs,
#endif
};
int ssv6xxx_minstrel_rate_control_register(void)
{
 ssv_minstrel_ht_init_sample_table();
 return ieee80211_rate_control_register(&ssv_minstrel);
}
void ssv6xxx_minstrel_rate_control_unregister(void)
{
 ieee80211_rate_control_unregister(&ssv_minstrel);
}
