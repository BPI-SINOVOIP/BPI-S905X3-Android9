// Copyright 2017 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef _BSDIFF_PATCH_READER_H_
#define _BSDIFF_PATCH_READER_H_

#include <stddef.h>
#include <stdint.h>

#include <memory>

#include "bsdiff/control_entry.h"
#include "bsdiff/decompressor_interface.h"
#include "bsdiff/file_interface.h"

namespace bsdiff {

// A wrapper class to read and parse the data in a bsdiff patch. The reader
// class contains the concatenated streams of control, diff, and extra data.
// After initialization, this class provides the function to read the metadata
// embedded in the control stream sequentially and decompresses the diff & extra
// stream based on this metadata.

class BsdiffPatchReader {
 public:
  BsdiffPatchReader() = default;

  // Initialize the control stream, diff stream and extra stream from the
  // corresponding offset of |patch_data|.
  bool Init(const uint8_t* patch_data, size_t patch_size);

  // Read the control stream and parse the metadata of |diff_size_|,
  // |extra_size_| and |offset_incremental_|.
  bool ParseControlEntry(ControlEntry* control_entry);

  // Read the data in |diff_stream_| and write |size| decompressed data to
  // |buf|.
  bool ReadDiffStream(uint8_t* buf, size_t size);

  // Read the data in |extra_stream_| and write |size| decompressed data to
  // |buf|.
  bool ReadExtraStream(uint8_t* buf, size_t size);

  uint64_t new_file_size() const { return new_file_size_; }

  // Close the control/diff/extra stream. Return false if errors occur when
  // closing any of these streams.
  bool Finish();

 private:
  // The compressed stream that contains the control data; i.e. length of each
  // diff/extra block and the corresponding offset to read in the source file.
  std::unique_ptr<DecompressorInterface> ctrl_stream_{nullptr};
  // The compressed stream that contains the concatenated diff blocks.
  std::unique_ptr<DecompressorInterface> diff_stream_{nullptr};
  // The compressed stream that contains the concatenated extra blocks.
  std::unique_ptr<DecompressorInterface> extra_stream_{nullptr};

  // Size of the target file.
  uint64_t new_file_size_;
};

}  // namespace bsdiff

#endif  // _BSDIFF_PATCH_READER_H_
