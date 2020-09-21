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

#ifndef __ANEURALNETWORKS_BIDIRECTIONAL_SEQUENCE_LSTM_HPP__
#define __ANEURALNETWORKS_BIDIRECTIONAL_SEQUENCE_LSTM_HPP__

#include "hal_limitation/support_macros.hpp"

// Input Spec
#define OP_SPEC_NAME BidirectionalSequenceLstmInput
OP_SPEC_BEGIN()
#define ARG_NAMES         \
    (input,               \
    fw_input_weight_i2i, \
    fw_input_weight_i2f, \
    fw_input_weight_i2c, \
    fw_input_weight_i2o, \
    fw_input_weight_r2i, \
    fw_input_weight_r2f, \
    fw_input_weight_r2c, \
    fw_input_weight_r2o, \
    fw_input_weight_c2i, \
    fw_input_weight_c2f, \
    fw_input_weight_c2o, \
    fw_input_bias_i, \
    fw_input_bias_f, \
    fw_input_bias_c, \
    fw_input_bias_o, \
    fw_input_weight_proj, \
    fw_input_bias_proj, \
    bw_input_weight_i2i, \
    bw_input_weight_i2f, \
    bw_input_weight_i2c, \
    bw_input_weight_i2o, \
    bw_input_weight_r2i, \
    bw_input_weight_r2f, \
    bw_input_weight_r2c, \
    bw_input_weight_r2o, \
    bw_input_weight_c2i, \
    bw_input_weight_c2f, \
    bw_input_weight_c2o, \
    bw_input_bias_i, \
    bw_input_bias_f, \
    bw_input_bias_c, \
    bw_input_bias_o, \
    bw_input_weight_proj, \
    bw_input_bias_proj, \
    fw_input_h_state, \
    fw_input_c_state, \
    bw_input_h_state, \
    bw_input_c_state, \
    aux_input, \
    fw_aux_input_weight_i2i, \
    fw_aux_input_weight_i2f, \
    fw_aux_input_weight_i2c, \
    fw_aux_input_weight_i2o, \
    bw_aux_input_weight_i2i, \
    bw_aux_input_weight_i2f, \
    bw_aux_input_weight_i2c, \
    bw_aux_input_weight_i2o, \
    activation, \
    cell_clip, \
    proj_clip, \
    merge_outputs, \
    time_major, \
    fw_input_layernorm_i, \
    fw_input_layernorm_f, \
    fw_input_layernorm_c, \
    fw_input_layernorm_o, \
    bw_input_layernorm_i, \
    bw_input_layernorm_f, \
    bw_input_layernorm_c, \
    bw_input_layernorm_o)
#define ARGC BOOST_PP_TUPLE_SIZE(ARG_NAMES)

#define BOOST_PP_LOCAL_MACRO(n) OP_SPEC_ARG(BOOST_PP_TUPLE_ELEM(ARGC, n, ARG_NAMES))
#define BOOST_PP_LOCAL_LIMITS (0, ARGC)
#include BOOST_PP_LOCAL_ITERATE()
OP_SPEC_END()

