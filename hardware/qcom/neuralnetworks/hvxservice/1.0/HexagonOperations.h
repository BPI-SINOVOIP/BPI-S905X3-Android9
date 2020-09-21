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

#ifndef ANDROID_HARDWARE_V1_0_HEXAGON_OPERATIONS_H
#define ANDROID_HARDWARE_V1_0_HEXAGON_OPERATIONS_H

#include <android/hardware/neuralnetworks/1.0/types.h>
#include <functional>
#include <map>
#include "HexagonUtils.h"
#include "hexagon_nn_controller/hexagon_nn_controller.h"

namespace android {
namespace hardware {
namespace neuralnetworks {
namespace V1_0 {
namespace implementation {
namespace hexagon {

class Model;
using HexagonModel = ::android::hardware::neuralnetworks::V1_0::implementation::hexagon::Model;

using ::android::hardware::neuralnetworks::V1_0::Operand;

using OperationTuple = std::pair<OperationType, OperandType>;

using HexagonOperationFn =
    std::function<bool(const std::vector<uint32_t>& /* ins */,
                       const std::vector<uint32_t>& /* outs */, HexagonModel* /* model */)>;

using OperationTable = std::map<OperationTuple, HexagonOperationFn>;
OperationTable& getOperationPrepareTable();
OperationTable& getOperationCheckTable();

}  // namespace hexagon
}  // namespace implementation
}  // namespace V1_0
}  // namespace neuralnetworks
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_V1_0_HEXAGON_OPERATIONS_H
