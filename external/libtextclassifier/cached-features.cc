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

#include "cached-features.h"

#include "tensor-view.h"
#include "util/base/logging.h"

namespace libtextclassifier2 {

namespace {

int CalculateOutputFeaturesSize(const FeatureProcessorOptions* options,
                                int feature_vector_size) {
  const bool bounds_sensitive_enabled =
      options->bounds_sensitive_features() &&
      options->bounds_sensitive_features()->enabled();

  int num_extracted_tokens = 0;
  if (bounds_sensitive_enabled) {
    const FeatureProcessorOptions_::BoundsSensitiveFeatures* config =
        options->bounds_sensitive_features();
    num_extracted_tokens += config->num_tokens_before();
    num_extracted_tokens += config->num_tokens_inside_left();
    num_extracted_tokens += config->num_tokens_inside_right();
    num_extracted_tokens += config->num_tokens_after();
    if (config->include_inside_bag()) {
      ++num_extracted_tokens;
    }
  } else {
    num_extracted_tokens = 2 * options->context_size() + 1;
  }

  int output_features_size = num_extracted_tokens * feature_vector_size;

  if (bounds_sensitive_enabled &&
      options->bounds_sensitive_features()->include_inside_length()) {
    ++output_features_size;
  }

  return output_features_size;
}

}  // namespace

std::unique_ptr<CachedFeatures> CachedFeatures::Create(
    const TokenSpan& extraction_span,
    std::unique_ptr<std::vector<float>> features,
    std::unique_ptr<std::vector<float>> padding_features,
    const FeatureProcessorOptions* options, int feature_vector_size) {
  const int min_feature_version =
      options->bounds_sensitive_features() &&
              options->bounds_sensitive_features()->enabled()
          ? 2
          : 1;
  if (options->feature_version() < min_feature_version) {
    TC_LOG(ERROR) << "Unsupported feature version.";
    return nullptr;
  }

  std::unique_ptr<CachedFeatures> cached_features(new CachedFeatures());
  cached_features->extraction_span_ = extraction_span;
  cached_features->features_ = std::move(features);
  cached_features->padding_features_ = std::move(padding_features);
  cached_features->options_ = options;

  cached_features->output_features_size_ =
      CalculateOutputFeaturesSize(options, feature_vector_size);

  return cached_features;
}

void CachedFeatures::AppendClickContextFeaturesForClick(
    int click_pos, std::vector<float>* output_features) const {
  click_pos -= extraction_span_.first;

  AppendFeaturesInternal(
      /*intended_span=*/ExpandTokenSpan(SingleTokenSpan(click_pos),
                                        options_->context_size(),
                                        options_->context_size()),
      /*read_mask_span=*/{0, TokenSpanSize(extraction_span_)}, output_features);
}

void CachedFeatures::AppendBoundsSensitiveFeaturesForSpan(
    TokenSpan selected_span, std::vector<float>* output_features) const {
  const FeatureProcessorOptions_::BoundsSensitiveFeatures* config =
      options_->bounds_sensitive_features();

  selected_span.first -= extraction_span_.first;
  selected_span.second -= extraction_span_.first;

  // Append the features for tokens around the left bound. Masks out tokens
  // after the right bound, so that if num_tokens_inside_left goes past it,
  // padding tokens will be used.
  AppendFeaturesInternal(
      /*intended_span=*/{selected_span.first - config->num_tokens_before(),
                         selected_span.first +
                             config->num_tokens_inside_left()},
      /*read_mask_span=*/{0, selected_span.second}, output_features);

  // Append the features for tokens around the right bound. Masks out tokens
  // before the left bound, so that if num_tokens_inside_right goes past it,
  // padding tokens will be used.
  AppendFeaturesInternal(
      /*intended_span=*/{selected_span.second -
                             config->num_tokens_inside_right(),
                         selected_span.second + config->num_tokens_after()},
      /*read_mask_span=*/{selected_span.first, TokenSpanSize(extraction_span_)},
      output_features);

  if (config->include_inside_bag()) {
    AppendBagFeatures(selected_span, output_features);
  }

  if (config->include_inside_length()) {
    output_features->push_back(
        static_cast<float>(TokenSpanSize(selected_span)));
  }
}

void CachedFeatures::AppendFeaturesInternal(
    const TokenSpan& intended_span, const TokenSpan& read_mask_span,
    std::vector<float>* output_features) const {
  const TokenSpan copy_span =
      IntersectTokenSpans(intended_span, read_mask_span);
  for (int i = intended_span.first; i < copy_span.first; ++i) {
    AppendPaddingFeatures(output_features);
  }
  output_features->insert(
      output_features->end(),
      features_->begin() + copy_span.first * NumFeaturesPerToken(),
      features_->begin() + copy_span.second * NumFeaturesPerToken());
  for (int i = copy_span.second; i < intended_span.second; ++i) {
    AppendPaddingFeatures(output_features);
  }
}

void CachedFeatures::AppendPaddingFeatures(
    std::vector<float>* output_features) const {
  output_features->insert(output_features->end(), padding_features_->begin(),
                          padding_features_->end());
}

void CachedFeatures::AppendBagFeatures(
    const TokenSpan& bag_span, std::vector<float>* output_features) const {
  const int offset = output_features->size();
  output_features->resize(output_features->size() + NumFeaturesPerToken());
  for (int i = bag_span.first; i < bag_span.second; ++i) {
    for (int j = 0; j < NumFeaturesPerToken(); ++j) {
      (*output_features)[offset + j] +=
          (*features_)[i * NumFeaturesPerToken() + j] / TokenSpanSize(bag_span);
    }
  }
}

int CachedFeatures::NumFeaturesPerToken() const {
  return padding_features_->size();
}

}  // namespace libtextclassifier2
