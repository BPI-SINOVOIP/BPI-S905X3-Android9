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

#ifndef _LOGICAL_VALIDATE_HPP_
#define _LOGICAL_VALIDATE_HPP_

#include "OperationValidate.hpp"

namespace android {
namespace nn {
namespace op_validate {
template <typename T_model, typename T_Operation>
class LogicalAndValidate : public OperationValidate<T_model, T_Operation> {
   public:
    LogicalAndValidate(const T_model& model, const T_Operation& operation)
        : OperationValidate<T_model, T_Operation>(model, operation) {}
    virtual bool SignatureCheck() override {
        return hal::limitation::nnapi::match("LogicalAndInput", this->m_InputArgTypes) &&
               hal::limitation::nnapi::match("LogicalAndOutput", this->m_OutputArgTypes);
    };
};

template <typename T_model, typename T_Operation>
class LogicalOrValidate : public OperationValidate<T_model, T_Operation> {
   public:
    LogicalOrValidate(const T_model& model, const T_Operation& operation)
        : OperationValidate<T_model, T_Operation>(model, operation) {}
    virtual bool SignatureCheck() override {
        return hal::limitation::nnapi::match("LogicalOrInput", this->m_InputArgTypes) &&
               hal::limitation::nnapi::match("LogicalOrOutput", this->m_OutputArgTypes);
    };
};

template <typename T_model, typename T_Operation>
class LogicalNotValidate : public OperationValidate<T_model, T_Operation> {
   public:
    LogicalNotValidate(const T_model& model, const T_Operation& operation)
        : OperationValidate<T_model, T_Operation>(model, operation) {}
    virtual bool SignatureCheck() override {
        return hal::limitation::nnapi::match("LogicalNotInput", this->m_InputArgTypes) &&
               hal::limitation::nnapi::match("LogicalNotOutput", this->m_OutputArgTypes);
    };
};

}  // end of op_validate
}
}

#endif