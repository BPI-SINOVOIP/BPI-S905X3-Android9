/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "CAMHAL_MemoryManager   "

#include "CameraHal.h"
#include "ExCameraParameters.h"

namespace android {
/*--------------------MemoryManager Class STARTS here-----------------------------*/
int MemoryManager::setRequestMemoryCallback(camera_request_memory get_memory)
{
    mRequestMemory = get_memory;
    return 0;
}

void* MemoryManager::allocateBuffer(int width, int height, const char* format, int &bytes, int numBufs)
{
    LOG_FUNCTION_NAME;
    ///We allocate numBufs+1 because the last entry will be marked NULL to indicate 
    ///end of array, which is used when freeing the buffers
    const uint numArrayEntriesC = (uint)(numBufs+1);

    if (!mRequestMemory)
        {
        CAMHAL_LOGEA("no req. mem cb");
        LOG_FUNCTION_NAME_EXIT;
        return NULL;
        }

    ///Allocate a buffer array
    uint32_t *bufsArr = new uint32_t [numArrayEntriesC];
    if(!bufsArr)
        {
        CAMHAL_LOGEB("Allocation failed when creating buffers array of %d uint32_t elements", numArrayEntriesC);
        LOG_FUNCTION_NAME_EXIT;
        return NULL;
        }

    ///Initialize the array with zeros - this will help us while freeing the array in case of error
    ///If a value of an array element is NULL, it means we didnt allocate it
    memset(bufsArr, 0, sizeof(*bufsArr) * numArrayEntriesC);

    //2D Allocations are not supported currently
    if(bytes != 0)
        {
        camera_memory_t* handle = NULL;

        ///1D buffers
        for (int i = 0; i < numBufs; i++)
            {
            handle = mRequestMemory(-1, bytes, 1, NULL);
            if(!handle)
                {
                CAMHAL_LOGEA("req. mem failed");
                goto error;
                }

            CAMHAL_LOGDB("handle = %x, nSize = %d", (uint32_t)handle, bytes);
            bufsArr[i] = (uint32_t)handle;

            mMemoryHandleMap.add(bufsArr[i], (unsigned int)handle);
            }
        }

        LOG_FUNCTION_NAME_EXIT;

        return (void*)bufsArr;

error:
    CAMHAL_LOGEA("Freeing buffers already allocated after error occurred");
    freeBuffer(bufsArr);

    if ( NULL != mErrorNotifier.get() )
        {
        mErrorNotifier->errorNotify(-ENOMEM);
        }

    LOG_FUNCTION_NAME_EXIT;
    return NULL;
}

uint32_t * MemoryManager::getOffsets()
{
    LOG_FUNCTION_NAME;
    LOG_FUNCTION_NAME_EXIT;
    return NULL;
}

int MemoryManager::getFd()
{
    LOG_FUNCTION_NAME;
    LOG_FUNCTION_NAME_EXIT;
    return -1;
}

int MemoryManager::freeBuffer(void* buf)
{
    status_t ret = NO_ERROR;
    LOG_FUNCTION_NAME;

    uint32_t *bufEntry = (uint32_t*)buf;

    if(!bufEntry)
        {
        CAMHAL_LOGEA("NULL pointer passed to freebuffer");
        LOG_FUNCTION_NAME_EXIT;
        return BAD_VALUE;
        }

    while(*bufEntry)
        {
        unsigned int ptr = (unsigned int) *bufEntry++;
        camera_memory_t* handle = (camera_memory_t*)mMemoryHandleMap.valueFor(ptr);
        if(handle)
            {
            handle->release(handle);
            }
        else
            {
            CAMHAL_LOGEA("Not a valid Memory Manager buffer");
            }
        }

    ///@todo Check if this way of deleting array is correct, else use malloc/free
    uint32_t * bufArr = (uint32_t*)buf;
    delete [] bufArr;

    LOG_FUNCTION_NAME_EXIT;
    return ret;
}

status_t MemoryManager::setErrorHandler(ErrorNotifier *errorNotifier)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    if ( NULL == errorNotifier )
        {
        CAMHAL_LOGEA("Invalid Error Notifier reference");
        ret = -EINVAL;
        }

    if ( NO_ERROR == ret )
        {
        mErrorNotifier = errorNotifier;
        }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

};


/*--------------------MemoryManager Class ENDS here-----------------------------*/
