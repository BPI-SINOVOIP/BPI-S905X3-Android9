#ifndef _IRQ_H_
#define _IRQ_H_


typedef void (*irq_handler)(void *);



/**
 *  Definition of first-level Interrupt Request Number
 */
#define IRQ_SDIO                            2
#define IRQ_BEACON_DONE						3
#define IRQ_BEACON							4
#define IRQ_PREBEACON						5

#define IRQ_EDCA_BK							6
#define IRQ_EDCA_BE							7
#define IRQ_EDCA_VI							8
#define IRQ_EDCA_VO							9
#define IRQ_EDCA_MNT						10
#define IRQ_EDCA_BCN						11
#define IRQ_EDCA_PREDTIM					12



//#define IRQ_MAILBOX_TRAP					19
#define IRQ_MAC_RX                          19
#define IRQ_MBOX                            19

#define IRQ_US_TIMER0                       20
#define IRQ_US_TIMER1                       21
#define IRQ_US_TIMER2                       22
#define IRQ_US_TIMER3                       23

#define IRQ_MS_TIMER0                       24
#define IRQ_MS_TIMER1                       25
#define IRQ_MS_TIMER2                       26
#define IRQ_MS_TIMER3                       27

#define IRQ_DMA0                            29
#define IRQ_PERI_GROUP                      31






#define IRQ_RTC				                31
#define IRQ_UART0_TX                        32
#define IRQ_UART0_RX                        33

#define IRQ_SDIO_IPC                        34
#define IRQ_SPI_IPC                        	35

#define IRQ_GPI0                        	36
#define IRQ_GPI1                        	37
#define IRQ_SCRT1                        	38
#define IRQ_MMU_ALC_ERR                    	39		//request an id has already been used
#define IRQ_MMU_RLS_ERR                    	40

#define IRQ_ID_MNG_ALLOCATE_TIMEOUT         41
#define IRQ_MBOX_RELSASE_TIMEOUT            42
#define IRQ_MBOX_UNDER_THRESHOLD            43
#define IRQ_MBOX_OVER_THRESHOLD           	44

#define IRQ_HCI           					45
#define IRQ_UART_RX_TIMEOUT					46
#define IRQ_UART_MLTI						47
#define IRQ_ID_MNG_REQ_LOCK					48
#define IRQ_ID_MNG_DMN_NOHIT				49		//R/W invalid address

#define IRQ_ID_THOLD_RX_INT					50
#define IRQ_ID_THOLD_TX_INT					51
#define IRQ_ID_DOUBLE_RLS_INT				52	


#define IRQ_DMN_MCU_INT						56

#define IRQ_UART1_TX                   		57
#define IRQ_UART1_RX                   		58
#define IRQ_UART1_RX_TIMEOUT          		59
#define IRQ_UART1_MLTI	                	60



#define IRQ_PERI_OFFSET                     31

// Use static declared IRQ table. Must be correctly initialized.
//#define STATIC_IRQ_TABLE

void irq_disable(void);
void irq_enable(void);
s32  _irq_request(u32 irq,irq_handler irq_handle,void * m_data);
s32  irq_init(void);
s32  irq_check (int int_no);

#ifdef STATIC_IRQ_TABLE
void dispatch_irq (u32 irq_status);
s32 request_irq (u32 irq_no, irq_handler new_irq_handler, void *m_data);
s32 enable_irq (u32 irq);
s32 disable_irq(u32 irq);
#define irq_request	request_irq
#else
#define irq_request _irq_request
#endif // STATIC_IRQ_TABLE

#ifdef ENABLE_MEAS_IRQ_TIME
extern Time_T    irq_time;
extern u32       irq_max_proc_time;
#endif

#endif /* _IRQ_H_ */

