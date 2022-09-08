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

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <vector>

#if ANDROID_SDK_VERSION >= 29
#include "public.hpp"
#endif
namespace {
// blacklist content:
// single line include operation index defined by android nn spec
// each number should end with ","
bool IsOpBlockedByBlacklist(int32_t op_type) {
    char env[128] = {0};
    __system_property_get("NN_DBG_OP_BLK_LIST", env);
    if (strlen(env) == 0) return false;

    std::fstream fs(env);
    if (!fs.good()) {
        LOG(INFO) << "can not open blacklist file from -> " << env;
        // Ignore if no whitelist found
        return false;
    }
    std::string op_list_line;  // = fs.getline();
    std::getline(fs, op_list_line);
    std::vector<int32_t> op_list;

    const char* splitor = ",";

    auto end = op_list_line.find(splitor);
    decltype(end) start = -1;
    while (end != op_list_line.npos && end != start) {
        auto value = op_list_line.substr(start + 1, end - start - 1);
        start = op_list_line.find(splitor, end);
        end = op_list_line.find(splitor, start + 1);
        op_list.push_back(std::stoi(value));
    }

    return op_list.end() !=
           std::find(op_list.begin(), op_list.end(), static_cast<int32_t>(op_type));
}
}  // namespace

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

bool isTensor(const HalPlatform::Operand& operand) {
    bool tensor = true;
    switch (operand.type) {
#if ANDROID_SDK_VERSION >= 29
        case OperandType::BOOL:
        case OperandType::FLOAT16:
#endif
        case OperandType::FLOAT32:
        case OperandType::INT32:
        case OperandType::UINT32:
            tensor = false;
            break;
        default:
            tensor = true;
    }
    return tensor;
}

