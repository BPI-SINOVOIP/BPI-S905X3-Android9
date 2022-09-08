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
#ifndef _OP_CONV2D_
#define _OP_CONV2D_

#include "nnrt/op/operation.hpp"

namespace nnrt {
namespace op {

struct Conv1DOperation : Operation {
    Conv1DOperation() : Operation(OperationType::CONV_1D) {
        strides.resize(1);
        dilations.resize(1);
        pad.resize(2);
    }
    std::vector<int32_t> strides;
    std::vector<int32_t> dilations;
    std::vector<int32_t> pad;
    PadType padType{PadType::AUTO};
};

struct Conv2DOperation : Operation {
    Conv2DOperation() : Operation(OperationType::CONV_2D) {
        strides.resize(2);
        dilations.resize(2);
        pad.resize(4);
    }

    virtual void handleLayoutInferenceOnInputs(
        nnrt::Model& model,
        std::unordered_map<uint32_t, nnrt::layout_inference::IPermuteVectorPtr>& out_permute_vectors)
        override;
    std::vector<int32_t> strides;
    std::vector<int32_t> dilations;
    std::vector<int32_t> pad;
    PadType padType{PadType::AUTO};
};

struct GroupedConv2DOperation : Operation {
    GroupedConv2DOperation() : Operation(OperationType::GROUPED_CONV_2D) {
        strides.resize(2);
        dilations.resize(2);
        pad.resize(4);
    }

    virtual void handleLayoutInferenceOnInputs(
        nnrt::Model& model,
        std::unordered_map<uint32_t, nnrt::layout_inference::IPermuteVectorPtr>& out_permute_vectors)
        override;
    std::vector<int32_t> strides;
    std::vector<int32_t> dilations;
    std::vector<int32_t> pad;
    PadType padType{PadType::AUTO};
    int32_t groups{1};
};

struct DepthwiseConv2DOperation : Operation {
    DepthwiseConv2DOperation() : Operation(OperationType::DEPTHWISE_CONV_2D) {
        strides.resize(2);
        dilations.resize(2);
        pad.resize(4);
    }
    virtual void handleLayoutInferenceOnInputs(
        nnrt::Model& model,
        std::unordered_map<uint32_t, nnrt::layout_inference::IPermuteVectorPtr>& out_permute_vectors)
        override;
    std::vector<int32_t> strides;
    std::vector<int32_t> dilations;
    std::vector<int32_t> pad;
    PadType padType{PadType::AUTO};
    int32_t multiplier;
};

struct Deconv2DOperation : Operation {
    Deconv2DOperation() : Operation(OperationType::DECONV_2D) {
        strides.resize(2);
        pad.resize(4);
    }

    virtual void handleLayoutInferenceOnInputs(
        nnrt::Model& model,
        std::unordered_map<uint32_t, nnrt::layout_inference::IPermuteVectorPtr>& out_permute_vectors)
        override;

    std::vector<int32_t> output_shape;
    std::vector<int32_t> strides;
    std::vector<int32_t> pad;
    PadType padType{PadType::AUTO};
};
}
}
#endif
