// Copyright 2017 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>
#include <vector>

#include "gtest/gtest.h"

#include "puffin/src/include/puffin/common.h"
#include "puffin/src/include/puffin/puffdiff.h"
#include "puffin/src/include/puffin/puffpatch.h"
#include "puffin/src/include/puffin/utils.h"
#include "puffin/src/memory_stream.h"
#include "puffin/src/puffin_stream.h"
#include "puffin/src/sample_generator.h"
#include "puffin/src/set_errors.h"
#include "puffin/src/unittest_common.h"

#define PRINT_SAMPLE 0  // Set to 1 if you want to print the generated samples.

namespace puffin {

using std::vector;
using std::string;

void TestPatching(const Buffer& src_buf,
                  const Buffer& dst_buf,
                  const vector<BitExtent>& src_deflates,
                  const vector<BitExtent>& dst_deflates,
                  const Buffer patch) {
  Buffer patch_out;
  string patch_path;
  ASSERT_TRUE(MakeTempFile(&patch_path, nullptr));
  ScopedPathUnlinker scoped_unlinker(patch_path);
  ASSERT_TRUE(PuffDiff(src_buf, dst_buf, src_deflates, dst_deflates, patch_path,
                       &patch_out));

#if PRINT_SAMPLE
  sample_generator::PrintArray("kPatchXXXXX", patch_out);
#endif

  EXPECT_EQ(patch_out, patch);

  auto src_stream = MemoryStream::CreateForRead(src_buf);
  Buffer dst_buf_out;
  auto dst_stream = MemoryStream::CreateForWrite(&dst_buf_out);
  ASSERT_TRUE(PuffPatch(std::move(src_stream), std::move(dst_stream),
                        patch.data(), patch.size()));
  EXPECT_EQ(dst_buf_out, dst_buf);
}

TEST(PatchingTest, Patching8To9Test) {
  TestPatching(kDeflates8, kDeflates9, kSubblockDeflateExtents8,
               kSubblockDeflateExtents9, kPatch8To9);
}

TEST(PatchingTest, Patching9To8Test) {
  TestPatching(kDeflates9, kDeflates8, kSubblockDeflateExtents9,
               kSubblockDeflateExtents8, kPatch9To8);
}

TEST(PatchingTest, Patching8ToEmptyTest) {
  TestPatching(kDeflates8, {}, kSubblockDeflateExtents8, {}, kPatch8ToEmpty);
}

TEST(PatchingTest, Patching8ToNoDeflateTest) {
  TestPatching(kDeflates8, {11, 22, 33, 44}, kSubblockDeflateExtents8, {},
               kPatch8ToNoDeflate);
}

// TODO(ahassani): add tests for:
//   TestPatchingEmptyTo9
//   TestPatchingNoDeflateTo9

// TODO(ahassani): Change tests data if you decided to compress the header of
// the patch.

}  // namespace puffin
