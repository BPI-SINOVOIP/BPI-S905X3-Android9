#ifndef _CMD_DEF_H_
#define _CMD_DEF_H_
#include "common.h"
#include <ssv_pktdef.h>

/************************************************************************************************************************************************/
/*                                                                Extra Info                                                                    */
/************************************************************************************************************************************************/


/************************************************************************************************************************************************/
/*                                                                Host Command                                                                    */
/************************************************************************************************************************************************/

typedef struct cmd_seq_ctrl_st{
	u32 cmd_seq_no;
}CMD_SEQ_CTRL;

//=========================================
//                Public Command Data
//=========================================



//=========================================
//                Public Event Data
//=========================================

#if 0
struct resp_evt_result {
    union{
        bool is_sucessful;  //Scan, SetHwMode
        s32  status_code;   //Join    
   }u;
   s32 aid;
   
} ;//__attribute__ ((packed));
#endif

#define  CMD_SUCESS 0
#define CMD_PASS_FRAME 1

struct resp_evt_result {
	#ifdef USE_CMD_RESP
	u8		cmd;
	u8		result;
	u32     cmd_seq_no;
	#endif // USE_CMD_RESP
	union {     
        #if 0		
        struct {
   		 u8                   		result_code;//SUCCESS,PASS_FRAME
		 struct ssv6xxx_ieee80211_bss 	bss_info;
         u16                  		dat_size;
		 u8                   		dat[0];//for saving probe resp
        } scan;		
        struct {
		 s32 			status_code;
		 s32			aid;
        } join;
        struct {
         s16			reason_code;
        } leave;
		struct {
			u8 policy;
			u8 dialog_token;
			u16 tid;
			u16 agg_size;
			u16 timeout;
			u16 start_seq_num;			
		} addba_req;
		struct {
			u16 initiator;
			u16 tid;
		 	u16 reason_code;
		} delba_req;
		#endif
        u8 dat[0];
    } u;
};

#define RESP_EVT_HEADER_SIZE		((size_t)(&((struct resp_evt_result *)100)->u.dat[0]) - 100U)
#define CMD_RESPONSE_BASE_SIZE		(sizeof(HDR_HostEvent) + RESP_EVT_HEADER_SIZE)


//=========================================
//                Private Event Data
//=========================================


typedef enum _CmdResult_E {
	CMD_OK,             // Command executed successfully. Check corresponding returned values for status.
	CMD_INVALID,        // Invalid command.
	CMD_STATE_ERROR,    // Not executable in current firmware state.
	CMD_TIMEOUT,        // Peer does not response in time. Command expired.
	CMD_BUSY,           // Too busy to accept new command.
	CMD_NOMEM,          // Out of resource to execute this command
} CmdResult_E;


struct MsgEvent_st;
//typedef struct MsgEvent_st MsgEvent;

struct MsgEvent_st *HostEventAlloc(ssv6xxx_soc_event hEvtID, u32 size);

struct MsgEvent_st *HostCmdRespEventAlloc(const struct cfg_host_cmd *host_cmd, CmdResult_E cmd_result, u32 resp_size, void **p_resp_data);

/**
 * Define Macros for host event manipulation: 
 *
 * @ HOST_EVENT_ALLOC():    Allocate a host event structure from the system.
 * @ HOST_EVENT_SET_LEN(): Set the host event length. The length excludes 
 *                                             the event header length.
 * @ HOST_EVENT_SEND():      Send the event to the host.
 */
#define HOST_EVENT_SET_LEN(ev, l)                       \
{                                                       \
    ((HDR_HostEvent *)((ev)->MsgData))->len =          \
    (l) + sizeof(HDR_HostEvent);                        \
}
#define HOST_EVENT_ALLOC_RET(ev, evid, l, ret)          \
{                                                       \
    (ev) = HostEventAlloc(evid, l);                     \
    ASSERT_RET(ev, ret);                                \
}
#define HOST_EVENT_ALLOC(ev, evid, l)                   \
{                                                       \
    (ev) = HostEventAlloc(evid, l);                     \
    ASSERT_RET(ev, EMPTY);                              \
}
#define HOST_EVENT_DATA_PTR(ev)                         \
    ((HDR_HostEvent *)((ev)->MsgData))->dat
#define HOST_EVENT_ASSIGN_EVT_NO(ev,no)			\
{														\
	 (((HDR_HostEvent *)((ev)->MsgData))->evt_seq_no)=no;	\
}

#define HOST_EVENT_ASSIGN_EVT(ev, evt_id)                   \
    do {                                                   \
	    ((HDR_HostEvent *)((ev)->MsgData))->h_event = evt_id;\
    } while (0)

#ifdef USE_CMD_RESP
#define HOST_EVENT_SEND(ev)                             \
{                                                       \
    extern u32 sg_soc_evt_seq_no;                       \
	sg_soc_evt_seq_no++;                                \
	HOST_EVENT_ASSIGN_EVT_NO(MsgEv,sg_soc_evt_seq_no);  \
    msg_evt_post(MBOX_CMD_ENGINE, (ev));                \
}
#else
#define HOST_EVENT_SEND(ev)                             \
{                                                       \
    msg_evt_post(MBOX_CMD_ENGINE, (ev));                \
}
#endif // USE_CMD_RESP


#define CMD_RESP_ALLOC(evt_msg, host_cmd, cmd_result, resp_size, p_resp_data) \
    do { \
        evt_msg = host_cmd_resp_alloc(host_cmd, cmd_result, resp_size, p_resp_data); \
        if (evt_msg == NULL) \
            return; \
    } while (0)

#define CMD_RESP_ALLOC_RET(evt_msg, host_cmd, cmd_result, resp_size, p_resp_data, fail_ret) \
    do { \
        evt_msg = host_cmd_resp_alloc(host_cmd, cmd_result, resp_size, p_resp_data); \
        if (evt_msg == NULL) \
            return fail_ret; \
    } while (0)

#endif//_CMD_DEF_H_

