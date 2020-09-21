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
#include <linux/nl80211.h>
#include <linux/etherdevice.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <linux/time.h>
#include <linux/sched.h>
#ifdef SSV_MAC80211
#include "ssv_mac80211.h"
#else
#include <net/mac80211.h>
#endif
#include <ssv6200.h>
#include "lib.h"
#include "dev.h"
#include "ap.h"
#include "ssv_rc_common.h"
#include "ssv_rc.h"
#include <hal.h>
int ssv6200_bcast_queue_len(struct ssv6xxx_bcast_txq *bcast_txq);
#define IS_EQUAL(a,b) ( (a) == (b) )
#define SET_BIT(v,b) ( (v) |= (0x01<<b) )
#define CLEAR_BIT(v,b) ( (v) &= ~(0x01<<b) )
#define IS_BIT_SET(v,b) ( (v) & (0x01<<(b) ) )
#define PBUF_BASE_ADDR 0x80000000
#define PBUF_ADDR_SHIFT 16
#define PBUF_MapPkttoID(_PKT) (((u32)_PKT&0x0FFF0000)>>PBUF_ADDR_SHIFT)
#define PBUF_MapIDtoPkt(_ID) (PBUF_BASE_ADDR|((_ID)<<PBUF_ADDR_SHIFT))
#define SSV6xxx_BEACON_MAX_ALLOCATE_CNT 10
#define MTX_BCN_PKTID_CH_LOCK_SHIFT MTX_BCN_PKTID_CH_LOCK_SFT
#define MTX_BCN_CFG_VLD_SHIFT MTX_BCN_CFG_VLD_SFT
#define MTX_BCN_CFG_VLD_MASK MTX_BCN_CFG_VLD_MSK
#define AUTO_BCN_ONGOING_MASK MTX_AUTO_BCN_ONGOING_MSK
#define AUTO_BCN_ONGOING_SHIFT MTX_AUTO_BCN_ONGOING_SFT
#define MTX_BCN_TIMER_EN_SHIFT MTX_BCN_TIMER_EN_SFT
#define MTX_TSF_TIMER_EN_SHIFT MTX_TSF_TIMER_EN_SFT
#define MTX_HALT_MNG_UNTIL_DTIM_SHIFT MTX_HALT_MNG_UNTIL_DTIM_SFT
#define MTX_BCN_ENABLE_MASK (MTX_BCN_TIMER_EN_I_MSK)
#define MTX_BCN_PERIOD_SHIFT MTX_BCN_PERIOD_SFT
#define MTX_DTIM_NUM_SHIFT MTX_DTIM_NUM_SFT
#define MTX_DTIM_OFST0 MTX_DTIM_OFST0_SFT
#ifndef SSV_SUPPORT_HAL
enum ssv6xxx_beacon_type{
 SSV6xxx_BEACON_0,
 SSV6xxx_BEACON_1,
};
void ssv6xxx_beacon_reg_lock(struct ssv_hw *sh, bool block)
{
 dbgprint(&sh->sc->cmd_data, sh->sc->log_ctrl, LOG_BEACON,
     "ssv6xxx_beacon_reg_lock   val[0x:%08x]\n ", block);
 SMAC_REG_SET_BITS(sh, ADR_MTX_BCN_MISC, block<<MTX_BCN_PKTID_CH_LOCK_SFT,
     MTX_BCN_PKTID_CH_LOCK_MSK);
}
void ssv6xxx_beacon_set_info(struct ssv_hw *sh, u8 beacon_interval, u8 dtim_cnt)
{
 u32 val;
 if(beacon_interval==0)
  beacon_interval = 100;
 dbgprint(&sh->sc->cmd_data, sh->sc->log_ctrl, LOG_BEACON,
     "[A] BSS_CHANGED_BEACON_INT beacon_int[%d] dtim_cnt[%d]\n",beacon_interval, (dtim_cnt));
 val = (beacon_interval<<MTX_BCN_PERIOD_SHIFT)| (dtim_cnt<<MTX_DTIM_NUM_SHIFT);
 SMAC_REG_WRITE(sh, ADR_MTX_BCN_PRD, val);
}
bool ssv6xxx_beacon_enable(struct ssv_softc *sc, bool bEnable)
{
 u32 regval=0;
 int ret = 0;
 if(bEnable && !sc->beacon_usage)
 {
  printk("[A] Reject to set beacon!!!.        ssv6xxx_beacon_enable bEnable[%d] sc->beacon_usage[%d]\n",bEnable ,sc->beacon_usage);
        sc->enable_beacon = BEACON_WAITING_ENABLED;
  return 0;
 }
 if((bEnable && (BEACON_ENABLED & sc->enable_beacon))||
        (!bEnable && !sc->enable_beacon))
 {
  printk("[A] ssv6xxx_beacon_enable bEnable[%d] and sc->enable_beacon[%d] are the same. no need to execute.\n",bEnable ,sc->enable_beacon);
        if(bEnable){
            printk("        Ignore enable beacon cmd!!!!\n");
            return 0;
        }
 }
 SMAC_REG_READ(sc->sh, ADR_MTX_BCN_EN_MISC, &regval);
 dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_BEACON,
     "[A] ssv6xxx_beacon_enable read misc reg val [%08x]\n", regval);
 regval &= MTX_BCN_ENABLE_MASK;
 dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_BEACON,
     "[A] ssv6xxx_beacon_enable read misc reg val [%08x]\n", regval);
 regval |= (bEnable<<MTX_BCN_TIMER_EN_SHIFT);
 ret = SMAC_REG_WRITE(sc->sh, ADR_MTX_BCN_EN_MISC, regval);
 dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_BEACON,
     "[A] ssv6xxx_beacon_enable read misc reg val [%08x]\n", regval);
    sc->enable_beacon = (bEnable==true)?BEACON_ENABLED:0;
 return ret;
}
int ssv6xxx_beacon_fill_content(struct ssv_softc *sc, u32 regaddr, u8 *beacon, int size)
{
 u32 i, j, val;
 size = size/4;
    for(i = 0; i < size; i++){
        val = 0;
        for ( j = 0; j < 4; j ++) {
            val += (*beacon++) << j*8;
        }
  dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_BEACON, "[%08x] ", val );
  SMAC_REG_WRITE(sc->sh, regaddr+i*4, val);
 }
 dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_BEACON,"\n");
    return 0;
}
inline enum ssv6xxx_beacon_type ssv6xxx_beacon_get_valid_reg(struct ssv_hw *sh)
{
 u32 regval =0;
 SMAC_REG_READ(sh, ADR_MTX_BCN_MISC, &regval);
 regval &= MTX_BCN_CFG_VLD_MASK;
 regval = regval >>MTX_BCN_CFG_VLD_SHIFT;
 if(regval==0x2 || regval == 0x0)
  return SSV6xxx_BEACON_0;
 else if(regval==0x1)
  return SSV6xxx_BEACON_1;
 else
  printk("=============>ERROR!!drv_bcn_reg_available\n");
 return SSV6xxx_BEACON_0;
}
inline bool ssv6xxx_auto_bcn_ongoing(struct ssv_hw *sh)
{
 u32 regval;
 SMAC_REG_READ(sh, ADR_MTX_BCN_MISC, &regval);
 return ((AUTO_BCN_ONGOING_MASK & regval) >> AUTO_BCN_ONGOING_SHIFT);
}
void ssv6xxx_beacon_fill_tx_desc(struct ssv_softc *sc, struct sk_buff* beacon_skb)
{
 struct ieee80211_tx_info *tx_info = IEEE80211_SKB_CB(beacon_skb);
 struct ssv6200_tx_desc *tx_desc;
 struct ssv_rate_info ssv_rate;
 skb_push(beacon_skb, TXPB_OFFSET);
 tx_desc = (struct ssv6200_tx_desc *)beacon_skb->data;
 memset(tx_desc,0, TXPB_OFFSET);
 ssv6xxx_rc_hw_rate_idx(sc, tx_info, &ssv_rate);
    tx_desc->len = beacon_skb->len-TXPB_OFFSET;
 tx_desc->c_type = M2_TXREQ;
    tx_desc->f80211 = 1;
 tx_desc->ack_policy = 1;
    tx_desc->hdr_offset = TXPB_OFFSET;
    tx_desc->hdr_len = 24;
    tx_desc->payload_offset = tx_desc->hdr_offset + tx_desc->hdr_len;
 tx_desc->crate_idx = ssv_rate.crate_hw_idx;
 tx_desc->drate_idx = ssv_rate.drate_hw_idx;
    skb_put(beacon_skb, 4);
}
#endif
bool ssv6xxx_beacon_set(struct ssv_softc *sc, struct sk_buff *beacon_skb, int dtim_offset)
{
 enum ssv6xxx_beacon_type avl_bcn_type = SSV6xxx_BEACON_0;
 bool ret = true;
 SSV_SET_BEACON_REG_LOCK(sc->sh, true);
    avl_bcn_type = SSV_BEACON_GET_VALID_CFG(sc->sh);
 dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_BEACON,
     "[A] ssv6xxx_beacon_set avl_bcn_type[%d]\n", avl_bcn_type);
 do{
  if(IS_BIT_SET(sc->beacon_usage, avl_bcn_type)){
   dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_BEACON,
             "[A] beacon has already been set old len[%d] new len[%d]\n",
             sc->beacon_info[avl_bcn_type].len, beacon_skb->len);
   if (sc->beacon_info[avl_bcn_type].len >= beacon_skb->len){
    break;
   } else {
          if (false == SSV_FREE_PBUF(sc, sc->beacon_info[avl_bcn_type].pubf_addr)){
     dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_BEACON,
                     "=============>ERROR!!Intend to allcoate beacon from ASIC fail.\n");
     ret = false;
     goto out;
    }
    CLEAR_BIT(sc->beacon_usage, avl_bcn_type);
   }
  }
     sc->beacon_info[avl_bcn_type].pubf_addr = SSV_ALLOC_PBUF(sc, beacon_skb->len, RX_BUF);
  sc->beacon_info[avl_bcn_type].len = beacon_skb->len;
  if(sc->beacon_info[avl_bcn_type].pubf_addr == 0){
   ret = false;
   goto out;
  }
  SET_BIT(sc->beacon_usage, avl_bcn_type);
  dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_BEACON,
         "[A] beacon type[%d] usage[%d] allocate new beacon addr[%08x] \n",
         avl_bcn_type, sc->beacon_usage, sc->beacon_info[avl_bcn_type].pubf_addr);
 }while(0);
