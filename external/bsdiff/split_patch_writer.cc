// Copyright 2017 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bsdiff/split_patch_writer.h"

#include <algorithm>

#include "bsdiff/logging.h"

namespace bsdiff {

bool SplitPatchWriter::Init(size_t new_size) {
  new_size_ = new_size;
  // Fail gracefully if re-initialized.
  if (current_patch_ || patches_.empty())
    return false;

  size_t expected_patches = (new_size_ + new_chunk_size_ - 1) / new_chunk_size_;
  if (expected_patches == 0)
    expected_patches = 1;
  if (expected_patches != patches_.size()) {
    LOG(ERROR) << "Expected " << expected_patches << " for a new file of size "
               << new_size_ << " split in chunks of " << new_chunk_size_
               << " but got " << patches_.size() << " instead.";
    return false;
  }

  return patches_[0]->Init(
      std::min(static_cast<uint64_t>(new_size_), new_chunk_size_));
}

bool SplitPatchWriter::WriteDiffStream(const uint8_t* data, size_t size) {
  return WriteToStream(&PatchWriterInterface::WriteDiffStream, &diff_sizes_,
                       data, size);
}

bool SplitPatchWriter::WriteExtraStream(const uint8_t* data, size_t size) {
  return WriteToStream(&PatchWriterInterface::WriteExtraStream, &extra_sizes_,
                       data, size);
}

bool SplitPatchWriter::AddControlEntry(const ControlEntry& entry) {
  ControlEntry remaining(entry);
  while (written_output_ + remaining.diff_size + remaining.extra_size >=
         (current_patch_ + 1) * new_chunk_size_) {
    // We need to write some of the current ControlEntry to the current patch
    // and move on to the next patch if there are more bytes to write.
    uint64_t remaining_bytes =
        (current_patch_ + 1) * new_chunk_size_ - written_output_;
    // The offset_increment is always 0 in this case since we don't plan to read
    // for the old file in the current_patch anymore.
    ControlEntry current_patch_entry(0, 0, 0);

    current_patch_entry.diff_size =
        std::min(remaining.diff_size, remaining_bytes);
    remaining_bytes -= current_patch_entry.diff_size;
    remaining.diff_size -= current_patch_entry.diff_size;

    // This will be positive only if we used all the diff_size bytes.
    current_patch_entry.extra_size =
        std::min(remaining.extra_size, remaining_bytes);
    remaining_bytes -= current_patch_entry.extra_size;
    remaining.extra_size -= current_patch_entry.extra_size;

    AddControlEntryToCurrentPatch(current_patch_entry);

    if (remaining.diff_size + remaining.extra_size > 0) {
      current_patch_++;
      if (current_patch_ >= patches_.size()) {
        LOG(ERROR) << "Writing past the last patch";
        return false;
      }
      if (!patches_[current_patch_]->Init(std::min(
              new_size_ - current_patch_ * new_chunk_size_, new_chunk_size_))) {
        LOG(ERROR) << "Failed to initialize patch " << current_patch_;
        return false;
      }
      if (!remaining.diff_size) {
        // When no diff need to be sent to the output, we can just push the
        // existing old_pos_ as part of the current triplet, since the extra
        // stream doesn't use the old_pos_;
        remaining.offset_increment += old_pos_;
        old_pos_ = 0;
      }
      // Need to add a dummy control entry at the beginning of the patch to
      // offset the old_pos in the new patch, which would start at 0.
      if (old_pos_ != 0) {
        if (!patches_[current_patch_]->AddControlEntry(
                ControlEntry(0, 0, old_pos_)))
          return false;
      }
    } else {
      // There was no need to write more bytes past the current patch, so just
      // update the old_pos_ we are tracking for the next patch, if any.
      old_pos_ += remaining.offset_increment;
      return true;
    }
  }

  // Trivial entries will be ignored.
  return AddControlEntryToCurrentPatch(remaining);
}

bool SplitPatchWriter::Close() {
  uint64_t missing_bytes = 0;
  for (auto size : diff_sizes_)
    missing_bytes += size;
  for (auto size : extra_sizes_)
    missing_bytes += size;
  if (missing_bytes > 0) {
    LOG(ERROR) << "Close() called but there are " << missing_bytes
               << " bytes missing from Write*Stream() calls";
    return false;
  }

  // |current_patch_| holds the last patch that was Init()'ed. If there are more
  // patches in the list those have not been initialized/closed, which is a
  // programming error.
  if (current_patch_ + 1 != patches_.size()) {
    LOG(ERROR)
        << "Close() called but no bytes habe been written to the last patch";
    return false;
  }

  // Close all the remaining streams.
  for (; closed_patches_ < patches_.size(); closed_patches_++) {
    if (!patches_[closed_patches_]->Close())
      return false;
  }
  return true;
}

bool SplitPatchWriter::AddControlEntryToCurrentPatch(
    const ControlEntry& entry) {
  // Ignore trivial control entries that don't modify the state.
  if (!entry.diff_size && !entry.extra_size && !entry.offset_increment)
    return true;

  if (current_patch_ >= patches_.size()) {
    LOG(ERROR) << "Writing past the last patch";
    return false;
  }
  old_pos_ += entry.diff_size + entry.offset_increment;
  written_output_ += entry.diff_size + entry.extra_size;
  // Register the diff/extra sizes as required bytes for the current patch.
  diff_sizes_[current_patch_] += entry.diff_size;
  extra_sizes_[current_patch_] += entry.extra_size;
  return patches_[current_patch_]->AddControlEntry(entry);
}

bool SplitPatchWriter::WriteToStream(WriteStreamMethod method,
                                     std::vector<size_t>* sizes_vector,
                                     const uint8_t* data,
                                     size_t size) {
  size_t written = 0;
  for (size_t i = closed_patches_; i <= current_patch_ && written < size; i++) {
    if ((*sizes_vector)[i]) {
      size_t flush_size = std::min(size - written, (*sizes_vector)[i]);
      if (!(patches_[i]->*method)(data + written, flush_size))
        return false;
      written += flush_size;
      (*sizes_vector)[i] -= flush_size;
    }

    if (i < current_patch_ && !diff_sizes_[i] && !extra_sizes_[i]) {
      // All bytes expected for the patch i are already sent.
      if (!patches_[i]->Close())
        return false;
      closed_patches_++;
    }
  }
  if (written < size) {
    LOG(ERROR) << "Calling Write*Stream() before the corresponding "
                  "AddControlEntry() is not supported.";
    return false;
  }
  return true;
}

}  // namespace bsdiff
