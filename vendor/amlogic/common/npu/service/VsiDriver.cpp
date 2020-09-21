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

#include <cmath>
#include <memory>
#include "nnrt/file_map_memory.hpp"

#include "VsiDriver.h"
#include "VsiPlatform.h"
#include "VsiRTInfo.h"

#if ANDROID_SDK_VERSION >= 29
#include "public.hpp"
#endif

namespace android {
namespace nn {
namespace vsi_driver {

#if ANDROID_SDK_VERSION >= 29
using OperationValidatePtr = std::unique_ptr<
    android::nn::op_validate::OperationValidate<HalPlatform::Model, HalPlatform::Operation>>;
#endif

void VsiDriver::initalizeEnv() {
    disable_float_feature_ = 0;
    char env[100] = {0};
    int ireturn = __system_property_get("DISABLE_FLOAT_FEATURE", env);
    if (ireturn) {
        disable_float_feature_ = atoi(env);
        if (disable_float_feature_) LOG(INFO) << "float-type model will not running on hal";
    }
}

const uint8_t* VsiDriver::getOperandDataPtr(const HalPlatform::Model& model,
                                            const HalPlatform::Operand& hal_operand,
                                            VsiRTInfo& vsiMemory) {
    if (OperandLifeTime::CONSTANT_COPY == hal_operand.lifetime) {
        return model.operandValues.data() + hal_operand.location.offset;
    } else if (OperandLifeTime::CONSTANT_REFERENCE == hal_operand.lifetime) {
        if (!mapHidlMem(model.pools[hal_operand.location.poolIndex], vsiMemory)) return nullptr;
        return vsiMemory.getPtr(hal_operand.location.offset);
    }
    return nullptr;
}

const uint8_t* getOpeandPtr(const HalPlatform::Model& model,
                            const HalPlatform::Operand& operand,
                            struct VsiRTInfo& rt) {
    auto& location = operand.location;
    if (operand.lifetime == OperandLifeTime::CONSTANT_COPY) {
        return model.operandValues.data() + location.offset;
    } else if (operand.lifetime == OperandLifeTime::CONSTANT_REFERENCE) {
        return VsiDriver::getOperandDataPtr(model, operand, rt);
    } else
        return nullptr;
};

template <typename T_type>
T_type getScalarData(const HalPlatform::Model& model, const HalPlatform::Operand& operand) {
    struct VsiRTInfo rt;
    auto ptr = getOpeandPtr(model, operand, rt);
    if (ptr)
        return *reinterpret_cast<T_type*>(const_cast<uint8_t*>(ptr));
    else
        return 0;
}

bool VsiDriver::isSupportedOperation(const HalPlatform::Operation& operation,
                                     const HalPlatform::Model& model) {
#if ANDROID_SDK_VERSION >= 29
    auto checkSupportedOperand = [](auto& operand) -> bool {
        bool isSupported = true;
        switch (operand.type) {
            // API 29 newly added operand
            case OperandType::BOOL:
            case OperandType::TENSOR_QUANT16_SYMM:
            case OperandType::TENSOR_FLOAT16:
            case OperandType::TENSOR_BOOL8:
            case OperandType::FLOAT16:
            case OperandType::TENSOR_QUANT8_SYMM_PER_CHANNEL:
            case OperandType::TENSOR_QUANT16_ASYMM:
            case OperandType::TENSOR_QUANT8_SYMM:
                isSupported = false;
                break;
            default:
                break;
        }
        return isSupported;
    };

    auto isConstantTensor = [](auto& operand) -> bool {
        if (operand.lifetime == OperandLifeTime::CONSTANT_COPY ||
            operand.lifetime == OperandLifeTime::CONSTANT_REFERENCE)
            return true;
        else
            return false;
    };

    bool isSupport = true;
    // each operation check
    switch (operation.type) {
        // TODO: remove all of the work around
        case OperationType::CONV_2D: {
            auto& input = model.operands[operation.inputs[0]];
            auto& weight = model.operands[operation.inputs[1]];
            auto& bias = model.operands[operation.inputs[2]];
            if (isConstantTensor(input)) {
                LOG(INFO) << "Device don't support constant input";
                isSupport &= false;
            }

            isSupport &= (!isConstantTensor(bias) && !isConstantTensor(weight)) ||
                         (isConstantTensor(bias) && isConstantTensor(weight));
            break;
        }
        case OperationType::DEPTHWISE_CONV_2D: {
            auto kernelDim = model.operands[operation.inputs[1]].dimensions;
            if (kernelDim[0] == 1) {
                isSupport &= true;
            } else {
                isSupport &= false;
            }
            break;
        }
        case OperationType::RNN: {
            int32_t fuseCode = getScalarData<int32_t>(model, model.operands[operation.inputs[5]]);
            if (fuseCode == static_cast<int32_t>(FusedActivationFunc::NONE) ||
                fuseCode == static_cast<int32_t>(FusedActivationFunc::RELU) ||
                fuseCode == static_cast<int32_t>(FusedActivationFunc::RELU1) ||
                fuseCode == static_cast<int32_t>(FusedActivationFunc::RELU6)) {
                isSupport &= true;
            } else {
                isSupport &= false;
            }
            break;
        }

        case OperationType::SOFTMAX: {
            auto& input = model.operands[operation.inputs[0]];
            if (isConstantTensor(input)) {
                LOG(INFO) << "Device don't support constant input";
                isSupport &= false;
            }
            break;
        }
        case OperationType::LSH_PROJECTION: {
            int32_t typePtr = getScalarData<int32_t>(model, model.operands[operation.inputs[3]]);
            if (3 == typePtr) isSupport &= false;
            break;
        }
        case OperationType::TANH: {
            if (OperandType::TENSOR_FLOAT32 != model.operands[operation.inputs[0]].type)
                isSupport &= false;
            break;
        }
        case OperationType::LSTM: {
            if (operation.inputs.size() > 23) isSupport &= false;
            break;
        }

        case OperationType::TRANSPOSE: {
            // according to the spec, perm is optinal.
            if (operation.inputs.size() == 1) isSupport &= false;

            auto& perm = model.operands[operation.inputs[1]];
            if (OperandLifeTime::MODEL_INPUT == perm.lifetime) {
                LOG(ERROR) << "do not support perm as input";
                isSupport &= false;
            }
            size_t dimSize = perm.location.length / sizeof((int32_t)0);
            if (dimSize < 4) {
                isSupport &= true;
                break;
            }

            struct VsiRTInfo rt;
            auto permData = getOperandDataPtr(model, perm, rt);
            isSupport &= permData && (*(int32_t*)permData == 0);
            break;
        }
        case OperationType::FULLY_CONNECTED: {
            auto& input = model.operands[operation.inputs[0]];
            auto& weight = model.operands[operation.inputs[1]];
            if (input.dimensions.size() != 2 ||
                (weight.dimensions.size() == 2 && input.dimensions[1] != weight.dimensions[1]))
                isSupport &= false;
            break;
        }
        case OperationType::PAD: {
            // TODO: support pad at channel and batch
            auto& pad = model.operands[operation.inputs[1]];
            if (!isConstantTensor(pad)) return false;
            size_t dimSize = pad.dimensions.size();
            // Pad only support 4D PAD
            if (dimSize != 4) return false;
            if (dimSize < 3) {
                isSupport &= true;
                break;
            }

            struct VsiRTInfo rt;
            auto padData = reinterpret_cast<const int32_t*>(getOperandDataPtr(model, pad, rt));

            if (!padData) isSupport &= false;
            if (dimSize > 2) {
                if (dimSize == 3 && padData[4] + padData[5] != 0) isSupport &= false;
                if (dimSize == 4 && padData[6] + padData[7] + padData[0] + padData[1] != 0)
                    isSupport &= false;
            }
            break;
        }
        // to-do: check operand with operation
        // API 29 newly added operataion
        case OperationType::ABS: {
            OperationValidatePtr absValidate = std::make_unique<
                op_validate::AbsValidate<HalPlatform::Model, HalPlatform::Operation>>(model,
                                                                                      operation);
            return absValidate->Validate();
        }

        case OperationType::ARGMAX:
        case OperationType::ARGMIN: {
            OperationValidatePtr argmaxArgmin = std::make_unique<
                op_validate::ArgmaxArgminValidate<HalPlatform::Model, HalPlatform::Operation>>(
                model, operation);
            return argmaxArgmin->Validate();
        }

        case OperationType::MINIMUM: {
            OperationValidatePtr maxMin = std::make_unique<
                op_validate::MaximumMinimumValidate<HalPlatform::Model, HalPlatform::Operation>>(
                model, operation);
            return maxMin->Validate();
        }

        case OperationType::RSQRT:
        case OperationType::SQRT: {
            OperationValidatePtr sqrtRsqrt = std::make_unique<
                op_validate::SqrtRsqrtValidate<HalPlatform::Model, HalPlatform::Operation>>(
                model, operation);
            return sqrtRsqrt->Validate();
        }

        case OperationType::LOG: {
            OperationValidatePtr logValidate = std::make_unique<
                op_validate::LogValidate<HalPlatform::Model, HalPlatform::Operation>>(model,
                                                                                      operation);
            return logValidate->Validate();
        }

        case OperationType::EXP: {
            OperationValidatePtr expValidate = std::make_unique<
                op_validate::ExpValidate<HalPlatform::Model, HalPlatform::Operation>>(model,
                                                                                      operation);
            return expValidate->Validate();
        }

        case OperationType::SIN: {
            OperationValidatePtr sinValidate = std::make_unique<
                op_validate::SinValidate<HalPlatform::Model, HalPlatform::Operation>>(model,
                                                                                      operation);
            return sinValidate->Validate();
        }

        case OperationType::RESIZE_BILINEAR:
        case OperationType::RESIZE_NEAREST_NEIGHBOR: {
            OperationValidatePtr resizeValidate = std::make_unique<
                op_validate::ResizeValidate<HalPlatform::Model, HalPlatform::Operation>>(model,
                                                                                         operation);
            return resizeValidate->Validate();
        }

        case OperationType::REDUCE_MAX: {
            OperationValidatePtr reduceMax = std::make_unique<
                op_validate::ReduceMaxValidate<HalPlatform::Model, HalPlatform::Operation>>(
                model, operation);
            return reduceMax->Validate();
        }

        case OperationType::REDUCE_MIN: {
            OperationValidatePtr reduceMin = std::make_unique<
                op_validate::ReduceMinValidate<HalPlatform::Model, HalPlatform::Operation>>(
                model, operation);
            return reduceMin->Validate();
        }

        case OperationType::REDUCE_PROD: {
            OperationValidatePtr reduceProd = std::make_unique<
                op_validate::ReduceProdValidate<HalPlatform::Model, HalPlatform::Operation>>(
                model, operation);
            return reduceProd->Validate();
        }

        case OperationType::REDUCE_SUM: {
            OperationValidatePtr reduceSum = std::make_unique<
                op_validate::ReduceSumValidate<HalPlatform::Model, HalPlatform::Operation>>(
                model, operation);
            return reduceSum->Validate();
        }

        case OperationType::NEG: {
            OperationValidatePtr neg = std::make_unique<
                op_validate::NegValidate<HalPlatform::Model, HalPlatform::Operation>>(model,
                                                                                      operation);
            return neg->Validate();
        }

        case OperationType::PRELU: {
            OperationValidatePtr prelu = std::make_unique<
                op_validate::PreluValidate<HalPlatform::Model, HalPlatform::Operation>>(model,
                                                                                        operation);
            return prelu->Validate();
        }

        case OperationType::LESS:
        case OperationType::LESS_EQUAL:
        case OperationType::EQUAL:
        case OperationType::GREATER:
        case OperationType::GREATER_EQUAL:
        case OperationType::NOT_EQUAL: {
            OperationValidatePtr compare = std::make_unique<
                op_validate::ComparisonValidate<HalPlatform::Model, HalPlatform::Operation>>(
                model, operation);
            return compare->Validate();
        }

        case OperationType::LOGICAL_AND: {
            OperationValidatePtr logicalAnd = std::make_unique<
                op_validate::LogicalAndValidate<HalPlatform::Model, HalPlatform::Operation>>(
                model, operation);
            return logicalAnd->Validate();
        }
        case OperationType::LOGICAL_NOT: {
            OperationValidatePtr logicalNot = std::make_unique<
                op_validate::LogicalNotValidate<HalPlatform::Model, HalPlatform::Operation>>(
                model, operation);
            return logicalNot->Validate();
        }
        case OperationType::LOGICAL_OR: {
            OperationValidatePtr logicalOr = std::make_unique<
                op_validate::LogicalOrValidate<HalPlatform::Model, HalPlatform::Operation>>(
                model, operation);
            return logicalOr->Validate();
        }
        case OperationType::EXPAND_DIMS: {
            OperationValidatePtr expandDims = std::make_unique<
                op_validate::ExpandDimsValidate<HalPlatform::Model, HalPlatform::Operation>>(
                model, operation);
            return expandDims->Validate();
        }
        case OperationType::POW: {
            OperationValidatePtr pow = std::make_unique<
                op_validate::PowValidate<HalPlatform::Model, HalPlatform::Operation>>(model,
                                                                                      operation);
            return pow->Validate();
        }
        case OperationType::INSTANCE_NORMALIZATION: {
            OperationValidatePtr instanceNorm = std::make_unique<
                op_validate::InstanceNormValidate<HalPlatform::Model, HalPlatform::Operation>>(
                model, operation);
            return false;
            // return instanceNorm->Validate();
        }
        case OperationType::SPLIT: {
            OperationValidatePtr split = std::make_unique<
                op_validate::SplitValidate<HalPlatform::Model, HalPlatform::Operation>>(model,
                                                                                        operation);
            return split->Validate();
        }
        case OperationType::LOG_SOFTMAX: {
            OperationValidatePtr logSoftmax = std::make_unique<
                op_validate::LogSoftmaxValidate<HalPlatform::Model, HalPlatform::Operation>>(
                model, operation);
            return logSoftmax->Validate();
        }
        case OperationType::REDUCE_ALL: {
            OperationValidatePtr reduceAll = std::make_unique<
                op_validate::ReduceAllValidate<HalPlatform::Model, HalPlatform::Operation>>(
                model, operation);
            return false;
            // return reduceAll->Validate();
        }
        case OperationType::REDUCE_ANY: {
            OperationValidatePtr reduceAny = std::make_unique<
                op_validate::ReduceAnyValidate<HalPlatform::Model, HalPlatform::Operation>>(
                model, operation);
            return false;
            // return reduceAny->Validate();
        }
        case OperationType::GATHER: {
            OperationValidatePtr gather = std::make_unique<
                op_validate::GatherValidate<HalPlatform::Model, HalPlatform::Operation>>(model,
                                                                                         operation);
            return gather->Validate();
        }

        case OperationType::AXIS_ALIGNED_BBOX_TRANSFORM: {
            OperationValidatePtr axisAlignedBBoxTransform = std::make_unique<
                op_validate::AxisAlignedBBoxTransformValidate<HalPlatform::Model, HalPlatform::Operation>>(model,
                                                                                      operation);
            return axisAlignedBBoxTransform->Validate();
        }
        case OperationType::UNIDIRECTIONAL_SEQUENCE_LSTM: {
            OperationValidatePtr unidirectionalSequenceLstm = std::make_unique<
                op_validate::UnidirectionalSequenceLstmValidate<HalPlatform::Model, HalPlatform::Operation>>(model,
                                                                                      operation);
            // All generated cases failed
            return false;
            // return unidirectionalSequenceLstm->Validate();
        }
        case OperationType::BIDIRECTIONAL_SEQUENCE_LSTM: {
            OperationValidatePtr bidirectionalSequenceLstm = std::make_unique<
                op_validate::BidirectionalSequenceLstmValidate<HalPlatform::Model, HalPlatform::Operation>>(model,
                                                                                      operation);
            // All generated cases failed, need to fix
            return false;
            // return bidirectionalSequenceLstm->Validate();
        }
        case OperationType::GENERATE_PROPOSALS: {
            OperationValidatePtr generateProposals = std::make_unique<
                op_validate::GenerateProposalsValidate<HalPlatform::Model, HalPlatform::Operation>>(model,
                                                                                      operation);
            // Some generated float32 cases failed
            return false;
            // return generateProposals->Validate();
        }
        case OperationType::DETECTION_POSTPROCESSING: {
            OperationValidatePtr detectionPostprocessing = std::make_unique<
                op_validate::DetectionPostprocessingValidate<HalPlatform::Model, HalPlatform::Operation>>(model,
                                                                                      operation);
            // Some generated float32 cases crashed
            return false;
            // return detectionPostprocessing->Validate();
        }
        case OperationType::UNIDIRECTIONAL_SEQUENCE_RNN: {
            OperationValidatePtr unidirectionalSequenceRnn = std::make_unique<
                op_validate::UnidirectionalSequenceRnnValidate<HalPlatform::Model, HalPlatform::Operation>>(model,
                                                                                      operation);
            // Some float32 cases failed
            return false;
            // return unidirectionalSequenceRnn->Validate();
        }
        case OperationType::BIDIRECTIONAL_SEQUENCE_RNN: {
            OperationValidatePtr bidirectionalSequenceRnn = std::make_unique<
                op_validate::BidirectionalSequenceRnnValidate<HalPlatform::Model, HalPlatform::Operation>>(model,
                                                                                      operation);
            // All generated cases failed, need to fix
            return false;
            // return bidirectionalSequenceRnn->Validate();
        }

        case OperationType::BOX_WITH_NMS_LIMIT:
        case OperationType::CAST:
        case OperationType::CHANNEL_SHUFFLE:
        case OperationType::GROUPED_CONV_2D:
        case OperationType::HEATMAP_MAX_KEYPOINT:
        case OperationType::PAD_V2:
        case OperationType::QUANTIZE:
        case OperationType::QUANTIZED_16BIT_LSTM:
        case OperationType::RANDOM_MULTINOMIAL:
        case OperationType::ROI_ALIGN:
        case OperationType::ROI_POOLING:
        case OperationType::SELECT:
        case OperationType::SLICE:
        case OperationType::TILE:
        case OperationType::TOPK_V2:
        case OperationType::TRANSPOSE_CONV_2D:
        case OperationType::MAXIMUM:
            isSupport &= false;
            break;
        default:
            isSupport &= true;
    }

    // Overall check
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

    // do not support constant tensor as operation's Input except whiteList.
    if (std::find(whiteList.begin(), whiteList.end(), operation.type) == whiteList.end()) {
        if (isConstantTensor(model.operands[operation.inputs[0]])) isSupport &= false;
    }

    // TODO: [NNRT-1] Support static shape inference for NNAPI 1.2
    for (size_t i = 0; i < operation.outputs.size(); i++) {
        auto& dims = model.operands[operation.outputs[i]].dimensions;
        for (auto dimIndex : dims)
            if (dimIndex == 0) isSupport &= false;
    }

    // TODO: nnapi 1.2 new operand type
    for (size_t i = 0; i < operation.inputs.size(); i++) {
        auto& operand = model.operands[operation.inputs[i]];
        if (false == checkSupportedOperand(operand)) isSupport &= false;
    }
    for (size_t i = 0; i < operation.outputs.size(); i++) {
        auto& operand = model.operands[operation.outputs[i]];
        if (false == checkSupportedOperand(operand)) isSupport &= false;
    }

    // Not support dynamic shape
    // Check inputs
    if (0 == operation.inputs.size()) isSupport &= false;
    for (auto inIdx : operation.inputs) {
        auto& dims = model.operands[inIdx].dimensions;
        for (auto dim : dims) {
            if (dim == 0) {
                isSupport &= false;
            }
        }
    }
    // Check outputs
    if (0 == operation.outputs.size()) isSupport = false;
    for (auto outIdx : operation.outputs) {
        auto& dims = model.operands[outIdx].dimensions;
        for (auto dim : dims) {
            if (dim == 0) {
                isSupport &= false;
            }
        }
    }

    return isSupport;
#endif
    return true;
}

}  // namespace vsi_driver
}  // namespace nn
}

using android::nn::vsi_driver::VsiDriver;
using android::sp;

int main() {
    sp<VsiDriver> driver(new VsiDriver());
    return driver->run();
}
