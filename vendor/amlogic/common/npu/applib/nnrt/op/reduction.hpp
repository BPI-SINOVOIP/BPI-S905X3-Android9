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
#ifndef _OP_REDUCTION_H_
#define _OP_REDUCTION_H_

#include <set>
#include <vector>

#include "operation.hpp"

namespace nnrt {
namespace op {

template <nnrt::OperationType reduction_op_type>
struct ReductionOperation : Operation {
    ReductionOperation() : Operation(reduction_op_type) {}

    virtual void handleLayoutInferenceOnInputs(
        Model& model,
        std::unordered_map<uint32_t, nnrt::layout_inference::IPermuteVectorPtr>&
            next_permute_vectors) {
        auto permuteVector = input_permute_cache_.cached_permutes_[inputs()[0]];

        for (uint32_t i = 0; i < axes.size(); ++i) {
            // convert axis to positive number
            if (axes[i] < 0) {
                axes[i] = permuteVector->rank() + axes[i];
            }
            // Convert axis to org platform format
            axes[i] = nnrt::op::utils::axisMapTo(permuteVector, axes[i]);
        }
        if (keepDim) {
            next_permute_vectors.insert(std::make_pair(outputs()[0], permuteVector));
        } else {
            std::set<int32_t> unique_axies;
            for (auto axis : axes) {
                unique_axies.insert(axis);
            }

            decltype(permuteVector) outputPermVec =
                layout_inference::make_shared(permuteVector->rank() - unique_axies.size());

            uint32_t j = 0;
            for (uint32_t i = 0; i < permuteVector->rank(); ++i) {
                if (unique_axies.end() != unique_axies.find(permuteVector->at(i))) continue;
                uint32_t cnt = 0;
                for (auto axis : unique_axies) {
                    if (permuteVector->at(i) > (uint32_t)axis) cnt++;
                }
                outputPermVec->at(j) = permuteVector->at(i) - cnt;
                j++;
            }
            next_permute_vectors.insert(std::make_pair(outputs()[0], outputPermVec));
        }
    }
    std::vector<int32_t> axes;
    bool keepDim{true};
};

using ReduceAllOperation = ReductionOperation<OperationType::REDUCE_ALL>;
using ReduceAnyOperation = ReductionOperation<OperationType::REDUCE_ANY>;
using ReduceMaxOperation = ReductionOperation<OperationType::REDUCE_MAX>;
using ReduceMinOperation = ReductionOperation<OperationType::REDUCE_MIN>;
using ReduceProdOperation = ReductionOperation<OperationType::REDUCE_PROD>;
using ReduceSumOperation = ReductionOperation<OperationType::REDUCE_SUM>;
using ReduceMeanOperation = ReductionOperation<OperationType::REDUCE_MEAN>;
}
}

#endif