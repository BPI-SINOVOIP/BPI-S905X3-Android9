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
#ifndef _OP_NORMALIZATION_H_
#define _OP_NORMALIZATION_H_

#include "nnrt/op/operation.hpp"
#include "nnrt/model.hpp"

namespace nnrt {
namespace op {

template <nnrt::OperationType OpType>
struct TNormalization : Operation {
    TNormalization() : Operation(OpType) {}
    virtual void handleLayoutInferenceOnInputs(
        nnrt::Model& model,
        std::unordered_map<uint32_t, nnrt::layout_inference::IPermuteVectorPtr>& out_permute_vectors)
        override {
        assert(input_permute_cache_.cached_permutes_.size() == 1);
        OperandPtr inputOperand = model.operand(inputs()[0]);
        OperandPtr outputOperand = model.operand(outputs()[0]);

        nnrt::layout_inference::IPermuteVectorPtr permuteVector =
            input_permute_cache_.cached_permutes_[inputs()[0]];

        if (inputOperand->ndim() != 4) {
            Operation::handleLayoutInferenceOnInputs(model, out_permute_vectors);
            return;
        }

        // {0, 1, 2, 3}
        auto requiredPermute = layout_inference::make_shared(inputOperand->ndim());
        if (DataLayout::NHWC == getDataLayout()) {
            requiredPermute = std::make_shared<layout_inference::PermuteVector<4>>(
                std::initializer_list<uint32_t>({0, 3, 1, 2}));
        }

        auto finalPermute = permuteVector->reverse()->add(requiredPermute);
        auto permuteOp = nnrt::op::utils::asOp(finalPermute);

        if (permuteOp) {
            insertPermute(model, permuteOp, finalPermute->asStdVec(), true, inputs()[0]);
        }

        out_permute_vectors.insert(std::make_pair(outputs()[0], requiredPermute));
    }
};

struct BatchNormalization : TNormalization<OperationType::BATCH_NORM> {
    BatchNormalization() : TNormalization() {}
    float eps;
};

template <typename DType>
struct InstanceNormOperation : TNormalization<OperationType::INSTANCE_NORM> {
    InstanceNormOperation() : TNormalization() {}
    std::vector<DType> gamma;
    std::vector<DType> beta;
    float eps;
};

template <nnrt::OperationType OpType>
struct TNormWithAxis : Operation {
    TNormWithAxis() : Operation(OpType) {}

    virtual void handleLayoutInferenceOnInputs(
        nnrt::Model& model,
        std::unordered_map<uint32_t, nnrt::layout_inference::IPermuteVectorPtr>& out_permute_vectors)
        override {
        assert(input_permute_cache_.cached_permutes_.size() == 1);
        OperandPtr inputOperand = model.operand(inputs()[0]);
        OperandPtr outputOperand = model.operand(outputs()[0]);

        nnrt::layout_inference::IPermuteVectorPtr permuteVector =
            input_permute_cache_.cached_permutes_[inputs()[0]];

        if (inputOperand->ndim() != 4) {
            auto reversePermVec = permuteVector->reverse();

            auto permuteOp = nnrt::op::utils::asOp(reversePermVec);
            if (permuteOp) {
                insertPermute(model, permuteOp, reversePermVec->asStdVec(), true, inputs()[0]);
            }
            out_permute_vectors.insert(
                std::make_pair(outputs()[0], nnrt::layout_inference::make_shared(outputOperand->ndim())));
            // convert axis to positive number
            if (axis < 0) {
                axis = permuteVector->rank() + axis;
            }

            return;
        }

        // {0, 1, 2, 3}
        auto requiredPermute = layout_inference::make_shared(inputOperand->ndim());
        if (DataLayout::NHWC == getDataLayout()) {
            requiredPermute = std::make_shared<layout_inference::PermuteVector<4>>(
                std::initializer_list<uint32_t>({0, 3, 1, 2}));
        }

        auto finalPermute = permuteVector->reverse()->add(requiredPermute);
        auto permuteOp = nnrt::op::utils::asOp(finalPermute);

        if (permuteOp) {
            insertPermute(model, permuteOp, finalPermute->asStdVec(), true, inputs()[0]);
        }

        // convert axis to positive number
        if (axis < 0) {
            axis = permuteVector->rank() + axis;
        }
        // Convert axis to org platform format
        axis = nnrt::op::utils::axisMapTo(finalPermute, axis);

        out_permute_vectors.insert(std::make_pair(outputs()[0], requiredPermute));
    }

    int32_t axis;
};

using L2NormOperation = TNormWithAxis<nnrt::OperationType::L2_NORM>;

struct LocalResponseNormOperation : TNormWithAxis<OperationType::LOCAL_RESPONSE_NORM> {
    LocalResponseNormOperation() : TNormWithAxis() {}

    int32_t radius;
    float bias;
    float scale;     // alpha
    float exponent;  // beta
    /// Normalization channel algorithm to use (Across, Within).
    NormalizationAlgorithmChannel channelType{NormalizationAlgorithmChannel::Across};
    /// Normalization method algorithm to use (LocalBrightness, LocalContrast).
    NormalizationAlgorithmMethod methodType{NormalizationAlgorithmMethod::LocalBrightness};
};

}
}

#endif
