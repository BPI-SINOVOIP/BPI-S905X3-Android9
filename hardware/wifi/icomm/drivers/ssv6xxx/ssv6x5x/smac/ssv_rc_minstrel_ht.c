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
#include <linux/types.h>
#include <linux/random.h>
#include <linux/math64.h>
#include <ssv6200.h>
#include "ssv_skb.h"
#include "dev.h"
#include <hal.h>
#include "ssv_rc_minstrel.h"
#include "ssv_rc_minstrel_ht.h"
#include <linux_80211.h>
#include "ampdu.h"
#define AVG_PKT_SIZE 1200
#define SAMPLE_COLUMNS 10
#define MCS_NBITS (AVG_PKT_SIZE << 3)
#define MCS_NSYMS(bps) ((MCS_NBITS + (bps) - 1) / (bps))
#define MCS_SYMBOL_TIME(sgi,syms) \
 (sgi ? \
   ((syms) * 18000 + 4000) / 5 : \
   ((syms) * 1000) << 2 \
 )
#define MCS_DURATION(streams,sgi,bps) MCS_SYMBOL_TIME(sgi, MCS_NSYMS((streams) * (bps)))
#define GROUP_IDX(_streams,_sgi,_ht40) \
 MINSTREL_MAX_STREAMS * 2 * _ht40 + \
 MINSTREL_MAX_STREAMS * _sgi + \
 _streams - 1
#define MCS_GROUP(_streams,_sgi,_ht40) \
 [GROUP_IDX(_streams, _sgi, _ht40)] = { \
  .streams = _streams, \
  .flags = \
   (_sgi ? IEEE80211_TX_RC_SHORT_GI : 0) | \
   (_ht40 ? IEEE80211_TX_RC_40_MHZ_WIDTH : 0), \
  .duration = { \
   MCS_DURATION(_streams, _sgi, _ht40 ? 54 : 26), \
   MCS_DURATION(_streams, _sgi, _ht40 ? 108 : 52), \
   MCS_DURATION(_streams, _sgi, _ht40 ? 162 : 78), \
   MCS_DURATION(_streams, _sgi, _ht40 ? 216 : 104), \
   MCS_DURATION(_streams, _sgi, _ht40 ? 324 : 156), \
   MCS_DURATION(_streams, _sgi, _ht40 ? 432 : 208), \
   MCS_DURATION(_streams, _sgi, _ht40 ? 486 : 234), \
   MCS_DURATION(_streams, _sgi, _ht40 ? 540 : 260) \
  } \
    }
#define CCK_DURATION(_bitrate,_short,_len) \
    (1000 * (10 + \
     (_short ? 72 + 24 : 144 + 48 ) + \
     (8 * (_len + 4) * 10) / (_bitrate)))
#define CCK_ACK_DURATION(_bitrate,_short) \
    (CCK_DURATION((_bitrate > 10 ? 20 : 10), false, 60) + \
     CCK_DURATION(_bitrate, _short, AVG_PKT_SIZE))
#define CCK_DURATION_LIST(_short) \
    CCK_ACK_DURATION(10, _short), \
    CCK_ACK_DURATION(20, _short), \
    CCK_ACK_DURATION(55, _short), \
    CCK_ACK_DURATION(110, _short)
#define CCK_GROUP \
    [MINSTREL_MAX_STREAMS * MINSTREL_STREAM_GROUPS] = { \
        .streams = 0, \
        .duration = { \
            CCK_DURATION_LIST(false), \
            CCK_DURATION_LIST(true) \
  } \
    }
