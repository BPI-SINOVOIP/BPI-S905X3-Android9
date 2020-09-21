#include <config.h>
#include <ssv_regs.h>
//#include <log.h>
#include <pbuf.h>
#include <hwmac/drv_mac.h>
#include <idmanage/drv_idmanage.h>


static OsMutex sg_pbuf_mutex;





/**
 *  Packet buffer driver for SSV6200 Hardware Packet Buffer Manipulation.
 *  Three APIs are implemented for this driver:
 *
 *      @ PBUF_Init()           -Initialize hardware packet buffer setting 
 *                                      and a mutex.
 *      @ PBUF_MAlloc()       -Request a packet buffer from hardware.      
 *      @ PBUF_MFree()        -Release a packet buffer to hardware.
 *
 */


s32 PBUF_Init(void)
{
    OS_MutexInit(&sg_pbuf_mutex);
    return OS_SUCCESS;
}

static PKT_Info* __PBUF_MAlloc_0(u32 size, u32 need_header, PBuf_Type_E buf_type)
{
    PKT_Info *pkt_info;
	u32 extra_header=0;
    if (need_header)
    {
        extra_header = (GET_PB_OFFSET + GET_TX_PKT_RSVD*TX_PKT_RES_BASE);
	    size += extra_header;
    }
	
    /**
        * The following code is for simulation/emulation platform only.
        * In real chip, this code shall be replaced by manipulation of 
        * hardware packet engine.
        */
    /* Request a packet buffer from hardware */
    pkt_info = (PKT_Info *)drv_id_mng_alloc(size, buf_type);
	if(extra_header)
		memset(pkt_info, 0, extra_header);
    
    return pkt_info;
}

void *PBUF_MAlloc_Raw(u32 size, u32 need_header, PBuf_Type_E buf_type)
{
    PKT_Info *pkt_info;
    if (gOsFromISR)
    {
        pkt_info = __PBUF_MAlloc_0(size,need_header,buf_type);
    }
    else
    {
            
        OS_MutexLock(sg_pbuf_mutex);

        pkt_info = __PBUF_MAlloc_0(size,need_header,buf_type);

        OS_MutexUnLock(sg_pbuf_mutex);
    }
    return (void *)pkt_info;
}


static void __PBUF_MFree_0(void *PKTMSG)
{   
    /**
        * The following code is for simulation/emulation platform only.
        * In real chip, this code shall be replaced by manipulation of 
        * hardware packet engine.
        */ 
    ASSERT(drv_id_mng_rls(PKTMSG)==TRUE); 

}

static inline void __PBUF_MFree_1(void *PKTMSG)
{   
    OS_MutexLock(sg_pbuf_mutex);

    __PBUF_MFree_0(PKTMSG);    

    OS_MutexUnLock(sg_pbuf_mutex);
}

void _PBUF_MFree (void *PKTMSG)
{
	if (gOsFromISR)
		__PBUF_MFree_0(PKTMSG);
	else
		__PBUF_MFree_1(PKTMSG);
}






