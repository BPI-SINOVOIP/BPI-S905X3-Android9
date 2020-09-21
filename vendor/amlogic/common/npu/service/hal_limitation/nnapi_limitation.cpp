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
#include "nnapi_limitation.hpp"

namespace hal {
namespace limitation {
namespace nnapi {

static OpSpecCollection nnapiSupportCollection;

MatchedArgumentPtr match(const std::string& name, const std::vector<nnrt::OperandType>& args) {
    auto matchedArg = nnapiSupportCollection.match(name, args);
    if (nullptr == matchedArg) {
        return MatchedArgumentPtr();
    }
    return std::make_shared<MatchedArgument>(args.size(), matchedArg);
}

void NNapiLimitationRegister(const std::string& opName, const IArgList* arglist) {
    if (nnapiSupportCollection.m_Collection.end() == nnapiSupportCollection.m_Collection.find(opName)) {
        std::vector<const IArgList*> args;
        args.push_back(arglist);
        nnapiSupportCollection.m_Collection.insert(std::make_pair(opName, args));
    } else {
        nnapiSupportCollection.m_Collection[opName].push_back(arglist);
    }
}
}  // end of namespace nnapi
}
}

#define OP_SPEC_REGISTER nnapi::NNapiLimitation
#include "nnapi_support/ANEURALNETWORKS_ABS.hpp"
#include "nnapi_support/ANEURALNETWORKS_CONV_2D.hpp"
#include "nnapi_support/ANEURALNETWORKS_ELEMENTWISE.hpp"
#include "nnapi_support/ANEURALNETWORKS_ACTIVATION.hpp"
#include "nnapi_support/ANEURALNETWORKS_RESIZE.hpp"
#include "nnapi_support/ANEURALNETWORKS_NORMALIZATION.hpp"
#include "nnapi_support/ANEURALNETWORKS_DEPTHWISE_CONV_2D.hpp"
#include "nnapi_support/ANEURALNETWORKS_TRANSPOSE_CONV_2D.hpp"
#include "nnapi_support/ANEURALNETWORKS_REDUCTION.hpp"
#include "nnapi_support/ANEURALNETWORKS_SOFTMAX.hpp"
#include "nnapi_support/ANEURALNETWORKS_POOL.hpp"
#include "nnapi_support/ANEURALNETWORKS_SQRT_RSQRT.hpp"
#include "nnapi_support/ANEURALNETWORKS_ARGMAX_ARGMIN.hpp"
#include "nnapi_support/ANEURALNETWORKS_MAXIMUM_MINIMUM.hpp"
#include "nnapi_support/ANEURALNETWORKS_LOG.hpp"
#include "nnapi_support/ANEURALNETWORKS_SIN.hpp"
#include "nnapi_support/ANEURALNETWORKS_EXP.hpp"
#include "nnapi_support/ANEURALNETWORKS_NEG.hpp"
#include "nnapi_support/ANEURALNETWORKS_PRELU.hpp"
#include "nnapi_support/ANEURALNETWORKS_UNIDIRECTIONAL_SEQUENCE_RNN.hpp"
#include "nnapi_support/ANEURALNETWORKS_BIDIRECTIONAL_SEQUENCE_RNN.hpp"
#include "nnapi_support/ANEURALNETWORKS_UNIDIRECTIONAL_SEQUENCE_LSTM.hpp"
#include "nnapi_support/ANEURALNETWORKS_BIDIRECTIONAL_SEQUENCE_LSTM.hpp"
#include "nnapi_support/ANEURALNETWORKS_GENERATE_PROPOSALS.hpp"
#include "nnapi_support/ANEURALNETWORKS_AXIS_ALIGNED_BBOX_TRANSFORM.hpp"
#include "nnapi_support/ANEURALNETWORKS_DETECTION_POSTPROCESSING.hpp"
#include "nnapi_support/ANEURALNETWORKS_COMPARISON.hpp"
#include "nnapi_support/ANEURALNETWORKS_LOGICAL.hpp"
#include "nnapi_support/ANEURALNETWORKS_EXPAND_DIMS.hpp"
#include "nnapi_support/ANEURALNETWORKS_POW.hpp"
#include "nnapi_support/ANEURALNETWORKS_SPLIT.hpp"
#include "nnapi_support/ANEURALNETWORKS_LOG_SOFTMAX.hpp"
#include "nnapi_support/ANEURALNETWORKS_GATHER.hpp"