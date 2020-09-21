#include <config.h>
#include <regs.h>

#include "drv_uart.h"

typedef struct REG_UART_st {
    SSV6XXX_REG     DATA;
    SSV6XXX_REG     IER;
    SSV6XXX_REG     ISR;
    SSV6XXX_REG     LCR;
    SSV6XXX_REG     MCR;
    SSV6XXX_REG     LSR;
    SSV6XXX_REG     MSR;
    SSV6XXX_REG     SPR;

} REG_UART, *PREG_UART;


#define REG_CPU_UART            ((REG_UART  *)(TO_SIM_ADDR(UART_REG_BASE)))
#define REG_HCI_UART            ((REG_UART  *)(TO_SIM_ADDR(UART_REG_BASE+0x0100)))

#define UART0                   (REG_CPU_UART + 0)
#define UART1                   (REG_HCI_UART + 0)

/* CPU=40MHz, Baudrate=115200 */
/* CPU=10MHz, Baudrate=28800 */
#define UART0_SPR               (0x0015U)
#define UART1_SPR               (0x0005U)

#define SSV6200_UART_ID_MAX	(1)
//#define UART0_SPR               (0x000B)

static REG_UART *uarts[SSV6200_UART_ID_MAX+1] = {
   (REG_UART *)UART_REG_BASE, 
   (REG_UART *)DAT_UART_REG_BASE};

s32 drv_uart_rx_ready(SSV6XXX_UART_ID_E uart_id)
{
    if ((u32)uart_id > (u32)SSV6200_UART_ID_MAX)
        return false;
        
    if ((uarts[uart_id]->LSR&0x01) == 0x01)
        return true;
    else return false;
}


s32 drv_uart_rx(SSV6XXX_UART_ID_E uart_id)
{
    if ((u32)uart_id > (u32)SSV6200_UART_ID_MAX)
        return (-1);
        
    if (uarts[uart_id]->LSR & 0x01)
       return uarts[uart_id]->DATA;
       
    return (-1);
}

s32 drv_uart_tx(SSV6XXX_UART_ID_E uart_id, s32 ch)
{
    if ((u32)uart_id > (u32)SSV6200_UART_ID_MAX)
        return (-1);
        
    while( (uarts[uart_id]->LSR & (0x1U<<5)) == 0U ) {}
    uarts[uart_id]->DATA = (u32)ch;
    return ch;
}


s32 drv_uart_init(SSV6XXX_UART_ID_E uart_id)
{
    if ((u32)uart_id > (u32)SSV6200_UART_ID_MAX)
        return (-1);
        
#if (CONFIG_SIM_PLATFORM == 0)
    if (uart_id == SSV6XXX_UART0)
    {
        uarts[SSV6XXX_UART0]->SPR = UART0_SPR; // set Baudrate
        //uart->ISR = 1;
        //uart->IER |= 0x1 ; // enable Rx data state output ( also enable UART interrupt )
    }
    else
    { /*lint -save -e774 */
        ASSERT_RET(0, -1);
    } /*lint -restore */
#endif
    return 0;
}


s32 drv_uart_rx_0 (void)
{
    if (UART0->LSR & 0x01)
       return UART0->DATA;
       
    return (-1);
} // end of - drv_uart_rx_0 -


s32 drv_uart_tx_0 (s8 ch)
{
    UART0->DATA = (u32)ch;
    return ch;
} // end of - drv_uart_tx_0 -


s32 drv_uart_is_tx_busy_0 (void)
{
    return ((UART0->LSR & (0x1U<<5)) == 0U);
} // end of - drv_uart_is_tx_busy -


s32 drv_uart_multiple_tx_0 (size_t num, const s8 *pch)
{
    while (num-- > 0)
        UART0->DATA = (u32)*pch++;
    return 0;
} // end of - drv_uart_multiple_tx_0 -


s32 drv_uart_get_uart_fifo_length_0(void)
{
    return 16;
} // end of - drv_uart_get_uart_fifo_length_0 -


void drv_uart_enable_tx_int_0 (void)
{
    UART0->IER |= 2;
} // end of - drv_uart_enable_tx_int_0 -


void drv_uart_disable_tx_int_0 (void)
{
    UART0->IER &= ~2;
} // end of - drv_uart_enable_tx_int_0 -


void drv_uart_enable_rx_int_0 (void)
{
    UART0->IER |= 1;
} // end of - drv_uart_enable_rx_int_0 -


void drv_uart_disable_rx_int_0 (void)
{
    UART0->IER &= ~1;
} // end of - drv_uart_disable_rx_int_0 -


