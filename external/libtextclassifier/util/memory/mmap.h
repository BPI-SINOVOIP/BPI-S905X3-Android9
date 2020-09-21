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

#ifndef LIBTEXTCLASSIFIER_UTIL_MEMORY_MMAP_H_
#define LIBTEXTCLASSIFIER_UTIL_MEMORY_MMAP_H_

#include <stddef.h>

#include <string>

#include "util/base/integral_types.h"
#include "util/strings/stringpiece.h"

namespace libtextclassifier2 {

// Handle for a memory area where a file has been mmapped.
//
// Similar to a pointer: you "allocate" it using MmapFile(filename) and "delete"
// it using Unmap().  Just like a pointer, it is passed around by value (see
// signature of MmapFile and Unmap; fortunately, it's a small class, so there
// shouldn't be any significant performance penalty) and its usage is not
// necessarily scoped (that's why the destructor is not performing the unmap).
//
// Note: on program termination, each still unmapped file is automatically
// unmapped.  Hence, it is not an error if you don't call Unmap() (provided you
// are ok keeping that file in memory the whole time).
class MmapHandle {
 public:
  MmapHandle(void *start, size_t num_bytes, void *unmap_addr = nullptr)
      : start_(start), num_bytes_(num_bytes), unmap_addr_(unmap_addr) {}

  // Returns start address for the memory area where a file has been mmapped.
  void *start() const { return start_; }

  // Returns address to use for munmap call. If unmap_addr was not specified
  // the start address is used.
  void *unmap_addr() const {
    if (unmap_addr_ != nullptr) {
      return unmap_addr_;
    } else {
      return start_;
    }
  }

  // Returns number of bytes of the memory area from start().
  size_t num_bytes() const { return num_bytes_; }

  // Shortcut to simplify checking success of MmapFile().  See usage example
  // from the doc of that function.
  bool ok() const { return start() != nullptr; }

  // Returns a StringPiece pointing to the same underlying bytes.
  StringPiece to_stringpiece() const {
    return StringPiece(reinterpret_cast<char *>(start_), num_bytes_);
  }

 private:
  // See doc for start().  Not owned.
  void *const start_;

  // See doc for num_bytes().
  const size_t num_bytes_;

  // Address to use for unmapping.
  void *const unmap_addr_;
};

// Maps the full content of a file in memory (using mmap).
//
// When done using the file content, one can unmap using Unmap().  Otherwise,
// all mapped files are unmapped when the program terminates.
//
// Sample usage:
//
// MmapHandle mmap_handle = MmapFile(filename);
// TC_DCHECK(mmap_handle.ok()) << "Unable to mmap " << filename;
//
// ... use data from addresses
// ... [mmap_handle.start, mmap_handle.start + mmap_handle.num_bytes)
//
// Unmap(mmap_handle);  // Unmap logs errors internally.
//
// Note: one can read *and* write the num_bytes bytes from start, but those
// writes are not propagated to the underlying file, nor to other processes that
// may have mmapped that file (all changes are local to current process).
MmapHandle MmapFile(const std::string &filename);

// Like MmapFile(const std::string &filename), but uses a file descriptor.
MmapHandle MmapFile(int fd);

// Maps a segment of a file to memory. File is given by a file descriptor, and
// offset (relative to the beginning of the file) and size specify the segment
// to be mapped. NOTE: Internally, we align the offset for the call to mmap
// system call to be a multiple of page size, so offset does NOT have to be a
// multiply of the page size.
MmapHandle MmapFile(int fd, int64 segment_offset, int64 segment_size);

// Unmaps a file mapped using MmapFile.  Returns true on success, false
// otherwise.
bool Unmap(MmapHandle mmap_handle);

// Scoped mmapping of a file.  Mmaps a file on construction, unmaps it on
// destruction.
class ScopedMmap {
 public:
  explicit ScopedMmap(const std::string &filename)
      : handle_(MmapFile(filename)) {}

  explicit ScopedMmap(int fd) : handle_(MmapFile(fd)) {}

  ScopedMmap(int fd, int segment_offset, int segment_size)
      : handle_(MmapFile(fd, segment_offset, segment_size)) {}

  ~ScopedMmap() {
    if (handle_.ok()) {
      Unmap(handle_);
    }
  }

  const MmapHandle &handle() { return handle_; }

 private:
  MmapHandle handle_;
};

}  // namespace libtextclassifier2

#endif  // LIBTEXTCLASSIFIER_UTIL_MEMORY_MMAP_H_
