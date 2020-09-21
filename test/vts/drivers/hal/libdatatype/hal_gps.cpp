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

#include "hal_gps.h"

#include <pthread.h>
#include <stdlib.h>

#include <hardware/gps.h>

#include "vts_datatype.h"

namespace android {
namespace vts {

// Callbacks {
static void vts_gps_location_callback(GpsLocation* /*location*/) {}
static void vts_gps_status_callback(GpsStatus* /*status*/) {}
static void vts_gps_sv_status_callback(GpsSvStatus* /*sv_info*/) {}
static void vts_gps_nmea_callback(
    GpsUtcTime /*timestamp*/, const char* /*nmea*/, int /*length*/) {}
static void vts_gps_set_capabilities(uint32_t /*capabilities*/) {}
static void vts_gps_acquire_wakelock() {}
static void vts_gps_release_wakelock() {}
static void vts_gps_request_utc_time() {}

static pthread_t vts_gps_create_thread(
    const char* /*name*/, void (*/*start*/)(void*), void* /*arg*/) {
  return (pthread_t)NULL;
}
// } Callbacks

GpsCallbacks* GenerateGpsCallbacks() {
  if (RandomBool()) {
    return NULL;
  } else {
    GpsCallbacks* cbs = (GpsCallbacks*)malloc(sizeof(GpsCallbacks));
    cbs->size = sizeof(GpsCallbacks);
    cbs->location_cb = vts_gps_location_callback;
    cbs->status_cb = vts_gps_status_callback;
    cbs->sv_status_cb = vts_gps_sv_status_callback;
    cbs->nmea_cb = vts_gps_nmea_callback;
    cbs->set_capabilities_cb = vts_gps_set_capabilities;
    cbs->acquire_wakelock_cb = vts_gps_acquire_wakelock;
    cbs->release_wakelock_cb = vts_gps_release_wakelock;
    cbs->create_thread_cb = vts_gps_create_thread;
    cbs->request_utc_time_cb = vts_gps_request_utc_time;

    return cbs;
  }
}

GpsUtcTime /*int64_t*/ GenerateGpsUtcTime() {
  // TOOD: consider returning the current time + a random number.
  return RandomInt64();
}

double GenerateLatitude() { return 10.0; }

double GenerateLongitude() { return 20.0; }

float GenerateGpsAccuracy() { return 5.0; }

uint16_t GenerateGpsFlagsUint16() { return 1; }

GpsPositionMode /*uint32_t*/ GenerateGpsPositionMode() {
  if (RandomBool()) {
    return RandomUint32();
  } else {
    if (RandomBool()) {
      return GPS_POSITION_MODE_STANDALONE;
    } else if (RandomBool()) {
      return GPS_POSITION_MODE_MS_BASED;
    } else {
      return GPS_POSITION_MODE_MS_ASSISTED;
    }
  }
  return RandomUint32();
}

GpsPositionRecurrence /*uint32_t*/ GenerateGpsPositionRecurrence() {
  if (RandomBool()) {
    return RandomUint32();
  } else {
    if (RandomBool()) {
      return GPS_POSITION_RECURRENCE_PERIODIC;
    } else {
      return GPS_POSITION_RECURRENCE_SINGLE;
    }
  }
}

// TODO: add generators for min_interval, preferred_accuracy, and preferred_time
// all uint32_t.

}  // namespace vts
}  // namespace android
