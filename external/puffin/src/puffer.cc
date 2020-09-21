// Copyright 2017 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "puffin/src/include/puffin/puffer.h"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "puffin/src/bit_reader.h"
#include "puffin/src/huffman_table.h"
#include "puffin/src/include/puffin/common.h"
#include "puffin/src/include/puffin/stream.h"
#include "puffin/src/puff_data.h"
#include "puffin/src/puff_writer.h"
#include "puffin/src/set_errors.h"

namespace puffin {

using std::vector;
using std::string;

Puffer::Puffer() : dyn_ht_(new HuffmanTable()), fix_ht_(new HuffmanTable()) {}

Puffer::~Puffer() {}

bool Puffer::PuffDeflate(BitReaderInterface* br,
                         PuffWriterInterface* pw,
                         vector<BitExtent>* deflates,
                         Error* error) const {
  *error = Error::kSuccess;
  PuffData pd;
  HuffmanTable* cur_ht;
  // No bits left to read, return. We try to cache at least eight bits because
  // the minimum length of a deflate bit stream is 8: (fixed huffman table) 3
  // bits header + 5 bits just one len/dist symbol.
  while (br->CacheBits(8)) {
    auto start_bit_offset = br->OffsetInBits();

    TEST_AND_RETURN_FALSE_SET_ERROR(br->CacheBits(3),
                                    Error::kInsufficientInput);
    uint8_t final_bit = br->ReadBits(1);  // BFINAL
    br->DropBits(1);
    uint8_t type = br->ReadBits(2);  // BTYPE
    br->DropBits(2);
    DVLOG(2) << "Read block type: "
             << BlockTypeToString(static_cast<BlockType>(type));

    // Header structure
    // +-+-+-+-+-+-+-+-+
    // |F| TP|   SKIP  |
    // +-+-+-+-+-+-+-+-+
    // F -> final_bit
    // TP -> type
    // SKIP -> skipped_bits (only in kUncompressed type)
    auto block_header = (final_bit << 7) | (type << 5);
    switch (static_cast<BlockType>(type)) {
      case BlockType::kUncompressed: {
        auto skipped_bits = br->ReadBoundaryBits();
        br->SkipBoundaryBits();
        TEST_AND_RETURN_FALSE_SET_ERROR(br->CacheBits(32),
                                        Error::kInsufficientInput);
        auto len = br->ReadBits(16);  // LEN
        br->DropBits(16);
        auto nlen = br->ReadBits(16);  // NLEN
        br->DropBits(16);

        if ((len ^ nlen) != 0xFFFF) {
          LOG(ERROR) << "Length of uncompressed data is invalid;"
                     << " LEN(" << len << ") NLEN(" << nlen << ")";
          *error = Error::kInvalidInput;
          return false;
        }

        // Put skipped bits into header.
        block_header |= skipped_bits;

        // Insert block header.
        pd.type = PuffData::Type::kBlockMetadata;
        pd.block_metadata[0] = block_header;
        pd.length = 1;
        TEST_AND_RETURN_FALSE(pw->Insert(pd, error));

        // Insert all the raw literals.
        pd.type = PuffData::Type::kLiterals;
        pd.length = len;
        TEST_AND_RETURN_FALSE_SET_ERROR(
            br->GetByteReaderFn(pd.length, &pd.read_fn),
            Error::kInsufficientInput);
        TEST_AND_RETURN_FALSE(pw->Insert(pd, error));

        pd.type = PuffData::Type::kEndOfBlock;
        TEST_AND_RETURN_FALSE(pw->Insert(pd, error));

        if (deflates != nullptr) {
          deflates->emplace_back(start_bit_offset,
                                 br->OffsetInBits() - start_bit_offset);
        }

        // continue the loop. Do not read any literal/length/distance.
        continue;
      }

      case BlockType::kFixed:
        fix_ht_->BuildFixedHuffmanTable();
        cur_ht = fix_ht_.get();
        pd.type = PuffData::Type::kBlockMetadata;
        pd.block_metadata[0] = block_header;
        pd.length = 1;
        TEST_AND_RETURN_FALSE(pw->Insert(pd, error));
        break;

      case BlockType::kDynamic:
        pd.type = PuffData::Type::kBlockMetadata;
        pd.block_metadata[0] = block_header;
        pd.length = sizeof(pd.block_metadata) - 1;
        TEST_AND_RETURN_FALSE(dyn_ht_->BuildDynamicHuffmanTable(
            br, &pd.block_metadata[1], &pd.length, error));
        pd.length += 1;  // For the header.
        TEST_AND_RETURN_FALSE(pw->Insert(pd, error));
        cur_ht = dyn_ht_.get();
        break;

      default:
        LOG(ERROR) << "Invalid block compression type: "
                   << static_cast<int>(type);
        *error = Error::kInvalidInput;
        return false;
    }

    while (true) {  // Breaks when the end of block is reached.
      auto max_bits = cur_ht->LitLenMaxBits();
      if (!br->CacheBits(max_bits)) {
        // It could be the end of buffer and the bit length of the end_of_block
        // symbol has less than maximum bit length of current Huffman table. So
        // only asking for the size of end of block symbol (256).
        TEST_AND_RETURN_FALSE_SET_ERROR(cur_ht->EndOfBlockBitLength(&max_bits),
                                        Error::kInvalidInput);
      }
      TEST_AND_RETURN_FALSE_SET_ERROR(br->CacheBits(max_bits),
                                      Error::kInsufficientInput);
      auto bits = br->ReadBits(max_bits);
      uint16_t lit_len_alphabet;
      size_t nbits;
      TEST_AND_RETURN_FALSE_SET_ERROR(
          cur_ht->LitLenAlphabet(bits, &lit_len_alphabet, &nbits),
          Error::kInvalidInput);
      br->DropBits(nbits);
      if (lit_len_alphabet < 256) {
        pd.type = PuffData::Type::kLiteral;
        pd.byte = lit_len_alphabet;
        TEST_AND_RETURN_FALSE(pw->Insert(pd, error));

      } else if (256 == lit_len_alphabet) {
        pd.type = PuffData::Type::kEndOfBlock;
        TEST_AND_RETURN_FALSE(pw->Insert(pd, error));
        if (deflates != nullptr) {
          deflates->emplace_back(start_bit_offset,
                                 br->OffsetInBits() - start_bit_offset);
        }
        break;  // Breaks the loop.
      } else {
        TEST_AND_RETURN_FALSE_SET_ERROR(lit_len_alphabet <= 285,
                                        Error::kInvalidInput);
        // Reading length.
        auto len_code_start = lit_len_alphabet - 257;
        auto extra_bits_len = kLengthExtraBits[len_code_start];
        uint16_t extra_bits_value = 0;
        if (extra_bits_len) {
          TEST_AND_RETURN_FALSE_SET_ERROR(br->CacheBits(extra_bits_len),
                                          Error::kInsufficientInput);
          extra_bits_value = br->ReadBits(extra_bits_len);
          br->DropBits(extra_bits_len);
        }
        auto length = kLengthBases[len_code_start] + extra_bits_value;

        TEST_AND_RETURN_FALSE_SET_ERROR(
            br->CacheBits(cur_ht->DistanceMaxBits()),
            Error::kInsufficientInput);
        auto bits = br->ReadBits(cur_ht->DistanceMaxBits());
        uint16_t distance_alphabet;
        size_t nbits;
        TEST_AND_RETURN_FALSE_SET_ERROR(
            cur_ht->DistanceAlphabet(bits, &distance_alphabet, &nbits),
            Error::kInvalidInput);
        br->DropBits(nbits);

        // Reading distance.
        extra_bits_len = kDistanceExtraBits[distance_alphabet];
        extra_bits_value = 0;
        if (extra_bits_len) {
          TEST_AND_RETURN_FALSE_SET_ERROR(br->CacheBits(extra_bits_len),
                                          Error::kInsufficientInput);
          extra_bits_value = br->ReadBits(extra_bits_len);
          br->DropBits(extra_bits_len);
        }

        pd.type = PuffData::Type::kLenDist;
        pd.length = length;
        pd.distance = kDistanceBases[distance_alphabet] + extra_bits_value;
        TEST_AND_RETURN_FALSE(pw->Insert(pd, error));
      }
    }
  }
  TEST_AND_RETURN_FALSE(pw->Flush(error));
  return true;
}

}  // namespace puffin
