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

#ifndef __ANEURALNETWORKS_BIDIRECTIONAL_SEQUENCE_RNN_HPP__
#define __ANEURALNETWORKS_BIDIRECTIONAL_SEQUENCE_RNN_HPP__

#include "hal_limitation/support_macros.hpp"

// Input Spec
#define OP_SPEC_NAME BidirectionalSequenceRnnInput
OP_SPEC_BEGIN()
#define ARG_NAMES         \
    (input, \
    fw_input_weight_i, \
    fw_input_weight_h, \
    fw_input_bias, \
    fw_input_h_state, \
    bw_input_weight_i, \
    bw_input_weight_h, \
    bw_input_bias, \
    bw_input_h_state, \
    aux_input, \
    fw_aux_input_weight, \
    bw_aux_input_weight, \
    activation, \
    timeMajor, \
    mergeOutputs)
#define ARGC BOOST_PP_TUPLE_SIZE(ARG_NAMES)

#define BOOST_PP_LOCAL_MACRO(n) OP_SPEC_ARG(BOOST_PP_TUPLE_ELEM(ARGC, n, ARG_NAMES))
#define BOOST_PP_LOCAL_LIMITS (0, ARGC)
#include BOOST_PP_LOCAL_ITERATE()
OP_SPEC_END()

// order of argument is important
MAKE_SPEC(bidirectional_sequence_rnn)
    .input_(nnrt::OperandType::TENSOR_FLOAT32)
    .fw_input_weight_i_(nnrt::OperandType::TENSOR_FLOAT32)
    .fw_input_weight_h_(nnrt::OperandType::TENSOR_FLOAT32)
    .fw_input_bias_(nnrt::OperandType::TENSOR_FLOAT32)
    .fw_input_h_state_(nnrt::OperandType::TENSOR_FLOAT32)
    .bw_input_weight_i_(nnrt::OperandType::TENSOR_FLOAT32)
    .bw_input_weight_h_(nnrt::OperandType::TENSOR_FLOAT32)
    .bw_input_bias_(nnrt::OperandType::TENSOR_FLOAT32)
    .bw_input_h_state_(nnrt::OperandType::TENSOR_FLOAT32)
    .aux_input_(nnrt::OperandType::TENSOR_FLOAT32, OPTIONAL)
    .fw_aux_input_weight_(nnrt::OperandType::TENSOR_FLOAT32, OPTIONAL)
    .bw_aux_input_weight_(nnrt::OperandType::TENSOR_FLOAT32, OPTIONAL)
    .activation_(nnrt::OperandType::INT32)
    .timeMajor_(nnrt::OperandType::BOOL)
    .mergeOutputs_(nnrt::OperandType::BOOL)
    );

    OVERRIDE_SPEC(bidirectional_sequence_rnn, float16)
    .input_(nnrt::OperandType::TENSOR_FLOAT16)
    .fw_input_weight_i_(nnrt::OperandType::TENSOR_FLOAT16)
    .fw_input_weight_h_(nnrt::OperandType::TENSOR_FLOAT16)
    .fw_input_bias_(nnrt::OperandType::TENSOR_FLOAT16)
    .fw_input_h_state_(nnrt::OperandType::TENSOR_FLOAT16)
    .bw_input_weight_i_(nnrt::OperandType::TENSOR_FLOAT16)
    .bw_input_weight_h_(nnrt::OperandType::TENSOR_FLOAT16)
    .bw_input_bias_(nnrt::OperandType::TENSOR_FLOAT16)
    .bw_input_h_state_(nnrt::OperandType::TENSOR_FLOAT16)
    .aux_input_(nnrt::OperandType::TENSOR_FLOAT16, OPTIONAL)
    .fw_aux_input_weight_(nnrt::OperandType::TENSOR_FLOAT16, OPTIONAL)
    .bw_aux_input_weight_(nnrt::OperandType::TENSOR_FLOAT16, OPTIONAL)
    );

#undef ARG_NAMES
#undef ARGC
#undef OP_SPEC_NAME

//Output Spec
#define OP_SPEC_NAME BidirectionalSequenceRnnOutput
OP_SPEC_BEGIN()
#define ARG_NAMES         \
    (input,               \
    output_fw,            \
    output_bw)
#define ARGC BOOST_PP_TUPLE_SIZE(ARG_NAMES)

#define BOOST_PP_LOCAL_MACRO(n) OP_SPEC_ARG(BOOST_PP_TUPLE_ELEM(ARGC, n, ARG_NAMES))
#define BOOST_PP_LOCAL_LIMITS (0, ARGC)
#include BOOST_PP_LOCAL_ITERATE()
OP_SPEC_END()

// order of argument is important
MAKE_SPEC(output)
    .input_(nnrt::OperandType::TENSOR_FLOAT32)
    .output_fw_(nnrt::OperandType::TENSOR_FLOAT32)
    .output_bw_(nnrt::OperandType::TENSOR_FLOAT32, OPTIONAL));

    OVERRIDE_SPEC(output, float16)
    .input_(nnrt::OperandType::TENSOR_FLOAT16)
    .output_fw_(nnrt::OperandType::TENSOR_FLOAT16)
    .output_bw_(nnrt::OperandType::TENSOR_FLOAT16, OPTIONAL));

#undef ARG_NAMES
#undef ARGC
#undef OP_SPEC_NAME

#endif