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
#ifndef _OP_LAYOUT_INFERENCE_H_
#define _OP_LAYOUT_INFERENCE_H_
#include <unordered_map>
#include "nnrt/permute_vector.hpp"

namespace nnrt {
class Model;

namespace layout_inference {

class ILayoutInference {
   public:
     virtual ~ILayoutInference() = default;
    /**
    * @brief layout inference will spread permute vector for tensor data for each op in the model
    *
    * @detail
    *    [1] if : all input tensor setup a permute vector
    *       [1.1] current operation adapt to permute vectors
    *       [1.2] current operation generate permute vector for output tensor
    *    [2] else : stash permute vector for current input tensor
    *       [2.1] stash input_tensor local_idx and its permute vector
    *
    * @param model container of currepermute_vector.hlayoutInference may change operation in the
    * model
    * @param std::unordered_map<uint32_t, PermuteVector> : permute vectors retrived from previous
    * op
    * @return std::unordered_map<uint32_t, PermuteVector> : generate permute vector for each output
    * operand
    */
    virtual std::unordered_map<uint32_t, IPermuteVectorPtr> layoutInference(
        nnrt::Model& model, const std::unordered_map<uint32_t, IPermuteVectorPtr>& operand_permute) = 0;
};

}
}

#endif
