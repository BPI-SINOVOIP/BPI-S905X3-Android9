#include <config.h>
#include <soc_global.h>
#include <log.h>
#include "drv_p2p.h"
#include <hdr80211.h>
#include <types.h>
#include <timer/drv_timer.h>
#include <rtos.h>






struct drv_p2p_noa_param g_drv_p2p_noa;
//#define os_tick2ms(_tick) ((_tick)*portTICK_RATE_MS)

#define P2P_HW_TIMER_US                    US_TIMER3//MS_TIMER1
#define P2P_HW_TIMER_MS                    MS_TIMER3//MS_TIMER1


//#define P2P_HW_TIMER_REMAIN_TIME_ADDR   ADR_TU2_CURRENT_MICROSECOND_TIME_VALUE//ADR_TM1_CURRENT_MILISECOND_TIME_VALUE





#define MS                              1000
//#define US                              1000
//#define TUtoUS(_x)                              ((_x)<<10)
#define P2P_NOA_SYNC_OFFSET             1*MS                            //ms

#define P2P_NOA_TIMER_OFFSET            5*MS                            //ms
#define P2P_NOA_TIMER_SLEEP_OFFSET      (P2P_NOA_TIMER_OFFSET*2)        //us

//hw timer just 16 bits, could be use only within 65535.
#define P2P_NOA_START_COUNT_DOWN_DIFF   0                //us
#define P2P_NOA_MIN_AWAKE_TIME          P2P_NOA_TIMER_SLEEP_OFFSET



//#define P2P_TIME_UNIT                   MS
//#define P2P_NOA_TIMER_UNIT_TO_MS        1
//#define P2P_NOA_TIMER_UNIT_TO_US        US
//#define P2P_NOA_TIMER_US_TO_TIME_UNIT   *1/1000





#define P2P_SYNC_TIMER_MASK             0x0

//#define P2P_HW_TIMER_MAX_VALUE          0xffff
//-------------------------------------------------
//function declare

void p2p_noa_timer_handler (void *data);
void	p2p_noa_send_evt(u8 evt_id);

//-------------------------------------------------
//function implement
#ifdef CONFIG_P2P_NOA

void p2p_noa_control_hw_queue(bool pause)
{
    u32 val= REG32(ADR_MTX_MISC_EN);
    val&=~(31<<MTX_HALT_Q_MB_SFT);
    
    if(pause)
        val|=31<<MTX_HALT_Q_MB_SFT;
    
    REG32(ADR_MTX_MISC_EN)=val;
}

//delta         ->us
//remain_time->us
void p2p_noa_process_state(u32 diff, u32 *remain_time){
    u8 state;
    u32 interval = g_drv_p2p_noa.interval;        //us
    u32 sleep_time = g_drv_p2p_noa.sleep_time;     //us
    do{
        
        diff = diff%interval;

        
        if(diff >= sleep_time){
            //awake phase
            state = P2P_NOA_STATE_AWAKE;

            //caculate remain time
            *remain_time = interval-diff;
        }else{
        
            //sleep phase
            state = P2P_NOA_STATE_ABENSE;

            //caculate remain time
            *remain_time = sleep_time-diff;
        }

        if(state != g_drv_p2p_noa.noa_state){
            //dosomething
            g_drv_p2p_noa.process_state_err++;
            
            g_drv_p2p_noa.noa_state = state;
            if(state == P2P_NOA_STATE_AWAKE)
                p2p_noa_control_hw_queue(FALSE);
            else
                p2p_noa_control_hw_queue(TRUE);

        }

    }while(0);

                
}

void inline p2p_noa_start_timer(u32 time){


    HW_TIMER *tmr = P2P_HW_TIMER_US;
    
    time--;

    if(time > HW_TIMER_MAX_VALUE_US){
        tmr = P2P_HW_TIMER_MS;
        time =time/MS;
        if(time <= 0)
            time=1;
        
    }            
    hwtmr_start(tmr, time, p2p_noa_timer_handler, (void*)(NULL), HTMR_ONESHOT);
}


