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

#ifndef _AP_H_
#define _AP_H_ 
#ifndef SSV_SUPPORT_HAL
#define BEACON_WAITING_ENABLED 1<<0
#define BEACON_ENABLED 1<<1
bool ssv6xxx_beacon_enable(struct ssv_softc *sc, bool bEnable);
void ssv6xxx_beacon_set_info(struct ssv_hw *sh, u8 beacon_interval, u8 dtim_cnt);
void ssv6xxx_beacon_fill_tx_desc(struct ssv_softc *sc, struct sk_buff* beacon_skb);
#endif
void ssv6xxx_beacon_change(struct ssv_softc *sc, struct ieee80211_hw *hw, struct ieee80211_vif *vif, bool aid0_bit_set);
void ssv6xxx_beacon_release(struct ssv_softc *sc);
void ssv6200_set_tim_work(struct work_struct *work);
void ssv6200_bcast_start_work(struct work_struct *work);
void ssv6200_bcast_stop_work(struct work_struct *work);
void ssv6200_bcast_tx_work(struct work_struct *work);
int ssv6200_bcast_queue_len(struct ssv6xxx_bcast_txq *bcast_txq);
struct sk_buff* ssv6200_bcast_dequeue(struct ssv6xxx_bcast_txq *bcast_txq, u8 *remain_len);
int ssv6200_bcast_enqueue(struct ssv_softc *sc, struct ssv6xxx_bcast_txq *bcast_txq, struct sk_buff *skb);
void ssv6200_bcast_start(struct ssv_softc *sc);
void ssv6200_bcast_stop(struct ssv_softc *sc);
void ssv6200_release_bcast_frame_res(struct ssv_softc *sc, struct ieee80211_vif *vif);
#ifdef SSV_SUPPORT_HAL
#define SSV_SET_BEACON_REG_LOCK(_sh,_bool) HAL_SET_BEACON_REG_LOCK( _sh, _bool)
#define SSV_BEACON_GET_VALID_CFG(_sh) HAL_BEACON_GET_VALID_CFG( _sh)
#define SSV_ALLOC_PBUF(_sc,_len,_TX_BUF) HAL_ALLOC_PBUF( _sc, _len, _TX_BUF)
#define SSV_BEACON_ENABLE(_sc,_bool) HAL_BEACON_ENABLE( _sc, _bool)
#define SSV_GET_BCN_ONGOING(_sh) HAL_GET_BCN_ONGOING( _sh)
#define SSV_SET_BCN_IFNO(_sh,_beacon_interval,_beacon_dtim_cnt) \
            HAL_SET_BCN_IFNO( _sh, _beacon_interval, _beacon_dtim_cnt)
#define SSV_FILL_BEACON_TX_DESC(_sc,_skb) HAL_FILL_BEACON_TX_DESC(_sc, _skb)
#else
#define SSV_SET_BEACON_REG_LOCK(_sh,_bool) ssv6xxx_beacon_reg_lock( _sh, _bool)
#define SSV_BEACON_GET_VALID_CFG(_sh) ssv6xxx_beacon_get_valid_reg( _sh)
#define SSV_ALLOC_PBUF(_sc,_len,_TX_BUF) ssv6xxx_pbuf_alloc( _sc, _len, _TX_BUF)
#define SSV_BEACON_ENABLE(_sc,_bool) ssv6xxx_beacon_enable( _sc, _bool)
#define SSV_GET_BCN_ONGOING(_sh) ssv6xxx_auto_bcn_ongoing( _sh)
#define SSV_SET_BCN_IFNO(_sh,_beacon_interval,_beacon_dtim_cnt) \
            ssv6xxx_beacon_set_info( _sh, _beacon_interval, _beacon_dtim_cnt)
#define SSV_FILL_BEACON_TX_DESC(_sc,_skb) ssv6xxx_beacon_fill_tx_desc(_sc, _skb)
#endif
#endif
