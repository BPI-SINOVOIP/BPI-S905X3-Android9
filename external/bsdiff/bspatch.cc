/*-
 * Copyright 2003-2005 Colin Percival
 * All rights reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted providing that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#if 0
__FBSDID("$FreeBSD: src/usr.bin/bsdiff/bspatch/bspatch.c,v 1.1 2005/08/06 01:59:06 cperciva Exp $");
#endif

#include "bsdiff/bspatch.h"

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <memory>
#include <vector>

#include "bsdiff/buffer_file.h"
#include "bsdiff/control_entry.h"
#include "bsdiff/extents.h"
#include "bsdiff/extents_file.h"
#include "bsdiff/file.h"
#include "bsdiff/file_interface.h"
#include "bsdiff/logging.h"
#include "bsdiff/memory_file.h"
#include "bsdiff/patch_reader.h"
#include "bsdiff/sink_file.h"
#include "bsdiff/utils.h"

namespace {
// Read the data in |stream| and write |size| decompressed data to |file|;
// using the buffer in |buf| with size |buf_size|.
// Returns 0 on success, 1 on I/O error and 2 on data error.
int ReadStreamAndWriteAll(
    const std::unique_ptr<bsdiff::FileInterface>& file,
    size_t size,
    uint8_t* buf,
    size_t buf_size,
    const std::function<bool(uint8_t*, size_t)>& read_func) {
  while (size > 0) {
    size_t bytes_to_output = std::min(size, buf_size);
    if (!read_func(buf, bytes_to_output)) {
      LOG(ERROR) << "Failed to read stream.";
      return 2;
    }

    if (!WriteAll(file, buf, bytes_to_output)) {
      PLOG(ERROR) << "WriteAll() failed.";
      return 1;
    }
    size -= bytes_to_output;
  }
  return 0;
}

}  // namespace

namespace bsdiff {

bool ReadAll(const std::unique_ptr<FileInterface>& file,
             uint8_t* data,
             size_t size) {
  size_t offset = 0, read;
  while (offset < size) {
    if (!file->Read(data + offset, size - offset, &read) || read == 0)
      return false;
    offset += read;
  }
  return true;
}

bool WriteAll(const std::unique_ptr<FileInterface>& file,
              const uint8_t* data,
              size_t size) {
  size_t offset = 0, written;
  while (offset < size) {
    if (!file->Write(data + offset, size - offset, &written) || written == 0)
      return false;
    offset += written;
  }
  return true;
}

bool IsOverlapping(const char* old_filename,
                   const char* new_filename,
                   const std::vector<ex_t>& old_extents,
                   const std::vector<ex_t>& new_extents) {
  struct stat old_stat, new_stat;
  if (stat(new_filename, &new_stat) == -1) {
    if (errno == ENOENT)
      return false;
    PLOG(ERROR) << "Error stat the new file: " << new_filename;
    return true;
  }
  if (stat(old_filename, &old_stat) == -1) {
    PLOG(ERROR) << "Error stat the old file: " << old_filename;
    return true;
  }

  if (old_stat.st_dev != new_stat.st_dev || old_stat.st_ino != new_stat.st_ino)
    return false;

  if (old_extents.empty() && new_extents.empty())
    return true;

  for (ex_t old_ex : old_extents)
    for (ex_t new_ex : new_extents)
      if (static_cast<uint64_t>(old_ex.off) < new_ex.off + new_ex.len &&
          static_cast<uint64_t>(new_ex.off) < old_ex.off + old_ex.len)
        return true;

  return false;
}

// Patch |old_filename| with |patch_filename| and save it to |new_filename|.
// |old_extents| and |new_extents| are comma-separated lists of "offset:length"
// extents of |old_filename| and |new_filename|.
// Returns 0 on success, 1 on I/O error and 2 on data error.
int bspatch(const char* old_filename,
            const char* new_filename,
            const char* patch_filename,
            const char* old_extents,
            const char* new_extents) {
  std::unique_ptr<FileInterface> patch_file =
      File::FOpen(patch_filename, O_RDONLY);
  if (!patch_file) {
    PLOG(ERROR) << "Error opening the patch file: " << patch_filename;
    return 1;
  }
  uint64_t patch_size;
  patch_file->GetSize(&patch_size);
  std::vector<uint8_t> patch(patch_size);
  if (!ReadAll(patch_file, patch.data(), patch_size)) {
    PLOG(ERROR) << "Error reading the patch file: " << patch_filename;
    return 1;
  }
  patch_file.reset();

  return bspatch(old_filename, new_filename, patch.data(), patch_size,
                 old_extents, new_extents);
}

// Patch |old_filename| with |patch_data| and save it to |new_filename|.
// |old_extents| and |new_extents| are comma-separated lists of "offset:length"
// extents of |old_filename| and |new_filename|.
// Returns 0 on success, 1 on I/O error and 2 on data error.
int bspatch(const char* old_filename,
            const char* new_filename,
            const uint8_t* patch_data,
            size_t patch_size,
            const char* old_extents,
            const char* new_extents) {
  int using_extents = (old_extents != NULL || new_extents != NULL);

  // Open input file for reading.
  std::unique_ptr<FileInterface> old_file = File::FOpen(old_filename, O_RDONLY);
  if (!old_file) {
    PLOG(ERROR) << "Error opening the old file: " << old_filename;
    return 1;
  }

  std::vector<ex_t> parsed_old_extents;
  if (using_extents) {
    if (!ParseExtentStr(old_extents, &parsed_old_extents)) {
      LOG(ERROR) << "Error parsing the old extents.";
      return 2;
    }
    old_file.reset(new ExtentsFile(std::move(old_file), parsed_old_extents));
  }

  // Open output file for writing.
  std::unique_ptr<FileInterface> new_file =
      File::FOpen(new_filename, O_CREAT | O_WRONLY);
  if (!new_file) {
    PLOG(ERROR) << "Error opening the new file: " << new_filename;
    return 1;
  }

  std::vector<ex_t> parsed_new_extents;
  if (using_extents) {
    if (!ParseExtentStr(new_extents, &parsed_new_extents)) {
      LOG(ERROR) << "Error parsing the new extents.";
      return 2;
    }
    new_file.reset(new ExtentsFile(std::move(new_file), parsed_new_extents));
  }

  if (IsOverlapping(old_filename, new_filename, parsed_old_extents,
                    parsed_new_extents)) {
    // New and old file is overlapping, we can not stream output to new file,
    // cache it in a buffer and write to the file at the end.
    uint64_t newsize = ParseInt64(patch_data + 24);
    new_file.reset(new BufferFile(std::move(new_file), newsize));
  }

  return bspatch(old_file, new_file, patch_data, patch_size);
}

// Patch |old_data| with |patch_data| and save it by calling sink function.
// Returns 0 on success, 1 on I/O error and 2 on data error.
int bspatch(const uint8_t* old_data,
            size_t old_size,
            const uint8_t* patch_data,
            size_t patch_size,
            const sink_func& sink) {
  std::unique_ptr<FileInterface> old_file(new MemoryFile(old_data, old_size));
  std::unique_ptr<FileInterface> new_file(new SinkFile(sink));

  return bspatch(old_file, new_file, patch_data, patch_size);
}

// Patch |old_file| with |patch_data| and save it to |new_file|.
// Returns 0 on success, 1 on I/O error and 2 on data error.
int bspatch(const std::unique_ptr<FileInterface>& old_file,
            const std::unique_ptr<FileInterface>& new_file,
            const uint8_t* patch_data,
            size_t patch_size) {
  BsdiffPatchReader patch_reader;
  if (!patch_reader.Init(patch_data, patch_size)) {
    LOG(ERROR) << "Failed to initialize patch reader.";
    return 2;
  }

  uint64_t old_file_size;
  if (!old_file->GetSize(&old_file_size)) {
    LOG(ERROR) << "Cannot obtain the size of old file.";
    return 1;
  }

  // The oldpos can be negative, but the new pos is only incremented linearly.
  int64_t oldpos = 0;
  uint64_t newpos = 0;
  std::vector<uint8_t> old_buf(1024 * 1024);
  std::vector<uint8_t> new_buf(1024 * 1024);
  uint64_t old_file_pos = 0;
  while (newpos < patch_reader.new_file_size()) {
    ControlEntry control_entry(0, 0, 0);
    if (!patch_reader.ParseControlEntry(&control_entry)) {
      LOG(ERROR) << "Failed to read control stream.";
      return 2;
    }

    // Sanity-check.
    if (newpos + control_entry.diff_size > patch_reader.new_file_size()) {
      LOG(ERROR) << "Corrupt patch.";
      return 2;
    }

    int ret = 0;
    // Add old data to diff string. It is enough to fseek once, at
    // the beginning of the sequence, to avoid unnecessary overhead.
    int64_t seek_offset = oldpos;
    if (seek_offset < 0) {
      // Write diff block directly to new file without adding old data,
      // because we will skip part where |oldpos| < 0.
      ret = ReadStreamAndWriteAll(
          new_file, oldpos - old_file_size, new_buf.data(), new_buf.size(),
          std::bind(&BsdiffPatchReader::ReadDiffStream, &patch_reader,
                    std::placeholders::_1, std::placeholders::_2));
      if (ret)
        return ret;
      seek_offset = 0;
    }

    // We just checked that |seek_offset| is not negative.
    if (static_cast<uint64_t>(seek_offset) != old_file_pos &&
        !old_file->Seek(seek_offset)) {
      PLOG(ERROR) << "Error seeking input file to offset: " << seek_offset;
      return 1;
    }

    old_file_pos =
        std::min<uint64_t>(oldpos + control_entry.diff_size, old_file_size);
    size_t chunk_size = old_file_pos - seek_offset;
    while (chunk_size > 0) {
      size_t read_bytes;
      size_t bytes_to_read = std::min(chunk_size, old_buf.size());
      if (!old_file->Read(old_buf.data(), bytes_to_read, &read_bytes)) {
        PLOG(ERROR) << "Error reading from input file.";
        return 1;
      }
      if (!read_bytes) {
        LOG(ERROR) << "EOF reached while reading from input file.";
        return 2;
      }
      // Read same amount of bytes from diff block
      if (!patch_reader.ReadDiffStream(new_buf.data(), read_bytes)) {
        LOG(ERROR) << "Failed to read diff stream.";
        return 2;
      }
      // new_buf already has data from diff block, adds old data to it.
      for (size_t k = 0; k < read_bytes; k++)
        new_buf[k] += old_buf[k];
      if (!WriteAll(new_file, new_buf.data(), read_bytes)) {
        PLOG(ERROR) << "Error writing to new file.";
        return 1;
      }
      chunk_size -= read_bytes;
    }

    // Adjust pointers.
    newpos += control_entry.diff_size;
    oldpos += control_entry.diff_size;

    if (oldpos > static_cast<int64_t>(old_file_size)) {
      // Write diff block directly to new file without adding old data,
      // because we skipped part where |oldpos| > old_file_size.
      ret = ReadStreamAndWriteAll(
          new_file, oldpos - old_file_size, new_buf.data(), new_buf.size(),
          std::bind(&BsdiffPatchReader::ReadDiffStream, &patch_reader,
                    std::placeholders::_1, std::placeholders::_2));
      if (ret)
        return ret;
    }

    // Sanity-check.
    if (newpos + control_entry.extra_size > patch_reader.new_file_size()) {
      LOG(ERROR) << "Corrupt patch.";
      return 2;
    }

    // Read extra block.
    ret = ReadStreamAndWriteAll(
        new_file, control_entry.extra_size, new_buf.data(), new_buf.size(),
        std::bind(&BsdiffPatchReader::ReadExtraStream, &patch_reader,
                  std::placeholders::_1, std::placeholders::_2));
    if (ret)
      return ret;

    // Adjust pointers.
    newpos += control_entry.extra_size;
    oldpos += control_entry.offset_increment;
  }

  // Close input file.
  old_file->Close();

  if (!patch_reader.Finish()) {
    LOG(ERROR) << "Failed to finish the patch reader.";
    return 2;
  }

  if (!new_file->Close()) {
    PLOG(ERROR) << "Error closing new file.";
    return 1;
  }

  return 0;
}

}  // namespace bsdiff