#ifdef SSV_SUPPORT_HAL
 HAL_FILL_BCN(sc, sc->beacon_info[avl_bcn_type].pubf_addr, beacon_skb);
 HAL_SET_BCN_ID_DTIM(sc, avl_bcn_type, dtim_offset);
#else
 ssv6xxx_beacon_fill_content(sc, sc->beacon_info[avl_bcn_type].pubf_addr, beacon_skb->data, beacon_skb->len);
    {
     u32 reg_tx_beacon_adr = ADR_MTX_BCN_CFG0;
     u32 val;
        val = (PBUF_MapPkttoID(sc->beacon_info[avl_bcn_type].pubf_addr)
                | (dtim_offset<<MTX_DTIM_OFST0));
     if(avl_bcn_type == SSV6xxx_BEACON_1)
      reg_tx_beacon_adr = ADR_MTX_BCN_CFG1;
     SMAC_REG_WRITE(sc->sh, reg_tx_beacon_adr, val);
    }
#endif
out:
    SSV_SET_BEACON_REG_LOCK(sc->sh, false);
    if(sc->beacon_usage && (sc->enable_beacon&BEACON_WAITING_ENABLED)){
        printk("[A] enable beacon for BEACON_WAITING_ENABLED flags\n");
        SSV_BEACON_ENABLE(sc, true);
    }
 return ret;
}
void ssv6xxx_beacon_release(struct ssv_softc *sc)
{
 int cnt=10;
 printk("[A] ssv6xxx_beacon_release Enter\n");
    cancel_work_sync(&sc->set_tim_work);
 do {
        if (SSV_GET_BCN_ONGOING(sc->sh)){
            SSV_BEACON_ENABLE(sc, false);
  } else {
   break;
     }
  cnt--;
  if(cnt<=0)
   break;
 } while(1);
 if(IS_BIT_SET(sc->beacon_usage, SSV6xxx_BEACON_0)){
     SSV_FREE_PBUF(sc, sc->beacon_info[SSV6xxx_BEACON_0].pubf_addr);
  CLEAR_BIT(sc->beacon_usage, SSV6xxx_BEACON_0);
 }
 if(IS_BIT_SET(sc->beacon_usage, SSV6xxx_BEACON_1)) {
        SSV_FREE_PBUF(sc, sc->beacon_info[SSV6xxx_BEACON_1].pubf_addr);
  CLEAR_BIT(sc->beacon_usage, SSV6xxx_BEACON_1);
 }
 sc->enable_beacon = 0;
    if(sc->beacon_buf){
        dev_kfree_skb_any(sc->beacon_buf);
        sc->beacon_buf = NULL;
    }
 dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_BEACON, "[A] ssv6xxx_beacon_release leave\n");
}
void ssv6xxx_beacon_change(struct ssv_softc *sc, struct ieee80211_hw *hw, struct ieee80211_vif *vif, bool aid0_bit_set)
{
 struct sk_buff *skb;
    struct sk_buff *old_skb = NULL;
 u16 tim_offset, tim_length;
    if(sc == NULL || hw == NULL || vif == NULL ){
        printk("[Error]........ssv6xxx_beacon_change input error\n");
  return;
    }
    do{
     skb = ieee80211_beacon_get_tim(hw, vif,
       &tim_offset, &tim_length);
        if(skb == NULL){
            printk("[Error]........skb is NULL\n");
            break;
        }
     if (tim_offset && tim_length >= 6) {
      skb->data[tim_offset + 2] = 0;
            if (!(sc->sh->cfg.hw_caps & SSV6200_HW_CAP_BEACON))
          skb->data[tim_offset + 3] = 1;
      if (aid0_bit_set)
       skb->data[tim_offset + 4] |= 1;
      else
       skb->data[tim_offset + 4] &= ~1;
     }
     dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_BEACON,
         "[A] beacon len [%d] tim_offset[%d]\n", skb->len, tim_offset);
     SSV_FILL_BEACON_TX_DESC(sc, skb);
     dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_BEACON,
         "[A] beacon len [%d] tim_offset[%d]\n", skb->len, tim_offset);
        if(sc->beacon_buf)
        {
            if(memcmp(sc->beacon_buf->data, skb->data, (skb->len-FCS_LEN)) == 0){
                old_skb = skb;
                break;
            }
            else{
                 old_skb = sc->beacon_buf;
                 sc->beacon_buf = skb;
            }
        }
        else{
            sc->beacon_buf = skb;
        }
     tim_offset+=2;
     if(ssv6xxx_beacon_set(sc, skb, tim_offset))
     {
      u8 dtim_cnt = vif->bss_conf.dtim_period-1;
      if(sc->beacon_dtim_cnt != dtim_cnt)
      {
       sc->beacon_dtim_cnt = dtim_cnt;
                dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_BEACON,
                 "[A] beacon_dtim_cnt [%d]\n", sc->beacon_dtim_cnt);
                SSV_SET_BCN_IFNO(sc->sh, sc->beacon_interval,
                    sc->beacon_dtim_cnt);
      }
     }
    }while(0);
    if(old_skb)
     dev_kfree_skb_any(old_skb);
}
void ssv6200_set_tim_work(struct work_struct *work)
{
    struct ssv_softc *sc =
            container_of(work, struct ssv_softc, set_tim_work);
#ifdef BROADCAST_DEBUG
 printk("%s() enter\n", __FUNCTION__);
#endif
 ssv6xxx_beacon_change(sc, sc->hw, sc->ap_vif, sc->aid0_bit_set);
#ifdef BROADCAST_DEBUG
    printk("%s() leave\n", __FUNCTION__);
#endif
}
int ssv6200_bcast_queue_len(struct ssv6xxx_bcast_txq *bcast_txq)
{
 u32 len;
    unsigned long flags;
    spin_lock_irqsave(&bcast_txq->txq_lock, flags);
    len = bcast_txq->cur_qsize;
    spin_unlock_irqrestore(&bcast_txq->txq_lock, flags);
    return len;
}
struct sk_buff* ssv6200_bcast_dequeue(struct ssv6xxx_bcast_txq *bcast_txq, u8 *remain_len)
{
    struct sk_buff *skb = NULL;
    unsigned long flags;
    spin_lock_irqsave(&bcast_txq->txq_lock, flags);
    if(bcast_txq->cur_qsize){
        bcast_txq->cur_qsize --;
        if(remain_len)
            *remain_len = bcast_txq->cur_qsize;
        skb = __skb_dequeue(&bcast_txq->qhead);
    }
    spin_unlock_irqrestore(&bcast_txq->txq_lock, flags);
    return skb;
}
int ssv6200_bcast_enqueue(struct ssv_softc *sc, struct ssv6xxx_bcast_txq *bcast_txq,
                                                        struct sk_buff *skb)
{
    unsigned long flags;
    spin_lock_irqsave(&bcast_txq->txq_lock, flags);
    if (bcast_txq->cur_qsize >=
                    SSV6200_MAX_BCAST_QUEUE_LEN){
        struct sk_buff *old_skb;
  old_skb = __skb_dequeue(&bcast_txq->qhead);
        bcast_txq->cur_qsize --;
        ssv6xxx_txbuf_free_skb(old_skb, (void*)sc);
        printk("[B] ssv6200_bcast_enqueue - remove oldest queue\n");
    }
    __skb_queue_tail(&bcast_txq->qhead, skb);
    bcast_txq->cur_qsize ++;
    spin_unlock_irqrestore(&bcast_txq->txq_lock, flags);
    return bcast_txq->cur_qsize;
}
void ssv6200_bcast_flush(struct ssv_softc *sc, struct ssv6xxx_bcast_txq *bcast_txq)
{
    struct sk_buff *skb;
    unsigned long flags;
#ifdef BCAST_DEBUG
    printk("ssv6200_bcast_flush\n");
#endif
    spin_lock_irqsave(&bcast_txq->txq_lock, flags);
    while(bcast_txq->cur_qsize > 0) {
        skb = __skb_dequeue(&bcast_txq->qhead);
        bcast_txq->cur_qsize --;
        ssv6xxx_txbuf_free_skb(skb, (void*)sc);
    }
    spin_unlock_irqrestore(&bcast_txq->txq_lock, flags);
}
static int queue_block_cnt = 0;
void ssv6200_bcast_tx_work(struct work_struct *work)
{
    struct ssv_softc *sc =
            container_of(work, struct ssv_softc, bcast_tx_work.work);
#if 1
    struct sk_buff* skb;
    int i;
    u8 remain_size;
#endif
    unsigned long flags;
    bool needtimer = true;
    long tmo = sc->bcast_interval;
    spin_lock_irqsave(&sc->ps_state_lock, flags);
    do{
#ifdef BCAST_DEBUG
        printk("[B] bcast_timer: hw_mng_used[%d] HCI_TXQ_EMPTY[%d] bcast_queue_len[%d].....................\n",
               sc->hw_mng_used, HCI_TXQ_EMPTY(sc->sh, 4), ssv6200_bcast_queue_len(&sc->bcast_txq));
#endif
        if(sc->hw_mng_used != 0 ||
            false == HCI_TXQ_EMPTY(sc->sh, 4)){
#ifdef BCAST_DEBUG
            printk("HW queue still have frames insdide. skip this one hw_mng_used[%d] bEmptyTXQ4[%d]\n",
                sc->hw_mng_used, HCI_TXQ_EMPTY(sc->sh, 4));
#endif
            queue_block_cnt++;
            if(queue_block_cnt>5){
                queue_block_cnt = 0;
                ssv6200_bcast_flush(sc, &sc->bcast_txq);
                needtimer = false;
            }
            break;
        }
        queue_block_cnt = 0;
        for(i=0;i<SSV6200_ID_MANAGER_QUEUE;i++){
            skb = ssv6200_bcast_dequeue(&sc->bcast_txq, &remain_size);
            if(!skb){
                needtimer = false;
                break;
            }
            if( (0 != remain_size) &&
                (SSV6200_ID_MANAGER_QUEUE-1) != i ){
                struct ieee80211_hdr *hdr;
                hdr = (struct ieee80211_hdr *) ((u8*)skb->data+TXPB_OFFSET);
                hdr->frame_control |= cpu_to_le16(IEEE80211_FCTL_MOREDATA);
            }
#ifdef BCAST_DEBUG
            printk("[B] bcast_timer:tx remain_size[%d] i[%d]\n", remain_size, i);
#endif
   spin_unlock_irqrestore(&sc->ps_state_lock, flags);
            if(HCI_SEND(sc->sh, skb, 4)<0){
                printk("bcast_timer send fail!!!!!!! \n");
                ssv6xxx_txbuf_free_skb(skb, (void*)sc);
                BUG_ON(1);
            }
   spin_lock_irqsave(&sc->ps_state_lock, flags);
        }
    }while(0);
    if(needtimer){
#ifdef BCAST_DEBUG
        printk("[B] bcast_timer:need more timer to tx bcast frame time[%d]\n", sc->bcast_interval);
#endif
        queue_delayed_work(sc->config_wq,
       &sc->bcast_tx_work,
       tmo);
    }
    else{
#ifdef BCAST_DEBUG
       printk("[B] bcast_timer: ssv6200_bcast_stop\n");
#endif
       ssv6200_bcast_stop(sc);
    }
    spin_unlock_irqrestore(&sc->ps_state_lock, flags);
#ifdef BCAST_DEBUG
    printk("[B] bcast_timer: leave.....................\n");
#endif
}
void ssv6200_bcast_start_work(struct work_struct *work)
{
 struct ssv_softc *sc =
  container_of(work, struct ssv_softc, bcast_start_work);
#ifdef BCAST_DEBUG
    printk("[B] ssv6200_bcast_start_work==\n");
#endif
    sc->bcast_interval = (sc->beacon_dtim_cnt+1) *
   (sc->beacon_interval + 20) * HZ / 1000;
 if (!sc->aid0_bit_set) {
  sc->aid0_bit_set = true;
  ssv6xxx_beacon_change(sc, sc->hw,
                        sc->ap_vif, sc->aid0_bit_set);
        queue_delayed_work(sc->config_wq,
       &sc->bcast_tx_work,
       sc->bcast_interval);
#ifdef BCAST_DEBUG
        printk("[B] bcast_start_work: Modify timer to DTIM[%d]ms==\n",
               (sc->beacon_dtim_cnt+1)*(sc->beacon_interval + 20));
#endif
 }
}
void ssv6200_bcast_stop_work(struct work_struct *work)
{
 struct ssv_softc *sc =
  container_of(work, struct ssv_softc, bcast_stop_work.work);
    long tmo = HZ / 100;
#ifdef BCAST_DEBUG
    printk("[B] ssv6200_bcast_stop_work\n");
#endif
 if (sc->aid0_bit_set) {
        if(0== ssv6200_bcast_queue_len(&sc->bcast_txq)){
            cancel_delayed_work_sync(&sc->bcast_tx_work);
      sc->aid0_bit_set = false;
      ssv6xxx_beacon_change(sc, sc->hw,
                            sc->ap_vif, sc->aid0_bit_set);
#ifdef BCAST_DEBUG
            printk("remove group bit in DTIM\n");
#endif
        }
        else{
#ifdef BCAST_DEBUG
            printk("bcast_stop_work: bcast queue still have data. just modify timer to 10ms\n");
#endif
            queue_delayed_work(sc->config_wq,
       &sc->bcast_tx_work,
       tmo);
        }
 }
}
void ssv6200_bcast_stop(struct ssv_softc *sc)
{
    queue_delayed_work(sc->config_wq,
         &sc->bcast_stop_work, sc->beacon_interval*HZ/1024);
}
void ssv6200_bcast_start(struct ssv_softc *sc)
{
    queue_work(sc->config_wq, &sc->bcast_start_work);
}
void ssv6200_release_bcast_frame_res(struct ssv_softc *sc, struct ieee80211_vif *vif)
{
    unsigned long flags;
    struct ssv_vif_priv_data *priv_vif = (struct ssv_vif_priv_data *)vif->drv_priv;
    spin_lock_irqsave(&sc->ps_state_lock, flags);
    priv_vif->sta_asleep_mask = 0;
    spin_unlock_irqrestore(&sc->ps_state_lock, flags);
    cancel_work_sync(&sc->bcast_start_work);
    cancel_delayed_work_sync(&sc->bcast_stop_work);
    ssv6200_bcast_flush(sc, &sc->bcast_txq);
    cancel_delayed_work_sync(&sc->bcast_tx_work);
}
