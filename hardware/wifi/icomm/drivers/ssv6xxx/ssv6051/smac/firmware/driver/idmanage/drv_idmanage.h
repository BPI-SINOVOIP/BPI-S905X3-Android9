#ifndef _DRV_IDMANAGE_H_
#define _DRV_IDMANAGE_H_

#include <ssv_pktdef.h>

void *drv_id_mng_alloc(u32 packet_len, PBuf_Type_E buf_type);
u32 drv_id_mng_rls(void *packet_address);
u8 drv_id_mng_full(void);

#endif /* _DRV_PBUF_H_ */

