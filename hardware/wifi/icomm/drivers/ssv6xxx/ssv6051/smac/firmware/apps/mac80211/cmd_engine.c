#include <config.h>
#include <msgevt.h>
#include <pbuf.h>

#include <cmd_def.h>
//#include <common.h>
//#include <host_apis.h>
#include <log.h>

#include "cmd_engine.h"
#include <hwmac/drv_mac.h>
#include <mbox/drv_mbox.h>
#include <phy/drv_phy.h>
#include <p2p/drv_p2p.h>


#include <rtos.h>
#include <lib/ssv_lib.h>
#include <dbg_timer/dbg_timer.h>
#include <watchdog/sw_watchdog.h>

#define CMD_PRINTF(fmt, ...) 		LOG_PRINTF_M(LOG_MODULE_CMD, fmt, ##__VA_ARGS__)
#define CMD_TRACE(fmt, ...)			LOG_TRACE_M(LOG_MODULE_CMD, fmt, ##__VA_ARGS__)
#define CMD_DEBUG(fmt, ...)			LOG_DEBUG_M(LOG_MODULE_CMD, fmt, ##__VA_ARGS__)
#define CMD_INFO(fmt, ...)			LOG_INFO_M(LOG_MODULE_CMD,  fmt, ##__VA_ARGS__)
#define CMD_WARN(fmt, ...)			LOG_WARN_M(LOG_MODULE_CMD,  fmt, ##__VA_ARGS__)
#define CMD_FAIL(fmt, ...)			LOG_FAIL_M(LOG_MODULE_CMD,  fmt, ##__VA_ARGS__)
#define CMD_ERROR(fmt, ...)			LOG_ERROR_M(LOG_MODULE_CMD, fmt, ##__VA_ARGS__)
#define CMD_FATAL(fmt, ...)			LOG_FATAL_M(LOG_MODULE_CMD, fmt, ##__VA_ARGS__)

#define CHECK_DATA_LEN(host_cmd, size) \
    do { \
        if ((host_cmd->len - HOST_CMD_HDR_LEN) != (size)) \
        { \
            CMD_ERROR("Host command %u with length %d does not match expected %u.\n", \
                      host_cmd->h_cmd, (s32)host_cmd->len - (s32)HOST_CMD_HDR_LEN, (u32)(size)); \
            return; \
        } \
    } while (0)
    
