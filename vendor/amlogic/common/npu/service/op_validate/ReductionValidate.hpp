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

#ifndef _REDUCTION_VALIDATE_HPP_
#define _REDUCTION_VALIDATE_HPP_

#include "OperationValidate.hpp"

namespace android {
namespace nn {
namespace op_validate {
template <typename T_model, typename T_Operation>
class ReduceAllValidate : public OperationValidate<T_model, T_Operation> {
   public:
    ReduceAllValidate(const T_model& model, const T_Operation& operation)
        : OperationValidate<T_model, T_Operation>(model, operation) {}
    virtual bool SignatureCheck() override {
        return hal::limitation::nnapi::match("ReduceAllInput", this->m_InputArgTypes) &&
               hal::limitation::nnapi::match("ReduceAllOutput", this->m_OutputArgTypes);
    };
};

template <typename T_model, typename T_Operation>
class ReduceAnyValidate : public OperationValidate<T_model, T_Operation> {
   public:
    ReduceAnyValidate(const T_model& model, const T_Operation& operation)
        : OperationValidate<T_model, T_Operation>(model, operation) {}
    virtual bool SignatureCheck() override {
        return hal::limitation::nnapi::match("ReduceAnyInput", this->m_InputArgTypes) &&
               hal::limitation::nnapi::match("ReduceAnyOutput", this->m_OutputArgTypes);
    };
};

template <typename T_model, typename T_Operation>
class ReduceMaxValidate : public OperationValidate<T_model, T_Operation> {
   public:
    ReduceMaxValidate(const T_model& model, const T_Operation& operation)
        : OperationValidate<T_model, T_Operation>(model, operation) {}
    virtual bool SignatureCheck() override {
        return hal::limitation::nnapi::match("ReduceMaxInput", this->m_InputArgTypes) &&
               hal::limitation::nnapi::match("ReduceMaxOutput", this->m_OutputArgTypes);
    };
};

template <typename T_model, typename T_Operation>
class ReduceMinValidate : public OperationValidate<T_model, T_Operation> {
   public:
    ReduceMinValidate(const T_model& model, const T_Operation& operation)
        : OperationValidate<T_model, T_Operation>(model, operation) {}
    virtual bool SignatureCheck() override {
        return hal::limitation::nnapi::match("ReduceMinInput", this->m_InputArgTypes) &&
               hal::limitation::nnapi::match("ReduceMinOutput", this->m_OutputArgTypes);
    };
};

template <typename T_model, typename T_Operation>
class ReduceSumValidate : public OperationValidate<T_model, T_Operation> {
   public:
    ReduceSumValidate(const T_model& model, const T_Operation& operation)
        : OperationValidate<T_model, T_Operation>(model, operation) {}
    virtual bool SignatureCheck() override {
        return hal::limitation::nnapi::match("ReduceSumInput", this->m_InputArgTypes) &&
               hal::limitation::nnapi::match("ReduceSumOutput", this->m_OutputArgTypes);
    };
};

template <typename T_model, typename T_Operation>
class ReduceProdValidate : public OperationValidate<T_model, T_Operation> {
   public:
    ReduceProdValidate(const T_model& model, const T_Operation& operation)
        : OperationValidate<T_model, T_Operation>(model, operation) {}
    virtual bool SignatureCheck() override {
        return hal::limitation::nnapi::match("ReduceProdInput", this->m_InputArgTypes) &&
               hal::limitation::nnapi::match("ReduceProdOutput", this->m_OutputArgTypes);
    };
};

}  // end of op_validate
}
}

#endif