/*
 * Copyright 2017 The Android Open Source Project
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

#ifndef VTS_COMPILATION_TOOLS_VTSC_CODE_GEN_COMMON_HALHIDLCODEGENUTILS_H_
#define VTS_COMPILATION_TOOLS_VTSC_CODE_GEN_COMMON_HALHIDLCODEGENUTILS_H_

#include "test/vts/proto/ComponentSpecificationMessage.pb.h"

namespace android {
namespace vts {

// Returns true iff type is elidable.
bool IsElidableType(const VariableType& type);
// Returns true if a HIDL type uses 'const' in its native C/C++ form.
bool IsConstType(const VariableType& type);

}  // namespace vts
}  // namespace android

#endif  // VTS_COMPILATION_TOOLS_VTSC_CODE_GEN_COMMON_HALHIDLCODEGENUTILS_H_