//33us
void p2p_noa_caculate_start_timer(u32 bcn_now){


    u32 diff;//us
    u32 internal_ts = REG32(ADR_MTX_BCN_TSF_L);;

    do{            
        g_drv_p2p_noa.noa_state = P2P_NOA_STATE_WAIT_START_TIME;

        //hw timer just 16 bits, could be use only within 65535.
        if(time_after(g_drv_p2p_noa.start_time_plus_offset, bcn_now)){
            
            
            diff= g_drv_p2p_noa.start_time_plus_offset-bcn_now;

//            printf("[%08x]\n", diff);
            if( diff >= P2P_NOA_START_COUNT_DOWN_DIFF)
                break;

            g_drv_p2p_noa.dbg_cal.cal_state = 1;
        }else{            
            u32 remain_time;
            g_drv_p2p_noa.dbg_cal.cal_state = 2;
         
            //start time already pass. need to caculate we are in which phase now.
            diff = bcn_now-g_drv_p2p_noa.start_time_plus_offset;

            g_drv_p2p_noa.internal_start_time_plus_offset = internal_ts-diff;
            g_drv_p2p_noa.internal_start_time = g_drv_p2p_noa.internal_start_time_plus_offset+P2P_NOA_TIMER_OFFSET;
            
            p2p_noa_process_state(diff, &remain_time);
            diff = remain_time;
        }
       
        //disable cal timer.     
        g_drv_p2p_noa.noa_calculate=0;

        p2p_noa_start_timer(diff);

    }while(0);


//----------------------------------------------------
//debug info
//        g_drv_p2p_noa.dbg_cal.internal_ts       = internal_ts;
        g_drv_p2p_noa.dbg_cal.start_time    = g_drv_p2p_noa.start_time_plus_offset;
        g_drv_p2p_noa.dbg_cal.now           = bcn_now;
        g_drv_p2p_noa.dbg_cal.diff          = diff;
//----------------------------------------------------

                
}





u32 g_timer1_interval=0;
u32 g_timer1_tsf=0;

void p2p_noa_timer_sync_state(){
    u32 diff;          //us
    u32 remian_time;    //us
    u32 i_bcn_now = REG32(ADR_MTX_BCN_TSF_L);
    u32 prev_state = g_drv_p2p_noa.noa_state;

    do{
            
        if(g_drv_p2p_noa.noa_state != P2P_NOA_STATE_AWAKE && 
                g_drv_p2p_noa.noa_state != P2P_NOA_STATE_ABENSE)
            break;


        //check real beacon state
        {
            u8 cal_state;
            u32 a = i_bcn_now-g_drv_p2p_noa.internal_start_time;

            a=a%g_drv_p2p_noa.interval;
            if(a>g_drv_p2p_noa.duration)
                cal_state = P2P_NOA_STATE_AWAKE;
            else
                cal_state = P2P_NOA_STATE_ABENSE;

            if((cal_state == P2P_NOA_STATE_ABENSE)/*&&
                (cal_state != g_drv_p2p_noa.noa_state)*/)
                g_drv_p2p_noa.tmr_sync_state_mistach_cnt++;
        }
        
        g_drv_p2p_noa.sync_cnt++;
        
        diff = i_bcn_now - g_drv_p2p_noa.internal_start_time_plus_offset;
        p2p_noa_process_state(diff, &remian_time);
        
        remian_time--;
        if(remian_time<=0)
           remian_time=1;
        
        p2p_noa_start_timer(remian_time);

//-----------------------------------------
        //debug
        if(g_timer1_interval!=0){

            g_drv_p2p_noa.debug_log[g_drv_p2p_noa.debug_cnt].prev_state     = prev_state;
            g_drv_p2p_noa.debug_log[g_drv_p2p_noa.debug_cnt].state          = g_drv_p2p_noa.noa_state;
            g_drv_p2p_noa.debug_log[g_drv_p2p_noa.debug_cnt].tick           = REG32(ADR_MTX_BCN_TSF_L)-g_timer1_tsf;        //tick record
            g_drv_p2p_noa.debug_log[g_drv_p2p_noa.debug_cnt].timer_count    = g_timer1_interval;                            //timer count
        }

        g_timer1_interval = remian_time;
        g_timer1_tsf = REG32(ADR_MTX_BCN_TSF_L);
        g_drv_p2p_noa.debug_cnt=(g_drv_p2p_noa.debug_cnt+1)%LOG_B_SIZE;
//--------------------------------------------    
        
    }while(0);
                  
    
}



