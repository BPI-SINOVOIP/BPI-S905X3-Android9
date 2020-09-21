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
#define LOG_TAG "VsiDevice"

#include "VsiDevice.h"

#include "HalInterfaces.h"
#include "ValidateHal.h"

#include <android-base/logging.h>
#include <hidl/LegacySupport.h>
#include <thread>
namespace android {
namespace nn {
namespace vsi_driver {

    Return<ErrorStatus> VsiDevice::prepareModel_1_2(const V1_2::Model& model, ExecutionPreference preference,
        const hidl_vec<hidl_handle>& modelCache,
        const hidl_vec<hidl_handle>& dataCache,
        const HidlToken& token,
        const sp<V1_2::IPreparedModelCallback>& callback) {
        return prepareModelBase(model, preference, callback);
    }

    Return<ErrorStatus> VsiDevice::prepareModelFromCache(
        const hidl_vec<hidl_handle>&, const hidl_vec<hidl_handle>&, const HidlToken&,
        const sp<V1_2::IPreparedModelCallback>& callback) {
        notify(callback, ErrorStatus::GENERAL_FAILURE, nullptr);
        return ErrorStatus::GENERAL_FAILURE;
    }
}
}
}

