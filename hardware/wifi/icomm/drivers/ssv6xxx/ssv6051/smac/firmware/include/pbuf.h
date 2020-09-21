#ifndef _PBUF_H_
#define _PBUF_H_

#include <ssv_pktdef.h>


void        *PBUF_MAlloc_Raw(u32 size, u32 need_header, PBuf_Type_E buf_type);
void		_PBUF_MFree(void *PKTMSG);




#define PBUF_BASE_ADDR	            0x80000000
#define PBUF_ADDR_SHIFT	            16
#define PBUF_ADDR_MAX               128



//#define PBUF_GetPktByID(_ID)        (PBUF_BASE_ADDR|((_ID)<<PBUF_ADDR_SHIFT))
//#define PBUF_GetPktID(_PKTMSG)      (((u32)_PKTMSG&0x0FFFFFFF)>>PBUF_ADDR_SHIFT)

#define PBUF_MapPkttoID(_PKT)		(((u32)_PKT&0x0FFFFFFF)>>PBUF_ADDR_SHIFT)	
#define PBUF_MapIDtoPkt(_ID)		(PBUF_BASE_ADDR|((_ID)<<PBUF_ADDR_SHIFT))

#define PBUF_isPkt(addr)    (((u32)(addr) >= (u32)PBUF_BASE_ADDR) && ((u32)(addr) <= (u32)PBUF_MapIDtoPkt(PBUF_ADDR_MAX-1)))



u32 PBUF_GetEngineID(PKT_Info *PktInfo, enum fcmd_seek_type seek);

#define PBUF_MAlloc(size, type)  PBUF_MAlloc_Raw(size, 1, type);
#define PBUF_MFree(PKTMSG) _PBUF_MFree(PKTMSG)



u32		PBUF_GetEngineID(PKT_Info *PktInfo, enum fcmd_seek_type seek);
s32		PBUF_Init(void);

#endif /* _PBUF_H_ */





