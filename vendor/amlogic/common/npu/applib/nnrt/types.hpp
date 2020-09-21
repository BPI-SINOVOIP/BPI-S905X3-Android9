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
#ifndef __OVXLIB_TYPES_H__
#define __OVXLIB_TYPES_H__

#include <cstdint>

#if defined(HALF_ROUND_STYLE)
#undef HALF_ROUND_STYLE
#endif

// Set nearest round policy
#define HALF_ROUND_STYLE 1
#include "float16.hpp"
namespace nnrt {

enum class OperationType: uint32_t {
    NONE,
    ADD,
    MUL,
    DIV,
    SUB,

    CONCATENATION,
    SPLIT,

    CONV_2D,
    DEPTHWISE_CONV_2D,
    FULLY_CONNECTED,
    CONV_1D,
    GROUPED_CONV_2D,
    //FCL2,

    AVERAGE_POOL_2D,
    MAX_POOL_2D,
    L2_POOL_2D,
    //CONV_RELU_POOL,
    DECONV_2D,
    TRANSPOSE_CONV_2D = DECONV_2D,
    //POOL_WITH_ARGMAX,
    ROI_POOLING,
    ROI_ALIGN,

    PAD,
    RELU,
    RELU1,
    RELU6,
    TANH,
    LEAKY_RELU,
    PRELU,
    SIGMOID, //LOGISTIC
    LOGISTIC = SIGMOID,
    SOFT_RELU,
    SOFTMAX,

    RESIZE_BILINEAR,
    RESIZE_NEAREST,
    RESIZE_NEAREST_NEIGHBOR = RESIZE_NEAREST,
    UNPOOL_2D,

    L2_NORM,
    L2_NORMALIZATION = L2_NORM,
    LOCAL_RESPONSE_NORM,
    LOCAL_RESPONSE_NORMALIZATION = LOCAL_RESPONSE_NORM,
    BATCH_NORM,
    //LAYER_NORM,
    //LOCAL_RESPONSE_NORM2,
    INSTANCE_NORM,
    INSTANCE_NORMALIZATION = INSTANCE_NORM,

    DATA_CONVERT,
    CAST = DATA_CONVERT,

    RESHAPE,
    SQUEEZE,
    TRANSPOSE,
    PERMUTE = TRANSPOSE,
    REVERSE,
    SPACE_TO_DEPTH,
    DEPTH_TO_SPACE,
    SPACE_TO_BATCH_ND,
    BATCH_TO_SPACE_ND,
    //REORG
    CHANNEL_SHUFFLE,
    EXPAND_DIMS,

    FLOOR,

    RNN,
    UNIDIRECTIONAL_SEQUENCE_RNN,
    BIDIRECTIONAL_SEQUENCE_RNN,
    SVDF,
    HASHTABLE_LOOKUP,
    EMBEDDING_LOOKUP,
    //LSTMUNIT_OVXLIB
    //LSTMUNIT_ACTIVATION
    UNIDIRECTIONAL_SEQUENCE_LSTM,
    BIDIRECTIONAL_SEQUENCE_LSTM,
    LSTM_UNIT,
    LSTM = LSTM_UNIT,
    LSTM_LAYER,

    LSH_PROJECTION,
    DEQUANTIZE,
    QUANTIZE,

    SLICE,
    STRIDED_SLICE,

    //PROPOSAL,
    NOOP,

    //VARIABLE
    //CROP
    SQRT,
    SQUARE,
    RSQRT,
    //DROPOUT, //No need
    //SHUFFLE_CHANNEL
    //SCALE
    MATRIX_MUL,
    //ELU
    //TENSOR_STACK_CONCAT
    //SIGNAL_FRAME
    //A_TIMES_B_PLUS_C
    ABS,
    //NBG
    POW,
    //FLOOR_DIV
    MINIMUM,
    MAXIMUM,
    //SPATIAL_TRANSFORMER
    ARGMAX,
    ARGMIN,
    EXP,
    GATHER,
    NEG,
    REDUCE_ALL,
    REDUCE_ANY,
    REDUCE_MAX,
    REDUCE_MIN,
    REDUCE_PROD,
    REDUCE_SUM,
    MEAN,
    REDUCE_MEAN = MEAN,
    SIN,
    TILE,
    TOPK,


    IMAGE_PROCESS,
    //SYNC_HOST
    //CONCAT_SHIFT

    //TENSOR_ADD_MEAN_STDDEV_NORM
    //STACK
    EQUAL,
    //FAKE_QUANTIZATION
    GREATER,
    GREATER_EQUAL,
    LESS,
    LESS_EQUAL,
    LOGICAL_AND,
    LOGICAL_OR,
    LOGICAL_NOT,
    NOT_EQUAL,
    SELECT,
    LOG,

    AXIS_ALIGNED_BBOX_TRANSFORM,
    BOX_WITH_NMS_LIMIT,
    DETECTION_POSTPROCESSING,
    GENERATE_PROPOSALS,
    HEATMAP_MAX_KEYPOINT,

    RANDOM_MULTINOMIAL,
    LOG_SOFTMAX,
    PAD_V2,

    LINEAR, // A_TIMES_B_PLUS_C
};

enum class OperandType: uint8_t {
    NONE,
    BOOL,
    INT8,
    INT16,
    INT32,
    UINT8,
    UINT16,
    UINT32,
    FLOAT16,
    FLOAT32,
    FLOAT64,
    TENSOR_INDEX_START,
    TENSOR_BOOL8 = TENSOR_INDEX_START,
    TENSOR_INT16,
    TENSOR_FLOAT16,
    TENSOR_FLOAT32,
    TENSOR_INT32,
    TENSOR_QUANT8_ASYMM,
    TENSOR_QUANT8_SYMM,
    TENSOR_QUANT16_ASYMM,
    TENSOR_QUANT16_SYMM,
    TENSOR_QUANT32_SYMM,
    TENSOR_QUANT8_SYMM_PER_CHANNEL,
};

enum TensorLifeTime
{
    VIRTUAL,
    NORMAL,
    CONST,
};

enum class FusedType: uint8_t {
    NONE,
    RELU,
    RELU1,
    RELU6,
    TANH,
    SIGMOID,
};

enum class QuantizerType: uint8_t {
    ASYMMETRIC,
    SYMMETRIC,
    ASYMMETRIC_PER_CHANNEL,
    SYMMETRIC_PER_CHANNEL
};

enum class PadType: uint8_t {
    AUTO,
    VALID,
    SAME,
};

enum class OverflowPolicy {
    WRAP,
    SATURATE,
};

enum class RoundingPolicy {
    TO_ZERO,
    RTNE,
};

enum class Rounding {
    FLOOR,
    CEILING,
    RTNE,
};

enum class PadMode {
    CONSTANT,
};

/*
 * In border mode, Caffe computes with full kernel size,
 * Android & Tensorflow computes with valid kernel size.
 */
enum class PoolMode {
    VALID,
    FULL,
};

enum class LshProjectionType {
    SPARSE = 1,
    DENSE = 2,
};

enum class DataLayout {
    NONE,
    NHWC,
    NCHW,
    WHCN, // openvx type
    CNT,
    ERR = CNT
};

enum class NormalizationAlgorithmChannel {
    Across = 0,
    Within = 1,
};

enum class NormalizationAlgorithmMethod {
    /// Krichevsky 2012: Local Brightness Normalization
    LocalBrightness = 0,
    /// Jarret 2009: Local Contrast Normalization
    LocalContrast = 1
};

enum class NmsKernelMethod {
    Hard = 0,
    Linear = 1,
    Gaussian = 2,
};

}
#endif
