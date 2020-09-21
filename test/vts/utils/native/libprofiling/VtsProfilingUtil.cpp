/*
 * Copyright(C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0(the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http:  // www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 *distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "VtsProfilingUtil.h"

#include <stdint.h>

#include "google/protobuf/io/coded_stream.h"

namespace android {
namespace vts {

bool writeOneDelimited(const google::protobuf::MessageLite& message,
                       google::protobuf::io::ZeroCopyOutputStream* out) {
  // We create a new coded stream for each message. This is fast.
  google::protobuf::io::CodedOutputStream output(out);

  // Write the size.
  const int size = message.ByteSize();
  output.WriteVarint32(size);

  uint8_t* buffer = output.GetDirectBufferForNBytesAndAdvance(size);
  if (buffer) {
    // Optimization: The message fits in one buffer, so use the faster
    // direct-to-array serialization path.
    message.SerializeWithCachedSizesToArray(buffer);
  } else {
    // Slightly-slower path when the message is multiple buffers.
    message.SerializeWithCachedSizes(&output);
    if (output.HadError()) {
      return false;
    }
  }

  return true;
}

bool readOneDelimited(google::protobuf::MessageLite* message,
                      google::protobuf::io::ZeroCopyInputStream* in) {
  // We create a new coded stream for each message. This is fast,
  // and it makes sure the 64MB total size limit is imposed per-message rather
  // than on the whole stream (See the CodedInputStream interface for more
  // info on this limit).
  google::protobuf::io::CodedInputStream input(in);

  // Read the size.
  uint32_t size;
  if (!input.ReadVarint32(&size)) {
    return false;
  }
  // Tell the stream not to read beyond that size.
  const auto limit = input.PushLimit(size);

  // Parse the message.
  if (!message->MergeFromCodedStream(&input)) {
    return false;
  }
  if (!input.ConsumedEntireMessage()) {
    return false;
  }

  // Release the limit.
  input.PopLimit(limit);

  return true;
}

}  // namespace vts
}  // namespace android
