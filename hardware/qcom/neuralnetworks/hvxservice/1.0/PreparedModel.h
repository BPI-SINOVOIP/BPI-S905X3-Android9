/*
 * Copyright (C) 2017 The Android Open Source Project
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

#ifndef ANDROID_HARDWARE_NEURALNETWORKS_V1_0_PREPAREDMODEL_H
#define ANDROID_HARDWARE_NEURALNETWORKS_V1_0_PREPAREDMODEL_H

#include <android/hardware/neuralnetworks/1.0/IPreparedModel.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>
#include <memory>
#include "HexagonModel.h"
#include "hexagon_nn_controller/hexagon_nn_controller.h"

namespace android {
namespace hardware {
namespace neuralnetworks {
namespace V1_0 {
namespace implementation {

using ::android::sp;
using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;

struct PreparedModel : public IPreparedModel {
   private:
    PreparedModel() = delete;
    PreparedModel(const PreparedModel&) = delete;
    PreparedModel(PreparedModel&&) = delete;
    PreparedModel& operator=(const PreparedModel&) = delete;
    PreparedModel& operator=(PreparedModel&&) = delete;

   public:
    PreparedModel(const Model& neuralNetworksModel,
                  const std::shared_ptr<hexagon::Model>& hexagonModel);
    ~PreparedModel() override;

    // Methods from IPreparedModel follow.
    Return<ErrorStatus> execute(const Request& request,
                                const sp<IExecutionCallback>& callback) override;

   private:
    Model mNeuralNetworksModel;
    std::shared_ptr<hexagon::Model> mHexagonModel;
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace neuralnetworks
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_NEURALNETWORKS_V1_0_PREPAREDMODEL_H