void p2p_noa_isr_sync_state(u32 bcn_now){

//    u32 remian_time;    //us

    do{
            
        if(g_drv_p2p_noa.noa_state != P2P_NOA_STATE_AWAKE && 
                g_drv_p2p_noa.noa_state != P2P_NOA_STATE_ABENSE)
            break;
                         

        {       
            u8 cal_state;
            u32 a = bcn_now-g_drv_p2p_noa.start_time;

            a=a%g_drv_p2p_noa.interval;
            if(a>g_drv_p2p_noa.duration)
                cal_state = P2P_NOA_STATE_AWAKE;
            else
                cal_state = P2P_NOA_STATE_ABENSE;

             if((cal_state == P2P_NOA_STATE_ABENSE)&&
                (cal_state != g_drv_p2p_noa.noa_state))
                g_drv_p2p_noa.isr_sync_state_mistach_cnt++;            
        }

        
//        p2p_noa_process_state(delta, &remian_time);
        
                                
    }while(0);
                  
    
}

#if P2P
void p2p_noa_timer_handler (void *data)
{
//    s32 delta=0;      //ms
    
    switch (g_drv_p2p_noa.noa_state) {
        
	    case P2P_NOA_STATE_WAIT_START_TIME:
            p2p_noa_control_hw_queue(TRUE);
            g_drv_p2p_noa.noa_state = P2P_NOA_STATE_ABENSE;
//            p2p_noa_timer_sync_state();
            break;

        case P2P_NOA_STATE_ABENSE:
            g_drv_p2p_noa.noa_state = P2P_NOA_STATE_AWAKE;
            p2p_noa_control_hw_queue(FALSE);
            
            if(g_drv_p2p_noa.noa_running_count != 255)
                g_drv_p2p_noa.noa_running_count--;
            
            if(g_drv_p2p_noa.noa_running_count <= 0){
                //restore to default value
                g_drv_p2p_noa.noa_running = 0;
                g_drv_p2p_noa.noa_state = P2P_NOA_STATE_IDLE;
                p2p_noa_send_evt(SSV6XXX_NOA_STOP);
                goto out;              
//            }else{
//
//                 p2p_noa_timer_sync_state();
            }            
            break;
            
        case P2P_NOA_STATE_AWAKE:

            g_drv_p2p_noa.noa_state = P2P_NOA_STATE_ABENSE;
            p2p_noa_control_hw_queue(TRUE);
//            p2p_noa_timer_sync_state();
                        
            break;

        default:
            printf("timer handler-state error[%d]\n", g_drv_p2p_noa.noa_state);
            return;
            break;
    }


    p2p_noa_timer_sync_state();        
   

    

//debug
out:
{;}
    
    

 
    
#if 0        
    p_p2p_noa_param->beacon[p_p2p_noa_param->cnt]=0;
    p_p2p_noa_param->tick[p_p2p_noa_param->cnt]=0;
    
    p_p2p_noa_param->cnt++;
    if(p_p2p_noa_param->cnt >= LOG_B_SIZE)
        hwtmr_stop(P2P_HW_TIMER);
#endif    
}
#endif