bool VsiDriver::isSupportedOperation(const HalPlatform::Operation& operation,
                                     const HalPlatform::Model& model,
                                     std::string& reason) {
    if (IsOpBlockedByBlacklist(static_cast<int32_t>(operation.type))) {
        reason += " reject op because it blocked by debug blacklist file\n";
        return false;
    }
#if ANDROID_SDK_VERSION >= 28
    bool isSupport = true;
#endif
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
#endif
#if ANDROID_SDK_VERSION >= 28
    auto isConstantTensor = [](auto& operand) -> bool {
        if (operand.lifetime == OperandLifeTime::CONSTANT_COPY ||
            operand.lifetime == OperandLifeTime::CONSTANT_REFERENCE)
            return true;
        else
            return false;
    };
    // each operation check
    switch (operation.type) {
#endif
#if ANDROID_SDK_VERSION >= 29
        // TODO: remove all of the work around
        case OperationType::CONV_2D: {
            OperationValidatePtr conv2D = std::make_unique<
                op_validate::Conv2dValidate<HalPlatform::Model, HalPlatform::Operation>>(
                model, operation);
            return conv2D->Validate(reason);
        }
#endif
#if ANDROID_SDK_VERSION >= 28
        case OperationType::ADD:
        case OperationType::SUB:
        case OperationType::MUL:
        case OperationType::DIV: {
            // validate constant rule
            auto input0 = model.operands[operation.inputs[0]];
            auto input1 = model.operands[operation.inputs[1]];
            if (isConstantTensor(input0) && isConstantTensor(input1)) {
                reason += ("reject ADD|SUB|MUL|DIV because all input tensor is constant\n");
                isSupport &= false;
            }
            // validate shape rule
            if (input0.dimensions.size() == input1.dimensions.size()) {
                for (size_t i = 0; i < input0.dimensions.size(); i++) {
                    if (input0.dimensions[i] != input1.dimensions[i]) {
                        if (input0.dimensions[i] != 1 && input1.dimensions[i] != 1) {
                            reason += "reject ADD|SUB|MUL|DIV because inputs shape is invalidated\n";
                            isSupport &= false;
                            break;
                        }
                    }
                }
            } else {
                isSupport &= true;
            }
            break;
        }
#endif
#if ANDROID_SDK_VERSION >= 29
        case OperationType::RNN: {
            int32_t fuseCode = getScalarData<int32_t>(model, model.operands[operation.inputs[5]]);
            if (fuseCode == static_cast<int32_t>(FusedActivationFunc::RELU) ||
                fuseCode == static_cast<int32_t>(FusedActivationFunc::RELU1) ||
                fuseCode == static_cast<int32_t>(FusedActivationFunc::RELU6)) {
                isSupport &= true;
            } else {
                reason += ("reject RNN because fuseCode not support \n");
                isSupport &= false;
            }
            break;
        }

        case OperationType::SOFTMAX: {
            auto& input = model.operands[operation.inputs[0]];
            if (isConstantTensor(input)) {
                reason += ("reject SOFTMAX because input tensor is constant\n");
                isSupport &= false;
            }
            break;
        }
        case OperationType::LSH_PROJECTION: {
            reason += ("rejcet LSH_PROJECTION because only SW implement\n");
            return false;
            // int32_t typePtr = getScalarData<int32_t>(model, model.operands[operation.inputs[3]]);
            // if (3 == typePtr) {
            //     isSupport &= false;
            // }
            // break;
        }
        case OperationType::TANH: {
            if (OperandType::TENSOR_FLOAT32 != model.operands[operation.inputs[0]].type) {
                reason += "reject TANH because only support input_type = FLOAT32 tensor\n";
                isSupport &= false;
            }
            break;
        }
        case OperationType::LSTM: {
            if (operation.inputs.size() > 23) {
                reason += "reject LSTM because New Spec in 1.2 not enabled\n";
                isSupport &= false;
            }
            break;
        }

        case OperationType::TRANSPOSE: {
            // according to the spec, perm is optinal.
            if (operation.inputs.size() == 1) {
                reason += "reject TRANSPOSE because no perm vetor provided\n";
                isSupport &= false;
            }

            auto& perm = model.operands[operation.inputs[1]];
            if (OperandLifeTime::MODEL_INPUT == perm.lifetime) {
                isSupport &= false;
                reason += "reject TRANSPOSE because permute not supported as an input \n";
            }
            size_t dimSize = perm.location.length / sizeof(int32_t);

            struct VsiRTInfo rt;
            auto permData = getOperandDataPtr(model, perm, rt);
            bool batchIsTransposed = permData && (*(int32_t*)permData != 0);
            if (dimSize >= 4 || batchIsTransposed) {
                reason += "reject TRANSPOSE because >=4D or transposed on Batch not supported\n";
                isSupport &= false;
            }

            break;
        }
        case OperationType::FULLY_CONNECTED: {
            auto& input = model.operands[operation.inputs[0]];
            auto& weight = model.operands[operation.inputs[1]];
            if (input.dimensions.size() != 2 ||
                (weight.dimensions.size() == 2 && input.dimensions[1] != weight.dimensions[1])) {
                reason += "reject FC because weight tensor require reshape\n";
                isSupport &= false;
            }
            break;
        }
        case OperationType::PAD: {
            // TODO: support pad at channel and batch
            auto& pad = model.operands[operation.inputs[1]];
            if (!isConstantTensor(pad)) return false;
            size_t dimSize = pad.dimensions[0];
            // Pad only support 4D PAD
            if (dimSize != 4) {
                reason += "reject Pad because dimSize !=4\n";
                return false;
            }
            if (dimSize < 3) {
                isSupport &= true;
                break;
            }

            struct VsiRTInfo rt;
            auto padData = reinterpret_cast<const int32_t*>(getOperandDataPtr(model, pad, rt));

            if (!padData) isSupport &= false;
            if (dimSize > 2) {
                bool noPadOn3rdDimFor3D = (dimSize == 3 && padData[4] + padData[5] != 0);
                bool noPadOn0rd3rdFor4D =
                    (dimSize == 4 && padData[6] + padData[7] + padData[0] + padData[1] == 0);
                isSupport &= (noPadOn3rdDimFor3D || noPadOn0rd3rdFor4D);
                if (!isSupport) {
                    reason += "reject PAD because pad value for 3rd or 0rd/3rd dimensions\n";
                }
            }
            break;
        }
        case OperationType::SVDF: {
            struct VsiRTInfo rtInfo;
            const auto& rankOperand = model.operands[operation.inputs[5]];
            const int32_t* rankValue =
                reinterpret_cast<const int32_t*>(getOperandDataPtr(model, rankOperand, rtInfo));
            if (rankValue && rankValue[0] <= 2) {
                reason += "reject SVDF because rankd <= 2 not support\n";
                isSupport &= false;
            }
            break;
        }

        case OperationType::HASHTABLE_LOOKUP: {
            auto& value_tensor = model.operands[operation.inputs[2]];

            if (2 != value_tensor.dimensions.size()) {
                reason += "reject HASHTABLE_LOOPUP until we support value tensor other than 2D\n";
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
            return absValidate->Validate(reason);
        }

        case OperationType::ARGMAX:
        case OperationType::ARGMIN: {
            OperationValidatePtr argmaxArgmin = std::make_unique<
                op_validate::ArgmaxArgminValidate<HalPlatform::Model, HalPlatform::Operation>>(
                model, operation);
            return argmaxArgmin->Validate(reason);
        }

        case OperationType::MAXIMUM:
        case OperationType::MINIMUM: {
            OperationValidatePtr maxMin = std::make_unique<
                op_validate::MaximumMinimumValidate<HalPlatform::Model, HalPlatform::Operation>>(
                model, operation);
            return maxMin->Validate(reason);
        }

        case OperationType::RSQRT:
        case OperationType::SQRT: {
            OperationValidatePtr sqrtRsqrt = std::make_unique<
                op_validate::SqrtRsqrtValidate<HalPlatform::Model, HalPlatform::Operation>>(
                model, operation);
            return sqrtRsqrt->Validate(reason);
        }

        case OperationType::LOG: {
            OperationValidatePtr logValidate = std::make_unique<
                op_validate::LogValidate<HalPlatform::Model, HalPlatform::Operation>>(model,
                                                                                      operation);
            return logValidate->Validate(reason);
        }

        case OperationType::EXP: {
            OperationValidatePtr expValidate = std::make_unique<
                op_validate::ExpValidate<HalPlatform::Model, HalPlatform::Operation>>(model,
                                                                                      operation);
            return expValidate->Validate(reason);
        }

        case OperationType::SIN: {
            OperationValidatePtr sinValidate = std::make_unique<
                op_validate::SinValidate<HalPlatform::Model, HalPlatform::Operation>>(model,
                                                                                      operation);
            return sinValidate->Validate(reason);
        }

        case OperationType::RESIZE_BILINEAR:
        case OperationType::RESIZE_NEAREST_NEIGHBOR: {
            OperationValidatePtr resizeValidate = std::make_unique<
                op_validate::ResizeValidate<HalPlatform::Model, HalPlatform::Operation>>(model,
                                                                                         operation);
            return resizeValidate->Validate(reason);
        }

        case OperationType::REDUCE_MAX: {
            OperationValidatePtr reduceMax = std::make_unique<
                op_validate::ReduceMaxValidate<HalPlatform::Model, HalPlatform::Operation>>(
                model, operation);
            return reduceMax->Validate(reason);
        }

        case OperationType::REDUCE_MIN: {
            OperationValidatePtr reduceMin = std::make_unique<
                op_validate::ReduceMinValidate<HalPlatform::Model, HalPlatform::Operation>>(
                model, operation);
            return reduceMin->Validate(reason);
        }

        case OperationType::REDUCE_PROD: {
            OperationValidatePtr reduceProd = std::make_unique<
                op_validate::ReduceProdValidate<HalPlatform::Model, HalPlatform::Operation>>(
                model, operation);
            return reduceProd->Validate(reason);
        }

        case OperationType::REDUCE_SUM: {
            OperationValidatePtr reduceSum = std::make_unique<
                op_validate::ReduceSumValidate<HalPlatform::Model, HalPlatform::Operation>>(
                model, operation);
            return reduceSum->Validate(reason);
        }

        case OperationType::NEG: {
            OperationValidatePtr neg = std::make_unique<
                op_validate::NegValidate<HalPlatform::Model, HalPlatform::Operation>>(model,
                                                                                      operation);
            return neg->Validate(reason);
        }

        case OperationType::PRELU: {
            OperationValidatePtr prelu = std::make_unique<
                op_validate::PreluValidate<HalPlatform::Model, HalPlatform::Operation>>(model,
                                                                                        operation);
            return prelu->Validate(reason);
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
            return compare->Validate(reason);
        }

        case OperationType::LOGICAL_AND: {
            OperationValidatePtr logicalAnd = std::make_unique<
                op_validate::LogicalAndValidate<HalPlatform::Model, HalPlatform::Operation>>(
                model, operation);
            return logicalAnd->Validate(reason);
        }
        case OperationType::LOGICAL_NOT: {
            OperationValidatePtr logicalNot = std::make_unique<
                op_validate::LogicalNotValidate<HalPlatform::Model, HalPlatform::Operation>>(
                model, operation);
            return logicalNot->Validate(reason);
        }
        case OperationType::LOGICAL_OR: {
            OperationValidatePtr logicalOr = std::make_unique<
                op_validate::LogicalOrValidate<HalPlatform::Model, HalPlatform::Operation>>(
                model, operation);
            return logicalOr->Validate(reason);
        }
        case OperationType::EXPAND_DIMS: {
            OperationValidatePtr expandDims = std::make_unique<
                op_validate::ExpandDimsValidate<HalPlatform::Model, HalPlatform::Operation>>(
                model, operation);
            return expandDims->Validate(reason);
        }
        case OperationType::POW: {
            OperationValidatePtr pow = std::make_unique<
                op_validate::PowValidate<HalPlatform::Model, HalPlatform::Operation>>(model,
                                                                                      operation);
            return pow->Validate(reason);
        }
        case OperationType::INSTANCE_NORMALIZATION: {
            OperationValidatePtr instanceNorm = std::make_unique<
                op_validate::InstanceNormValidate<HalPlatform::Model, HalPlatform::Operation>>(
                model, operation);
            return instanceNorm->Validate(reason);
        }
        case OperationType::SPLIT: {
            OperationValidatePtr split = std::make_unique<
                op_validate::SplitValidate<HalPlatform::Model, HalPlatform::Operation>>(model,
                                                                                        operation);
            return split->Validate(reason);
        }
        case OperationType::LOG_SOFTMAX: {
            OperationValidatePtr logSoftmax = std::make_unique<
                op_validate::LogSoftmaxValidate<HalPlatform::Model, HalPlatform::Operation>>(
                model, operation);
            return logSoftmax->Validate(reason);
        }
        case OperationType::REDUCE_ALL: {
            OperationValidatePtr reduceAll = std::make_unique<
                op_validate::ReduceAllValidate<HalPlatform::Model, HalPlatform::Operation>>(
                model, operation);
            return reduceAll->Validate(reason);
        }
        case OperationType::REDUCE_ANY: {
            OperationValidatePtr reduceAny = std::make_unique<
                op_validate::ReduceAnyValidate<HalPlatform::Model, HalPlatform::Operation>>(
                model, operation);
            return reduceAny->Validate(reason);
        }
        case OperationType::GATHER: {
            OperationValidatePtr gather = std::make_unique<
                op_validate::GatherValidate<HalPlatform::Model, HalPlatform::Operation>>(model,
                                                                                         operation);
            return gather->Validate(reason);
        }

        case OperationType::AXIS_ALIGNED_BBOX_TRANSFORM: {
            OperationValidatePtr axisAlignedBBoxTransform = std::make_unique<
                op_validate::AxisAlignedBBoxTransformValidate<HalPlatform::Model,
                                                              HalPlatform::Operation>>(model,
                                                                                       operation);
            return axisAlignedBBoxTransform->Validate(reason);
        }
        case OperationType::UNIDIRECTIONAL_SEQUENCE_LSTM: {
            // All generated cases failed
            reason += "reject UNIDIRECTIONAL_SEQUENCE_LSTM \n";
            return false;
            // OperationValidatePtr unidirectionalSequenceLstm = std::make_unique<
            //    op_validate::UnidirectionalSequenceLstmValidate<HalPlatform::Model,
            //    HalPlatform::Operation>>(model,
            //                                                                          operation);

            // return unidirectionalSequenceLstm->Validate(reason);
        }
        case OperationType::BIDIRECTIONAL_SEQUENCE_LSTM: {
            // All generated cases failed, need to fix
            reason += "reject BIDIRECTIONAL_SEQUENCE_LSTM\n";
            return false;
            // OperationValidatePtr bidirectionalSequenceLstm = std::make_unique<
            //    op_validate::BidirectionalSequenceLstmValidate<HalPlatform::Model,
            //    HalPlatform::Operation>>(model,
            //                                                                          operation);
            // return bidirectionalSequenceLstm->Validate(reason);
        }
        case OperationType::GENERATE_PROPOSALS: {
            // Some generated float32 cases failed
            return false;
            // OperationValidatePtr generateProposals = std::make_unique<
            //    op_validate::GenerateProposalsValidate<HalPlatform::Model,
            //    HalPlatform::Operation>>(model,
            //                                                                          operation);
            // return generateProposals->Validate(reason);
        }
        case OperationType::DETECTION_POSTPROCESSING: {
            // Some generated float32 cases crashed
            return false;
            // OperationValidatePtr detectionPostprocessing = std::make_unique<
            //    op_validate::DetectionPostprocessingValidate<HalPlatform::Model,
            //    HalPlatform::Operation>>(model,
            //                                                                          operation);

            // return detectionPostprocessing->Validate(reason);
        }
        case OperationType::UNIDIRECTIONAL_SEQUENCE_RNN: {
            OperationValidatePtr unidirectionalSequenceRnn = std::make_unique<
                op_validate::UnidirectionalSequenceRnnValidate<HalPlatform::Model,
                                                               HalPlatform::Operation>>(model,
                                                                                        operation);
            return unidirectionalSequenceRnn->Validate(reason);
        }
        case OperationType::BIDIRECTIONAL_SEQUENCE_RNN: {
            // All generated cases failed, need to fix
            return false;
            // OperationValidatePtr bidirectionalSequenceRnn = std::make_unique<
            //    op_validate::BidirectionalSequenceRnnValidate<HalPlatform::Model,
            //    HalPlatform::Operation>>(model,
            //                                                                          operation);
            // return bidirectionalSequenceRnn->Validate(reason);
        }
        case OperationType::ROI_ALIGN: {
            OperationValidatePtr roiAlign = std::make_unique<
                op_validate::RoiAlignValidate<HalPlatform::Model, HalPlatform::Operation>>(
                model, operation);
            return roiAlign->Validate(reason);
        }
        case OperationType::TOPK_V2: {
            return false;
            // OperationValidatePtr topkV2 = std::make_unique<
            //    op_validate::TopkV2Validate<HalPlatform::Model, HalPlatform::Operation>>(
            //    model, operation);
            // return topkV2->Validate(reason);
        }
        case OperationType::CAST: {
            OperationValidatePtr cast = std::make_unique<
                op_validate::CastValidate<HalPlatform::Model, HalPlatform::Operation>>(model,
                                                                                       operation);
            return cast->Validate(reason);
        }
        case OperationType::QUANTIZE: {
            OperationValidatePtr quantize = std::make_unique<
                op_validate::QuantizeValidate<HalPlatform::Model, HalPlatform::Operation>>(
                model, operation);
            return quantize->Validate(reason);
        }
        case OperationType::SELECT: {
            OperationValidatePtr select = std::make_unique<
                op_validate::SelectValidate<HalPlatform::Model, HalPlatform::Operation>>(model,
                                                                                         operation);
            return select->Validate(reason);
        }
        case OperationType::RANDOM_MULTINOMIAL: {
            OperationValidatePtr random = std::make_unique<
                op_validate::RandomMultinomialValidate<HalPlatform::Model, HalPlatform::Operation>>(
                model, operation);
            return random->Validate(reason);
        }
        case OperationType::HEATMAP_MAX_KEYPOINT: {
            OperationValidatePtr heatmap =
                std::make_unique<op_validate::HeatmapMaxKeypointValidate<HalPlatform::Model,
                                                                         HalPlatform::Operation>>(
                    model, operation);
            return heatmap->Validate(reason);
        }
        case OperationType::CHANNEL_SHUFFLE: {
            OperationValidatePtr channelShuffle = std::make_unique<
                op_validate::ChannelShuffleValidate<HalPlatform::Model, HalPlatform::Operation>>(
                model, operation);
            return channelShuffle->Validate(reason);
        }
        case OperationType::GROUPED_CONV_2D: {
            OperationValidatePtr groupedConv2D = std::make_unique<
                op_validate::GroupedConv2DValidate<HalPlatform::Model, HalPlatform::Operation>>(
                model, operation);
            return groupedConv2D->Validate(reason);
        }
        case OperationType::TRANSPOSE_CONV_2D: {
            OperationValidatePtr transposeConv2D = std::make_unique<
                op_validate::TransposeConv2dValidate<HalPlatform::Model, HalPlatform::Operation>>(
                model, operation);
            return transposeConv2D->Validate(reason);
        }
        case OperationType::RESHAPE: {
            OperationValidatePtr reshape = std::make_unique<
                op_validate::ReshapeValidate<HalPlatform::Model, HalPlatform::Operation>>(
                model, operation);
            return reshape->Validate(reason);
        }
        case OperationType::DEPTHWISE_CONV_2D: {
            OperationValidatePtr depthwiseConv2d = std::make_unique<
                op_validate::DepthwiseConv2dValidate<HalPlatform::Model, HalPlatform::Operation>>(
                model, operation);
            return depthwiseConv2d->Validate(reason);
        }
        case OperationType::BOX_WITH_NMS_LIMIT:
        case OperationType::PAD_V2:
        case OperationType::QUANTIZED_16BIT_LSTM:
        case OperationType::ROI_POOLING:
        case OperationType::SLICE:
        case OperationType::TILE:
            isSupport &= false;
            break;
#endif
#if ANDROID_SDK_VERSION >= 28
        default:
            isSupport &= true;
    }
#endif
#if ANDROID_SDK_VERSION >= 29

    // Overall check
    // TODO: if all of LSTM's inputs are constant, the result will fail.
    std::vector<OperationType> whiteList = { OperationType::CONCATENATION,
                                            OperationType::LSTM};

    // do not support constant tensor as operation's Input except whiteList.
    if (std::find(whiteList.begin(), whiteList.end(), operation.type) == whiteList.end()) {
        if (isConstantTensor(model.operands[operation.inputs[0]])) {
            reason += "reject op because its input[0] is constant tensor\n";
            isSupport &= false;
        }
    }

    // TODO: nnapi 1.2 new operand type
    for (size_t i = 0; i < operation.inputs.size(); i++) {
        auto& operand = model.operands[operation.inputs[i]];
        if (false == checkSupportedOperand(operand)) {
            reason += "reject op because its operand data type is not supported yet\n";
            isSupport &= false;
        }
    }

    for (size_t i = 0; i < operation.outputs.size(); i++) {
        auto& operand = model.operands[operation.outputs[i]];
        if (false == checkSupportedOperand(operand)) {
            reason += "reject op because its operand data type is not supported yet\n";
            isSupport &= false;
        }
    }
#endif

#if ANDROID_SDK_VERSION >= 28
    // Not support dynamic shape
    // Check inputs
    if (0 == operation.inputs.size()) {
        reason += "reject op because no input\n";
        isSupport &= false;
    }
    for (auto inIdx : operation.inputs) {
        auto& dims = model.operands[inIdx].dimensions;
        if (isTensor(model.operands[inIdx]) && dims.size() == 0) {
            isSupport &= false;
            reason += "reject op because its input tensor rank = 0\n";
        }
        if (dims.size() > 6) {
            isSupport &= false;
            reason += "reject op because its input rank > 6\n";
        }
        for (auto dim : dims) {
            if (dim == 0) {
                reason +=
                    "reject op because its input shape not determined which require shape "
                    "inference\n";
                isSupport &= false;
            }
        }
    }

    if (0 == operation.outputs.size()) {
        reason += "reject op because no output\n";
        isSupport = false;
    }
    for (size_t i = 0; i < operation.outputs.size(); i++) {
        auto& dims = model.operands[operation.outputs[i]].dimensions;
        if (isTensor(model.operands[operation.outputs[i]]) && dims.size() == 0) {
            isSupport &= false;
            reason += "reject op because its output tensor rank = 0\n";
        }
        if (dims.size() > 6) {
            isSupport &= false;
            reason += "reject op because its output rank > 6\n";
        }
        for (auto dimIndex : dims)
            if (dimIndex == 0) {
                reason +=
                    "reject op because its output shape not determined which require shape "
                    "inference\n";
                isSupport &= false;
            }
    }
    return isSupport;
#endif
    return true;
}

}  // namespace vsi_driver
}  // namespace nn
}  // namespace android

using android::sp;
using android::nn::vsi_driver::VsiDriver;

int main() {
    sp<VsiDriver> driver(new VsiDriver());
    return driver->run();
}
