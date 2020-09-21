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

#ifndef __ANEURALNETWORKS_DETECTION_POSTPROCESSING_HPP__
#define __ANEURALNETWORKS_DETECTION_POSTPROCESSING_HPP__

#include "hal_limitation/support_macros.hpp"

// Input Spec
#define OP_SPEC_NAME DetectionPostprocessingInput
OP_SPEC_BEGIN()
#define ARG_NAMES         \
    (input,               \
     bbox,                \
     anchor_shape,        \
     dy,                  \
     dx,                  \
     dh,                  \
     dw,                  \
     nms_type,            \
     max_num_detections,  \
     maximum_class_per_detection,\
     maximum_detection_per_class,\
     score_threshold,     \
     iou_threshold,       \
     is_bg_in_label)
#define ARGC BOOST_PP_TUPLE_SIZE(ARG_NAMES)

#define BOOST_PP_LOCAL_MACRO(n) OP_SPEC_ARG(BOOST_PP_TUPLE_ELEM(ARGC, n, ARG_NAMES))
#define BOOST_PP_LOCAL_LIMITS (0, ARGC)
#include BOOST_PP_LOCAL_ITERATE()
OP_SPEC_END()

// order of argument is important
MAKE_SPEC(detection_postprocessing)
    .input_(nnrt::OperandType::TENSOR_FLOAT32)
    .bbox_(nnrt::OperandType::TENSOR_FLOAT32)
    .anchor_shape_(nnrt::OperandType::TENSOR_FLOAT32)
    .dy_(nnrt::OperandType::FLOAT32)
    .dx_(nnrt::OperandType::FLOAT32)
    .dh_(nnrt::OperandType::FLOAT32)
    .dw_(nnrt::OperandType::FLOAT32)
    .nms_type_(nnrt::OperandType::BOOL)
    .max_num_detections_(nnrt::OperandType::INT32)
    .maximum_class_per_detection_(nnrt::OperandType::INT32)
    .maximum_detection_per_class_(nnrt::OperandType::INT32)
    .score_threshold_(nnrt::OperandType::FLOAT32)
    .iou_threshold_(nnrt::OperandType::FLOAT32)
    .is_bg_in_label_(nnrt::OperandType::BOOL)
    );

    // OVERRIDE_SPEC(detection_postprocessing, float16)
    // .input_(nnrt::OperandType::TENSOR_FLOAT16)
    // .bbox_(nnrt::OperandType::TENSOR_FLOAT16)
    // .anchor_shape_(nnrt::OperandType::TENSOR_FLOAT16)
    // .score_threshold_(nnrt::OperandType::FLOAT16)
    // .iou_threshold_(nnrt::OperandType::FLOAT16)
    // );

#undef ARG_NAMES
#undef ARGC
#undef OP_SPEC_NAME

//Output Spec
#define OP_SPEC_NAME DetectionPostprocessingOutput
OP_SPEC_BEGIN()
#define ARG_NAMES         \
    (input,               \
    score,                \
    bbox_coordinate,      \
    class_label,          \
    detections_num)
#define ARGC BOOST_PP_TUPLE_SIZE(ARG_NAMES)

#define BOOST_PP_LOCAL_MACRO(n) OP_SPEC_ARG(BOOST_PP_TUPLE_ELEM(ARGC, n, ARG_NAMES))
#define BOOST_PP_LOCAL_LIMITS (0, ARGC)
#include BOOST_PP_LOCAL_ITERATE()
OP_SPEC_END()

// order of argument is important
MAKE_SPEC(output)
    .input_(nnrt::OperandType::TENSOR_FLOAT32)
    .score_(nnrt::OperandType::TENSOR_FLOAT32)
    .bbox_coordinate_(nnrt::OperandType::TENSOR_INT32)
    .class_label_(nnrt::OperandType::TENSOR_INT32)
    .detections_num_(nnrt::OperandType::TENSOR_INT32)
    );

    OVERRIDE_SPEC(output, float16)
    .input_(nnrt::OperandType::TENSOR_FLOAT16)
    .score_(nnrt::OperandType::TENSOR_FLOAT16)
    );

#undef ARG_NAMES
#undef ARGC
#undef OP_SPEC_NAME

#endif