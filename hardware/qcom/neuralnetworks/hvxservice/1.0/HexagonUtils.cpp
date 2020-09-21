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

#define LOG_TAG "android.hardware.neuralnetworks@1.0-impl-hvx"

#include "HexagonUtils.h"
#include <hidlmemory/mapping.h>
#include <algorithm>
#include <numeric>
#include <vector>
#include "OperationsUtils.h"

namespace android {
namespace hardware {
namespace neuralnetworks {
namespace V1_0 {
namespace implementation {
namespace hexagon {

bool isHexagonAvailable() {
    int version = -1;
    Controller::getInstance().version(&version);
    if (version != 92) {
        LOG(INFO) << "ATTEMPTING TO RESTART NNLIB";
        Controller::getInstance().resetNnlib();
        Controller::getInstance().version(&version);
    }
    return version == 92;
}

hexagon_nn_padding_type getPadding(uint32_t pad) {
    switch (pad) {
        case ::android::nn::kPaddingSame:
            return NN_PAD_SAME;
        case ::android::nn::kPaddingValid:
            return NN_PAD_VALID;
        case ::android::nn::kPaddingUnknown:
        default:
            return NN_PAD_NA;
    };
}

hexagon_nn_padding_type getPadding(int32_t inWidth, int32_t inHeight, int32_t strideWidth,
                                   int32_t strideHeight, int32_t filterWidth, int32_t filterHeight,
                                   int32_t paddingLeft, int32_t paddingRight, int32_t paddingTop,
                                   int32_t paddingBottom) {
    return getPadding(::android::nn::getPaddingScheme(inWidth, inHeight, strideWidth, strideHeight,
                                                      filterWidth, filterHeight, paddingLeft,
                                                      paddingRight, paddingTop, paddingBottom));
}

op_type getFloatActivationFunction(FusedActivationFunc act) {
    switch (act) {
        case FusedActivationFunc::RELU:
            return OP_Relu_f;
        case FusedActivationFunc::RELU1:
            return OP_Clamp_f;
        case FusedActivationFunc::RELU6:
            return OP_ReluX_f;
        case FusedActivationFunc::NONE:
            FALLTHROUGH_INTENDED;
        default:
            return OP_Nop;
    };
}

op_type getQuantizedActivationFunction(FusedActivationFunc act) {
    switch (act) {
        case FusedActivationFunc::RELU:
            return OP_QuantizedRelu_8;
        case FusedActivationFunc::RELU1:
            return OP_QuantizedClamp_8;
        case FusedActivationFunc::RELU6:
            return OP_QuantizedReluX_8;
        case FusedActivationFunc::NONE:
            FALLTHROUGH_INTENDED;
        default:
            return OP_Nop;
    };
}

uint32_t getSize(OperandType type) {
    static const uint32_t sizes[] = {
        4,  // FLOAT32
        4,  // INT32
        4,  // UINT32
        4,  // TENSOR_FLOAT32
        4,  // TENSOR_INT32
        1,  // TENSOR_SYMMETRICAL_QUANT8
    };
    HEXAGON_SOFT_ASSERT(static_cast<uint32_t>(type) < sizeof(sizes) / sizeof(*sizes),
                        "Error: type exceeds max enum value");
    return sizes[static_cast<uint32_t>(type)];
}

std::vector<uint32_t> getAlignedDimensions(const std::vector<uint32_t>& dims, uint32_t N) {
    HEXAGON_SOFT_ASSERT_GE(
        N, dims.size(),
        "Error: constant data dimensions " << dims.size() << " exceeds alignment of " << N);
    std::vector<uint32_t> dimensions(N - dims.size(), 1);
    dimensions.insert(dimensions.end(), dims.begin(), dims.end());
    return dimensions;
}

std::vector<RunTimePoolInfo> mapPools(const hidl_vec<hidl_memory>& pools) {
    std::vector<RunTimePoolInfo> poolInfos;
    poolInfos.reserve(pools.size());
    bool fail = false;
    for (const auto& pool : pools) {
        poolInfos.emplace_back(pool, &fail);
    }
    HEXAGON_SOFT_ASSERT(!fail, "Error setting pools");
    return poolInfos;
}

std::unordered_set<uint32_t> getPoolIndexes(const std::vector<RequestArgument>& inputsOutputs) {
    std::unordered_set<uint32_t> indexes;
    for (const RequestArgument& inputOutput : inputsOutputs) {
        indexes.insert(inputOutput.location.poolIndex);
    }
    return indexes;
}

namespace {
const uint8_t* getDataFromBlock(const hidl_vec<uint8_t>& block, uint32_t offset, uint32_t length) {
    HEXAGON_SOFT_ASSERT_LE(offset + length, block.size(),
                           "Error: trying to copy data from outside of block bounds");
    return block.data() + offset;
}

const uint8_t* getDataFromPool(const RunTimePoolInfo& pool, uint32_t offset,
                               [[maybe_unused]] uint32_t length) {
    // HEXAGON_SOFT_ASSERT_LE(offset + length, pool->getSize(),
    //                       "Error: trying to copy data from outside of pool bounds");
    return pool.getBuffer() + offset;
}
}  // anonymous namespace

const uint8_t* getData(const Operand& operand, const hidl_vec<uint8_t>& block,
                       const std::vector<RunTimePoolInfo>& pools) {
    switch (operand.lifetime) {
        case OperandLifeTime::TEMPORARY_VARIABLE:
            return nullptr;
        case OperandLifeTime::MODEL_INPUT:
        case OperandLifeTime::MODEL_OUTPUT:
            HEXAGON_SOFT_ASSERT(false,
                                "Error: trying to retrieve data that is only known at runtime");
        case OperandLifeTime::CONSTANT_COPY:
            return getDataFromBlock(block, operand.location.offset, operand.location.length);
        case OperandLifeTime::CONSTANT_REFERENCE:
            return getDataFromPool(pools[operand.location.poolIndex], operand.location.offset,
                                   operand.location.length);
        default:
            HEXAGON_SOFT_ASSERT(false, "Error: unrecognized operand lifetime");
    }
}

bool operator==(const hexagon_nn_input& lhs, const hexagon_nn_input& rhs) {
    return lhs.src_id == rhs.src_id && lhs.output_idx == rhs.output_idx;
}

bool operator!=(const hexagon_nn_input& lhs, const hexagon_nn_input& rhs) {
    return !(lhs == rhs);
}

bool operator==(const hexagon_nn_output& lhs, const hexagon_nn_output& rhs) {
    return lhs.rank == rhs.rank && lhs.max_sizes[0] == rhs.max_sizes[0] &&
           lhs.max_sizes[1] == rhs.max_sizes[1] && lhs.max_sizes[2] == rhs.max_sizes[2] &&
           lhs.max_sizes[3] == rhs.max_sizes[3] && lhs.max_sizes[4] == rhs.max_sizes[4] &&
           lhs.max_sizes[5] == rhs.max_sizes[5] && lhs.max_sizes[6] == rhs.max_sizes[6] &&
           lhs.max_sizes[7] == rhs.max_sizes[7] && lhs.elementsize == rhs.elementsize &&
           lhs.zero_offset == rhs.zero_offset && lhs.stepsize == rhs.stepsize;
}

bool operator!=(const hexagon_nn_output& lhs, const hexagon_nn_output& rhs) {
    return !(lhs == rhs);
}

hexagon_nn_output make_hexagon_nn_output(const std::vector<uint32_t>& dims, uint32_t size) {
    std::vector<uint32_t> alignedDims = getAlignedDimensions(dims, 4);
    hexagon_nn_output output = {
        .rank = std::min(8u, static_cast<uint32_t>(alignedDims.size())),
        .max_sizes = {0, 0, 0, 0, 0, 0, 0, 0},
        .elementsize = size,
        .zero_offset = 0,
        .stepsize = 0.0f,
    };
    for (size_t i = 0; i < alignedDims.size() && i < 8; ++i) {
        output.max_sizes[i] = alignedDims[i];
    }
    return output;
}

// printers
std::string toString(uint32_t val) {
    return std::to_string(val);
}

std::string toString(float val) {
    return std::to_string(val);
}

std::string toString(hexagon_nn_nn_id id) {
    return std::to_string(static_cast<int32_t>(id));
}

std::string toString(op_type op) {
    static const char* opText[] = {
#define DEF_OP(NAME, ...) "OP_" #NAME,
#include "hexagon_nn_controller/ops.def"
#undef DEF_OP
    };
    return static_cast<size_t>(op) < sizeof(opText) / sizeof(char*)
               ? opText[static_cast<size_t>(op)]
               : "<invalid op_type>";
}

std::string toString(hexagon_nn_padding_type padding) {
    static const char* paddingText[] = {
        "NN_PAD_NA",
        "NN_PAD_SAME",
        "NN_PAD_VALID",
        "NN_PAD_MIRROR_REFLECT",
        "NN_PAD_MIRROR_SYMMETRIC",
        "NN_PAD_SAME_CAFFE",
    };
    return static_cast<size_t>(padding) < sizeof(paddingText) / sizeof(char*)
               ? paddingText[static_cast<size_t>(padding)]
               : "<invalid hexagon_nn_padding_type>";
}

std::string toString(const hexagon_nn_input& input) {
    return "hexagon_nn_input{.src_id: " + std::to_string(input.src_id) +
           ", .output_idx: " + std::to_string(input.output_idx) + "}";
}

std::string toString(const hexagon_nn_output& output) {
    return "hexagon_nn_output{.rank: " + std::to_string(output.rank) + ", .max_sizes: [" +
           std::to_string(output.max_sizes[0]) + ", " + std::to_string(output.max_sizes[1]) + ", " +
           std::to_string(output.max_sizes[2]) + ", " + std::to_string(output.max_sizes[3]) + ", " +
           std::to_string(output.max_sizes[4]) + ", " + std::to_string(output.max_sizes[5]) + ", " +
           std::to_string(output.max_sizes[6]) + ", " + std::to_string(output.max_sizes[7]) + "]" +
           ", .elementsize: " + std::to_string(output.elementsize) +
           ", .zero_offset: " + std::to_string(output.zero_offset) +
           ", .stepsize: " + std::to_string(output.stepsize) + "}";
}

std::string toString(const hexagon_nn_tensordef& tensordef) {
    return "hexagon_nn_tensordef{.batches: " + std::to_string(tensordef.batches) +
           ", .height: " + std::to_string(tensordef.height) +
           ", .width: " + std::to_string(tensordef.width) +
           ", .depth: " + std::to_string(tensordef.depth) +
           ", .data: " + std::to_string(reinterpret_cast<uintptr_t>(tensordef.data)) +
           ", .dataLen: " + std::to_string(tensordef.dataLen) +
           ", .data_valid_len: " + std::to_string(tensordef.data_valid_len) +
           ", .unused: " + std::to_string(tensordef.unused) + "}";
}

std::string toString(const hexagon_nn_perfinfo& perfinfo) {
    return "hexagon_nn_perfinfo{.node_id: " + std::to_string(perfinfo.node_id) +
           ", .executions: " + std::to_string(perfinfo.executions) +
           ", .counter_lo: " + std::to_string(perfinfo.counter_lo) +
           ", .counter_hi: " + std::to_string(perfinfo.counter_hi) + "}";
}

std::string toString(const ::android::nn::Shape& shape) {
    return "Shape{.type: " + toString(shape.type) +
           ", .dimensions: " + toString(shape.dimensions.data(), shape.dimensions.size()) +
           ", .scale: " + std::to_string(shape.scale) +
           ", .zeroPoint: " + std::to_string(shape.offset) + "}";
}

}  // namespace hexagon
}  // namespace implementation
}  // namespace V1_0
}  // namespace neuralnetworks
}  // namespace hardware
}  // namespace android
