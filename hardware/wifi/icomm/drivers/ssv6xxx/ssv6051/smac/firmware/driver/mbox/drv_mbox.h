#ifndef _DRV_MBOX_H_
#define _DRV_MBOX_H_



#define HW_ID_OFFSET                    7


extern inline s32 ENG_MBOX_NEXT(u32 pktmsg);
extern inline s32 ENG_MBOX_SEND(u32 hw_number,u32 pktmsg);
extern inline s32 TX_FRAME(u32 pktmsg);
extern inline s32 FRAME_TO_HCI(u32 pktmsg);




#endif /* _DRV_MBOX_H_ */

