#ifndef _DRV_UART_H_
#define _DRV_UART_H_

#include <types.h>

typedef enum __SSV6XXX_UART_ID_E {
    SSV6XXX_UART0,
    SSV6XXX_UART1
} SSV6XXX_UART_ID_E;
    
s32 drv_uart_rx_ready(SSV6XXX_UART_ID_E	uart_id);
s32 drv_uart_rx(SSV6XXX_UART_ID_E uart_id);
s32 drv_uart_tx(SSV6XXX_UART_ID_E uart_id, s32 ch);
s32 drv_uart_init(SSV6XXX_UART_ID_E uart_id);

s32 drv_uart_rx_0(void);
s32 drv_uart_tx_0(s8 ch);
s32 drv_uart_is_tx_busy_0(void);
s32 drv_uart_multiple_tx_0(size_t num, const s8 *pch);
s32 drv_uart_get_uart_fifo_length_0(void);
void drv_uart_enable_tx_int_0(void);
void drv_uart_disable_tx_int_0(void);
void drv_uart_enable_rx_int_0(void);
void drv_uart_disable_rx_int_0(void);

#endif /* _DRV_UART_H_ */

