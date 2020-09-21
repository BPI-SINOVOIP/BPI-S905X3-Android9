// Copyright 2017 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef _BSDIFF_ENDSLEY_PATCH_WRITER_H_
#define _BSDIFF_ENDSLEY_PATCH_WRITER_H_

#include <memory>
#include <string>
#include <vector>

#include "bsdiff/compressor_interface.h"
#include "bsdiff/constants.h"
#include "bsdiff/patch_writer_interface.h"

namespace bsdiff {

// A PatchWriterInterface class compatible with the format used by Android Play
// Store's bsdiff implementation, which is based on Matthew Endsley's bsdiff
// implementation. See https://github.com/mendsley/bsdiff for the original
// implementation of this format. See also Google's APK patch size estimator for
// more information on the file-by-file format used by Play Store:
// https://github.com/googlesamples/apk-patch-size-estimator

// This format, identified by the "ENDSLEY/BSDIFF43" magic string, uses a single
// stream with the control entries, diff data and extra data interleaved. After
// the header, each Control Entry is stored in 24 bytes followed by the diff
// stream data for that entry only, and then followed by the extra stream data
// for that entry only. The format doesn't handle the compression of the data,
// instead, the whole file (including the magic string) is compressed with any
// compression algorithm.

// This format is easier to parse and allows the patch to be streamed, but by
// mixing the diff and extra data into the same compression context offers a
// slightly worse compression ratio (about 3.5% compared to upstream's format).

class EndsleyPatchWriter : public PatchWriterInterface {
 public:
  // Create the patch writer that will write the data to the passed vector
  // |patch|, resizing it as needed. The |patch| vector must be valid until
  // Close() is called or this patch is destroyed. The data in |patch| will be
  // compressed using the compressor type |type|.
  EndsleyPatchWriter(std::vector<uint8_t>* patch,
                     CompressorType type,
                     int quality)
      : patch_(patch), compressor_type_(type), quality_(quality) {}

  // PatchWriterInterface overrides.
  bool Init(size_t new_size) override;
  bool WriteDiffStream(const uint8_t* data, size_t size) override;
  bool WriteExtraStream(const uint8_t* data, size_t size) override;
  bool AddControlEntry(const ControlEntry& entry) override;
  bool Close() override;

 private:
  // Emit at the end of the |patch_| vector the passed control entry.
  void EmitControlEntry(const ControlEntry& entry);

  // Emit at the end of the |patch_| vector the passed buffer.
  void EmitBuffer(const uint8_t* data, size_t size);

  // Flush as much as possible of the pending data.
  void Flush();

  // The vector we are writing to, owned by the caller.
  std::vector<uint8_t>* patch_;

  // The compressor type to use and its quality (if any).
  CompressorType compressor_type_;
  int quality_;

  std::unique_ptr<CompressorInterface> compressor_;

  // The pending diff and extra data to be encoded in the file. These vectors
  // would not be used whenever is possible to the data directly to the patch_
  // vector; namely when the control, diff and extra stream data are provided in
  // that order for each control entry.
  std::vector<uint8_t> diff_data_;
  std::vector<uint8_t> extra_data_;
  std::vector<ControlEntry> control_;

  // Defined as the sum of all the diff_size and extra_size values in
  // |control_|. This is used to determine whether it is worth Flushing the
  // pending data.
  size_t pending_control_data_{0};

  // Number of bytes in the diff and extra stream that are pending in the
  // last control entry encoded in the |patch_|. If both are zero the last
  // control entry was completely emitted.
  size_t pending_diff_{0};
  size_t pending_extra_{0};
};

}  // namespace bsdiff

#endif  // _BSDIFF_ENDSLEY_PATCH_WRITER_H_
