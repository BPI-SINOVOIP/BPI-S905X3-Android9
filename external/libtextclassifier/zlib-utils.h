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

// Functions to compress and decompress low entropy entries in the model.

#ifndef LIBTEXTCLASSIFIER_ZLIB_UTILS_H_
#define LIBTEXTCLASSIFIER_ZLIB_UTILS_H_

#include <memory>

#include "model_generated.h"
#include "util/utf8/unilib.h"
#include "zlib.h"

namespace libtextclassifier2 {

class ZlibDecompressor {
 public:
  static std::unique_ptr<ZlibDecompressor> Instance();
  ~ZlibDecompressor();

  bool Decompress(const CompressedBuffer* compressed_buffer, std::string* out);

 private:
  ZlibDecompressor();
  z_stream stream_;
  bool initialized_;
};

class ZlibCompressor {
 public:
  static std::unique_ptr<ZlibCompressor> Instance();
  ~ZlibCompressor();

  void Compress(const std::string& uncompressed_content,
                CompressedBufferT* out);

 private:
  explicit ZlibCompressor(int level = Z_BEST_COMPRESSION,
                          // Tmp. buffer size was set based on the current set
                          // of patterns to be compressed.
                          int tmp_buffer_size = 64 * 1024);
  z_stream stream_;
  std::unique_ptr<Bytef[]> buffer_;
  unsigned int buffer_size_;
  bool initialized_;
};

// Compresses regex and datetime rules in the model in place.
bool CompressModel(ModelT* model);

// Decompresses regex and datetime rules in the model in place.
bool DecompressModel(ModelT* model);

// Compresses regex and datetime rules in the model.
std::string CompressSerializedModel(const std::string& model);

// Create and compile a regex pattern from optionally compressed pattern.
std::unique_ptr<UniLib::RegexPattern> UncompressMakeRegexPattern(
    const UniLib& unilib, const flatbuffers::String* uncompressed_pattern,
    const CompressedBuffer* compressed_pattern, ZlibDecompressor* decompressor,
    std::string* result_pattern_text = nullptr);

}  // namespace libtextclassifier2

#endif  // LIBTEXTCLASSIFIER_ZLIB_UTILS_H_