// order of argument is important
MAKE_SPEC(bidirectional_sequence_lstm)
    .input_(nnrt::OperandType::TENSOR_FLOAT32)
    .fw_input_weight_i2i_(nnrt::OperandType::TENSOR_FLOAT32, OPTIONAL)
    .fw_input_weight_i2f_(nnrt::OperandType::TENSOR_FLOAT32)
    .fw_input_weight_i2c_(nnrt::OperandType::TENSOR_FLOAT32)
    .fw_input_weight_i2o_(nnrt::OperandType::TENSOR_FLOAT32)
    .fw_input_weight_r2i_(nnrt::OperandType::TENSOR_FLOAT32, OPTIONAL)
    .fw_input_weight_r2f_(nnrt::OperandType::TENSOR_FLOAT32)
    .fw_input_weight_r2c_(nnrt::OperandType::TENSOR_FLOAT32)
    .fw_input_weight_r2o_(nnrt::OperandType::TENSOR_FLOAT32)
    .fw_input_weight_c2i_(nnrt::OperandType::TENSOR_FLOAT32, OPTIONAL)
    .fw_input_weight_c2f_(nnrt::OperandType::TENSOR_FLOAT32, OPTIONAL)
    .fw_input_weight_c2o_(nnrt::OperandType::TENSOR_FLOAT32, OPTIONAL)
    .fw_input_bias_i_(nnrt::OperandType::TENSOR_FLOAT32, OPTIONAL)
    .fw_input_bias_f_(nnrt::OperandType::TENSOR_FLOAT32)
    .fw_input_bias_c_(nnrt::OperandType::TENSOR_FLOAT32)
    .fw_input_bias_o_(nnrt::OperandType::TENSOR_FLOAT32)
    .fw_input_weight_proj_(nnrt::OperandType::TENSOR_FLOAT32, OPTIONAL)
    .fw_input_bias_proj_(nnrt::OperandType::TENSOR_FLOAT32, OPTIONAL)
    .bw_input_weight_i2i_(nnrt::OperandType::TENSOR_FLOAT32, OPTIONAL)
    .bw_input_weight_i2f_(nnrt::OperandType::TENSOR_FLOAT32)
    .bw_input_weight_i2c_(nnrt::OperandType::TENSOR_FLOAT32)
    .bw_input_weight_i2o_(nnrt::OperandType::TENSOR_FLOAT32)
    .bw_input_weight_r2i_(nnrt::OperandType::TENSOR_FLOAT32, OPTIONAL)
    .bw_input_weight_r2f_(nnrt::OperandType::TENSOR_FLOAT32)
    .bw_input_weight_r2c_(nnrt::OperandType::TENSOR_FLOAT32)
    .bw_input_weight_r2o_(nnrt::OperandType::TENSOR_FLOAT32)
    .bw_input_weight_c2i_(nnrt::OperandType::TENSOR_FLOAT32, OPTIONAL)
    .bw_input_weight_c2f_(nnrt::OperandType::TENSOR_FLOAT32, OPTIONAL)
    .bw_input_weight_c2o_(nnrt::OperandType::TENSOR_FLOAT32, OPTIONAL)
    .bw_input_bias_i_(nnrt::OperandType::TENSOR_FLOAT32, OPTIONAL)
    .bw_input_bias_f_(nnrt::OperandType::TENSOR_FLOAT32)
    .bw_input_bias_c_(nnrt::OperandType::TENSOR_FLOAT32)
    .bw_input_bias_o_(nnrt::OperandType::TENSOR_FLOAT32)
    .bw_input_weight_proj_(nnrt::OperandType::TENSOR_FLOAT32, OPTIONAL)
    .bw_input_bias_proj_(nnrt::OperandType::TENSOR_FLOAT32, OPTIONAL)
    .fw_input_h_state_(nnrt::OperandType::TENSOR_FLOAT32)
    .fw_input_c_state_(nnrt::OperandType::TENSOR_FLOAT32)
    .bw_input_h_state_(nnrt::OperandType::TENSOR_FLOAT32)
    .bw_input_c_state_(nnrt::OperandType::TENSOR_FLOAT32)
    .aux_input_(nnrt::OperandType::TENSOR_FLOAT32, OPTIONAL)
    .fw_aux_input_weight_i2i_(nnrt::OperandType::TENSOR_FLOAT32, OPTIONAL)
    .fw_aux_input_weight_i2f_(nnrt::OperandType::TENSOR_FLOAT32, OPTIONAL)
    .fw_aux_input_weight_i2c_(nnrt::OperandType::TENSOR_FLOAT32, OPTIONAL)
    .fw_aux_input_weight_i2o_(nnrt::OperandType::TENSOR_FLOAT32, OPTIONAL)
    .bw_aux_input_weight_i2i_(nnrt::OperandType::TENSOR_FLOAT32, OPTIONAL)
    .bw_aux_input_weight_i2f_(nnrt::OperandType::TENSOR_FLOAT32, OPTIONAL)
    .bw_aux_input_weight_i2c_(nnrt::OperandType::TENSOR_FLOAT32, OPTIONAL)
    .bw_aux_input_weight_i2o_(nnrt::OperandType::TENSOR_FLOAT32, OPTIONAL)
    .activation_(nnrt::OperandType::INT32)
    .cell_clip_(nnrt::OperandType::FLOAT32)
    .proj_clip_(nnrt::OperandType::FLOAT32)
    .merge_outputs_(nnrt::OperandType::BOOL)
    .time_major_(nnrt::OperandType::BOOL)
    .fw_input_layernorm_i_(nnrt::OperandType::TENSOR_FLOAT32, OPTIONAL)
    .fw_input_layernorm_f_(nnrt::OperandType::TENSOR_FLOAT32, OPTIONAL)
    .fw_input_layernorm_c_(nnrt::OperandType::TENSOR_FLOAT32, OPTIONAL)
    .fw_input_layernorm_o_(nnrt::OperandType::TENSOR_FLOAT32, OPTIONAL)
    .bw_input_layernorm_i_(nnrt::OperandType::TENSOR_FLOAT32, OPTIONAL)
    .bw_input_layernorm_f_(nnrt::OperandType::TENSOR_FLOAT32, OPTIONAL)
    .bw_input_layernorm_c_(nnrt::OperandType::TENSOR_FLOAT32, OPTIONAL)
    .bw_input_layernorm_o_(nnrt::OperandType::TENSOR_FLOAT32, OPTIONAL)
    );

    OVERRIDE_SPEC(bidirectional_sequence_lstm, float16)
    .input_(nnrt::OperandType::TENSOR_FLOAT16)
    .fw_input_weight_i2i_(nnrt::OperandType::TENSOR_FLOAT16, OPTIONAL)
    .fw_input_weight_i2f_(nnrt::OperandType::TENSOR_FLOAT16)
    .fw_input_weight_i2c_(nnrt::OperandType::TENSOR_FLOAT16)
    .fw_input_weight_i2o_(nnrt::OperandType::TENSOR_FLOAT16)
    .fw_input_weight_r2i_(nnrt::OperandType::TENSOR_FLOAT16, OPTIONAL)
    .fw_input_weight_r2f_(nnrt::OperandType::TENSOR_FLOAT16)
    .fw_input_weight_r2c_(nnrt::OperandType::TENSOR_FLOAT16)
    .fw_input_weight_r2o_(nnrt::OperandType::TENSOR_FLOAT16)
    .fw_input_weight_c2i_(nnrt::OperandType::TENSOR_FLOAT16, OPTIONAL)
    .fw_input_weight_c2f_(nnrt::OperandType::TENSOR_FLOAT16, OPTIONAL)
    .fw_input_weight_c2o_(nnrt::OperandType::TENSOR_FLOAT16, OPTIONAL)
    .fw_input_bias_i_(nnrt::OperandType::TENSOR_FLOAT16, OPTIONAL)
    .fw_input_bias_f_(nnrt::OperandType::TENSOR_FLOAT16)
    .fw_input_bias_c_(nnrt::OperandType::TENSOR_FLOAT16)
    .fw_input_bias_o_(nnrt::OperandType::TENSOR_FLOAT16)
    .fw_input_weight_proj_(nnrt::OperandType::TENSOR_FLOAT16, OPTIONAL)
    .fw_input_bias_proj_(nnrt::OperandType::TENSOR_FLOAT16, OPTIONAL)
    .bw_input_weight_i2i_(nnrt::OperandType::TENSOR_FLOAT16, OPTIONAL)
    .bw_input_weight_i2f_(nnrt::OperandType::TENSOR_FLOAT16)
    .bw_input_weight_i2c_(nnrt::OperandType::TENSOR_FLOAT16)
    .bw_input_weight_i2o_(nnrt::OperandType::TENSOR_FLOAT16)
    .bw_input_weight_r2i_(nnrt::OperandType::TENSOR_FLOAT16, OPTIONAL)
    .bw_input_weight_r2f_(nnrt::OperandType::TENSOR_FLOAT16)
    .bw_input_weight_r2c_(nnrt::OperandType::TENSOR_FLOAT16)
    .bw_input_weight_r2o_(nnrt::OperandType::TENSOR_FLOAT16)
    .bw_input_weight_c2i_(nnrt::OperandType::TENSOR_FLOAT16, OPTIONAL)
    .bw_input_weight_c2f_(nnrt::OperandType::TENSOR_FLOAT16, OPTIONAL)
    .bw_input_weight_c2o_(nnrt::OperandType::TENSOR_FLOAT16, OPTIONAL)
    .bw_input_bias_i_(nnrt::OperandType::TENSOR_FLOAT16, OPTIONAL)
    .bw_input_bias_f_(nnrt::OperandType::TENSOR_FLOAT16)
    .bw_input_bias_c_(nnrt::OperandType::TENSOR_FLOAT16)
    .bw_input_bias_o_(nnrt::OperandType::TENSOR_FLOAT16)
    .bw_input_weight_proj_(nnrt::OperandType::TENSOR_FLOAT16, OPTIONAL)
    .bw_input_bias_proj_(nnrt::OperandType::TENSOR_FLOAT16, OPTIONAL)
    .fw_input_h_state_(nnrt::OperandType::TENSOR_FLOAT16)
    .fw_input_c_state_(nnrt::OperandType::TENSOR_FLOAT16)
    .bw_input_h_state_(nnrt::OperandType::TENSOR_FLOAT16)
    .bw_input_c_state_(nnrt::OperandType::TENSOR_FLOAT16)
    .aux_input_(nnrt::OperandType::TENSOR_FLOAT16, OPTIONAL)
    .fw_aux_input_weight_i2i_(nnrt::OperandType::TENSOR_FLOAT16, OPTIONAL)
    .fw_aux_input_weight_i2f_(nnrt::OperandType::TENSOR_FLOAT16, OPTIONAL)
    .fw_aux_input_weight_i2c_(nnrt::OperandType::TENSOR_FLOAT16, OPTIONAL)
    .fw_aux_input_weight_i2o_(nnrt::OperandType::TENSOR_FLOAT16, OPTIONAL)
    .bw_aux_input_weight_i2i_(nnrt::OperandType::TENSOR_FLOAT16, OPTIONAL)
    .bw_aux_input_weight_i2f_(nnrt::OperandType::TENSOR_FLOAT16, OPTIONAL)
    .bw_aux_input_weight_i2c_(nnrt::OperandType::TENSOR_FLOAT16, OPTIONAL)
    .bw_aux_input_weight_i2o_(nnrt::OperandType::TENSOR_FLOAT16, OPTIONAL)
    .fw_input_layernorm_i_(nnrt::OperandType::TENSOR_FLOAT16, OPTIONAL)
    .fw_input_layernorm_f_(nnrt::OperandType::TENSOR_FLOAT16, OPTIONAL)
    .fw_input_layernorm_c_(nnrt::OperandType::TENSOR_FLOAT16, OPTIONAL)
    .fw_input_layernorm_o_(nnrt::OperandType::TENSOR_FLOAT16, OPTIONAL)
    .bw_input_layernorm_i_(nnrt::OperandType::TENSOR_FLOAT16, OPTIONAL)
    .bw_input_layernorm_f_(nnrt::OperandType::TENSOR_FLOAT16, OPTIONAL)
    .bw_input_layernorm_c_(nnrt::OperandType::TENSOR_FLOAT16, OPTIONAL)
    .bw_input_layernorm_o_(nnrt::OperandType::TENSOR_FLOAT16, OPTIONAL)
    );

#undef ARG_NAMES
#undef ARGC
#undef OP_SPEC_NAME

//Output Spec
#define OP_SPEC_NAME BidirectionalSequenceLstmOutput
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