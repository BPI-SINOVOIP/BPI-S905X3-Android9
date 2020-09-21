#include <config.h>
#include <msgevt.h>
#include <pbuf.h>
#include <bsp/bsp.h>
#include <bsp/serial.h>
#include <ssv_firmware_version.h>
#include <apps/mac80211/soft-main.h>
#include <apps/mac80211/cmd_engine.h>
#include <cmd_def.h>
#include <log.h>
#include <lib/ssv_lib.h>
#include <regs.h>
#include <stdio.h>
#include <stdarg.h>

#if (CONFIG_SIM_PLATFORM==0 && CLI_ENABLE == 1)       
#include <cli/cli.h>
#endif

/**
 *  Task Information Table:
 *
 *  Each Task registered here is created in system initial time. After a task
 *  is created, a Message Queue for the task is also created. Once the 
 *  task starts, the task is blocked and waitting for message from the 
 *  Message queue.
 */
struct task_info_st g_soc_task_info[] = {
    { "Soft-Mac",       (OsMsgQ)0, 40, OS_TASK_PRIO2, 128, NULL, soft_mac_task      },
    { "CmdEngine",      (OsMsgQ)0, 4, OS_TASK_PRIO2, 128, NULL, CmdEngine_Task		},
#if (CONFIG_SIM_PLATFORM==0 && CLI_ENABLE == 1)       
    { "cli_task",       (OsMsgQ)0, 0, OS_TASK_PRIO3, 128, NULL, Cli_Task   		},
#endif

};

#if(MAILBOX_DBG == 1)

#define DEBUG_SIZE 2048
char debug_buffer[DEBUG_SIZE];

void dump_mailbox_dbg (int num)
{
	int i;
	int total_num = (num > 0) ? (num * 4) : sizeof(debug_buffer);
	u32 a = *(volatile u32 *)(0xcd01003c);
	//u32 b = *(volatile u32 *)(0xc10000a8);
	//su32 c = *(volatile u32 *)(0xc10000b0);

    LOG_PRINTF("0xCD01003C: %08X\n", a);
    LOG_PRINTF("0xC10000A0: %08X\n", *(volatile u32 *)(0xc10000A0));
    LOG_PRINTF("0xC10000A4: %08X\n", *(volatile u32 *)(0xc10000A4));
    LOG_PRINTF("0xC10000A8: %08X\n", *(volatile u32 *)(0xc10000A8));
    LOG_PRINTF("0xC10000AC: %08X\n", *(volatile u32 *)(0xc10000AC));
    LOG_PRINTF("0xC10000B0: %08X\n", *(volatile u32 *)(0xc10000B0));

	for (i = 0; i < total_num; i += 4)
	{
#if 0
		u32    log_type = (u32)(debug_buffer[0] >> 5);
		u32    time_tag = (((u32)(debug_buffer[0] & 0x1F)) << 7) + (u32)(debug_buffer[1] >> 1);
#endif
		u32    log = *(u32 *)&debug_buffer[i];
		u32    log_type = log >> 29;
		u32    time_tag = (log >> 17) & 0x0FFF;

		LOG_PRINTF("%d: %d - ", time_tag, log_type);

		switch (log_type) {
		case 0: // Normal case
			{
			u32     r_w = (log >> 16) & 0x01;
			u32		port = (log >> 12) & 0x0F;
			u32		HID = (log >> 8) & 0x0F;
			u32		PID = log & 0x0FF;
			LOG_PRINTF("RW: %u, PRT: %u, HID: %u, PID: %u\n", r_w, port, HID, PID);
			}
			break;
			break;
		case 1: // Allocate ID failed
			{
			u32		ID = log & 0x03F;
			LOG_PRINTF("PID: %u\n", ID);
			}
			break;
		case 2: // Release ID failed
			{
			u32		ID = log & 0x03F;
			LOG_PRINTF("PID: %u\n", ID);
			}
			break;
		case 4: // Packet demand request
			{
			u32     port = (log >> 8) & 0x0F;
			u32		ID = log & 0x0FF;
			LOG_PRINTF("PRT: %u, HID: %u\n", port, ID);
			}
			break;
		case 5: // ID allocation request
			{
			u32     page_length = (log >> 7) & 0xFF;
			u32     chl = (log >> 15) & 0x03;
			u32		ID = log & 0x07F;
			LOG_PRINTF("LEN: %u, CHL: %u, ID: %u\n", page_length, chl, ID);
			}
			break;
		case 6: // ID allocation ack
			{
			u32		nack = 0;
			u32     page_length = (log >> 7) & 0xFF;
			u32     chl = (log >> 15) & 0x03;
			u32		ID = log & 0x07F;
			LOG_PRINTF("NCK: %u, LEN: %u, CHL: %u, ID: %u\n", nack, page_length, chl, ID);
			}
                        break;
		case 7: // ID allocation nack
			{
			u32		nack = 1;
			u32     page_length = (log >> 7) & 0xFF;
			u32     chl = (log >> 15) & 0x03;
			u32		ID = log & 0x07F;
			LOG_PRINTF("NCK: %u, LEN: %u, CHL: %u, ID: %u\n", nack, page_length, chl, ID);
			}
			break;
		}
#if 0
		LOG_PRINTF("%02X %02X %02X %02X\n",
				   debug_buffer[i + 0], debug_buffer[i + 1],
				   debug_buffer[i + 2], debug_buffer[i + 3]);
#endif
	}
}

