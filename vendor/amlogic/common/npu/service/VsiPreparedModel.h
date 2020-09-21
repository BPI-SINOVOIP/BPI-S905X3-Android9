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
#ifndef ANDROID_ML_VSI_PREPAREMODEL_H
#define ANDROID_ML_VSI_PREPAREMODEL_H
#include <vector>

#include "nnrt/model.hpp"
#include "nnrt/types.hpp"
#include "nnrt/compilation.hpp"
#include "nnrt/execution.hpp"
#include "nnrt/op/public.hpp"
#include "nnrt/file_map_memory.hpp"
#include "HalInterfaces.h"
#include "VsiPlatform.h"
#include "VsiRTInfo.h"

#include "Utils.h"
using android::sp;

namespace android {
namespace nn {
namespace vsi_driver {

class VsiPreparedModel : public HalPlatform::PrepareModel
{
   public:
    VsiPreparedModel(const HalPlatform::Model& model, ExecutionPreference preference):
        model_(model), preference_(preference){
        native_model_ = std::make_shared<nnrt::Model>();
    }

    ~VsiPreparedModel() override {
            release_rtinfo(const_buffer_);
        }

    /*map hidl model to ovxlib model and compliation*/
    Return<ErrorStatus> initialize();
    Return<ErrorStatus> execute(const Request& request,
                                const sp<V1_0::IExecutionCallback>& callback) override;

#if ANDROID_SDK_VERSION >= 29
    Return<void> executeSynchronously(const Request& request, MeasureTiming measure,
                                   executeSynchronously_cb cb) override;

    Return<ErrorStatus> execute_1_2(const Request& request, MeasureTiming measure,
                                 const sp<V1_2::IExecutionCallback>& callback) override;

    Return<void> configureExecutionBurst(
            const sp<V1_2::IBurstCallback>& callback,
            const android::hardware::MQDescriptorSync<V1_2::FmqRequestDatum>& requestChannel,
            const android::hardware::MQDescriptorSync<V1_2::FmqResultDatum>& resultChannel,
            configureExecutionBurst_cb cb) override;
#endif

   private:
        enum IO{INPUT = 0, OUTPUT};
#if ANDROID_SDK_VERSION < 29
        struct OutputShape{
            std::vector<uint32_t> dimensions;
            bool    isSufficient;
        };
#endif
        void release_rtinfo(std::vector<VsiRTInfo>& rtInfos);

        void fill_operand_value(nnrt::op::OperandPtr ovx_operand, const HalPlatform::Operand& hal_operand) ;

        Return<ErrorStatus>  construct_ovx_operand( nnrt::op::OperandPtr ovx_oprand,
                                                         const HalPlatform::Operand& hal_operand);

        Return<ErrorStatus> map_rtinfo_from_hidl_memory( const hidl_vec<hidl_memory>& pools,
                                                              std::vector<VsiRTInfo>& rtInfos);

        void update_operand_from_request( const std::vector<uint32_t>& indexes,
                                          const hidl_vec<RequestArgument>& arguments);

        // Adjust the runtime info for the arguments passed to the model,
        // modifying the buffer location, and possibly the dimensions.
        Return<ErrorStatus> update_pool_info_from_request( const Request& request,
                                                           const std::vector<uint32_t>& indexes,
                                                           const hidl_vec<RequestArgument>& arguments,
                                                           IO flag, std::vector<OutputShape> &outputShapes);

        template <typename T_IExecutionCallback>
        Return<ErrorStatus> executeBase(const Request& request,
                                         const T_IExecutionCallback& callback);

#if ANDROID_SDK_VERSION >= 29
        template <typename T_IExecutionCallback>
        Return<ErrorStatus> executeBase(const Request& request,
                                         MeasureTiming measure,
                                         const T_IExecutionCallback& callback);

        static Return<void> notify( const sp<V1_0::IExecutionCallback>& callback, const ErrorStatus& status,
                                 const hidl_vec<OutputShape>&, Timing);

        static Return<void> notify(const sp<V1_2::IExecutionCallback>& callback, const ErrorStatus& status,
                                const hidl_vec<OutputShape>& outputShapes, Timing timing);

        static void notify(const V1_2::IPreparedModel::executeSynchronously_cb& callback, const ErrorStatus& status,
                        const hidl_vec<OutputShape>& outputShapes, Timing timing);
#endif

        const HalPlatform::Model model_;
        ExecutionPreference preference_;
        std::shared_ptr<nnrt::Model> native_model_;
        std::shared_ptr<nnrt::Compilation> native_compile_;
        std::shared_ptr<nnrt::Execution> native_exec_;

        /*store pointer of all of hidl_memory to buffer*/
        std::vector<VsiRTInfo> const_buffer_;
        std::vector<VsiRTInfo> io_buffer_;
};
}
}
}
#endif
