#include <config.h>
#include <regs.h>
#include <rtos.h>
#include <log.h>
#include "cpu.h"
#include "irq.h"

#ifdef ENABLE_MEAS_IRQ_TIME
#include <dbg_timer/dbg_timer.h>

Time_T    irq_time;
u32       irq_max_proc_time;
#endif

/* Debug statistic counter for IRQ: */
#define DEBUG_UPDATE_IRQ_COUNT(x)           (x)->irq_count++

struct irq_entry {
    s32             irq_no;
    void *          irq_data;
    irq_handler     irq_handle;
    u32             irq_count;
};


// extern u8 gOsFromISR;
static struct irq_entry *main_irq_tbl;


#if (CONFIG_SIM_PLATFORM == 0)
void irq_disable( void )
{
    asm volatile (
    "stmdb  sp!, {R0}       \n\t"
    "mrs    r0, CPSR        \n\t"
    "orr    r0, r0, #0xC0   \n\t"
    "msr    CPSR, r0        \n\t"
    "ldmia  sp!, {r0}       \n\t"
    );
}


void irq_enable( void )
{
    asm volatile(
    "stmdb  sp!, {r0}       \n\t"
    "mrs    r0, CPSR        \n\t"
    "bic    r0, r0, #0xC0   \n\t"
    "msr    CPSR, r0        \n\t"
    "ldmia  sp!, {r0}       \n\t"
    );
}

#else
#define irq_enable()
#define irq_disable()
#endif


static void irq_peri_handler(void *m_data)
{
    const struct irq_entry *irq_en, *max_en;
    u32 IrqStatus;
    
    /**
        *  Peripheral Group ISR! If there is an irq from peripheral, the 
        *  irq_dispatch() will dispatch it to this function for further 
        *  processing.
        */

    /* Get Interrupt status * clear interrupt immediately */
    IrqStatus = REG_INTR_CTRL->INT_PERI_STS;
    REG_INTR_CTRL->INT_PERI_STS = IrqStatus;
    
    /* Start of peripheral IRQ handle table */
    irq_en = (struct irq_entry *)&main_irq_tbl[MAX_MAIN_IRQ_ENTRY];
    max_en = irq_en + MAX_PERI_IRQ_ENTRY;
    
    for( ; irq_en<max_en; irq_en++ ) {
        if ( IrqStatus & (1<<irq_en->irq_no) ) {
            /**
                      *  Note: we DO NOT check the validation of m_func().
                      *  This should be guaranteed in irq_request().
                      */
            irq_en->irq_handle(irq_en->irq_data);
        }
    }
    
}



#if (CONFIG_SIM_PLATFORM == 0)
void do_exception( void ) __attribute__ ((interrupt("IRQ")));
void do_exception( void )
{
    s32 i;
    /*lint -save -e522 */
    cpu_get_registers();
    /*lint -restore */
    printf("CPU Exception!!\nCPSR:0x%08X  SPSR:0x%08X  ", gCpuRegs[0], gCpuRegs[1]);
    for(i=2; i<18; i++)
    {
        printf(" R%02d:0x%08X  ", i-2, gCpuRegs[i]);
        if (((i+1)%4) == 0)
            printf("\n");
    }
    printf("\n");
}
#endif

portTickType xCurrentTick;
static portTickType xLastTickCnt = 0;

inline void reset_phy()
{
    //volatile u32 *reset_phy = (u32 *)(ADR_PHY_EN_1);
    //*reset_phy = 0x0000017E;
    //*reset_phy = 0x0000017F;
    u32 debug_count = 0;

    //Stop all TX queue
    SET_MTX_HALT_Q_MB(MTX_QUEUE_MASK);

    //Dectect TX is pending
    while (GET_MTX_DMA_FSM)
    {
        debug_count++;
        if(debug_count > 9999)
        {
            printf("*Error ADR_MTX_STATUS is locking*\n");
            break;
        }
    };

    SET_RG_PHY_MD_EN(0);
    SET_RG_PHY_MD_EN(1);

    //Resume all TX queue
    SET_MTX_HALT_Q_MB(0x0);

    //printf("******************\n");
    printf("*   PHY RESET    *\n");
    //printf("******************\n");
}

