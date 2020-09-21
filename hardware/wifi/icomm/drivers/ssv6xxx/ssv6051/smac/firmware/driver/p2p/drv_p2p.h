#ifndef _DRV_P2P_H_
#define _DRV_P2P_H_

//#include <regs.h>
//#include "ssv6200_configuration.h"



#define LOG_B_SIZE 100

#define P2P 1


#define P2P_TEST 0
#define P2P_TEST_DIFF_TIMER 1
#define P2P_TEST_DIFF_BEACON 0




struct drv_p2p_debug {
    u8 prev_state;
    u8 state;        
    u32 tick;
    u32 timer_count;
//    u32 diff;
//    u32 last_intval;
//    u32 diff2;
};



struct drv_p2p_debug_cal {
    
//    u32 internal_ts;    
//    u32 waiting_time;    
    u16 cal_state;
    u32 diff;                      //when will trigger the timer
    u32 start_time;
    u32 now;    
};



struct drv_p2p_noa_param {
    u32 duration;                   //ms-->us
    u32 interval;                   //ms-->us
    u32 start_time;                 //us
    u32 enable:8;
    u32 count:8;
    u8 addr[6];
    u8 vif_id;
//the abrove element mapping to struct ssv6xxx_p2p_noa_param
//-------------------------------------
    enum P2P_NOA_TIMER_TYPE {
        P2P_NOA_TIMER_US,
        P2P_NOA_TIMER_MS,
    } p2p_timer_type;

    u8 noa_running;               //noa operation is running.
    u8 noa_calculate;
    
    enum P2P_NOA_STATE {
        P2P_NOA_STATE_IDLE,
        P2P_NOA_STATE_WAIT_START_TIME,    
        P2P_NOA_STATE_ABENSE,
        P2P_NOA_STATE_AWAKE,

    } noa_state;
    
    s32 noa_running_count;                  //copy form count

    u32 last_bcn_ts;                        //last beacon time stamp
    u32 recv_bcn_cnt;                            //beacon counter
        
    u32 start_time_plus_offset;             //us        beacon start time plus delta.
    u32 awake_time;                         //us        internal value is already plus our shift
    u32 sleep_time;                         //us        internal value is already plus our shift


    u32 internal_start_time;                //transfer start time to local timer tick (ADR_MTX_BCN_TSF_L)
    u32 internal_start_time_plus_offset;    //transfer start time plus offset to local timer tick (ADR_MTX_BCN_TSF_L)


    
    
    //---------------------------------------------------------------------
    //for debug

    //phase 1  
    struct drv_p2p_debug_cal dbg_cal;
//    s32 tick_diff;

//    u32 c1;
//    u32 c2;
//    u32 c3;

    //pahse2
//    u32 last_tick_time;                             //fw time
//    u32 last_tick_interval;

//    u32 tick_zero;          //very precisely
//    u32 tick_one;           //
//    u32 tick_two;    

    u32 sync_cnt;
    u32 tmr_sync_state_mistach_cnt;
    u32 isr_sync_state_mistach_cnt;
//    u32 sync_set_cnt;
    u32 process_state_err;
    
    struct drv_p2p_debug debug_log[LOG_B_SIZE];
    u32 debug_cnt;

    u8 timer[100];
    u32 timer_cnt;    

    

#if P2P_TEST
    u32 beacon[LOG_B_SIZE];
    u32 tick[LOG_B_SIZE];
    u32 cnt;
#endif
    
};




void drv_p2p_process_noa(struct drv_p2p_noa_param *pDrvNoaParam, u16 valid_len);
void drv_p2p_isr_check_beacon(u32 rx_data);


#endif /* _DRV_P2P_H_ */

