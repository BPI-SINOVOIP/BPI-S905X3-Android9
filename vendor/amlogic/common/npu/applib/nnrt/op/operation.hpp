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
#ifndef __OVXLIB_OPERATION_H__
#define __OVXLIB_OPERATION_H__

#include <algorithm>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <vector>
#include "nnrt/model.hpp"
#include "nnrt/float16.hpp"
#include "nnrt/logging.hpp"
#include "nnrt/permute_vector.hpp"
#include "operand.hpp"
#include "ILayoutInference.hpp"

namespace nnrt {
class Model;
namespace op {

struct VxParam {
    OverflowPolicy overflowPolicy;
    RoundingPolicy roundingPolicy;
    Rounding downScaleSizeRounding;
    uint32_t accumulatorBits;
};

class Operation;
using OperationPtr = std::shared_ptr<Operation>;

struct PermuteOperation;

class Operation : nnrt::layout_inference::ILayoutInference {
   public:
    Operation(OperationType type);

    virtual ~Operation() {}

    OperationType type() { return type_; }

    void setType(OperationType type) { type_ = type; }

    std::vector<uint32_t>& inputs() { return inputs_; }

    std::vector<uint32_t>& outputs() { return outputs_; }

    size_t inputNum() { return inputs_.size(); }

    size_t outputNum() { return outputs_.size(); }

    uint32_t input(uint32_t index);

    uint32_t output(uint32_t index);

    void setInputs(const uint32_t* inputs, uint32_t input_size);

    void setOutputs(const uint32_t* outputs, uint32_t output_size);

    void setInputs(const std::vector<uint32_t>& inputs);

    void setOutputs(const std::vector<uint32_t>& outputs);

    bool replaceOutputs(uint32_t org_index, uint32_t new_index);

    bool replaceInputs(uint32_t org_index, uint32_t new_index);

    int find_position(std::vector<uint32_t> operands_indexes, uint32_t index);

    void echo(uint32_t index = 0);

    void setVxParam(OverflowPolicy overflow_policy = OverflowPolicy::WRAP,
                    RoundingPolicy rounding_policy = RoundingPolicy::TO_ZERO,
                    Rounding down_scale_size_rounding = Rounding::FLOOR,
                    uint32_t accumulator_bits = 0);

    VxParam& vxParam() { return vx_param_; }

    void setFusedType(FusedType fuse_type) { fused_type_ = fuse_type; }

    FusedType fusedType() { return fused_type_; }

    void setDataLayout(DataLayout layout) { operand_layout_ = layout; }
    DataLayout getDataLayout() { return operand_layout_; }

    virtual std::unordered_map<uint32_t, nnrt::layout_inference::IPermuteVectorPtr> layoutInference(
        nnrt::Model& model,
        const std::unordered_map<uint32_t, nnrt::layout_inference::IPermuteVectorPtr>&
            operand_permute) override;

   protected:
    /**
     * @brief Cache Permute Vector for each Input Tensor
     *
     */
    struct InputTensorPermuteVectorCache {
        explicit InputTensorPermuteVectorCache(const Operation& op) : op_(op) {}

        bool add(Model& model,
                 const std::unordered_map<uint32_t, nnrt::layout_inference::IPermuteVectorPtr>&
                     permuteVecs) {
            std::for_each(
                permuteVecs.begin(),
                permuteVecs.end(),
                [this](const std::pair<uint32_t, nnrt::layout_inference::IPermuteVectorPtr>&
                           permuteVec) { cached_permutes_.insert(permuteVec); });

            return isAllInputTensorSetupWithPermute(model);
        }

        bool isAllInputTensorSetupWithPermute(Model& model) const;

        std::unordered_map<uint32_t, nnrt::layout_inference::IPermuteVectorPtr>
            cached_permutes_;  //!< key:= operand id
        const Operation& op_;  //!< referenced operation
    };

    InputTensorPermuteVectorCache input_permute_cache_;

