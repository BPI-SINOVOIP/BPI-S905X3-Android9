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
#ifndef __VERSION__HPP__
#define __VERSION__HPP__

#include <cstdint>
#include <string>

#define NNRT_MAJOR_VERSION 1
#define NNRT_MINOR_VERSION 1
#define NNRT_PATCH_VERSION 7
#define _STR(A) #A
#define STR(A) _STR(A)
#ifdef GIT_STRING
#define VERSION_STR(mj, mi, pt) STR(mj) "." STR(mi) "." STR(pt) "." STR(GIT_STRING)
#else
#define VERSION_STR(mj, mi, pt) STR(mj) "." STR(mi) "." STR(pt)
#endif
namespace nnrt {
template <uint32_t Major, uint32_t Minor, uint32_t Patch>
struct Version {
    static_assert(Major < 100 && Minor < 100 && Patch < 100, "Invalid Version Number");
    static constexpr uint32_t value = Major * 10000U + Minor * 100U + Patch;

    static const char* as_str() {
        static const char* nnrt_version =
            VERSION_STR(NNRT_MAJOR_VERSION, NNRT_MINOR_VERSION, NNRT_PATCH_VERSION);
        return nnrt_version;
    }
};

using VERSION = Version<NNRT_MAJOR_VERSION, NNRT_MINOR_VERSION, NNRT_PATCH_VERSION>;
static constexpr uint32_t VERSION_NUM = VERSION::value;
};  // namespace nnrt

#undef STR
#undef _STR
#undef VERSION_STR
#endif
