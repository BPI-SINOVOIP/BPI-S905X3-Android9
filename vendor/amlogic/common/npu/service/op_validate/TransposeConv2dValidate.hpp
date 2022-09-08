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

#ifndef _TRANSPOSE_CONV2D_VALIDATE_HPP_
#define _TRANSPOSE_CONV2D_VALIDATE_HPP_

#include "OperationValidate.hpp"

namespace android {
namespace nn {
namespace op_validate {
template <typename T_model, typename T_Operation>
class TransposeConv2dValidate : public OperationValidate<T_model, T_Operation> {
   public:
    TransposeConv2dValidate(const T_model& model, const T_Operation& operation)
        : OperationValidate<T_model, T_Operation>(model, operation) {}
    bool SignatureCheck(std::string& reason) override {
        auto inputList = ::hal::limitation::nnapi::match("TransposeConv2DInput", this->InputArgTypes());
        auto outputList = ::hal::limitation::nnapi::match("TransposeConv2DOutput", this->OutputArgTypes());
        if (inputList && outputList) {
            int32_t inputIndex = inputList->ArgPos("input");
            int32_t kernelIndex = inputList->ArgPos("kernel");
            auto model = this->ModelForRead();
            auto operation = this->OperationForRead();
            if (operation.inputs[inputIndex] == operation.inputs[kernelIndex]) {
                reason += "reject TRANSPOSE_CONV_2D because input and kernel share the same tensor\n";
                return false;
            }
            auto inputOperand = model.operands[operation.inputs[inputIndex]];
            if (inputOperand.dimensions[0] > 1 && inputOperand.type == OperandType::TENSOR_FLOAT32) {
                reason += "reject TRANSPOSE_CONV_2D because not support input with multi batch\n";
                return false;
            }
            auto kernelOperand = model.operands[operation.inputs[kernelIndex]];
            if (kernelOperand.dimensions[1] * kernelOperand.dimensions[2] > 6400) {
                reason += "reject TRANSPOSE_CONV_2D because kernel size > 6400\n";
                return false;
            }
            return true;
        } else {
            reason += "reject TRANSPOSE_CONV_2D because not support the input data type\n";
            return false;
        }
    };
};

}  // end of op_validate
}
}

#endif