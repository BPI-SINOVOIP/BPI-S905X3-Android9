// Copyright 2017 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bsdiff/bsdiff_arguments.h"

#include <getopt.h>

#include <algorithm>
#include <iostream>

#include "brotli/encode.h"

using std::endl;
using std::string;

namespace {

// The name in string for the compression algorithms.
constexpr char kNoCompressionString[] = "nocompression";
constexpr char kBZ2String[] = "bz2";
constexpr char kBrotliString[] = "brotli";

// The name in string for the bsdiff format.
constexpr char kLegacyString[] = "legacy";
constexpr char kBsdf2String[] = "bsdf2";
constexpr char kBsdiff40String[] = "bsdiff40";
constexpr char kEndsleyString[] = "endsley";

const struct option OPTIONS[] = {
    {"format", required_argument, nullptr, 0},
    {"minlen", required_argument, nullptr, 0},
    {"type", required_argument, nullptr, 0},
    {"quality", required_argument, nullptr, 0},
    {nullptr, 0, nullptr, 0},
};

const uint32_t kBrotliDefaultQuality = BROTLI_MAX_QUALITY;

}  // namespace

namespace bsdiff {

bool BsdiffArguments::IsValid() const {
  if (compressor_type_ == CompressorType::kBrotli &&
      (compression_quality_ < BROTLI_MIN_QUALITY ||
       compression_quality_ > BROTLI_MAX_QUALITY)) {
    return false;
  }

  if (format_ == BsdiffFormat::kLegacy) {
    return compressor_type_ == CompressorType::kBZ2;
  } else if (format_ == BsdiffFormat::kBsdf2) {
    return (compressor_type_ == CompressorType::kBZ2 ||
            compressor_type_ == CompressorType::kBrotli);
  } else if (format_ == BsdiffFormat::kEndsley) {
    // All compression options are valid for this format.
    return true;
  }
  return false;
}

bool BsdiffArguments::ParseCommandLine(int argc, char** argv) {
  int opt;
  int option_index;
  while ((opt = getopt_long(argc, argv, "", OPTIONS, &option_index)) != -1) {
    if (opt != 0) {
      return false;
    }

    string name = OPTIONS[option_index].name;
    if (name == "format") {
      if (!ParseBsdiffFormat(optarg, &format_)) {
        return false;
      }
    } else if (name == "minlen") {
      if (!ParseMinLength(optarg, &min_length_)) {
        return false;
      }
    } else if (name == "type") {
      if (!ParseCompressorType(optarg, &compressor_type_)) {
        return false;
      }
    } else if (name == "quality") {
      if (!ParseQuality(optarg, &compression_quality_)) {
        return false;
      }
    } else {
      std::cerr << "Unrecognized options: " << name << endl;
      return false;
    }
  }

  // If quality is uninitialized for brotli, set it to default value.
  if (format_ != BsdiffFormat::kLegacy &&
      compressor_type_ == CompressorType::kBrotli &&
      compression_quality_ == -1) {
    compression_quality_ = kBrotliDefaultQuality;
  } else if (compressor_type_ != CompressorType::kBrotli &&
             compression_quality_ != -1) {
    std::cerr << "Warning: Compression quality is only used in the brotli"
                 " compressor." << endl;
  }

  return true;
}

bool BsdiffArguments::ParseCompressorType(const string& str,
                                          CompressorType* type) {
  string type_string = str;
  std::transform(type_string.begin(), type_string.end(), type_string.begin(),
                 ::tolower);
  if (type_string == kNoCompressionString) {
    *type = CompressorType::kNoCompression;
    return true;
  } else if (type_string == kBZ2String) {
    *type = CompressorType::kBZ2;
    return true;
  } else if (type_string == kBrotliString) {
    *type = CompressorType::kBrotli;
    return true;
  }
  std::cerr << "Failed to parse compressor type in " << str << endl;
  return false;
}

bool BsdiffArguments::ParseMinLength(const string& str, size_t* len) {
  errno = 0;
  char* end;
  const char* s = str.c_str();
  long result = strtol(s, &end, 10);
  if (errno != 0 || s == end || *end != '\0') {
    return false;
  }

  if (result < 0) {
    std::cerr << "Minimum length must be non-negative: " << str << endl;
    return false;
  }

  *len = result;
  return true;
}

bool BsdiffArguments::ParseBsdiffFormat(const string& str,
                                        BsdiffFormat* format) {
  string format_string = str;
  std::transform(format_string.begin(), format_string.end(),
                 format_string.begin(), ::tolower);
  if (format_string == kLegacyString || format_string == kBsdiff40String) {
    *format = BsdiffFormat::kLegacy;
    return true;
  } else if (format_string == kBsdf2String) {
    *format = BsdiffFormat::kBsdf2;
    return true;
  } else if (format_string == kEndsleyString) {
    *format = BsdiffFormat::kEndsley;
    return true;
  }
  std::cerr << "Failed to parse bsdiff format in " << str << endl;
  return false;
}

bool BsdiffArguments::ParseQuality(const string& str, int* quality) {
  errno = 0;
  char* end;
  const char* s = str.c_str();
  long result = strtol(s, &end, 10);
  if (errno != 0 || s == end || *end != '\0') {
    return false;
  }

  if (result < BROTLI_MIN_QUALITY || result > BROTLI_MAX_QUALITY) {
    std::cerr << "Compression quality out of range " << str << endl;
    return false;
  }

  *quality = result;
  return true;
}

}  // namespace bsdiff
