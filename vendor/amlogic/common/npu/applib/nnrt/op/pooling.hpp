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
#ifndef _OP_POOLING_H_
#define _OP_POOLING_H_

#include "operation.hpp"
#include "nnrt/model.hpp"
#include "nnrt/logging.hpp"

namespace nnrt {
namespace op {

namespace {
    static const std::string tag = "Pooling";
}

template <nnrt::OperationType OpType>
struct TPoolOperation : Operation {
    TPoolOperation() : Operation(OpType) {}

    virtual void handleLayoutInferenceOnInputs(
        nnrt::Model& model,
        std::unordered_map<uint32_t, nnrt::layout_inference::IPermuteVectorPtr>& out_permute_vectors)
        override {
        assert(input_permute_cache_.cached_permutes_.size() == 1);
        OperandPtr inputOperand = model.operand(inputs()[0]);
        OperandPtr outputOperand = model.operand(outputs()[0]);

        auto permuteVector = input_permute_cache_.cached_permutes_[inputs()[0]];

        if (!permuteVector) {
            NNRT_LOGE(tag)<< "Fatal Error: Input permute vector not setup properly" ;
            assert(false);
        }

        if (inputOperand->ndim() != 4) {
            Operation::handleLayoutInferenceOnInputs(model, out_permute_vectors);
            return;
        }

        // {0, 1, 2, 3}
        auto requiredPermute = nnrt::layout_inference::make_shared(inputOperand->ndim());
        if (DataLayout::NHWC == getDataLayout()) {
            requiredPermute = std::make_shared<nnrt::layout_inference::PermuteVector<4>>(
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

struct AveragePool2DOperation : TPoolOperation<OperationType::AVERAGE_POOL_2D> {
    AveragePool2DOperation() : TPoolOperation() {
        strides.resize(2);
        ksize.resize(2);
        pad.resize(4);
    }

    std::vector<int32_t> strides;
    std::vector<int32_t> ksize;
    std::vector<int32_t> pad;
    PadType padType{PadType::AUTO};
    Rounding roundType{Rounding::FLOOR};
    PoolMode poolMode{PoolMode::VALID};
};

struct MaxPool2DOperation : TPoolOperation<OperationType::MAX_POOL_2D> {
    MaxPool2DOperation() : TPoolOperation() {
        strides.resize(2);
        ksize.resize(2);
        pad.resize(4);
    }

    std::vector<int32_t> strides;
    std::vector<int32_t> ksize;
    std::vector<int32_t> pad;
    PadType padType{PadType::AUTO};
    Rounding roundType{Rounding::FLOOR};
};

struct L2Pool2DOperation : TPoolOperation<OperationType::L2_POOL_2D> {
    L2Pool2DOperation() : TPoolOperation() {
        strides.resize(2);
        ksize.resize(2);
        pad.resize(4);
    }

    std::vector<int32_t> strides;
    std::vector<int32_t> ksize;
    std::vector<int32_t> pad;
    PadType padType{PadType::AUTO};
    Rounding roundType{Rounding::FLOOR};
};

struct Unpool2DOperation : Operation {
    Unpool2DOperation() : Operation(OperationType::UNPOOL_2D) {}
    int output_height;
    int output_width;
};
}
}

#endif
