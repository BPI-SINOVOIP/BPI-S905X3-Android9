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

#include "zlib-utils.h"

#include <memory>

#include "util/base/logging.h"
#include "util/flatbuffers.h"

namespace libtextclassifier2 {

std::unique_ptr<ZlibDecompressor> ZlibDecompressor::Instance() {
  std::unique_ptr<ZlibDecompressor> result(new ZlibDecompressor());
  if (!result->initialized_) {
    result.reset();
  }
  return result;
}

ZlibDecompressor::ZlibDecompressor() {
  memset(&stream_, 0, sizeof(stream_));
  stream_.zalloc = Z_NULL;
  stream_.zfree = Z_NULL;
  initialized_ = (inflateInit(&stream_) == Z_OK);
}

ZlibDecompressor::~ZlibDecompressor() {
  if (initialized_) {
    inflateEnd(&stream_);
  }
}

bool ZlibDecompressor::Decompress(const CompressedBuffer* compressed_buffer,
                                  std::string* out) {
  out->resize(compressed_buffer->uncompressed_size());
  stream_.next_in =
      reinterpret_cast<const Bytef*>(compressed_buffer->buffer()->Data());
  stream_.avail_in = compressed_buffer->buffer()->Length();
  stream_.next_out = reinterpret_cast<Bytef*>(const_cast<char*>(out->c_str()));
  stream_.avail_out = compressed_buffer->uncompressed_size();
  return (inflate(&stream_, Z_SYNC_FLUSH) == Z_OK);
}

std::unique_ptr<ZlibCompressor> ZlibCompressor::Instance() {
  std::unique_ptr<ZlibCompressor> result(new ZlibCompressor());
  if (!result->initialized_) {
    result.reset();
  }
  return result;
}

ZlibCompressor::ZlibCompressor(int level, int tmp_buffer_size) {
  memset(&stream_, 0, sizeof(stream_));
  stream_.zalloc = Z_NULL;
  stream_.zfree = Z_NULL;
  buffer_size_ = tmp_buffer_size;
  buffer_.reset(new Bytef[buffer_size_]);
  initialized_ = (deflateInit(&stream_, level) == Z_OK);
}

ZlibCompressor::~ZlibCompressor() { deflateEnd(&stream_); }

void ZlibCompressor::Compress(const std::string& uncompressed_content,
                              CompressedBufferT* out) {
  out->uncompressed_size = uncompressed_content.size();
  out->buffer.clear();
  stream_.next_in =
      reinterpret_cast<const Bytef*>(uncompressed_content.c_str());
  stream_.avail_in = uncompressed_content.size();
  stream_.next_out = buffer_.get();
  stream_.avail_out = buffer_size_;
  unsigned char* buffer_deflate_start_position =
      reinterpret_cast<unsigned char*>(buffer_.get());
  int status;
  do {
    // Deflate chunk-wise.
    // Z_SYNC_FLUSH causes all pending output to be flushed, but doesn't
    // reset the compression state.
    // As we do not know how big the compressed buffer will be, we compress
    // chunk wise and append the flushed content to the output string buffer.
    // As we store the uncompressed size, we do not have to do this during
    // decompression.
    status = deflate(&stream_, Z_SYNC_FLUSH);
    unsigned char* buffer_deflate_end_position =
        reinterpret_cast<unsigned char*>(stream_.next_out);
    if (buffer_deflate_end_position != buffer_deflate_start_position) {
      out->buffer.insert(out->buffer.end(), buffer_deflate_start_position,
                         buffer_deflate_end_position);
      stream_.next_out = buffer_deflate_start_position;
      stream_.avail_out = buffer_size_;
    } else {
      break;
    }
  } while (status == Z_OK);
}

// Compress rule fields in the model.
bool CompressModel(ModelT* model) {
  std::unique_ptr<ZlibCompressor> zlib_compressor = ZlibCompressor::Instance();
  if (!zlib_compressor) {
    TC_LOG(ERROR) << "Cannot compress model.";
    return false;
  }

  // Compress regex rules.
  if (model->regex_model != nullptr) {
    for (int i = 0; i < model->regex_model->patterns.size(); i++) {
      RegexModel_::PatternT* pattern = model->regex_model->patterns[i].get();
      pattern->compressed_pattern.reset(new CompressedBufferT);
      zlib_compressor->Compress(pattern->pattern,
                                pattern->compressed_pattern.get());
      pattern->pattern.clear();
    }
  }

  // Compress date-time rules.
  if (model->datetime_model != nullptr) {
    for (int i = 0; i < model->datetime_model->patterns.size(); i++) {
      DatetimeModelPatternT* pattern = model->datetime_model->patterns[i].get();
      for (int j = 0; j < pattern->regexes.size(); j++) {
        DatetimeModelPattern_::RegexT* regex = pattern->regexes[j].get();
        regex->compressed_pattern.reset(new CompressedBufferT);
        zlib_compressor->Compress(regex->pattern,
                                  regex->compressed_pattern.get());
        regex->pattern.clear();
      }
    }
    for (int i = 0; i < model->datetime_model->extractors.size(); i++) {
      DatetimeModelExtractorT* extractor =
          model->datetime_model->extractors[i].get();
      extractor->compressed_pattern.reset(new CompressedBufferT);
      zlib_compressor->Compress(extractor->pattern,
                                extractor->compressed_pattern.get());
      extractor->pattern.clear();
    }
  }
  return true;
}

namespace {

bool DecompressBuffer(const CompressedBufferT* compressed_pattern,
                      ZlibDecompressor* zlib_decompressor,
                      std::string* uncompressed_pattern) {
  std::string packed_pattern =
      PackFlatbuffer<CompressedBuffer>(compressed_pattern);
  if (!zlib_decompressor->Decompress(
          LoadAndVerifyFlatbuffer<CompressedBuffer>(packed_pattern),
          uncompressed_pattern)) {
    return false;
  }
  return true;
}

}  // namespace

bool DecompressModel(ModelT* model) {
  std::unique_ptr<ZlibDecompressor> zlib_decompressor =
      ZlibDecompressor::Instance();
  if (!zlib_decompressor) {
    TC_LOG(ERROR) << "Cannot initialize decompressor.";
    return false;
  }

  // Decompress regex rules.
  if (model->regex_model != nullptr) {
    for (int i = 0; i < model->regex_model->patterns.size(); i++) {
      RegexModel_::PatternT* pattern = model->regex_model->patterns[i].get();
      if (!DecompressBuffer(pattern->compressed_pattern.get(),
                            zlib_decompressor.get(), &pattern->pattern)) {
        TC_LOG(ERROR) << "Cannot decompress pattern: " << i;
        return false;
      }
      pattern->compressed_pattern.reset(nullptr);
    }
  }

  // Decompress date-time rules.
  if (model->datetime_model != nullptr) {
    for (int i = 0; i < model->datetime_model->patterns.size(); i++) {
      DatetimeModelPatternT* pattern = model->datetime_model->patterns[i].get();
      for (int j = 0; j < pattern->regexes.size(); j++) {
        DatetimeModelPattern_::RegexT* regex = pattern->regexes[j].get();
        if (!DecompressBuffer(regex->compressed_pattern.get(),
                              zlib_decompressor.get(), &regex->pattern)) {
          TC_LOG(ERROR) << "Cannot decompress pattern: " << i << " " << j;
          return false;
        }
        regex->compressed_pattern.reset(nullptr);
      }
    }
    for (int i = 0; i < model->datetime_model->extractors.size(); i++) {
      DatetimeModelExtractorT* extractor =
          model->datetime_model->extractors[i].get();
      if (!DecompressBuffer(extractor->compressed_pattern.get(),
                            zlib_decompressor.get(), &extractor->pattern)) {
        TC_LOG(ERROR) << "Cannot decompress pattern: " << i;
        return false;
      }
      extractor->compressed_pattern.reset(nullptr);
    }
  }
  return true;
}

std::string CompressSerializedModel(const std::string& model) {
  std::unique_ptr<ModelT> unpacked_model = UnPackModel(model.c_str());
  TC_CHECK(unpacked_model != nullptr);
  TC_CHECK(CompressModel(unpacked_model.get()));
  flatbuffers::FlatBufferBuilder builder;
  FinishModelBuffer(builder, Model::Pack(builder, unpacked_model.get()));
  return std::string(reinterpret_cast<const char*>(builder.GetBufferPointer()),
                     builder.GetSize());
}

std::unique_ptr<UniLib::RegexPattern> UncompressMakeRegexPattern(
    const UniLib& unilib, const flatbuffers::String* uncompressed_pattern,
    const CompressedBuffer* compressed_pattern, ZlibDecompressor* decompressor,
    std::string* result_pattern_text) {
  UnicodeText unicode_regex_pattern;
  std::string decompressed_pattern;
  if (compressed_pattern != nullptr &&
      compressed_pattern->buffer() != nullptr) {
    if (decompressor == nullptr ||
        !decompressor->Decompress(compressed_pattern, &decompressed_pattern)) {
      TC_LOG(ERROR) << "Cannot decompress pattern.";
      return nullptr;
    }
    unicode_regex_pattern =
        UTF8ToUnicodeText(decompressed_pattern.data(),
                          decompressed_pattern.size(), /*do_copy=*/false);
  } else {
    if (uncompressed_pattern == nullptr) {
      TC_LOG(ERROR) << "Cannot load uncompressed pattern.";
      return nullptr;
    }
    unicode_regex_pattern =
        UTF8ToUnicodeText(uncompressed_pattern->c_str(),
                          uncompressed_pattern->Length(), /*do_copy=*/false);
  }

  if (result_pattern_text != nullptr) {
    *result_pattern_text = unicode_regex_pattern.ToUTF8String();
  }

  std::unique_ptr<UniLib::RegexPattern> regex_pattern =
      unilib.CreateRegexPattern(unicode_regex_pattern);
  if (!regex_pattern) {
    TC_LOG(ERROR) << "Could not create pattern: "
                  << unicode_regex_pattern.ToUTF8String();
  }
  return regex_pattern;
}

}  // namespace libtextclassifier2
