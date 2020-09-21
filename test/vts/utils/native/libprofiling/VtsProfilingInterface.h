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

#ifndef __VTS_DRIVER_PROFILING_INTERFACE_H_
#define __VTS_DRIVER_PROFILING_INTERFACE_H_

#include <android-base/macros.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <hidl/HidlSupport.h>
#include <utils/Condition.h>
#include <fstream>

#include "test/vts/proto/ComponentSpecificationMessage.pb.h"

using namespace std;

namespace android {
namespace vts {

// Library class to trace, record and profile a HIDL HAL implementation.
class VtsProfilingInterface {
 public:
  explicit VtsProfilingInterface(const string& trace_file_path);

  virtual ~VtsProfilingInterface();

  // Get and create the VtsProfilingInterface singleton.
  static VtsProfilingInterface& getInstance(const string& trace_file_path);

  // returns true if the given message is added to the tracing queue.
  void AddTraceEvent(
      android::hardware::details::HidlInstrumentor::InstrumentationEvent event,
      const char* package, const char* version, const char* interface,
      const FunctionSpecificationMessage& message);

 private:
  // Internal method to get the corresponding trace file descriptor for a HAL
  // with given package and version.
  int GetTraceFile(const string& package, const string& version);
  // Internal method to create a trace file based on the trace_file_path_prefix_
  // the given package and version, the device info and the current time.
  int CreateTraceFile(const string& package, const string& version);
  // Get the current time in nano seconds.
  int64_t NanoTime();

  // Prefix of all trace files.
  string trace_file_path_prefix_;  // Prefix of the trace file.

  // map between the HAL to the trace file description.
  map<string, int> trace_map_;
  Mutex mutex_;  // Mutex used to synchronize the writing to the trace file.

  DISALLOW_COPY_AND_ASSIGN(VtsProfilingInterface);
};

}  // namespace vts
}  // namespace android

#endif  // __VTS_DRIVER_PROFILING_INTERFACE_H_
