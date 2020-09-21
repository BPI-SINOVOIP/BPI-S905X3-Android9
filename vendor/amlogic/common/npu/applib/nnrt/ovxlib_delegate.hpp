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
#ifndef __OVXLIB_DELEGATE_H__
#define __OVXLIB_DELEGATE_H__

#include "vsi_nn_pub.h"
#include "model.hpp"
#include "op/public.hpp"
#include "execution_io.hpp"

namespace nnrt
{

class OvxlibDelegate
{
    public:
        OvxlibDelegate(std::vector<ExecutionIOPtr> &inputPtr);
        virtual ~OvxlibDelegate();

        int process(nnrt::Model* model, vsi_nn_context_t ctx = nullptr);

        vsi_nn_graph_t* throwGraph();

        std::map<uint32_t, vsi_nn_tensor_id_t> getTensorMapping()const;

        vsi_nn_pad_e getPaddingType(PadType type);

        vsi_nn_round_type_e getRoundType(Rounding type);

        vsi_nn_op_t getActivation(FusedType fused_code);

        vsi_nn_type_e mapTensorType(OperandType code);

        int addNode(vsi_nn_op_t op,
                std::vector<uint32_t> & inputs,
                std::vector<uint32_t> & outputs, FusedType fused_code,
                std::vector<vsi_nn_node_t*>* output_nodes, uint32_t uid);

        int addNode(vsi_nn_op_t op, nnrt::op::OperationPtr operation,
                std::vector<vsi_nn_node_t*>* output_nodes, uint32_t uid) {
            return addNode(op, operation->inputs(),
                    operation->outputs(), operation->fusedType(), output_nodes, uid);
        }

        inline bool hasFusedCode(FusedType code) {
            return (code != FusedType::NONE);
        }

        void packTensorAttr(vsi_nn_tensor_attr_t* attr,
            vsi_nn_type_e dtype, std::vector<uint32_t> & nchw_shape,
            bool is_quantized, float scale, int32_t zero_point,
            TensorLifeTime type);

        void packTensorAttr(vsi_nn_tensor_attr_t* attr,
            nnrt::op::OperandPtr operand, TensorLifeTime type);

        void mapTensorId(uint32_t operand_id, vsi_nn_tensor_id_t tensor_id);

        int addTensor(vsi_nn_graph_t* graph, nnrt::op::OperandPtr operand,
                TensorLifeTime type, size_t idx, const void* data = nullptr, bool isFromHandle = false);

        int addTensor(vsi_nn_graph_t* graph, vsi_nn_type_e dtype,
            std::vector<uint32_t> & shape, bool is_quantized,
            float scale, int32_t zero_point, TensorLifeTime type, size_t idx,
            const void* data = nullptr, bool isFromHandle = false);

        int addTensor(vsi_nn_graph_t* graph,
            vsi_nn_tensor_attr_t* attr, size_t idx,
            const void* data = nullptr, bool isFromHandle = false);

        inline uint32_t newNodeUid() {
            node_unique_id_ --;
            return node_unique_id_;
        }

        vsi_nn_pad_mode_e mapPadMode(PadMode mode);

        std::vector<uint32_t> reorderOperands(
                std::vector<uint32_t>& operands, std::vector<int> order);

        std::vector<vsi_nn_tensor_id_t> getMappedTensors(
                std::vector<uint32_t> & operand_indexes);

        vsi_nn_tensor_id_t getMappedTensor(uint32_t operand_index);

        vsi_nn_lsh_projection_type_e mapLshProjectionType(LshProjectionType type);

        void fillVxParam(vsi_nn_vx_param_t* c_vx_param, nnrt::op::VxParam& vx_param);

        template<typename T>
        std::vector<T> reverseArray(std::vector<T> &data)
        {
            std::vector<T> buf(data.size());
            buf.assign(data.rbegin(), data.rend());
            return buf;
        };

        template<typename T>
        T *addParamPool(std::vector<T> &data, bool reverse = false)
        {
            std::vector<int8_t> handler(data.size() * sizeof(T));
            if (reverse == false) {
                memcpy(handler.data(), data.data(), data.size() * sizeof(T));
            }
            else {
                std::vector<T> reverse_buf = reverseArray(data);
                memcpy(handler.data(), reverse_buf.data(), data.size() * sizeof(T));
            }
            size_pool_.push_back(handler);
            return reinterpret_cast<T*>(size_pool_.back().data());
        };

        template<typename T>
        std::vector<T> convertPermute(std::vector<T> &perm)
        {
            return convertAxes(perm, perm.size());
        };

        template<typename T>
        std::vector<T> convertAxes(std::vector<T> &axes, size_t dim_num)
        {
            std::vector<T> new_axes(axes.size());
            size_t max_size = axes.size() - 1;
            for (size_t i = 0; i < axes.size(); ++i) {
                new_axes[i] = convertAxis(axes[max_size - i], dim_num);
            }
            return new_axes;
        };