const struct ssv_mcs_ht_group ssv_minstrel_mcs_groups[] = {
 MCS_GROUP(1, 0, 0),
 MCS_GROUP(1, 1, 0),
 MCS_GROUP(1, 0, 1),
 MCS_GROUP(1, 1, 1),
    CCK_GROUP
};
#define SSV_MINSTREL_CCK_GROUP (ARRAY_SIZE(ssv_minstrel_mcs_groups) - 1)
#define SSV_MINSTREL_STA_GROUP(_idx) (_idx/MCS_GROUP_RATES)
static u8 ssv_minstrel_ht_sample_table[SAMPLE_COLUMNS][MCS_GROUP_RATES];
static inline struct ssv_minstrel_ht_rate_stats *
   ssv_minstrel_ht_get_ratestats(struct ssv_minstrel_ht_sta *mhs, int index)
{
 return &mhs->groups[index / MCS_GROUP_RATES].rates[index % MCS_GROUP_RATES];
}
void ssv_minstrel_ht_init_sample_table(void)
{
 int col, i, new_idx;
 u8 rnd[MCS_GROUP_RATES];
 memset(ssv_minstrel_ht_sample_table, 0xff, sizeof(ssv_minstrel_ht_sample_table));
 for (col = 0; col < SAMPLE_COLUMNS; col++) {
   for (i = 0; i < MCS_GROUP_RATES; i++) {
    get_random_bytes(rnd, sizeof(rnd));
   new_idx = (i + rnd[i]) % MCS_GROUP_RATES;
   while (ssv_minstrel_ht_sample_table[col][new_idx] != 0xff)
    new_idx = (new_idx + 1) % MCS_GROUP_RATES;
   ssv_minstrel_ht_sample_table[col][new_idx] = i;
   }
 }
}
static void ssv_minstrel_ht_calc_rate_ewma(struct ssv_softc *sc, struct ssv_minstrel_ht_rate_stats *mr)
{
    struct rc_setting *rc_setting = &sc->sh->cfg.rc_setting;
 if (unlikely(mr->attempts > 0)) {
  mr->sample_skipped = 0;
  mr->cur_prob = MINSTREL_FRAC(mr->success, mr->attempts);
  if ((!mr->att_hist) || (mr->probability < MINSTREL_FRAC(10, 100)))
   mr->probability = mr->cur_prob;
  else
   mr->probability = minstrel_ewma(mr->probability, mr->cur_prob, EWMA_LEVEL);
  mr->att_hist += mr->attempts;
  mr->succ_hist += mr->success;
  mr->last_jiffies = jiffies;
 } else {
  mr->sample_skipped++;
  if (time_after(jiffies, mr->last_jiffies + msecs_to_jiffies(rc_setting->aging_period))){
      mr->probability = minstrel_ewma(mr->probability, 0, EWMA_LEVEL);
      mr->last_jiffies = jiffies;
        }
 }
 mr->last_success = mr->success;
 mr->last_attempts = mr->attempts;
 mr->success = 0;
 mr->attempts = 0;
}
static void ssv_minstrel_ht_calc_tp(struct ssv_minstrel_ht_sta *mhs, int group, int rate)
{
 struct ssv_minstrel_ht_rate_stats *mr;
 unsigned int nsecs = 0;
 u64 s = 1000000;
 mr = &mhs->groups[group].rates[rate];
 if (mr->probability < MINSTREL_FRAC(1, 10)) {
  mr->cur_tp = 0;
  return;
 }
    if (group != SSV_MINSTREL_CCK_GROUP)
        nsecs = (MINSTREL_TRUNC(mhs->avg_ampdu_len)!= 0) ? (1000 * mhs->overhead / MINSTREL_TRUNC(mhs->avg_ampdu_len)) : 0 ;
 nsecs += ssv_minstrel_mcs_groups[group].duration[rate];
 mr->cur_tp = (nsecs != 0 ) ? (u32)(MINSTREL_TRUNC(div_u64((s * mr->probability * 1000), nsecs))) : 0;
}
static void ssv_minstrel_ht_calc_retransmit(struct ssv_minstrel_priv *smp,
   struct ssv_minstrel_ht_sta *mhs, int index)
{
 struct ssv_minstrel_ht_rate_stats *mr;
 const struct ssv_mcs_ht_group *group;
 unsigned int tx_time, tx_time_rtscts, tx_time_data;
 unsigned int cw = smp->cw_min;
 unsigned int ctime = 0;
 unsigned int t_slot = 9;
 unsigned int ampdu_len = MINSTREL_TRUNC(mhs->avg_ampdu_len);
 mr = ssv_minstrel_ht_get_ratestats(mhs, index);
 if (mr->probability < MINSTREL_FRAC(1, 10)) {
  mr->retry_count = 1;
  mr->retry_count_rtscts = 1;
  return;
 }
 mr->retry_count = 2;
 mr->retry_count_rtscts = 2;
 mr->retry_updated = true;
 group = &ssv_minstrel_mcs_groups[index / MCS_GROUP_RATES];
 tx_time_data = group->duration[index % MCS_GROUP_RATES] * ampdu_len;
 ctime = (t_slot * cw) >> 1;
 cw = min((cw << 1) | 1, smp->cw_max);
 ctime += (t_slot * cw) >> 1;
 cw = min((cw << 1) | 1, smp->cw_max);
 tx_time = ctime + 2 * (mhs->overhead + tx_time_data);
 tx_time_rtscts = ctime + 2 * (mhs->overhead_rtscts + tx_time_data);
 do {
  ctime = (t_slot * cw) >> 1;
  cw = min((cw << 1) | 1, smp->cw_max);
  tx_time += ctime + mhs->overhead + tx_time_data;
  tx_time_rtscts += ctime + mhs->overhead_rtscts + tx_time_data;
  if (tx_time_rtscts < smp->segment_size)
   mr->retry_count_rtscts++;
 } while ((tx_time < smp->segment_size) && (++mr->retry_count < smp->max_retry));
}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,7,0)
static void ssv_minstrel_ht_set_rate(struct ssv_softc *sc, struct ssv_minstrel_priv *smp, struct ssv_minstrel_ht_sta *mhs,
   struct ieee80211_tx_rate *rate, int rate_series, int index, enum nl80211_band band, bool sample, bool rtscts)
#else
static void ssv_minstrel_ht_set_rate(struct ssv_softc *sc, struct ssv_minstrel_priv *smp, struct ssv_minstrel_ht_sta *mhs,
   struct ieee80211_tx_rate *rate, int rate_series, int index, enum ieee80211_band band, bool sample, bool rtscts)
