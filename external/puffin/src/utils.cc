// Copyright 2017 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "puffin/src/include/puffin/utils.h"

#include <inttypes.h>

#include <string>
#include <vector>

#include <zlib.h>

#include "puffin/src/bit_reader.h"
#include "puffin/src/file_stream.h"
#include "puffin/src/include/puffin/common.h"
#include "puffin/src/include/puffin/errors.h"
#include "puffin/src/include/puffin/puffer.h"
#include "puffin/src/memory_stream.h"
#include "puffin/src/puff_writer.h"
#include "puffin/src/set_errors.h"

namespace {
// Use memcpy to access the unaligned data of type |T|.
template <typename T>
inline T get_unaligned(const void* address) {
  T result;
  memcpy(&result, address, sizeof(T));
  return result;
}

// Calculate both the compressed size and uncompressed size of the deflate
// block that starts from the offset |start| of buffer |data|.
bool CalculateSizeOfDeflateBlock(const puffin::Buffer& data,
                                 uint64_t start,
                                 uint64_t* compressed_size,
                                 uint64_t* uncompressed_size) {
  TEST_AND_RETURN_FALSE(compressed_size != nullptr &&
                        uncompressed_size != nullptr);

  TEST_AND_RETURN_FALSE(start < data.size());

  z_stream strm = {};
  strm.avail_in = data.size() - start;
  strm.next_in = data.data() + start;

  // -15 means we are decoding a 'raw' stream without zlib headers.
  if (inflateInit2(&strm, -15)) {
    LOG(ERROR) << "Failed to initialize inflate: " << strm.msg;
    return false;
  }

  const unsigned int kBufferSize = 32768;
  std::vector<uint8_t> uncompressed_data(kBufferSize);
  *uncompressed_size = 0;
  int status = Z_OK;
  do {
    // Overwrite the same buffer since we don't need the uncompressed data.
    strm.avail_out = kBufferSize;
    strm.next_out = uncompressed_data.data();
    status = inflate(&strm, Z_NO_FLUSH);
    if (status < 0) {
      LOG(ERROR) << "Inflate failed: " << strm.msg << ", has decompressed "
                 << *uncompressed_size << " bytes.";
      return false;
    }
    *uncompressed_size += kBufferSize - strm.avail_out;
  } while (status != Z_STREAM_END);

  *compressed_size = data.size() - start - strm.avail_in;
  TEST_AND_RETURN_FALSE(inflateEnd(&strm) == Z_OK);
  return true;
}

}  // namespace

