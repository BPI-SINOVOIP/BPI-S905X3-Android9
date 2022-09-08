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
#ifndef ANDROID_ML_NN_VSI_PALTFORM_H
#define ANDROID_ML_NN_VSI_PALTFORM_H

#include "nnrt/file_map_memory.hpp"
#include "Utils.h"
#include "HalInterfaces.h"

#if ANDROID_SDK_VERSION >= 28
#include "ValidateHal.h"
#endif

#if ANDROID_SDK_VERSION >= 29
#include <ui/GraphicBuffer.h>
#include "Tracing.h"
#include "ExecutionBurstServer.h"
#endif

/*alias namespace for Model*/
#if ANDROID_SDK_VERSION >= 29
    using android::hardware::hidl_array;
    using android::hardware::hidl_memory;
    using android::hidl::memory::V1_0::IMemory;
#endif

#if ANDROID_SDK_VERSION >= 29
    using android::sp;
    using HidlToken = hidl_array<uint8_t, ANEURALNETWORKS_BYTE_SIZE_OF_CACHE_TOKEN>;
#endif

namespace android {
namespace nn {
namespace hal {}
namespace vsi_driver {
    using namespace hal;

    template<int ANDROID_VERSION>
    struct Hal{};

    template<>
    struct Hal<27>{
        using Device            = V1_0::IDevice;
        using PrepareModel      = V1_0::IPreparedModel;
        using Operand           = V1_0::Operand;
        using OperandType       = V1_0::OperandType;
        using Operation         = V1_0::Operation;
        using OperationType     = V1_0::OperationType;
        using Model             = V1_0::Model;
        using OperandLifeTime   = V1_0::OperandLifeTime;
        using ErrorStatus       = V1_0::ErrorStatus;
        using Request           = V1_0::Request;

        template<typename T_type>
        static inline T_type convertVersion(const T_type& variable){
            return variable;
        }

        template<typename T_type>
        static bool inline validatePool(const T_type &hidl_pool){
            return true;
        }
    };

#if ANDROID_SDK_VERSION >= 28
    template<>
    struct Hal<28> {
        using Device            = V1_1::IDevice;
        using PrepareModel      = V1_0::IPreparedModel;
        using Operand           = V1_0::Operand;
        using OperandType       = V1_0::OperandType;
        using Operation         = V1_1::Operation;
        using OperationType     = V1_1::OperationType;
        using Model             = V1_1::Model;
        using OperandLifeTime   = V1_0::OperandLifeTime;
        using ErrorStatus       = V1_0::ErrorStatus;
        using Request           = V1_0::Request;

        template<typename T_type>
        static auto inline convertVersion(T_type variable){
            return android::nn::convertToV1_1(variable);
        }

        template<typename T_type>
        static bool inline validatePool(const T_type& hidl_pool){
            return true;
        }
    };
#endif

#if ANDROID_SDK_VERSION >= 29
    template<>
    struct Hal<29> {
        using Device            = V1_2::IDevice;
        using PrepareModel      = V1_2::IPreparedModel;
        using Operand           = V1_2::Operand;
        using OperandType       = V1_2::OperandType;
        using Operation         = V1_2::Operation;
        using OperationType     = V1_2::OperationType;
        using Model             = V1_2::Model;
        using OperandLifeTime   = V1_0::OperandLifeTime;
        using ErrorStatus       = V1_0::ErrorStatus;
        using Request           = V1_0::Request;

        template<typename T_type>
        static auto inline convertVersion(const T_type& variable) {
            return android::nn::convertToV1_2(variable);
        }

        template<typename T_type>
        static bool inline validatePool(const T_type &hidl_pool){
            return android::nn::validatePool(hidl_pool);;
        }
    };
#endif

#if ANDROID_SDK_VERSION >= 29
using HalPlatform = struct Hal<29>;
#else
using HalPlatform = struct Hal<ANDROID_SDK_VERSION>;
#endif

using ErrorStatus       = HalPlatform::ErrorStatus;
using Request           = HalPlatform::Request;
using OperandLifeTime   = HalPlatform::OperandLifeTime;
using OperandType       = HalPlatform::OperandType;
using OperationType     = HalPlatform::OperationType;
}
}
}

#endif
