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

#ifndef _OPERATION_VALIDATW_H_
#define _OPERATION_VALIDATW_H_

#include <android-base/logging.h>
#include <vector>
#include "HalInterfaces.h"
#include "hal_limitation/nnapi_limitation.hpp"
#include "nnrt/types.hpp"

namespace android {
namespace nn {
namespace op_validate {
template <typename T_Model, typename T_Operation>
class OperationValidate {
   public:
    OperationValidate(const T_Model& model, const T_Operation& operation)
        : m_Model(model), m_Operation(operation) {
        // GenInputArgTypes
        for (auto inIdx : m_Operation.inputs) {
            m_InputArgTypes.push_back(MapToNnrtOperandType(m_Model.operands[inIdx].type));
        }
        // GenOutputArgTypes
        // push first input into output argtypes
        m_OutputArgTypes.push_back(
                MapToNnrtOperandType(m_Model.operands[m_Operation.inputs[0]].type));
        for (auto outIdx : m_Operation.outputs) {
            m_OutputArgTypes.push_back(MapToNnrtOperandType(m_Model.operands[outIdx].type));
        }
    };
    virtual ~OperationValidate(){};

    bool DynamicShapeCheck() {
        // Check inputs
        if (0 == m_Operation.inputs.size()) return false;
        for (auto inIdx : m_Operation.inputs) {
            auto& dims = m_Model.operands[inIdx].dimensions;
            for (auto dim : dims) {
                if (dim == 0) {
                    return false;
                }
            }
        }
        // Check outputs
        if (0 == m_Operation.outputs.size()) return false;
        for (auto outIdx : m_Operation.outputs) {
            auto& dims = m_Model.operands[outIdx].dimensions;
            for (auto dim : dims) {
                if (dim == 0) {
                    return false;
                }
            }
        }
        return true;
    }

    bool ConstantTensorCheck() {
        std::vector<OperationType> whiteList = {OperationType::ADD,
                                                OperationType::SUB,
                                                OperationType::MUL,
                                                OperationType::DIV,
                                                OperationType::MAXIMUM,
                                                OperationType::MINIMUM,
                                                OperationType::CONCATENATION,
                                                OperationType::CONV_2D,
                                                OperationType::FULLY_CONNECTED,
                                                OperationType::LSTM,
                                                OperationType::DEPTHWISE_CONV_2D};

        if (std::find(whiteList.begin(), whiteList.end(), m_Operation.type) == whiteList.end()) {
            if (IsConstantTensor(m_Operation.inputs[0])) {
                return false;
            }
        }
        return true;
    }

    // Default implementation
    virtual bool SignatureCheck() { return true; };

    virtual bool Validate() {
        bool isSupport = true;
        isSupport &= DynamicShapeCheck();
        isSupport &= ConstantTensorCheck();
        isSupport &= SignatureCheck();
        return isSupport;
    };

    bool IsConstantTensor(size_t index) {
        auto& operand = m_Model.operands[index];
        return operand.lifetime == OperandLifeTime::CONSTANT_COPY ||
               operand.lifetime == OperandLifeTime::CONSTANT_REFERENCE;
    }

    T_Model m_Model;
    T_Operation m_Operation;
    std::vector<nnrt::OperandType> m_InputArgTypes;
    std::vector<nnrt::OperandType> m_OutputArgTypes;

   private:
    nnrt::OperandType MapToNnrtOperandType(OperandType type) {
        switch (type) {
            case OperandType::BOOL:
                return nnrt::OperandType::BOOL;
            case OperandType::INT32:
                return nnrt::OperandType::INT32;
            case OperandType::UINT32:
                return nnrt::OperandType::UINT32;
            case OperandType::FLOAT16:
                return nnrt::OperandType::FLOAT16;
            case OperandType::FLOAT32:
                return nnrt::OperandType::FLOAT32;
            case OperandType::TENSOR_BOOL8:
                return nnrt::OperandType::TENSOR_BOOL8;
            case OperandType::TENSOR_FLOAT16:
                return nnrt::OperandType::TENSOR_FLOAT16;
            case OperandType::TENSOR_FLOAT32:
                return nnrt::OperandType::TENSOR_FLOAT32;
            case OperandType::TENSOR_INT32:
                return nnrt::OperandType::TENSOR_INT32;
            case OperandType::TENSOR_QUANT8_ASYMM:
                return nnrt::OperandType::TENSOR_QUANT8_ASYMM;
            case OperandType::TENSOR_QUANT8_SYMM:
                return nnrt::OperandType::TENSOR_QUANT8_SYMM;
            case OperandType::TENSOR_QUANT16_ASYMM:
                return nnrt::OperandType::TENSOR_QUANT16_ASYMM;
            case OperandType::TENSOR_QUANT16_SYMM:
                return nnrt::OperandType::TENSOR_QUANT16_SYMM;
            case OperandType::TENSOR_QUANT8_SYMM_PER_CHANNEL:
                return nnrt::OperandType::TENSOR_QUANT8_SYMM_PER_CHANNEL;
            default:
                return nnrt::OperandType::NONE;
        }
    };
};

}  // end of op_validate
}
}

#endif