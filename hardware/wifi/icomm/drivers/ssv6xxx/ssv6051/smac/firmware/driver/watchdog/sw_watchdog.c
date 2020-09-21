#include <regs.h>
#include <pbuf.h>
#include <mbox/drv_mbox.h>

static OsTimer watchdog_timer;

void sw_watchdog_handler()
{
    u32 notification_data;
    HDR_HostEvent *host_evt;
    u32 evt_size = sizeof(HDR_HostEvent);

    do{
        notification_data = (u32)PBUF_MAlloc_Raw(evt_size, 0, RX_BUF);
    }while(notification_data == 0);

    host_evt = (HDR_HostEvent *)notification_data;
    host_evt->c_type = HOST_EVENT;
    host_evt->h_event = SOC_EVT_WATCHDOG_TRIGGER;
    host_evt->len = evt_size;

    // Send out to host
    TX_FRAME(notification_data);
    return;
}

static u32 timeInterval = 5000;
s32 sw_watchdog_init()
{
	//Init 2500ms timer 
	if( OS_TimerCreate(&watchdog_timer, timeInterval, (u8)TRUE, NULL, (OsTimerHandler)sw_watchdog_handler) == OS_FAILED)
		return OS_FAILED;

    OS_TimerStop(watchdog_timer);
	return OS_SUCCESS;
}

void sw_watchdog_start()
{
	OS_TimerStart(watchdog_timer);
}

void sw_watchdog_stop()
{
	OS_TimerStop(watchdog_timer);
}


