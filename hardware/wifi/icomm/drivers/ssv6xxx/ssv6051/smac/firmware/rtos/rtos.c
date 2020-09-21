#include <config.h>
#include <rtos.h>




volatile u8 gOsFromISR;
volatile u8 gHigherPrioTaskReady=pdFALSE;



OS_APIs s32  OS_Init( void )
{
    #ifdef __WIN32__
    extern unsigned long ulCriticalNesting;

    /**
        *  Note!! ulCriticalNesting is a FreeRTOS gobal variable. Before
        *  using Mutex, we shall reset it to 0.
        */
    ulCriticalNesting = 0;
    #endif // __WIN32__
    gOsFromISR = 0;
    return OS_SUCCESS;
}


OS_APIs void OS_Terminate( void )
{
    vTaskEndScheduler();
}



/* Task: */
OS_APIs s32 OS_TaskCreate( OsTask task, const s8 *name, u16 stackSize, void *param, u32 pri, OsTaskHandle *taskHandle )
{
    return xTaskCreate(
            task,                   /* The task to create */
            (const signed char * const)name,     /* Task name */
            stackSize,              /* Stack Size (in WORD (4bytes)) */
            param,                  /* Parameter for Task */
            pri,                    /* Priority: 0 (low) */
            taskHandle );           /* Task Handle */
}


OS_APIs void OS_TaskDelete(OsTaskHandle taskHandle)
{
    vTaskDelete(taskHandle);
}



OS_APIs void OS_StartScheduler( void )
{
    vTaskStartScheduler();  
}



/* Mutex APIs: */
OS_APIs s32 OS_MutexInit( OsMutex *mutex )
{
    *mutex = xSemaphoreCreateMutex();
    if ( NULL == *mutex )
        return OS_FAILED;
    else return OS_SUCCESS;
}


OS_APIs void OS_MutexLock( OsMutex mutex )
{
    xSemaphoreTake( mutex, portMAX_DELAY);
}



OS_APIs void OS_MutexUnLock( OsMutex mutex )
{
    xSemaphoreGive( mutex );
}


OS_APIs void OS_MsDelay(u32 ms)
{
    u32  ticks = ms / portTICK_RATE_MS;
    if (ticks == 0)
        ticks = 1;
    vTaskDelay(ticks);
}





/* Message Queue: */
OS_APIs s32 OS_MsgQCreate( OsMsgQ *MsgQ, u32 QLen )
{
    *MsgQ = xQueueCreate( QLen, sizeof( OsMsgQEntry ) );
    if ( NULL == *MsgQ )
        return OS_FAILED;
    return OS_SUCCESS;
}


OS_APIs s32 OS_MsgQEnqueue( OsMsgQ MsgQ, OsMsgQEntry *MsgItem, bool fromISR )
{
    s32 err, val;
    if (fromISR == false)
        err = xQueueSendToBack( MsgQ, (void *)MsgItem, portMAX_DELAY );
    else {
        err = xQueueSendToBackFromISR( MsgQ, (void *)MsgItem, &val );
        gHigherPrioTaskReady = (pdPASS != err) ? pdFALSE : val;
    }
    return ( pdPASS!=err )? OS_FAILED: OS_SUCCESS;
}

OS_APIs s32 OS_MsgQEnqueueFront( OsMsgQ MsgQ, OsMsgQEntry *MsgItem, bool fromISR )
{
    s32 err, val;
    if (fromISR == false)
        err = xQueueSendToFront( MsgQ, (void *)MsgItem, portMAX_DELAY );
    else {
        err = xQueueSendToFrontFromISR( MsgQ, (void *)MsgItem, &val );
        gHigherPrioTaskReady = (pdPASS != err) ? pdFALSE : val;
    }
    return ( pdPASS!=err )? OS_FAILED: OS_SUCCESS;
}

OS_APIs s32 OS_MsgQDequeue( OsMsgQ MsgQ, OsMsgQEntry *MsgItem, bool block, bool fromISR )
{
    s32 err, val;
	u32 BlockTick;
	block = (fromISR==true)? false: block;
	BlockTick = (block==true)? portMAX_DELAY: 0;

    if (fromISR == false)
        err = xQueueReceive( MsgQ, (void *)MsgItem, BlockTick );
    else {
        err = xQueueReceiveFromISR( MsgQ, (void *)MsgItem, &val );
        gHigherPrioTaskReady = (pdPASS != err) ? pdFALSE : val;
    }
    return ( pdPASS!=err )? OS_FAILED: OS_SUCCESS;
}


OS_APIs s32 OS_MsgQWaitingSize( OsMsgQ MsgQ )
{
    return ( uxQueueMessagesWaiting( MsgQ ) );
}



/* Timer: */
OS_APIs s32 OS_TimerCreate( OsTimer *timer, u32 ms, u8 autoReload, void *args, OsTimerHandler timHandler )
{
    ms = ( 0 == ms )? 1: ms;
    *timer = xTimerCreate( NULL, OS_MS2TICK(ms), autoReload, args, timHandler);
    if ( NULL == *timer )
        return OS_FAILED;
    return OS_SUCCESS;
}

OS_APIs s32 OS_TimerSet( OsTimer timer, u32 ms, u8 autoReload, void *args )
{
    if ( pdFAIL == xTimerChangeSetting( timer, OS_MS2TICK(ms), autoReload, args) )
        return OS_FAILED;
    return OS_SUCCESS;    
}

OS_APIs s32 OS_TimerStart( OsTimer timer )
{
    return xTimerStart( timer, 0 );
}

OS_APIs s32 OS_TimerStop( OsTimer timer )
{
    return xTimerStop( timer, 0 );
}

OS_APIs void *OS_TimerGetData( OsTimer timer )
{
    return pvTimerGetTimerID(timer);
}



#if 0
OS_APIs void OS_TimerGetSetting( OsTimer timer, u8 *autoReload, void **args )
{
    xTimerGetSetting(timer, autoReload, args);
}

OS_APIs bool OS_TimerIsRunning( OsTimer timer )
{
    return xTimerIsTimerActive(timer);
}

#endif


OS_APIs void *OS_MemAlloc( u32 size )
{
    /**
        *  Platform dependent code. Please rewrite 
        *  this piece of code for different system.
        */
    return malloc(size);
}


/**
 *  We do not recommend using OS_MemFree() API 
 *  because we do not want to support memory 
 *  management mechanism in embedded system.
 */
OS_APIs void OS_MemFree( void *m )
{
    /**
        *  Platform depedent code. Please rewrite
        *  this piece of code for different system.
        */
    free(m);
}





