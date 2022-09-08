#define LOG_TAG "ION Interface"

#include "ion_if.h"
#include <linux/ion.h>
#include <ion/ion.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
#include <utils/Log.h>
#include <utils/threads.h>

namespace android {
IONInterface* IONInterface::mIONInstance = nullptr;
int IONInterface::mIONDevice_fd = -1;
Mutex IONInterface::IonLock;
int IONInterface::mCount = 0;
IONInterface::IONInterface() {
    for (int i = 0; i < BUFFER_NUM; i++) {
        mPicBuffers[i].vaddr = nullptr;
        mPicBuffers[i].ion_handle = -1;
        mPicBuffers[i].share_fd = -1;
        mPicBuffers[i].size = 0;
        mPicBuffers[i].IsUsed = false;
    }
    ALOGD("%s construct\n", __FUNCTION__);
}

IONInterface::~IONInterface() {
    for (int i = 0; i < BUFFER_NUM; i++) {
        if (mPicBuffers[i].IsUsed)
            release_node(&mPicBuffers[i]);
    }
    if (mIONDevice_fd >= 0) {
        ion_close(mIONDevice_fd);
        mIONDevice_fd = -1;
    }
}

IONInterface* IONInterface::get_instance() {
    ALOGD("%s\n", __FUNCTION__);
    Mutex::Autolock lock(&IonLock);
    if (mIONInstance != nullptr) {
        mIONDevice_fd = ion_open();
        if (mIONDevice_fd < 0) {
            ALOGE("ion_open failed, %s", strerror(errno));
            mIONDevice_fd = -1;
        } else {
            mCount++;
            return mIONInstance;
        }
    }
    mIONInstance = new IONInterface;
    return mIONInstance;
}

void IONInterface::put_instance() {
    ALOGD("%s\n", __FUNCTION__);
    mCount = mCount - 1;
    if (!mCount && mIONInstance != nullptr) {
        ALOGD("%s delete ION Instance \n", __FUNCTION__);
        delete mIONInstance;
        mIONInstance=nullptr;
    }
}

uint8_t* IONInterface::alloc_buffer(size_t size,int* share_fd) {
        ALOGD("%s\n", __FUNCTION__);
        IONBufferNode* pBuffer = nullptr;
        int i =0;
         if (mIONDevice_fd < 0) {
            ALOGE("ion dose not init");
            return nullptr;
        }
        Mutex::Autolock lock(&IonLock);
        for (i = 0; i < BUFFER_NUM; i++) {
            if (mPicBuffers[i].IsUsed == false) {
                mPicBuffers[i].IsUsed = true;
                pBuffer = &mPicBuffers[i];
                ALOGD("---------------%s:size= %d,buffer idx = %d\n", __FUNCTION__,size,i);
                break;
            }
        }
        if (i == BUFFER_NUM) {
            ALOGE("%s: alloc fail",__FUNCTION__);
            return nullptr;
        }
        pBuffer->size = size;
        int ret = ion_alloc(mIONDevice_fd, size, 0, 1 << ION_HEAP_TYPE_DMA, 0, &pBuffer->ion_handle);
        if (ret < 0) {
                ALOGE("ion_alloc failed, %s", strerror(errno));
                return nullptr;
        }
        ret = ion_share(mIONDevice_fd, pBuffer->ion_handle, &pBuffer->share_fd);
        if (ret < 0) {
                ALOGE("ion_share failed, %s", strerror(errno));
                ion_free(mIONDevice_fd, pBuffer->ion_handle);
                return nullptr;
        }
        uint8_t* cpu_ptr = (uint8_t*)mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED,
                    pBuffer->share_fd, 0);
            if (MAP_FAILED == cpu_ptr) {
                ALOGE("ion mmap error!\n");
                ion_free(mIONDevice_fd, pBuffer->ion_handle);
                return nullptr;
            }
            if (cpu_ptr == nullptr)
                ALOGE("cpu_ptr is NULL");
        pBuffer->vaddr = cpu_ptr;
        ALOGE("vaddr=%p, share_fd = %d",pBuffer->vaddr,pBuffer->share_fd);
        *share_fd = pBuffer->share_fd;
        return pBuffer->vaddr;
}

int IONInterface::release_node(IONBufferNode* pBuffer) {
    pBuffer->IsUsed = false;
    int ret = munmap(pBuffer->vaddr, pBuffer->size);
    ALOGD("-----------%s: vaddr = %p", __FUNCTION__,pBuffer->vaddr);
    if (ret)
        ALOGD("munmap fail: %s\n",strerror(errno));

    ret = close(pBuffer->share_fd);
    ALOGD("-----------%s: share_fd = %d", __FUNCTION__,pBuffer->share_fd);
    if (ret != 0)
        ALOGD("close ion shared fd failed for reason %s",strerror(errno));
    ret = ion_free(mIONDevice_fd, pBuffer->ion_handle);
    if (ret != 0)
        ALOGD("ion_free failed for reason %s",strerror(errno));
    return ret;
}

void IONInterface::free_buffer(int share_fd) {
    ALOGD("%s\n", __FUNCTION__);
    IONBufferNode* pBuffer = nullptr;
    Mutex::Autolock lock(&IonLock);
    for (int i = 0; i < BUFFER_NUM; i++) {
        if (mPicBuffers[i].IsUsed && share_fd == mPicBuffers[i].share_fd) {
            pBuffer = &mPicBuffers[i];
            ALOGD("---------------%s:buffer idx = %d\n", __FUNCTION__,i);
            break;
        }
    }
    if (pBuffer == nullptr) {
        ALOGD("%s:free buffer error!\n",__FUNCTION__);
        return;
    }
    release_node(pBuffer);
}
}
