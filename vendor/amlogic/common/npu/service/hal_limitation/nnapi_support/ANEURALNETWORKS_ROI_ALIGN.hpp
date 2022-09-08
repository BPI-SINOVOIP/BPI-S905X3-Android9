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

#ifndef __AANEURALNETWORKS_ROI_ALIGN_HPP__
#define __AANEURALNETWORKS_ROI_ALIGN_HPP__

#define OP_SPEC_NAME ROIAlignOperationInput
OP_SPEC_BEGIN()
#define ARG_NAMES         \
    (input,               \
     roi_location,        \
     batch_index,         \
     height,              \
     width,               \
     height_ratio,        \
     width_ratio,         \
     sampling_points_height,\
     sampling_points_width,\
     layout)
#define ARGC BOOST_PP_TUPLE_SIZE(ARG_NAMES)

#define BOOST_PP_LOCAL_MACRO(n) OP_SPEC_ARG(BOOST_PP_TUPLE_ELEM(ARGC, n, ARG_NAMES))
#define BOOST_PP_LOCAL_LIMITS (0, ARGC)
#include BOOST_PP_LOCAL_ITERATE()
OP_SPEC_END()

// order of argument is important
MAKE_SPEC(roi_align)
    .input_(nnrt::OperandType::TENSOR_FLOAT32)
    .roi_location_(nnrt::OperandType::TENSOR_FLOAT32)
    .batch_index_(nnrt::OperandType::TENSOR_INT32)
    .height_(nnrt::OperandType::INT32)
    .width_(nnrt::OperandType::INT32)
    .height_ratio_(nnrt::OperandType::FLOAT32)
    .width_ratio_(nnrt::OperandType::FLOAT32)
    .sampling_points_height_(nnrt::OperandType::INT32)
    .sampling_points_width_(nnrt::OperandType::INT32)
    .layout_(nnrt::OperandType::BOOL)
    );

    OVERRIDE_SPEC(roi_align, float16)
    .input_(nnrt::OperandType::TENSOR_FLOAT16)
    .roi_location_(nnrt::OperandType::TENSOR_FLOAT16)
    .height_ratio_(nnrt::OperandType::FLOAT16)
    .width_ratio_(nnrt::OperandType::FLOAT16)
    );

    // Not support aysmm_u8 currently
    // OVERRIDE_SPEC(roi_align, aysmm_u8)
    // .input_(nnrt::OperandType::TENSOR_QUANT8_ASYMM)
    // .roi_location_(nnrt::OperandType::TENSOR_QUANT16_ASYMM)
    // );

#undef ARG_NAMES
#undef ARGC
#undef OP_SPEC_NAME

//Output Spec
#define OP_SPEC_NAME ROIAlignOperationOutput
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

    OVERRIDE_SPEC(output, float16)
    .input_(nnrt::OperandType::TENSOR_FLOAT16)
    .output_(nnrt::OperandType::TENSOR_FLOAT16));

    OVERRIDE_SPEC(output, asymm_u8)
    .input_(nnrt::OperandType::TENSOR_QUANT8_ASYMM)
    .output_(nnrt::OperandType::TENSOR_QUANT8_ASYMM));

#undef ARG_NAMES
#undef ARGC
#undef OP_SPEC_NAME

#endif
