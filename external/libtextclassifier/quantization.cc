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

#include "quantization.h"

#include "util/base/logging.h"

namespace libtextclassifier2 {
namespace {
float DequantizeValue(int num_sparse_features, int quantization_bias,
                      float multiplier, int value) {
  return 1.0 / num_sparse_features * (value - quantization_bias) * multiplier;
}

void DequantizeAdd8bit(const float* scales, const uint8* embeddings,
                       int bytes_per_embedding, const int num_sparse_features,
                       const int bucket_id, float* dest, int dest_size) {
  static const int kQuantizationBias8bit = 128;
  const float multiplier = scales[bucket_id];
  for (int k = 0; k < dest_size; ++k) {
    dest[k] +=
        DequantizeValue(num_sparse_features, kQuantizationBias8bit, multiplier,
                        embeddings[bucket_id * bytes_per_embedding + k]);
  }
}

void DequantizeAddNBit(const float* scales, const uint8* embeddings,
                       int bytes_per_embedding, int num_sparse_features,
                       int quantization_bits, int bucket_id, float* dest,
                       int dest_size) {
  const int quantization_bias = 1 << (quantization_bits - 1);
  const float multiplier = scales[bucket_id];
  for (int i = 0; i < dest_size; ++i) {
    const int bit_offset = i * quantization_bits;
    const int read16_offset = bit_offset / 8;

    uint16 data = embeddings[bucket_id * bytes_per_embedding + read16_offset];
    // If we are not at the end of the embedding row, we can read 2-byte uint16,
    // but if we are, we need to only read uint8.
    if (read16_offset < bytes_per_embedding - 1) {
      data |= embeddings[bucket_id * bytes_per_embedding + read16_offset + 1]
              << 8;
    }
    int value = (data >> (bit_offset % 8)) & ((1 << quantization_bits) - 1);
    dest[i] += DequantizeValue(num_sparse_features, quantization_bias,
                               multiplier, value);
  }
}
}  // namespace

bool CheckQuantizationParams(int bytes_per_embedding, int quantization_bits,
                             int output_embedding_size) {
  if (bytes_per_embedding * 8 / quantization_bits < output_embedding_size) {
    return false;
  }

  return true;
}

bool DequantizeAdd(const float* scales, const uint8* embeddings,
                   int bytes_per_embedding, int num_sparse_features,
                   int quantization_bits, int bucket_id, float* dest,
                   int dest_size) {
  if (quantization_bits == 8) {
    DequantizeAdd8bit(scales, embeddings, bytes_per_embedding,
                      num_sparse_features, bucket_id, dest, dest_size);
  } else if (quantization_bits != 8) {
    DequantizeAddNBit(scales, embeddings, bytes_per_embedding,
                      num_sparse_features, quantization_bits, bucket_id, dest,
                      dest_size);
  } else {
    TC_LOG(ERROR) << "Unsupported quantization_bits: " << quantization_bits;
    return false;
  }

  return true;
}

}  // namespace libtextclassifier2
