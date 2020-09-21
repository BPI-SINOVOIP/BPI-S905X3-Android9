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

#include "HalHidlCodeGenUtils.h"

namespace android {
namespace vts {

bool IsElidableType(const VariableType& type) {
  if (type == TYPE_SCALAR || type == TYPE_ENUM || type == TYPE_MASK ||
      type == TYPE_POINTER || type == TYPE_HIDL_INTERFACE ||
      type == TYPE_VOID) {
    return true;
  }
  return false;
}

bool IsConstType(const VariableType& type) {
  if (type == TYPE_ARRAY || type == TYPE_VECTOR || type == TYPE_REF ||
      type == TYPE_HIDL_INTERFACE) {
    return true;
  }
  if (IsElidableType(type)) {
    return false;
  }
  return true;
}

}  // namespace vts
}  // namespace android