    /**
     * @brief insert permute operation before input or after output for a opertion
     *
     * @param model
     * @param permuteOp : operation going to insert
     * @param appliedPermuteVec : the permute vector already applied to input data
     * @param beforeInOrAfterOut : true indicate insert permute op before input of current OP, else
     * insert
     * permute after output of current OP
     * @param operandId
     */
    void insertPermute(Model& model,
                       std::shared_ptr<PermuteOperation>& permuteOp,
                       const std::vector<uint32_t>& appliedPermuteVec,
                       bool beforeInOrAfterOut,
                       uint32_t operandId);

    /**
     * @brief
     *
     * @param model
     * @param constOperandIds
     * @param permVec
     */
    void permuteConstOperands(Model& model,
                              std::vector<uint32_t>& constOperandIds,
                              nnrt::layout_inference::IPermuteVectorPtr permVec);

    std::vector<uint32_t> dimensionTrans(std::vector<uint32_t>& orgDims,
                                         const std::vector<uint32_t> perm);

    virtual void handleLayoutInferenceOnInputs(
        Model& model,
        std::unordered_map<uint32_t, nnrt::layout_inference::IPermuteVectorPtr>&
            out_permute_vectors);
    void handleLayoutInferenceOnOutputs(
        Model& mode, std::unordered_map<uint32_t, nnrt::layout_inference::IPermuteVectorPtr>&);

   private:
    OperationType type_{OperationType::NONE};

    std::vector<uint32_t> inputs_;

    std::vector<uint32_t> outputs_;

    FusedType fused_type_{FusedType::NONE};

    VxParam vx_param_;

    DataLayout operand_layout_{DataLayout::NONE};
};

struct PermuteOperation : Operation {
    PermuteOperation() : Operation(OperationType::TRANSPOSE) {}
    std::vector<int32_t> perm;
};

namespace utils {
/**
 * @brief create Permute Operation
 *
 * Don't insert permute operation for 1-D tensor
 *
 * @param model which model will use this permute, input/output operand also added into model
 * @return OperationPtr return none-nullptr if permute is required else nullptr and nothing
 * changed in model
 */
inline std::shared_ptr<nnrt::op::PermuteOperation> asOp(
    const nnrt::layout_inference::IPermuteVectorPtr& permVec) {
    if (!permVec->isAligned()) {
        std::shared_ptr<nnrt::op::PermuteOperation> permute =
            std::make_shared<nnrt::op::PermuteOperation>();
        std::vector<uint32_t> permVal = permVec->asStdVec();
        permute->perm.assign(permVal.begin(), permVal.end());

        return permute;
    }
    return nullptr;
}

inline uint32_t axisMapTo(const nnrt::layout_inference::IPermuteVectorPtr perm, uint32_t axisVal) {
    for (uint32_t i = 0; i < perm->rank(); i++) {
        if (axisVal == perm->at(i)) {
            return i;
        }
    }
    NNRT_LOGE_PRINT("Cannot find the axis val");
    assert(false);
    return perm->rank() - 1;
}

inline std::vector<int32_t> permuteArray(std::vector<int32_t>& array,
                                         nnrt::layout_inference::IPermuteVectorPtr permVec) {
    std::vector<uint32_t> permVal;
    for (auto i = 0U; i < permVec->rank(); ++i) {
        permVal.push_back(permVec->at(i));
    }
    std::vector<int32_t> tmp;
    for (uint32_t i = 0; i < permVal.size(); i++) {
        tmp.push_back(array[permVal[i]]);
    }
    return tmp;
}
}

struct ReshapeOperation : Operation {
    ReshapeOperation() : Operation(OperationType::RESHAPE) {}

