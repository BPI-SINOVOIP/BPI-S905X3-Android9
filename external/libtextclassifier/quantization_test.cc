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

#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using testing::ElementsAreArray;
using testing::FloatEq;
using testing::Matcher;

namespace libtextclassifier2 {
namespace {

Matcher<std::vector<float>> ElementsAreFloat(const std::vector<float>& values) {
  std::vector<Matcher<float>> matchers;
  for (const float value : values) {
    matchers.push_back(FloatEq(value));
  }
  return ElementsAreArray(matchers);
}

TEST(QuantizationTest, DequantizeAdd8bit) {
  std::vector<float> scales{{0.1, 9.0, -7.0}};
  std::vector<uint8> embeddings{{/*0: */ 0x00, 0xFF, 0x09, 0x00,
                                 /*1: */ 0xFF, 0x09, 0x00, 0xFF,
                                 /*2: */ 0x09, 0x00, 0xFF, 0x09}};

  const int quantization_bits = 8;
  const int bytes_per_embedding = 4;
  const int num_sparse_features = 7;
  {
    const int bucket_id = 0;
    std::vector<float> dest(4, 0.0);
    DequantizeAdd(scales.data(), embeddings.data(), bytes_per_embedding,
                  num_sparse_features, quantization_bits, bucket_id,
                  dest.data(), dest.size());

    EXPECT_THAT(dest,
                ElementsAreFloat(std::vector<float>{
                    // clang-format off
                    {1.0 / 7 * 0.1 * (0x00 - 128),
                     1.0 / 7 * 0.1 * (0xFF - 128),
                     1.0 / 7 * 0.1 * (0x09 - 128),
                     1.0 / 7 * 0.1 * (0x00 - 128)}
                    // clang-format on
                }));
  }

  {
    const int bucket_id = 1;
    std::vector<float> dest(4, 0.0);
    DequantizeAdd(scales.data(), embeddings.data(), bytes_per_embedding,
                  num_sparse_features, quantization_bits, bucket_id,
                  dest.data(), dest.size());

    EXPECT_THAT(dest,
                ElementsAreFloat(std::vector<float>{
                    // clang-format off
                    {1.0 / 7 * 9.0 * (0xFF - 128),
                     1.0 / 7 * 9.0 * (0x09 - 128),
                     1.0 / 7 * 9.0 * (0x00 - 128),
                     1.0 / 7 * 9.0 * (0xFF - 128)}
                    // clang-format on
                }));
  }
}

TEST(QuantizationTest, DequantizeAdd1bitZeros) {
  const int bytes_per_embedding = 4;
  const int num_buckets = 3;
  const int num_sparse_features = 7;
  const int quantization_bits = 1;
  const int bucket_id = 1;

  std::vector<float> scales(num_buckets);
  std::vector<uint8> embeddings(bytes_per_embedding * num_buckets);
  std::fill(scales.begin(), scales.end(), 1);
  std::fill(embeddings.begin(), embeddings.end(), 0);

  std::vector<float> dest(32);
  DequantizeAdd(scales.data(), embeddings.data(), bytes_per_embedding,
                num_sparse_features, quantization_bits, bucket_id, dest.data(),
                dest.size());

  std::vector<float> expected(32);
  std::fill(expected.begin(), expected.end(),
            1.0 / num_sparse_features * (0 - 1));
  EXPECT_THAT(dest, ElementsAreFloat(expected));
}

TEST(QuantizationTest, DequantizeAdd1bitOnes) {
  const int bytes_per_embedding = 4;
  const int num_buckets = 3;
  const int num_sparse_features = 7;
  const int quantization_bits = 1;
  const int bucket_id = 1;

  std::vector<float> scales(num_buckets, 1.0);
  std::vector<uint8> embeddings(bytes_per_embedding * num_buckets, 0xFF);

  std::vector<float> dest(32);
  DequantizeAdd(scales.data(), embeddings.data(), bytes_per_embedding,
                num_sparse_features, quantization_bits, bucket_id, dest.data(),
                dest.size());
  std::vector<float> expected(32);
  std::fill(expected.begin(), expected.end(),
            1.0 / num_sparse_features * (1 - 1));
  EXPECT_THAT(dest, ElementsAreFloat(expected));
}

TEST(QuantizationTest, DequantizeAdd3bit) {
  const int bytes_per_embedding = 4;
  const int num_buckets = 3;
  const int num_sparse_features = 7;
  const int quantization_bits = 3;
  const int bucket_id = 1;

  std::vector<float> scales(num_buckets, 1.0);
  scales[1] = 9.0;
  std::vector<uint8> embeddings(bytes_per_embedding * num_buckets, 0);
  // For bucket_id=1, the embedding has values 0..9 for indices 0..9:
  embeddings[4] = (1 << 7) | (1 << 6) | (1 << 4) | 1;
  embeddings[5] = (1 << 6) | (1 << 4) | (1 << 3);
  embeddings[6] = (1 << 4) | (1 << 3) | (1 << 2) | (1 << 1) | 1;

  std::vector<float> dest(10);
  DequantizeAdd(scales.data(), embeddings.data(), bytes_per_embedding,
                num_sparse_features, quantization_bits, bucket_id, dest.data(),
                dest.size());

  std::vector<float> expected;
  expected.push_back(1.0 / num_sparse_features * (1 - 4) * scales[bucket_id]);
  expected.push_back(1.0 / num_sparse_features * (2 - 4) * scales[bucket_id]);
  expected.push_back(1.0 / num_sparse_features * (3 - 4) * scales[bucket_id]);
  expected.push_back(1.0 / num_sparse_features * (4 - 4) * scales[bucket_id]);
  expected.push_back(1.0 / num_sparse_features * (5 - 4) * scales[bucket_id]);
  expected.push_back(1.0 / num_sparse_features * (6 - 4) * scales[bucket_id]);
  expected.push_back(1.0 / num_sparse_features * (7 - 4) * scales[bucket_id]);
  expected.push_back(1.0 / num_sparse_features * (0 - 4) * scales[bucket_id]);
  expected.push_back(1.0 / num_sparse_features * (0 - 4) * scales[bucket_id]);
  expected.push_back(1.0 / num_sparse_features * (0 - 4) * scales[bucket_id]);
  EXPECT_THAT(dest, ElementsAreFloat(expected));
}

}  // namespace
}  // namespace libtextclassifier2
