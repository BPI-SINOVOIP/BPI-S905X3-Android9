
#include <net/atbm_mac80211.h>
#include <linux/kthread.h>

#include "apollo.h"
#include "bh.h"
#include "hwio.h"
#include "wsm.h"
#include "sbus.h"
#include "debug.h"
#include "apollo_plat.h"
#include "sta.h"
#include "ap.h"
#include "scan.h"
extern int atbm_bh_schedule_tx(struct atbm_common *hw_priv);
/* Must be called from BH thraed. */
void atbm_enable_powersave(struct atbm_vif *priv,
			     bool enable)
{
	atbm_dbg(ATBM_APOLLO_DBG_MSG, "[BH] Powerave is %s.\n",
			enable ? "enabled" : "disabled");
	priv->powersave_enabled = enable;
}

void atbm_bh_wakeup(struct atbm_common *hw_priv)
{
	atbm_dbg(ATBM_APOLLO_DBG_MSG, "[BH] wakeup.\n");
#ifndef USB_BUS
	if (/*WARN_ON*/(hw_priv->bh_error)||atbm_bh_is_term(hw_priv)){
		printk("atbm_wifi [BH] err drop\n");
		return;
	}
	if (atomic_add_return(1, &hw_priv->bh_tx) == 1)
		wake_up(&hw_priv->bh_wq);
#else //usb_usb
	 atbm_bh_schedule_tx(hw_priv);
#endif //USB_BUS
}

int wsm_release_vif_tx_buffer(struct atbm_common *hw_priv, int if_id,
				int count)
{
	int ret = 0;

	spin_lock_bh(&hw_priv->tx_com_lock);
	hw_priv->hw_bufs_used_vif[if_id] -= count;

	if (WARN_ON(hw_priv->hw_bufs_used_vif[if_id] < 0)){
		printk(KERN_ERR "%s:[%d][%d]\n",__func__,if_id,hw_priv->hw_bufs_used_vif[if_id]);
		hw_priv->hw_bufs_used_vif[if_id] = 0;
		//BUG_ON(1);
		//ret = -1;
	}

	spin_unlock_bh(&hw_priv->tx_com_lock);

	if (!hw_priv->hw_bufs_used_vif[if_id])
		wake_up(&hw_priv->bh_evt_wq);

	return ret;
}


int wsm_release_vif_tx_buffer_Nolock(struct atbm_common *hw_priv, int if_id,
				int count)
{
	int ret = 0;

	hw_priv->hw_bufs_used_vif[if_id] -= count;

	if (!hw_priv->hw_bufs_used_vif[if_id])
		wake_up(&hw_priv->bh_evt_wq);

	if (WARN_ON(hw_priv->hw_bufs_used_vif[if_id] < 0)){
		printk(KERN_ERR "%s:[%d][%d]\n",__func__,if_id,hw_priv->hw_bufs_used_vif[if_id]);
		hw_priv->hw_bufs_used_vif[if_id] =0;
		//BUG_ON(1);
		//ret = -1;
	}

	return ret;
}
void atbm_monitor_pc(struct atbm_common *hw_priv)
{
	u32 testreg1[10] = {0};
	u32 testreg_pc = 0;
	u32 testreg_ipc = 0;
	u32 val28;
	u32 val20;
	int i = 0;

	atbm_direct_write_reg_32(hw_priv,0x16100050,1);
	//atbm_direct_read_reg_32(hw_priv,0x18e00014,&testreg1);
	atbm_direct_read_reg_32(hw_priv,0x16100054,&testreg_pc);
	atbm_direct_read_reg_32(hw_priv,0x16100058,&testreg_ipc);
	atbm_direct_read_reg_32(hw_priv,0x16101028,&val28);
	atbm_direct_read_reg_32(hw_priv,0x16101020,&val20);
	//atbm_direct_read_reg_32(hw_priv,0x16400000,&testreg_uart);

	for(i=0;i<10;i++){
		atbm_direct_read_reg_32(hw_priv,hw_priv->wsm_caps.exceptionaddr+4*i+4,&testreg1[i]);
	}
	atbm_direct_write_reg_32(hw_priv,0x16100050,0);

	printk("ERROR !! pc:[%x],ipc[%x] \n",testreg_pc,testreg_ipc);
	printk("ERROR !! reg0:[%x],reg1[%x],reg2[%x],reg3[%x],reg4[%x],reg5[%x],reg6[%x] \n",
														testreg1[0],
														testreg1[1],
														testreg1[2],
														testreg1[3],
														testreg1[4],
														testreg1[5],
														testreg1[6]);

	printk(KERN_ERR "%s:0x16101028(%x)\n",__func__,val28);	
	printk(KERN_ERR "%s:0x16101020(%x)\n",__func__,val20);
	
}


#ifdef	ATBM_WIFI_QUEUE_LOCK_BUG

void atbm_set_priv_queue_cap(struct atbm_vif *priv)
{
	struct atbm_common	*hw_priv = priv->hw_priv;
	struct atbm_vif *other_priv;
	u8 i = 0;

	priv->queue_cap = ATBM_QUEUE_SINGLE_CAP;
	atbm_for_each_vif(hw_priv, other_priv, i) {
		if(other_priv == NULL)
			continue;
		if(other_priv == priv)
			continue;
		if(other_priv->join_status <=  ATBM_APOLLO_JOIN_STATUS_MONITOR)
			continue;
		other_priv->queue_cap = ATBM_QUEUE_COMB_CAP;
		priv->queue_cap = ATBM_QUEUE_COMB_CAP;
		printk(KERN_ERR "%s:[%d],queue_cap[%d]\n",__func__,other_priv->if_id,other_priv->queue_cap);
	}

	printk(KERN_ERR "%s:[%d],queue_cap[%d]\n",__func__,priv->if_id,priv->queue_cap);
}

void atbm_clear_priv_queue_cap(struct atbm_vif *priv)
{
	struct atbm_common	*hw_priv = priv->hw_priv;
	struct atbm_vif *other_priv;
	struct atbm_vif *prev_priv = NULL;
	u8 i = 0;

	priv->queue_cap = ATBM_QUEUE_DEFAULT_CAP;

	atbm_for_each_vif(hw_priv, other_priv, i) {

		if(other_priv == NULL)
			continue;
		if(other_priv == priv)
			continue;
		if(other_priv->join_status <=  ATBM_APOLLO_JOIN_STATUS_MONITOR)
			continue;
		
		other_priv->queue_cap = ATBM_QUEUE_SINGLE_CAP;

		if(prev_priv == NULL){
			prev_priv = other_priv;
			continue;
		}

		prev_priv->queue_cap = ATBM_QUEUE_COMB_CAP;
		other_priv->queue_cap = ATBM_QUEUE_COMB_CAP;
		prev_priv = other_priv;
		
		printk(KERN_ERR "%s:[%d],queue_cap[%d]\n",__func__,prev_priv->if_id,prev_priv->queue_cap);
	}
}
#endif
