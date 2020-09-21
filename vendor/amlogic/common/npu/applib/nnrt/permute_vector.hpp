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
#ifndef __PERMUTE_VECTOR__
#define __PERMUTE_VECTOR__

#include <array>
#include <vector>
#include <cassert>

#include "types.hpp"

namespace nnrt {
namespace layout_inference {
class IPermuteVector;
using IPermuteVectorPtr = std::shared_ptr<IPermuteVector>;

class IPermuteVector {
   public:
    virtual ~IPermuteVector() = default;
    virtual uint32_t rank() const = 0;

    virtual const uint32_t& at(const uint32_t) const = 0;
    virtual uint32_t& at(const uint32_t) = 0;

    /**
     * @brief get reverse permute vector
     *
     * PermuteVector + PermuteVector.reverse() = {0, 1, 2...R}
     *
     * Data layout = NHWC, current Permute = 0, 3, 1, 2, output layout = NCHW
     * its reverse layout is 0, 2, 3, 1
     *
     * @return PermuteVector<R> reverse permute vector have same rank as current permute
     */
    virtual IPermuteVectorPtr reverse() = 0;

    virtual std::string asText() const = 0;

    /**
     * @brief apply addtional permute parameter
     *
     * @detail
     *   assume data stored as NHWC, this->param_ = {0, 3, 1, 2}
     *   if apply current permute vector, data stored as NCHW
     *   other->param_ = {0, 2, 1, 3}
     *   if apply the addtion permute, data stored as NHCW, current permute paramter become {0, 1,
     * 3, 2}
     *
     * @param other addtional permute vector
     * @return PermuteVector result = data.apply_this_permute().apply_other_permute()
     */
    virtual IPermuteVectorPtr add(const IPermuteVectorPtr& other) const = 0;

    virtual void reinitialize() = 0;

    virtual bool isAligned() const = 0 ;

    virtual std::vector<uint32_t> asStdVec() const = 0;
};

template <uint32_t R>
class PermuteVector : public IPermuteVector {
   public:
    static constexpr uint32_t MAX_RANK = 10;

    PermuteVector() {
        for (uint32_t i = 0; i < R; ++i) {
            param_[i] = i;
        }
    }
    // Copy Constructor
    PermuteVector(const PermuteVector& other): param_(other.param_) {
    }

    // Move Constructor
    PermuteVector(PermuteVector&& other): param_(std::move(other.param_)) {
    }

    // Initialize list
    PermuteVector(std::initializer_list<uint32_t> init_list) {
        std::vector<uint32_t> vec(init_list);
        assert(vec.size() == R);

        for(uint32_t i = 0; i < R; ++i) {
            param_[i] = vec[i];
        }
    }

    template <uint32_t S>
    explicit PermuteVector(const PermuteVector<S>& smaller) {
        // With this: you can construct a PermuteVector with larger Rank from a smaller rank permute
        static_assert(S < R, "Cut Permute Vector is not allowed");
        for (auto i = 0; i < R; ++i) {
            param_[i] = i < S ? smaller[i] : i;
        }
    }

    virtual const uint32_t& at(uint32_t idx) const override { return param_[idx]; }

    virtual uint32_t& at(uint32_t idx) override { return param_[idx]; }

    virtual uint32_t rank() const override { return R; }

    virtual bool isAligned() const override {
        uint32_t i = 0;
        for (; i < R; ++i) {
            if (i != param_[i]) break;
        }

        return i == R;
    }

    IPermuteVectorPtr reverse() override {
        IPermuteVectorPtr r = std::make_shared<PermuteVector<R>>();
        for (uint32_t i = 0; i < R; ++i) {
            r->at(param_[i]) = i;
        }
        return r;
    }

    void reinitialize() override {
        for (uint32_t i = 0; i < R; ++i) {
            param_[i] = i;
        }
    }

    virtual IPermuteVectorPtr add(const IPermuteVectorPtr& other) const override {
        IPermuteVectorPtr r = std::make_shared<PermuteVector<R>>();
        for (uint32_t i = 0; i < other->rank(); ++i){
            r->at(i) = param_[other->at(i)];
        }
        return r;
    }

    virtual std::string asText() const override {
        std::string str(R + 1, '\0');
        for (uint32_t i = 0; i < R; i++) {
            str[i] = (char(param_[i]));
        }
        // str[R] = '\0';
        return str;
    }

    virtual std::vector<uint32_t> asStdVec() const override {
        std::vector<uint32_t> data(R);

        for (uint32_t i(0); i < R; ++i) {
            data[i] = param_[i];
        }
        // return std::move(data);
        return data;
    }

   private:
    std::array<uint32_t, R> param_;
};

using PermuteVector1 = PermuteVector<1>;

/**
 * @brief
 *
 * @param rankVal
 * @return IPermuteVectorPtr
 */
inline IPermuteVectorPtr make_shared(uint32_t rankVal) {
    switch (rankVal) {
        // 0: represent scalar
        case 0:
        case 1:
            return std::make_shared<PermuteVector<1>>();
        case 2:
            return std::make_shared<PermuteVector<2>>();
        case 3:
            return std::make_shared<PermuteVector<3>>();
        case 4:
            return std::make_shared<PermuteVector<4>>();
        case 5:
            return std::make_shared<PermuteVector<5>>();
        case 6:
            return std::make_shared<PermuteVector<6>>();
        case 7:
            return std::make_shared<PermuteVector<7>>();
        case 8:
            return std::make_shared<PermuteVector<8>>();
        case 9:
            return std::make_shared<PermuteVector<9>>();
        case 10:
            return std::make_shared<PermuteVector<10>>();
        default:
            assert("Not supported rankVal");
            return nullptr;
    }
}
}
}

#endif