        int32_t convertAxis(int32_t axis, int32_t dim_num)
        {
            if (axis < 0) {
                axis += dim_num;
            }
            return (dim_num - axis - 1);
        };

        int32_t reverseMask(int32_t mask, size_t dim_num)
        {
            auto get_bit_in_mask = [](int mask, int index) -> int {
                return (((int)0x1) << index) & mask;
            };
            int32_t new_mask = 0;
            for (int i = (int)dim_num - 1; i >= 0; --i) {
                new_mask |= (get_bit_in_mask(mask, i) >> i) << ((dim_num - 1) - i);
            }
            return new_mask;
        };

#define REGISTER_OP(NAME)   \
        int addNode_##NAME(nnrt::Model* model, nnrt::op::OperationPtr operation, uint32_t)
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
        REGISTER_OP(LEAKY_RELU);
        REGISTER_OP(SOFT_RELU);
        REGISTER_OP(SQRT);
        REGISTER_OP(SQUARE);
        REGISTER_OP(DIV);
        REGISTER_OP(SUB);
        REGISTER_OP(QUANTIZE);
        REGISTER_OP(DEQUANTIZE);
        REGISTER_OP(SPACE_TO_DEPTH);
        REGISTER_OP(DEPTH_TO_SPACE);
        REGISTER_OP(SPACE_TO_BATCH_ND);
        REGISTER_OP(BATCH_TO_SPACE_ND);
        REGISTER_OP(L2_NORM);
        REGISTER_OP(RESIZE_BILINEAR);
        REGISTER_OP(LOCAL_RESPONSE_NORM);
        REGISTER_OP(STRIDED_SLICE);
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
        REGISTER_OP(FLOOR);
        REGISTER_OP(BATCH_NORM);
        REGISTER_OP(MAXIMUM);
        REGISTER_OP(MINIMUM);
        REGISTER_OP(RSQRT);
        REGISTER_OP(PRELU);
        REGISTER_OP(RESIZE_NEAREST);
        REGISTER_OP(ABS);
        REGISTER_OP(ARGMAX);
        REGISTER_OP(ARGMIN);
        REGISTER_OP(EQUAL);
        REGISTER_OP(EXP);
        REGISTER_OP(GATHER);
        REGISTER_OP(CHANNEL_SHUFFLE);
        REGISTER_OP(GREATER);
        REGISTER_OP(GREATER_EQUAL);
        REGISTER_OP(GROUPED_CONV_2D);
        REGISTER_OP(INSTANCE_NORM);
        REGISTER_OP(LESS);
        REGISTER_OP(LESS_EQUAL);
        REGISTER_OP(LOGICAL_AND);
        REGISTER_OP(LOGICAL_OR);
        REGISTER_OP(LOGICAL_NOT);
        REGISTER_OP(LOG);
        REGISTER_OP(NEG);
        REGISTER_OP(NOT_EQUAL);
        REGISTER_OP(POW);
        REGISTER_OP(ROI_ALIGN);
        REGISTER_OP(ROI_POOLING);
        REGISTER_OP(SELECT);
        REGISTER_OP(SLICE);
        REGISTER_OP(SPLIT);
        REGISTER_OP(DECONV_2D);
        REGISTER_OP(SIN);
        REGISTER_OP(REDUCE_ALL);
        REGISTER_OP(REDUCE_ANY);
        REGISTER_OP(REDUCE_MAX);
        REGISTER_OP(REDUCE_MIN);
        REGISTER_OP(REDUCE_PROD);
        REGISTER_OP(REDUCE_SUM);
        REGISTER_OP(AXIS_ALIGNED_BBOX_TRANSFORM);
        REGISTER_OP(GENERATE_PROPOSALS);
        REGISTER_OP(RANDOM_MULTINOMIAL);
        REGISTER_OP(HEATMAP_MAX_KEYPOINT);
        REGISTER_OP(BOX_WITH_NMS_LIMIT);
        REGISTER_OP(LOG_SOFTMAX);
        REGISTER_OP(TOPK);
        REGISTER_OP(TILE);
        REGISTER_OP(DETECTION_POSTPROCESSING);
        REGISTER_OP(DATA_CONVERT);
        REGISTER_OP(PAD_V2);
        REGISTER_OP(LINEAR);
#undef  REGISTER_OP

    private:
        typedef int (OvxlibDelegate::*AddNodeFunc)(Model*, nnrt::op::OperationPtr, uint32_t);
        std::map<OperationType, AddNodeFunc> op_container_;
        uint32_t node_unique_id_;
        std::map<uint32_t, vsi_nn_tensor_id_t> tensor_map_;
        std::map<uint32_t, vsi_nn_node_id_t> node_map_;
        vsi_nn_graph_t* graph_{nullptr};
        std::vector<std::vector<int8_t>> size_pool_;
        std::vector<ExecutionIOPtr> inputs_;
};

inline std::map<uint32_t, vsi_nn_tensor_id_t> OvxlibDelegate::getTensorMapping()const{
    return tensor_map_;
}

}

#endif
