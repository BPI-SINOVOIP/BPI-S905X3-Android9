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


// Note: Compatible with RELU, RELU1, RELU6, TANH

#ifndef __ANEURALNETWORKS_ACTIVATION_HPP__
#define __ANEURALNETWORKS_ACTIVATION_HPP__

#include "hal_limitation/support_macros.hpp"

// Input Spec
#define OP_SPEC_NAME ActivationInput
OP_SPEC_BEGIN()
#define ARG_NAMES         \
    (input)
#define ARGC BOOST_PP_TUPLE_SIZE(ARG_NAMES)

#define BOOST_PP_LOCAL_MACRO(n) OP_SPEC_ARG(BOOST_PP_TUPLE_ELEM(ARGC, n, ARG_NAMES))
#define BOOST_PP_LOCAL_LIMITS (0, ARGC)
#include BOOST_PP_LOCAL_ITERATE()
OP_SPEC_END()

// order of argument is important
MAKE_SPEC(activation)
    .input_(nnrt::OperandType::TENSOR_FLOAT32));

    OVERRIDE_SPEC(activation, 0)
    .input_(nnrt::OperandType::TENSOR_FLOAT16));

    OVERRIDE_SPEC(activation, 1)
    .input_(nnrt::OperandType::TENSOR_QUANT8_ASYMM));

#undef ARG_NAMES
#undef ARGC
#undef OP_SPEC_NAME

#define OP_SPEC_NAME ActivationOutput
OP_SPEC_BEGIN()
#define ARG_NAMES         \
    (input,     \
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