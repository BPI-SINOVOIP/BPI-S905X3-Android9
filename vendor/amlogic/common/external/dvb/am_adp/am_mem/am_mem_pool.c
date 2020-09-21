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
 * \brief 内存池管理
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-05-14: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 5

#include <am_debug.h>
#include <assert.h>
#include <am_mem.h>

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

#define AM_PTR_ALIGN (sizeof(void*))

/****************************************************************************
 * Type definitions
 ***************************************************************************/

typedef struct AM_MEM_BlockHeader AM_MEM_BlockHeader_t;

/**\brief 内存池调用系统malloc函数分配的每个内存块基本信息*/
struct AM_MEM_BlockHeader
{
	AM_MEM_BlockHeader_t *next;        /**< 链表中的下一个内存块*/
	int                   size;        /**< 内存块大小*/
	int                   used;        /**< 已使用内存大小*/
};

/****************************************************************************
 * API functions
 ***************************************************************************/

/**\brief 初始化一个缓冲池
 * \param[out] pool 需要初始化的缓冲池结构
 * \param pool_size 每次调用系统分配函数分配的内存大小
 */
void AM_MEM_PoolInit(AM_MEM_Pool_t *pool, int pool_size)
{
	assert(pool && pool_size);
	
	pool->pools = NULL;
	pool->pool_size = pool_size;
}

/**\brief 从缓冲池分配内存
 * \param[in,out] pool 缓冲池指针
 * \param size 要分配的内存大小
 * \return
 *   - 返回分配内存的指针
 *   - 如果分配失败返回NULL
 */
void* AM_MEM_PoolAlloc(AM_MEM_Pool_t *pool, int size)
{
	AM_MEM_BlockHeader_t *hdr;
	void *ptr;
	
	assert(pool && size);
	
	size = (size+AM_PTR_ALIGN-1)&~(AM_PTR_ALIGN-1);
	hdr = (AM_MEM_BlockHeader_t*)pool->pools;
	
	if(!hdr || (size>(hdr->size-hdr->used)))
	{
		int allocs=AM_MAX(size, pool->pool_size);
	
		hdr = (AM_MEM_BlockHeader_t*)AM_MEM_Alloc(allocs+sizeof(AM_MEM_BlockHeader_t));
		if(!hdr)
			return NULL;
		
		hdr->next = pool->pools;
		hdr->size = allocs;
		hdr->used = 0;
	}
	
	ptr = ((char*)(hdr+1))+hdr->used;
	hdr->used += size;
	
	return ptr;
}

/**\brief 将缓冲池内全部以分配的内存标记，但不调用系统free()
 * \param[in,out] pool 缓冲池指针
 */
void AM_MEM_PoolClear(AM_MEM_Pool_t *pool)
{
	AM_MEM_BlockHeader_t *hdr;
	
	assert(pool);
	
	hdr = (AM_MEM_BlockHeader_t*)pool->pools;
	if(hdr)
	{
		while(hdr->next)
		{
			AM_MEM_BlockHeader_t *tmp = hdr;
			hdr = hdr->next;
			AM_MEM_Free(tmp);
		}
		
		hdr->used = 0;
		pool->pools = hdr;
	}
}

/**\brief 将缓冲池内全部以分配的内存标记，调用系统free()释放全部资源
 * \param[in,out] pool 缓冲池指针
 */
void AM_MEM_PoolFree(AM_MEM_Pool_t *pool)
{
	AM_MEM_BlockHeader_t *hdr;
	
	assert(pool);
	
	hdr = (AM_MEM_BlockHeader_t*)pool->pools;
	while(hdr)
	{
		AM_MEM_BlockHeader_t *tmp = hdr;
		hdr = hdr->next;
		AM_MEM_Free(tmp);
	}
	
	pool->pools = NULL;
}