void adjust_scrambler_seed()
{
    static u8 scrambler_seed = 1;
    
    SET_RG_SMB_DEF(scrambler_seed);
    scrambler_seed++;

    if (scrambler_seed == 0x80) {
        scrambler_seed = 1;
    }
}

void rx_packet_check()
{
    //ASSERT(gOsFromISR == false);
    gOsFromISR = true;
    
    REG32(ADR_RX_11B_PKT_STAT_EN) = 0x00100001;
    REG32(ADR_PHY_REG_18_RSSI_SNR) = 0x01100004;
    volatile u32 b_len = GET_B_LENGTH_FIELD;
    volatile u32 b_signal = GET_SIGNAL_FIELD;
    volatile u32 b_state = GET_RO_TBUS_O >> 16;
#ifdef DEBUG_RXEN_CNT
    SET_RG_PACKET_STAT_EN_11GN(1);
#endif //DEBUG_RXEN_CNT

    xCurrentTick = xTaskGetTickCountFromISR();  
    if ((xCurrentTick - xLastTickCnt) >= 11) {
        volatile u32 rxen_cnt = GET_RO_MRX_EN_CNT;
        static volatile u32 last_rxen_cnt = 0;
        u32 ret = 0;
       
        if (last_rxen_cnt == rxen_cnt && b_state == 0x8) {
#ifdef DEBUG_RXEN_CNT
            printf("gn: 0x%x, b: 0x%x, rx_en: %d, b_len: 0x%x, SIGNAL: 0x%x\n",
                    GET_CSTATE, b_state, rxen_cnt, GET_B_LENGTH_FIELD, GET_SIGNAL_FIELD);

            printf("b_ma: 0x%x, ed: 0x%x, gn_ma: 0x%x\n", GET_RO_CCA_PWR_MA_11B, GET_RO_ED_STATE, GET_RO_CCA_PWR_MA_11GN);
            printf("rf_ff1: 0x%x, pga_ff1: 0x%x\n", GET_RO_RF_PWR_FF1, GET_RO_PGA_PWR_FF1);
            printf("rf_ff2: 0x%x, pga_ff2: 0x%x\n", GET_RO_RF_PWR_FF2, GET_RO_PGA_PWR_FF2);
            printf("rf_ff3: 0x%x, pga_ff3: 0x%x\n", GET_RO_RF_PWR_FF3, GET_RO_PGA_PWR_FF3); 
            printf("******************\n");
            printf("* RX EN CNT SAME *\n");
            printf("******************\n");
#endif //DEBUG_RXEN_CNT

            printf("RX EN CNT IS THE SAME\n");
            reset_phy();
            ret = 1;
        }
        xLastTickCnt = xCurrentTick;
        last_rxen_cnt = rxen_cnt;
        if (ret)
            goto end;
    }
    //S_PSDU, decode state
    if (b_state == 0x8) {
        //0xa = 1Mbps, 0x14 = 2Mbps, 0x37 = 5.5Mbps, 0x6e = 11Mbps
        //max len = (30+2312+4)*8 = 18768 = 0x4950
        if (b_len == 0 || b_len > 0x4950 || (b_signal != 0x0a && b_signal != 0x14 && b_signal != 0x37 && b_signal != 0x6e)) {
            printf("b_state: 0x%x, b_len: 0x%x, SFD: 0x%x, SIGNAL: 0x%x, SRV: 0x%x\n", 
                b_state, GET_B_LENGTH_FIELD, GET_SFD_FIELD, GET_SIGNAL_FIELD, GET_B_SERVICE_FIELD);
            reset_phy();
        }
    }
end:
    gOsFromISR --;    
}

void irq_dispatch( u32 irq_status )
{
    struct irq_entry *irq_en;
    const struct irq_entry *max_en;

    /* ToDo: we may add IRQ Priority at this function */

	ASSERT(gOsFromISR == false);
    gOsFromISR = true;
    for (irq_en = main_irq_tbl, max_en = irq_en + MAX_MAIN_IRQ_ENTRY;
         irq_status && (irq_en < max_en);
         irq_en++ ) {
    	u32 irq_bit_mask = (1 << irq_en->irq_no);
        if ((irq_status & irq_bit_mask) && (irq_en->irq_handle)) {
            DEBUG_UPDATE_IRQ_COUNT(irq_en);
            irq_en->irq_handle( irq_en->irq_data );
        }
        irq_status &= ~irq_bit_mask;
    }
    gOsFromISR --;
}