#ifdef	CMD_ENGINE_DEBUG
#define CMD_DUMP(p_host_cmd, fmt, ...) \
            CMD_INFO("%s(): len=%u, " fmt, __FUNCTION__, p_host_cmd->len, ##__VA_ARGS__)
#else
#define CMD_DUMP(p_host_cmd, fmt, ...) do{}while(0)
#endif

static OsMutex CmdEngineMutex;
//static bool    _CmdEnginBatchMode = false;

u32 sg_soc_evt_seq_no;

extern void send_data_to_host(u32 size,u32 loopTimes);
/*---------------------------  Start of HostCmd  ---------------------------*/
MsgEvent *host_cmd_resp_alloc (const struct cfg_host_cmd *host_cmd, CmdResult_E cmd_result, u32 resp_size, void **p_resp_data)
{
	MsgEvent 				*msg_evt;
	struct  resp_evt_result *resp;
	u32						 event_size;

#ifdef USE_CMD_RESP
	event_size = RESP_EVT_HEADER_SIZE + resp_size;
	HOST_EVENT_ALLOC_RET(msg_evt, SOC_EVT_CMD_RESP, event_size, NULL);
    resp = (struct resp_evt_result *)HOST_EVENT_DATA_PTR(msg_evt);
    resp->cmd 			= host_cmd->h_cmd;
    resp->cmd_seq_no 	= host_cmd->cmd_seq_no;
    resp->result 		= cmd_result;
#else
    event_size = resp_size;
    HOST_EVENT_ALLOC_RET(msg_evt, (ssv6xxx_soc_event)0, ETHER_ADDR_LEN, NULL);
    resp = (struct resp_evt_result *)HOST_EVENT_DATA_PTR(msg_evt);
#endif // USE_CMD_RESP
    HOST_EVENT_SET_LEN(msg_evt, event_size);
    if (p_resp_data != NULL)
    	*p_resp_data = (void *)&resp->u.dat[0];
    return msg_evt;
}


/* Send command response with no data */
static void _simple_resp_ex (const struct cfg_host_cmd *host_cmd, CmdResult_E cmd_result,
		                     u32 extra_size, const u8 *extra_data)
{
#ifdef USE_CMD_RESP
    MsgEvent *MsgEv;
    u8 		 *resp_msg;

	if (_CmdEnginBatchMode)
		return;

    CMD_RESP_ALLOC(MsgEv, host_cmd, cmd_result, extra_size, (void *)&resp_msg);
    if ((extra_data != NULL) && (extra_size > 0))
    	memcpy(resp_msg, extra_data, extra_size);
    // HOST_EVENT_SET_LEN(MsgEv, extra_size);
    HOST_EVENT_SEND(MsgEv);
#endif // USE_CMD_RESP
} // end of - _simple_resp_ex -


static void _simple_resp (const struct cfg_host_cmd *host_cmd, CmdResult_E cmd_result)
{
	_simple_resp_ex(host_cmd, cmd_result, 0, NULL);
} // end of - _simple_resp -


// Examples
#if 0
static void CmdEngine_SetReg( const struct cfg_host_cmd *HostCmd )
{
    u32 reg_addr = HostCmd->dat32[0];
    u32 reg_val  = HostCmd->dat32[1];
    CMD_DUMP(HostCmd, "addr=0x%08x, val=0x%08x\n", reg_addr, reg_val);
    REG32(reg_addr) = reg_val;
    _simple_resp(HostCmd, CMD_OK);
}


static void CmdEngine_GetReg( const struct cfg_host_cmd *HostCmd )
{
    MsgEvent *MsgEv;
    u32 reg_addr = HostCmd->dat32[0];
    u8 *msg;

    CMD_RESP_ALLOC(MsgEv, HostCmd, CMD_OK, sizeof(u32), (void *)&msg);
 	#ifndef USE_CMD_RESP
    HOST_EVENT_ASSIGN_EVT(MsgEv, SOC_EVT_GET_REG_RESP);
 	#endif // USE_CMD_RESP
    *(u32 *)msg = REG32(reg_addr);
    HOST_EVENT_SEND(MsgEv);
}
#endif // 0

static void CmdEngine_LOG( const struct cfg_host_cmd *HostCmd )
{
    // CMD_INFO("%s <= \n\r", __FUNCTION__);
    // LOG_hcmd_dump((log_hcmd_st *)(&(HostCmd->dat8)));
    #ifdef ENABLE_LOG_TO_HOST
    LOG_soc_exec_hcmd((log_hcmd_st *)(HostCmd->dat8));
    // CMD_INFO("%s => \n\r", __FUNCTION__);
    _simple_resp(HostCmd, CMD_OK);
    #else
    _simple_resp(HostCmd, CMD_INVALID);
    #endif
}
static void CmdEngine_PS( const struct cfg_host_cmd *HostCmd )
{
    MsgEvent *MsgEv;

    //LOG_PRINTF("CmdEngine_PS,aid=%d\n",HostCmd->dummy);

    MsgEv = msg_evt_alloc();
    ASSERT(MsgEv != NULL);

    MsgEv->MsgType = MEVT_PS_DOZE;
    MsgEv->MsgData = HostCmd->dummy;
    msg_evt_post(MBOX_SOFT_MAC,MsgEv);

}
static void CmdEngine_InitCali(const struct cfg_host_cmd *HostCmd) {

    struct ssv6xxx_iqk_cfg *p_iqk_cfg;

    p_iqk_cfg = (struct ssv6xxx_iqk_cfg *)HostCmd->dat32;

    g_rf_config.cfg_xtal      = p_iqk_cfg->cfg_xtal;
    g_rf_config.cfg_pa        = p_iqk_cfg->cfg_pa;
    g_rf_config.cfg_pabias_ctrl = GET_RG_PABIAS_CTRL;
    g_rf_config.cfg_pacascode_ctrl = GET_RG_PACASCODE_CTRL;
    g_rf_config.cfg_tssi_trgt = p_iqk_cfg->cfg_tssi_trgt;
    g_rf_config.cfg_tssi_div  = p_iqk_cfg->cfg_tssi_div;

    printf("xtal type[%d]\n",p_iqk_cfg->cfg_xtal);

    g_rf_config.cfg_def_tx_scale_11b      = p_iqk_cfg->cfg_def_tx_scale_11b;
    g_rf_config.cfg_def_tx_scale_11b_p0d5 = p_iqk_cfg->cfg_def_tx_scale_11b_p0d5;
    g_rf_config.cfg_def_tx_scale_11g      = p_iqk_cfg->cfg_def_tx_scale_11g;
    g_rf_config.cfg_def_tx_scale_11g_p0d5 = p_iqk_cfg->cfg_def_tx_scale_11g_p0d5;

    if (p_iqk_cfg->cmd_sel == SSV6XXX_IQK_CMD_INIT_CALI ) {

        LOG_PRINTF("# got a init-cali cmd\n");

        extern ssv_cabrio_reg firmware_phy_setting[MAX_PHY_SETTING_TABLE_SIZE];
        extern ssv_cabrio_reg firmware_rf_setting[MAX_RF_SETTING_TABLE_SIZE];
        extern u32 firmware_phy_setting_len;
        extern u32 firmware_rf_setting_len;
        firmware_phy_setting_len = p_iqk_cfg->phy_tbl_size;
        firmware_rf_setting_len = p_iqk_cfg->rf_tbl_size;

        if(firmware_phy_setting_len)
        {
            memset(firmware_phy_setting,0x00,MAX_PHY_SETTING_TABLE_SIZE);
            memcpy(firmware_phy_setting, HostCmd->dat8+IQK_CFG_LEN, firmware_phy_setting_len);
        }
    
        if(firmware_rf_setting_len)
        {
            memset(firmware_rf_setting,0x00,MAX_RF_SETTING_TABLE_SIZE);
            memcpy(firmware_rf_setting, HostCmd->dat8+IQK_CFG_LEN+firmware_phy_setting_len, firmware_rf_setting_len);
        }

        drv_phy_init_cali       (p_iqk_cfg->fx_sel);
        drv_phy_cali_rtbl_export(p_iqk_cfg->fx_sel);
    }
    else if (p_iqk_cfg->cmd_sel == SSV6XXX_IQK_CMD_RTBL_LOAD) {

        LOG_PRINTF("# got a rtbl-load cmd\n");

        drv_phy_cali_rtbl_load(p_iqk_cfg->fx_sel);
    }
    else if (p_iqk_cfg->cmd_sel == SSV6XXX_IQK_CMD_RTBL_LOAD_DEF) {

        LOG_PRINTF("# got a rtbl-load-def cmd\n");

        drv_phy_cali_rtbl_load_def(p_iqk_cfg->fx_sel);
    }
    else if (p_iqk_cfg->cmd_sel == SSV6XXX_IQK_CMD_RTBL_RESET) {

        LOG_PRINTF("# got a rtbl-reset cmd\n");

        drv_phy_cali_rtbl_reset(p_iqk_cfg->fx_sel);
    }
    else if (p_iqk_cfg->cmd_sel == SSV6XXX_IQK_CMD_RTBL_SET) {

        LOG_PRINTF("# got a rtbl-set cmd\n");

        drv_phy_cali_rtbl_set(p_iqk_cfg->fx_sel);
    }
    else if (p_iqk_cfg->cmd_sel == SSV6XXX_IQK_CMD_RTBL_EXPORT) {

        LOG_PRINTF("# got a rtbl-export cmd\n");

        drv_phy_cali_rtbl_export(p_iqk_cfg->fx_sel);
    }
    else if (p_iqk_cfg->cmd_sel == SSV6XXX_IQK_CMD_TK_EVM) {

        LOG_PRINTF("# got a tk-evm cmd\n");

        drv_phy_evm_tk(p_iqk_cfg->argv);
    }
    else if (p_iqk_cfg->cmd_sel == SSV6XXX_IQK_CMD_TK_TONE) {

        LOG_PRINTF("# got a tk-tone cmd\n");

        drv_phy_tone_tk(p_iqk_cfg->argv);
    }
    else if (p_iqk_cfg->cmd_sel == SSV6XXX_IQK_CMD_TK_CHCH) {

        LOG_PRINTF("# change channel cmd\n");

        drv_phy_channel_change(p_iqk_cfg->argv);
    }
    else {
        LOG_PRINTF("# got a invalid cmd\n");
    }
	SET_RG_MRX_EN_CNT_RST_N(1); //enable rx en cnt
    drv_phy_tx_loopback();

    _simple_resp(HostCmd, CMD_OK);
}

void txThroughputHandler(u32 sdio_tx_ptk_len)
{
	static u32 txtput_cnt;
    u32 time_offset, throughput;
	u32 allocate_fail=0;
	PKT_RxInfo *NewPkt=NULL;	
    HDR_HostEvent *host_evt;
	static Time_T pre_t;

	if (txtput_cnt) {
		if ((txtput_cnt % 1000) == 0) {
			time_offset = dbg_get_elapsed(&pre_t);
			throughput = sdio_tx_ptk_len * txtput_cnt;
			printf("transferring %d bytes, time_offset %d\n", throughput, time_offset);
			if(throughput < 0x1fffff)
				throughput = ((throughput * 1000)/(time_offset/1000))/1000;
			else
				throughput = throughput/(time_offset/1000);
			
			NewPkt = (PKT_RxInfo *)PBUF_MAlloc(sizeof(HDR_HostEvent), NOTYPE_BUF);
			if(NewPkt == NULL)
			{
				OS_MsDelay(10);
				allocate_fail++;
			} else {
				host_evt = (HDR_HostEvent *)NewPkt;
				host_evt->c_type = HOST_EVENT;
				host_evt->h_event = SOC_EVT_SDIO_TXTPUT_RESULT;
				host_evt->len = sizeof(HDR_HostEvent);
				host_evt->evt_seq_no = throughput<<3;
			
				TX_FRAME((u32)NewPkt);
			}
			NewPkt = NULL;
			txtput_cnt = 0;
			printf("txtput: %d Kbps per 1000 frames\n",throughput<<3);
			return;
		}
	} else {
		//dbg_timer_init();
		dbg_get_time(&pre_t);
	}
    txtput_cnt++;

    return;
}

static void CmdEngine_TxTput( const struct cfg_host_cmd *HostCmd )
{		
	u32 sdio_tx_ptk_len = 0;
    sdio_tx_ptk_len = HostCmd->len;
	txThroughputHandler(sdio_tx_ptk_len);
}

static void CmdEngine_RxTput( const struct cfg_host_cmd *HostCmd )
{
	struct sdio_rxtput_cfg *cmd_rxtput_cfg;
    cmd_rxtput_cfg = (struct sdio_rxtput_cfg *)HostCmd->dat32;
    send_data_to_host(cmd_rxtput_cfg->size_per_frame, cmd_rxtput_cfg->total_frames);
}

static void CmdEngine_watchdog_start( const struct cfg_host_cmd *HostCmd )
{
	sw_watchdog_start();
}

static void CmdEngine_watchdog_stop( const struct cfg_host_cmd *HostCmd )
{
    sw_watchdog_stop();
}

#ifdef FW_WSID_WATCH_LIST
static void CmdEngine_WSID_OP(const struct cfg_host_cmd *HostCmd) {
    MsgEvent *MsgEv;

    MsgEv = msg_evt_alloc();
    ASSERT(MsgEv != NULL);

    MsgEv->MsgType = MEVT_WSID_OP;
    MsgEv->MsgData = (u32)HostCmd->dat8;
    msg_evt_post(MBOX_SOFT_MAC,MsgEv);
    
}
#endif

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#ifdef CONFIG_P2P_NOA
extern struct drv_p2p_noa_param g_p2p_noa;

static void CmdEngine_SetNoa(const struct cfg_host_cmd *HostCmd) {

    struct ssv6xxx_p2p_noa_param *pNoaParam;
    struct drv_p2p_noa_param pDrvNoaParam;

    pNoaParam = (struct ssv6xxx_p2p_noa_param *)HostCmd->dat32;
    memcpy (&pDrvNoaParam, pNoaParam, sizeof(struct ssv6xxx_p2p_noa_param));
    drv_p2p_process_noa(&pDrvNoaParam, sizeof(struct ssv6xxx_p2p_noa_param));     
}
#endif

/*----------------------------  End of HostCmd  ----------------------------*/
static void CmdEngine_LOG( const struct cfg_host_cmd *HostCmd );

/**
 *  Host Command Registration Table:
 *  
 *  Register supported host command to this table, so the 
 *  CmdEng_CmdRequest() can dispatch commands to their 
 *  corresponding handlers.
 */
typedef void (*TBL_HostCmdHandler)(const struct cfg_host_cmd *);


typedef struct _HostCmdMap_S {
    ssv6xxx_host_cmd_id     event_id;
    TBL_HostCmdHandler  event_handler;
} HostCmdMap_S;

const HostCmdMap_S HostCmdMap[] = {
    //===========================================================================    
    //Public command        
    //{SSV6XXX_HOST_CMD_SET_REG,               CmdEngine_SetReg                 },
    //{SSV6XXX_HOST_CMD_GET_REG,               CmdEngine_GetReg                 },
	{SSV6XXX_HOST_CMD_LOG,					 CmdEngine_LOG                    },
    {SSV6XXX_HOST_CMD_PS,                    CmdEngine_PS                     },
    {SSV6XXX_HOST_CMD_INIT_CALI,             CmdEngine_InitCali               },
    {SSV6XXX_HOST_CMD_RX_TPUT,				 CmdEngine_RxTput                 },
    {SSV6XXX_HOST_CMD_TX_TPUT,				 CmdEngine_TxTput                 },
    {SSV6XXX_HOST_CMD_WATCHDOG_START,		CmdEngine_watchdog_start                 },
    {SSV6XXX_HOST_CMD_WATCHDOG_STOP,		CmdEngine_watchdog_stop                 },

#ifdef FW_WSID_WATCH_LIST
    {SSV6XXX_HOST_CMD_WSID_OP,             CmdEngine_WSID_OP               },
#endif	
#ifdef CONFIG_P2P_NOA	
	{SSV6XXX_HOST_CMD_SET_NOA,               CmdEngine_SetNoa               },
#endif
};

#if 0
static void _send_hcmd_ack_to_host(void)
{
    MsgEvent *MsgEv;
    u8 *msg;

	HOST_EVENT_ALLOC(MsgEv, SOC_EVT_ACK, sizeof(u32));
	msg = HOST_EVENT_DATA_PTR(MsgEv);
	*(u32 *)msg = 1;
	HOST_EVENT_SET_LEN(MsgEv, sizeof(u32));
	HOST_EVENT_SEND(MsgEv);
	return;
}
#endif

static void _exec_host_cmd(struct cfg_host_cmd *p_host_cmd)
{
    s32 idx, max_idx = sizeof(HostCmdMap)/sizeof(HostCmdMap_S );
    for (idx = 0; idx < max_idx; idx++)
	{
        if (HostCmdMap[idx].event_id == p_host_cmd->h_cmd)
        {
			HostCmdMap[idx].event_handler(p_host_cmd);
            return;
        }
	}
    CMD_ERROR("Host command %u is not found.\n", p_host_cmd->h_cmd);
} // end of - _exec_host_cmd -



/**
* CmdEngoine_HostEventAlloc() - Allocate both a packet buffer and a message
*                                                   event and then link them together.
*
*/
MsgEvent *HostEventAlloc(ssv6xxx_soc_event hEvtID, u32 size)
{
    MsgEvent *MsgEv;
    HDR_HostEvent *chevt;

    MsgEv = (MsgEvent *)msg_evt_alloc();
    if (MsgEv == NULL)
        return (MsgEvent *)NULL;
    memset((void *)MsgEv, 0, sizeof(MsgEvent));
    MsgEv->MsgType = MEVT_PKT_BUF;
    MsgEv->MsgData = (u32)PBUF_MAlloc(size+sizeof(HDR_HostEvent) ,NOTYPE_BUF);
    if (MsgEv->MsgData == 0) {
        msg_evt_free(MsgEv);
        return (MsgEvent *)NULL;
    }
    chevt = (HDR_HostEvent *)MsgEv->MsgData;
    chevt->c_type  = HOST_EVENT;
    chevt->h_event = hEvtID;
    chevt->len     = size;
    return MsgEv;
}


/**
* void CmdEngine_Task() - SoC Command Engine main loop.
*
*/
void CmdEngine_Task( void *args )
{
    struct cfg_host_cmd *HostCmd;
    MsgEvent *MsgEv;
    s32 res;

    /*lint -save -e716 */
    while(1)
    {
        /*lint -restore */
        /* Wait OS Event here: */
        res = msg_evt_fetch(MBOX_CMD_ENGINE, &MsgEv);
        ASSERT(res == OS_SUCCESS);
        HostCmd = (struct cfg_host_cmd *)MsgEv->MsgData; 
        //LOG_PRINTF("Get cmd,c_type=%d\n",HostCmd->c_type);
        msg_evt_free(MsgEv);
        
        switch (HostCmd->c_type)
        {
        
        case HOST_CMD:
            if (HostCmd->h_cmd >= SSV6XXX_HOST_SOC_CMD_MAXID) {
                LOG_PRINTF("%s(): Unknown command type (0x%02x)!!\n", 
                           __FUNCTION__, HostCmd->h_cmd);
                PBUF_MFree(HostCmd);
                break;
            }
            
            _exec_host_cmd(HostCmd);
            PBUF_MFree(HostCmd);
            break;
        case HOST_EVENT:
            if (HostCmd->h_cmd >= SOC_EVT_MAXID) {
                LOG_PRINTF("%s(): Unknown event type (0x%02x)!!\n", 
                           __FUNCTION__, HostCmd->h_cmd);
                PBUF_MFree(HostCmd);
                break;
            }
            // log_printf("HOST_EVENT # %u\n",HostCmd->h_cmd);
             
            TX_FRAME((u32)HostCmd);
            break;

        default: PBUF_MFree(HostCmd);
        }
    }
}


s32 cmd_engine_init()
{
    OS_MutexInit(&CmdEngineMutex);
    LOG_MODULE_TURN_ON(LOG_MODULE_CMD);
	sg_soc_evt_seq_no=1000;

    return 0;
}

