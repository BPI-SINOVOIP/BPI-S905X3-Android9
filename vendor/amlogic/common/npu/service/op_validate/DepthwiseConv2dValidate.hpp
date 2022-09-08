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

#ifndef _DEPTHWISE_CONV2D_VALIDATE_HPP_
#define _DEPTHWISE_CONV2D_VALIDATE_HPP_

#include "OperationValidate.hpp"

namespace android {
namespace nn {
namespace op_validate {
template <typename T_model, typename T_Operation>
class DepthwiseConv2dValidate : public OperationValidate<T_model, T_Operation> {
   public:
    DepthwiseConv2dValidate(const T_model& model, const T_Operation& operation)
        : OperationValidate<T_model, T_Operation>(model, operation) {}
    bool SignatureCheck(std::string& reason) override {
        auto model = this->ModelForRead();
        auto operation = this->OperationForRead();
        auto inputList =
            ::hal::limitation::nnapi::match("DepthwiseConvolution2DInput", this->InputArgTypes());
        auto outputList =
            ::hal::limitation::nnapi::match("DepthwiseConvolution2DOutput", this->OutputArgTypes());
        if (inputList && outputList) {
            int32_t inputIndex = inputList->ArgPos("input");
            int32_t kernelIndex = inputList->ArgPos("kernel");
            if (operation.inputs[inputIndex] == operation.inputs[kernelIndex]) {
                reason += "reject Depthwise_conv_2d because input and kernel share a same tensor\n";
                return false;
            }
            if (this->IsConstantTensor(operation.inputs[inputIndex])) {
                reason += "reject Depthwise_conv_2d because input is constant tensor\n";
                return false;
            }

            if (-1 != kernelIndex) {
                auto kernelDim = model.operands[operation.inputs[kernelIndex]].dimensions;
                if (kernelDim[0] != 1) {
                    reason += ("reject Depthwise_conv_2d because kernel[0] != 1\n");
                    return false;
                }
                if (!this->IsConstantTensor(operation.inputs[kernelIndex])) {
                    reason += "reject Depthwise_conv_2d because kernel is't constant tensor\n";
                    return false;
                }
            } else {
                LOG(ERROR) << "DepthwiseValidate: kernelIndex = -1";
                assert(false);
            }
            auto kernelOperand = model.operands[operation.inputs[kernelIndex]];
            if (kernelOperand.dimensions[1] * kernelOperand.dimensions[2] > 6400) {
                reason += "reject Depthwise_conv_2d because kernel size > 6400\n";
                return false;
            }
            return true;
        } else {
            reason += ("reject Depthwise_conv_2d because input data type not support\n");
            return false;
        }
    };
};

}  // end of op_validate
}
}

#endif
