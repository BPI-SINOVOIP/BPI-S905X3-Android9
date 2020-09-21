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

#ifndef ANDROID_HARDWARE_V1_0_UTILS_H
#define ANDROID_HARDWARE_V1_0_UTILS_H

#include <android-base/logging.h>
#include <android/hardware/neuralnetworks/1.0/types.h>
#include <string>
#include <unordered_set>
#include <vector>
#include "CpuExecutor.h"
#include "HexagonController.h"
#include "OperationsUtils.h"
#include "hexagon_nn_controller/hexagon_nn_controller.h"

#define HEXAGON_SOFT_ASSERT(condition, message)                          \
    if (!(condition)) {                                                  \
        LOG(DEBUG) << __FILE__ << "::" << __LINE__ << " -- " << message; \
        return {};                                                       \
    }

#define HEXAGON_SOFT_ASSERT_CMP(cmp, lhs, rhs, message)                        \
    HEXAGON_SOFT_ASSERT(((lhs)cmp(rhs)), "failed " #lhs " " #cmp " " #rhs " (" \
                                             << (lhs) << " " #cmp " " << (rhs) << "): " message)

#define HEXAGON_SOFT_ASSERT_EQ(lhs, rhs, message) HEXAGON_SOFT_ASSERT_CMP(==, lhs, rhs, message)
#define HEXAGON_SOFT_ASSERT_NE(lhs, rhs, message) HEXAGON_SOFT_ASSERT_CMP(!=, lhs, rhs, message)
#define HEXAGON_SOFT_ASSERT_LT(lhs, rhs, message) HEXAGON_SOFT_ASSERT_CMP(<, lhs, rhs, message)
#define HEXAGON_SOFT_ASSERT_LE(lhs, rhs, message) HEXAGON_SOFT_ASSERT_CMP(<=, lhs, rhs, message)
#define HEXAGON_SOFT_ASSERT_GT(lhs, rhs, message) HEXAGON_SOFT_ASSERT_CMP(>, lhs, rhs, message)
#define HEXAGON_SOFT_ASSERT_GE(lhs, rhs, message) HEXAGON_SOFT_ASSERT_CMP(>=, lhs, rhs, message)

namespace android {
namespace hardware {
namespace neuralnetworks {
namespace V1_0 {
namespace implementation {
namespace hexagon {

using ::android::sp;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_vec;
using ::android::hardware::neuralnetworks::V1_0::FusedActivationFunc;
using ::android::hardware::neuralnetworks::V1_0::Operand;
using ::android::nn::RunTimePoolInfo;

bool isHexagonAvailable();

hexagon_nn_padding_type getPadding(uint32_t pad);
hexagon_nn_padding_type getPadding(int32_t inWidth, int32_t inHeight, int32_t strideWidth,
                                   int32_t strideHeight, int32_t filterWidth, int32_t filterHeight,
                                   int32_t paddingLeft, int32_t paddingRight, int32_t paddingTop,
                                   int32_t paddingBottom);
op_type getFloatActivationFunction(FusedActivationFunc act);
op_type getQuantizedActivationFunction(FusedActivationFunc act);

uint32_t getSize(OperandType type);
std::vector<uint32_t> getAlignedDimensions(const std::vector<uint32_t>& dims, uint32_t N);

std::vector<RunTimePoolInfo> mapPools(const hidl_vec<hidl_memory>& pools);

std::unordered_set<uint32_t> getPoolIndexes(const std::vector<RequestArgument>& inputsOutputs);

const uint8_t* getData(const Operand& operand, const hidl_vec<uint8_t>& block,
                       const std::vector<RunTimePoolInfo>& pools);

template <typename Type>
std::vector<Type> transpose(uint32_t height, uint32_t width, const Type* input) {
    std::vector<Type> output(height * width);
    for (uint32_t i = 0; i < height; ++i) {
        for (uint32_t j = 0; j < width; ++j) {
            output[j * height + i] = input[i * width + j];
        }
    }
    return output;
}

hexagon_nn_output make_hexagon_nn_output(const std::vector<uint32_t>& dims, uint32_t size);

bool operator==(const hexagon_nn_input& lhs, const hexagon_nn_input& rhs);
bool operator!=(const hexagon_nn_input& lhs, const hexagon_nn_input& rhs);
bool operator==(const hexagon_nn_output& lhs, const hexagon_nn_output& rhs);
bool operator!=(const hexagon_nn_output& lhs, const hexagon_nn_output& rhs);

// printers
std::string toString(uint32_t val);
std::string toString(float val);
std::string toString(hexagon_nn_nn_id id);
std::string toString(op_type op);
std::string toString(hexagon_nn_padding_type padding);
std::string toString(const hexagon_nn_input& input);
std::string toString(const hexagon_nn_output& output);
std::string toString(const hexagon_nn_tensordef& tensordef);
std::string toString(const hexagon_nn_perfinfo& perfinfo);
std::string toString(const ::android::nn::Shape& input);

template <typename Type>
std::string toString(const Type* buffer, uint32_t count) {
    std::string os = "[";
    for (uint32_t i = 0; i < count; ++i) {
        os += (i == 0 ? "" : ", ") + toString(buffer[i]);
    }
    return os += "]";
}

template <typename CharT, typename Traits>
std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& os,
                                              const hexagon_nn_input& obj) {
    return os << toString(obj);
}

template <typename CharT, typename Traits>
std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& os,
                                              const hexagon_nn_output& obj) {
    return os << toString(obj);
}

template <typename CharT, typename Traits>
std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& os,
                                              const hexagon_nn_tensordef& obj) {
    return os << toString(obj);
}

template <typename CharT, typename Traits>
std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& os,
                                              const hexagon_nn_perfinfo& obj) {
    return os << toString(obj);
}

template <typename CharT, typename Traits>
std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& os,
                                              const ::android::nn::Shape& obj) {
    return os << toString(obj);
}

template <typename CharT, typename Traits>
std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& os,
                                              ErrorStatus status) {
    return os << toString(status);
}

}  // namespace hexagon
}  // namespace implementation
}  // namespace V1_0
}  // namespace neuralnetworks
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_V1_0_UTILS_H
