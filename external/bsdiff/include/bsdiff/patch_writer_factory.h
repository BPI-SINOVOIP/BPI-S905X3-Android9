// Copyright 2017 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef _BSDIFF_PATCH_WRITER_FACTORY_H_
#define _BSDIFF_PATCH_WRITER_FACTORY_H_

#include <memory>
#include <string>
#include <vector>

#include "bsdiff/common.h"
#include "bsdiff/constants.h"
#include "bsdiff/patch_writer_interface.h"

namespace bsdiff {

// Create a patch writer compatible with upstream's "BSDIFF40" patch format
// using bz2 as a compressor.
BSDIFF_EXPORT
std::unique_ptr<PatchWriterInterface> CreateBsdiffPatchWriter(
    const std::string& patch_filename);

// Create a patch writer using the "BSDF2" patch format. It uses the compressor
// specified in |type| with compression quality |quality|.
BSDIFF_EXPORT
std::unique_ptr<PatchWriterInterface> CreateBSDF2PatchWriter(
    const std::string& patch_filename,
    CompressorType type,
    int quality);

// Create a patch writer compatible with Android Play Store bsdiff patches.
// The data will be written to the passed |patch| vector, which must be valid
// until Close() is called or this patch is destroyed. The data will be
// compressed using the compressor type |type|. To get an uncompressed patch,
// pass CompressortType::kNoCompression.
BSDIFF_EXPORT
std::unique_ptr<PatchWriterInterface> CreateEndsleyPatchWriter(
    std::vector<uint8_t>* patch,
    CompressorType type,
    int quality);

// Helper function to create an Endsley patch writer with no compression.
BSDIFF_EXPORT
std::unique_ptr<PatchWriterInterface> CreateEndsleyPatchWriter(
    std::vector<uint8_t>* patch);


}  // namespace bsdiff

#endif  // _BSDIFF_PATCH_WRITER_FACTORY_H_