    std::vector<int32_t> shape;
};

struct ConcatOperation : Operation {
    ConcatOperation() : Operation(OperationType::CONCATENATION) {}
    virtual void handleLayoutInferenceOnInputs(
        Model& model,
        std::unordered_map<uint32_t, nnrt::layout_inference::IPermuteVectorPtr>&
            out_permute_vectors) override;
    int32_t axis{-1};
};

struct SplitOperation : Operation {
    SplitOperation() : Operation(OperationType::SPLIT) {}
    virtual void handleLayoutInferenceOnInputs(
        Model& model,
        std::unordered_map<uint32_t, nnrt::layout_inference::IPermuteVectorPtr>&
            out_permute_vectors) override;
    int32_t axis{-1};
    int32_t split_number;
    std::vector<int32_t> slices;
};

struct SqueezeOperation : Operation {
    SqueezeOperation() : Operation(OperationType::SQUEEZE) {}
    std::vector<int32_t> axes;
};
struct ExpandDimsOperation : Operation {
    ExpandDimsOperation() : Operation(OperationType::EXPAND_DIMS) {}
    int axis;
};

struct SoftmaxOperation : Operation {
    SoftmaxOperation() : Operation(OperationType::SOFTMAX) {}
    virtual void handleLayoutInferenceOnInputs(
        Model& model,
        std::unordered_map<uint32_t, nnrt::layout_inference::IPermuteVectorPtr>&
            out_permute_vectors) override;
    float beta{1.0f};
    int axis{-1};
};

struct PadOperation : Operation {
    PadOperation() : Operation(OperationType::PAD) {}
    virtual void handleLayoutInferenceOnInputs(
        Model& model,
        std::unordered_map<uint32_t, nnrt::layout_inference::IPermuteVectorPtr>&
            out_permute_vectors) override;
    std::vector<int32_t> padFront;
    std::vector<int32_t> padBack;
    float padValue{0.0f};
    PadMode padMode{PadMode::CONSTANT};
};

template <typename DType>
struct PadV2Operation : Operation {
    PadV2Operation() : Operation(OperationType::PAD_V2) {}
    virtual void handleLayoutInferenceOnInputs(
        Model& model,
        std::unordered_map<uint32_t, nnrt::layout_inference::IPermuteVectorPtr>&
            next_permute_vectors) override {
        assert(input_permute_cache_.cached_permutes_.size() == 1);
        OperandPtr inputOperand = model.operand(inputs()[0]);
        OperandPtr outputOperand = model.operand(outputs()[0]);

        nnrt::layout_inference::IPermuteVectorPtr permuteVector =
            input_permute_cache_.cached_permutes_[inputs()[0]];

        if (inputOperand->ndim() != 4) {
            Operation::handleLayoutInferenceOnInputs(model, next_permute_vectors);
            auto reversePermVec = permuteVector->reverse();
            return;
        }

        // {0, 1, 2, 3}
        auto requiredPermute = nnrt::layout_inference::make_shared(inputOperand->ndim());
        if (DataLayout::NHWC == getDataLayout()) {
            requiredPermute = std::make_shared<nnrt::layout_inference::PermuteVector<4>>(
                std::initializer_list<uint32_t>({0, 3, 1, 2}));
            padFront = nnrt::op::utils::permuteArray(padFront, requiredPermute);
            padBack = nnrt::op::utils::permuteArray(padBack, requiredPermute);
        }

        auto finalPermute = permuteVector->reverse()->add(requiredPermute);
        auto permuteOp = nnrt::op::utils::asOp(finalPermute);

        if (permuteOp) {
            insertPermute(model, permuteOp, finalPermute->asStdVec(), true, inputs()[0]);
        }

        next_permute_vectors.insert(std::make_pair(outputs()[0], requiredPermute));
    };
    std::vector<int32_t> padFront;
    std::vector<int32_t> padBack;
    DType padValue;
    PadMode padMode{PadMode::CONSTANT};
};

struct ResizeBilinearOperation : Operation {
    ResizeBilinearOperation() : Operation(OperationType::RESIZE_BILINEAR) {}

