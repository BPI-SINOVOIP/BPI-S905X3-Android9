/*
 * Copyright (C) 2017 The Android Open Source Project
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

// Utility functions for working with FlatBuffers.

#ifndef LIBTEXTCLASSIFIER_UTIL_FLATBUFFERS_H_
#define LIBTEXTCLASSIFIER_UTIL_FLATBUFFERS_H_

#include <memory>
#include <string>

#include "model_generated.h"
#include "flatbuffers/flatbuffers.h"

namespace libtextclassifier2 {

// Loads and interprets the buffer as 'FlatbufferMessage' and verifies its
// integrity.
template <typename FlatbufferMessage>
const FlatbufferMessage* LoadAndVerifyFlatbuffer(const void* buffer, int size) {
  const FlatbufferMessage* message =
      flatbuffers::GetRoot<FlatbufferMessage>(buffer);
  if (message == nullptr) {
    return nullptr;
  }
  flatbuffers::Verifier verifier(reinterpret_cast<const uint8_t*>(buffer),
                                 size);
  if (message->Verify(verifier)) {
    return message;
  } else {
    return nullptr;
  }
}

// Same as above but takes string.
template <typename FlatbufferMessage>
const FlatbufferMessage* LoadAndVerifyFlatbuffer(const std::string& buffer) {
  return LoadAndVerifyFlatbuffer<FlatbufferMessage>(buffer.c_str(),
                                                    buffer.size());
}

// Loads and interprets the buffer as 'FlatbufferMessage', verifies its
// integrity and returns its mutable version.
template <typename FlatbufferMessage>
std::unique_ptr<typename FlatbufferMessage::NativeTableType>
LoadAndVerifyMutableFlatbuffer(const void* buffer, int size) {
  const FlatbufferMessage* message =
      LoadAndVerifyFlatbuffer<FlatbufferMessage>(buffer, size);
  if (message == nullptr) {
    return nullptr;
  }
  return std::unique_ptr<typename FlatbufferMessage::NativeTableType>(
      message->UnPack());
}

// Same as above but takes string.
template <typename FlatbufferMessage>
std::unique_ptr<typename FlatbufferMessage::NativeTableType>
LoadAndVerifyMutableFlatbuffer(const std::string& buffer) {
  return LoadAndVerifyMutableFlatbuffer<FlatbufferMessage>(buffer.c_str(),
                                                           buffer.size());
}

template <typename FlatbufferMessage>
const char* FlatbufferFileIdentifier() {
  return nullptr;
}

template <>
const char* FlatbufferFileIdentifier<Model>();

// Packs the mutable flatbuffer message to string.
template <typename FlatbufferMessage>
std::string PackFlatbuffer(
    const typename FlatbufferMessage::NativeTableType* mutable_message) {
  flatbuffers::FlatBufferBuilder builder;
  builder.Finish(FlatbufferMessage::Pack(builder, mutable_message),
                 FlatbufferFileIdentifier<FlatbufferMessage>());
  return std::string(reinterpret_cast<const char*>(builder.GetBufferPointer()),
                     builder.GetSize());
}

}  // namespace libtextclassifier2

#endif  // LIBTEXTCLASSIFIER_UTIL_FLATBUFFERS_H_
