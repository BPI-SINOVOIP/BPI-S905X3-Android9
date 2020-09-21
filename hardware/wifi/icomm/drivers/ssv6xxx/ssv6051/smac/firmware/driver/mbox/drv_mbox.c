#include <config.h>

#include <regs.h>
#include <pbuf.h>
#include "drv_mbox.h"


//Mount mailbox FIFO for HW module(low priority)
/*
	default CPU(0)
	Command Flow => 87654321   drv_mailbox_cpu_ff(0x80000000,1)  => 1-2-3-4-5-6-7-8-9-0
	Command Flow => 87654321   drv_mailbox_cpu_ff(0x80000000,4)  => 4-5-6-7-8-9-0
	Command Flow => 87654321   drv_mailbox_cpu_ff(0x80000000,0)  => 0
	Command Flow => 87654321   drv_mailbox_cpu_ff(0x80000000,9)  => 0
*/

#define ADDRESS_OFFSET	16
s32 drv_mailbox_cpu_ff(u32 pktmsg, u32 hw_number)
{
	u8 failCount=0;
#ifdef MAILBOX_DEBUG
	LOG_PRINTF("mailbox 0x%08x\n\r",(u32)((pktmsg >> ADDRESS_OFFSET) | (hw_number << HW_ID_OFFSET))));
#endif

	while (GET_CH0_FULL)
	{
	    if (failCount++ < 1000) continue;
            printf("ERROR!!MAILBOX Block[%d]\n", failCount);
            return FALSE;
	} //Wait until input queue of cho is not full.

	{
		SET_HW_PKTID((u32)((pktmsg >> ADDRESS_OFFSET) | (hw_number << HW_ID_OFFSET)));
		return TRUE;
	}
}

//Mount mailbox FIFO for HW module(high priority) 
s32 drv_mailbox_cpu_pri_ff(u32 pktmsg, u32 hw_number)
{
	u8 failCount=0;
#ifdef MAILBOX_DEBUG
	LOG_PRINTF("mailbox 0x%08x\n\r",(u32)((pktmsg >> ADDRESS_OFFSET) | (hw_number << HW_ID_OFFSET)));
#endif

	while(GET_CH0_FULL)
	{
		printf("ERROR!!MAILBOX Block[%d]\n",failCount++);
	};//Wait until input queue of cho is not full.


	{
		SET_PRI_HW_PKTID((u32)((pktmsg >> ADDRESS_OFFSET) | (hw_number << HW_ID_OFFSET)));
		return TRUE;
	}
}


s32 drv_mailbox_cpu_next(u32 pktmsg)
{
    PKT_Info *pkt_info=(PKT_Info *)pktmsg;
    u32 shiftbit, eng_id;

    // shiftbit = pkt_info->fCmdIdx << 2;
    pkt_info->fCmdIdx ++;
    shiftbit = pkt_info->fCmdIdx << 2;
    eng_id = ((pkt_info->fCmd & (0x0F<<shiftbit))>>shiftbit);	   

    return drv_mailbox_cpu_ff(pktmsg, eng_id);
}


void drv_mailbox_arb_ff(void)
{
#ifdef MAILBOX_DEBUG
		LOG_PRINTF("ARB FIFO=(%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x)\n\r", 
			GET_FF0_CNT,GET_FF1_CNT,GET_FF2_CNT,GET_FF3_CNT,GET_FF4_CNT,
			GET_FF5_CNT,GET_FF6_CNT,GET_FF7_CNT,GET_FF8_CNT,GET_FF9_CNT);
#endif
}

inline s32 ENG_MBOX_NEXT(u32 pktmsg){
	return drv_mailbox_cpu_next(pktmsg);
}

inline s32 ENG_MBOX_SEND(u32 hw_number,u32 pktmsg){
	return drv_mailbox_cpu_ff(pktmsg,hw_number);
}

inline s32 TX_FRAME(u32 pktmsg){
	return drv_mailbox_cpu_ff(pktmsg,M_ENG_HWHCI);
}

inline s32 FRAME_TO_HCI(u32 pktmsg){
	return drv_mailbox_cpu_ff(pktmsg,M_ENG_HWHCI);
}



