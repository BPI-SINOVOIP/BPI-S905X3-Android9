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

#ifndef __VTS_MEASUREMENT_H__
#define __VTS_MEASUREMENT_H__

#if USE_CTIME
#include <time.h>
#else
#include <sys/time.h>
#endif

#include <vector>

using namespace std;

namespace android {
namespace vts {

// Class to do measurements before and after calling a target function.
class VtsMeasurement {
 public:
  VtsMeasurement() {}

  // Starts the measurement
  void Start();

  // Stops the measurement and returns the measured values.
  vector<float>* Stop();

 private:
// data structure to keep the start time.
#if USE_CTIME
  struct timeval tv_;
#else
  timespec ts_;
#endif
};

}  // namespace vts
}  // namespace android

#endif
