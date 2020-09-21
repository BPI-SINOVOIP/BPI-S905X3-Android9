#include <config.h>
#include <bsp/irq.h>
#include <bsp/bsp.h>
#include <uart/drv_uart.h>
#include <timer/drv_timer.h>
#include <rtc/drv_rtc.h>
#include <hwmac/drv_mac.h>
//#include <sim_regs.h>
#include <log.h>


#if (CONFIG_SIM_PLATFORM == 0)
extern u32 __stack_end__;
extern u32 __bss_end__;
extern u32 __end_of_text__;
u32 g_heap_start;
u32 g_heap_size;
#endif

	

static void sys_heap_init(void)
{

    /**
        *  Note that we shall make sure the start addresss of 
        *  heap space is 4-byte alignment. If not, we shall align
        *  it to 4-byte.
        */
#if (CONFIG_SIM_PLATFORM == 0)        
    g_heap_start = (u32)&__bss_end__;
    if (g_heap_start & 0x03)
        g_heap_start = ((g_heap_start>>2)+1)<<2;
    g_heap_size  = ((u32)&__stack_end__ - g_heap_start) - 256; // ??? RSVD 256 ??
    
    sys_init_prnf("Heap Start: 0x%08x, Heap Size: %u(bytes)\n", g_heap_start, g_heap_size);
#endif

}


static void chip_init(void)
{

}







s32 bsp_init(void)
{
    chip_init();
    drv_uart_init(SSV6XXX_UART0);

#if (CONFIG_SIM_PLATFORM == 0)
    sys_init_prnf("\n\nSystem booting ...\n");
    sys_init_prnf("System Stack: %08x - %08x\n", (u32)&__stack_end__, 192*1024);
    sys_init_prnf("Initial UART-0(CPU Debuging)\n");
#endif

    sys_heap_init();
    irq_init();
    hwtmr_init();
	//DMA init
	//drv_pkt_dma_init();
    //drv_dma_init();

    //drv_rtc_init();    
    drv_mac_init();


#if (CONFIG_SIM_PLATFORM == 0)
    sys_init_prnf("System memory layout:\n");
    sys_init_prnf("\timage: %08x - %08x (%d B)\n", 0, (u32)&__end_of_text__, (u32)&__end_of_text__);
    sys_init_prnf("\theap : %08x - %08x (%d B)\n", g_heap_start, g_heap_start+g_heap_size, g_heap_size);
    sys_init_prnf("IRQ mask: 0x%08x\n", REG_INTR_CTRL->INT_MASK);
    sys_init_prnf("IRQ mode: 0x%08x\n", REG_INTR_CTRL->INT_MODE);
#endif

    return OS_SUCCESS;
}
