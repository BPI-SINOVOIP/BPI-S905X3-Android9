#ifndef __DBG_TIMER_H__
#define __DBG_TIMER_H__

#include <types.h>

void dbg_timer_init (void);
void dbg_get_time (Time_T *time);
u32 dbg_get_elapsed (Time_T *begin);
void dbg_timer_push (void);
void dbg_timer_pop (const char *label);
void dbg_timer_switch(int sw);

#define dbg_calc_elapsed(begin, end) \
    ({ \
        Time_T elapsed_time; \
        elapsed_time.t = (end)->t - (begin)->t; \
        elapsed_time.ts.ut ? (u32)(-1) : elapsed_time.ts.lt; \
    })

#endif // __DBG_TIMER_H__