void enable_mailbox_dbg()
{
	printf("MAILBOX debug buffer = %08x\n", (int)debug_buffer);

	SET_MB_DBG_CFG_ADDR((int)debug_buffer);
	SET_MB_DBG_LENGTH(DEBUG_SIZE);
	SET_MB_DBG_EN(1);
	SET_MB_ERR_AUTO_HALT_EN(1);
}

#endif

#if (CONFIG_SIM_PLATFORM == 0)
#if defined(ENABLE_FW_SELF_CHECK)
#include <ssv_regs.h>
void _calc_fw_checksum (void)
{
   
    volatile u32 *FW_RDY_REG = (volatile u32 *)(SPI_REG_BASE + 0x10);
    u32          block_count = (*FW_RDY_REG >> 16) & 0x0FF;
    u32          fw_checksum = FW_CHECKSUM_INIT;
    u32         *addr = 0;
    u32          total_words = block_count * FW_BLOCK_SIZE / sizeof(u32);

    // Don't calculate checksum if host does not provide block count.
    if (block_count == 0)
        return;
        
    while (total_words--) {
        fw_checksum += *addr++;
    }
    fw_checksum = ((fw_checksum >> 24) + (fw_checksum >> 16) + (fw_checksum >> 8) + fw_checksum) & 0x0FF;
    fw_checksum = (fw_checksum << 16) & 0x00FF0000;	
    *FW_RDY_REG = (*FW_RDY_REG & 0xFF00FFFF) | fw_checksum;
    // wait until host confirm the checksum.
    fw_checksum = ~fw_checksum & 0x00FF0000;
	
	do {
        if ((*FW_RDY_REG & 0x00FF0000) == fw_checksum)
            break;
    } while (1);
}
#endif // ENABLE_FW_SELF_CHECK

static void _zero_bss (void)
{
	extern u32   __bss_beg__;
	extern u32   __bss_end__;
	u8          *p_bss = (u8 *)&__bss_beg__;
	u8          *p_bss_end = (u8 *)&__bss_end__;

	while (p_bss != p_bss_end)
		*p_bss++ = 0; 
}
#endif

/**
 *  Entry point of the firmware code. After system booting, this is the
 *  first function to be called from boot code. This function need to 
 *  initialize the chip register, software protoctol, RTOS and create
 *  tasks. Note that, all memory resource needed for each software
 *  module shall be pre-allocated in initialization time.
 *
 */
