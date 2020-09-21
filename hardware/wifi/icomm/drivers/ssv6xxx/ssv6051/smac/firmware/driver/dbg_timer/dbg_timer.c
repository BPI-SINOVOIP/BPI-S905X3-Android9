#include <config.h>
#include "dbg_timer.h"
#include <regs.h>

#ifdef ENABLE_DEBUG_TIMER

#define DBG_TIME_ELEMENTS        (16)
static Time_T               dbg_time_elem[DBG_TIME_ELEMENTS];
static int                  current_idx = 0;
#endif // ENABLE_DEBUG_TIMER

void dbg_timer_init (void)
{
    #ifdef ENABLE_DEBUG_TIMER
    if (!GET_MTX_TSF_TIMER_EN)
    {
        SET_MTX_TSF_TIMER_EN(1);
    }
    #endif // ENABLE_DEBUG_TIMER
}

void dbg_timer_switch(int sw)
{
    #ifdef ENABLE_DEBUG_TIMER
    //if (!GET_MTX_TSF_TIMER_EN)
    {
        SET_MTX_TSF_TIMER_EN(sw);
    }
    #endif // ENABLE_DEBUG_TIMER
}

void dbg_get_time (Time_T *time)
{
    #ifdef ENABLE_DEBUG_TIMER
    time->ts.ut = GET_MTX_BCN_TSF_U;
    time->ts.lt = GET_MTX_BCN_TSF_L;
    #endif
}


u32 dbg_get_elapsed (Time_T *begin)
{
    #ifdef ENABLE_DEBUG_TIMER
    Time_T    cur_time;
    dbg_get_time(&cur_time);
    return dbg_calc_elapsed(begin, &cur_time);
    #else
    return 0;
    #endif
}


void dbg_timer_push (void)
{
    #ifdef ENABLE_DEBUG_TIMER
    int new_idx = current_idx + 1;
    if (new_idx > DBG_TIME_ELEMENTS)
        return;

    dbg_get_time(&dbg_time_elem[current_idx]);
    current_idx = new_idx;
    #endif // ENABLE_DEBUG_TIMER
    
} 


void dbg_timer_pop (const char *label)
{
    #ifdef ENABLE_DEBUG_TIMER
    u32 elapsed_time;
    if (current_idx == 0)
        return;

    --current_idx;
    elapsed_time = dbg_get_elapsed(&dbg_time_elem[current_idx]);

    if (label)
        printf("\n-> %d: %s elapsed %uus.\n", current_idx, label, 
               elapsed_time);
    else
        printf("\n -> %d time elapsed %uus.\n", current_idx, elapsed_time);
    #endif // ENABLE_DEBUG_TIMER
    
} 
