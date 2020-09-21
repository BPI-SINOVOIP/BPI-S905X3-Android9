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
 * \brief pthread 调试工具
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-10-15: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 5

#include <am_debug.h>
#include <am_mem.h>
#include <pthread.h>

/****************************************************************************
 * Type define
 ***************************************************************************/

typedef struct {
	const char       *file;
	const char       *func;
	int               line;
} AM_ThreadFrame_t;

typedef struct AM_Thread AM_Thread_t;
struct AM_Thread {
	AM_Thread_t      *prev;
	AM_Thread_t      *next;
	pthread_t         thread;
	char             *name;
	void* (*entry)(void*);
	void             *arg;
	AM_ThreadFrame_t *frame;
	int               frame_size;
	int               frame_top;
};

/****************************************************************************
 * Static data
 ***************************************************************************/
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_once_t  once = PTHREAD_ONCE_INIT;
static AM_Thread_t    *threads = NULL;

/****************************************************************************
 * Static functions
 ***************************************************************************/

static void thread_init(void)
{
	AM_Thread_t *th;
	
	th = malloc(sizeof(AM_Thread_t));
	if(!th)
	{
		AM_DEBUG(1, "not enough memory");
		return;
	}
	
	th->thread= pthread_self();
	th->name  = strdup("main");
	th->entry = NULL;
	th->arg   = NULL;
	th->prev  = NULL;
	th->next  = NULL;
	th->frame = NULL;
	th->frame_size = 0;
	th->frame_top  = 0;
	
	threads = th;
	
	AM_DEBUG(1, "Register thread \"main\"");
}

static void thread_remove(AM_Thread_t *th)
{
	if(th->prev)
		th->prev->next = th->next;
	else
		threads = th->next;
	if(th->next)
		th->next->prev = th->prev;
	
	if(th->name)
		free(th->name);
	if(th->frame)
		free(th->frame);
	
	free(th);
}

static void* thread_entry(void *arg)
{
	AM_Thread_t *th = (AM_Thread_t*)arg;
	void *r;
	
	pthread_mutex_lock(&lock);
	th->thread = pthread_self();
	pthread_mutex_unlock(&lock);
	
	AM_DEBUG(1, "Register thread \"%s\" %p", th->name, (void*)th->thread);
	
	r = th->entry(th->arg);
	
	pthread_mutex_lock(&lock);
	thread_remove(th);
	pthread_mutex_unlock(&lock);
	
	return r;
}

static AM_Thread_t* thread_get(pthread_t t)
{
	AM_Thread_t *th;
	
	for(th=threads; th; th=th->next)
	{
		if(th->thread==t)
			return th;
	}
	
	return NULL;
}

/****************************************************************************
 * API functions
 ***************************************************************************/

/**\brief 创建一个被AM_THREAD管理的线程
 * \param[out] thread 返回线程句柄
 * \param[in] attr 线程属性，等于NULL时使用缺省属性
 * \param start_routine 线程入口函数
 * \param[in] arg 线程入口函数的参数
 * \param[in] name 线程名
 * \return 成功返回0，失败返回错误代码
 */
int AM_pthread_create_name(pthread_t *thread,
		const pthread_attr_t *attr,
		void* (*start_routine)(void*),
		void *arg,
		const char *name)
{
	AM_Thread_t *th;
	int ret;
	
	pthread_once(&once, thread_init);
	
	th = malloc(sizeof(AM_Thread_t));
	if(!th)
	{
		AM_DEBUG(1, "not enough memory");
		return -1;
	}
	
	th->thread= -1;
	th->name  = name?strdup(name):NULL;
	th->entry = start_routine;
	th->arg   = arg;
	th->prev  = NULL;
	th->frame = NULL;
	th->frame_size = 0;
	th->frame_top  = 0;
	
	pthread_mutex_lock(&lock);
	if(threads)
		threads->prev = th;
	th->next = threads;
	threads = th;
	pthread_mutex_unlock(&lock);
	
	ret = pthread_create(thread, attr, thread_entry, th);
	if(ret)
	{
		AM_DEBUG(1, "create thread failed");
		pthread_mutex_lock(&lock);
		thread_remove(th);
		pthread_mutex_unlock(&lock);
		
		return ret;
	}
	
	return 0;
}

