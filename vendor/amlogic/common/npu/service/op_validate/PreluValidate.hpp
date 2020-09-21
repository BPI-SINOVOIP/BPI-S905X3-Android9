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

#ifndef _PRELU_VALIDATE_HPP_
#define _PRELU_VALIDATE_HPP_

#include "OperationValidate.hpp"

namespace android {
namespace nn {
namespace op_validate {
template <typename T_model, typename T_Operation>
class PreluValidate : public OperationValidate<T_model, T_Operation> {
   public:
    PreluValidate(const T_model& model, const T_Operation& operation)
        : OperationValidate<T_model, T_Operation>(model, operation) {}
    virtual bool SignatureCheck() override {
        auto inputList = hal::limitation::nnapi::match("PreluInput", this->m_InputArgTypes);
        auto outputList = hal::limitation::nnapi::match("PreluOutput", this->m_OutputArgTypes);
        if (inputList && outputList) {
            // Alpha only support 1D shape in ovxlib currently.
            int32_t alphaIndex = inputList->ArgPos("alpha");
            auto alphaRank =
                this->m_Model.operands[this->m_Operation.inputs[alphaIndex]].dimensions.size();
            return alphaRank == 1 ? true : false;
        } else {
            // Input or output data type not support
            return false;
        }
    };
};

}  // end of op_validate
}
}

#endif