#ifndef _ATBM_P2P_H_
#define _ATBM_P2P_H_
int TxRxPublicActionFrame(u8 *pframe ,u32 len,int bTx);

#ifdef ATBM_P2P_CHANGE
void atbm_parase_p2p_scan_resp(struct atbm_vif *priv,struct sk_buff *skb);
bool atbm_parase_p2p_action_frame(struct atbm_vif *priv,struct sk_buff *skb,bool tx);
bool atbm_parase_p2p_mgmt_frame(struct atbm_vif *priv,struct sk_buff *skb,bool tx);
#endif

#endif  //_ATBM_P2P_H_

