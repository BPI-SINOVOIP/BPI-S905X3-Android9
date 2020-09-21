#ifdef _FORTIFY_SOURCE
#undef _FORTIFY_SOURCE
#endif
/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file
  * \brief 驱动模块事件接口
 *事件是替代Callback函数的一种异步触发机制。每一个事件上可以同时注册多个回调函数。
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-09-13: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 5
#define _GNU_SOURCE

#include <am_debug.h>
#include <am_evt.h>
#include <am_mem.h>
#include <am_thread.h>
#include <assert.h>

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

#define AM_EVT_BUCKET_COUNT    50

/****************************************************************************
 * Type definitions
 ***************************************************************************/

/**\brief 事件*/
typedef struct AM_Event AM_Event_t;
struct AM_Event
{
	AM_Event_t        *next;    /**< 指向链表中的下一个事件*/
	AM_EVT_Callback_t  cb;      /**< 回调函数*/
	int                type;    /**< 事件类型*/
	long               dev_no;  /**< 设备号*/
	void              *data;    /**< 用户回调参数*/
};

/****************************************************************************
 * Static data
 ***************************************************************************/

static AM_Event_t *events[AM_EVT_BUCKET_COUNT];

#ifndef ANDROID
static pthread_mutex_t lock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
#else
#ifdef PTHREAD_RECURSIVE_MUTEX_INITIALIZER
static pthread_mutex_t lock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER;
#else
static pthread_mutex_t lock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
#endif
#endif

pthread_mutexattr_t attr;

pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER;

/****************************************************************************
 * API functions
 ***************************************************************************/

/**\brief 注册一个事件回调函数
 * \param dev_no 回调函数对应的设备ID
 * \param event_type 回调函数对应的事件类型
 * \param cb 回调函数指针
 * \param data 传递给回调函数的用户定义参数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_EVT_Subscribe(long dev_no, int event_type, AM_EVT_Callback_t cb, void *data)
{
	AM_Event_t *evt;
	int pos;
    int i;
	
	assert(cb);
	
    //同样的callback注册,过滤掉.
    //有相同dev_no的可能性.(malloc函数分配的在释放后,再次分配,经常是同一个指针)
    //暂时还是原来的设计,使用dev_no作为事件标志之一,但实际上意义不大,主要还是使用event_type,比如合理.
    //同样cb与dev_no绑定的意义不大,造成上层需要每次配对调用Subscribe/unSubscribe
    //Subscribe/unSubscribe如果在cb函数里面做,又极易出现逻辑难题(延后处理),或者死锁.
    //除非上层使用msg重启用线程来处理,但这是个不合理的强制设计,pass
    //事件与监听有多对多的需求吗?好像没有

    pthread_rwlock_rdlock(&rwlock);
    for(i = 0; i < AM_EVT_BUCKET_COUNT; i++)	
	for(evt=events[i]; evt; evt=evt->next)
	{
        //--如果使用dev_no过滤,那就会有可能会有少量内存浪费,如果不使用,但dev_no又是动态的(handle),fk
        if(evt->dev_no == dev_no && evt->cb == cb && evt->type == event_type)
        {
            AM_DEBUG(1, "the same cb set");
            pthread_rwlock_unlock(&rwlock);
            return AM_SUCCESS;
        }
	}
    pthread_rwlock_unlock(&rwlock);
    
    
	/*分配事件*/
	evt = malloc(sizeof(AM_Event_t));
	if(!evt)
	{
		AM_DEBUG(1, "not enough memory");
		return AM_EVT_ERR_NO_MEM;
	}
	evt->dev_no = dev_no;
	evt->type   = event_type;
	evt->cb     = cb;
	evt->data   = data;
	
	pos = event_type%AM_EVT_BUCKET_COUNT;
	
	/*加入事件哈希表中*/
	//pthread_mutex_lock(&lock);
	pthread_rwlock_wrlock(&rwlock);
	evt->next   = events[pos];
	events[pos] = evt;
	
	//pthread_mutex_unlock(&lock);
	pthread_rwlock_unlock(&rwlock);
	return AM_SUCCESS;
}

/**\brief 反注册一个事件回调函数
 * \param dev_no 回调函数对应的设备ID
 * \param event_type 回调函数对应的事件类型
 * \param cb 回调函数指针
 * \param data 传递给回调函数的用户定义参数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_EVT_Unsubscribe(long dev_no, int event_type, AM_EVT_Callback_t cb, void *data)
{
	AM_Event_t *evt, *eprev;
	int pos;
	
	assert(cb);
	
	pos = event_type%AM_EVT_BUCKET_COUNT;
	
	//pthread_mutex_lock(&lock);
	pthread_rwlock_wrlock(&rwlock);
	for(eprev=NULL,evt=events[pos]; evt; eprev=evt,evt=evt->next)
	{
		if((evt->dev_no==dev_no) && (evt->type==event_type) && (evt->cb==cb) &&
				(evt->data==data))
		{
			if(eprev)
				eprev->next = evt->next;
			else
				events[pos] = evt->next;
			break;
		}
	}
	
	//pthread_mutex_unlock(&lock);
	pthread_rwlock_unlock(&rwlock);
	if(evt)
	{
		free(evt);
		return AM_SUCCESS;
	}
	
	return AM_EVT_ERR_NOT_SUBSCRIBED;
}

/**\brief 触发一个事件
 * \param dev_no 产生事件的设备ID
 * \param event_type 产生事件的类型
 * \param[in] param 事件参数
 */
AM_ErrorCode_t AM_EVT_Signal(long dev_no, int event_type, void *param)
{
	AM_Event_t *evt;
	int pos = event_type%AM_EVT_BUCKET_COUNT;
	
	//1.使用读写锁,
    //2.多线程可以同时发送signal事件,
    //3.多线程如果同时发送同一事件,并且同时触发同一callback,会有风险,但一般来说,同一事件,只由同一线程发送,更不应该同时刻.
    //4.如果有同一时刻,不同线程,发送同一事件,触发同一callback存在,那就是相当的个例,可以在上层那个个例的cb函数里面自己加锁.
    //5.但上层要避免在cb事件响应函数里面去AM_EVT_Subscribe/AM_EVT_Unsubscribe事件链表,因为这样会造成死锁.
    //6.不应该也不需要在cb函数里面去AM_EVT_Subscribe/AM_EVT_Unsubscribe事件链表,切记!!!
	pthread_rwlock_rdlock(&rwlock);
	for(evt=events[pos]; evt; evt=evt->next)
	{
		if((evt->dev_no==dev_no) && (evt->type==event_type))
		{
            //pthread_mutex_lock(&lock);
			evt->cb(dev_no, event_type, param, evt->data);
            //pthread_mutex_unlock(&lock);
		}
	}
    pthread_rwlock_unlock(&rwlock);
	return AM_SUCCESS;
}
/*
*/
AM_ErrorCode_t AM_EVT_Init()
{
    int i;
    pthread_mutexattr_init(&attr);    
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);  
    pthread_mutex_init(&lock, &attr);    
    pthread_mutexattr_destroy(&attr);
    
    for(i = 0; i < AM_EVT_BUCKET_COUNT; i++) events[i] = NULL;
    return AM_SUCCESS;
}
/*
*/
AM_ErrorCode_t AM_EVT_Destory()
{
    AM_Event_t *evt;
    int i;
    pthread_mutex_destroy(&lock);
	for(i = 0; i < AM_EVT_BUCKET_COUNT; i++)	
	for(evt=events[i]; evt; evt=evt->next)
	{
        free(evt);
	}
    
    return AM_SUCCESS;
}
