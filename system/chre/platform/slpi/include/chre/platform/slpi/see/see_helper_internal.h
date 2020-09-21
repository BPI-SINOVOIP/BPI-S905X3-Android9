/*
 * Copyright (C) 2018 The Android Open Source Project
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

#ifndef CHRE_PLATFORM_SLPI_SEE_SEE_HELPER_INTERNAL_H_
#define CHRE_PLATFORM_SLPI_SEE_SEE_HELPER_INTERNAL_H_

#include <stddef.h>
#include <stdint.h>

#include "sns_suid.pb.h"

#include "chre/util/optional.h"

namespace chre {

//! A struct to store a sensor's calibration data
struct SeeCalData {
  float bias[3];
  float scale[3];
  float matrix[9];
  bool hasBias;
  bool hasScale;
  bool hasMatrix;
  uint8_t accuracy;
};

//! A struct to store a cal sensor's UID and its cal data.
struct SeeCalInfo {
  Optional<sns_std_suid> suid;
  SeeCalData cal;
};

//! The list of SEE cal sensors supported.
enum class SeeCalSensor {
  AccelCal,
  GyroCal,
  MagCal,
  NumCalSensors,
};

//! A convenience constant.
constexpr size_t kNumSeeCalSensors = static_cast<size_t>(
    SeeCalSensor::NumCalSensors);

}  // namespace chre

#endif  // CHRE_PLATFORM_SLPI_SEE_SEE_HELPER_INTERNAL_H_
