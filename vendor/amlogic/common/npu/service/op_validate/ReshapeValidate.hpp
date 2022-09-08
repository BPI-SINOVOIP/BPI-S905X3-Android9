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

#ifndef _RESHAPE_VALIDATE_HPP_
#define _RESHAPE_VALIDATE_HPP_

#include "OperationValidate.hpp"

namespace android {
namespace nn {
namespace op_validate {
template <typename T_model, typename T_Operation>
class ReshapeValidate : public OperationValidate<T_model, T_Operation> {
   public:
    ReshapeValidate(const T_model& model, const T_Operation& operation)
        : OperationValidate<T_model, T_Operation>(model, operation) {}
    bool SignatureCheck(std::string& reason) override {
        bool support = true;
        auto model = this->ModelForRead();
        auto operation = this->OperationForRead();
        auto inputOperand = model.operands[operation.inputs[0]];
        size_t sizeOfInput{1};
        for (auto itor = inputOperand.dimensions.begin(); itor != inputOperand.dimensions.end();
             ++itor) {
            sizeOfInput *= *(itor);
        }
        if (sizeOfInput <= 1) {
            reason = "reject RESHAPE because 1-value input tensor \n";
            support &= false;
        }
        return support && ::hal::limitation::nnapi::match("ReshapeInput", this->InputArgTypes()) &&
               ::hal::limitation::nnapi::match("ReshapeOutput", this->OutputArgTypes());
    };
};

}  // end of op_validate
}
}

#endif