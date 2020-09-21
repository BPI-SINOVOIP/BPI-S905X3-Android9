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

#ifndef __vts_libdatatype_hal_gps_h__
#define __vts_libdatatype_hal_gps_h__

#include <hardware/gps.h>

namespace android {
namespace vts {

typedef double vts_gps_longitude;
typedef double vts_gps_latitude;
typedef float vts_gps_accuracy;
typedef uint16_t vts_gps_flags_uint16;

typedef uint32_t min_interval;
typedef uint32_t preferred_accuracy;
typedef uint32_t preferred_time;

// Generates a GpsCallback instance used for GPS HAL v1 init().
extern GpsCallbacks* GenerateGpsCallbacks();

// Generates a GpsUtcTime value which is int64_t.
extern GpsUtcTime GenerateGpsUtcTime();

// Generates a GPS latitude value.
extern double GenerateLatitude();

// Generates a GPS longitude value.
extern double GenerateLongitude();

// Generates a GPS accuracy value.
extern float GenerateGpsAccuracy();

// Generates a uint16_t flag value.
extern uint16_t GenerateGpsFlagsUint16();

// Generates a GpsPositionMode value which is uint32_t.
extern GpsPositionMode GenerateGpsPositionMode();

// Generates a GpsPositionRecurrence value which is uint32_t.
extern GpsPositionRecurrence GenerateGpsPositionRecurrence();

}  // namespace vts
}  // namespace android

#endif
