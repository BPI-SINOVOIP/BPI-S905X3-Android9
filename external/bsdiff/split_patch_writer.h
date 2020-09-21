// Copyright 2017 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef _BSDIFF_SPLIT_PATCH_WRITER_H_
#define _BSDIFF_SPLIT_PATCH_WRITER_H_

#include <stdint.h>

#include <vector>

#include "bsdiff/patch_writer_interface.h"

namespace bsdiff {

// A PatchWriterInterface class that splits the output patch into multiple
// patches of a fixed new-file size. The size of each patch data will depend on
// the contents of the new file data, and won't necessarily be uniform.
class SplitPatchWriter : public PatchWriterInterface {
 public:
  // Create a PatchWriter that will split the patch in several patches where
  // each one will write |new_chunk_size| bytes of new file data. Each patch
  // will use the old file as a whole input file.
  SplitPatchWriter(uint64_t new_chunk_size,
                   const std::vector<PatchWriterInterface*>& patches)
      : new_chunk_size_(new_chunk_size), patches_(patches) {
    diff_sizes_.resize(patches.size());
    extra_sizes_.resize(patches.size());
  }

  // PatchWriterInterface overrides.
  // Note: Calling WriteDiffStream/WriteExtraStream before calling the
  // corresponding AddControlEntry() is not supported and will fail. The reason
  // for this is because which underlying patch takes the bytes depends on the
  // control entries.
  bool Init(size_t new_size) override;
  bool WriteDiffStream(const uint8_t* data, size_t size) override;
  bool WriteExtraStream(const uint8_t* data, size_t size) override;
  bool AddControlEntry(const ControlEntry& entry) override;
  bool Close() override;

 private:
  // Add a ControlEntry to the |current_patch_| without splitting it. Updates
  // the internal structures of used data.
  bool AddControlEntryToCurrentPatch(const ControlEntry& entry);

  using WriteStreamMethod = bool (PatchWriterInterface::*)(const uint8_t* data,
                                                           size_t size);

  // Write to the diff or extra stream as determined by |method|.
  bool WriteToStream(WriteStreamMethod method,
                     std::vector<size_t>* sizes_vector,
                     const uint8_t* data,
                     size_t size);

  // The size of the new file for the patch we are writing.
  size_t new_size_{0};

  // The size of each chunk of the new file written to.
  uint64_t new_chunk_size_;
  std::vector<PatchWriterInterface*> patches_;

  // The size of the diff and extra streams that should go in each patch and has
  // been written so far.
  std::vector<size_t> diff_sizes_;
  std::vector<size_t> extra_sizes_;

  // The current patch number in the |patches_| array we are writing to.
  size_t current_patch_{0};

  // The number of patches we already called Close() on. The patches are always
  // closed in order.
  size_t closed_patches_{0};

  // Bytes of the new files already written. Needed to store the new length in
  // the header of the file.
  uint64_t written_output_{0};

  // The current pointer into the old stream.
  uint64_t old_pos_{0};
};

}  // namespace bsdiff

#endif  // _BSDIFF_SPLIT_PATCH_WRITER_H_