void drv_p2p_dump_noa(int r)
{

    
    if(r==0){
        
        printf("===============================================>\n");
        printf("NOA Parameter:\nEnable=%d\nInterval=%d\nDuration=%d\nStart_time=0x%08x\nCount=%d\nAddr=[%02x:%02x:%02x:%02x:%02x:%02x]vif[%d]\n\n", 
                                g_drv_p2p_noa.enable,
                                g_drv_p2p_noa.interval, 
                                g_drv_p2p_noa.duration, 
                                g_drv_p2p_noa.start_time, 
                                g_drv_p2p_noa.count,
                                g_drv_p2p_noa.addr[0],
                                g_drv_p2p_noa.addr[1],
                                g_drv_p2p_noa.addr[2],
                                g_drv_p2p_noa.addr[3],
                                g_drv_p2p_noa.addr[4],
                                g_drv_p2p_noa.addr[5], 
                                g_drv_p2p_noa.vif_id);  
    
        printf("Internal Parameter:\n"            
            "start[%d]\n"
            "start_offset[%d]\n"            
            "internal start[%d]\n"
            "internal start_offset[%d]\n"            
            "awake_time=[%d]\n"
            "sleep_time=[%d]\n"          
            "---\n"        
            "noa_running=%d\n"
            "noa_calculate=%d\n"
            "noa_state=%d\n"
            "noa_running_count=%d\n"
            "---\n",
            
            g_drv_p2p_noa.start_time,
            g_drv_p2p_noa.start_time_plus_offset,
            g_drv_p2p_noa.internal_start_time,
            g_drv_p2p_noa.internal_start_time_plus_offset,                  
            g_drv_p2p_noa.awake_time,
            g_drv_p2p_noa.sleep_time,
                 
            g_drv_p2p_noa.noa_running,
            g_drv_p2p_noa.noa_calculate,
            g_drv_p2p_noa.noa_state,
            g_drv_p2p_noa.noa_running_count
            
            );

         printf("last bcn timestmp[%d] recv_bcn_cnt[%d]\n", g_drv_p2p_noa.last_bcn_ts, g_drv_p2p_noa.recv_bcn_cnt);

         return;

    }else if(r==1){ 
    
        printf("===============================================>\n");

        printf("cal_state[%d] start[0x%08x] now[0x%08x] diff[%d]\n",
                                g_drv_p2p_noa.dbg_cal.cal_state,                               
                                g_drv_p2p_noa.dbg_cal.start_time,
                                g_drv_p2p_noa.dbg_cal.now,
                                g_drv_p2p_noa.dbg_cal.diff
                                );
       
       
        return;        
    }else if(r==2){
        int i;
        printf("timer_sync_state\n");
        for(i=0;i<LOG_B_SIZE;i++){
            
            printf("prev_state[%d] state[%d] tick record[%08d]=timer count[%08d]*1.025\n",
                g_drv_p2p_noa.debug_log[i].prev_state,
                g_drv_p2p_noa.debug_log[i].state,
                g_drv_p2p_noa.debug_log[i].tick,
                g_drv_p2p_noa.debug_log[i].timer_count
            );
        }
        return;
#if P2P_TEST        
    }else if(r==3){ 
        int i;
        for(i=0;i<LOG_B_SIZE;i++){
             printf("[%d] [%d]\n", g_drv_p2p_noa.tick[i], g_drv_p2p_noa.beacon[i]);
        }
        return;
#endif  
    }else if(r==4){

        printf("process_state_err[%d] sync_cnt[%d]\ntmr_sync_state_mistach[%d] isr_sync_state_mistach[%d]\n",
                                g_drv_p2p_noa.process_state_err,
                                g_drv_p2p_noa.sync_cnt,
                                g_drv_p2p_noa.tmr_sync_state_mistach_cnt,
                                g_drv_p2p_noa.isr_sync_state_mistach_cnt                                
                                );      
    }
    else
    {;}
    
    

}



