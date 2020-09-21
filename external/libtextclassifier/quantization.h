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

#ifndef LIBTEXTCLASSIFIER_QUANTIZATION_H_
#define LIBTEXTCLASSIFIER_QUANTIZATION_H_

#include "util/base/integral_types.h"

namespace libtextclassifier2 {

// Returns true if the quantization parameters are valid.
bool CheckQuantizationParams(int bytes_per_embedding, int quantization_bits,
                             int output_embedding_size);

// Dequantizes embeddings (quantized to 1 to 8 bits) into the floats they
// represent. The algorithm proceeds by reading 2-byte words from the embedding
// storage to handle well the cases when the quantized value crosses the byte-
// boundary.
bool DequantizeAdd(const float* scales, const uint8* embeddings,
                   int bytes_per_embedding, int num_sparse_features,
                   int quantization_bits, int bucket_id, float* dest,
                   int dest_size);

}  // namespace libtextclassifier2

#endif  // LIBTEXTCLASSIFIER_QUANTIZATION_H_
