#ifndef _DRV_PMU_H_
#define _DRV_PMU_H_

#include <regs.h>
#ifdef __PMU_VERIFY__
#include "rtc/drv_rtc.h"
#include "log.h"
#endif  /*__PMU_VERIFY__ */

#define WAKEUP_EVENT_TIMER      1
#define WAKEUP_EVENT_INTERRUPT  2
//------------------------------------------------------------------------------
//
//  Function:   drv_pmu_setwake_cnt 
//
//  Description
//      Set PMU sleep wake cnt
//
//  Parameters
//      wake_cnt: time to sleep, count by 32K RC clock.
//
//  Return Value
//      0 indicates success.
//      -1 indicates failure.
//
//------------------------------------------------------------------------------
s32 drv_pmu_setwake_cnt (u32 t_ms);

//------------------------------------------------------------------------------
//
//  Function:   drv_pmu_sleep
//
//  Description
//      Set PMU go into sleep state
//
//  Parameters
//
//  Return Value
//      0 indicates success.
//      -1 indicates failure.
//
//------------------------------------------------------------------------------
s32 drv_pmu_sleep (void);

//------------------------------------------------------------------------------
//
//  Function:   drv_pmu_init
//
//  Description
//      Prepare works before PMU go to sleep mode, including:
//      1. Set DIGI_TOP_POR_MASK
//
//  Parameters
//
//  Return Value
//
//------------------------------------------------------------------------------
void drv_pmu_init(void);

//------------------------------------------------------------------------------
//
//  Function:   drv_pmu_chk
//
//  Description
//      Check the reason PMU go to ON state, and clear DIGI_TOP_POR_MASK
//
//  Parameters
//
//  Return Value
//
//------------------------------------------------------------------------------
void drv_pmu_chk (void);
bool drv_pmu_check_interupt_event(void);

#endif /* _DRV_PMU_H_ */

