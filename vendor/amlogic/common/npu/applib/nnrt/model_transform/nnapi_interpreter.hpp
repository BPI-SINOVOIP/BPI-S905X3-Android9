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
#ifndef __OVXLIB_NNAPI_INTERPRETER_H__
#define __OVXLIB_NNAPI_INTERPRETER_H__

#include <vector>
#include <set>
#include "nnrt/model.hpp"
#include "nnrt/op/public.hpp"
#include "nnrt/interpreter.hpp"
#include "nnrt/api_requirement/nnapi_requirement.hpp"

namespace nnrt
{
using namespace nnrt::op;
class NnApiInterpreter : public Interpreter
{
    public:
        NnApiInterpreter();
        virtual ~NnApiInterpreter();

        const char* name() override {return "NnApiInterpreter";}

        int run(Model* model, bool* modified) override;

        FusedType mapFusedType(int fused_code);

        PadType mapPadType(int code);

        FusedType mapLstmActivationType(int value);

        LshProjectionType mapLshProjectionType(int value);

        inline std::vector<int32_t> convertPermute(std::vector<int32_t> & perm) {
            return convertAxes(perm, perm.size());
        }

        inline std::vector<int32_t> convertPermute(int32_t* perm_buffer, size_t length) {
            return convertAxes(perm_buffer, length, length);
        }

        std::vector<int32_t> convertAxes(int32_t* axes_buffer, size_t length, size_t dim_num);

        std::vector<int32_t> convertAxes(std::vector<int32_t> & axes, size_t dim_num);

        inline int32_t convertAxis(int32_t axis, int32_t dim_num) {
            return (dim_num - computeAxis(axis, dim_num) - 1);
        }

        void fillIntArray(Model* model, OperationPtr operation,
                std::vector<int32_t>& array, int32_t op_index, bool is_axis);

        inline int32_t computeAxis(int32_t axis, int32_t dim_num) {
            if (axis >= 0) {
                return axis;
            } else {
                return dim_num + axis;
            }
        }

        void removeScalarOperand(OperationPtr& op, size_t ofst, size_t cnt);

        void truncateOperationIOs(Model* model, OperationPtr operation,
                int32_t input_num, int32_t output_num);

        inline void resetFusedType(Model* model, OperationPtr operation,
                int32_t input_index) {
            OperandPtr operand = model->operand(operation->input(input_index));
            operation->setFusedType(mapFusedType(operand->scalar.int32));
        }

