#ifndef _MSGEVT_H_
#define _MSGEVT_H_
#include <config.h>
#include <soc_global.h>



/**
 *  Define Message Type for MsgType:
 *
 *  Note that we assign ID from 10 due to that ID 0-9 are reserved for 
 *  lwip-tcpip message type.
 *
 *  @ MEVT_MSG_PKTBUFF
 *  @ MEVT_MSG_SOFT_TIMER
 *  @ MEVT_MSG_HOST_TIMER
 */
typedef enum msgevt_type_en
{
    MEVT_PKT_BUF                        = 10,
    MEVT_PS_DOZE                            ,
    MEVT_PS_WAKEUP                          ,
    MEVT_PS_DEEP                            ,
#ifdef FW_WSID_WATCH_LIST
    MEVT_WSID_OP                           ,
#endif
//	MEVT_DTIM_EXPIRED						,
} msgevt_type;


struct MsgEvent_st
{
	msgevt_type     MsgType;
    u32             MsgData;
    u32             MsgData1;    
    u32             MsgData2;
	u32             MsgData3;
};

typedef struct MsgEvent_st MsgEvent;
typedef struct MsgEvent_st *PMsgEvent;

/**
* Define Message Event Queue:
* 
* @ MBOX_SOFT_MAC
* @ MBOX_MLME
* @ MBOX_CMD_ENGINE
* @ MBOX_HCMD_ENGINE
* @ MBOX_TCPIP
*/

enum MBX_IDX{
    SOFT_MAC=0,
//    MLME,
    CMD_ENG,
};
#define MBOX_SOFT_MAC           g_soc_task_info[SOFT_MAC].qevt
//#define MBOX_MLME               g_soc_task_info[MLME].qevt
#define MBOX_CMD_ENGINE         g_soc_task_info[CMD_ENG].qevt


#define MBOX_HCMD_ENGINE        g_host_task_info[0].qevt
#define MBOX_SIM_DRIVER         g_host_task_info[1].qevt
#define MBOX_SIM_TX_DRIVER      g_host_task_info[2].qevt

#define MBOX_TCPIP 


void *msg_evt_alloc_0(void);
void *msg_evt_alloc(void);
void  msg_evt_free(MsgEvent *msgev);
s32   msg_evt_post(OsMsgQ msgevq, MsgEvent *msgev);
s32   msg_evt_fetch(OsMsgQ msgevq, MsgEvent **msgev);
s32   msg_evt_init(void);
s32	  msg_evt_post_data1(OsMsgQ msgevq, msgevt_type msg_type, u32 msg_data, bool bPostFront);

#endif /* _MSGEVT_H_ */