void inline drv_p2p_noa_reset(){
    drv_p2p_dump_noa(0);
    memset(&g_drv_p2p_noa, 0, sizeof(struct drv_p2p_noa_param));
    hwtmr_stop(P2P_HW_TIMER_US);
    hwtmr_stop(P2P_HW_TIMER_MS);
    p2p_noa_control_hw_queue(FALSE);
}

void drv_p2p_process_noa(struct drv_p2p_noa_param *pDrvNewNoaParam, u16 valid_len)
{   

    ssv6xxx_host_noa_event noa_event = SSV6XXX_NOA_STOP;
    if(valid_len != 21)
    {
        printf("Wrong noa parameters....reject to set noa\n");
        return;
        //valid_len = 20;
    }
    drv_p2p_noa_reset();
    memcpy(&g_drv_p2p_noa, pDrvNewNoaParam, valid_len);

    if(pDrvNewNoaParam->enable){
        noa_event = SSV6XXX_NOA_START;

//------------------------------------------------
        /*  Adjust 
               * g_drv_p2p_noa.interval
               * g_drv_p2p_noa.sleep_time
               * g_drv_p2p_noa.awake_time
               */

        //process interval/sleep time/awake time
        if( (g_drv_p2p_noa.interval == 0) || 
            (g_drv_p2p_noa.interval == g_drv_p2p_noa.duration)){

            /*sleep all the time*/
            g_drv_p2p_noa.interval = g_drv_p2p_noa.duration+P2P_NOA_TIMER_SLEEP_OFFSET;
            g_drv_p2p_noa.sleep_time = g_drv_p2p_noa.interval;
            g_drv_p2p_noa.awake_time = 0;

        }else{
            
            g_drv_p2p_noa.sleep_time = g_drv_p2p_noa.duration+P2P_NOA_TIMER_SLEEP_OFFSET;
            
            if(g_drv_p2p_noa.sleep_time >= g_drv_p2p_noa.interval){

                //plus delta cause no space to awake
                drv_p2p_noa_reset();
                noa_event = SSV6XXX_NOA_STOP;
                
                goto out;
            }else{
                g_drv_p2p_noa.awake_time = g_drv_p2p_noa.interval - g_drv_p2p_noa.sleep_time;
            }
        }
                    
        //check if our hw timer can support NOA parameter      
        if((g_drv_p2p_noa.sleep_time > HW_TIMER_MAX_VALUE_MS) ||
           (g_drv_p2p_noa.awake_time > HW_TIMER_MAX_VALUE_MS)){
           printf("[ERROR]==>HW timer can not fit NOA parameter\n");

           drv_p2p_noa_reset();
           noa_event = SSV6XXX_NOA_STOP;
           goto out;
        }
//------------------------------------------------

        //Finally. NOA is going to start.
                    
        g_drv_p2p_noa.noa_running_count = g_drv_p2p_noa.count;
        g_drv_p2p_noa.start_time_plus_offset = (g_drv_p2p_noa.start_time-(P2P_NOA_TIMER_OFFSET));

        g_drv_p2p_noa.noa_calculate = 1;
        g_drv_p2p_noa.noa_state = P2P_NOA_STATE_IDLE;
    }

out:
    
    drv_p2p_dump_noa(0);
    p2p_noa_send_evt(noa_event);
    
    if(pDrvNewNoaParam->enable)
        g_drv_p2p_noa.noa_running = 1;



   
//-----------
//test code

#if P2P_TEST


#define TEST_TIMER US_TIMER1
#define TEST_TIME 46003

#if P2P_TEST_DIFF_TIMER
    hwtmr_start(TEST_TIMER, TEST_TIME, p2p_noa_timer_handler, (void*)NULL, HTMR_ONESHOT);
#endif
#endif



}

