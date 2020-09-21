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

#ifndef __ANEURALNETWORKS_GENERATE_PROPOSALS_HPP__
#define __ANEURALNETWORKS_GENERATE_PROPOSALS_HPP__

#include "hal_limitation/support_macros.hpp"

// Input Spec
#define OP_SPEC_NAME GenerateProposalsInput
OP_SPEC_BEGIN()
#define ARG_NAMES         \
    (score,                 \
     bounding_box,           \
     anchor_shape,           \
     image_size,            \
     ratio_h,               \
     ratio_w,               \
     pre_nms_topn,           \
     post_nms_topn,          \
     iou_threshold,          \
     min_size,              \
     layout)
#define ARGC BOOST_PP_TUPLE_SIZE(ARG_NAMES)

#define BOOST_PP_LOCAL_MACRO(n) OP_SPEC_ARG(BOOST_PP_TUPLE_ELEM(ARGC, n, ARG_NAMES))
#define BOOST_PP_LOCAL_LIMITS (0, ARGC)
#include BOOST_PP_LOCAL_ITERATE()
OP_SPEC_END()

// order of argument is important
MAKE_SPEC(generate_proposals)
    .score_(nnrt::OperandType::TENSOR_FLOAT32)
    .bounding_box_(nnrt::OperandType::TENSOR_FLOAT32)
    .anchor_shape_(nnrt::OperandType::TENSOR_FLOAT32)
    .image_size_(nnrt::OperandType::TENSOR_FLOAT32)
    .ratio_h_(nnrt::OperandType::FLOAT32)
    .ratio_w_(nnrt::OperandType::FLOAT32)
    .pre_nms_topn_(nnrt::OperandType::INT32)
    .post_nms_topn_(nnrt::OperandType::INT32)
    .iou_threshold_(nnrt::OperandType::FLOAT32)
    .min_size_(nnrt::OperandType::FLOAT32)
    .layout_(nnrt::OperandType::BOOL));

    // OVERRIDE_SPEC(generate_proposals, float16)
    // .score_(nnrt::OperandType::TENSOR_FLOAT16)
    // .bounding_box_(nnrt::OperandType::TENSOR_FLOAT16)
    // .anchor_shape_(nnrt::OperandType::TENSOR_FLOAT16)
    // .image_size_(nnrt::OperandType::TENSOR_FLOAT16));

    // OVERRIDE_SPEC(generate_proposals, quant8_asymm)
    // .score_(nnrt::OperandType::TENSOR_QUANT8_ASYMM)
    // .bounding_box_(nnrt::OperandType::TENSOR_QUANT8_ASYMM)
    // .anchor_shape_(nnrt::OperandType::TENSOR_QUANT16_SYMM)
    // .image_size_(nnrt::OperandType::TENSOR_QUANT16_SYMM));

#undef ARG_NAMES
#undef ARGC
#undef OP_SPEC_NAME

//Output Spec
#define OP_SPEC_NAME GenerateProposalsOutput
OP_SPEC_BEGIN()
#define ARG_NAMES         \
    (score,               \
    output_score,              \
    bbox_coordinate,              \
    batch_index)
#define ARGC BOOST_PP_TUPLE_SIZE(ARG_NAMES)

#define BOOST_PP_LOCAL_MACRO(n) OP_SPEC_ARG(BOOST_PP_TUPLE_ELEM(ARGC, n, ARG_NAMES))
#define BOOST_PP_LOCAL_LIMITS (0, ARGC)
#include BOOST_PP_LOCAL_ITERATE()
OP_SPEC_END()

// order of argument is important
MAKE_SPEC(output)
    .score_(nnrt::OperandType::TENSOR_FLOAT32)
    .output_score_(nnrt::OperandType::TENSOR_FLOAT32)
    .bbox_coordinate_(nnrt::OperandType::TENSOR_FLOAT32)
    .batch_index_(nnrt::OperandType::TENSOR_INT32)
    );

    OVERRIDE_SPEC(output, float16)
    .score_(nnrt::OperandType::TENSOR_FLOAT16)
    .output_score_(nnrt::OperandType::TENSOR_FLOAT16)
    .bbox_coordinate_(nnrt::OperandType::TENSOR_FLOAT16));

    OVERRIDE_SPEC(output, quant8_asymm)
    .score_(nnrt::OperandType::TENSOR_QUANT8_ASYMM)
    .output_score_(nnrt::OperandType::TENSOR_QUANT8_ASYMM)
    .bbox_coordinate_(nnrt::OperandType::TENSOR_QUANT8_ASYMM));

#undef ARG_NAMES
#undef ARGC
#undef OP_SPEC_NAME

#endif