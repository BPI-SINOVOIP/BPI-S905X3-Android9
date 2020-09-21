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
#ifndef ANDROID_ML_NN_VSI_RTINFO_H
#define ANDROID_ML_NN_VSI_RTINFO_H

#include "nnrt/file_map_memory.hpp"
#include "Utils.h"
#include "HalInterfaces.h"

#if ANDROID_SDK_VERSION >= 29
#include <ui/GraphicBuffer.h>
#endif

namespace android {
namespace nn {
namespace vsi_driver {

    /*record the info that is gotten from hidl_memory*/
    struct VsiRTInfo{
        sp<IMemory>         shared_mem;             /* if hidl_memory is "ashmem", */
                                                                    /* the shared_mem is relative to ptr */
        size_t                buffer_size;
        std::string         mem_type;               /* record type of hidl_memory*/
        uint8_t *           ptr;                          /* record the data pointer gotten from "ashmem" hidl_memory*/
        std::shared_ptr<nnrt::Memory>  vsi_mem;   /* ovx memory object converted from "mmap_fd" hidl_memory*/
#if ANDROID_SDK_VERSION >= 29
        sp<GraphicBuffer> graphic_buffer;
#endif

        VsiRTInfo(): buffer_size(0), ptr(nullptr)
#if ANDROID_SDK_VERSION >= 29
        ,graphic_buffer(nullptr)
#endif
        {};

        ~VsiRTInfo(){
            if("mmap_fd" == mem_type){
                if(vsi_mem) vsi_mem.reset();
            }
#if ANDROID_SDK_VERSION >= 29
            else if("hardware_buffer_blob" == mem_type){
                if(graphic_buffer){
                    graphic_buffer->unlock();
                    graphic_buffer = nullptr;
                }
            }
#endif
        };
        const uint8_t * getPtr(size_t offset){
            if ("ashmem" == mem_type) {
                return ptr;
            } else if ("mmap_fd" == mem_type) {
                return static_cast<const uint8_t*>(
                    vsi_mem->data(offset));
            }
#if ANDROID_SDK_VERSION >= 29
            else if ("hardware_buffer_blob" == mem_type) {
                return ptr;
            }
#endif
            return nullptr;
        }
    };

    extern bool mapHidlMem(const hidl_memory& hidl_memory, VsiRTInfo& vsiMemory);
}
}
}
#endif
