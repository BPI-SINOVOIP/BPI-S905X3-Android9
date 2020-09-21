#ifndef _RTOS_H_
#define _RTOS_H_

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <timers.h>



typedef pdTASK_CODE				    OsTask;
typedef xTaskHandle				    OsTaskHandle;
typedef xSemaphoreHandle			OsMutex;
typedef xSemaphoreHandle			OsSemaphore;
typedef xQueueHandle				OsMsgQ;
typedef xTimerHandle                OsTimer;
typedef tmrTIMER_CALLBACK           OsTimerHandler;




/* Define Task Priority: 0 is the lowest priority */
#define OS_TASK_PRIO0			    0
#define OS_TASK_PRIO1			    1
#define OS_TASK_PRIO2			    2
#define OS_TASK_PRIO3			    3




/* Define OS error values */
#define OS_SUCCESS                  0
#define OS_FAILED                   1

#define OS_MS2TICK(_ms)             ( (_ms)/portTICK_RATE_MS )


/* Message Commands: */
#define OS_MSG_FRAME_TRAPPED        1



typedef struct OsMsgQEntry_st
{
	u32         MsgCmd;
    void        *MsgData;
    
} OsMsgQEntry, *POsMsgQEntry;


typedef struct task_info_st 
{
    const s8   *task_name;
    OsMsgQ      qevt;
    u8          qlength;
    u8          prio;
    u8          stack_size;  /* unit: 16 */
    void        *args;
    TASK_FUNC   task_func; 
    
} task_info;


#define OS_INTR_DISABLE()         \
    asm volatile (                \
    "STMDB  SP!, {R0}       \n\t" \
    "MRS    R0, CPSR        \n\t" \
    "ORR    R0, R0, #0xC0   \n\t" \
    "MSR    CPSR, R0        \n\t" \
    "LDMIA  SP!, {R0}           " \
    )

#define OS_INTR_ENABLE()          \
    asm volatile (                \
    "STMDB  SP!, {R0}       \n\t" \
    "MRS    R0, CPSR        \n\t" \
    "BIC    R0, R0, #0xC0   \n\t" \
    "MSR    CPSR, R0        \n\t" \
    "LDMIA  SP!, {R0}           " \
    )

#define OS_INTR_MAY_DISABLE()     \
    if (gOsFromISR == false)      \
        OS_INTR_DISABLE()

#define OS_INTR_MAY_ENABLE()      \
    if (gOsFromISR == false)      \
        OS_INTR_ENABLE()          \


/**
 *  Flag to indicate whether ISR handler is running or not.
 */
extern volatile u8 gOsFromISR;




OS_APIs s32  OS_Init( void );

/* Task: */
OS_APIs s32  OS_TaskCreate( OsTask task, const s8 *name, u16 stackSize, void *param, u32 pri, OsTaskHandle *taskHandle );
OS_APIs void OS_TaskDelete( OsTaskHandle taskHandle );
OS_APIs void OS_StartScheduler( void );
OS_APIs void OS_Terminate( void );



/* Mutex: */
OS_APIs s32  OS_MutexInit( OsMutex *mutex );
OS_APIs void OS_MutexLock( OsMutex mutex );
OS_APIs void OS_MutexUnLock( OsMutex mutex );



/* Delay: */
OS_APIs void OS_MsDelay(u32 ms);


/* Timer: */
OS_APIs s32 OS_TimerCreate( OsTimer *timer, u32 ms, u8 autoReload, void *args, OsTimerHandler timHandler );
OS_APIs s32 OS_TimerSet( OsTimer timer, u32 ms, u8 autoReload, void *args );
OS_APIs s32 OS_TimerStart( OsTimer timer );
OS_APIs s32 OS_TimerStop( OsTimer timer );
OS_APIs void *OS_TimerGetData( OsTimer timer );

//OS_APIs void OS_TimerGetSetting( OsTimer timer, u8 *autoReload, void **args );
//OS_APIs bool OS_TimerIsRunning( OsTimer timer );



/* Message Queue: */
OS_APIs s32 OS_MsgQCreate( OsMsgQ *MsgQ, u32 QLen );
OS_APIs s32 OS_MsgQEnqueue( OsMsgQ MsgQ, OsMsgQEntry *MsgItem, bool fromISR );
OS_APIs s32 OS_MsgQDequeue( OsMsgQ MsgQ, OsMsgQEntry *MsgItem, bool block, bool fromISR );
OS_APIs s32 OS_MsgQWaitingSize( OsMsgQ MsgQ );
#if 0
OS_APIs void *OS_MsgAlloc( void );
OS_APIs void OS_MsgFree( void *Msg );
#endif
OS_APIs s32 OS_MsgQEnqueueFront( OsMsgQ MsgQ, OsMsgQEntry *MsgItem, bool fromISR );



/* Memory: */
OS_APIs void *OS_MemAlloc( u32 size );
OS_APIs void OS_MemFree( void *m );






#endif /* _RTOS_H_ */

