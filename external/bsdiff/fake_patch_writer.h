// Copyright 2017 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef _BSDIFF_FAKE_PATCH_WRITER_H_
#define _BSDIFF_FAKE_PATCH_WRITER_H_

#include <gtest/gtest.h>
#include <vector>

#include "bsdiff/patch_writer_interface.h"

namespace bsdiff {

// A fake PatchWriterInterface derived class with easy access to the data passed
// to it.
class FakePatchWriter : public PatchWriterInterface {
 public:
  FakePatchWriter() = default;
  ~FakePatchWriter() override = default;

  // PatchWriterInterface overrides.
  bool Init(size_t new_size) override {
    EXPECT_FALSE(initialized_);
    initialized_ = true;
    new_size_ = new_size;
    return true;
  }

  bool AddControlEntry(const ControlEntry& entry) override {
    EXPECT_TRUE(initialized_);
    EXPECT_FALSE(closed_);
    entries_.push_back(entry);
    return true;
  }

  bool WriteDiffStream(const uint8_t* data, size_t size) override {
    diff_stream_.insert(diff_stream_.end(), data, data + size);
    return true;
  }

  bool WriteExtraStream(const uint8_t* data, size_t size) override {
    extra_stream_.insert(extra_stream_.end(), data, data + size);
    return true;
  }

  bool Close() override {
    EXPECT_FALSE(closed_) << "Close() already called";
    closed_ = true;
    return true;
  }

  // Fake getter methods.
  const std::vector<ControlEntry>& entries() const { return entries_; }
  const std::vector<uint8_t>& diff_stream() const { return diff_stream_; }
  const std::vector<uint8_t>& extra_stream() const { return extra_stream_; }
  size_t new_size() const { return new_size_; }

 private:
  // The list of ControlEntry passed to this class.
  std::vector<ControlEntry> entries_;
  std::vector<uint8_t> diff_stream_;
  std::vector<uint8_t> extra_stream_;

  // The size of the new file for the patch we are writing.
  size_t new_size_{0};

  // Whether this class was initialized.
  bool initialized_{false};

  // Whether the patch was closed.
  bool closed_{false};
};

}  // namespace bsdiff

#endif  // _BSDIFF_FAKE_PATCH_WRITER_H_
