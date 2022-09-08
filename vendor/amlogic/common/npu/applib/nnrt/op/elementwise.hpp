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
#ifndef _OP_ELEMENTWISE_H_
#define _OP_ELEMENTWISE_H_

#include "nnrt/op/operation.hpp"

namespace nnrt {
namespace op {

struct EltwiseOperation : Operation {
    EltwiseOperation(OperationType type) : Operation(type) {}
    virtual void handleLayoutInferenceOnInputs(
        nnrt::Model& model,
        std::unordered_map<uint32_t, nnrt::layout_inference::IPermuteVectorPtr>& out_permute_vectors)
        override;
};

struct AddOperation : EltwiseOperation {
    AddOperation() : EltwiseOperation(OperationType::ADD) {}
};

struct AddNOperation : EltwiseOperation {
    AddNOperation() : EltwiseOperation(OperationType::ADDN) {}
};

struct MulOperation : EltwiseOperation {
    MulOperation() : EltwiseOperation(OperationType::MUL) {}
};

struct SubOperation : EltwiseOperation {
    SubOperation() : EltwiseOperation(OperationType::SUB) {}
};

struct DivOperation : EltwiseOperation {
    DivOperation() : EltwiseOperation(OperationType::DIV) {}
};

struct MinimumOperation : EltwiseOperation {
    MinimumOperation() : EltwiseOperation(OperationType::MINIMUM) {}
};

struct MaximumOperation : EltwiseOperation {
    MaximumOperation() : EltwiseOperation(OperationType::MAXIMUM) {}
};

} // namespace op
} // namespace nnrt

#endif