namespace puffin {

using std::string;
using std::vector;

uint64_t BytesInByteExtents(const vector<ByteExtent>& extents) {
  uint64_t bytes = 0;
  for (const auto& extent : extents) {
    bytes += extent.length;
  }
  return bytes;
}

// This function uses RFC1950 (https://www.ietf.org/rfc/rfc1950.txt) for the
// definition of a zlib stream.  For finding the deflate blocks, we relying on
// the proper size of the zlib stream in |data|. Basically the size of the zlib
// stream should be known before hand. Otherwise we need to parse the stream and
// find the location of compressed blocks using CalculateSizeOfDeflateBlock().
bool LocateDeflatesInZlib(const Buffer& data,
                          std::vector<ByteExtent>* deflate_blocks) {
  // A zlib stream has the following format:
  // 0           1     compression method and flag
  // 1           1     flag
  // 2           4     preset dictionary (optional)
  // 2 or 6      n     compressed data
  // n+(2 or 6)  4     Adler-32 checksum
  TEST_AND_RETURN_FALSE(data.size() >= 6 + 4);  // Header + Footer
  uint16_t cmf = data[0];
  auto compression_method = cmf & 0x0F;
  // For deflate compression_method should be 8.
  TEST_AND_RETURN_FALSE(compression_method == 8);

  auto cinfo = (cmf & 0xF0) >> 4;
  // Value greater than 7 is not allowed in deflate.
  TEST_AND_RETURN_FALSE(cinfo <= 7);

  auto flag = data[1];
  TEST_AND_RETURN_FALSE(((cmf << 8) + flag) % 31 == 0);

  uint64_t header_len = 2;
  if (flag & 0x20) {
    header_len += 4;  // 4 bytes for the preset dictionary.
  }

  // 4 is for ADLER32.
  deflate_blocks->emplace_back(header_len, data.size() - header_len - 4);
  return true;
}

bool FindDeflateSubBlocks(const UniqueStreamPtr& src,
                          const vector<ByteExtent>& deflates,
                          vector<BitExtent>* subblock_deflates) {
  Puffer puffer;
  Buffer deflate_buffer;
  for (const auto& deflate : deflates) {
    TEST_AND_RETURN_FALSE(src->Seek(deflate.offset));
    // Read from src into deflate_buffer.
    deflate_buffer.resize(deflate.length);
    TEST_AND_RETURN_FALSE(src->Read(deflate_buffer.data(), deflate.length));

    // Find all the subblocks.
    BufferBitReader bit_reader(deflate_buffer.data(), deflate.length);
    BufferPuffWriter puff_writer(nullptr, 0);
    Error error;
    vector<BitExtent> subblocks;
    TEST_AND_RETURN_FALSE(
        puffer.PuffDeflate(&bit_reader, &puff_writer, &subblocks, &error));
    TEST_AND_RETURN_FALSE(deflate.length == bit_reader.Offset());
    for (const auto& subblock : subblocks) {
      subblock_deflates->emplace_back(subblock.offset + deflate.offset * 8,
                                      subblock.length);
    }
  }
  return true;
}

bool LocateDeflatesInZlibBlocks(const string& file_path,
                                const vector<ByteExtent>& zlibs,
                                vector<BitExtent>* deflates) {
  auto src = FileStream::Open(file_path, true, false);
  TEST_AND_RETURN_FALSE(src);

  Buffer buffer;
  for (auto& zlib : zlibs) {
    buffer.resize(zlib.length);
    TEST_AND_RETURN_FALSE(src->Seek(zlib.offset));
    TEST_AND_RETURN_FALSE(src->Read(buffer.data(), buffer.size()));

    vector<ByteExtent> deflate_blocks;
    TEST_AND_RETURN_FALSE(LocateDeflatesInZlib(buffer, &deflate_blocks));

    vector<BitExtent> deflate_subblocks;
    auto zlib_blc_src = MemoryStream::CreateForRead(buffer);
    TEST_AND_RETURN_FALSE(
        FindDeflateSubBlocks(zlib_blc_src, deflate_blocks, &deflate_subblocks));

    // Relocated based on the offset of the zlib.
    for (const auto& def : deflate_subblocks) {
      deflates->emplace_back(zlib.offset * 8 + def.offset, def.length);
    }
  }
  return true;
}

// For more information about gzip format, refer to RFC 1952 located at:
// https://www.ietf.org/rfc/rfc1952.txt
bool LocateDeflatesInGzip(const Buffer& data,
                          vector<ByteExtent>* deflate_blocks) {
  uint64_t member_start = 0;
  while (member_start < data.size()) {
    // Each member entry has the following format
    // 0      1     0x1F
    // 1      1     0x8B
    // 2      1     compression method (8 denotes deflate)
    // 3      1     set of flags
    // 4      4     modification time
    // 8      1     extra flags
    // 9      1     operating system
    TEST_AND_RETURN_FALSE(member_start + 10 <= data.size());
    TEST_AND_RETURN_FALSE(data[member_start + 0] == 0x1F);
    TEST_AND_RETURN_FALSE(data[member_start + 1] == 0x8B);
    TEST_AND_RETURN_FALSE(data[member_start + 2] == 8);

    uint64_t offset = member_start + 10;
    int flag = data[member_start + 3];
    // Extra field
    if (flag & 4) {
      TEST_AND_RETURN_FALSE(offset + 2 <= data.size());
      uint16_t extra_length = data[offset++];
      extra_length |= static_cast<uint16_t>(data[offset++]) << 8;
      TEST_AND_RETURN_FALSE(offset + extra_length <= data.size());
      offset += extra_length;
    }
    // File name field
    if (flag & 8) {
      while (true) {
        TEST_AND_RETURN_FALSE(offset + 1 <= data.size());
        if (data[offset++] == 0) {
          break;
        }
      }
    }
    // File comment field
    if (flag & 16) {
      while (true) {
        TEST_AND_RETURN_FALSE(offset + 1 <= data.size());
        if (data[offset++] == 0) {
          break;
        }
      }
    }
    // CRC16 field
    if (flag & 2) {
      offset += 2;
    }

    uint64_t compressed_size, uncompressed_size;
    TEST_AND_RETURN_FALSE(CalculateSizeOfDeflateBlock(
        data, offset, &compressed_size, &uncompressed_size));
    TEST_AND_RETURN_FALSE(offset + compressed_size <= data.size());
    deflate_blocks->push_back(ByteExtent(offset, compressed_size));
    offset += compressed_size;

    // Ignore CRC32;
    TEST_AND_RETURN_FALSE(offset + 8 <= data.size());
    offset += 4;
    uint32_t u_size = 0;
    for (size_t i = 0; i < 4; i++) {
      u_size |= static_cast<uint32_t>(data[offset++]) << (i * 8);
    }
    TEST_AND_RETURN_FALSE(uncompressed_size % (1 << 31) == u_size);
    member_start = offset;
  }
  return true;
}

// For more information about the zip format, refer to
// https://support.pkware.com/display/PKZIP/APPNOTE
bool LocateDeflatesInZipArchive(const Buffer& data,
                                vector<ByteExtent>* deflate_blocks) {
  uint64_t pos = 0;
  while (pos <= data.size() - 30) {
    // TODO(xunchang) add support for big endian system when searching for
    // magic numbers.
    if (get_unaligned<uint32_t>(data.data() + pos) != 0x04034b50) {
      pos++;
      continue;
    }

    // local file header format
    // 0      4     0x04034b50
    // 4      2     minimum version needed to extract
    // 6      2     general purpose bit flag
    // 8      2     compression method
    // 10     4     file last modification date & time
    // 14     4     CRC-32
    // 18     4     compressed size
    // 22     4     uncompressed size
    // 26     2     file name length
    // 28     2     extra field length
    // 30     n     file name
    // 30+n   m     extra field
    auto compression_method = get_unaligned<uint16_t>(data.data() + pos + 8);
    if (compression_method != 8) {  // non-deflate type
      pos += 4;
      continue;
    }

    auto compressed_size = get_unaligned<uint32_t>(data.data() + pos + 18);
    auto uncompressed_size = get_unaligned<uint32_t>(data.data() + pos + 22);
    auto file_name_length = get_unaligned<uint16_t>(data.data() + pos + 26);
    auto extra_field_length = get_unaligned<uint16_t>(data.data() + pos + 28);
    uint64_t header_size = 30 + file_name_length + extra_field_length;

    // sanity check
    if (static_cast<uint64_t>(header_size) + compressed_size > data.size() ||
        pos > data.size() - header_size - compressed_size) {
      pos += 4;
      continue;
    }

    uint64_t calculated_compressed_size;
    uint64_t calculated_uncompressed_size;
    if (!CalculateSizeOfDeflateBlock(data, pos + header_size,
                                     &calculated_compressed_size,
                                     &calculated_uncompressed_size)) {
      LOG(ERROR) << "Failed to decompress the zip entry starting from: " << pos
                 << ", skip adding deflates for this entry.";
      pos += 4;
      continue;
    }

    // Double check the compressed size and uncompressed size if they are
    // available in the file header.
    if (compressed_size > 0 && compressed_size != calculated_compressed_size) {
      LOG(WARNING) << "Compressed size in the file header: " << compressed_size
                   << " doesn't equal the real size: "
                   << calculated_compressed_size;
    }

    if (uncompressed_size > 0 &&
        uncompressed_size != calculated_uncompressed_size) {
      LOG(WARNING) << "Uncompressed size in the file header: "
                   << uncompressed_size << " doesn't equal the real size: "
                   << calculated_uncompressed_size;
    }

    deflate_blocks->emplace_back(pos + header_size, calculated_compressed_size);
    pos += header_size + calculated_compressed_size;
  }

  return true;
}

bool LocateDeflateSubBlocksInZipArchive(const Buffer& data,
                                        vector<BitExtent>* deflates) {
  vector<ByteExtent> deflate_blocks;
  if (!LocateDeflatesInZipArchive(data, &deflate_blocks)) {
    return false;
  }

  auto src = MemoryStream::CreateForRead(data);
  return FindDeflateSubBlocks(src, deflate_blocks, deflates);
}

bool FindPuffLocations(const UniqueStreamPtr& src,
                       const vector<BitExtent>& deflates,
                       vector<ByteExtent>* puffs,
                       uint64_t* out_puff_size) {
  Puffer puffer;
  Buffer deflate_buffer;

  // Here accumulate the size difference between each corresponding deflate and
  // puff. At the end we add this cummulative size difference to the size of the
  // deflate stream to get the size of the puff stream. We use signed size
  // because puff size could be smaller than deflate size.
  int64_t total_size_difference = 0;
  for (auto deflate = deflates.begin(); deflate != deflates.end(); ++deflate) {
    // Read from src into deflate_buffer.
    auto start_byte = deflate->offset / 8;
    auto end_byte = (deflate->offset + deflate->length + 7) / 8;
    deflate_buffer.resize(end_byte - start_byte);
    TEST_AND_RETURN_FALSE(src->Seek(start_byte));
    TEST_AND_RETURN_FALSE(
        src->Read(deflate_buffer.data(), deflate_buffer.size()));
    // Find the size of the puff.
    BufferBitReader bit_reader(deflate_buffer.data(), deflate_buffer.size());
    uint64_t bits_to_skip = deflate->offset % 8;
    TEST_AND_RETURN_FALSE(bit_reader.CacheBits(bits_to_skip));
    bit_reader.DropBits(bits_to_skip);

    BufferPuffWriter puff_writer(nullptr, 0);
    Error error;
    TEST_AND_RETURN_FALSE(
        puffer.PuffDeflate(&bit_reader, &puff_writer, nullptr, &error));
    TEST_AND_RETURN_FALSE(deflate_buffer.size() == bit_reader.Offset());

    // 1 if a deflate ends at the same byte that the next deflate starts and
    // there is a few bits gap between them. In practice this may never happen,
    // but it is a good idea to support it anyways. If there is a gap, the value
    // of the gap will be saved as an integer byte to the puff stream. The parts
    // of the byte that belogs to the deflates are shifted out.
    int gap = 0;
    if (deflate != deflates.begin()) {
      auto prev_deflate = std::prev(deflate);
      if ((prev_deflate->offset + prev_deflate->length == deflate->offset)
          // If deflates are on byte boundary the gap will not be counted later,
          // so we won't worry about it.
          && (deflate->offset % 8 != 0)) {
        gap = 1;
      }
    }

    start_byte = ((deflate->offset + 7) / 8);
    end_byte = (deflate->offset + deflate->length) / 8;
    int64_t deflate_length_in_bytes = end_byte - start_byte;

    // If there was no gap bits between the current and previous deflates, there
    // will be no extra gap byte, so the offset will be shifted one byte back.
    auto puff_offset = start_byte - gap + total_size_difference;
    auto puff_size = puff_writer.Size();
    // Add the location into puff.
    puffs->emplace_back(puff_offset, puff_size);
    total_size_difference +=
        static_cast<int64_t>(puff_size) - deflate_length_in_bytes - gap;
  }

  uint64_t src_size;
  TEST_AND_RETURN_FALSE(src->GetSize(&src_size));
  auto final_size = static_cast<int64_t>(src_size) + total_size_difference;
  TEST_AND_RETURN_FALSE(final_size >= 0);
  *out_puff_size = final_size;
  return true;
}

}  // namespace puffin
