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

#ifndef _CONV_2D_VALIDATE_HPP_
#define _CONV_2D_VALIDATE_HPP_

#include "OperationValidate.hpp"

namespace android {
namespace nn {
namespace op_validate {
template <typename T_model, typename T_Operation>
class Conv2dValidate : public OperationValidate<T_model, T_Operation> {
   public:
    Conv2dValidate(const T_model& model, const T_Operation& operation)
        : OperationValidate<T_model, T_Operation>(model, operation) {}
    virtual bool SignatureCheck() override {
        bool isSupport = true;
        isSupport &= hal::limitation::nnapi::match("Convolution2DInput", this->m_InputArgTypes) &&
                     hal::limitation::nnapi::match("Convolution2DOutput", this->m_OutputArgTypes);

        // Special validate
        auto inputIdx = this->m_Operation.inputs[0];
        isSupport &= this->IsConstantTensor(inputIdx);
        auto weightIdx = this->m_Operation.inputs[1];
        auto biasIdx = this->m_Operation.inputs[2];
        isSupport &= (!this->IsConstantTensor(weightIdx) && !this->IsConstantTensor(biasIdx)) ||
                     (this->IsConstantTensor(weightIdx) && this->IsConstantTensor(biasIdx));
        return isSupport;
    };
};

}  // end of op_validate
}
}

#endif