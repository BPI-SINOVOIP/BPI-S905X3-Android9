// Copyright 2017 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef _BSDIFF_BSDIFF_ARGUMENTS_H_
#define _BSDIFF_BSDIFF_ARGUMENTS_H_

#include <stdint.h>

#include <string>

#include "bsdiff/constants.h"
#include "bsdiff/patch_writer_interface.h"

namespace bsdiff {

// Class to store the patch writer options about format, type and quality.
class BsdiffArguments {
 public:
  BsdiffArguments()
      : format_(BsdiffFormat::kLegacy),
        compressor_type_(CompressorType::kBZ2),
        compression_quality_(-1) {}

  BsdiffArguments(BsdiffFormat format, CompressorType type, int quality)
      : format_(format),
        compressor_type_(type),
        compression_quality_(quality) {}

  // Check if the compressor type is compatible with the bsdiff format.
  bool IsValid() const;

  // Getter functions.
  BsdiffFormat format() const { return format_; }

  int min_length() const { return min_length_; }

  CompressorType compressor_type() const { return compressor_type_; }

  int compression_quality() const { return compression_quality_; }

  // Parse the command line arguments of the main function and set all the
  // fields accordingly.
  bool ParseCommandLine(int argc, char** argv);

  // Parse the compression type from string.
  static bool ParseCompressorType(const std::string& str, CompressorType* type);

  // Parse the minimum length parameter from string.
  static bool ParseMinLength(const std::string& str, size_t* len);

  // Parse the bsdiff format from string.
  static bool ParseBsdiffFormat(const std::string& str, BsdiffFormat* format);

  // Parse the compression quality (for brotli) from string.
  static bool ParseQuality(const std::string& str, int* quality);

 private:
  // Current format supported are the legacy "BSDIFF40" or "BSDF2".
  BsdiffFormat format_;

  // The algorithm to compress the patch, i.e. BZ2 or Brotli.
  CompressorType compressor_type_;

  // The quality of compression, only valid when using brotli as the
  // compression algorithm.
  int compression_quality_;

  size_t min_length_{0};
};

}  // namespace bsdiff

#endif  // _BSDIFF_BSDIFF_ARGUMENTS_H_
