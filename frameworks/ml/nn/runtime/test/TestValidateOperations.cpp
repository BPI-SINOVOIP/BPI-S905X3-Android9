/*
 * Copyright (C) 2018 The Android Open Source Project
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

#include "NeuralNetworksWrapper.h"
#include "NeuralNetworksOEM.h"

#include <gtest/gtest.h>

using namespace android::nn::wrapper;

namespace {

static const int32_t kAvailableOperandCodes[] = {
    ANEURALNETWORKS_FLOAT32,
    ANEURALNETWORKS_INT32,
    ANEURALNETWORKS_UINT32,
    ANEURALNETWORKS_TENSOR_FLOAT32,
    ANEURALNETWORKS_TENSOR_INT32,
    ANEURALNETWORKS_TENSOR_QUANT8_ASYMM,
    ANEURALNETWORKS_TENSOR_OEM_BYTE
};

class OperationTestBase {
public:
    OperationTestBase(ANeuralNetworksOperationType opCode,
                      std::vector<ANeuralNetworksOperandType> validInputs,
                      std::vector<ANeuralNetworksOperandType> validOutputs)
        : mOpCode(opCode),
          mValidInputs(std::move(validInputs)),
          mValidOutputs(std::move(validOutputs)) {}

    // Add each operand separately and add the operation using these operands.
    // This function does not cover the cases that a operand used mutiple times.
    int32_t addOperation(const std::vector<ANeuralNetworksOperandType>& inputs,
                         const std::vector<ANeuralNetworksOperandType>& outputs) {
        ANeuralNetworksModel* model = nullptr;
        ANeuralNetworksModel_create(&model);

        uint32_t opIdx = 0;
        std::vector<uint32_t> inputIds;
        std::vector<uint32_t> outputIds;
        for (uint32_t i = 0; i < inputs.size(); i++) {
            ANeuralNetworksModel_addOperand(model, &inputs[i]);
            inputIds.push_back(opIdx++);
        }
        for (uint32_t i = 0; i < outputs.size(); i++) {
            ANeuralNetworksModel_addOperand(model, &outputs[i]);
            outputIds.push_back(opIdx++);
        }

        int32_t result = ANeuralNetworksModel_addOperation(model, mOpCode,
                                                 static_cast<uint32_t>(inputIds.size()),
                                                 inputIds.data(),
                                                 static_cast<uint32_t>(outputIds.size()),
                                                 outputIds.data());
        ANeuralNetworksModel_free(model);
        return result;
    }

    bool testMutatingInputOperandCode() {
        for (uint32_t i = 0; i < mValidInputs.size(); i++) {
            // LSH_PROJECTION's second argument is allowed to have any type.
            // This is the only operation that currently has a type that can be
            // anything independent from any other type. Changing the operand
            // type to any other type will result in a valid model for
            // LSH_PROJECTION. If this is the case, skip the test.
            if (mOpCode == ANEURALNETWORKS_LSH_PROJECTION && i == 1) {
                continue;
            }
            ANeuralNetworksOperandType newType = mValidInputs[i];
            int32_t originalOperandCode = mValidInputs[i].type;
            for (int32_t newOperandCode : kAvailableOperandCodes) {
                if (newOperandCode == originalOperandCode) {
                    continue;
                }
                newType.type = newOperandCode;
                std::vector<ANeuralNetworksOperandType> inputs = mValidInputs;
                inputs[i] = newType;
                int32_t result = addOperation(inputs, mValidOutputs);
                if (ANEURALNETWORKS_NO_ERROR == result) {
                    return false;
                }
            }
        }
        return true;
    }

    bool testMutatingOutputOperandCode() {
        for (uint32_t i = 0; i < mValidOutputs.size(); i++) {
            // LSH_PROJECTION's second argument is allowed to have any type.
            // This is the only operation that currently has a type that can be
            // anything independent from any other type. Changing the operand
            // type to any other type will result in a valid model for
            // LSH_PROJECTION. If this is the case, skip the test.
            if (mOpCode == ANEURALNETWORKS_LSH_PROJECTION && i == 1) {
                continue;
            }
            ANeuralNetworksOperandType newType = mValidOutputs[i];
            int32_t originalOperandCode = mValidOutputs[i].type;
            for (int32_t newOperandCode : kAvailableOperandCodes) {
                if (newOperandCode == originalOperandCode) {
                    continue;
                }
                newType.type = newOperandCode;
                std::vector<ANeuralNetworksOperandType> outputs = mValidOutputs;
                outputs[i] = newType;
                int32_t result = addOperation(mValidInputs, outputs);
                if (ANEURALNETWORKS_NO_ERROR == result) {
                    return false;
                }
            }
        }
        return true;
    }

    bool testMutatingInputOperandCounts() {
        std::vector<ANeuralNetworksOperandType> inputs = mValidInputs;
        for (uint32_t i = 0; i < 5; i++) {
            inputs.push_back(inputs[0]);
            if (ANEURALNETWORKS_NO_ERROR == addOperation(inputs, mValidOutputs)) {
                return false;
            }
        }
        return true;
    }

    bool testMutatingOutputOperandCounts() {
        std::vector<ANeuralNetworksOperandType> outputs = mValidOutputs;
        for (int i = 0; i < 5; i++) {
            outputs.push_back(outputs[0]);
            if (ANEURALNETWORKS_NO_ERROR == addOperation(mValidInputs, outputs)) {
                return false;
            }
        }
        return true;
    }

private:
    ANeuralNetworksOperationType mOpCode;
    // The dimensions in the ANeuralNetworksOperandType must outlive the test object.
    std::vector<ANeuralNetworksOperandType> mValidInputs;
    std::vector<ANeuralNetworksOperandType> mValidOutputs;
};

TEST(OperationValidationTest, DEQUANTIZE_float32) {
    uint32_t inputDimensions[4] = {2, 2, 2, 2};
    ANeuralNetworksOperandType input = {.type = ANEURALNETWORKS_TENSOR_QUANT8_ASYMM,
                                        .dimensionCount = 4,
                                        .dimensions = inputDimensions,
                                        .scale = 1.0f,
                                        .zeroPoint = 0};
    ANeuralNetworksOperandType output = {.type = ANEURALNETWORKS_TENSOR_FLOAT32,
                                         .dimensionCount = 4,
                                         .dimensions = inputDimensions,
                                         .scale = 0.0f,
                                         .zeroPoint = 0};
    OperationTestBase dequantizeTest(ANEURALNETWORKS_DEQUANTIZE, {input}, {output});

    EXPECT_TRUE(dequantizeTest.testMutatingInputOperandCode());
    EXPECT_TRUE(dequantizeTest.testMutatingInputOperandCounts());
    EXPECT_TRUE(dequantizeTest.testMutatingOutputOperandCode());
    EXPECT_TRUE(dequantizeTest.testMutatingOutputOperandCounts());
}

void simpleMathOpTest(ANeuralNetworksOperationType operationCode, int32_t operandCode) {
    uint32_t inputDimensions[4] = {2, 2, 2, 2};
    ANeuralNetworksOperandType input1 = {.type = operandCode,
                                         .dimensionCount = 4,
                                         .dimensions = inputDimensions,
                                         .scale = 0.0f,
                                         .zeroPoint = 0};
    if (operandCode == ANEURALNETWORKS_TENSOR_QUANT8_ASYMM) {
        input1.scale = 0.5f;
    }

    ANeuralNetworksOperandType input2 = input1;
    ANeuralNetworksOperandType output = input1;
    ANeuralNetworksOperandType activation = {.type = ANEURALNETWORKS_INT32,
                                             .dimensionCount = 0,
                                             .dimensions = nullptr,
                                             .scale = 0.0f,
                                             .zeroPoint = 0};

    OperationTestBase simpleMathTest(operationCode, {input1, input2, activation}, {output});

    EXPECT_TRUE(simpleMathTest.testMutatingInputOperandCode());
    EXPECT_TRUE(simpleMathTest.testMutatingInputOperandCounts());
    EXPECT_TRUE(simpleMathTest.testMutatingOutputOperandCode());
    EXPECT_TRUE(simpleMathTest.testMutatingOutputOperandCounts());
}

TEST(OperationValidationTest, ADD_float32) {
    simpleMathOpTest(ANEURALNETWORKS_ADD, ANEURALNETWORKS_TENSOR_FLOAT32);
}

TEST(OperationValidationTest, MUL_float32) {
    simpleMathOpTest(ANEURALNETWORKS_MUL, ANEURALNETWORKS_TENSOR_FLOAT32);
}

TEST(OperationValidationTest, SUB_float32) {
    simpleMathOpTest(ANEURALNETWORKS_SUB, ANEURALNETWORKS_TENSOR_FLOAT32);
}

TEST(OperationValidationTest, DIV_float32) {
    simpleMathOpTest(ANEURALNETWORKS_DIV, ANEURALNETWORKS_TENSOR_FLOAT32);
}

TEST(OperationValidationTest, ADD_quant8) {
    simpleMathOpTest(ANEURALNETWORKS_ADD, ANEURALNETWORKS_TENSOR_QUANT8_ASYMM);
}

TEST(OperationValidationTest, MUL_quant8) {
    simpleMathOpTest(ANEURALNETWORKS_MUL, ANEURALNETWORKS_TENSOR_QUANT8_ASYMM);
}

void activationOpTest(ANeuralNetworksOperationType operationCode, int32_t operandCode) {
    uint32_t inputDimensions[4] = {2, 2, 2, 2};
    ANeuralNetworksOperandType input = {.type = operandCode,
                                        .dimensionCount = 4,
                                        .dimensions = inputDimensions,
                                        .scale = 0.0f,
                                        .zeroPoint = 0};
    if (operandCode == ANEURALNETWORKS_TENSOR_QUANT8_ASYMM) {
        input.scale = 1.f / 256;
    }

    ANeuralNetworksOperandType output = input;
    OperationTestBase activationTest(operationCode, {input}, {output});

    EXPECT_TRUE(activationTest.testMutatingInputOperandCode());
    EXPECT_TRUE(activationTest.testMutatingInputOperandCounts());
    EXPECT_TRUE(activationTest.testMutatingOutputOperandCode());
    EXPECT_TRUE(activationTest.testMutatingOutputOperandCounts());
}

TEST(OperationValidationTest, L2_NORMALIZATION_float32) {
    activationOpTest(ANEURALNETWORKS_L2_NORMALIZATION, ANEURALNETWORKS_TENSOR_FLOAT32);
}

TEST(OperationValidationTest, FLOOR_float32) {
    activationOpTest(ANEURALNETWORKS_FLOOR, ANEURALNETWORKS_TENSOR_FLOAT32);
}

TEST(OperationValidationTest, TANH_float32) {
    activationOpTest(ANEURALNETWORKS_TANH, ANEURALNETWORKS_TENSOR_FLOAT32);
}

TEST(OperationValidationTest, RELU_float32) {
    activationOpTest(ANEURALNETWORKS_RELU, ANEURALNETWORKS_TENSOR_FLOAT32);
}

TEST(OperationValidationTest, RELU1_float32) {
    activationOpTest(ANEURALNETWORKS_RELU, ANEURALNETWORKS_TENSOR_FLOAT32);
}

TEST(OperationValidationTest, RELU6_float32) {
    activationOpTest(ANEURALNETWORKS_RELU, ANEURALNETWORKS_TENSOR_FLOAT32);
}

TEST(OperationValidationTest, LOGISTIC_float32) {
    activationOpTest(ANEURALNETWORKS_LOGISTIC, ANEURALNETWORKS_TENSOR_FLOAT32);
}

TEST(OperationValidationTest, RELU_quant8) {
    activationOpTest(ANEURALNETWORKS_RELU, ANEURALNETWORKS_TENSOR_QUANT8_ASYMM);
}

TEST(OperationValidationTest, RELU1_quant8) {
    activationOpTest(ANEURALNETWORKS_RELU, ANEURALNETWORKS_TENSOR_QUANT8_ASYMM);
}

TEST(OperationValidationTest, RELU6_quant8) {
    activationOpTest(ANEURALNETWORKS_RELU, ANEURALNETWORKS_TENSOR_QUANT8_ASYMM);
}

TEST(OperationValidationTest, LOGISTIC_quant8) {
    activationOpTest(ANEURALNETWORKS_LOGISTIC, ANEURALNETWORKS_TENSOR_QUANT8_ASYMM);
}

void softmaxOpTest(int32_t operandCode) {
    uint32_t inputDimensions[4] = {2, 2, 2, 2};
    ANeuralNetworksOperandType input = {.type = operandCode,
                                         .dimensionCount = 4,
                                         .dimensions = inputDimensions,
                                         .scale = 0.0f,
                                         .zeroPoint = 0};
    if (operandCode == ANEURALNETWORKS_TENSOR_QUANT8_ASYMM) {
        input.scale = 1.f / 256;
    }

    ANeuralNetworksOperandType output = input;
    ANeuralNetworksOperandType beta = {.type = ANEURALNETWORKS_FLOAT32,
                                       .dimensionCount = 0,
                                       .dimensions = nullptr,
                                       .scale = 0.0f,
                                       .zeroPoint = 0};

    OperationTestBase softmaxTest(ANEURALNETWORKS_SOFTMAX, {input, beta}, {output});

    EXPECT_TRUE(softmaxTest.testMutatingInputOperandCode());
    EXPECT_TRUE(softmaxTest.testMutatingInputOperandCounts());
    EXPECT_TRUE(softmaxTest.testMutatingOutputOperandCode());
    EXPECT_TRUE(softmaxTest.testMutatingOutputOperandCounts());
}

TEST(OperationValidationTest, SOFTMAX_float32) {
    softmaxOpTest(ANEURALNETWORKS_TENSOR_FLOAT32);
}

TEST(OperationValidationTest, SOFTMAX_quant8) {
    softmaxOpTest(ANEURALNETWORKS_TENSOR_QUANT8_ASYMM);
}

void poolingOpTest(ANeuralNetworksOperationType operationCode, int32_t operandCode) {
    uint32_t inputDimensions[4] = {2, 4, 4, 2};
    ANeuralNetworksOperandType input = {.type = operandCode,
                                        .dimensionCount = 4,
                                        .dimensions = inputDimensions,
                                        .scale = 0.0f,
                                        .zeroPoint = 0};
    if (operandCode == ANEURALNETWORKS_TENSOR_QUANT8_ASYMM) {
        input.scale = 1.f / 256;
    }
    ANeuralNetworksOperandType output = input;

    ANeuralNetworksOperandType scalar = {.type = ANEURALNETWORKS_INT32,
                                         .dimensionCount = 0,
                                         .dimensions = nullptr,
                                         .scale = 0.0f,
                                         .zeroPoint = 0};
    ANeuralNetworksOperandType padLeft = scalar;
    ANeuralNetworksOperandType padRight = scalar;
    ANeuralNetworksOperandType padTop = scalar;
    ANeuralNetworksOperandType padBottom = scalar;
    ANeuralNetworksOperandType strideWidth = scalar;
    ANeuralNetworksOperandType strideHeight = scalar;
    ANeuralNetworksOperandType filterWidth = scalar;
    ANeuralNetworksOperandType filterHeight = scalar;
    ANeuralNetworksOperandType activation = scalar;

    OperationTestBase explicitPoolingTest(operationCode,
                                          {input,
                                           padLeft, padRight, padTop, padBottom,
                                           strideWidth, strideHeight,
                                           filterWidth, filterHeight,
                                           activation},
                                          {output});

    EXPECT_TRUE(explicitPoolingTest.testMutatingInputOperandCode());
    EXPECT_TRUE(explicitPoolingTest.testMutatingInputOperandCounts());
    EXPECT_TRUE(explicitPoolingTest.testMutatingOutputOperandCode());
    EXPECT_TRUE(explicitPoolingTest.testMutatingOutputOperandCounts());

    ANeuralNetworksOperandType padImplicit = scalar;
    OperationTestBase implicitPoolingTest(operationCode,
                                          {input,
                                           padImplicit,
                                           strideWidth, strideHeight,
                                           filterWidth, filterHeight,
                                           activation},
                                          {output});

    EXPECT_TRUE(implicitPoolingTest.testMutatingInputOperandCode());
    EXPECT_TRUE(implicitPoolingTest.testMutatingInputOperandCounts());
    EXPECT_TRUE(implicitPoolingTest.testMutatingOutputOperandCode());
    EXPECT_TRUE(implicitPoolingTest.testMutatingOutputOperandCounts());
}

TEST(OperationValidationTest, AVERAGE_POOL_2D_float32) {
    poolingOpTest(ANEURALNETWORKS_AVERAGE_POOL_2D, ANEURALNETWORKS_TENSOR_FLOAT32);
}

TEST(OperationValidationTest, MAX_POOL_2D_float32) {
    poolingOpTest(ANEURALNETWORKS_MAX_POOL_2D, ANEURALNETWORKS_TENSOR_FLOAT32);
}

TEST(OperationValidationTest, L2_POOL_2D_float32) {
    poolingOpTest(ANEURALNETWORKS_L2_POOL_2D, ANEURALNETWORKS_TENSOR_FLOAT32);
}

TEST(OperationValidationTest, AVERAGE_POOL_2D_quant8) {
    poolingOpTest(ANEURALNETWORKS_AVERAGE_POOL_2D, ANEURALNETWORKS_TENSOR_QUANT8_ASYMM);
}

TEST(OperationValidationTest, MAX_POOL_2D_quant8) {
    poolingOpTest(ANEURALNETWORKS_MAX_POOL_2D, ANEURALNETWORKS_TENSOR_QUANT8_ASYMM);
}

void spaceDepthOpTest(ANeuralNetworksOperationType operationCode, int32_t operandCode) {
    uint32_t inputDimensions[4] = {2, 2, 2, 2};
    ANeuralNetworksOperandType input = {.type = operandCode,
                                        .dimensionCount = 4,
                                        .dimensions = inputDimensions,
                                        .scale = 0.0f,
                                        .zeroPoint = 0};
    if (operandCode == ANEURALNETWORKS_TENSOR_QUANT8_ASYMM) {
        input.scale = 1.f / 256;
    }

    ANeuralNetworksOperandType block_size = {.type = ANEURALNETWORKS_INT32,
                                             .dimensionCount = 0,
                                             .dimensions = nullptr,
                                             .scale = 0.0f,
                                             .zeroPoint = 0};

    ANeuralNetworksOperandType output = input;
    OperationTestBase spaceDepthTest(operationCode, {input, block_size}, {output});

    EXPECT_TRUE(spaceDepthTest.testMutatingInputOperandCode());
    EXPECT_TRUE(spaceDepthTest.testMutatingInputOperandCounts());
    EXPECT_TRUE(spaceDepthTest.testMutatingOutputOperandCode());
    EXPECT_TRUE(spaceDepthTest.testMutatingOutputOperandCounts());
}

TEST(OperationValidationTest, SPACE_TO_DEPTH_float32) {
    spaceDepthOpTest(ANEURALNETWORKS_SPACE_TO_DEPTH, ANEURALNETWORKS_TENSOR_FLOAT32);
}

TEST(OperationValidationTest, DEPTH_TO_SPACE_float32) {
    spaceDepthOpTest(ANEURALNETWORKS_DEPTH_TO_SPACE, ANEURALNETWORKS_TENSOR_FLOAT32);
}

TEST(OperationValidationTest, SPACE_TO_DEPTH_quant8) {
    spaceDepthOpTest(ANEURALNETWORKS_SPACE_TO_DEPTH, ANEURALNETWORKS_TENSOR_QUANT8_ASYMM);
}

TEST(OperationValidationTest, DEPTH_TO_SPACE_quant8) {
    spaceDepthOpTest(ANEURALNETWORKS_DEPTH_TO_SPACE, ANEURALNETWORKS_TENSOR_QUANT8_ASYMM);
}

void spaceBatchOpTest(ANeuralNetworksOperationType operationCode, int32_t operandCode) {
    uint32_t inputDimensions[4] = {2, 2, 2, 2};
    ANeuralNetworksOperandType input = {.type = operandCode,
                                        .dimensionCount = 4,
                                        .dimensions = inputDimensions,
                                        .scale = 0.0f,
                                        .zeroPoint = 0};
    if (operandCode == ANEURALNETWORKS_TENSOR_QUANT8_ASYMM) {
        input.scale = 1.f / 256;
    }

    uint32_t blockDimensions[1] = {2};
    ANeuralNetworksOperandType blockShape = {.type = ANEURALNETWORKS_TENSOR_INT32,
                                             .dimensionCount = 1,
                                             .dimensions = blockDimensions,
                                             .scale = 0.0f,
                                             .zeroPoint = 0};

    ANeuralNetworksOperandType padding = blockShape;
    ANeuralNetworksOperandType output = input;
    if (operationCode == ANEURALNETWORKS_SPACE_TO_BATCH_ND) {
        OperationTestBase spaceBatchTest(operationCode, {input, blockShape, padding}, {output});

        EXPECT_TRUE(spaceBatchTest.testMutatingInputOperandCode());
        EXPECT_TRUE(spaceBatchTest.testMutatingInputOperandCounts());
        EXPECT_TRUE(spaceBatchTest.testMutatingOutputOperandCode());
        EXPECT_TRUE(spaceBatchTest.testMutatingOutputOperandCounts());
    } else {
        OperationTestBase spaceBatchTest(operationCode, {input, blockShape}, {output});

        EXPECT_TRUE(spaceBatchTest.testMutatingInputOperandCode());
        EXPECT_TRUE(spaceBatchTest.testMutatingInputOperandCounts());
        EXPECT_TRUE(spaceBatchTest.testMutatingOutputOperandCode());
        EXPECT_TRUE(spaceBatchTest.testMutatingOutputOperandCounts());
    }
}

TEST(OperationValidationTest, SPACE_TO_BATCH_ND_float32) {
    spaceBatchOpTest(ANEURALNETWORKS_SPACE_TO_BATCH_ND, ANEURALNETWORKS_TENSOR_FLOAT32);
}

TEST(OperationValidationTest, BATCH_TO_SPACE_ND_float32) {
    spaceBatchOpTest(ANEURALNETWORKS_BATCH_TO_SPACE_ND, ANEURALNETWORKS_TENSOR_FLOAT32);
}

TEST(OperationValidationTest, SPACE_TO_BATCH_ND_quant8) {
    spaceBatchOpTest(ANEURALNETWORKS_SPACE_TO_BATCH_ND, ANEURALNETWORKS_TENSOR_QUANT8_ASYMM);
}

TEST(OperationValidationTest, BATCH_TO_SPACE_ND_quant8) {
    spaceBatchOpTest(ANEURALNETWORKS_BATCH_TO_SPACE_ND, ANEURALNETWORKS_TENSOR_QUANT8_ASYMM);
}

void transposeAndSqueezeOpTest(ANeuralNetworksOperationType operationCode, int32_t operandCode) {
    uint32_t inputDimensions[4] = {2, 2, 2, 2};
    ANeuralNetworksOperandType input = {.type = operandCode,
                                        .dimensionCount = 4,
                                        .dimensions = inputDimensions,
                                        .scale = 0.0f,
                                        .zeroPoint = 0};
    if (operandCode == ANEURALNETWORKS_TENSOR_QUANT8_ASYMM) {
        input.scale = 1.f / 256;
    }

    uint32_t blockDimensions[1] = {4};
    ANeuralNetworksOperandType dims = {.type = ANEURALNETWORKS_TENSOR_INT32,
                                       .dimensionCount = 1,
                                       .dimensions = blockDimensions,
                                       .scale = 0.0f,
                                       .zeroPoint = 0};

    ANeuralNetworksOperandType output = input;
    OperationTestBase transposeAndSqueezeTest(operationCode, {input, dims}, {output});

    EXPECT_TRUE(transposeAndSqueezeTest.testMutatingInputOperandCode());
    EXPECT_TRUE(transposeAndSqueezeTest.testMutatingInputOperandCounts());
    EXPECT_TRUE(transposeAndSqueezeTest.testMutatingOutputOperandCode());
    EXPECT_TRUE(transposeAndSqueezeTest.testMutatingOutputOperandCounts());
}

TEST(OperationValidationTest, TRANSPOSE_float32) {
    transposeAndSqueezeOpTest(ANEURALNETWORKS_TRANSPOSE, ANEURALNETWORKS_TENSOR_FLOAT32);
}

TEST(OperationValidationTest, SQUEEZE_float32) {
    transposeAndSqueezeOpTest(ANEURALNETWORKS_SQUEEZE, ANEURALNETWORKS_TENSOR_FLOAT32);
}

TEST(OperationValidationTest, TRANSPOSE_quant8) {
    transposeAndSqueezeOpTest(ANEURALNETWORKS_TRANSPOSE, ANEURALNETWORKS_TENSOR_QUANT8_ASYMM);
}

TEST(OperationValidationTest, SQUEEZE_quant8) {
    transposeAndSqueezeOpTest(ANEURALNETWORKS_SQUEEZE, ANEURALNETWORKS_TENSOR_QUANT8_ASYMM);
}

void convOpTest(int32_t operandCode) {
    uint32_t inputDimensions[4] = {2, 4, 4, 2};
    ANeuralNetworksOperandType input = {.type = operandCode,
                                        .dimensionCount = 4,
                                        .dimensions = inputDimensions,
                                        .scale = 0.0f,
                                        .zeroPoint = 0};
    if (operandCode == ANEURALNETWORKS_TENSOR_QUANT8_ASYMM) {
        input.scale = 0.5f;
    }

    ANeuralNetworksOperandType filter = input;
    ANeuralNetworksOperandType output = input;

    uint32_t biasDimensions[1] = {2};
    ANeuralNetworksOperandType bias = {.type = operandCode,
                                       .dimensionCount = 1,
                                       .dimensions = biasDimensions,
                                       .scale = 0.0f,
                                       .zeroPoint = 0};
    if (operandCode == ANEURALNETWORKS_TENSOR_QUANT8_ASYMM) {
        bias.type = ANEURALNETWORKS_TENSOR_INT32;
        bias.scale = 0.25f;
    }

    ANeuralNetworksOperandType scalar = {.type = ANEURALNETWORKS_INT32,
                                         .dimensionCount = 0,
                                         .dimensions = nullptr,
                                         .scale = 0.0f,
                                         .zeroPoint = 0};
    ANeuralNetworksOperandType padLeft = scalar;
    ANeuralNetworksOperandType padRight = scalar;
    ANeuralNetworksOperandType padTop = scalar;
    ANeuralNetworksOperandType padBottom = scalar;
    ANeuralNetworksOperandType strideWidth = scalar;
    ANeuralNetworksOperandType strideHeight = scalar;
    ANeuralNetworksOperandType activation = scalar;

    OperationTestBase explicitConvTest(ANEURALNETWORKS_CONV_2D,
                                       {input, filter, bias,
                                        padLeft, padRight, padTop, padBottom,
                                        strideWidth, strideHeight,
                                        activation},
                                       {output});

    EXPECT_TRUE(explicitConvTest.testMutatingInputOperandCode());
    EXPECT_TRUE(explicitConvTest.testMutatingInputOperandCounts());
    EXPECT_TRUE(explicitConvTest.testMutatingOutputOperandCode());
    EXPECT_TRUE(explicitConvTest.testMutatingOutputOperandCounts());

    ANeuralNetworksOperandType padImplicit = scalar;
    OperationTestBase implicitConvTest(ANEURALNETWORKS_CONV_2D,
                                       {input, filter, bias,
                                        padImplicit,
                                        strideWidth, strideHeight,
                                        activation},
                                       {output});

    EXPECT_TRUE(implicitConvTest.testMutatingInputOperandCode());
    EXPECT_TRUE(implicitConvTest.testMutatingInputOperandCounts());
    EXPECT_TRUE(implicitConvTest.testMutatingOutputOperandCode());
    EXPECT_TRUE(implicitConvTest.testMutatingOutputOperandCounts());
}

TEST(OperationValidationTest, CONV_2D_float32) {
    convOpTest(ANEURALNETWORKS_TENSOR_FLOAT32);
}

TEST(OperationValidationTest, CONV_2D_quant8) {
    convOpTest(ANEURALNETWORKS_TENSOR_QUANT8_ASYMM);
}

void depthwiseConvOpTest(int32_t operandCode) {
    uint32_t inputDimensions[4] = {1, 2, 2, 2};
    ANeuralNetworksOperandType input = {.type = operandCode,
                                        .dimensionCount = 4,
                                        .dimensions = inputDimensions,
                                        .scale = 0.0f,
                                        .zeroPoint = 0};
    if (operandCode == ANEURALNETWORKS_TENSOR_QUANT8_ASYMM) {
        input.scale = 0.5f;
    }

    ANeuralNetworksOperandType filter = input;
    ANeuralNetworksOperandType output = input;

    uint32_t biasDimensions[1] = {2};
    ANeuralNetworksOperandType bias = {.type = operandCode,
                                       .dimensionCount = 1,
                                       .dimensions = biasDimensions,
                                       .scale = 0.0f,
                                       .zeroPoint = 0};
    if (operandCode == ANEURALNETWORKS_TENSOR_QUANT8_ASYMM) {
        bias.type = ANEURALNETWORKS_TENSOR_INT32;
        bias.scale = 0.25f;
    }

    ANeuralNetworksOperandType scalar = {.type = ANEURALNETWORKS_INT32,
                                         .dimensionCount = 0,
                                         .dimensions = nullptr,
                                         .scale = 0.0f,
                                         .zeroPoint = 0};
    ANeuralNetworksOperandType padLeft = scalar;
    ANeuralNetworksOperandType padRight = scalar;
    ANeuralNetworksOperandType padTop = scalar;
    ANeuralNetworksOperandType padBottom = scalar;
    ANeuralNetworksOperandType strideWidth = scalar;
    ANeuralNetworksOperandType strideHeight = scalar;
    ANeuralNetworksOperandType multiplier = scalar;
    ANeuralNetworksOperandType activation = scalar;

    OperationTestBase explicitDepthwiseConvTest(ANEURALNETWORKS_DEPTHWISE_CONV_2D,
                                                {input, filter, bias,
                                                 padLeft, padRight, padTop, padBottom,
                                                 strideWidth, strideHeight,
                                                 multiplier, activation},
                                                {output});

    EXPECT_TRUE(explicitDepthwiseConvTest.testMutatingInputOperandCode());
    EXPECT_TRUE(explicitDepthwiseConvTest.testMutatingInputOperandCounts());
    EXPECT_TRUE(explicitDepthwiseConvTest.testMutatingOutputOperandCode());
    EXPECT_TRUE(explicitDepthwiseConvTest.testMutatingOutputOperandCounts());

    ANeuralNetworksOperandType padImplicit = scalar;
    OperationTestBase implicitDepthwiseConvTest(ANEURALNETWORKS_DEPTHWISE_CONV_2D,
                                                {input, filter, bias,
                                                 padImplicit,
                                                 strideWidth, strideHeight,
                                                 multiplier, activation},
                                                {output});

    EXPECT_TRUE(implicitDepthwiseConvTest.testMutatingInputOperandCode());
    EXPECT_TRUE(implicitDepthwiseConvTest.testMutatingInputOperandCounts());
    EXPECT_TRUE(implicitDepthwiseConvTest.testMutatingOutputOperandCode());
    EXPECT_TRUE(implicitDepthwiseConvTest.testMutatingOutputOperandCounts());
}

TEST(OperationValidationTest, DEPTHWISE_CONV_2D_float32) {
    depthwiseConvOpTest(ANEURALNETWORKS_TENSOR_FLOAT32);
}

TEST(OperationValidationTest, DEPTHWISE_CONV_2D_quant8) {
    depthwiseConvOpTest(ANEURALNETWORKS_TENSOR_QUANT8_ASYMM);
}

void fullyConnectedOpTest(int32_t operandCode) {
    uint32_t inputDimensions[2] = {5, 5};
    ANeuralNetworksOperandType input = {.type = operandCode,
                                        .dimensionCount = 2,
                                        .dimensions = inputDimensions,
                                        .scale = 0.0f,
                                        .zeroPoint = 0};
    if (operandCode == ANEURALNETWORKS_TENSOR_QUANT8_ASYMM) {
        input.scale = 0.5f;
    }

    ANeuralNetworksOperandType weights = input;
    ANeuralNetworksOperandType output = input;

    uint32_t biasDimensions[1] = {5};
    ANeuralNetworksOperandType bias = {.type = operandCode,
                                       .dimensionCount = 1,
                                       .dimensions = biasDimensions,
                                       .scale = 0.0f,
                                       .zeroPoint = 0};
    if (operandCode == ANEURALNETWORKS_TENSOR_QUANT8_ASYMM) {
        bias.type = ANEURALNETWORKS_TENSOR_INT32;
        bias.scale = 0.25f;
    }

    ANeuralNetworksOperandType activation = {.type = ANEURALNETWORKS_INT32,
                                             .dimensionCount = 0,
                                             .dimensions = nullptr,
                                             .scale = 0.0f,
                                             .zeroPoint = 0};

    OperationTestBase fullyConnectedTest(ANEURALNETWORKS_FULLY_CONNECTED,
                                         {input, weights, bias, activation},
                                         {output});

    EXPECT_TRUE(fullyConnectedTest.testMutatingInputOperandCode());
    EXPECT_TRUE(fullyConnectedTest.testMutatingInputOperandCounts());
    EXPECT_TRUE(fullyConnectedTest.testMutatingOutputOperandCode());
    EXPECT_TRUE(fullyConnectedTest.testMutatingOutputOperandCounts());
}

TEST(OperationValidationTest, FULLY_CONNECTED_float32) {
    fullyConnectedOpTest(ANEURALNETWORKS_TENSOR_FLOAT32);
}

TEST(OperationValidationTest, FULLY_CONNECTED_quant8) {
    fullyConnectedOpTest(ANEURALNETWORKS_TENSOR_QUANT8_ASYMM);
}

void concatenationTest(int32_t operandCode) {
    uint32_t inputDimensions[2] = {5, 5};
    ANeuralNetworksOperandType input1 = {.type = operandCode,
                                        .dimensionCount = 2,
                                        .dimensions = inputDimensions,
                                        .scale = 0.0f,
                                        .zeroPoint = 0};
    if (operandCode == ANEURALNETWORKS_TENSOR_QUANT8_ASYMM) {
        input1.scale = 0.5f;
    }
    ANeuralNetworksOperandType input2 = input1;
    ANeuralNetworksOperandType output = input1;

    ANeuralNetworksOperandType activation = {.type = ANEURALNETWORKS_INT32,
                                             .dimensionCount = 0,
                                             .dimensions = nullptr,
                                             .scale = 0.0f,
                                             .zeroPoint = 0};

    OperationTestBase concat2Test(ANEURALNETWORKS_CONCATENATION,
                                  {input1, input2, activation}, {output});

    EXPECT_TRUE(concat2Test.testMutatingInputOperandCode());
    EXPECT_TRUE(concat2Test.testMutatingOutputOperandCode());
    EXPECT_TRUE(concat2Test.testMutatingOutputOperandCounts());

    OperationTestBase concat1Test(ANEURALNETWORKS_CONCATENATION,
                                  {input1, activation}, {output});

    EXPECT_TRUE(concat1Test.testMutatingInputOperandCode());
    EXPECT_TRUE(concat1Test.testMutatingOutputOperandCode());
    EXPECT_TRUE(concat1Test.testMutatingOutputOperandCounts());
}

TEST(OperationValidationTest, CONCATENATION_float32) {
    concatenationTest(ANEURALNETWORKS_TENSOR_FLOAT32);
}

TEST(OperationValidationTest, CONCATENATION_quant8) {
    concatenationTest(ANEURALNETWORKS_TENSOR_QUANT8_ASYMM);
}

TEST(OperationValidationTest, RESIZE_BILINEAR_float32) {
    uint32_t inputDimensions[4] = {2, 2, 2, 2};
    ANeuralNetworksOperandType input = {.type = ANEURALNETWORKS_TENSOR_FLOAT32,
                                        .dimensionCount = 4,
                                        .dimensions = inputDimensions,
                                        .scale = 0.0f,
                                        .zeroPoint = 0};
    ANeuralNetworksOperandType height = {.type = ANEURALNETWORKS_INT32,
                                         .dimensionCount = 0,
                                         .dimensions = nullptr,
                                         .scale = 0.0f,
                                         .zeroPoint = 0};
    ANeuralNetworksOperandType width = height;
    ANeuralNetworksOperandType output = input;
    OperationTestBase resizeTest(ANEURALNETWORKS_RESIZE_BILINEAR,
                                 {input, height, width}, {output});

    EXPECT_TRUE(resizeTest.testMutatingInputOperandCode());
    EXPECT_TRUE(resizeTest.testMutatingInputOperandCounts());
    EXPECT_TRUE(resizeTest.testMutatingOutputOperandCode());
    EXPECT_TRUE(resizeTest.testMutatingOutputOperandCounts());
}

void embeddingLookupTest(int32_t operandCode) {
    uint32_t lookupDimensions[1] = {5};
    ANeuralNetworksOperandType lookup = {.type = ANEURALNETWORKS_TENSOR_INT32,
                                         .dimensionCount = 1,
                                         .dimensions = lookupDimensions,
                                         .scale = 0.0f,
                                         .zeroPoint = 0};

    uint32_t inputDimensions[2] = {5, 5};
    ANeuralNetworksOperandType input = {.type = operandCode,
                                        .dimensionCount = 2,
                                        .dimensions = inputDimensions,
                                        .scale = 0.0f,
                                        .zeroPoint = 0};
    if (operandCode == ANEURALNETWORKS_TENSOR_QUANT8_ASYMM) {
        input.scale = 0.5f;
    }
    ANeuralNetworksOperandType output = input;

    OperationTestBase embedLookupTest(ANEURALNETWORKS_EMBEDDING_LOOKUP,
                                      {lookup, input}, {output});

    EXPECT_TRUE(embedLookupTest.testMutatingInputOperandCode());
    EXPECT_TRUE(embedLookupTest.testMutatingInputOperandCounts());
    EXPECT_TRUE(embedLookupTest.testMutatingOutputOperandCode());
    EXPECT_TRUE(embedLookupTest.testMutatingOutputOperandCounts());
}

TEST(OperationValidationTest, EMBEDDING_LOOKUP_float32) {
    embeddingLookupTest(ANEURALNETWORKS_TENSOR_FLOAT32);
}

TEST(OperationValidationTest, EMBEDDING_LOOKUP_quant8) {
    embeddingLookupTest(ANEURALNETWORKS_TENSOR_QUANT8_ASYMM);
}

void hashtableLookupTest(int32_t operandCode) {
    uint32_t lookupDimensions[1] = {5};
    ANeuralNetworksOperandType lookup = {.type = ANEURALNETWORKS_TENSOR_INT32,
                                         .dimensionCount = 1,
                                         .dimensions = lookupDimensions,
                                         .scale = 0.0f,
                                         .zeroPoint = 0};
    ANeuralNetworksOperandType keys = lookup;

    uint32_t valuesDimensions[2] = {5, 5};
    ANeuralNetworksOperandType values = {.type = operandCode,
                                         .dimensionCount = 2,
                                         .dimensions = valuesDimensions,
                                         .scale = 0.0f,
                                         .zeroPoint = 0};
    if (operandCode == ANEURALNETWORKS_TENSOR_QUANT8_ASYMM) {
        values.scale = 0.5f;
    }
    ANeuralNetworksOperandType output = values;

    ANeuralNetworksOperandType hits = lookup;
    hits.type = ANEURALNETWORKS_TENSOR_QUANT8_ASYMM;
    hits.scale = 1.0f;

    OperationTestBase hashLookupTest(ANEURALNETWORKS_HASHTABLE_LOOKUP,
                                      {lookup, keys, values}, {output, hits});

    EXPECT_TRUE(hashLookupTest.testMutatingInputOperandCode());
    EXPECT_TRUE(hashLookupTest.testMutatingInputOperandCounts());
    EXPECT_TRUE(hashLookupTest.testMutatingOutputOperandCode());
    EXPECT_TRUE(hashLookupTest.testMutatingOutputOperandCounts());
}

TEST(OperationValidationTest, HASHTABLE_LOOKUP_float32) {
    hashtableLookupTest(ANEURALNETWORKS_TENSOR_FLOAT32);
}

TEST(OperationValidationTest, HASHTABLE_LOOKUP_quant8) {
    hashtableLookupTest(ANEURALNETWORKS_TENSOR_QUANT8_ASYMM);
}

void lshProjectionTest(int32_t operandCode) {
    uint32_t inputDimensions[2] = {5, 5};
    ANeuralNetworksOperandType hash = {.type = ANEURALNETWORKS_TENSOR_FLOAT32,
                                       .dimensionCount = 2,
                                       .dimensions = inputDimensions,
                                       .scale = 0.0f,
                                       .zeroPoint = 0};

    ANeuralNetworksOperandType input = hash;
    input.type = operandCode;
    if (operandCode == ANEURALNETWORKS_TENSOR_QUANT8_ASYMM) {
        input.scale = 0.5f;
    }

    uint32_t weightDimensions[1] = {5};
    ANeuralNetworksOperandType weight = {.type = ANEURALNETWORKS_TENSOR_FLOAT32,
                                         .dimensionCount = 1,
                                         .dimensions = weightDimensions,
                                         .scale = 0.0f,
                                         .zeroPoint = 0};

    ANeuralNetworksOperandType type = {.type = ANEURALNETWORKS_INT32,
                                       .dimensionCount = 0,
                                       .dimensions = nullptr,
                                       .scale = 0.0f,
                                       .zeroPoint = 0};

    ANeuralNetworksOperandType output = weight;
    output.type = ANEURALNETWORKS_TENSOR_INT32;

    OperationTestBase lshProjTest(ANEURALNETWORKS_LSH_PROJECTION,
                                  {hash, input, weight, type}, {output});

    EXPECT_TRUE(lshProjTest.testMutatingInputOperandCode());
    EXPECT_TRUE(lshProjTest.testMutatingInputOperandCounts());
    EXPECT_TRUE(lshProjTest.testMutatingOutputOperandCode());
    EXPECT_TRUE(lshProjTest.testMutatingOutputOperandCounts());
}

TEST(OperationValidationTest, LSH_PROJECTION_float32) {
    lshProjectionTest(ANEURALNETWORKS_TENSOR_FLOAT32);
}

TEST(OperationValidationTest, LSH_PROJECTION_quant8) {
    lshProjectionTest(ANEURALNETWORKS_TENSOR_QUANT8_ASYMM);
}

TEST(OperationValidationTest, LSTM_float32) {
    uint32_t oneDimensional[1] = {5};
    uint32_t twoDimensional[2] = {5, 5};
    ANeuralNetworksOperandType floatTensor1D = {.type = ANEURALNETWORKS_TENSOR_FLOAT32,
                                                .dimensionCount = 1,
                                                .dimensions = oneDimensional,
                                                .scale = 0.0f,
                                                .zeroPoint = 0};
    ANeuralNetworksOperandType floatTensor2D = {.type = ANEURALNETWORKS_TENSOR_FLOAT32,
                                                .dimensionCount = 2,
                                                .dimensions = twoDimensional,
                                                .scale = 0.0f,
                                                .zeroPoint = 0};
    ANeuralNetworksOperandType intScalar = {.type = ANEURALNETWORKS_INT32,
                                            .dimensionCount = 0,
                                            .dimensions = nullptr,
                                            .scale = 0.0f,
                                            .zeroPoint = 0};
    ANeuralNetworksOperandType floatScalar = {.type = ANEURALNETWORKS_FLOAT32,
                                              .dimensionCount = 0,
                                              .dimensions = nullptr,
                                              .scale = 0.0f,
                                              .zeroPoint = 0};

    ANeuralNetworksOperandType input = floatTensor2D;
    ANeuralNetworksOperandType inputToInput = floatTensor2D;
    ANeuralNetworksOperandType inputToForget = floatTensor2D;
    ANeuralNetworksOperandType inputToCell = floatTensor2D;
    ANeuralNetworksOperandType inputToOutput = floatTensor2D;
    ANeuralNetworksOperandType recurrentToInput = floatTensor2D;
    ANeuralNetworksOperandType recurrentToForget = floatTensor2D;
    ANeuralNetworksOperandType recurrentToCell = floatTensor2D;
    ANeuralNetworksOperandType recurrentToOutput = floatTensor2D;
    ANeuralNetworksOperandType cellToInput = floatTensor1D;
    ANeuralNetworksOperandType cellToForget = floatTensor1D;
    ANeuralNetworksOperandType cellToOutput = floatTensor1D;
    ANeuralNetworksOperandType inputGateBias = floatTensor1D;
    ANeuralNetworksOperandType forgetGateBias = floatTensor1D;
    ANeuralNetworksOperandType cellBias = floatTensor1D;
    ANeuralNetworksOperandType outputGateBias = floatTensor1D;
    ANeuralNetworksOperandType projWeights = floatTensor2D;
    ANeuralNetworksOperandType projBias = floatTensor1D;
    ANeuralNetworksOperandType outputStateIn = floatTensor2D;
    ANeuralNetworksOperandType cellStateIn = floatTensor2D;
    ANeuralNetworksOperandType activation = intScalar;
    ANeuralNetworksOperandType clipCellState = floatScalar;
    ANeuralNetworksOperandType clipProjLayer = floatScalar;

    ANeuralNetworksOperandType scratch = floatTensor2D;
    ANeuralNetworksOperandType outputStateOut = floatTensor2D;
    ANeuralNetworksOperandType cellStateOut = floatTensor2D;
    ANeuralNetworksOperandType output = floatTensor2D;

    OperationTestBase lstmTest(ANEURALNETWORKS_LSTM,
        {input, inputToInput, inputToForget, inputToCell, inputToOutput, recurrentToInput,
         recurrentToForget, recurrentToCell, recurrentToOutput, cellToInput, cellToForget,
         cellToOutput, inputGateBias, forgetGateBias, cellBias, outputGateBias, projWeights,
         projBias, outputStateIn, cellStateIn, activation, clipCellState, clipProjLayer},
        {scratch, outputStateOut, cellStateOut, output});

    EXPECT_TRUE(lstmTest.testMutatingInputOperandCode());
    EXPECT_TRUE(lstmTest.testMutatingInputOperandCounts());
    EXPECT_TRUE(lstmTest.testMutatingOutputOperandCode());
    EXPECT_TRUE(lstmTest.testMutatingOutputOperandCounts());
}

TEST(OperationValidationTest, RNN_float32) {
    uint32_t oneDimensional[1] = {5};
    uint32_t twoDimensional[2] = {5, 5};
    ANeuralNetworksOperandType floatTensor1D = {.type = ANEURALNETWORKS_TENSOR_FLOAT32,
                                                .dimensionCount = 1,
                                                .dimensions = oneDimensional,
                                                .scale = 0.0f,
                                                .zeroPoint = 0};
    ANeuralNetworksOperandType floatTensor2D = {.type = ANEURALNETWORKS_TENSOR_FLOAT32,
                                                .dimensionCount = 2,
                                                .dimensions = twoDimensional,
                                                .scale = 0.0f,
                                                .zeroPoint = 0};
    ANeuralNetworksOperandType intScalar = {.type = ANEURALNETWORKS_INT32,
                                            .dimensionCount = 0,
                                            .dimensions = nullptr,
                                            .scale = 0.0f,
                                            .zeroPoint = 0};

    ANeuralNetworksOperandType input = floatTensor2D;
    ANeuralNetworksOperandType weights = floatTensor2D;
    ANeuralNetworksOperandType recurrentWeights = floatTensor2D;
    ANeuralNetworksOperandType bias = floatTensor1D;
    ANeuralNetworksOperandType hiddenStateIn = floatTensor2D;
    ANeuralNetworksOperandType activation = intScalar;

    ANeuralNetworksOperandType hiddenStateOut = floatTensor2D;
    ANeuralNetworksOperandType output = floatTensor2D;

    OperationTestBase rnnTest(ANEURALNETWORKS_RNN,
                              {input, weights, recurrentWeights, bias, hiddenStateIn, activation},
                              {hiddenStateOut, output});

    EXPECT_TRUE(rnnTest.testMutatingInputOperandCode());
    EXPECT_TRUE(rnnTest.testMutatingInputOperandCounts());
    EXPECT_TRUE(rnnTest.testMutatingOutputOperandCode());
    EXPECT_TRUE(rnnTest.testMutatingOutputOperandCounts());
}

TEST(OperationValidationTest, SVDF_float32) {
    uint32_t oneDimensional[1] = {5};
    uint32_t twoDimensional[2] = {5, 5};
    ANeuralNetworksOperandType floatTensor1D = {.type = ANEURALNETWORKS_TENSOR_FLOAT32,
                                                .dimensionCount = 1,
                                                .dimensions = oneDimensional,
                                                .scale = 0.0f,
                                                .zeroPoint = 0};
    ANeuralNetworksOperandType floatTensor2D = {.type = ANEURALNETWORKS_TENSOR_FLOAT32,
                                                .dimensionCount = 2,
                                                .dimensions = twoDimensional,
                                                .scale = 0.0f,
                                                .zeroPoint = 0};
    ANeuralNetworksOperandType intScalar = {.type = ANEURALNETWORKS_INT32,
                                            .dimensionCount = 0,
                                            .dimensions = nullptr,
                                            .scale = 0.0f,
                                            .zeroPoint = 0};

    ANeuralNetworksOperandType input = floatTensor2D;
    ANeuralNetworksOperandType weightsFeature = floatTensor2D;
    ANeuralNetworksOperandType weightsTime = floatTensor2D;
    ANeuralNetworksOperandType bias = floatTensor1D;
    ANeuralNetworksOperandType stateIn = floatTensor2D;
    ANeuralNetworksOperandType rank = intScalar;
    ANeuralNetworksOperandType activation = intScalar;

    ANeuralNetworksOperandType stateOut = floatTensor2D;
    ANeuralNetworksOperandType output = floatTensor2D;

    OperationTestBase svdfTest(ANEURALNETWORKS_SVDF,
        {input, weightsFeature, weightsTime, bias, stateIn, rank, activation},
        {stateOut, output});

    EXPECT_TRUE(svdfTest.testMutatingInputOperandCode());
    EXPECT_TRUE(svdfTest.testMutatingInputOperandCounts());
    EXPECT_TRUE(svdfTest.testMutatingOutputOperandCode());
    EXPECT_TRUE(svdfTest.testMutatingOutputOperandCounts());
}

void stridedSliceOpTest(int32_t operandCode) {
    uint32_t inputDimensions[2] = {5, 5};
    ANeuralNetworksOperandType input = {.type = operandCode,
                                        .dimensionCount = 2,
                                        .dimensions = inputDimensions,
                                        .scale = 0.0f,
                                        .zeroPoint = 0};
    if (operandCode == ANEURALNETWORKS_TENSOR_QUANT8_ASYMM) {
        input.scale = 0.5f;
    }
    ANeuralNetworksOperandType output = input;

    uint32_t beginsDimensions[1] = {2};
    ANeuralNetworksOperandType begins = {.type = ANEURALNETWORKS_TENSOR_INT32,
                                         .dimensionCount = 1,
                                         .dimensions = beginsDimensions,
                                         .scale = 0.0f,
                                         .zeroPoint = 0};

    ANeuralNetworksOperandType ends = begins;
    ANeuralNetworksOperandType strides = begins;

    ANeuralNetworksOperandType beginMask = {.type = ANEURALNETWORKS_INT32,
                                            .dimensionCount = 0,
                                            .dimensions = nullptr,
                                            .scale = 0.0f,
                                            .zeroPoint = 0};
    ANeuralNetworksOperandType endMask = beginMask;
    ANeuralNetworksOperandType shrinkAxisMask = beginMask;

    OperationTestBase stridedSliceTest(ANEURALNETWORKS_STRIDED_SLICE,
                                       {input, begins, ends, strides,
                                        beginMask, endMask, shrinkAxisMask},
                                       {output});

    EXPECT_TRUE(stridedSliceTest.testMutatingInputOperandCode());
    EXPECT_TRUE(stridedSliceTest.testMutatingInputOperandCounts());
    EXPECT_TRUE(stridedSliceTest.testMutatingOutputOperandCode());
    EXPECT_TRUE(stridedSliceTest.testMutatingOutputOperandCounts());
}

TEST(OperationValidationTest, STRIDED_SLICE_float32) {
    stridedSliceOpTest(ANEURALNETWORKS_TENSOR_FLOAT32);
}

TEST(OperationValidationTest, STRIDED_SLICE_quant8) {
    stridedSliceOpTest(ANEURALNETWORKS_TENSOR_QUANT8_ASYMM);
}

}  // end namespace