    virtual void handleLayoutInferenceOnInputs(
        Model& model,
        std::unordered_map<uint32_t, nnrt::layout_inference::IPermuteVectorPtr>&
            out_permute_vectors) override;
    int outputHeight;
    int outputWidth;
};

struct ResizeNearestNeighborOperation : Operation {
    ResizeNearestNeighborOperation() : Operation(OperationType::RESIZE_NEAREST) {}

    virtual void handleLayoutInferenceOnInputs(
        Model& model,
        std::unordered_map<uint32_t, nnrt::layout_inference::IPermuteVectorPtr>&
            out_permute_vectors) override;
    int outputHeight;
    int outputWidth;
};

struct MatrixMulOperation : Operation {
    MatrixMulOperation() : Operation(OperationType::MATRIX_MUL) {}
    bool transpose[2];
};

struct SvdfOperation : Operation {
    SvdfOperation() : Operation(OperationType::SVDF) {}
    int32_t rank;
};

struct LstmUnitOperation : Operation {
    LstmUnitOperation() : Operation(OperationType::LSTM_UNIT) {}
    FusedType activation{FusedType::TANH};
    float cellClip{0.0f};
    float projClip{0.0f};
    float forgetBias{0.0f};

    static const uint8_t INPUT_COUNT = 24;
    static const uint8_t OUTPUT_COUNT = 4;
};

struct RnnOperation : Operation {
    RnnOperation() : Operation(OperationType::RNN) {}

    FusedType activation{FusedType::SIGMOID};
};

struct UnidirectionalSequenceRnnOperation : Operation {
    UnidirectionalSequenceRnnOperation() : Operation(OperationType::UNIDIRECTIONAL_SEQUENCE_RNN) {}

    FusedType activation{FusedType::SIGMOID};
    bool timeMajor;
};

struct BidirectionalSequenceRnnOperation : Operation {
    BidirectionalSequenceRnnOperation() : Operation(OperationType::BIDIRECTIONAL_SEQUENCE_RNN) {}

    FusedType activation{FusedType::SIGMOID};
    bool timeMajor;
    bool mergeOutputs;
};

struct UnidirectionalSequenceLstmOperation : Operation {
    UnidirectionalSequenceLstmOperation()
        : Operation(OperationType::UNIDIRECTIONAL_SEQUENCE_LSTM) {}

    FusedType activation{FusedType::TANH};
    bool timeMajor;
    float cell_clip;
    float proj_clip;
};

struct BidirectionalSequenceLstmOperation : Operation {
    BidirectionalSequenceLstmOperation() : Operation(OperationType::BIDIRECTIONAL_SEQUENCE_LSTM) {}

