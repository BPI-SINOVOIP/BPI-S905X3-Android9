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
#include "TestHarness.h"

#include <gtest/gtest.h>

#include <tuple>
#include <vector>

using namespace android::nn::wrapper;
using namespace test_helper;

namespace {

const uint32_t INTENDED_SIZE = 3;
const uint32_t OTHER_SIZE    = 2;
const uint32_t UNKNOWN_SIZE  = 0;
typedef uint8_t IntendedMatrix[INTENDED_SIZE][INTENDED_SIZE];

// TODO: add a float version of this test for use against drivers that don't
// support quantized add. b/72448000

// We test three basic scenarios for each tensor dimension:
//     INTENDED_AT_COMPILE_AND_EXECUTE: set the dimension at compile
//     (addOperand) time to INTENDED_SIZE, use same size at execution
//     (setInput/setOutput) time. This should always work.
//
//     INTENDED_AT_COMPILE_NOT_SET_AT_EXECUTE: set the dimension at compile
//     (addOperand) time to INTENDED_SIZE, give no size at execution time.
//     This should always work.
//
//     UNKNOWN_AT_COMPILE_INTENDED_AT_EXECUTE: don't set the dimension at
//     compile (addOperand) time, use INTENDED_SIZE at execute
//     (setInput/setOutput) time. Note for constants, this just means using an
//     unknown dimension at addOperand as there is no type parameter to
//     setOperandValue. This should work for inputs and outputs and give an
//     error for constants at compile time.
//
//     UNKNOWN_AT_COMPILE_OTHER_AT_EXECUTE: don't set the dimension at compile
//     (addOperand) time, use OTHER_SIZE at execute (setInput/setOutput) time.
//     This should give an error at execute time (as the constant value will
//     have a different size).
enum class DimensionKind { INTENDED_AT_COMPILE_AND_EXECUTE,
                           INTENDED_AT_COMPILE_NOT_SET_AT_EXECUTE,
                           UNKNOWN_AT_COMPILE_INTENDED_AT_EXECUTE,
                           UNKNOWN_AT_COMPILE_OTHER_AT_EXECUTE };
typedef std::tuple<DimensionKind, DimensionKind> OperandParams;
typedef std::tuple<OperandParams,  // first input
                   OperandParams,  // second input
                   OperandParams,  // constant
                   OperandParams   // output
                  > TestParams;
// All relevant combinations of the basic scenarios are then created with TEST_P
auto ioDimensionValues = testing::Values(DimensionKind::INTENDED_AT_COMPILE_AND_EXECUTE,
                                         DimensionKind::INTENDED_AT_COMPILE_NOT_SET_AT_EXECUTE,
                                         DimensionKind::UNKNOWN_AT_COMPILE_INTENDED_AT_EXECUTE,
                                         DimensionKind::UNKNOWN_AT_COMPILE_OTHER_AT_EXECUTE);
auto constantDimensionValues = testing::Values(
        DimensionKind::INTENDED_AT_COMPILE_NOT_SET_AT_EXECUTE,
        DimensionKind::UNKNOWN_AT_COMPILE_INTENDED_AT_EXECUTE);
auto ioValues = testing::Combine(ioDimensionValues, ioDimensionValues);
auto constantValues = testing::Combine(constantDimensionValues, constantDimensionValues);
auto combinedValues = testing::Combine(ioValues, ioValues, constantValues, ioValues);

class UnknownDimensionsTest : public ::testing::TestWithParam<TestParams> {
protected:
    const IntendedMatrix ones = { { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 } };
    const IntendedMatrix twos = { { 2, 2, 2 }, { 2, 2, 2 }, { 2, 2, 2 } };
    const IntendedMatrix fives = { { 5, 5, 5 }, { 5, 5, 5 }, { 5, 5, 5 } };
};

TEST_P(UnknownDimensionsTest, UnknownDimensions) {
    TestParams params = GetParam();
    auto paramsForInput0 = std::get<0>(params),
         paramsForInput1 = std::get<1>(params),
         paramsForConst  = std::get<2>(params),
         paramsForOutput = std::get<3>(params);

    Model model;
    std::string input0Scope("Input 0:"), input1Scope("Input 1:"),
                constantScope("Constant:"), outputScope("Output:");

    auto getDimForCompile = [](DimensionKind kind, std::string* scope) {
        switch (kind) {
            case DimensionKind::INTENDED_AT_COMPILE_AND_EXECUTE:
                if (scope) scope->append(" INTENDED_AT_COMPILE_AND_EXECUTE");
                return INTENDED_SIZE;
            case DimensionKind::INTENDED_AT_COMPILE_NOT_SET_AT_EXECUTE:
                if (scope) scope->append(" INTENDED_AT_COMPILE_NOT_SET_AT_EXECUTE");
                return INTENDED_SIZE;
            case DimensionKind::UNKNOWN_AT_COMPILE_INTENDED_AT_EXECUTE:
                if (scope) scope->append(" UNKNOWN_AT_COMPILE_INTENDED_AT_EXECUTE");
                return UNKNOWN_SIZE;
            case DimensionKind::UNKNOWN_AT_COMPILE_OTHER_AT_EXECUTE:
                if (scope) scope->append(" UNKNOWN_AT_COMPILE_OTHER_AT_EXECUTE");
                return UNKNOWN_SIZE;
        }
    };
    auto addOperand = [&model, &getDimForCompile](OperandParams params,
                                                  std::string* scope = nullptr) {
        OperandType matrixTypeWithPotentiallyUnknownDims(
                Type::TENSOR_QUANT8_ASYMM,
                { getDimForCompile(std::get<0>(params), scope),
                  getDimForCompile(std::get<1>(params), scope) },
                1.0f);
        return model.addOperand(&matrixTypeWithPotentiallyUnknownDims);
    };
    auto inputOpd0 = addOperand(paramsForInput0, &input0Scope);
    auto inputOpd1 = addOperand(paramsForInput1, &input1Scope);
    auto intermediateOpd0 = addOperand(OperandParams{
            // Dimensions for intermediate operand actually deduced at execution time
            DimensionKind::UNKNOWN_AT_COMPILE_INTENDED_AT_EXECUTE,
            DimensionKind::UNKNOWN_AT_COMPILE_INTENDED_AT_EXECUTE});
    auto constantOpd0 = addOperand(paramsForConst, &constantScope);
    auto outputOpd0 = addOperand(paramsForOutput, &outputScope);

    // Make the gtest failure easier to read, TEST_P just outputs a list of
    // numbers
    SCOPED_TRACE(input0Scope);
    SCOPED_TRACE(input1Scope);
    SCOPED_TRACE(constantScope);
    SCOPED_TRACE(outputScope);

    OperandType scalarType(Type::INT32, {});
    int32_t activation(ANEURALNETWORKS_FUSED_NONE);
    auto activationOpd0 = model.addOperand(&scalarType);

    model.setOperandValue(activationOpd0, &activation, sizeof(activation));
    model.setOperandValue(constantOpd0, twos, sizeof(twos));
    model.addOperation(ANEURALNETWORKS_ADD,
                       {inputOpd0, inputOpd1, activationOpd0},
                       {intermediateOpd0});
    model.addOperation(ANEURALNETWORKS_ADD,
                       {intermediateOpd0, constantOpd0, activationOpd0},
                       {outputOpd0});
    model.identifyInputsAndOutputs({inputOpd0, inputOpd1}, {outputOpd0});
    if (std::get<0>(paramsForConst) == DimensionKind::INTENDED_AT_COMPILE_NOT_SET_AT_EXECUTE &&
        std::get<1>(paramsForConst) == DimensionKind::INTENDED_AT_COMPILE_NOT_SET_AT_EXECUTE) {
        ASSERT_TRUE(model.isValid());
        ASSERT_EQ(model.finish(), Result::NO_ERROR);
    } else {
        ASSERT_FALSE(model.isValid());
        // There is no contract (yet) for specific errors in NeuralNetworks.h,
        // so we just assert on not being successful.
        ASSERT_NE(model.finish(), Result::NO_ERROR);
        return;
    }

    Compilation compilation(&model);
    ASSERT_EQ(compilation.finish(), Result::NO_ERROR);

    IntendedMatrix actual = { { 10, 10, 10 }, { 10, 10, 10 }, { 10, 10, 10 } };
    Execution execution(&compilation);

    OperandType matrixTypeIntended(Type::TENSOR_QUANT8_ASYMM, {INTENDED_SIZE, INTENDED_SIZE}, 1.0f);
    OperandType matrixTypeFirstOther(Type::TENSOR_QUANT8_ASYMM, {OTHER_SIZE, INTENDED_SIZE}, 1.0f);
    OperandType matrixTypeSecondOther(Type::TENSOR_QUANT8_ASYMM, {INTENDED_SIZE, OTHER_SIZE}, 1.0f);
    OperandType matrixTypeBothOther(Type::TENSOR_QUANT8_ASYMM, {OTHER_SIZE, OTHER_SIZE}, 1.0f);
    bool allAreIntendedSizeAtExecution = true;

    // Helper to return appropriate "type" parameter to setInput/setOutput based
    // on OperandParams
    auto typeAtSet = [&](OperandParams params) {
        auto first = std::get<0>(params), second = std::get<1>(params);
        if (first == DimensionKind::UNKNOWN_AT_COMPILE_OTHER_AT_EXECUTE &&
            second == DimensionKind::UNKNOWN_AT_COMPILE_OTHER_AT_EXECUTE) {
            allAreIntendedSizeAtExecution = false;
            return &matrixTypeBothOther.operandType;
        } else if (first == DimensionKind::UNKNOWN_AT_COMPILE_OTHER_AT_EXECUTE) {
            allAreIntendedSizeAtExecution = false;
            return &matrixTypeFirstOther.operandType;
        } else if (second == DimensionKind::UNKNOWN_AT_COMPILE_OTHER_AT_EXECUTE) {
            allAreIntendedSizeAtExecution = false;
            return &matrixTypeSecondOther.operandType;
        } else if (first == DimensionKind::INTENDED_AT_COMPILE_AND_EXECUTE &&
                   second == DimensionKind::INTENDED_AT_COMPILE_AND_EXECUTE) {
            return &matrixTypeIntended.operandType;
        } else if (first == DimensionKind::INTENDED_AT_COMPILE_NOT_SET_AT_EXECUTE &&
                   second == DimensionKind::INTENDED_AT_COMPILE_NOT_SET_AT_EXECUTE) {
            return static_cast<ANeuralNetworksOperandType*>(nullptr);
        } else {
            return &matrixTypeIntended.operandType;
        }
    };
    // Helper to return appropriate "size" parameter to setInput/setOutput based
    // on OperandParams
    auto sizeAtSet = [](OperandParams params) {
        auto first = std::get<0>(params), second = std::get<1>(params);
        size_t firstDim = (first == DimensionKind::UNKNOWN_AT_COMPILE_OTHER_AT_EXECUTE) ?
            OTHER_SIZE : INTENDED_SIZE;
        size_t secondDim = (second == DimensionKind::UNKNOWN_AT_COMPILE_OTHER_AT_EXECUTE) ?
            OTHER_SIZE : INTENDED_SIZE;
        return firstDim * secondDim * sizeof(fives[0][0]);
    };
    ASSERT_EQ(execution.setInput(0, ones, sizeAtSet(paramsForInput0), typeAtSet(paramsForInput0)),
              Result::NO_ERROR);
    ASSERT_EQ(execution.setInput(1, twos, sizeAtSet(paramsForInput1), typeAtSet(paramsForInput1)),
              Result::NO_ERROR);
    ASSERT_EQ(execution.setOutput(0, actual, sizeAtSet(paramsForOutput),
                                  typeAtSet(paramsForOutput)),
              Result::NO_ERROR);

    if (allAreIntendedSizeAtExecution) {
        ASSERT_EQ(execution.compute(), Result::NO_ERROR);
    } else {
        // There is no contract (yet) for specific errors in NeuralNetworks.h,
        // so we just assert on not being successful.
        ASSERT_NE(execution.compute(), Result::NO_ERROR);
        return;
    }

    using qvec = std::vector<uint8_t>;
    constexpr size_t count = sizeof(fives) / sizeof(fives[0][0]);
    Quant8Operands expected_opds{{0, qvec{&fives[0][0], &fives[0][0] + count}}};
    Quant8Operands actual_opds{{0, qvec{&actual[0][0], &actual[0][0] + count}}};
    compare(MixedTyped{ {}, {}, expected_opds }, MixedTyped{ {}, {}, actual_opds });
}

INSTANTIATE_TEST_CASE_P(UnknownCombinationsTest, UnknownDimensionsTest,
                        combinedValues);

}  // end namespace
