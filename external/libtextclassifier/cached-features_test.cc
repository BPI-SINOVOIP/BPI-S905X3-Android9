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

#include "model-executor.h"
#include "tensor-view.h"

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

std::unique_ptr<std::vector<float>> MakeFeatures(int num_tokens) {
  std::unique_ptr<std::vector<float>> features(new std::vector<float>());
  for (int i = 1; i <= num_tokens; ++i) {
    features->push_back(i * 11.0f);
    features->push_back(-i * 11.0f);
    features->push_back(i * 0.1f);
  }
  return features;
}

std::vector<float> GetCachedClickContextFeatures(
    const CachedFeatures& cached_features, int click_pos) {
  std::vector<float> output_features;
  cached_features.AppendClickContextFeaturesForClick(click_pos,
                                                     &output_features);
  return output_features;
}

std::vector<float> GetCachedBoundsSensitiveFeatures(
    const CachedFeatures& cached_features, TokenSpan selected_span) {
  std::vector<float> output_features;
  cached_features.AppendBoundsSensitiveFeaturesForSpan(selected_span,
                                                       &output_features);
  return output_features;
}

TEST(CachedFeaturesTest, ClickContext) {
  FeatureProcessorOptionsT options;
  options.context_size = 2;
  options.feature_version = 1;
  flatbuffers::FlatBufferBuilder builder;
  builder.Finish(CreateFeatureProcessorOptions(builder, &options));
  flatbuffers::DetachedBuffer options_fb = builder.Release();

  std::unique_ptr<std::vector<float>> features = MakeFeatures(9);
  std::unique_ptr<std::vector<float>> padding_features(
      new std::vector<float>{112233.0, -112233.0, 321.0});

  const std::unique_ptr<CachedFeatures> cached_features =
      CachedFeatures::Create(
          {3, 10}, std::move(features), std::move(padding_features),
          flatbuffers::GetRoot<FeatureProcessorOptions>(options_fb.data()),
          /*feature_vector_size=*/3);
  ASSERT_TRUE(cached_features);

  EXPECT_THAT(GetCachedClickContextFeatures(*cached_features, 5),
              ElementsAreFloat({11.0, -11.0, 0.1, 22.0, -22.0, 0.2, 33.0, -33.0,
                                0.3, 44.0, -44.0, 0.4, 55.0, -55.0, 0.5}));

  EXPECT_THAT(GetCachedClickContextFeatures(*cached_features, 6),
              ElementsAreFloat({22.0, -22.0, 0.2, 33.0, -33.0, 0.3, 44.0, -44.0,
                                0.4, 55.0, -55.0, 0.5, 66.0, -66.0, 0.6}));

  EXPECT_THAT(GetCachedClickContextFeatures(*cached_features, 7),
              ElementsAreFloat({33.0, -33.0, 0.3, 44.0, -44.0, 0.4, 55.0, -55.0,
                                0.5, 66.0, -66.0, 0.6, 77.0, -77.0, 0.7}));
}

TEST(CachedFeaturesTest, BoundsSensitive) {
  std::unique_ptr<FeatureProcessorOptions_::BoundsSensitiveFeaturesT> config(
      new FeatureProcessorOptions_::BoundsSensitiveFeaturesT());
  config->enabled = true;
  config->num_tokens_before = 2;
  config->num_tokens_inside_left = 2;
  config->num_tokens_inside_right = 2;
  config->num_tokens_after = 2;
  config->include_inside_bag = true;
  config->include_inside_length = true;
  FeatureProcessorOptionsT options;
  options.bounds_sensitive_features = std::move(config);
  options.feature_version = 2;
  flatbuffers::FlatBufferBuilder builder;
  builder.Finish(CreateFeatureProcessorOptions(builder, &options));
  flatbuffers::DetachedBuffer options_fb = builder.Release();

  std::unique_ptr<std::vector<float>> features = MakeFeatures(9);
  std::unique_ptr<std::vector<float>> padding_features(
      new std::vector<float>{112233.0, -112233.0, 321.0});

  const std::unique_ptr<CachedFeatures> cached_features =
      CachedFeatures::Create(
          {3, 9}, std::move(features), std::move(padding_features),
          flatbuffers::GetRoot<FeatureProcessorOptions>(options_fb.data()),
          /*feature_vector_size=*/3);
  ASSERT_TRUE(cached_features);

  EXPECT_THAT(
      GetCachedBoundsSensitiveFeatures(*cached_features, {5, 8}),
      ElementsAreFloat({11.0,     -11.0,     0.1,   22.0,  -22.0, 0.2,   33.0,
                        -33.0,    0.3,       44.0,  -44.0, 0.4,   44.0,  -44.0,
                        0.4,      55.0,      -55.0, 0.5,   66.0,  -66.0, 0.6,
                        112233.0, -112233.0, 321.0, 44.0,  -44.0, 0.4,   3.0}));

  EXPECT_THAT(
      GetCachedBoundsSensitiveFeatures(*cached_features, {5, 7}),
      ElementsAreFloat({11.0,  -11.0, 0.1,   22.0,  -22.0, 0.2,   33.0,
                        -33.0, 0.3,   44.0,  -44.0, 0.4,   33.0,  -33.0,
                        0.3,   44.0,  -44.0, 0.4,   55.0,  -55.0, 0.5,
                        66.0,  -66.0, 0.6,   38.5,  -38.5, 0.35,  2.0}));

  EXPECT_THAT(
      GetCachedBoundsSensitiveFeatures(*cached_features, {6, 8}),
      ElementsAreFloat({22.0,     -22.0,     0.2,   33.0,  -33.0, 0.3,   44.0,
                        -44.0,    0.4,       55.0,  -55.0, 0.5,   44.0,  -44.0,
                        0.4,      55.0,      -55.0, 0.5,   66.0,  -66.0, 0.6,
                        112233.0, -112233.0, 321.0, 49.5,  -49.5, 0.45,  2.0}));

  EXPECT_THAT(
      GetCachedBoundsSensitiveFeatures(*cached_features, {6, 7}),
      ElementsAreFloat({22.0,     -22.0,     0.2,   33.0,     -33.0,     0.3,
                        44.0,     -44.0,     0.4,   112233.0, -112233.0, 321.0,
                        112233.0, -112233.0, 321.0, 44.0,     -44.0,     0.4,
                        55.0,     -55.0,     0.5,   66.0,     -66.0,     0.6,
                        44.0,     -44.0,     0.4,   1.0}));
}

}  // namespace
}  // namespace libtextclassifier2