    FusedType activation{FusedType::TANH};
    bool timeMajor;
    bool mergeOutputs;
    float cell_clip;
    float proj_clip;
};

struct DepthToSpaceOperation : Operation {
    DepthToSpaceOperation() : Operation(OperationType::DEPTH_TO_SPACE) {}
    virtual void handleLayoutInferenceOnInputs(
        Model& model,
        std::unordered_map<uint32_t, nnrt::layout_inference::IPermuteVectorPtr>&
            out_permute_vectors);
    int32_t blockSize[2];
};

struct SpaceToDepthOperation : Operation {
    SpaceToDepthOperation() : Operation(OperationType::SPACE_TO_DEPTH) {}
    virtual void handleLayoutInferenceOnInputs(
        Model& model,
        std::unordered_map<uint32_t, nnrt::layout_inference::IPermuteVectorPtr>&
            out_permute_vectors);
    int32_t blockSize[2];
};

struct BatchToSpaceNDOperation : Operation {
    BatchToSpaceNDOperation() : Operation(OperationType::BATCH_TO_SPACE_ND) {}
    virtual void handleLayoutInferenceOnInputs(
        Model& model,
        std::unordered_map<uint32_t, nnrt::layout_inference::IPermuteVectorPtr>&
            out_permute_vectors);
    std::vector<int32_t> blockSize;
    std::vector<int32_t> cropStart;
    std::vector<int32_t> cropEnd;
};

struct SpaceToBatchNDOperation : Operation {
    SpaceToBatchNDOperation() : Operation(OperationType::SPACE_TO_BATCH_ND) {}
    virtual void handleLayoutInferenceOnInputs(
        Model& model,
        std::unordered_map<uint32_t, nnrt::layout_inference::IPermuteVectorPtr>&
            out_permute_vectors);
    std::vector<int32_t> blockSize;
    std::vector<int32_t> padFront;
    std::vector<int32_t> padBack;
};

struct StridedSliceOperation : Operation {
    StridedSliceOperation() : Operation(OperationType::STRIDED_SLICE) {}
    virtual void handleLayoutInferenceOnInputs(
        Model& model,
        std::unordered_map<uint32_t, nnrt::layout_inference::IPermuteVectorPtr>&
            out_permute_vectors);
    std::vector<int32_t> starts;
    std::vector<int32_t> ends;
    std::vector<int32_t> strides;
    int32_t beginMask;
    int32_t endMask;
    int32_t shrinkAxisMask;
};

struct SliceOperation : Operation {
    SliceOperation() : Operation(OperationType::SLICE) {}
    std::vector<int32_t> starts;
    std::vector<int32_t> sizes;
};

struct ReverseOperation : Operation {
    ReverseOperation() : Operation(OperationType::REVERSE) {}
    std::vector<int32_t> axes;
};

struct LshProjectionOperation : Operation {
    LshProjectionOperation() : Operation(OperationType::LSH_PROJECTION) {}
    LshProjectionType type{LshProjectionType::SPARSE};
};

struct ArgmaxOperation : Operation {
    ArgmaxOperation() : Operation(OperationType::ARGMAX) {}
    virtual void handleLayoutInferenceOnInputs(
        Model& model,
        std::unordered_map<uint32_t, nnrt::layout_inference::IPermuteVectorPtr>&
            out_permute_vectors) override;
    int axis{0};
};

struct ArgminOperation : Operation {
    ArgminOperation() : Operation(OperationType::ARGMIN) {}
    virtual void handleLayoutInferenceOnInputs(
        Model& model,
        std::unordered_map<uint32_t, nnrt::layout_inference::IPermuteVectorPtr>&
            out_permute_vectors) override;
    int axis{0};
};

struct ChannelShuffleOperation : Operation {
    ChannelShuffleOperation() : Operation(OperationType::CHANNEL_SHUFFLE) {}
    virtual void handleLayoutInferenceOnInputs(
        Model& model,
        std::unordered_map<uint32_t, nnrt::layout_inference::IPermuteVectorPtr>&
            out_permute_vectors) override;
    int groups{1};
    int axis{0};
};

struct GatherOperation : Operation {
    GatherOperation() : Operation(OperationType::GATHER) {}
    int axis{-1};
    std::vector<int> indices;
};

struct ROIAlignOperation : Operation {
    ROIAlignOperation() : Operation(OperationType::ROI_ALIGN) {}
    virtual void handleLayoutInferenceOnInputs(
        Model& model,
        std::unordered_map<uint32_t, nnrt::layout_inference::IPermuteVectorPtr>&
            out_permute_vectors) override;
    int height;
    int width;
    float height_ratio;
    float width_ratio;
    int sampling_points_height;
    int sampling_points_width;
};

struct HeatmapMaxKeypointOperation : Operation {
    HeatmapMaxKeypointOperation() : Operation(OperationType::HEATMAP_MAX_KEYPOINT) {}
    virtual void handleLayoutInferenceOnInputs(
        Model& model,
        std::unordered_map<uint32_t, nnrt::layout_inference::IPermuteVectorPtr>&
            out_permute_vectors) override;
};

struct ROIPoolingOperation : Operation {
    ROIPoolingOperation() : Operation(OperationType::ROI_POOLING) {}
    int height;
    int width;
    float height_ratio;
    float width_ratio;
};

struct DetectionPostprocessingOperation : Operation {
    DetectionPostprocessingOperation() : Operation(OperationType::DETECTION_POSTPROCESSING) {}
    float dy;
    float dx;
    float dh;
    float dw;
    bool nms_type;
    int max_num_detections;
    int maximum_class_per_detection;
    int maximum_detection_per_class;
    float score_threshold;
    float iou_threshold;
    bool is_bg_in_label;
};

struct GenerateProposalsOperation : Operation {
    GenerateProposalsOperation() : Operation(OperationType::GENERATE_PROPOSALS) {}
    virtual void handleLayoutInferenceOnInputs(
        Model& model,
        std::unordered_map<uint32_t, nnrt::layout_inference::IPermuteVectorPtr>&
            out_permute_vectors) override;
    float ratio_h;
    float ratio_w;
    int pre_nms_topn;
    int post_nms_topn;
    float iou_threshold;
    float min_size;
};

struct RandomMultinomialOperation : Operation {
    RandomMultinomialOperation() : Operation(OperationType::RANDOM_MULTINOMIAL) {}
    int sample_num;
};

struct TopkOperation : Operation {
    TopkOperation() : Operation(OperationType::TOPK) {}
    int k{1};
};

struct TileOperation : Operation {
    TileOperation() : Operation(OperationType::TILE) {}
    std::vector<int32_t> multiples;
};

struct BoxWithNmsLimitOperation : Operation {
    BoxWithNmsLimitOperation() : Operation(OperationType::BOX_WITH_NMS_LIMIT) {}
    float score_threshold;
    int32_t max_boxes;
    NmsKernelMethod nms_kernel_method;
    float iou_threshold;
    float nms_sigma;
    float nms_score_threshold;
};

struct LogSoftmaxOperation : Operation {
    LogSoftmaxOperation() : Operation(OperationType::LOG_SOFTMAX) {}

