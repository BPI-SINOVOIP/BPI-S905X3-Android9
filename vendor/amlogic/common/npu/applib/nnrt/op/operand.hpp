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
#ifndef __OVXLIB_OPERAND_H__
#define __OVXLIB_OPERAND_H__

#include <cassert>
#include <memory>
#include <vector>

#include "nnrt/types.hpp"
#include "nnrt/memory_pool.hpp"
#include "nnrt/permute_vector.hpp"

namespace nnrt{
namespace op {
static constexpr uint32_t NNRT_INVALID_OPERAND_INDEX = 0xFFFFFFFF;

struct QuantizationParams {
    struct {
        float scale;
        int32_t zeroPoint;
    } scalar;

    struct {
        uint32_t channelDim;/* The index of the channel dimension. */
        std::vector<float> scale;
        std::vector<int32_t> zeroPoint;
    } vec;
};

struct BaseOperand {
    OperandType type;
    std::vector<uint32_t> dimensions;
    QuantizationParams quant;
    union {
        bool boolean;
        int32_t int32;
        uint32_t uint32;
        uint16_t float16;
        float float32;
        double float64;
    } scalar;

    mem_pool::weak_ref weak_mem_ref;
};

class Operand;

using OperandPtr = std::shared_ptr<Operand>;

class Operand : public BaseOperand {
   public:
    void setPerm(const std::vector<uint32_t>& perm) { perm_ = perm; }

    std::vector<uint32_t>& perm() { return perm_; };

    size_t bytes() const;

    size_t size() const {
        if (dimensions.size() == 0) {
            return 0;
        }
        uint32_t num = 1;
        for (auto i : dimensions) {
            num *= i;
        }
        return num;
    }

    size_t ndim() const { return dimensions.size(); }

    // life time
    // data location
    bool isTensor() const;

    bool isQuantized() const;

    bool isPerChannel() const;

    bool isConst() const {
        if (auto mem_ref = weak_mem_ref.lock()) {
            return (mem_ref->len_ > 0 && !is_graph_input_output_);
        } else {
            // NNRT_LOGW_PRINT("Operand Memory Isn't prepared yet: usually its input/output of model");
            return false;
        }
    }

    void setGraphInputOutput() { is_graph_input_output_ = true; }

    bool isNull() const { return optional_null_; }

    void setNull() { optional_null_ = true; }

    void clearNull() { optional_null_ = false; }

    bool isValid() {
        if (!isConst()) {
            return true;
        }
        for (auto i : dimensions) {
            if (i <= 0) {
                return false;
            }
        }
        return true;
    }

    OperandPtr clone();

    void echo(uint32_t index = 0) const;

    nnrt::layout_inference::IPermuteVectorPtr getPermVector() {
        return perm_vector_;
    }

    void setPermVector(nnrt::layout_inference::IPermuteVectorPtr permVector) {
        perm_vector_ = permVector;
    }

   private:
    void cloneQuantParams(Operand* operand);

   private:
    uint32_t number_of_consumers_;
    std::vector<uint32_t> perm_;
    nnrt::layout_inference::IPermuteVectorPtr perm_vector_ = nullptr;
    bool optional_null_ = false;
    bool is_graph_input_output_ = false;
};

}
}
#endif
