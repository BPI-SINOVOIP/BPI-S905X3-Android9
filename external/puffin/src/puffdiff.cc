// Copyright 2017 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "puffin/src/include/puffin/puffdiff.h"

#include <endian.h>
#include <inttypes.h>
#include <unistd.h>

#include <string>
#include <vector>

#include "bsdiff/bsdiff.h"

#include "puffin/src/include/puffin/common.h"
#include "puffin/src/include/puffin/puffer.h"
#include "puffin/src/include/puffin/puffpatch.h"
#include "puffin/src/include/puffin/utils.h"
#include "puffin/src/file_stream.h"
#include "puffin/src/memory_stream.h"
#include "puffin/src/puffin.pb.h"
#include "puffin/src/puffin_stream.h"
#include "puffin/src/set_errors.h"

namespace puffin {

using std::string;
using std::vector;

namespace {

template <typename T>
void CopyVectorToRpf(
    const T& from,
    google::protobuf::RepeatedPtrField<metadata::BitExtent>* to,
    size_t coef) {
  to->Reserve(from.size());
  for (const auto& ext : from) {
    auto tmp = to->Add();
    tmp->set_offset(ext.offset * coef);
    tmp->set_length(ext.length * coef);
  }
}

// Structure of a puffin patch
// +-------+------------------+-------------+--------------+
// |P|U|F|1| PatchHeader Size | PatchHeader | bsdiff_patch |
// +-------+------------------+-------------+--------------+
bool CreatePatch(const Buffer& bsdiff_patch,
                 const vector<BitExtent>& src_deflates,
                 const vector<BitExtent>& dst_deflates,
                 const vector<ByteExtent>& src_puffs,
                 const vector<ByteExtent>& dst_puffs,
                 uint64_t src_puff_size,
                 uint64_t dst_puff_size,
                 Buffer* patch) {
  metadata::PatchHeader header;
  header.set_version(1);

  CopyVectorToRpf(src_deflates, header.mutable_src()->mutable_deflates(), 1);
  CopyVectorToRpf(dst_deflates, header.mutable_dst()->mutable_deflates(), 1);
  CopyVectorToRpf(src_puffs, header.mutable_src()->mutable_puffs(), 8);
  CopyVectorToRpf(dst_puffs, header.mutable_dst()->mutable_puffs(), 8);

  header.mutable_src()->set_puff_length(src_puff_size);
  header.mutable_dst()->set_puff_length(dst_puff_size);

  const uint32_t header_size = header.ByteSize();

  uint64_t offset = 0;
  patch->resize(kMagicLength + sizeof(header_size) + header_size +
                bsdiff_patch.size());

  memcpy(patch->data() + offset, kMagic, kMagicLength);
  offset += kMagicLength;

  // Read header size from big-endian mode.
  uint32_t be_header_size = htobe32(header_size);
  memcpy(patch->data() + offset, &be_header_size, sizeof(be_header_size));
  offset += 4;

  TEST_AND_RETURN_FALSE(
      header.SerializeToArray(patch->data() + offset, header_size));
  offset += header_size;

  memcpy(patch->data() + offset, bsdiff_patch.data(), bsdiff_patch.size());

  if (bsdiff_patch.size() > patch->size()) {
    LOG(ERROR) << "Puffin patch is invalid";
  }
  return true;
}

}  // namespace

bool PuffDiff(UniqueStreamPtr src,
              UniqueStreamPtr dst,
              const vector<BitExtent>& src_deflates,
              const vector<BitExtent>& dst_deflates,
              const string& tmp_filepath,
              Buffer* patch) {
  auto puffer = std::make_shared<Puffer>();
  auto puff_deflate_stream =
      [&puffer](UniqueStreamPtr stream, const vector<BitExtent>& deflates,
                Buffer* puff_buffer, vector<ByteExtent>* puffs) {
        uint64_t puff_size;
        TEST_AND_RETURN_FALSE(stream->Seek(0));
        TEST_AND_RETURN_FALSE(
            FindPuffLocations(stream, deflates, puffs, &puff_size));
        TEST_AND_RETURN_FALSE(stream->Seek(0));
        auto src_puffin_stream = PuffinStream::CreateForPuff(
            std::move(stream), puffer, puff_size, deflates, *puffs);
        puff_buffer->resize(puff_size);
        TEST_AND_RETURN_FALSE(
            src_puffin_stream->Read(puff_buffer->data(), puff_buffer->size()));
        return true;
      };

  Buffer src_puff_buffer;
  Buffer dst_puff_buffer;
  vector<ByteExtent> src_puffs, dst_puffs;
  TEST_AND_RETURN_FALSE(puff_deflate_stream(std::move(src), src_deflates,
                                            &src_puff_buffer, &src_puffs));
  TEST_AND_RETURN_FALSE(puff_deflate_stream(std::move(dst), dst_deflates,
                                            &dst_puff_buffer, &dst_puffs));

  TEST_AND_RETURN_FALSE(
      0 == bsdiff::bsdiff(src_puff_buffer.data(), src_puff_buffer.size(),
                          dst_puff_buffer.data(), dst_puff_buffer.size(),
                          tmp_filepath.c_str(), nullptr));

  auto bsdiff_patch = FileStream::Open(tmp_filepath, true, false);
  TEST_AND_RETURN_FALSE(bsdiff_patch);
  uint64_t patch_size;
  TEST_AND_RETURN_FALSE(bsdiff_patch->GetSize(&patch_size));
  Buffer bsdiff_patch_buf(patch_size);
  TEST_AND_RETURN_FALSE(
      bsdiff_patch->Read(bsdiff_patch_buf.data(), bsdiff_patch_buf.size()));
  TEST_AND_RETURN_FALSE(bsdiff_patch->Close());

  TEST_AND_RETURN_FALSE(CreatePatch(
      bsdiff_patch_buf, src_deflates, dst_deflates, src_puffs, dst_puffs,
      src_puff_buffer.size(), dst_puff_buffer.size(), patch));
  return true;
}

bool PuffDiff(const Buffer& src,
              const Buffer& dst,
              const vector<BitExtent>& src_deflates,
              const vector<BitExtent>& dst_deflates,
              const string& tmp_filepath,
              Buffer* patch) {
  return PuffDiff(MemoryStream::CreateForRead(src),
                  MemoryStream::CreateForRead(dst), src_deflates, dst_deflates,
                  tmp_filepath, patch);
}

}  // namespace puffin
