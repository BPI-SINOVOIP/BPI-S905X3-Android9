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

#include "VsiBurstExecutor.h"
#include "Tracing.h"

namespace android {
namespace nn {
namespace vsi_driver {

    using time_point = std::chrono::steady_clock::time_point;
    static const Timing kNoTiming = {.timeOnDevice = UINT64_MAX, .timeInDriver = UINT64_MAX};

    bool BurstExecutorWithCache::isCacheEntryPresent(int32_t slot) const {
        LOG(INFO)<<__FUNCTION__;
        const auto it = memoryCache_.find(slot);
        return (it != memoryCache_.end()) && it->second.has_value();
    }

    void BurstExecutorWithCache::addCacheEntry(const hidl_memory& memory, int32_t slot) {
        LOG(INFO)<<__FUNCTION__;
        memoryCache_[slot] = memory;
    }

    std::tuple<ErrorStatus, hidl_vec<OutputShape>, Timing>
        BurstExecutorWithCache:: execute(
            const Request& request, const std::vector<int32_t>& slots,
            MeasureTiming measure)  {
        NNTRACE_FULL(NNTRACE_LAYER_DRIVER, NNTRACE_PHASE_EXECUTION,
                     "BurstExecutorWithCache::execute");
        LOG(INFO)<<__FUNCTION__;

        // ensure all relevant pools are valid
        if (!std::all_of(slots.begin(), slots.end(),
                         [this](int32_t slot) { return isCacheEntryPresent(slot); })) {
            LOG(ERROR)<<"There is invalid slot";
            return {ErrorStatus::INVALID_ARGUMENT, {}, kNoTiming};
        }

        // finish the request object (for validation)
        hidl_vec<hidl_memory> pools(slots.size());
        std::transform(slots.begin(), slots.end(), pools.begin(),
                       [this](int32_t slot) { return memoryCache_[slot].value(); });
        Request fullRequest = request;
        fullRequest.pools = std::move(pools);

        // validate request object against the model
        if (!validateRequest(fullRequest, model_)) {
            LOG(ERROR)<<"invalid request";
            return {ErrorStatus::INVALID_ARGUMENT, {}, kNoTiming};
        }

        if(!perpareModel_){
            perpareModel_ = new VsiPreparedModel(model_, preference_);
            auto status = perpareModel_->initialize();
            if( ErrorStatus::NONE != status){
                LOG(ERROR)<<"Fail to initialize";
                return {ErrorStatus::INVALID_ARGUMENT, {}, kNoTiming};
            }
        }

        ErrorStatus result = ErrorStatus::NONE;
        hidl_vec<OutputShape> outputShapes;
        Timing timing = kNoTiming;
        perpareModel_->executeSynchronously(fullRequest, MeasureTiming::NO,
            [&result, &outputShapes, &timing](ErrorStatus error, const hidl_vec<OutputShape>& shapes,
                                            const Timing& time) {
                result = error;
                outputShapes = shapes;
                timing = time;
            });
        LOG(INFO)<<__FUNCTION__<<" exit";
        return std::make_tuple(result, outputShapes, timing);
     }
};
}
}
