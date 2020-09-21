#ifndef _DRV_RTC_H_
#define _DRV_RTC_H_

#include <regs.h>
#include <bsp/irq.h>

#if __cplusplus
extern "C"
{
#endif

typedef enum
{
	RTC_DISABLE = 0x0,
	RTC_ENABLE = 0x1,
} RTC_EN;

typedef enum
{
	RTC_SOURCE_PRESCALER = 0x0,
	RTC_SOURCE_OSCILLATOR = 0x1,
} RTC_SOURCE;

typedef enum
{
	RTC_INT_UN_MASKED = 0x0,
	RTC_INT_MASKED = 0x1,
} RTC_INT_MASK;

typedef enum
{
	RTC_NO_INT_PEND = 0x0,
	RTC_INT_PEND = 0x1,
} RTC_INT_STATUS;

//------------------------------------------------------------------------------
//
//  Function:   drv_rtc_init
//
//  Description
//      This function is called by bsp init to let the real-time clock init
//
//  Parameters
//
//  Return Value
//
void drv_rtc_init(void);

//------------------------------------------------------------------------------
//
//  Function:   drv_rtc_en
//
//  Description
//      make rtc enable or disable
//
//  Parameters
//
//  Return Value
//
void drv_rtc_en(RTC_EN en);

//------------------------------------------------------------------------------
//
//  Function:   drv_rtc_set_source
//
//  Description
//      set rtc source
//
//  Parameters
//      source: [in]
//			PRESCALER = 0x0,
//			OSCILLATOR = 0x1
//  Return Value
//
void drv_rtc_set_source(RTC_SOURCE source);

//------------------------------------------------------------------------------
//
//  Function:   drv_rtc_get_source
//
//  Description
//      get rtc source
//
//  Parameters
//
//  Return Value
//		RTC_SOURCE
//
RTC_SOURCE drv_rtc_get_source(void);

//void drv_rtc_setrealtime(u32 time);

//------------------------------------------------------------------------------
//
//  Function:   drv_rtc_getrealtime
//
//  Description
//      Reads the current RTC value and returns a system time.
//
//  Parameters
//      pTime: [out] Pointer to the u32 that contains the
//                   current time [sec].
//
//  Return Value
//      0 indicates success.
//      -1 indicates failure.
//
s32 drv_rtc_getrealtime(u32 *pTime);

//------------------------------------------------------------------------------
//
//  Function:   drv_rtc_setalarmtime
//
//  Description
//     set the real-time clock alarm.
//
//  Parameters
//      alarmtime: [in] alarm time [sec].
//
//  Return Value
//      0 indicates success.
//      -1 indicates failure.
//
s32 drv_rtc_setalarmtime(u32 alarmtime);

//------------------------------------------------------------------------------
//
//  Function:   drv_rtc_getalarmtime
//
//  Description
//      Reads the current RTC value and returns a alarm time.
//
//  Parameters
//      pTime: [out] Pointer to the u32 that contains the
//                   alarm time [sec].
//
//  Return Value
//      0 indicates success.
//      -1 indicates failure.
//
s32 drv_rtc_getalarmtime(u32 *pAlarmTime);

//------------------------------------------------------------------------------
//
//  Function:   drv_rtc_cali
//
//  Description
//      trigger RTC calibration
//
//  Parameters
//
//  Return Value
//      0 indicates success.
//      -1 indicates failure.
//
s32 drv_rtc_cali(void);

//------------------------------------------------------------------------------
//
//  Function:   drv_rtc_set_sec_interrupt_handle
//
//  Description
//     set the real-time clock per-sec interrupt callback handle and private data.
//
//  Parameters
//      sec_handle: [in] per-sec interrupt callback handle.
//      m_data: [in] private data.
//
//  Return Value
//      0 indicates success.
//      -1 indicates failure.
//
s32 drv_rtc_set_sec_interrupt_handle(irq_handler sec_handle, void *m_data);

//------------------------------------------------------------------------------
//
//  Function:   drv_rtc_get_sec_interrupt_status
//
//  Description
//     Reads the current RTC sec interrupt status
//
//  Parameters
//
//  Return Value
//      NO_INT_PEND : no interrupt pending
//      INT_PEND : has interrupt pending
//
RTC_INT_STATUS drv_rtc_get_sec_interrupt_status(void);

//------------------------------------------------------------------------------
//
//  Function:   drv_rtc_set_sec_interrupt_mask
//
//  Description
//     mask the RTC sec interrupt
//
//  Parameters
//
//  Return Value
//
void drv_rtc_set_sec_interrupt_mask(void);

//------------------------------------------------------------------------------
//
//  Function:   drv_rtc_clear_sec_interrupt_mask
//
//  Description
//     unmask the RTC sec interrupt
//
//  Parameters
//
//  Return Value
//
void drv_rtc_clear_sec_interrupt_mask(void);

//------------------------------------------------------------------------------
//
//  Function:   drv_rtc_clear_sec_interrupt_pending
//
//  Description
//     clear the RTC sec interrupt pending
//
//  Parameters
//
//  Return Value
//
void drv_rtc_clear_sec_interrupt_pending(void);

//------------------------------------------------------------------------------
//
//  Function:   drv_rtc_set_alarm_interrupt_handle
//
//  Description
//     set the real-time clock alarm interrupt callback handle and private data.
//
//  Parameters
//      sec_handle: [in] alarm interrupt callback handle.
//      m_data: [in] private data.
//
//  Return Value
//      0 indicates success.
//      -1 indicates failure.
//
s32 drv_rtc_set_alarm_interrupt_handle(irq_handler alarm_handle, void *m_data);

//------------------------------------------------------------------------------
//
//  Function:   drv_rtc_get_alarm_interrupt_status
//
//  Description
//     Reads the current RTC alarm interrupt status
//
//  Parameters
//
//  Return Value
//      NO_INT_PEND : no interrupt pending
//      INT_PEND : has interrupt pending
//
RTC_INT_STATUS drv_rtc_get_alarm_interrupt_status(void);

//------------------------------------------------------------------------------
//
//  Function:   drv_rtc_set_alarm_interrupt_mask
//
//  Description
//     mask the RTC alarm interrupt
//
//  Parameters
//
//  Return Value
//
void drv_rtc_set_alarm_interrupt_mask(void);

//------------------------------------------------------------------------------
//
//  Function:   drv_rtc_clear_alarm_interrupt_mask
//
//  Description
//     unmask the RTC alarm interrupt
//
//  Parameters
//
//  Return Value
//
void drv_rtc_clear_alarm_interrupt_mask(void);

//------------------------------------------------------------------------------
//
//  Function:   drv_rtc_clear_alarm_interrupt_pending
//
//  Description
//     clear the RTC alarm interrupt pending
//
//  Parameters
//
//  Return Value
//
void drv_rtc_clear_alarm_interrupt_pending(void);

//------------------------------------------------------------------------------
//
//  Function:   drv_rtc_get_ramaddress
//
//  Description
//     Reads the RTC ram start address
//
//  Parameters
//
//  Return Value
//		ram start address
//
u32* drv_rtc_get_ramaddress(void);

//------------------------------------------------------------------------------
//
//  Function:   drv_rtc_get_ramsize
//
//  Description
//     Reads the RTC ram size
//
//  Parameters
//
//  Return Value
//		ram size ( bytes )
//
u32 drv_rtc_get_ramsize(void);

#if __cplusplus
}
#endif
#endif /* _DRV_RTC_H_ */

