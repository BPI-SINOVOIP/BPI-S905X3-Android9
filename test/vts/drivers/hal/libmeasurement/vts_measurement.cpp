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

#include "vts_measurement.h"

#if USE_CTIME
#include <time.h>
#else
#include <sys/time.h>
#endif
#include <stdlib.h>

#include <iostream>

#define COVERAGE_SANCOV 0

#if COVERAGE_SANCOV
#include <sanitizer/coverage_interface.h>
#endif

#include <vector>

using namespace std;

#if COVERAGE_SANCOV

extern "C" {
// Re-declare some of the sanitizer functions as "weak" so that
// libFuzzer can be linked w/o the sanitizers and sanitizer-coverage
// (in which case it will complain at start-up time).
__attribute__((weak)) void __sanitizer_print_stack_trace();
__attribute__((weak)) void __sanitizer_reset_coverage();
__attribute__((weak)) size_t __sanitizer_get_total_unique_caller_callee_pairs();
__attribute__((weak)) size_t __sanitizer_get_total_unique_coverage();
__attribute__((weak)) void __sanitizer_set_death_callback(
    void (*callback)(void));
__attribute__((weak)) size_t __sanitizer_get_number_of_counters();
__attribute__((weak)) uintptr_t
__sanitizer_update_counter_bitset_and_clear_counters(uint8_t* bitset);
}

#define CHECK_WEAK_API_FUNCTION(fn)       \
  do {                                    \
    if (!fn) MissingWeakApiFunction(#fn); \
  } while (false)

static void MissingWeakApiFunction(const char* FnName) {
  cerr << "ERROR: " << FnName << " is not defined. Exiting.\n"
       << "Did you use -fsanitize-coverage=... to build your code?" << endl;
  exit(1);
}
#endif

namespace android {
namespace vts {

void VtsMeasurement::Start() {
#if COVERAGE_SANCOV
  cout << "reset coverage";
  CHECK_WEAK_API_FUNCTION(__sanitizer_reset_coverage);
  __sanitizer_reset_coverage();
  cout << endl;
#endif

#if USE_CTIME
  gettimeofday(&tv_, NULL);
#else
  clock_gettime(CLOCK_REALTIME, &ts_);
#endif
}

vector<float>* VtsMeasurement::Stop() {
#if USE_CTIME
  struct timeval curr_tv;
  gettimeofday(&curr_tv, NULL);
#else
  timespec ts_now;
  clock_gettime(CLOCK_REALTIME, &ts_now);
#endif

  vector<float>* result = new vector<float>;

#if USE_CTIME
  float stop_time_usecs;
  float start_time_usecs;
  stop_time_usecs = curr_tv.tv_sec + curr_tv.tv_usec / 1000000.0;
  start_time_usecs = tv_.tv_sec + tv_.tv_usec / 1000000.0;
  result->push_back(stop_time_usecs - start_time_usecs);
#else
  float stop_time_usecs;
  float start_time_usecs;
  stop_time_usecs = ts_now.tv_sec + ts_now.tv_nsec / 1000000000.0;
  start_time_usecs = ts_.tv_sec + ts_.tv_nsec / 1000000000.0;
  result->push_back(stop_time_usecs - start_time_usecs);
#endif

#if COVERAGE_SANCOV
  cout << "coverage: ";
  cout << __sanitizer_get_total_unique_caller_callee_pairs() << endl;
#endif

  return result;
}

}  // namespace vts
}  // namespace android
