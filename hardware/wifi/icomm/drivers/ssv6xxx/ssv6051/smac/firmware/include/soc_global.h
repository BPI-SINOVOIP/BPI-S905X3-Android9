#ifndef _SOC_GLOBAL_H_
#define _SOC_GLOBAL_H_


struct soc_mib_st
{
	/* SW MIB counter for soft-mac */
	u32			sw_mac_rx_cnt;	
	u32 		sw_mac_drop_cnt;

	/* SW MIB counter for CPU reason dispatcher */
	u32			sw_mac_handle_cnt;
	struct {
		u32		hdl_defrag_rx_cnt;
		u32     hdl_defrag_accept;
		
	} hdl;
	
	/* SW MIB counter for cmd-engine */
	u32			sw_cmd_rx_cnt;

	/* SW MIB counter for MLME */
	struct {
		u32		sw_mlme_rx_cnt;

	} mlme;

};


extern struct task_info_st g_soc_task_info[];
extern struct soc_table_st *g_soc_table;

#if (CLI_HISTORY_ENABLE==1)

s8 gCmdHistoryBuffer[CLI_HISTORY_NUM][CLI_BUFFER_SIZE+1];
s8 gCmdHistoryIdx;
s8 gCmdHistoryCnt;

#endif//CLI_HISTORY_ENABLE



#endif /* _SOC_GLOBAL_ */