#endif
{
 const struct ssv_mcs_ht_group *group = &ssv_minstrel_mcs_groups[index / MCS_GROUP_RATES];
 struct ssv_minstrel_ht_rate_stats *mr;
 mr = ssv_minstrel_ht_get_ratestats(mhs, index);
 if (!mr->retry_updated)
  ssv_minstrel_ht_calc_retransmit(smp, mhs, index);
 if (sample)
  rate->count = 2;
 else if (mr->probability < MINSTREL_FRAC(20, 100))
  rate->count = 2;
 else if (rtscts)
  rate->count = mr->retry_count_rtscts;
 else
  rate->count = 4;
    rate->flags = 0;
 if (rtscts)
     rate->flags |= IEEE80211_TX_RC_USE_RTS_CTS;
    if (SSV_MINSTREL_STA_GROUP(index) != SSV_MINSTREL_CCK_GROUP) {
     rate->flags |= IEEE80211_TX_RC_MCS;
        if (band == INDEX_80211_BAND_2GHZ) {
      if (group->flags & IEEE80211_TX_RC_40_MHZ_WIDTH) {
       if (mhs->secondary_channel_clear)
        rate->flags |= IEEE80211_TX_RC_40_MHZ_WIDTH;
   }
     } else {
      if (group->flags & IEEE80211_TX_RC_40_MHZ_WIDTH)
       rate->flags |= IEEE80211_TX_RC_40_MHZ_WIDTH;
     }
     rate->idx = index % MCS_GROUP_RATES;
        if (rate->idx == 0)
         rate->flags &= ~IEEE80211_TX_RC_40_MHZ_WIDTH;
     if (((rate->idx == 7) || (rate->idx == 6)) && (group->flags & IEEE80211_TX_RC_SHORT_GI) &&
         ((mhs->sgi_state == SGI_ENABLE_SGI) || (mhs->sgi_state == SGI_DETECT_SGI)))
      rate->flags |= IEEE80211_TX_RC_SHORT_GI;
    } else {
     rate->idx = index % ARRAY_SIZE(smp->cck_rates);
        if (((rate->idx == 3) || (rate->idx == 2)) && mhs->cck_supported_short)
            rate->flags |= IEEE80211_RATE_SHORT_PREAMBLE;
    }
}
static inline int ssv_minstrel_ht_get_duration(int index)
{
 const struct ssv_mcs_ht_group *group = &ssv_minstrel_mcs_groups[index / MCS_GROUP_RATES];
 return group->duration[index % MCS_GROUP_RATES];
}
static void ssv_minstrel_ht_next_sample_idx(struct ssv_minstrel_ht_sta *mhs)
{
 struct ssv_minstrel_ht_mcs_group_data *mg;
 int group = SSV_MINSTREL_STA_GROUP(mhs->max_tp_rate);
 int i;
 for (i = 0 ; i < ARRAY_SIZE(ssv_minstrel_mcs_groups) ; ++i) {
  mhs->sample_group++;
  mhs->sample_group %= ARRAY_SIZE(ssv_minstrel_mcs_groups);
  mg = &mhs->groups[mhs->sample_group];
        if ((group != SSV_MINSTREL_CCK_GROUP) && (mhs->sample_group == SSV_MINSTREL_CCK_GROUP))
            continue;
  if (!mg->supported)
   continue;
  if (++mg->index >= MCS_GROUP_RATES) {
   mg->index = 0;
   if (++mg->column >= ARRAY_SIZE(ssv_minstrel_ht_sample_table))
    mg->column = 0;
  }
  break;
 }
}
static int ssv_minstrel_ht_get_sample_rate(struct ssv_softc *sc,
    struct ssv_minstrel_priv *smp, struct ssv_minstrel_ht_sta *mhs)
{
 struct ssv_minstrel_ht_rate_stats *mr, *max_mr;
 struct ssv_minstrel_ht_mcs_group_data *mg;
 int sample_idx = 0, sample_rate_idx, current_rate_idx;
    int cur_group = SSV_MINSTREL_STA_GROUP(mhs->max_tp_rate);
    int cur_sample_group = mhs->sample_group;
    struct rc_setting *rc_setting = &sc->sh->cfg.rc_setting;
 if (mhs->sample_wait > 0) {
  mhs->sample_wait--;
  return -1;
 }
 if (!mhs->sample_tries)
  return -1;
 mg = &mhs->groups[cur_sample_group];
 sample_idx = ssv_minstrel_ht_sample_table[mg->column][mg->index];
    if (cur_group == SSV_MINSTREL_CCK_GROUP) {
        if (cur_sample_group == SSV_MINSTREL_CCK_GROUP) {
            sample_idx = sample_idx % ARRAY_SIZE(smp->cck_rates);
            if ((mhs->cck_supported_short) && (sample_idx > 1))
                sample_idx += ARRAY_SIZE(smp->cck_rates);
        } else {
            sample_idx = 1;
        }
    }
    mr = &mg->rates[sample_idx];
 ssv_minstrel_ht_next_sample_idx(mhs);
 if (!(mg->supported & BIT(sample_idx)))
  return -1;
    if ((cur_group != SSV_MINSTREL_CCK_GROUP) && (cur_group != cur_sample_group))
        return -1;
 max_mr = ssv_minstrel_ht_get_ratestats(mhs, mhs->max_tp_rate);
 mr = &mg->rates[sample_idx];
 sample_rate_idx = sample_idx;
 current_rate_idx = GET_RATE_INDEX(mhs->max_tp_rate);
    sample_idx += cur_sample_group * MCS_GROUP_RATES;
    if ((cur_group == SSV_MINSTREL_CCK_GROUP) && (cur_sample_group !=SSV_MINSTREL_CCK_GROUP)){
         if (max_mr->probability < MINSTREL_FRAC(80, 100))
               return -1;
    }
    if (cur_group != SSV_MINSTREL_CCK_GROUP) {
        if ((sample_rate_idx > current_rate_idx)){
            int sample_up_pr, forbid_time, force_sample_pr;
            switch (current_rate_idx)
            {
                case 4:
                    sample_up_pr = rc_setting->up_pr4;
                    forbid_time = rc_setting->forbid4;
                    break;
                case 5:
                    sample_up_pr = rc_setting->up_pr5;;
                    forbid_time = rc_setting->forbid5;
                    break;
                case 3:
                    sample_up_pr = rc_setting->up_pr3;
                    forbid_time = rc_setting->forbid3;
                case 6:
                    sample_up_pr = rc_setting->up_pr6;
                    forbid_time = rc_setting->forbid6;
                    break;
                default:
                    sample_up_pr = rc_setting->up_pr;
                    forbid_time = rc_setting->forbid;
                    break;
            }
            if (sc->primary_edca_mib > 10)
                force_sample_pr = rc_setting->force_sample_pr;
            else
                force_sample_pr = 10;
            if (!(mr->probability < MINSTREL_FRAC(force_sample_pr, 100))) {
#if 1
                if ((max_mr->probability < MINSTREL_FRAC(sample_up_pr, 100))
                    || (!(time_after(jiffies, mr->last_jiffies + msecs_to_jiffies(forbid_time)))))
                    return -1;
                if (current_rate_idx == 4)
                    if (sample_rate_idx == 5 ){
                        if (mr->probability < MINSTREL_FRAC(rc_setting->sample_pr_5, 100))
                        return -1;
                    }
                if (current_rate_idx == 3)
                    if (sample_rate_idx == 4 ){
                        if (mr->probability < MINSTREL_FRAC(rc_setting->sample_pr_4, 100))
                        return -1;
                    }
#endif
      } else {
          mr->probability = 0;
      }
        } else {
               return -1;
        }
    }
     if (sc->sh->cfg.rc_log) {
        printk("sample_rate_idx %x, current rate idx %x\n", sample_rate_idx, current_rate_idx);
     }
#if 0
 if ((sample_idx == mhs->max_tp_rate) ||
  (sample_idx == mhs->max_tp_rate2) ||
  (sample_idx == mhs->max_prob_rate))
  return -1;
 if ((ssv_minstrel_ht_get_duration(sample_idx) > ssv_minstrel_ht_get_duration(mhs->max_tp_rate2)) &&
  (ssv_minstrel_ht_get_duration(sample_idx) > ssv_minstrel_ht_get_duration(mhs->max_prob_rate))) {
  if (mr->sample_skipped < 20)
   return -1;
  if (mhs->sample_slow++ > 2)
   return -1;
 }
#endif
 mhs->sample_tries--;
 return sample_idx;
}
static void ssv_minstrel_ht_update_stats(struct ssv_softc *sc, struct ssv_minstrel_priv *smp,
            struct ssv_minstrel_sta_priv *minstrel_sta_priv, struct ssv_minstrel_ht_sta *mhs)
{
 struct ssv_minstrel_ht_mcs_group_data *mg;
 struct ssv_minstrel_ht_rate_stats *mr;
 int cur_prob, cur_prob_tp, cur_tp, cur_tp2;
 int group, i, index, rate_idx;
 struct rc_setting *rc_setting = &sc->sh->cfg.rc_setting;
    struct ssv_sta_priv_data *ssv_sta_priv = (struct ssv_sta_priv_data *)minstrel_sta_priv->sta->drv_priv;
    int signal = -(ssv_sta_priv->beacon_rssi >> RSSI_DECIMAL_POINT_SHIFT);
 if (mhs->ampdu_packets > 0) {
  mhs->avg_ampdu_len = minstrel_ewma(mhs->avg_ampdu_len,
   MINSTREL_FRAC(mhs->ampdu_len, mhs->ampdu_packets), EWMA_LEVEL);
  mhs->ampdu_len = 0;
  mhs->ampdu_packets = 0;
 }
 mhs->sample_slow = 0;
 mhs->sample_count = 0;
 mhs->max_tp_rate = mhs->group_idx * MCS_GROUP_RATES;
 mhs->max_tp_rate2 = 0;
 mhs->max_prob_rate = 0;
 for (group = 0; group < ARRAY_SIZE(ssv_minstrel_mcs_groups); group++) {
  cur_prob = 0;
  cur_prob_tp = 0;
  cur_tp = 0;
  cur_tp2 = 0;
  mg = &mhs->groups[group];
  if (!mg->supported)
   continue;
  mg->max_tp_rate = 0;
  mg->max_tp_rate2 = 0;
  mg->max_prob_rate = 0;
  mhs->sample_count++;
  for (i = 0; i < MCS_GROUP_RATES; i++) {
   if (!(mg->supported & BIT(i)))
    continue;
   mr = &mg->rates[i];
   mr->retry_updated = false;
   index = MCS_GROUP_RATES * group + i;
   ssv_minstrel_ht_calc_rate_ewma(sc, mr);
   ssv_minstrel_ht_calc_tp(mhs, group, i);
   if (!mr->cur_tp)
    continue;
   if (!i && ssv_minstrel_mcs_groups[group].streams == 1)
    continue;
   if ((i == 1) && (group == SSV_MINSTREL_CCK_GROUP))
    continue;
            if ((mr->cur_tp > cur_prob_tp && mr->probability > MINSTREL_FRAC(3, 4)) || mr->probability > cur_prob) {
    mg->max_prob_rate = index;
    cur_prob = mr->probability;
    cur_prob_tp = mr->cur_tp;
   }
   if (mr->cur_tp > cur_tp) {
    swap(index, mg->max_tp_rate);
    cur_tp = mr->cur_tp;
    mr = ssv_minstrel_ht_get_ratestats(mhs, index);
   }
   if (index >= mg->max_tp_rate)
    continue;
   if (mr->cur_tp > cur_tp2) {
    mg->max_tp_rate2 = index;
    cur_tp2 = mr->cur_tp;
   }
  }
 }
 mhs->sample_count *= 4;
 cur_prob = 0;
 cur_prob_tp = 0;
 cur_tp = 0;
 cur_tp2 = 0;
 for (group = 0; group < ARRAY_SIZE(ssv_minstrel_mcs_groups); group++) {
  mg = &mhs->groups[group];
  if (!mg->supported)
   continue;
  mr = ssv_minstrel_ht_get_ratestats(mhs, mg->max_prob_rate);
  if (cur_prob_tp < mr->cur_tp && ssv_minstrel_mcs_groups[group].streams == 1) {
   mhs->max_prob_rate = mg->max_prob_rate;
   cur_prob = mr->cur_prob;
   cur_prob_tp = mr->cur_tp;
  }
  mr = ssv_minstrel_ht_get_ratestats(mhs, mg->max_tp_rate);
  if (cur_tp < mr->cur_tp) {
   mhs->max_tp_rate2 = mhs->max_tp_rate;
   cur_tp2 = cur_tp;
   mhs->max_tp_rate = mg->max_tp_rate;
   cur_tp = mr->cur_tp;
  }
  mr = ssv_minstrel_ht_get_ratestats(mhs, mg->max_tp_rate2);
  if (cur_tp2 < mr->cur_tp) {
   mhs->max_tp_rate2 = mg->max_tp_rate2;
   cur_tp2 = mr->cur_tp;
  }
 }
#define SSV_HT_RC_CHANGE_CCK_RSSI_THRESHOLD (-70)
    group = SSV_MINSTREL_STA_GROUP(mhs->max_tp_rate);
    if (mhs->cck_supported && (group != SSV_MINSTREL_CCK_GROUP) && (signal < SSV_HT_RC_CHANGE_CCK_RSSI_THRESHOLD)) {
        if ((0 == (GET_RATE_INDEX(mhs->max_tp_rate))) || (1 == (GET_RATE_INDEX(mhs->max_tp_rate)))) {
            mr = ssv_minstrel_ht_get_ratestats(mhs, mhs->max_tp_rate);
            if ((mr->probability < MINSTREL_FRAC(1, 4) )||
                (0 == (GET_RATE_INDEX(mhs->max_tp_rate)))) {
                mhs->max_tp_rate = SSV_MINSTREL_CCK_GROUP * MCS_GROUP_RATES + 3;
                if (mhs->cck_supported_short)
                    mhs->max_tp_rate += 4;
          cur_tp = 0;
          mg = &mhs->groups[SSV_MINSTREL_CCK_GROUP];
          for (i = MCS_GROUP_RATES - 1; i >= 0; i--) {
           if (!(mg->supported & BIT(i)))
            continue;
           mr = &mg->rates[i];
                    if (cur_tp < mr->cur_tp) {
                        mhs->max_tp_rate = SSV_MINSTREL_CCK_GROUP * MCS_GROUP_RATES + i;
                        cur_tp = mr->cur_tp;
                    }
                }
            }
        }
    }
    mr = ssv_minstrel_ht_get_ratestats(mhs, mhs->max_tp_rate);
    ssv_sta_priv->max_ampdu_size = SSV_AMPDU_SIZE_1_2(sc->sh);
    if (mr->last_attempts > 0){
        if (MINSTREL_FRAC(mr->last_success, mr->last_attempts)
            < MINSTREL_FRAC( sc->sh->cfg.aggr_size_sel_pr, 100))
            ssv_sta_priv->max_ampdu_size = SSV_AMPDU_SIZE_3_7(sc->sh);
    }
    group = SSV_MINSTREL_STA_GROUP(mhs->max_tp_rate);
    if (sc->sh->cfg.rc_log) {
        printk(" max_tp_rate after selection %d\n", mhs->max_tp_rate);
    }
    if (group != SSV_MINSTREL_CCK_GROUP) {
        u16 target_success;
        rate_idx = GET_RATE_INDEX(mhs->max_tp_rate);
        if (rate_idx >=2){
            if (rate_idx > 5)
                target_success = rc_setting->target_success_67;
            else if (rate_idx == 5)
                target_success = rc_setting->target_success_5;
            else if (rate_idx == 4)
                target_success = rc_setting->target_success_4;
            else
                target_success = rc_setting->target_success;
            if (mr->probability < MINSTREL_FRAC(target_success, 100)){
                mhs->max_tp_rate --;
                rate_idx --;
                if (mr->sample_skipped > 0 )
                    mr->probability = minstrel_ewma(mr->probability, 0, 80);
            }
        }
    } else {
        rate_idx = GET_CCK_RATE_INDEX(mhs->max_tp_rate);
        if (mr->probability < MINSTREL_FRAC(3, 10)){
            if (rate_idx!=0) {
               mhs->max_tp_rate --;
               rate_idx --;
            }
            if ((GET_RATE_INDEX(mhs->max_tp_rate) == 4) || (GET_RATE_INDEX(mhs->max_tp_rate) == 5)) {
               mhs->max_tp_rate -= 4;
               rate_idx -= 4;
            }
            if (mr->sample_skipped > 0 )
               mr->probability = minstrel_ewma(mr->probability, 0, 80);
        }
    }
    if (rate_idx >= 2) {
        mhs->max_tp_rate2 = mhs->max_tp_rate - 1;
        if (mhs->max_prob_rate >= mhs->max_tp_rate2)
            mhs->max_prob_rate = mhs->max_tp_rate2 - 1;
    } else {
        mhs->max_tp_rate2 = group * MCS_GROUP_RATES;
        if (mhs->max_prob_rate > mhs->max_tp_rate2)
            mhs->max_prob_rate = mhs->max_tp_rate2;
    }
    if (group == SSV_MINSTREL_CCK_GROUP){
        if (rate_idx == 1)
            mhs->max_tp_rate -=1;
        if (GET_CCK_RATE_INDEX(mhs->max_tp_rate2) == 1)
            mhs->max_tp_rate2 -=1;
        if (GET_CCK_RATE_INDEX(mhs->max_prob_rate) == 1)
            mhs->max_prob_rate -= 1;
    }
    if (group != SSV_MINSTREL_CCK_GROUP)
        minstrel_sta_priv->update_aggr_check = true;
    mg->max_tp_rate =mhs->max_tp_rate;
    mg->max_tp_rate2 =mhs->max_tp_rate2;
    mg->max_prob_rate = mhs->max_prob_rate;
    mhs->stats_update = jiffies;
 if (sc->sh->cfg.rc_log) {
  printk("%s()\n", __FUNCTION__);
  for (group = 0; group < ARRAY_SIZE(ssv_minstrel_mcs_groups); group++) {
   mg = &mhs->groups[group];
   if (!mg->supported)
    continue;
   printk("rate group[%d]\n", group);
   for (i = 0; i < MCS_GROUP_RATES; i++) {
    printk("rate[%d], succ_hist=%llu, att_hist=%llu, cur_prob=%d, probability=%d, cur_tp=%d\n",
     i, mg->rates[i].succ_hist, mg->rates[i].att_hist,
     mg->rates[i].cur_prob, mg->rates[i].probability, mg->rates[i].cur_tp);
   }
   printk("\n\n");
   printk("max_tp_rate=%d, max_tp_rate2=%d, max_prob_rate=%d\n",
    mg->max_tp_rate, mg->max_tp_rate2, mg->max_prob_rate);
  }
 }
}
static void ssv_minstrel_ht_update_secondary_edcca_stats(struct ssv_softc *sc, struct ssv_minstrel_ht_sta *mhs)
{
 int primary = 0, secondary = 0;
 primary = GET_PRIMARY_EDCA(sc);
 secondary = GET_SECONDARY_EDCA(sc);
 if ((sc->hw_chan_type == NL80211_CHAN_HT40MINUS) || (sc->hw_chan_type == NL80211_CHAN_HT40PLUS)) {
        if (secondary < 50)
      mhs->secondary_channel_clear = 1;
     else
      mhs->secondary_channel_clear = 0;
    } else {
        mhs->secondary_channel_clear = 0;
    }
}
static void _check_sgi(struct ssv_softc *sc, struct ssv_minstrel_ht_sta *mhs, int ampdu_len, int ampdu_ack_len, int short_gi){
 if (mhs->sgi_state == SGI_DETECT_MCS67){
        if (mhs->sgi_state_count < 16) {
         if (ampdu_len >= 4) {
             mhs->sgi_state_count++ ;
             mhs->sgi_state_total += ampdu_len;
             mhs->sgi_state_success += ampdu_ack_len;
         }
         if (sc->sh->cfg.auto_sgi & AUTOSGI_DBG)
             printk(" state: SGI_DETECT_MCS67 ampdu_size %d, ack_num %d total %d, success %d\n", ampdu_len,ampdu_ack_len
                 , mhs->sgi_state_total, mhs->sgi_state_success);
     } else {
         if ((mhs->sgi_state_total-mhs->sgi_state_success) < (mhs->sgi_state_total >>2)) {
             mhs->sgi_state = SGI_DETECT_SGI;
             if (sc->sh->cfg.auto_sgi & AUTOSGI_DBG)
                 printk("Change to state SGI_DETECT_SGI\n");
         }
         mhs->sgi_state_lgi_success = MINSTREL_FRAC(mhs->sgi_state_success, mhs->sgi_state_total);
         mhs->sgi_state_count = 0;
         mhs->sgi_state_total = 0;
         mhs->sgi_state_success = 0;
     }
    } else if ((mhs->sgi_state == SGI_DETECT_SGI) && (short_gi!=0)){
        if (mhs->sgi_state_count < 32) {
         if (ampdu_len >= 4) {
             mhs->sgi_state_count++ ;
             mhs->sgi_state_total += ampdu_len;
             mhs->sgi_state_success += ampdu_ack_len;
         }
         if (sc->sh->cfg.auto_sgi & AUTOSGI_DBG)
             printk(" state SGI_DETECT_SGI:ampdu_size %d, ack_num %d count %d ,total %d, success %d\n",
                 ampdu_len,ampdu_ack_len,mhs->sgi_state_count, mhs->sgi_state_total, mhs->sgi_state_success);
     } else {
         if (MINSTREL_FRAC(mhs->sgi_state_success, mhs->sgi_state_total) > (mhs->sgi_state_lgi_success*9/10)) {
             mhs->sgi_state = SGI_ENABLE_SGI;
             if (sc->sh->cfg.auto_sgi & AUTOSGI_DBG)
                 printk("change state: SGI_ENABLE_SGI %d, threshold %d\n",
                     MINSTREL_FRAC(mhs->sgi_state_success, mhs->sgi_state_total), (mhs->sgi_state_lgi_success*9/10));
         } else {
             mhs->sgi_state = SGI_FORBID_SGI;
             if (sc->sh->cfg.auto_sgi & AUTOSGI_DBG)
                 printk("change state: SGI_FORBID_SGI sgi %d, threshold %d\n",
                     MINSTREL_FRAC(mhs->sgi_state_success, mhs->sgi_state_total), (mhs->sgi_state_lgi_success*9/10));
         }
     }
 }
}
void ssv_minstrel_ht_tx_status(struct ssv_softc *sc, void *rc_info,
  struct ssv_minstrel_ht_rpt ht_rpt[], int ht_rpt_num,
  int ampdu_len, int ampdu_ack_len, bool is_sample)
{
    struct ieee80211_hw *hw = sc->hw;
 struct ssv_minstrel_sta_priv *minstrel_sta_priv = (struct ssv_minstrel_sta_priv *)rc_info;
 struct ssv_minstrel_ht_sta *mhs;
 struct ssv_minstrel_ht_rate_stats *rate;
 struct ssv_minstrel_priv *smp = (struct ssv_minstrel_priv *)sc->rc;
 u16 sta_cap = minstrel_sta_priv->sta->ht_cap.cap;
 int group, r_idx, short_gi, ht40, phy;
    enum nl80211_channel_type channel_type;
 int i = 0;
 if (!minstrel_sta_priv || !minstrel_sta_priv->is_ht)
  return;
 mhs = &minstrel_sta_priv->ht;
 mhs->ampdu_packets++;
 mhs->ampdu_len += ampdu_len;
 if (!mhs->sample_wait && !mhs->sample_tries && mhs->sample_count > 0) {
  mhs->sample_wait = 16 + 2 * MINSTREL_TRUNC(mhs->avg_ampdu_len);
  mhs->sample_tries = 2;
  mhs->sample_count--;
 }
 if (is_sample)
  mhs->sample_packets += ampdu_len;
 for (i = 0; i < ht_rpt_num; i++) {
  r_idx = ht_rpt[i].dword & 0x07;
  short_gi = ((ht_rpt[i].dword & 0x10) >> 4);
  ht40 = ((ht_rpt[i].dword & 0x20) >> 5);
  phy = ((ht_rpt[i].dword & 0xC0) >> 6);
  if (r_idx < 0)
   break;
        if (phy == 0x3){
      group = mhs->group_idx;
      rate = &mhs->groups[group].rates[r_idx % 8];
      if ((sc->sh->cfg.auto_sgi & AUTOSGI_CTL) && (!is_sample)) {
          if (ampdu_len >= 4) {
              if ((r_idx == 6) || (r_idx ==7)) {
                  _check_sgi(sc, mhs, ampdu_len, ampdu_ack_len, short_gi);
              } else {
                     mhs->sgi_state_count = 0;
                     mhs->sgi_state_total = 0;
                     mhs->sgi_state_success = 0;
              }
          }
      }
        } else {
            group = SSV_MINSTREL_CCK_GROUP;
            if (short_gi)
          rate = &mhs->groups[group].rates[(r_idx % 4) + 4];
            else
          rate = &mhs->groups[group].rates[r_idx % 4];
        }
  rate->success += ht_rpt[i].success * ampdu_ack_len;
  if ( ht_rpt[i].success) {
      rate->attempts += (ht_rpt[i].count-1) + ampdu_len;
      mhs->hw_success_acc ++;
            mhs->hw_retry_acc += (ht_rpt[i].count-1);
  } else {
      rate->attempts += ht_rpt[i].count;
      mhs->hw_retry_acc += ht_rpt[i].count;
     }
  if (sc->sh->cfg.rc_log) {
   printk("%s(), group[%d],rates[%d], success=%d, attempts=%d\n",
    __FUNCTION__, group, r_idx % 8, rate->success, rate->attempts);
  }
  if (ht_rpt[i].last)
   break;
 }
 if (time_after(jiffies, mhs->stats_update + (smp->update_interval / 2 * HZ) / 1000)) {
    #if LINUX_VERSION_CODE < KERNEL_VERSION(3,7,0)
        channel_type = hw->conf.channel_type;
    #else
        channel_type = cfg80211_get_chandef_type(&hw->conf.chandef);
    #endif
  if ((sta_cap & IEEE80211_HT_CAP_SUP_WIDTH_20_40) &&
            ((channel_type == NL80211_CHAN_HT40MINUS) || (channel_type == NL80211_CHAN_HT40PLUS)))
   ssv_minstrel_ht_update_secondary_edcca_stats(sc, mhs);
  ssv_minstrel_ht_update_stats(sc, smp, minstrel_sta_priv, mhs);
    }
}
void ssv_minstrel_ht_get_rate(void *priv, struct ieee80211_sta *sta, void *priv_sta,
                     struct ieee80211_tx_rate_control *txrc)
{
 struct ieee80211_tx_info *info = IEEE80211_SKB_CB(txrc->skb);
 struct ieee80211_tx_rate *ar = info->control.rates;
 struct ssv_minstrel_sta_priv *sta_priv = priv_sta;
 struct ssv_minstrel_ht_sta *mhs = &sta_priv->ht;
 struct ssv_softc *sc = priv;
 struct ssv_minstrel_priv *smp = (struct ssv_minstrel_priv *)sc->rc;
 int sample_idx;
 bool sample = false;
 if (rate_control_send_low(sta, priv_sta, txrc))
  return;
 info->flags |= mhs->tx_flags;
 if (smp->max_rates == 1 && txrc->skb->protocol == cpu_to_be16(ETH_P_PAE))
  sample_idx = -1;
 else
  sample_idx = ssv_minstrel_ht_get_sample_rate(sc, smp, mhs);
 if (sample_idx >= 0) {
  sample = true;
  ssv_minstrel_ht_set_rate(sc, smp, mhs, &ar[0], 0, sample_idx, sc->cur_channel->band, true, false);
  info->flags |= IEEE80211_TX_CTL_RATE_CTRL_PROBE;
 } else {
  ssv_minstrel_ht_set_rate(sc, smp, mhs, &ar[0], 0, mhs->max_tp_rate, sc->cur_channel->band, false, false);
 }
 if (smp->max_rates >= 3) {
  if (sample_idx >= 0)
   ssv_minstrel_ht_set_rate(sc, smp, mhs, &ar[1], 1, mhs->max_tp_rate, sc->cur_channel->band, false, false);
  else
   ssv_minstrel_ht_set_rate(sc, smp, mhs, &ar[1], 1, mhs->max_tp_rate2, sc->cur_channel->band, false, false);
  ssv_minstrel_ht_set_rate(sc, smp, mhs, &ar[2], 2, mhs->max_prob_rate, sc->cur_channel->band, false, true);
  ar[3].count = 0;
  ar[3].idx = -1;
 } else if (smp->max_rates == 2) {
  ssv_minstrel_ht_set_rate(sc, smp, mhs, &ar[1], 1, mhs->max_prob_rate, sc->cur_channel->band, false, true);
  ar[2].count = 0;
  ar[2].idx = -1;
 } else {
  ar[1].count = 0;
  ar[1].idx = -1;
 }
 mhs->total_packets++;
 if (mhs->total_packets == ~0) {
  mhs->total_packets = 0;
  mhs->sample_packets = 0;
 }
 dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_RATE_CONTROL,
     "%s():\nar[0].idx=%02x, ar[0].flags=%02x, ar[0].count=%d\n"
  "ar[1].idx=%02x, ar[1].flags=%02x, ar[1].count=%d\n"
  "ar[2].idx=%02x, ar[2].flags=%02x, ar[2].count=%d\n"
  "ar[3].idx=%02x, ar[3].flags=%02x, ar[3].count=%d\n",
  __FUNCTION__, ar[0].idx, ar[0].flags, ar[0].count, ar[1].idx, ar[1].flags, ar[1].count,
  ar[2].idx, ar[2].flags, ar[2].count, ar[3].idx, ar[3].flags, ar[3].count);
}
static void ssv_minstrel_ht_update_cck(struct ssv_softc *sc, struct ssv_minstrel_priv *smp,
                struct ssv_minstrel_ht_sta *mhs,
                struct ieee80211_supported_band *sband,
                struct ieee80211_sta *sta)
{
    int i;
    if (!sc->sh->cfg.rc_ht_support_cck)
        return;
    if (sband->band != INDEX_80211_BAND_2GHZ)
        return;
    mhs->cck_supported = 0;
    mhs->cck_supported_short = 0;
    for (i = 0; i < 4; i++) {
        if (!rate_supported(sta, sband->band, smp->cck_rates[i]))
            continue;
        mhs->cck_supported |= BIT(i);
        if (sband->bitrates[i].flags & IEEE80211_RATE_SHORT_PREAMBLE)
            mhs->cck_supported_short |= BIT(i);
    }
    if (mhs->cck_supported_short) {
        mhs->groups[SSV_MINSTREL_CCK_GROUP].supported = 0xc3;
        mhs->groups[SSV_MINSTREL_CCK_GROUP].max_tp_rate = SSV_MINSTREL_CCK_GROUP * MCS_GROUP_RATES + 4;
    } else {
        mhs->groups[SSV_MINSTREL_CCK_GROUP].supported = 0x0f;
        mhs->groups[SSV_MINSTREL_CCK_GROUP].max_tp_rate = SSV_MINSTREL_CCK_GROUP * MCS_GROUP_RATES;
    }
}
void ssv_minstrel_ht_update_caps(void *priv, struct ieee80211_supported_band *sband,
    struct ieee80211_sta *sta, void *priv_sta, enum nl80211_channel_type oper_chan_type)
{
 struct ssv_softc *sc = priv;
 struct ssv_minstrel_priv *smp = (struct ssv_minstrel_priv *)sc->rc;
 struct ssv_minstrel_sta_priv *sta_priv = priv_sta;
 struct ssv_minstrel_ht_sta *mhs = &sta_priv->ht;
 struct ieee80211_mcs_info *mcs = &sta->ht_cap.mcs;
 u16 sta_cap = sta->ht_cap.cap;
 int ack_dur;
 int stbc;
    bool is_sgi = false;
 sta_priv->is_ht = true;
 memset(mhs, 0, sizeof(*mhs));
 mhs->stats_update = jiffies;
 ack_dur = ieee80211_frame_duration(sband->band, 10, 60, 1, 1);
 mhs->overhead = ieee80211_frame_duration(sband->band, 0, 60, 1, 1) + ack_dur;
 mhs->overhead_rtscts = mhs->overhead + 2 * ack_dur;
 mhs->avg_ampdu_len = MINSTREL_FRAC(1, 1);
 if (smp->has_mrr) {
  mhs->sample_count = 16;
  mhs->sample_wait = 0;
 } else {
  mhs->sample_count = 8;
  mhs->sample_wait = 8;
 }
 mhs->sample_tries = 4;
 stbc = (sta_cap & IEEE80211_HT_CAP_RX_STBC) >> IEEE80211_HT_CAP_RX_STBC_SHIFT;
 mhs->tx_flags |= stbc << IEEE80211_TX_CTL_STBC_SHIFT;
 if (sta_cap & IEEE80211_HT_CAP_LDPC_CODING)
  mhs->tx_flags |= IEEE80211_TX_CTL_LDPC;
 if (oper_chan_type != NL80211_CHAN_HT40MINUS && oper_chan_type != NL80211_CHAN_HT40PLUS)
  sta_cap &= ~IEEE80211_HT_CAP_SUP_WIDTH_20_40;
 if (sta_cap & IEEE80211_HT_CAP_SUP_WIDTH_20_40) {
  if (sta_cap & IEEE80211_HT_CAP_SGI_40){
   mhs->group_idx = GROUP_IDX(1, 1, 1);
   is_sgi = true;
  }else
   mhs->group_idx = GROUP_IDX(1, 0, 1);
 } else {
  if (sta_cap & IEEE80211_HT_CAP_SGI_20){
   mhs->group_idx = GROUP_IDX(1, 1, 0);
   is_sgi = true;
  }else
   mhs->group_idx = GROUP_IDX(1, 0, 0);
 }
    ssv_minstrel_ht_update_cck(sc, smp, mhs, sband, sta);
    if (sc->sh->cfg.auto_sgi & AUTOSGI_CTL){
         if (is_sgi) {
             mhs->sgi_state = SGI_DETECT_MCS67;
         } else {
             mhs->sgi_state = SGI_FORBID_SGI;
         }
    } else {
        mhs->sgi_state = SGI_ENABLE_SGI;
    }
 mhs->groups[mhs->group_idx].supported = mcs->rx_mask[ssv_minstrel_mcs_groups[mhs->group_idx].streams - 1];
    mhs->groups[mhs->group_idx].max_tp_rate = mhs->group_idx * MCS_GROUP_RATES;
}
int ssv_minstrel_ht_update_rate(struct ssv_softc *sc, struct sk_buff *skb)
{
 struct ssv_minstrel_priv *smp = (struct ssv_minstrel_priv *)sc->rc;
 struct SKB_info_st *skb_info = (struct SKB_info_st *)skb->head;
 struct ieee80211_sta *sta = skb_info->sta;
 struct ieee80211_tx_info *info = IEEE80211_SKB_CB(skb);
 struct ieee80211_tx_rate *ar = info->control.rates;
 struct ssv_sta_priv_data *ssv_sta_priv;
 struct ssv_minstrel_sta_priv *msp;
 struct ssv_minstrel_ht_sta *mhs;
 int lowest_rate = 0;
 int sample_idx;
 bool sample = false;
 if (sta == NULL) {
  WARN_ON(1);
  return lowest_rate;
 }
    ssv_sta_priv = (struct ssv_sta_priv_data *)sta->drv_priv;
 msp = (struct ssv_minstrel_sta_priv *)ssv_sta_priv->rc_info;
 if ((!msp) || (msp->is_ht == false)) {
  WARN_ON(1);
  return lowest_rate;
 }
    if (sc->sh->cfg.auto_rate_enable == false) {
        ssv_minstrel_set_fix_data_rate(sc, msp, ar);
        return ar[2].idx;
    }
 mhs = &msp->ht;
 info->flags |= mhs->tx_flags;
 if (smp->max_rates == 1)
  sample_idx = -1;
 else
  sample_idx = ssv_minstrel_ht_get_sample_rate(sc, smp, mhs);
 if (sample_idx >= 0) {
  sample = true;
  ssv_minstrel_ht_set_rate(sc, smp, mhs, &ar[0], 0, sample_idx, sc->cur_channel->band, true, false);
  info->flags |= IEEE80211_TX_CTL_RATE_CTRL_PROBE;
 } else {
  ssv_minstrel_ht_set_rate(sc, smp, mhs, &ar[0], 0, mhs->max_tp_rate, sc->cur_channel->band, false, false);
 }
 if (smp->max_rates >= 3) {
  if (sample_idx >= 0)
   ssv_minstrel_ht_set_rate(sc, smp, mhs, &ar[1], 1, mhs->max_tp_rate, sc->cur_channel->band, false, false);
  else
   ssv_minstrel_ht_set_rate(sc, smp, mhs, &ar[1], 1, mhs->max_tp_rate2, sc->cur_channel->band, false, false);
  ssv_minstrel_ht_set_rate(sc, smp, mhs, &ar[2], 2, mhs->max_prob_rate, sc->cur_channel->band, false, true);
  ar[3].count = 0;
  ar[3].idx = -1;
  lowest_rate = ar[2].idx;
 } else if (smp->max_rates == 2) {
  ssv_minstrel_ht_set_rate(sc, smp, mhs, &ar[1], 1, mhs->max_prob_rate, sc->cur_channel->band, false, true);
  ar[2].count = 0;
  ar[2].idx = -1;
  lowest_rate = ar[1].idx;
 } else {
  ar[1].count = 0;
  ar[1].idx = -1;
  lowest_rate = ar[0].idx;
 }
 dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_RATE_CONTROL,
     "%s():\nar[0].idx=%02x, ar[0].flags=%02x, ar[0].count=%d\n"
  "ar[1].idx=%02x, ar[1].flags=%02x, ar[1].count=%d\n"
  "ar[2].idx=%02x, ar[2].flags=%02x, ar[2].count=%d\n"
  "ar[3].idx=%02x, ar[3].flags=%02x, ar[3].count=%d\n",
  __FUNCTION__, ar[0].idx, ar[0].flags, ar[0].count, ar[1].idx, ar[1].flags, ar[1].count,
  ar[2].idx, ar[2].flags, ar[2].count, ar[3].idx, ar[3].flags, ar[3].count);
 return lowest_rate;
}
bool ssv_minstrel_ht_sta_is_cck_rates(struct ieee80211_sta *sta)
{
 struct ssv_sta_priv_data *ssv_sta_priv;
 struct ssv_minstrel_sta_priv *msp;
 struct ssv_minstrel_ht_sta *mhs;
 if (sta == NULL)
  return false;
    ssv_sta_priv = (struct ssv_sta_priv_data *)sta->drv_priv;
 msp = (struct ssv_minstrel_sta_priv *)ssv_sta_priv->rc_info;
 if ((!msp) || (msp->is_ht == false))
  return false;
 mhs = &msp->ht;
    if (SSV_MINSTREL_CCK_GROUP == SSV_MINSTREL_STA_GROUP(mhs->max_tp_rate))
        return true;
    else
        return false;
}