void APP_Main( void )
{
    volatile u32 *FW_RDY_REG = (volatile u32 *)(SPI_REG_BASE + 0x10);
    register u32 i;

    /* Check integraty of downloaded firmware */
    #if (CONFIG_SIM_PLATFORM == 0)
    #ifdef ENABLE_FW_SELF_CHECK
    _calc_fw_checksum();
    #endif // ENABLE_FW_SELF_CHECK
	/* Fill BSS with 0 */
	_zero_bss();
    #endif
    /* Initialize hardware & device drivers */
    ASSERT(bsp_init() == OS_SUCCESS);

#if (CONFIG_SIM_PLATFORM == 0)    
    /**
        * Initialize RTOS before starting use it. If in simulation/emulation platform, 
        * we shall ignore this initialization because the simulation/emulation has 
        * initialzed.
        */
    ASSERT( OS_Init() == OS_SUCCESS );

#ifdef USE_SERIAL_DRV
	// Initialize uart
	xSerialPortInitMinimal(ser115200, 256);
#endif // USE_SERIAL_DRV

    LOG_init(true, true, LOG_LEVEL_ON, LOG_MODULE_MASK(LOG_MODULE_EMPTY) | LOG_MODULE_MASK(LOG_MODULE_ALL), false);
    LOG_OUT_DST_OPEN(LOG_OUT_SOC_TERM);
    LOG_OUT_DST_CUR_ON(LOG_OUT_SOC_TERM);
#endif
    LOG_PRINTF("OS version: %s\n", tskKERNEL_VERSION_NUMBER);
    #if (CONFIG_SIM_PLATFORM == 0)
    LOG_PRINTF("%s FW built at %s %s\n",
               CONFIG_PRODUCT_STR, __DATE__, __TIME__);
    #endif

	/**
        * Initialize software components. 
        */
    ASSERT( msg_evt_init() == OS_SUCCESS ); 
    ASSERT( PBUF_Init() == OS_SUCCESS );
    ASSERT( soft_mac_init() == OS_SUCCESS );
    ASSERT( cmd_engine_init() == OS_SUCCESS);

    //Write firmware Version(SVN version)
    *FW_RDY_REG = ssv_firmware_version;

	/**
        * Create Tasks and Message Queues. Creating Message Queue or not
        * depends on the xxx.qevt field is zero or not. If zero, no need to
        * create a message queue for the task.
        */
    for(i=0; i<(sizeof(g_soc_task_info)/sizeof(g_soc_task_info[0])); i++) {
        /* Create Registered Task: */
        OS_TaskCreate(g_soc_task_info[i].task_func, 
            g_soc_task_info[i].task_name,
            g_soc_task_info[i].stack_size<<4, 
            g_soc_task_info[i].args, 
            g_soc_task_info[i].prio, 
            NULL); 

        if ((u32)g_soc_task_info[i].qlength> 0) 
		{
            ASSERT(OS_MsgQCreate(&g_soc_task_info[i].qevt, 
            (u32)g_soc_task_info[i].qlength)==OS_SUCCESS);
        }
    }

	LOG_PRINTF("Initialzation done. \nStarting OS scheduler...\n");

    sw_watchdog_init();
    //sw_watchdog_start();

#if (CONFIG_SIM_PLATFORM == 0)

#if(MAILBOX_DBG == 1)
	enable_mailbox_dbg();
#endif//__MAILBOX_DBG__
    SET_DIS_DEMAND(1);      //Disable demand, force system hang when demand happen at anywhere
    // MSDU frame, 0=3839 bytes, 1 =7935 
    // We only support 3839 bytes
    SET_MRX_LEN_FLT(0x1000); //Set lens filter, limit lens to 2000

    /* Start the scheduler so our tasks start executing. */
    OS_StartScheduler();
#endif
}