#if P2P
//ISR
void drv_p2p_isr_check_beacon(u32 rx_data){
    
    u32 timestamp = *(u32 *)(rx_data + GET_PB_OFFSET+HDR80211_MGMT_LEN);
    u8 *addr = (u8 *)(rx_data + GET_PB_OFFSET+10);
    //printf("beacon\n");

    if(g_drv_p2p_noa.noa_running && (IS_EQUAL_MACADDR((const s8 *)addr, (const s8 *)g_drv_p2p_noa.addr))){

        g_drv_p2p_noa.last_bcn_ts = timestamp;
        g_drv_p2p_noa.recv_bcn_cnt++;

        if((g_drv_p2p_noa.noa_state > P2P_NOA_STATE_WAIT_START_TIME)&& 
            (g_drv_p2p_noa.noa_calculate==0)){
                                    
            if((g_drv_p2p_noa.recv_bcn_cnt&P2P_SYNC_TIMER_MASK) == 0)
                 p2p_noa_isr_sync_state(timestamp);
        }

        if(g_drv_p2p_noa.noa_calculate)
            p2p_noa_caculate_start_timer(timestamp);                      
        
    }
}
#endif



void p2p_noa_send_evt(u8 evt_id)
{
	u16				len;
	MsgEvent		*msg_evt;
	struct ssv62xx_noa_evt *noa_evt;
	len = sizeof(struct ssv62xx_noa_evt);
	HOST_EVENT_ALLOC(msg_evt, SOC_EVT_NOA, len);

    if(msg_evt){

        printf("p2p_noa_send_evt [%d] len[%d]\n", evt_id, len);
	    noa_evt = (struct ssv62xx_noa_evt *)(LOG_EVENT_DATA_PTR(msg_evt));
        noa_evt->evt_id = evt_id;

	    HOST_EVENT_SET_LEN(msg_evt, len);
	    HOST_EVENT_SEND(msg_evt);
    }

	return;
}




//=====================================================================================
//testing function.
#if P2P_TEST
u32 last=0;
u32 blast=0;
void drv_p2p_isr_check_beacon(u32 rx_data){

#if P2P_TEST_DIFF_BEACON
    u32 timestamp = *(u32 *)(rx_data + GET_PB_OFFSET+HDR80211_MGMT_LEN);
    u8 *addr = (u8 *)(rx_data + GET_PB_OFFSET+10);
    //printf("beacon\n");

    if(g_drv_p2p_noa.noa_running && (IS_EQUAL_MACADDR((const s8 *)addr, (const s8 *)g_drv_p2p_noa.addr))){

        u32 now = REG32(ADR_MTX_BCN_TSF_L);
          //hwtmr_start(TEST_TIMER, TEST_TIME, p2p_noa_timer_handler, (void*)NULL, HTMR_ONESHOT);
     
        if (g_drv_p2p_noa.cnt<LOG_B_SIZE){
            g_drv_p2p_noa.beacon[g_drv_p2p_noa.cnt]= (timestamp-blast);
            g_drv_p2p_noa.tick[g_drv_p2p_noa.cnt++]= now-last;
            last = now;
            blast = timestamp;
        }
    }
#endif
    
}


u32 t=60000;
void p2p_noa_timer_handler (void *data){

#if P2P_TEST_DIFF_TIMER
     u32 now = REG32(ADR_MTX_BCN_TSF_L);

     if(t==60000){
        p2p_noa_control_hw_queue(FALSE);
        t=35000;
     }
     else{
        p2p_noa_control_hw_queue(TRUE);
        t=60000;
     }
     printf("[%d]\n", t);
     hwtmr_start(TEST_TIMER, t, p2p_noa_timer_handler, (void*)NULL, HTMR_ONESHOT);

     if (g_drv_p2p_noa.cnt<LOG_B_SIZE)
        g_drv_p2p_noa.tick[g_drv_p2p_noa.cnt++]= now-last; 
     
     last = now;
#endif

}


#endif//P2P_TEST
//=====================================================================================    
    

#endif//#ifdef CONFIG_P2P_NOA
