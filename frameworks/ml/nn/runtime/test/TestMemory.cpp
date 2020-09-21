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

#include "TestMemory.h"

#include "NeuralNetworksWrapper.h"

#include <gtest/gtest.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>

using WrapperCompilation = ::android::nn::wrapper::Compilation;
using WrapperExecution = ::android::nn::wrapper::Execution;
using WrapperMemory = ::android::nn::wrapper::Memory;
using WrapperModel = ::android::nn::wrapper::Model;
using WrapperOperandType = ::android::nn::wrapper::OperandType;
using WrapperResult = ::android::nn::wrapper::Result;
using WrapperType = ::android::nn::wrapper::Type;

namespace {

// Tests the various ways to pass weights and input/output data.
class MemoryTest : public ::testing::Test {
protected:
    void SetUp() override {}

};

TEST_F(MemoryTest, TestFd) {
    // Create a file that contains matrix2 and matrix3.
    char path[] = "/data/local/tmp/TestMemoryXXXXXX";
    int fd = mkstemp(path);
    const uint32_t offsetForMatrix2 = 20;
    const uint32_t offsetForMatrix3 = 200;
    static_assert(offsetForMatrix2 + sizeof(matrix2) < offsetForMatrix3, "matrices overlap");
    lseek(fd, offsetForMatrix2, SEEK_SET);
    write(fd, matrix2, sizeof(matrix2));
    lseek(fd, offsetForMatrix3, SEEK_SET);
    write(fd, matrix3, sizeof(matrix3));
    fsync(fd);

    WrapperMemory weights(offsetForMatrix3 + sizeof(matrix3), PROT_READ, fd, 0);
    ASSERT_TRUE(weights.isValid());

    WrapperModel model;
    WrapperOperandType matrixType(WrapperType::TENSOR_FLOAT32, {3, 4});
    WrapperOperandType scalarType(WrapperType::INT32, {});
    int32_t activation(0);
    auto a = model.addOperand(&matrixType);
    auto b = model.addOperand(&matrixType);
    auto c = model.addOperand(&matrixType);
    auto d = model.addOperand(&matrixType);
    auto e = model.addOperand(&matrixType);
    auto f = model.addOperand(&scalarType);

    model.setOperandValueFromMemory(e, &weights, offsetForMatrix2, sizeof(Matrix3x4));
    model.setOperandValueFromMemory(a, &weights, offsetForMatrix3, sizeof(Matrix3x4));
    model.setOperandValue(f, &activation, sizeof(activation));
    model.addOperation(ANEURALNETWORKS_ADD, {a, c, f}, {b});
    model.addOperation(ANEURALNETWORKS_ADD, {b, e, f}, {d});
    model.identifyInputsAndOutputs({c}, {d});
    ASSERT_TRUE(model.isValid());
    model.finish();

    // Test the three node model.
    Matrix3x4 actual;
    memset(&actual, 0, sizeof(actual));
    WrapperCompilation compilation2(&model);
    ASSERT_EQ(compilation2.finish(), WrapperResult::NO_ERROR);
    WrapperExecution execution2(&compilation2);
    ASSERT_EQ(execution2.setInput(0, matrix1, sizeof(Matrix3x4)), WrapperResult::NO_ERROR);
    ASSERT_EQ(execution2.setOutput(0, actual, sizeof(Matrix3x4)), WrapperResult::NO_ERROR);
    ASSERT_EQ(execution2.compute(), WrapperResult::NO_ERROR);
    ASSERT_EQ(CompareMatrices(expected3, actual), 0);

    close(fd);
    unlink(path);
}

}  // end namespace