#if (CONFIG_SIM_PLATFORM == 0)
volatile u32 uPreemptiveTickTrigger=0;

#ifdef ENABLE_MEAS_IRQ_TIME
u32 g_int_max_latency;
u32 g_int_min_latency;
u32 g_int_total_latency;
u32 g_int_cnt;
u32 g_enable;
u32 g_temp;
#define ISR_CAL_TIME 500000
#endif




/* NOTE: DO NOT use local variable in this function !! */
void do_irq( void ) __attribute__ ((interrupt ("IRQ"), naked));
void do_irq( void )
{
    register u32 irq_status, irq_status_all;
    asm volatile("STMDB             SP!, {R0-R12, R14}");

#ifdef ENABLE_MEAS_IRQ_TIME
    dbg_get_time(&irq_time);
#endif


#ifdef ENABLE_MEAS_IRQ_TIME
    g_temp = REG32(ADR_MTX_BCN_TSF_L);
#endif


    // Wake up from clock gated. Disable MCU clock gating before 
    // handling interrupt.
    if (GET_MCU_CLK_EN == 0)
    { 
        SET_MCU_CLK_EN(1);
    }

	irq_status_all = irq_status = REG_INTR_CTRL->INT_IRQ_STS;

    // If IRQ is triggered by host-issued SDIO interrupt, just clear it.
    //if(GET_IRQ_SDIO == 1)
    //{
        //SET_IRQ_SDIO(0);
    //}

	// Check if OS tick timer happened.
	if (irq_status & (1<<IRQ_MS_TIMER0))
	{
        adjust_scrambler_seed(); //WSD-188
        rx_packet_check();
        uPreemptiveTickTrigger = 1;

        irq_status &= ~(1<<IRQ_MS_TIMER0);
	}

	// Handle other interrupts if any
	if (irq_status)
		#ifdef STATIC_IRQ_TABLE
		dispatch_irq(irq_status);
		#else
		irq_dispatch(irq_status);
		#endif // STATIC_IRQ_TABLE

    //Do not clear this reg before irq_dispatch
    //INT_PERI_STS will be clear if clear bit31 of INT_IRQ_STS
    // Clear all interrupt status.
   REG_INTR_CTRL->INT_IRQ_STS = irq_status_all;

#ifdef ENABLE_MEAS_IRQ_TIME
    {
    u32 irq_proc_time = dbg_get_elapsed (&irq_time);
    if (irq_max_proc_time < irq_proc_time)
    	irq_max_proc_time = irq_proc_time;
    }
#endif


#ifdef ENABLE_MEAS_IRQ_TIME    
    if(g_enable == 1)
        g_enable = g_temp;

    g_temp = REG32(ADR_MTX_BCN_TSF_L)-g_temp;
           
    if(g_enable != 0){
        
        if(g_int_total_latency != 0){       
            g_int_total_latency += g_temp;

            if(g_int_max_latency<g_temp)
                g_int_max_latency = g_temp;
            
            if(g_int_min_latency>g_temp)
                g_int_min_latency = g_temp;        
        }
        else{        
            g_int_total_latency = g_temp;
            g_int_max_latency = g_temp;
            g_int_min_latency = g_temp;                
        }    
        g_int_cnt++;

        if(REG32(ADR_MTX_BCN_TSF_L)-g_enable > ISR_CAL_TIME)
            g_enable = 0;        
    } 
#endif
    

    asm volatile("LDMIA             SP!, {R0-R12, R14}");
    asm volatile("B                 vPreemptiveTick");
}
#endif