/**\brief 结束当前线程
 * \param[in] r 返回值
 */
void AM_pthread_exit(void *r)
{
	AM_Thread_t *th;
	
	pthread_once(&once, thread_init);
	
	pthread_mutex_lock(&lock);
	
	th = thread_get(pthread_self());
	if(th)
		thread_remove(th);
	else
		AM_DEBUG(1, "thread %p is not registered", (void*)pthread_self());
		
	pthread_mutex_unlock(&lock);
	
	pthread_exit(r);
}

/**\brief 记录当前线程进入一个函数
 * \param[in] file 文件名
 * \param[in] func 函数名
 * \param line 行号
 * \return 成功返回0，失败返回错误代码
 */
int AM_pthread_enter(const char *file, const char *func, int line)
{
	AM_Thread_t *th;
	int ret = 0;
	
	pthread_once(&once, thread_init);
	
	pthread_mutex_lock(&lock);
	
	th = thread_get(pthread_self());
	if(th)
	{
		if((th->frame_top+1)>=th->frame_size)
		{
			AM_ThreadFrame_t *f;
			int size = AM_MAX(th->frame_size*2, 16);
			
			f = realloc(th->frame, sizeof(AM_ThreadFrame_t)*size);
			if(!f)
			{
				AM_DEBUG(1, "not enough memory");
				ret = -1;
				goto error;
			}
			
			th->frame = f;
			th->frame_size = size;
		}
		th->frame[th->frame_top].file = file;
		th->frame[th->frame_top].func = func;
		th->frame[th->frame_top].line = line;
		th->frame_top++;
	}
	else
	{
		AM_DEBUG(1, "thread %p is not registered", (void*)pthread_self());
		ret = -1;
	}
error:
	pthread_mutex_unlock(&lock);
	return ret;
}

/**\brief 记录当前线程离开一个函数
 * \param[in] file 文件名
 * \param[in] func 函数名
 * \param line 行号
 * \return 成功返回0，失败返回错误代码
 */
int AM_pthread_leave(const char *file, const char *func, int line)
{
	AM_Thread_t *th;
	
	pthread_once(&once, thread_init);
	
	pthread_mutex_lock(&lock);
	
	th = thread_get(pthread_self());
	if(th)
	{
		if(!th->frame_top)
			AM_DEBUG(1, "AM_pthread_enter and AM_pthread_leave mismatch");
		else
			th->frame_top--;
	}
	else
	{
		AM_DEBUG(1, "thread %p is not registered", (void*)pthread_self());
	}
		
	pthread_mutex_unlock(&lock);
	
	return 0;
}

/**\brief 打印当前所有注册线程的状态信息
 * \return 成功返回0，失败返回错误代码
 */
int AM_pthread_dump(void)
{
	AM_Thread_t *th;
	int i, l, n;
	
	pthread_once(&once, thread_init);
	
	pthread_mutex_lock(&lock);
	
	for(th=threads,i=0; th; th=th->next, i++)
	{
		if(th->thread==-1)
			continue;
		
		fprintf(stdout, "Thread %d (%p:%s)\n", i, (void*)th->thread, th->name?th->name:"");
		for(l=th->frame_top-1,n=0; l>=0; l--,n++)
		{
			AM_ThreadFrame_t *f = &th->frame[l];
			fprintf(stdout, "\t<%d> %s line %d [%s]\n", n, f->func?f->func:NULL, f->line, f->file?f->file:NULL);
		}
	}
	
	pthread_mutex_unlock(&lock);
	
	return 0;
}

