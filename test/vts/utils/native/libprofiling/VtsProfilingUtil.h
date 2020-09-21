/*
 *  * Copyright (C) 2017 The Android Open Source Project
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing,
 *  software distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 */
#ifndef DRIVERS_HAL_COMMON_UTILS_VTSPROFILINGUTIL_H_
#define DRIVERS_HAL_COMMON_UTILS_VTSPROFILINGUTIL_H_

#include "google/protobuf/io/zero_copy_stream.h"
#include "google/protobuf/message_lite.h"

// This file defines methods for serialization of protobuf messages in the same
// way as Java's writeDelimitedTo()/parseDelimitedFrom().
namespace android {
namespace vts {

// Serializes one message to the out file, delimited by message size.
bool writeOneDelimited(const ::google::protobuf::MessageLite& message,
                       google::protobuf::io::ZeroCopyOutputStream* out);

// Deserialize one message from the in file, delimited by message size.
bool readOneDelimited(::google::protobuf::MessageLite* message,
                      google::protobuf::io::ZeroCopyInputStream* in);

}  // namespace vts
}  // namespace android
#endif  // DRIVERS_HAL_COMMON_UTILS_VTSPROFILINGUTIL_H_