s32 _irq_request(u32 irq, irq_handler irq_handle, void *m_data)
{
    struct irq_entry *irq_en, *empty_en;
    const struct irq_entry *max_en;
    s32 retval=-1;
	u8 peri=0;
   
    irq_en = main_irq_tbl;
    max_en = irq_en + MAX_MAIN_IRQ_ENTRY;

    /* Peripheral IRQ: use peripheral IRQ Handle Table */
    if ( irq >= IRQ_PERI_OFFSET ) {
        irq -= IRQ_PERI_OFFSET;
        irq_en += MAX_MAIN_IRQ_ENTRY;
        max_en += MAX_PERI_IRQ_ENTRY;
		peri = 1;
    }
    
//    OS_INTR_MAY_DISABLE();
    empty_en=(struct irq_entry *)NULL;
    while (irq_en < max_en) {
        if (irq_en->irq_handle) {
            if ( irq == irq_en->irq_no )
                break;
        }
        else {
            if (empty_en == NULL)
                empty_en = irq_en;
        }
        irq_en++;
    }

    /* Entry found: modify or remove the entry */
    if (irq_en < max_en) {
        if ( irq_handle == NULL ) {
            irq_en->irq_no = 0;

			if(peri)
            	REG_INTR_CTRL->INT_PERI_MASK |= (1<<irq);
			else
				REG_INTR_CTRL->INT_MASK |= (1<<irq);
        }
        empty_en = irq_en;
        goto __done;
    }
    
    /* Empty entry found only: Add a new entry */
    if (empty_en != NULL) {
        empty_en->irq_no = irq;

		if(peri)
        	REG_INTR_CTRL->INT_PERI_MASK &= ~(1<<irq);
		else
			REG_INTR_CTRL->INT_MASK &= ~(1<<irq);
		
__done:
        empty_en->irq_handle = irq_handle;
        empty_en->irq_data = m_data;
        retval = 0;
    }
//    OS_INTR_MAY_ENABLE();
    return retval;   

}


s32 irq_init(void)
{
    struct irq_entry *irq;
    u32 size;

    /**
        *  Mask out all IRQs and set to IRQ mode by default.
        *  Note that for INT_MASK, set to 1 disable interrupt
        *  while 0 enable interrupt.
        */
    REG_INTR_CTRL->INT_MASK = 0xFFFFFFFF;
    REG_INTR_CTRL->INT_PERI_MASK = 0xFFFFFFFF;
    REG_INTR_CTRL->INT_MODE = 0;

    /**
        *  Allocate IRQ Table. The Interrupt Controller is 2-level
        *  architecture. We allocate MAX_MAIN_IRQ_ENTRY entries 
        *  for first level while allocating MAX_PERI_IRQ_ENTRY 
        *  entries for peripheral IRQ (the second level).
        */
    size = (MAX_MAIN_IRQ_ENTRY + MAX_PERI_IRQ_ENTRY) *
            sizeof(struct irq_entry);
    main_irq_tbl = (struct irq_entry *)malloc(size);
    memset((void *)main_irq_tbl, 0, size);

    /**
        *  By default, we add peripheral IRQ to the last entry
        *  of IRQ Table (main_irq_tbl). And unmask the
        *  IRQ_PERI_GROUP bit to enable this IRQ.
        */
    irq = &main_irq_tbl[MAX_MAIN_IRQ_ENTRY-1];
    irq->irq_no     = IRQ_PERI_GROUP;
    irq->irq_handle = irq_peri_handler;
    irq->irq_data   = (irq + 1);
    irq->irq_count  = 0;
    REG_INTR_CTRL->INT_MASK &= ~(1U<<IRQ_PERI_GROUP);

#ifdef ENABLE_MEAS_IRQ_TIME
    dbg_timer_init();
    irq_max_proc_time = 0;
#endif
    return 0;
}

s32  irq_check (int int_no)
{
    int peri = 0;
    u32 status;

    // Freddie ToDo: Check if int status is held for int_no.
    if (int_no >= IRQ_PERI_OFFSET) {
        int_no -= IRQ_PERI_OFFSET;
        peri = 1;
    }

    if (peri)
        status = REG_INTR_CTRL->INT_PERI_STS;
    else
        status = REG_INTR_CTRL->INT_IRQ_STS;

    return (status & (1 << int_no));
}

// Static IRQ table

#ifdef STATIC_IRQ_TABLE

extern void irq_mbox_handler(void *m_data);
extern void UART0_RxISR( void *custom_data );
extern void UART0_TxISR( void *custom_data );
extern void drv_rtc_int_handler (void *);
extern void irq_timer_handler (void *);
extern void irq_sdio_handler(void *m_data);

