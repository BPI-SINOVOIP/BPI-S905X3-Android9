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
#include "VsiRTInfo.h"

namespace android {
namespace nn {
namespace op_validate {

namespace {
template <typename T_model, typename T_Operation>
bool ReduceOpRule(T_model model, T_Operation operation, std::string& reason) {
    auto& inputOperand = model.operands[operation.inputs[0]];
    size_t inputRank = inputOperand.dimensions.size();
    auto& axesOperand = model.operands[operation.inputs[1]];

    vsi_driver::VsiRTInfo axesMemInfo;
    const int32_t* axes = reinterpret_cast<const int32_t*>(
        get_buffer::getOperandDataPtr(model, axesOperand, axesMemInfo));
    const int32_t* axesEnd = axes + axesMemInfo.buffer_size;
    std::set<int32_t> uniqueAxes;
    while (axes < axesEnd) {
        uniqueAxes.insert((*axes + inputRank) % inputRank);
        ++axes;
    }

    if (uniqueAxes.size() == inputRank) {
        reason += "reject Reduce because all dimension required reduce is not supported\n";
        return false;
    }
    return true;
}
}  // namespace

template <typename T_model, typename T_Operation>
class ReduceAllValidate : public OperationValidate<T_model, T_Operation> {
   public:
    ReduceAllValidate(const T_model& model, const T_Operation& operation)
        : OperationValidate<T_model, T_Operation>(model, operation) {}
    bool SignatureCheck(std::string& reason) override {
        return ReduceOpRule<T_model, T_Operation>(
                   this->ModelForRead(), this->OperationForRead(), reason) &&
               ::hal::limitation::nnapi::match("ReduceAllInput", this->InputArgTypes()) &&
               ::hal::limitation::nnapi::match("ReduceAllOutput", this->OutputArgTypes());
    };
};

template <typename T_model, typename T_Operation>
class ReduceAnyValidate : public OperationValidate<T_model, T_Operation> {
   public:
    ReduceAnyValidate(const T_model& model, const T_Operation& operation)
        : OperationValidate<T_model, T_Operation>(model, operation) {}
    bool SignatureCheck(std::string& reason) override {
        return ReduceOpRule<T_model, T_Operation>(
                   this->ModelForRead(), this->OperationForRead(), reason) &&
               ::hal::limitation::nnapi::match("ReduceAnyInput", this->InputArgTypes()) &&
               ::hal::limitation::nnapi::match("ReduceAnyOutput", this->OutputArgTypes());
    };
};

template <typename T_model, typename T_Operation>
class ReduceMaxValidate : public OperationValidate<T_model, T_Operation> {
   public:
    ReduceMaxValidate(const T_model& model, const T_Operation& operation)
        : OperationValidate<T_model, T_Operation>(model, operation) {}
    bool SignatureCheck(std::string& reason) override {
        return ReduceOpRule<T_model, T_Operation>(
                   this->ModelForRead(), this->OperationForRead(), reason) &&
               ::hal::limitation::nnapi::match("ReduceMaxInput", this->InputArgTypes()) &&
               ::hal::limitation::nnapi::match("ReduceMaxOutput", this->OutputArgTypes());
    };
};

template <typename T_model, typename T_Operation>
class ReduceMinValidate : public OperationValidate<T_model, T_Operation> {
   public:
    ReduceMinValidate(const T_model& model, const T_Operation& operation)
        : OperationValidate<T_model, T_Operation>(model, operation) {}
    bool SignatureCheck(std::string& reason) override {
        return ReduceOpRule<T_model, T_Operation>(
                   this->ModelForRead(), this->OperationForRead(), reason) &&
               ::hal::limitation::nnapi::match("ReduceMinInput", this->InputArgTypes()) &&
               ::hal::limitation::nnapi::match("ReduceMinOutput", this->OutputArgTypes());
    };
};

template <typename T_model, typename T_Operation>
class ReduceSumValidate : public OperationValidate<T_model, T_Operation> {
   public:
    ReduceSumValidate(const T_model& model, const T_Operation& operation)
        : OperationValidate<T_model, T_Operation>(model, operation) {}
    bool SignatureCheck(std::string& reason) override {
        return ReduceOpRule<T_model, T_Operation>(
                   this->ModelForRead(), this->OperationForRead(), reason) &&
               ::hal::limitation::nnapi::match("ReduceSumInput", this->InputArgTypes()) &&
               ::hal::limitation::nnapi::match("ReduceSumOutput", this->OutputArgTypes());
    };
};

template <typename T_model, typename T_Operation>
class ReduceProdValidate : public OperationValidate<T_model, T_Operation> {
   public:
    ReduceProdValidate(const T_model& model, const T_Operation& operation)
        : OperationValidate<T_model, T_Operation>(model, operation) {}
    bool SignatureCheck(std::string& reason) override {
        return ReduceOpRule<T_model, T_Operation>(
                   this->ModelForRead(), this->OperationForRead(), reason) &&
               ::hal::limitation::nnapi::match("ReduceProdInput", this->InputArgTypes()) &&
               ::hal::limitation::nnapi::match("ReduceProdOutput", this->OutputArgTypes());
    };
};

}  // end of op_validate
}
}

#endif