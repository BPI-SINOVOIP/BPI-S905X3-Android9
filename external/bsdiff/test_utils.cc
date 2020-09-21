// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bsdiff/test_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <gtest/gtest.h>

using std::vector;

namespace {

// If |path| is absolute, or explicit relative to the current working directory,
// leaves it as is. Otherwise, if TMPDIR is defined in the environment and is
// non-empty, prepends it to |path|. Otherwise, prepends /tmp.  Returns the
// resulting path.
const std::string PrependTmpdir(const std::string& path) {
  if (path[0] == '/')
    return path;

  const char* tmpdir = getenv("TMPDIR");
  const std::string prefix = (tmpdir && *tmpdir ? tmpdir : "/tmp");
  return prefix + "/" + path;
}

bool MakeTempFile(const std::string& base_filename_template,
                  std::string* filename) {
  const std::string filename_template = PrependTmpdir(base_filename_template);
  vector<char> result(filename_template.size() + 1, '\0');
  memcpy(result.data(), filename_template.data(), filename_template.size());

  int mkstemp_fd = mkstemp(result.data());
  if (mkstemp_fd < 0) {
    PLOG(ERROR) << "mkstemp() Failed";
    return false;
  }
  close(mkstemp_fd);

  if (filename)
    *filename = result.data();
  return true;
}

}  // namespace

namespace test_utils {

void BsdiffTestEnvironment::SetUp() {
#ifdef BSDIFF_TARGET_UNITTEST
#define BSDIFF_TARGET_TMP_BASE "/data/tmp"
      if (access(BSDIFF_TARGET_TMP_BASE, F_OK) == -1) {
        mkdir(BSDIFF_TARGET_TMP_BASE, S_IRWXU | S_IRWXG | S_IROTH | S_IWOTH);
      }
      setenv("TMPDIR", BSDIFF_TARGET_TMP_BASE, 1);
#endif  // defined (BSDIFF_TARGET_UNITTEST)
}

bool ReadFile(const std::string& path, vector<uint8_t>* out) {
  FILE* fp = fopen(path.c_str(), "r");
  if (!fp)
    return false;
  out->clear();

  uint8_t buf[16 * 1024];
  while (true) {
    size_t bytes_read = fread(buf, 1, sizeof(buf), fp);
    if (!bytes_read)
      break;
    out->insert(out->end(), buf, buf + bytes_read);
  }
  bool result = !ferror(fp);
  fclose(fp);
  return result;
}

bool WriteFile(const std::string& path, vector<uint8_t> contents) {
  FILE* fp = fopen(path.c_str(), "r");
  if (!fp)
    return false;
  size_t written = fwrite(contents.data(), 1, contents.size(), fp);
  bool result = written == contents.size() && !ferror(fp);
  fclose(fp);
  return result;
}

ScopedTempFile::ScopedTempFile(const std::string& pattern) {
  EXPECT_TRUE(MakeTempFile(pattern, &filename_));
}

ScopedTempFile::~ScopedTempFile() {
  if (!filename_.empty() && unlink(filename_.c_str()) < 0) {
    PLOG(ERROR) << "Unable to remove temporary file.";
  }
}

bool BsdiffPatchFile::LoadFromFile(const std::string& filename) {
  vector<uint8_t> contents;
  if (!ReadFile(filename, &contents))
    return false;
  file_size = contents.size();
  // Check that the file includes at least the header.
  TEST_AND_RETURN_FALSE(contents.size() >= kHeaderSize);
  magic = std::string(contents.data(), contents.data() + 8);
  memcpy(&ctrl_len, contents.data() + 8, sizeof(ctrl_len));
  memcpy(&diff_len, contents.data() + 16, sizeof(diff_len));
  memcpy(&new_file_len, contents.data() + 24, sizeof(new_file_len));

  // Sanity check before we attempt to parse the bz2 streams.
  TEST_AND_RETURN_FALSE(ctrl_len >= 0);
  TEST_AND_RETURN_FALSE(diff_len >= 0);

  // The cast is safe since ctrl_len and diff_len are both positive.
  TEST_AND_RETURN_FALSE(file_size >=
        static_cast<uint64_t>(kHeaderSize + ctrl_len + diff_len));
  extra_len = file_size - kHeaderSize - ctrl_len - diff_len;

  uint8_t* ptr = contents.data() + kHeaderSize;
  bz2_ctrl = vector<uint8_t>(ptr, ptr + ctrl_len);
  ptr += ctrl_len;
  bz2_diff = vector<uint8_t>(ptr, ptr + diff_len);
  ptr += diff_len;
  bz2_extra = vector<uint8_t>(ptr, ptr + extra_len);

  return true;
}

bool BsdiffPatchFile::IsValid() const {
  TEST_AND_RETURN_FALSE(ctrl_len >= 0);
  TEST_AND_RETURN_FALSE(diff_len >= 0);
  TEST_AND_RETURN_FALSE(new_file_len >= 0);

  // TODO(deymo): Test that the length of the decompressed bz2 streams |diff|
  // plus |extra| are equal to the |new_file_len|.
  // TODO(deymo): Test that all the |bz2_ctrl| triplets (x, y, z) have a "x"
  // and "y" value >= 0 ("z" can be negative).
  return true;
}

}  // namespace test_utils
