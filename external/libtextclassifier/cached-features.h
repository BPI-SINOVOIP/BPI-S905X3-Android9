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

#ifndef LIBTEXTCLASSIFIER_CACHED_FEATURES_H_
#define LIBTEXTCLASSIFIER_CACHED_FEATURES_H_

#include <memory>
#include <vector>

#include "model-executor.h"
#include "model_generated.h"
#include "types.h"

namespace libtextclassifier2 {

// Holds state for extracting features across multiple calls and reusing them.
// Assumes that features for each Token are independent.
class CachedFeatures {
 public:
  static std::unique_ptr<CachedFeatures> Create(
      const TokenSpan& extraction_span,
      std::unique_ptr<std::vector<float>> features,
      std::unique_ptr<std::vector<float>> padding_features,
      const FeatureProcessorOptions* options, int feature_vector_size);

  // Appends the click context features for the given click position to
  // 'output_features'.
  void AppendClickContextFeaturesForClick(
      int click_pos, std::vector<float>* output_features) const;

  // Appends the bounds-sensitive features for the given token span to
  // 'output_features'.
  void AppendBoundsSensitiveFeaturesForSpan(
      TokenSpan selected_span, std::vector<float>* output_features) const;

  // Returns number of features that 'AppendFeaturesForSpan' appends.
  int OutputFeaturesSize() const { return output_features_size_; }

 private:
  CachedFeatures() {}

  // Appends token features to the output. The intended_span specifies which
  // tokens' features should be used in principle. The read_mask_span restricts
  // which tokens are actually read. For tokens outside of the read_mask_span,
  // padding tokens are used instead.
  void AppendFeaturesInternal(const TokenSpan& intended_span,
                              const TokenSpan& read_mask_span,
                              std::vector<float>* output_features) const;

  // Appends features of one padding token to the output.
  void AppendPaddingFeatures(std::vector<float>* output_features) const;

  // Appends the features of tokens from the given span to the output. The
  // features are averaged so that the appended features have the size
  // corresponding to one token.
  void AppendBagFeatures(const TokenSpan& bag_span,
                         std::vector<float>* output_features) const;

  int NumFeaturesPerToken() const;

  TokenSpan extraction_span_;
  const FeatureProcessorOptions* options_;
  int output_features_size_;
  std::unique_ptr<std::vector<float>> features_;
  std::unique_ptr<std::vector<float>> padding_features_;
};

}  // namespace libtextclassifier2

#endif  // LIBTEXTCLASSIFIER_CACHED_FEATURES_H_
