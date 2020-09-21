#include <config.h>
#include <mbox/drv_mbox.h>
#include <timer/drv_timer.h>
#include <bsp/irq.h>
#include <log.h>
#include <./../drv_comm.h>
#include <pbuf.h>
#include <ssv_pktdef.h>
#include <hdr80211.h>
#include "drv_rc.h"
#include <hwmac/drv_mac.h>

static u32 _firmware_rc_get_mac_tc_count(u8 wsid)
{
    u32 value = 0;
    if(wsid == 0)
        value = GET_MTX_MIB_CNT0;
    else if(wsid == 1)
        value = GET_MTX_MIB_CNT1;

    return value;
}

void _firmware_rate_control_report (u32 tx_data,u8 event)
{
    struct ssv6200_tx_desc *tx_desc;
    tx_desc = (struct ssv6200_tx_desc*)tx_data;
    u8 wsid = tx_desc->wsid;
    u8 drate = tx_desc->drate_idx;
    HDR_HostEvent *host_evt;
    u32 evt_size =   sizeof(HDR_HostEvent) + sizeof(struct firmware_rate_control_report_data);
    u32 txCounter = 0;
    struct firmware_rate_control_report_data *report_data;
    u16 mpduFrames=0,mpduFrameRetry=0,mpduFrameSuccess=0;

    //Hardware only support 2 WSID for rate control.
    if(wsid > 2)
    {
        PBUF_MFree((void *)tx_data);
        return;
    }

    if(event == SSV6XXX_RC_COUNTER_CLEAR)
    {
        //Read counter
        _firmware_rc_get_mac_tc_count(wsid);
        PBUF_MFree((void *)tx_data);
    }
    else
    {
        txCounter = _firmware_rc_get_mac_tc_count(wsid);

        mpduFrames = (u16)(txCounter & 0x3ff);
        mpduFrameRetry = (u16)((txCounter >> 10) & 0x3ff);
        mpduFrameSuccess = (u16)((txCounter >> 20) & 0x3ff);

        if(mpduFrames < mpduFrameSuccess)
        {
            printf("Rate control issue - 1!!\n");
            printf("mpduFrames[%d] mpduFrameRetry[%d] mpduFrameSuccess[%d]\n",mpduFrames,mpduFrameRetry,mpduFrameSuccess);
            PBUF_MFree((void *)tx_data);
            return;
        }

        if(mpduFrameRetry < (mpduFrames - mpduFrameSuccess))
        {
            printf("Rate control issue - 2!!\n");
            printf("mpduFrames[%d] mpduFrameRetry[%d] mpduFrameSuccess[%d]\n",mpduFrames,mpduFrameRetry,mpduFrameSuccess);
            PBUF_MFree((void *)tx_data);
            return;
        }

        //Fix hardware issue
        //(1,7,0) shoubld be (1,6,0) 
        mpduFrameRetry -= (mpduFrames - mpduFrameSuccess);

        if((mpduFrames + mpduFrameRetry) > 255 )
        {
            printf("Rate control issue - 3!!\n");
            printf("mpduFrames[%d] mpduFrameRetry[%d] mpduFrameSuccess[%d]\n",mpduFrames,mpduFrameRetry,mpduFrameSuccess);
            PBUF_MFree((void *)tx_data);
            return;
        }

        //frameTotal = report_data->mpduFrames + report_data->mpduFrameRetry;
        //frameFail = frameTotal - report_data->mpduFrameSuccess;
        //frameSuccess = report_data->mpduFrameSuccess;

        host_evt = (HDR_HostEvent *)tx_data;
        host_evt->c_type = HOST_EVENT;
        host_evt->h_event = SOC_EVT_RC_MPDU_REPORT;
        host_evt->len = evt_size;
        report_data = (struct firmware_rate_control_report_data *)&host_evt->dat[0];
        report_data->wsid = wsid;

        report_data->ampdu_len = mpduFrames;;
        report_data->ampdu_ack_len = mpduFrameSuccess;
#if 0
        printf("rate[%d]f[%d]r[%d]s[%d]\n",drate,mpduFrames,mpduFrameRetry,mpduFrameSuccess);
#endif
        report_data->rates[0].data_rate = drate;
        report_data->rates[0].count = mpduFrameRetry + mpduFrames;
        report_data->rates[1].data_rate = -1;
        report_data->rates[1].count = 0;

        TX_FRAME(tx_data);
    }
}
