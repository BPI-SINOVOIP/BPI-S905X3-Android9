/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef LIBTEXTCLASSIFIER_UTIL_BASE_MACROS_H_
#define LIBTEXTCLASSIFIER_UTIL_BASE_MACROS_H_

#include "util/base/config.h"

namespace libtextclassifier2 {

#if LANG_CXX11
#define TC_DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName &) = delete;        \
  TypeName &operator=(const TypeName &) = delete
#else  // C++98 case follows

// Note that these C++98 implementations cannot completely disallow copying,
// as members and friends can still accidentally make elided copies without
// triggering a linker error.
#define TC_DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName &);                 \
  TypeName &operator=(const TypeName &)
#endif  // LANG_CXX11

// The TC_FALLTHROUGH_INTENDED macro can be used to annotate implicit
// fall-through between switch labels:
//
//  switch (x) {
//    case 40:
//    case 41:
//      if (truth_is_out_there) {
//        ++x;
//        TC_FALLTHROUGH_INTENDED;  // Use instead of/along with annotations in
//                                  // comments.
//      } else {
//        return x;
//      }
//    case 42:
//      ...
//
//  As shown in the example above, the TC_FALLTHROUGH_INTENDED macro should be
//  followed by a semicolon. It is designed to mimic control-flow statements
//  like 'break;', so it can be placed in most places where 'break;' can, but
//  only if there are no statements on the execution path between it and the
//  next switch label.
//
//  When compiled with clang in C++11 mode, the TC_FALLTHROUGH_INTENDED macro is
//  expanded to [[clang::fallthrough]] attribute, which is analysed when
//  performing switch labels fall-through diagnostic ('-Wimplicit-fallthrough').
//  See clang documentation on language extensions for details:
//  http://clang.llvm.org/docs/AttributeReference.html#fallthrough-clang-fallthrough
//
//  When used with unsupported compilers, the TC_FALLTHROUGH_INTENDED macro has
//  no effect on diagnostics.
//
//  In either case this macro has no effect on runtime behavior and performance
//  of code.
#if defined(__clang__) && defined(__has_warning)
#if __has_feature(cxx_attributes) && __has_warning("-Wimplicit-fallthrough")
#define TC_FALLTHROUGH_INTENDED [[clang::fallthrough]]
#endif
#elif defined(__GNUC__) && __GNUC__ >= 7
#define TC_FALLTHROUGH_INTENDED [[gnu::fallthrough]]
#endif

#ifndef TC_FALLTHROUGH_INTENDED
#define TC_FALLTHROUGH_INTENDED do { } while (0)
#endif

}  // namespace libtextclassifier2

#endif  // LIBTEXTCLASSIFIER_UTIL_BASE_MACROS_H_
