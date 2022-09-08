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

#ifndef _GROUPED_CONV2D_VALIDATE_HPP_
#define _GROUPED_CONV2D_VALIDATE_HPP_

#include "OperationValidate.hpp"

namespace android {
namespace nn {
namespace op_validate {
template <typename T_model, typename T_Operation>
class GroupedConv2DValidate : public OperationValidate<T_model, T_Operation> {
   public:
    GroupedConv2DValidate(const T_model& model, const T_Operation& operation)
        : OperationValidate<T_model, T_Operation>(model, operation) {}
    bool SignatureCheck(std::string& reason) override {
        auto inputList = ::hal::limitation::nnapi::match("GroupedConv2DInput", this->InputArgTypes());
        auto outputList = ::hal::limitation::nnapi::match("GroupedConv2DOutput", this->OutputArgTypes());
        if (inputList && outputList) {
            int32_t inputIndex = inputList->ArgPos("input");
            int32_t kernelIndex = inputList->ArgPos("kernel");
            int32_t groupNumberIndex = inputList->ArgPos("groups_num");
            int32_t layoutIndex = inputList->ArgPos("data_layout");
            auto model = this->ModelForRead();
            auto operation = this->OperationForRead();
            if (operation.inputs[inputIndex] == operation.inputs[kernelIndex]) {
                reason += "reject GROUPED_CONV_2D because input and kernel share a same tensor\n";
                return false;
            }
            auto& groupNumberOperand = model.operands[operation.inputs[groupNumberIndex]];
            auto& layoutOperand = model.operands[operation.inputs[layoutIndex]];
            uint32_t groupNumber = get_buffer::getScalarData<uint32_t>(model, groupNumberOperand);
            bool layout = get_buffer::getScalarData<bool>(model, layoutOperand);
            uint32_t inputChannel = 0;
            uint32_t outputChannel = 0;
            auto inputOperand = model.operands[operation.inputs[inputIndex]];
            auto outputOperand = model.operands[operation.outputs[0]];
            if (layout) {
                // NCHW
                inputChannel = inputOperand.dimensions[1];
                outputChannel = outputOperand.dimensions[1];
            } else {
                // NHWC
                inputChannel = inputOperand.dimensions[3];
                outputChannel = outputOperand.dimensions[3];
            }
            if (groupNumber != inputChannel / outputChannel) {
                reason +=
                    std::string(
                        "reject GROUPED_CONV2D because the group number is invalid groupNumber") +
                    std::to_string(groupNumber) + " != " + std::to_string(inputChannel) + "/" +
                    std::to_string(outputChannel) + "\n";
                return false;
            }
            auto kernelOperand = model.operands[operation.inputs[kernelIndex]];
            if (kernelOperand.lifetime != OperandLifeTime::CONSTANT_COPY) {
                reason += "reject GROUPED_CONV2D because kernel is not constant \n";
                return false;
            }
            if (kernelOperand.dimensions[1] * kernelOperand.dimensions[2] > 6400) {
                reason += "reject GROUPED_CONV2D because kernel size > 6400\n";
                return false;
            }
            return true;
        } else {
            reason += "reject GROUPED_CONV_2D because input data type not support\n";
            return false;
        }
    };
};

}  // end of op_validate
}
}

#endif