        void replaceOperation(Model* model, uint32_t op_index,
                OperationPtr new_operation);

#define REGISTER_OP(NAME)   \
        OperationPtr map_##NAME(Model* model, OperationPtr operation, uint32_t)
        REGISTER_OP(ADD);
        REGISTER_OP(CONV_2D);
        REGISTER_OP(DEPTHWISE_CONV_2D);
        REGISTER_OP(RELU);
        REGISTER_OP(RESHAPE);
        REGISTER_OP(FULLY_CONNECTED);
        REGISTER_OP(TRANSPOSE);
        REGISTER_OP(CONCATENATION);
        REGISTER_OP(AVERAGE_POOL_2D);
        REGISTER_OP(SQUEEZE);
        REGISTER_OP(SOFTMAX);
        REGISTER_OP(MAX_POOL_2D);
        REGISTER_OP(PAD);
        REGISTER_OP(MUL);
        REGISTER_OP(MEAN);
        REGISTER_OP(RELU1);
        REGISTER_OP(RELU6);
        REGISTER_OP(SIGMOID);
        REGISTER_OP(TANH);
        REGISTER_OP(FLOOR);
        REGISTER_OP(DIV);
        REGISTER_OP(SUB);
        REGISTER_OP(QUANTIZE);
        REGISTER_OP(DEQUANTIZE);
        REGISTER_OP(SPACE_TO_DEPTH);
        REGISTER_OP(DEPTH_TO_SPACE);
        REGISTER_OP(SPACE_TO_BATCH_ND);
        REGISTER_OP(BATCH_TO_SPACE_ND);
        REGISTER_OP(L2_NORMALIZATION);
        REGISTER_OP(RESIZE_BILINEAR);
        REGISTER_OP(LOCAL_RESPONSE_NORMALIZATION);
        REGISTER_OP(EMBEDDING_LOOKUP);
        REGISTER_OP(RNN);
        REGISTER_OP(UNIDIRECTIONAL_SEQUENCE_RNN);
        REGISTER_OP(BIDIRECTIONAL_SEQUENCE_RNN);
        REGISTER_OP(HASHTABLE_LOOKUP);
        REGISTER_OP(LSTM);
        REGISTER_OP(UNIDIRECTIONAL_SEQUENCE_LSTM);
        REGISTER_OP(BIDIRECTIONAL_SEQUENCE_LSTM);
        REGISTER_OP(SVDF);
        REGISTER_OP(LSH_PROJECTION);
        REGISTER_OP(L2_POOL_2D);
        REGISTER_OP(STRIDED_SLICE);
        REGISTER_OP(RESIZE_NEAREST);
        REGISTER_OP(ABS);
        REGISTER_OP(ARGMAX);
        REGISTER_OP(ARGMIN);
        REGISTER_OP(EQUAL);
        REGISTER_OP(EXP);
        REGISTER_OP(EXPAND_DIMS);
        REGISTER_OP(GATHER);
        REGISTER_OP(CHANNEL_SHUFFLE);
        REGISTER_OP(GREATER);
        REGISTER_OP(GREATER_EQUAL);
        REGISTER_OP(GROUPED_CONV_2D);
        REGISTER_OP(INSTANCE_NORMALIZATION);
        REGISTER_OP(LESS);
        REGISTER_OP(LESS_EQUAL);
        REGISTER_OP(LOG);
        REGISTER_OP(LOGICAL_AND);
        REGISTER_OP(LOGICAL_OR);
        REGISTER_OP(LOGICAL_NOT);
        REGISTER_OP(MAXIMUM);
        REGISTER_OP(MINIMUM);
        REGISTER_OP(NEG);
        REGISTER_OP(NOT_EQUAL);
        REGISTER_OP(POW);
        REGISTER_OP(PRELU);
        REGISTER_OP(ROI_ALIGN);
        REGISTER_OP(ROI_POOLING);
        REGISTER_OP(SQRT);
        REGISTER_OP(RSQRT);
        REGISTER_OP(SELECT);
        //REGISTER_OP(SLICE);
        REGISTER_OP(SPLIT);
        REGISTER_OP(DECONV_2D);
        REGISTER_OP(SIN);
        REGISTER_OP(REDUCE_ALL);
        REGISTER_OP(REDUCE_ANY);
        REGISTER_OP(REDUCE_MAX);
        REGISTER_OP(REDUCE_MIN);
        REGISTER_OP(REDUCE_SUM);
        REGISTER_OP(REDUCE_PROD);
        REGISTER_OP(AXIS_ALIGNED_BBOX_TRANSFORM);
        REGISTER_OP(GENERATE_PROPOSALS);
        REGISTER_OP(RANDOM_MULTINOMIAL);
        REGISTER_OP(HEATMAP_MAX_KEYPOINT);
        REGISTER_OP(BOX_WITH_NMS_LIMIT);
        REGISTER_OP(LOG_SOFTMAX);
        REGISTER_OP(TOPK);
        REGISTER_OP(DETECTION_POSTPROCESSING);
        REGISTER_OP(TILE);
        REGISTER_OP(PAD_V2);
#undef  REGISTER_OP
    private:
        inline DataLayout getDataLayout(bool isNCHW) {
            return isNCHW ? DataLayout::NCHW : DataLayout::NHWC;
        }

        inline const api::requirement::MatchedArgumentPtr matchArgList(
                std::vector<OperandPtr>& operands, const std::string& opName) {
            std::vector<OperandType> argTypes;
            std::transform(operands.begin(), operands.end(),
                    std::back_inserter(argTypes), [](const OperandPtr& operand){
                return operand->type;
            });
            return api::requirement::nnapi::match(opName, argTypes);
        }

        typedef OperationPtr (NnApiInterpreter::*AddNodeFunc)(Model*, OperationPtr, uint32_t);
        std::map<OperationType, AddNodeFunc> op_container_;
        std::set<uint32_t> operands_to_remove_;
};
};
#endif
