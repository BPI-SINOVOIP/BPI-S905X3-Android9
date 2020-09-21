#include <config.h>
#include "drv_timer.h"

// ToDo:
// For static IRQ table. Use local declared function as IRQ 
// callback function then deligate to user's
void irq_timer_handler (void *data)
{
}

s32 hwtmr_stop(HW_TIMER *tmr)
{
    u32 irq = (u32)HTMR_GET_IRQ(tmr);
    tmr->TMR_CTRL = 0x00;
    return irq_request(irq, NULL, NULL);
}


s32 hwtmr_start(HW_TIMER *tmr, u16 count, irq_handler tmr_handle,
                void *m_data, enum hwtmr_op_mode mode)
{
    u32 irq = (u32)HTMR_GET_IRQ(tmr);


   // printf("request irq %d", irq);
    //only for this version
    //if (irq > 23)
    //    irq = irq - 4;
    
    tmr->TMR_CTRL = HTMR_COUNT_VAL(count)|HTMR_OP_MODE(mode);
    
    return irq_request(irq, tmr_handle, m_data);
}


void hwtmr_init(void)
{
    s32 i;
    for(i=0; i<MAX_HW_TIMER; i++) {
        /*lint -save -e845 */
        REG_TIMER[i].TMR_CTRL = 
        HTMR_COUNT_VAL(0)|HTMR_OP_MODE(HTMR_ONESHOT);
        /*lint -restore */
    }
}


