    float beta;
    int32_t axis;
};

#define DECLARE_OPERATION(name, type)                         \
    struct name##Operation : Operation {                      \
        name##Operation() : Operation(OperationType::type) {} \
    }

DECLARE_OPERATION(Floor, FLOOR);
DECLARE_OPERATION(Quantize, QUANTIZE);
DECLARE_OPERATION(Dequantize, DEQUANTIZE);
DECLARE_OPERATION(Noop, NOOP);
DECLARE_OPERATION(Pow, POW);
DECLARE_OPERATION(HashtableLookup, HASHTABLE_LOOKUP);
DECLARE_OPERATION(EmbeddingLookup, EMBEDDING_LOOKUP);
DECLARE_OPERATION(ImageProcess, IMAGE_PROCESS);
DECLARE_OPERATION(FullyConnected, FULLY_CONNECTED);
DECLARE_OPERATION(DataConvert, DATA_CONVERT);
DECLARE_OPERATION(Equal, EQUAL);
DECLARE_OPERATION(Exp, EXP);
DECLARE_OPERATION(Greater, GREATER);
DECLARE_OPERATION(GreaterEqual, GREATER_EQUAL);
DECLARE_OPERATION(Less, LESS);
DECLARE_OPERATION(LessEqual, LESS_EQUAL);
DECLARE_OPERATION(Log, LOG);
DECLARE_OPERATION(LogicalAnd, LOGICAL_AND);
DECLARE_OPERATION(LogicalOr, LOGICAL_OR);
DECLARE_OPERATION(LogicalNot, LOGICAL_NOT);
DECLARE_OPERATION(Neg, NEG);
DECLARE_OPERATION(NotEqual, NOT_EQUAL);
DECLARE_OPERATION(Select, SELECT);
DECLARE_OPERATION(PRelu, PRELU);
DECLARE_OPERATION(Sin, SIN);
DECLARE_OPERATION(AxisAlignedBBoxTransform, AXIS_ALIGNED_BBOX_TRANSFORM);

#undef DECLARE_OPERATION
}
}

#endif