static u32 _irq_status[2];
#define IRQ_BIT_MASK(IRQ_NO)    (1 << ((IRQ_NO >= IRQ_PERI_OFFSET) ? (IRQ_NO - IRQ_PERI_OFFSET) : (IRQ_NO)))
#define IRQ_STATUS_IDX(IRQ_NO)  ((IRQ_NO >= IRQ_PERI_GROUP) ? 1 : 0)
#define PERI_IRQ_BIT_MASK		(1 << IRQ_PERI_OFFSET)

// Static declared IRQ table. All intended IRQ must be filled into the table before enable it.
struct irq_tbl_entry {
    const s32       no;
    s32             data;
    irq_handler     handler;
    u32             count;
    const u32       bit_mask;
    const u32       status_idx;
};


#define IRQ_TBL_ENTRY_INIT(IRQ_NO, IRQ_HANDLER) \
		{IRQ_NO, \
		 IRQ_NO, \
		 IRQ_HANDLER, \
		 0, \
		 IRQ_BIT_MASK(IRQ_NO), \
		 IRQ_STATUS_IDX(IRQ_NO) \
		}


static struct irq_tbl_entry _irq_tbl[] = {
		IRQ_TBL_ENTRY_INIT(IRQ_MBOX, irq_mbox_handler),
		IRQ_TBL_ENTRY_INIT(IRQ_US_TIMER0, irq_timer_handler),
		//IRQ_TBL_ENTRY_INIT(IRQ_US_TIMER1, irq_timer_handler),
		//IRQ_TBL_ENTRY_INIT(IRQ_US_TIMER2, irq_timer_handler),
		//IRQ_TBL_ENTRY_INIT(IRQ_US_TIMER3, irq_timer_handler),
		//IRQ_TBL_ENTRY_INIT(IRQ_MS_TIMER1, irq_timer_handler),
		//IRQ_TBL_ENTRY_INIT(IRQ_MS_TIMER2, irq_timer_handler),
		//IRQ_TBL_ENTRY_INIT(IRQ_MS_TIMER1, irq_timer_handler),
		//IRQ_TBL_ENTRY_INIT(IRQ_RTC, drv_rtc_int_handler),
		IRQ_TBL_ENTRY_INIT(IRQ_UART0_TX, UART0_TxISR),
		IRQ_TBL_ENTRY_INIT(IRQ_UART0_RX, UART0_RxISR),
        IRQ_TBL_ENTRY_INIT(IRQ_SDIO_IPC, irq_sdio_handler)
};

#define IRQ_TBL_SIZE  (sizeof(_irq_tbl)/sizeof(_irq_tbl[0]))

void dispatch_irq (u32 irq_status)
{
    u32 irq_idx;

	ASSERT(gOsFromISR == false);

	// Update peripheral interrupt status
	_irq_status[0] = irq_status;
	if (irq_status & PERI_IRQ_BIT_MASK)
    {
    	// Clear peripheral IRQ before handling it.
		_irq_status[1] = REG_INTR_CTRL->INT_PERI_STS;
        REG_INTR_CTRL->INT_PERI_STS = _irq_status[1];
		//drv_uart_tx_0('P');
    }
    else
    	_irq_status[1] = 0;

	// Process interrupt by priority order as in _irq_tbl.
	++gOsFromISR;
    for (irq_idx = 0; irq_idx < IRQ_TBL_SIZE; irq_idx++)
    {
    	struct irq_tbl_entry *irq_entry = &_irq_tbl[irq_idx];
    	// Check if the respective status bit of this IRQ handler is raised.
		//drv_uart_tx_0('a'+irq_idx);
    	if ((_irq_status[irq_entry->status_idx] & irq_entry->bit_mask) == 0)
    		continue;
		//drv_uart_tx_0('A'+irq_idx);

    	// Handle it if handler is ready.
    	if (irq_entry->handler != NULL)
    	{
    		irq_entry->handler((void *)irq_entry->data);
    		irq_entry->count++;
    	}
    	// Clear the corresponding status bit. Quit if no more interrupt status.
    	_irq_status[irq_entry->status_idx] &= ~irq_entry->bit_mask;
        if ((_irq_status[0] == 0) && (_irq_status[1] == 0))
        	break;
    }
    --gOsFromISR;
    // Check _irq_status[0] and _irq_status[1] to ensure no unhandled interrupt?
} // end of - dispatch_irq -


