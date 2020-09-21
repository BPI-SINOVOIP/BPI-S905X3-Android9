/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: 
 */
#ifdef _FORTIFY_SOURCE
#undef _FORTIFY_SOURCE
#endif
#include "includes.h"
#include "stdio.h"
#include "stdlib.h"
#include "linux/list.h"
#include "memwatch.h"

#define MEMORY_DEBUG                (0x00)

#if MEMORY_DEBUG
#define mem_dbg                     AM_TRACE
#else
#define mem_dbg(...)                {do{}while(0);}
#endif

#if MEMORY_DEBUG
struct mem_log
{
    list_t  head;
    void*   address;
    INT32U  size;
};

static LIST_HEAD(mem_head);

static inline void add_mem_log(void* p, INT32U size)
{
    struct mem_log *log = (struct mem_log *)0;

    log = (struct mem_log*)malloc(sizeof(struct mem_log));
    if (log)
    {
        INIT_LIST_HEAD(&log->head);
        log->address = p;
        log->size = size;

        list_add_tail(&log->head, &mem_head);
    }
    else
    {
        mem_dbg("[%s] out of memory!\r\n");
    }
}

static inline void del_mem_log(struct mem_log *log)
{
    if (log)
    {
        list_del(&log->head);
        free(log);
    }
}

static inline struct mem_log * find_mem_log(void* p)
{
    struct mem_log *log = (struct mem_log *)0;
    list_t* entry = (list_t*)0;

    if (p)
    {
        list_for_each(entry, &mem_head)
        {
            log = list_entry(entry, struct mem_log, head);
            if (log->address == p)
            {
                return log;
            }
        }
    }

    return (struct mem_log *)0;
}

#endif

void  sub_mem_init(void)
{
#if MEMORY_DEBUG
    INIT_LIST_HEAD(&mem_head);
#endif
    return ;
}

void  sub_mem_deinit(void)
{
#if MEMORY_DEBUG
    if (!list_empty(&mem_head))
    {
        struct mem_log *log = (struct mem_log *)0;
        list_t* entry = (list_t*)0;
        list_t* next = (list_t*)0;
        INT32U size = 0;

        list_for_each(entry, &mem_head)
        {
            log = list_entry(entry, struct mem_log, head);
            size += log->size;
        }

        mem_dbg("[%s] memory log not empty, maybe memory leak %d bytes!\r\n", __func__, size);

        list_for_each_safe(entry, next, &mem_head)
        {
            log = list_entry(entry, struct mem_log, head);
            if (log->address)
            {
                free(log->address);
            }
            mem_dbg("[%s] memory leak: address: %p size: %d!\r\n", __func__, log->address, log->size);
            del_mem_log(log);
        }
    }
#endif
    return ;
}

void* sub_mem_malloc(size_t size, char* message)
{
#if MEMORY_DEBUG
    void* p = (void*)0;
    if (size)
    {
        p = malloc(size);
        if (p)
        {
            add_mem_log(p, size);
        }

        mem_dbg("[%s] %s size: %d address: %p\r\n", __func__, message, size, p);
    }
    return p;
#else
    if (size)
    {
        return malloc(size);
    }
    return (void*)0;
#endif
}

void* sub_mem_calloc(size_t num, size_t size, char* message)
{
#if MEMORY_DEBUG
    void* p = (void*)0;
    if (num && size)
    {
        p = calloc(num, size);
        if (p)
        {
            add_mem_log(p, size * num);
        }

        mem_dbg("[%s] %s num: %d size: %d address: %p\r\n", __func__, message, num, size, p);
    }
    return p;
#else
    if (num && size)
    {
        return calloc(num, size);
    }
    return (void*)0;
#endif
}

void* sub_mem_realloc(void *mem, size_t size, char* message)
{
#if MEMORY_DEBUG
    void* p = (void*)mem;
    if (size)
    {
        p = realloc(mem, size);
        if (p)
        {
            del_mem_log(find_mem_log(mem));
            add_mem_log(p, size);
        }

        mem_dbg("[%s] %s old address: %p new size: %d new address: %p\r\n", __func__, message, mem, size, p);
    }
    return p;
#else
    if (size)
    {
        return realloc(mem, size);
    }
    return mem;
#endif
}

void sub_mem_free(void *mem, char* message)
{
#if MEMORY_DEBUG
    struct mem_log *log = (struct mem_log *)0;
    if (mem)
    {
        log = find_mem_log(mem);
        if (log)
        {
            mem_dbg("[%s] %s address: %p size: %d\r\n", __func__, message, mem, log->size);
            del_mem_log(log);
        }
        else
        {
            mem_dbg("[%s] %s address: %p (not in log)\r\n", __func__, message, mem);
        }

        free(mem);
    }
#else
    if (mem)
    {
        free(mem);
    }
#endif
    return ;
}
