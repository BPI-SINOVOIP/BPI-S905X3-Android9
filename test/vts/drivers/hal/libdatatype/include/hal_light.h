/*
 * Copyright 2016 The Android Open Source Project
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

#ifndef __vts_libdatatype_hal_light_h__
#define __vts_libdatatype_hal_light_h__

#include <hardware/lights.h>

#include "test/vts/proto/ComponentSpecificationMessage.pb.h"

namespace android {
namespace vts {

// Generates light_state_t instance.
light_state_t* GenerateLightState();

light_state_t* GenerateLightStateUsingMessage(
    const VariableSpecificationMessage& msg);

}  // namespace vts
}  // namespace android

#endif
