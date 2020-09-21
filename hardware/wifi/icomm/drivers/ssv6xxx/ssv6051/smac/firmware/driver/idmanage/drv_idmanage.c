#include <config.h>

#include "drv_idmanage.h"
#include <mbox/drv_mbox.h>

#include <regs.h>

//Rrerun Value  0x800(bytes) + 5bits packet number(1 byte) + 16bits address(4 bytes) -- 0x80010000 
//Allocate fail will return 0
//Retry 5 times or 50 times in ISR
void *drv_id_mng_alloc(u32 packet_len, PBuf_Type_E buf_type)
{
	u32	*packet_address;
	u32	 retryCount= gOsFromISR ? 50 : 5;

    if(packet_len)
    {
        do {
            packet_len |= buf_type << 16;
            SET_ALC_LENG(packet_len);
            packet_address = (u32 *)GET_MCU_PKTID;
            if (packet_address)
                return (void *)packet_address;
            else if (!gOsFromISR)
                OS_MsDelay(1);
    	} while (retryCount--);
    }

    return NULL;
}

u32 drv_id_mng_rls(void *packet_address)
{
	return ENG_MBOX_SEND(M_ENG_TRASH_CAN,(u32)packet_address);
}

//Show paccket buffer usage status 
u8 drv_id_mng_full(void)
{
	return GET_ID_FULL;
}

