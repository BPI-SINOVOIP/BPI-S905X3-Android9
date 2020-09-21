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

#ifndef __ANEURALNETWORKS_TRANSPOSE_CONV_2D_HPP__
#define __ANEURALNETWORKS_TRANSPOSE_CONV_2D_HPP__

#include "hal_limitation/support_macros.hpp"

#define OP_SPEC_NAME TransposeConv2DOperationInput
OP_SPEC_BEGIN()
#define ARG_NAMES         \
    (input,                 \
     kernel,           \
     bias,           \
     pad_left,  \
     pad_right, \
     pad_top,   \
     pad_bottom,    \
     stride_w,  \
     stride_h, \
     fuse_code, \
     layout,    \
     output_shape,  \
     implicit_pad)
#define ARGC BOOST_PP_TUPLE_SIZE(ARG_NAMES)

#define BOOST_PP_LOCAL_MACRO(n) OP_SPEC_ARG(BOOST_PP_TUPLE_ELEM(ARGC, n, ARG_NAMES))
#define BOOST_PP_LOCAL_LIMITS (0, ARGC)
#include BOOST_PP_LOCAL_ITERATE()
OP_SPEC_END()

// order of argument is important
MAKE_SPEC(explicit_padding_transpose_conv_2d)
    .input_(nnrt::OperandType::TENSOR_FLOAT32)
    .kernel_(nnrt::OperandType::TENSOR_FLOAT32)
    .bias_(nnrt::OperandType::TENSOR_FLOAT32)
    .pad_left_(nnrt::OperandType::INT32)
    .pad_top_(nnrt::OperandType::INT32)
    .pad_right_(nnrt::OperandType::INT32)
    .pad_bottom_(nnrt::OperandType::INT32)
    .stride_w_(nnrt::OperandType::INT32)
    .stride_h_(nnrt::OperandType::INT32)
    .fuse_code_(nnrt::OperandType::INT32)
    .layout_(nnrt::OperandType::BOOL));

    // // Note: Bia not support float16
    // OVERRIDE_SPEC(explicit_padding_transpose_conv_2d, float16)
    // .input_(nnrt::OperandType::TENSOR_FLOAT16)
    // .kernel_(nnrt::OperandType::TENSOR_FLOAT16)
    // .bias_(nnrt::OperandType::TENSOR_FLOAT16));

    OVERRIDE_SPEC(explicit_padding_transpose_conv_2d, quant8)
    .input_(nnrt::OperandType::TENSOR_QUANT8_ASYMM)
    .kernel_(nnrt::OperandType::TENSOR_QUANT8_ASYMM)
    .bias_(nnrt::OperandType::TENSOR_INT32));

    // // Note: Kernel not support perchannel
    // OVERRIDE_SPEC(explicit_padding_transpose_conv_2d, per_channel_quant8)
    // .input_(nnrt::OperandType::TENSOR_QUANT8_ASYMM)
    // .kernel_(nnrt::OperandType::TENSOR_QUANT8_SYMM_PER_CHANNEL)
    // .bias_(nnrt::OperandType::TENSOR_INT32));

MAKE_SPEC(implicit_padding_transpose_conv_2d)
    .input_(nnrt::OperandType::TENSOR_FLOAT32)
    .kernel_(nnrt::OperandType::TENSOR_FLOAT32)
    .bias_(nnrt::OperandType::TENSOR_FLOAT32)
    .output_shape_(nnrt::OperandType::TENSOR_INT32)
    .implicit_pad_(nnrt::OperandType::INT32)
    .stride_w_(nnrt::OperandType::INT32)
    .stride_h_(nnrt::OperandType::INT32)
    .fuse_code_(nnrt::OperandType::INT32)
    .layout_(nnrt::OperandType::BOOL));

    // OVERRIDE_SPEC(implicit_padding_transpose_conv_2d, float16)
    // .input_(nnrt::OperandType::TENSOR_FLOAT16)
    // .kernel_(nnrt::OperandType::TENSOR_FLOAT16)
    // .bias_(nnrt::OperandType::TENSOR_FLOAT16));

    OVERRIDE_SPEC(implicit_padding_transpose_conv_2d, quant8)
    .input_(nnrt::OperandType::TENSOR_QUANT8_ASYMM)
    .kernel_(nnrt::OperandType::TENSOR_QUANT8_ASYMM)
    .bias_(nnrt::OperandType::TENSOR_INT32));

    // OVERRIDE_SPEC(implicit_padding_transpose_conv_2d, per_channel_quant8)
    // .input_(nnrt::OperandType::TENSOR_QUANT8_ASYMM)
    // .kernel_(nnrt::OperandType::TENSOR_QUANT8_SYMM_PER_CHANNEL)
    // .bias_(nnrt::OperandType::TENSOR_INT32));
#undef ARG_NAMES
#undef ARGC
#undef OP_SPEC_NAME

// Output Spec
#define OP_SPEC_NAME TransposeConv2DOperationOutput
OP_SPEC_BEGIN()
#define ARG_NAMES         \
    (input,               \
     output)
#define ARGC BOOST_PP_TUPLE_SIZE(ARG_NAMES)

#define BOOST_PP_LOCAL_MACRO(n) OP_SPEC_ARG(BOOST_PP_TUPLE_ELEM(ARGC, n, ARG_NAMES))
#define BOOST_PP_LOCAL_LIMITS (0, ARGC)
#include BOOST_PP_LOCAL_ITERATE()
OP_SPEC_END()

// order of argument is important
MAKE_SPEC(output)
    .input_(nnrt::OperandType::TENSOR_FLOAT32)
    .output_(nnrt::OperandType::TENSOR_FLOAT32));

    OVERRIDE_SPEC(output, 0)
    .input_(nnrt::OperandType::TENSOR_FLOAT16)
    .output_(nnrt::OperandType::TENSOR_FLOAT16));

    OVERRIDE_SPEC(output, 1)
    .input_(nnrt::OperandType::TENSOR_QUANT8_ASYMM)
    .output_(nnrt::OperandType::TENSOR_QUANT8_ASYMM));

#undef ARG_NAMES
#undef ARGC
#undef OP_SPEC_NAME

#endif
