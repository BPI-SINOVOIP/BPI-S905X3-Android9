/****************************************************************************
*
*    Copyright (c) 2020 Vivante Corporation
*
*    Permission is hereby granted, free of charge, to any person obtaining a
*    copy of this software and associated documentation files (the "Software"),
*    to deal in the Software without restriction, including without limitation
*    the rights to use, copy, modify, merge, publish, distribute, sublicense,
*    and/or sell copies of the Software, and to permit persons to whom the
*    Software is furnished to do so, subject to the following conditions:
*
*    The above copyright notice and this permission notice shall be included in
*    all copies or substantial portions of the Software.
*
*    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
*    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
*    DEALINGS IN THE SOFTWARE.
*
*****************************************************************************/
#include "VsiPlatform.h"
#include "VsiRTInfo.h"

namespace android {
namespace nn {
namespace vsi_driver {

bool mapHidlMem(const hidl_memory& hidl_memory, VsiRTInfo& vsiMemory) {
#if ANDROID_SDK_VERSION >= 29
    sp<GraphicBuffer> graphic_buffer = nullptr;
#endif
    std::shared_ptr<nnrt::Memory> vsi_mem = nullptr;
    sp<IMemory> shared_mem = nullptr;
    uint8_t* buffer = nullptr;

    if (!HalPlatform::validatePool(hidl_memory)) {
        LOG(ERROR) << "invalid hidl memory pool";
        return false;
    }

    if ("ashmem" == hidl_memory.name()) {
        shared_mem = mapMemory(hidl_memory);
        assert(shared_mem);
        shared_mem->read();
        buffer = reinterpret_cast<uint8_t*>(static_cast<void*>(shared_mem->getPointer()));
    } else if ("mmap_fd" == hidl_memory.name()) {
        size_t size = hidl_memory.size();
        int fd = hidl_memory.handle()->data[0];
        int mode = hidl_memory.handle()->data[1];
        size_t offset =
            getSizeFromInts(hidl_memory.handle()->data[2], hidl_memory.handle()->data[3]);

        vsi_mem = std::make_shared<nnrt::Memory>();
        vsi_mem->readFromFd(size, mode, fd, offset);
    }
#if ANDROID_SDK_VERSION >= 29
    else if ("hardware_buffer_blob" == hidl_memory.name()) {
        auto handle = hidl_memory.handle();
        auto format = AHARDWAREBUFFER_FORMAT_BLOB;
        auto usage = AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN | AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN;
        const uint32_t width = hidl_memory.size();
        const uint32_t height = 1;  // height is always 1 for BLOB mode AHardwareBuffer.
        const uint32_t layers = 1;  // layers is always 1 for BLOB mode AHardwareBuffer.
        const uint32_t stride = hidl_memory.size();
        graphic_buffer = new GraphicBuffer(handle,
                                           GraphicBuffer::HandleWrapMethod::CLONE_HANDLE,
                                           width,
                                           height,
                                           format,
                                           layers,
                                           usage,
                                           stride);
        void* gBuffer = nullptr;
        auto status = graphic_buffer->lock(usage, &gBuffer);
        if (status != NO_ERROR) {
            LOG(ERROR) << "RunTimePoolInfo Can't lock the AHardwareBuffer.";
            return false;
        }
        buffer = static_cast<uint8_t*>(gBuffer);
    } else {
        LOG(ERROR) << "invalid hidl_memory";
        return false;
    }
#endif
    vsiMemory.shared_mem = shared_mem;
    vsiMemory.mem_type = std::string(hidl_memory.name());
    vsiMemory.ptr = buffer;
    vsiMemory.vsi_mem = vsi_mem;
    vsiMemory.buffer_size = hidl_memory.size();
#if ANDROID_SDK_VERSION >= 29
    vsiMemory.graphic_buffer = graphic_buffer;
#endif
    return true;
}
}
}
}

