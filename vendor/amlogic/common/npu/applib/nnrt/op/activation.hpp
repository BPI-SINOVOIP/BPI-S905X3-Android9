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
#ifndef _OP_ACTIVIATION_H_
#define _OP_ACTIVIATION_H_

#include "nnrt/op/operation.hpp"

namespace nnrt {
namespace op {
template <nnrt::OperationType activationType>
struct ActivationOperation : Operation {
    ActivationOperation() : Operation(activationType) {}

    virtual void handleLayoutInferenceOnInputs(
        nnrt::Model& model,
        std::unordered_map<uint32_t, nnrt::layout_inference::IPermuteVectorPtr>&
            next_permute_vectors) {
        assert(input_permute_cache_.cached_permutes_.size() == 1);
        nnrt::layout_inference::IPermuteVectorPtr permuteVector =
            input_permute_cache_.cached_permutes_[inputs()[0]];
        next_permute_vectors.insert(std::make_pair(outputs()[0], permuteVector));
    }
};

struct TanhOperation : ActivationOperation<OperationType::TANH> {
    float scaleA{1.0f};
    float scaleB{1.0f};
};

struct LeakyReluOperation : ActivationOperation<OperationType::LEAKY_RELU> {
    float ratio{0.1f};
};

using Relu1Operation = ActivationOperation<OperationType::RELU1>;
using Relu6Operation = ActivationOperation<OperationType::RELU6>;
using ReluOperation = ActivationOperation<OperationType::RELU>;
using SigmoidOperation = ActivationOperation<OperationType::SIGMOID>;
using SoftReluOperation = ActivationOperation<OperationType::SOFT_RELU>;
using AbsOperation = ActivationOperation<OperationType::ABS>;
using SqrtOperation = ActivationOperation<OperationType::SQRT>;
using RSqrtOperation = ActivationOperation<OperationType::RSQRT>;
using SquareOperation = ActivationOperation<OperationType::SQUARE>;
using LinearOperation = ActivationOperation<OperationType::LINEAR>;

}
}

#endif
