#ifndef _SOC_CONFIG_H_
#define _SOC_CONFIG_H_

#include <types.h>
#include <rtos.h>
#include <errno.h>

#define CONFIG_VENDOR_STR               "South Silicon Valley"
#define CONFIG_VENDOR_ID
#define CONFIG_PRODUCT_STR              "SSV6200"
#define CONFIG_PRODUCT_ID
#define CONFIG_PRODUCT_VERSION
#define CONFIG_FW_NAME_STR
#define CONFIG_FW_VERSION
//#define CONFIG_P2P_NOA
//#define CONFIG_RX_MGMT_CHECK

#define TARGET                          prj1_config

#define IRQ_STACK_SIZE                  256 // IRQ stack size
#define OS_MUTEX_SIZE                   10  // Mutex
#define OS_COND_SIZE                    5   // Conditional variable
#define OS_TIMER_SIZE                   5   // OS Timer
#define OS_MSGQ_SIZE                    5   // Message Queue
#define UART_BAUD_RATE                  115200 // UART Baud Rate
#define CLI_ENABLE                      1   // Enable/Disable CLI
#define CLI_BUFFER_SIZE                 80  // CLI Buffer size
#define CLI_ARG_SIZE                    6   // The number of arguments
#define CLI_PROMPT                      "wifi> "
#define IRQ_STACK_SIZE                  256 // IRQ Stack size
#define TCPIP_ENABLE                    0
#define AP_MODE_ENABLE                  0
#define ENABLE_DEBUG_TIMER              1   // Debug timer for profiling. (Using MTX timer)
#define ENABLE_BACKGROUND_PMK_CALC      1   // Let WPA2/PSK PMK calculation run at background.
#define ENABLE_PSK_CACHE                1   // Cache PSK.
#define CONSOLE_SERIAL_PORT             serCOM1
#define ENABLE_MEAS_IRQ_TIME            1   // Measure IRQ process time. Require ENABLE_DEBUG_TIMER.
#define ENEABLE_MEAS_AMPDU_TX_TIME      1   // Measure elapsed time between AMPDU TX and its BA or time out.

#define CONFIG_LOG_ENABLE               1
#define DEFRAG_USE_OS_TICK              1

#define PACKED                              __attribute__((packed))

#define HZ                                   1000             // 100Hz => 100 ticks per second
                                                                                
#define MAX_MAIN_IRQ_ENTRY                   10
#define MAX_PERI_IRQ_ENTRY                   10

/* Message Queue Length: */
#define MBOX_MAX_MSG_EVENT                   48              // Max message event in the system

#define WLAN_MAX_STA                         2

#define CONFIG_HW_MODEL                 1
#define CONFIG_CHECKSUM_DCHECK          0

#define _EDCA_HW_CALCULATE_NAV_

#define __FPGA_PAGE_MODE__
//#define __TEST_CODE__    //notify ASIC can start load vcfg for simulae
//#define __REG_PRINT__    //direct printf to register to speed up for simulate

#define CLI_HISTORY_ENABLE			0            // Enable/Disable CLI history log. 
#define CLI_HISTORY_NUM				5						


//Debug option
//----------------------------------------

//Just for debug
//******************************
//#define ZDebug                          1
//#define CMD_ENGINE_DEBUG
//#define BOOT_MALLOC_DEBUG

#define FPGA_DEBUG                       1    
#define MAILBOX_DBG                      1

//******************************

//----------------------------------------
#define USE_SERIAL_DRV                   1        // Use interrupt-driven serial driver instead of direct UART register access

#include <lib/ssv_lib.h>

#endif /* _SOC_CONFIG_H_ */


