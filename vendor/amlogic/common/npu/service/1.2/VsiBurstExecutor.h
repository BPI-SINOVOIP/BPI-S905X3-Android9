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
#ifndef ANDROID_ML_NN_VSI_BURST_EXECUTOR_H
#define ANDROID_ML_NN_VSI_BURST_EXECUTOR_H

#include "VsiPreparedModel.h"
#include "VsiDevice.h"

#include <android-base/logging.h>
#include <hidl/LegacySupport.h>
#include <chrono>
#include <optional>
#include <thread>

namespace android {
namespace nn {
namespace vsi_driver {
class BurstExecutorWithCache : public ExecutionBurstServer::IBurstExecutorWithCache {
   public:
    BurstExecutorWithCache(const HalPlatform::Model& model, ExecutionPreference preference):
        model_(model), preference_(preference){};

    bool isCacheEntryPresent(int32_t slot) const override;

    void addCacheEntry(const hidl_memory& memory, int32_t slot) override;

    void removeCacheEntry(int32_t slot) override { memoryCache_.erase(slot); }

    std::tuple<ErrorStatus, hidl_vec<OutputShape>, Timing> execute(
            const Request& request, const std::vector<int32_t>& slots,
            MeasureTiming measure) override;

   private:
    const HalPlatform::Model model_;
    sp<VsiPreparedModel> perpareModel_;
    std::map<int32_t, std::optional<hidl_memory>> memoryCache_;  // cached hidl memory pool
    ExecutionPreference preference_;
};
}
}
}
#endif
