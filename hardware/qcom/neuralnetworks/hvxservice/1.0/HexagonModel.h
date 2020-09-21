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

#ifndef ANDROID_HARDWARE_V1_0_HEXAGON_MODEL_H
#define ANDROID_HARDWARE_V1_0_HEXAGON_MODEL_H

#include <android/hardware/neuralnetworks/1.0/types.h>
#include <atomic>
#include <string>
#include <vector>
#include "CpuExecutor.h"
#include "HexagonController.h"
#include "HexagonOperations.h"
#include "HexagonUtils.h"
#include "OperationsUtils.h"
#include "hexagon_nn_controller/hexagon_nn_controller.h"

namespace android {
namespace hardware {
namespace neuralnetworks {
namespace V1_0 {
namespace implementation {
namespace hexagon {

using ::android::nn::RunTimePoolInfo;
using ::android::nn::Shape;

using NeuralnetworksModel = ::android::hardware::neuralnetworks::V1_0::Model;

// runtime operand information
struct OperandInfo {
    // tensor information
    OperandType type;
    std::vector<uint32_t> dimensions;

    // (optional) quantization paramters
    float scale;
    int32_t zeroPoint;

    // lifetime
    OperandLifeTime lifetime;

    // data location
    uint8_t* buffer;
    uint32_t length;

    // Hexagon nnlib identifiers
    hexagon_nn_input hexagon_input;
    hexagon_nn_input hexagon_input_min;
    hexagon_nn_input hexagon_input_max;
    hexagon_nn_output hexagon_output;
};

// interface wrapper
class Model {
   public:
    // methods
    Model() = delete;
    Model(const Model&) = delete;
    Model& operator=(const Model&) = delete;
    Model(Model&& other);
    Model& operator=(Model&& other);

    Model(const NeuralnetworksModel& model);
    ~Model();

    std::string getLog();
    std::string getGraph();

    // model check
    const int32_t* getPointer(uint32_t operand);
    Shape getShape(uint32_t operand);
    bool setShape(uint32_t operand, const Shape& shape);
    bool isConstant(uint32_t operand);

    // model prepare types
    const hexagon_nn_input& getTensor(uint32_t operand);
    const hexagon_nn_input& getQuantizationMin(uint32_t operand);
    const hexagon_nn_input& getQuantizationMax(uint32_t operand);
    hexagon_nn_input createQuantizationValue(uint32_t operand, int32_t quant_value);
    hexagon_nn_input createConvFilterTensor(uint32_t operand);
    hexagon_nn_input createDepthwiseFilterTensor(uint32_t operand, int32_t depth_multiplier);
    hexagon_nn_input createFullyConnectedWeightTensor(uint32_t operand);
    template <typename Type>
    Type getScalar(uint32_t operand);
    op_type getFloatActivation(uint32_t operand);
    op_type getQuantizedActivation(uint32_t operand);
    hexagon_nn_padding_type getPadding(uint32_t operand);

    template <typename Type>
    hexagon_nn_input createTensor(uint32_t B, uint32_t H, uint32_t W, uint32_t D,
                                  const std::vector<Type>& values);
    hexagon_nn_input createShape(uint32_t B, uint32_t H, uint32_t W, uint32_t D);
    template <typename Type>
    hexagon_nn_input createValues(const std::vector<Type>& values);
    template <typename Type>
    hexagon_nn_input createScalar(Type value);

    // model prepare operations
    bool addBasicOperation(op_type op, hexagon_nn_padding_type pad,
                           const std::vector<hexagon_nn_input>& inputs,
                           const std::vector<uint32_t>& outputs);
    bool addFloatOperationWithActivation(op_type op, hexagon_nn_padding_type pad,
                                         op_type activation,
                                         const std::vector<hexagon_nn_input>& inputs,
                                         const std::vector<uint32_t>& outputs);
    bool addQuant8OperationWithActivation(op_type op, hexagon_nn_padding_type pad,
                                          op_type activation,
                                          const std::vector<hexagon_nn_input>& inputs,
                                          const std::vector<uint32_t>& outputs);
    bool addFusedFloatOperation(op_type op, hexagon_nn_padding_type pad,
                                const hexagon_nn_input& bias, op_type activation,
                                const std::vector<hexagon_nn_input>& inputs,
                                const std::vector<uint32_t>& outputs);
    bool addFusedQuant8Operation(op_type op, hexagon_nn_padding_type pad,
                                 const std::vector<hexagon_nn_input>& bias, op_type activation,
                                 const std::vector<hexagon_nn_input>& inputs,
                                 const std::vector<uint32_t>& outputs);

    std::vector<bool> supportedOperations();
    bool prepare();
    bool execute(const Request& request);

   private:
    uint32_t getNextNode();
    uint32_t addOperationInternal(op_type op, hexagon_nn_padding_type pad,
                                  const std::vector<hexagon_nn_input>& inputs,
                                  const std::vector<hexagon_nn_output>& outputs);
    hexagon_nn_input createTensorInternal(uint32_t B, uint32_t H, uint32_t W, uint32_t D,
                                          const uint8_t* ptr, size_t size);
    std::vector<hexagon_nn_input> setupActivationArgs(op_type op);
    hexagon_nn_input addOperand(uint32_t operand);
    std::vector<hexagon_nn_output> getHexagonOutputs(const std::vector<uint32_t>& operands);
    bool registerHexagonInputs(const std::vector<uint32_t>& operands, uint32_t node);

    bool verifyOperations();
    bool verifyOperands();
    bool addInputs();
    bool addOperations();
    bool addOutputs();

    void clearModel();

    // members
    hexagon_nn_nn_id mGraphId;
    uint32_t mNodeCount;
    bool mCompiled;
    std::vector<OperandInfo> mOperands;
    std::vector<Operation> mOperations;
    std::vector<uint32_t> mInputs;
    std::vector<uint32_t> mOutputs;
    std::vector<RunTimePoolInfo> mPools;
};

// template implementations

template <typename Type>
Type Model::getScalar(uint32_t operand) {
    return *reinterpret_cast<const Type*>(mOperands[operand].buffer);
}

template <typename Type>
hexagon_nn_input Model::createTensor(uint32_t B, uint32_t H, uint32_t W, uint32_t D,
                                     const std::vector<Type>& values) {
    return createTensorInternal(B, H, W, D, reinterpret_cast<const uint8_t*>(values.data()),
                                values.size() * sizeof(Type));
}

template <typename Type>
hexagon_nn_input Model::createValues(const std::vector<Type>& values) {
    return createTensor(1, 1, 1, values.size(), values);
}

template <typename Type>
hexagon_nn_input Model::createScalar(Type value) {
    return createTensorInternal(1, 1, 1, 1, reinterpret_cast<uint8_t*>(&value), sizeof(Type));
}

}  // namespace hexagon
}  // namespace implementation
}  // namespace V1_0
}  // namespace neuralnetworks
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_V1_0_HEXAGON_MODEL_H