static struct irq_tbl_entry * _find_irq_entry (u32 irq_no)
{
	u32 irq_idx;
	struct irq_tbl_entry *irq_entry = NULL;

	// Find corresponding interrupt entry in _irq_tbl.
    for (irq_idx = 0; irq_idx < IRQ_TBL_SIZE; irq_idx++)
    {
		if (_irq_tbl[irq_idx].no != irq_no)
			continue;

		irq_entry = &_irq_tbl[irq_idx];
		break;
	}

	return irq_entry;
} // end of - _find_irq_entry -


s32 enable_irq (u32 irq_no)
{
	struct irq_tbl_entry *irq_entry = _find_irq_entry(irq_no);

	// Not found. Exit.
	if (irq_entry == NULL) 
	{
		LOG_PRINTF("%s: No IRQ entry for %d is found.\n", __FUNCTION__, irq_no);
		return (-1);
	}

    if (irq_no >= IRQ_PERI_OFFSET ) 
	{
        irq_no -= IRQ_PERI_OFFSET;
    	//OS_INTR_MAY_DISABLE();
        REG_INTR_CTRL->INT_PERI_MASK &= ~(1 << irq_no);
		// REG_INTR_CTRL->INT_MASK &= ~(1U<<IRQ_PERI_GROUP);
       // OS_INTR_MAY_ENABLE();
    } 
	else 
	{
    	//OS_INTR_MAY_DISABLE();
		REG_INTR_CTRL->INT_MASK &= ~(1 << irq_no);
        //OS_INTR_MAY_ENABLE();
    }
    return 0;
}


s32 disable_irq (u32 irq_no)
{
	struct irq_tbl_entry *irq_entry = _find_irq_entry(irq_no);

	// Not found. Exit.
	if (irq_entry == NULL) 
	{
		LOG_PRINTF("%s: No IRQ entry for %d is found.\n", __FUNCTION__, irq_no);
		return (-1);
	}

    if (irq_no >= IRQ_PERI_OFFSET) {
        irq_no -= IRQ_PERI_OFFSET;
    	//OS_INTR_MAY_DISABLE();
        REG_INTR_CTRL->INT_PERI_MASK |= (1 << irq_no);
		if (REG_INTR_CTRL->INT_PERI_MASK == 0xFFFFFFFF)
			REG_INTR_CTRL->INT_MASK |= (1U<<IRQ_PERI_GROUP);
        //OS_INTR_MAY_ENABLE();
    } else {
    	//OS_INTR_MAY_DISABLE();
		REG_INTR_CTRL->INT_MASK |= (1 << irq_no);
        //OS_INTR_MAY_ENABLE();
    }
    return 0;
}


s32 request_irq (u32 irq_no, irq_handler new_irq_handler, void *user_data)
{
	struct irq_tbl_entry *irq_entry = _find_irq_entry(irq_no);

	// Not found. Exit.
	if (irq_entry == NULL) 
	{
		LOG_PRINTF("%s: No IRQ entry for %d is found.\n", __FUNCTION__, irq_no);
		return (-1);
	}

    //OS_INTR_MAY_DISABLE();
	irq_entry->handler = new_irq_handler;
	irq_entry->data = (s32)user_data;
    //OS_INTR_MAY_ENABLE();

	if (new_irq_handler != NULL)
		enable_irq(irq_no);
	else
		disable_irq(irq_no);
	return 0;
}

void dump_irq_tbl(void)
{
	u32 irq_idx;
	// Find corresponding interrupt entry in _irq_tbl.
    for (irq_idx = 0; irq_idx < IRQ_TBL_SIZE; irq_idx++)
    {
		printf("%d: %08X - %08X - %d\n", 
		       _irq_tbl[irq_idx].no,
			   _irq_tbl[irq_idx].handler,
			   _irq_tbl[irq_idx].bit_mask,
			   _irq_tbl[irq_idx].status_idx);
	}
	printf("%08X - %08X \n", UART0_TxISR, UART0_RxISR);
}

#endif // STATIC_IRQ_TABLE

