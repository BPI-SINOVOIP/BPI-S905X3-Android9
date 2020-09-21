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

#include "feature-processor.h"

#include "model-executor.h"
#include "tensor-view.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace libtextclassifier2 {
namespace {

using testing::ElementsAreArray;
using testing::FloatEq;
using testing::Matcher;

flatbuffers::DetachedBuffer PackFeatureProcessorOptions(
    const FeatureProcessorOptionsT& options) {
  flatbuffers::FlatBufferBuilder builder;
  builder.Finish(CreateFeatureProcessorOptions(builder, &options));
  return builder.Release();
}

template <typename T>
std::vector<T> Subvector(const std::vector<T>& vector, int start, int end) {
  return std::vector<T>(vector.begin() + start, vector.begin() + end);
}

Matcher<std::vector<float>> ElementsAreFloat(const std::vector<float>& values) {
  std::vector<Matcher<float>> matchers;
  for (const float value : values) {
    matchers.push_back(FloatEq(value));
  }
  return ElementsAreArray(matchers);
}

class TestingFeatureProcessor : public FeatureProcessor {
 public:
  using FeatureProcessor::CountIgnoredSpanBoundaryCodepoints;
  using FeatureProcessor::FeatureProcessor;
  using FeatureProcessor::ICUTokenize;
  using FeatureProcessor::IsCodepointInRanges;
  using FeatureProcessor::SpanToLabel;
  using FeatureProcessor::StripTokensFromOtherLines;
  using FeatureProcessor::supported_codepoint_ranges_;
  using FeatureProcessor::SupportedCodepointsRatio;
};

// EmbeddingExecutor that always returns features based on
class FakeEmbeddingExecutor : public EmbeddingExecutor {
 public:
  bool AddEmbedding(const TensorView<int>& sparse_features, float* dest,
                    int dest_size) const override {
    TC_CHECK_GE(dest_size, 4);
    EXPECT_EQ(sparse_features.size(), 1);
    dest[0] = sparse_features.data()[0];
    dest[1] = sparse_features.data()[0];
    dest[2] = -sparse_features.data()[0];
    dest[3] = -sparse_features.data()[0];
    return true;
  }

 private:
  std::vector<float> storage_;
};

TEST(FeatureProcessorTest, SplitTokensOnSelectionBoundariesMiddle) {
  std::vector<Token> tokens{Token("Hělló", 0, 5),
                            Token("fěěbař@google.com", 6, 23),
                            Token("heře!", 24, 29)};

  internal::SplitTokensOnSelectionBoundaries({9, 12}, &tokens);

  // clang-format off
  EXPECT_THAT(tokens, ElementsAreArray(
                          {Token("Hělló", 0, 5),
                           Token("fěě", 6, 9),
                           Token("bař", 9, 12),
                           Token("@google.com", 12, 23),
                           Token("heře!", 24, 29)}));
  // clang-format on
}

TEST(FeatureProcessorTest, SplitTokensOnSelectionBoundariesBegin) {
  std::vector<Token> tokens{Token("Hělló", 0, 5),
                            Token("fěěbař@google.com", 6, 23),
                            Token("heře!", 24, 29)};

  internal::SplitTokensOnSelectionBoundaries({6, 12}, &tokens);

  // clang-format off
  EXPECT_THAT(tokens, ElementsAreArray(
                          {Token("Hělló", 0, 5),
                           Token("fěěbař", 6, 12),
                           Token("@google.com", 12, 23),
                           Token("heře!", 24, 29)}));
  // clang-format on
}

TEST(FeatureProcessorTest, SplitTokensOnSelectionBoundariesEnd) {
  std::vector<Token> tokens{Token("Hělló", 0, 5),
                            Token("fěěbař@google.com", 6, 23),
                            Token("heře!", 24, 29)};

  internal::SplitTokensOnSelectionBoundaries({9, 23}, &tokens);

  // clang-format off
  EXPECT_THAT(tokens, ElementsAreArray(
                          {Token("Hělló", 0, 5),
                           Token("fěě", 6, 9),
                           Token("bař@google.com", 9, 23),
                           Token("heře!", 24, 29)}));
  // clang-format on
}

TEST(FeatureProcessorTest, SplitTokensOnSelectionBoundariesWhole) {
  std::vector<Token> tokens{Token("Hělló", 0, 5),
                            Token("fěěbař@google.com", 6, 23),
                            Token("heře!", 24, 29)};

  internal::SplitTokensOnSelectionBoundaries({6, 23}, &tokens);

  // clang-format off
  EXPECT_THAT(tokens, ElementsAreArray(
                          {Token("Hělló", 0, 5),
                           Token("fěěbař@google.com", 6, 23),
                           Token("heře!", 24, 29)}));
  // clang-format on
}

TEST(FeatureProcessorTest, SplitTokensOnSelectionBoundariesCrossToken) {
  std::vector<Token> tokens{Token("Hělló", 0, 5),
                            Token("fěěbař@google.com", 6, 23),
                            Token("heře!", 24, 29)};

  internal::SplitTokensOnSelectionBoundaries({2, 9}, &tokens);

  // clang-format off
  EXPECT_THAT(tokens, ElementsAreArray(
                          {Token("Hě", 0, 2),
                           Token("lló", 2, 5),
                           Token("fěě", 6, 9),
                           Token("bař@google.com", 9, 23),
                           Token("heře!", 24, 29)}));
  // clang-format on
}

TEST(FeatureProcessorTest, KeepLineWithClickFirst) {
  CREATE_UNILIB_FOR_TESTING;
  FeatureProcessorOptionsT options;
  options.only_use_line_with_click = true;
  flatbuffers::DetachedBuffer options_fb = PackFeatureProcessorOptions(options);
  TestingFeatureProcessor feature_processor(
      flatbuffers::GetRoot<FeatureProcessorOptions>(options_fb.data()),
      &unilib);

  const std::string context = "Fiřst Lině\nSěcond Lině\nThiřd Lině";
  const CodepointSpan span = {0, 5};
  // clang-format off
  std::vector<Token> tokens = {Token("Fiřst", 0, 5),
                               Token("Lině", 6, 10),
                               Token("Sěcond", 11, 17),
                               Token("Lině", 18, 22),
                               Token("Thiřd", 23, 28),
                               Token("Lině", 29, 33)};
  // clang-format on

  // Keeps the first line.
  feature_processor.StripTokensFromOtherLines(context, span, &tokens);
  EXPECT_THAT(tokens,
              ElementsAreArray({Token("Fiřst", 0, 5), Token("Lině", 6, 10)}));
}

TEST(FeatureProcessorTest, KeepLineWithClickSecond) {
  CREATE_UNILIB_FOR_TESTING;
  FeatureProcessorOptionsT options;
  options.only_use_line_with_click = true;
  flatbuffers::DetachedBuffer options_fb = PackFeatureProcessorOptions(options);
  TestingFeatureProcessor feature_processor(
      flatbuffers::GetRoot<FeatureProcessorOptions>(options_fb.data()),
      &unilib);

  const std::string context = "Fiřst Lině\nSěcond Lině\nThiřd Lině";
  const CodepointSpan span = {18, 22};
  // clang-format off
  std::vector<Token> tokens = {Token("Fiřst", 0, 5),
                               Token("Lině", 6, 10),
                               Token("Sěcond", 11, 17),
                               Token("Lině", 18, 22),
                               Token("Thiřd", 23, 28),
                               Token("Lině", 29, 33)};
  // clang-format on

  // Keeps the first line.
  feature_processor.StripTokensFromOtherLines(context, span, &tokens);
  EXPECT_THAT(tokens, ElementsAreArray(
                          {Token("Sěcond", 11, 17), Token("Lině", 18, 22)}));
}

TEST(FeatureProcessorTest, KeepLineWithClickThird) {
  CREATE_UNILIB_FOR_TESTING;
  FeatureProcessorOptionsT options;
  options.only_use_line_with_click = true;
  flatbuffers::DetachedBuffer options_fb = PackFeatureProcessorOptions(options);
  TestingFeatureProcessor feature_processor(
      flatbuffers::GetRoot<FeatureProcessorOptions>(options_fb.data()),
      &unilib);

  const std::string context = "Fiřst Lině\nSěcond Lině\nThiřd Lině";
  const CodepointSpan span = {24, 33};
  // clang-format off
  std::vector<Token> tokens = {Token("Fiřst", 0, 5),
                               Token("Lině", 6, 10),
                               Token("Sěcond", 11, 17),
                               Token("Lině", 18, 22),
                               Token("Thiřd", 23, 28),
                               Token("Lině", 29, 33)};
  // clang-format on

  // Keeps the first line.
  feature_processor.StripTokensFromOtherLines(context, span, &tokens);
  EXPECT_THAT(tokens, ElementsAreArray(
                          {Token("Thiřd", 23, 28), Token("Lině", 29, 33)}));
}

TEST(FeatureProcessorTest, KeepLineWithClickSecondWithPipe) {
  CREATE_UNILIB_FOR_TESTING;
  FeatureProcessorOptionsT options;
  options.only_use_line_with_click = true;
  flatbuffers::DetachedBuffer options_fb = PackFeatureProcessorOptions(options);
  TestingFeatureProcessor feature_processor(
      flatbuffers::GetRoot<FeatureProcessorOptions>(options_fb.data()),
      &unilib);

  const std::string context = "Fiřst Lině|Sěcond Lině\nThiřd Lině";
  const CodepointSpan span = {18, 22};
  // clang-format off
  std::vector<Token> tokens = {Token("Fiřst", 0, 5),
                               Token("Lině", 6, 10),
                               Token("Sěcond", 11, 17),
                               Token("Lině", 18, 22),
                               Token("Thiřd", 23, 28),
                               Token("Lině", 29, 33)};
  // clang-format on

  // Keeps the first line.
  feature_processor.StripTokensFromOtherLines(context, span, &tokens);
  EXPECT_THAT(tokens, ElementsAreArray(
                          {Token("Sěcond", 11, 17), Token("Lině", 18, 22)}));
}

TEST(FeatureProcessorTest, KeepLineWithCrosslineClick) {
  CREATE_UNILIB_FOR_TESTING;
  FeatureProcessorOptionsT options;
  options.only_use_line_with_click = true;
  flatbuffers::DetachedBuffer options_fb = PackFeatureProcessorOptions(options);
  TestingFeatureProcessor feature_processor(
      flatbuffers::GetRoot<FeatureProcessorOptions>(options_fb.data()),
      &unilib);

  const std::string context = "Fiřst Lině\nSěcond Lině\nThiřd Lině";
  const CodepointSpan span = {5, 23};
  // clang-format off
  std::vector<Token> tokens = {Token("Fiřst", 0, 5),
                               Token("Lině", 6, 10),
                               Token("Sěcond", 18, 23),
                               Token("Lině", 19, 23),
                               Token("Thiřd", 23, 28),
                               Token("Lině", 29, 33)};
  // clang-format on

  // Keeps the first line.
  feature_processor.StripTokensFromOtherLines(context, span, &tokens);
  EXPECT_THAT(tokens, ElementsAreArray(
                          {Token("Fiřst", 0, 5), Token("Lině", 6, 10),
                           Token("Sěcond", 18, 23), Token("Lině", 19, 23),
                           Token("Thiřd", 23, 28), Token("Lině", 29, 33)}));
}

TEST(FeatureProcessorTest, SpanToLabel) {
  CREATE_UNILIB_FOR_TESTING;
  FeatureProcessorOptionsT options;
  options.context_size = 1;
  options.max_selection_span = 1;
  options.snap_label_span_boundaries_to_containing_tokens = false;

  options.tokenization_codepoint_config.emplace_back(
      new TokenizationCodepointRangeT());
  auto& config = options.tokenization_codepoint_config.back();
  config->start = 32;
  config->end = 33;
  config->role = TokenizationCodepointRange_::Role_WHITESPACE_SEPARATOR;

  flatbuffers::DetachedBuffer options_fb = PackFeatureProcessorOptions(options);
  TestingFeatureProcessor feature_processor(
      flatbuffers::GetRoot<FeatureProcessorOptions>(options_fb.data()),
      &unilib);
  std::vector<Token> tokens = feature_processor.Tokenize("one, two, three");
  ASSERT_EQ(3, tokens.size());
  int label;
  ASSERT_TRUE(feature_processor.SpanToLabel({5, 8}, tokens, &label));
  EXPECT_EQ(kInvalidLabel, label);
  ASSERT_TRUE(feature_processor.SpanToLabel({5, 9}, tokens, &label));
  EXPECT_NE(kInvalidLabel, label);
  TokenSpan token_span;
  feature_processor.LabelToTokenSpan(label, &token_span);
  EXPECT_EQ(0, token_span.first);
  EXPECT_EQ(0, token_span.second);

  // Reconfigure with snapping enabled.
  options.snap_label_span_boundaries_to_containing_tokens = true;
  flatbuffers::DetachedBuffer options2_fb =
      PackFeatureProcessorOptions(options);
  TestingFeatureProcessor feature_processor2(
      flatbuffers::GetRoot<FeatureProcessorOptions>(options2_fb.data()),
      &unilib);
  int label2;
  ASSERT_TRUE(feature_processor2.SpanToLabel({5, 8}, tokens, &label2));
  EXPECT_EQ(label, label2);
  ASSERT_TRUE(feature_processor2.SpanToLabel({6, 9}, tokens, &label2));
  EXPECT_EQ(label, label2);
  ASSERT_TRUE(feature_processor2.SpanToLabel({5, 9}, tokens, &label2));
  EXPECT_EQ(label, label2);

  // Cross a token boundary.
  ASSERT_TRUE(feature_processor2.SpanToLabel({4, 9}, tokens, &label2));
  EXPECT_EQ(kInvalidLabel, label2);
  ASSERT_TRUE(feature_processor2.SpanToLabel({5, 10}, tokens, &label2));
  EXPECT_EQ(kInvalidLabel, label2);

  // Multiple tokens.
  options.context_size = 2;
  options.max_selection_span = 2;
  flatbuffers::DetachedBuffer options3_fb =
      PackFeatureProcessorOptions(options);
  TestingFeatureProcessor feature_processor3(
      flatbuffers::GetRoot<FeatureProcessorOptions>(options3_fb.data()),
      &unilib);
  tokens = feature_processor3.Tokenize("zero, one, two, three, four");
  ASSERT_TRUE(feature_processor3.SpanToLabel({6, 15}, tokens, &label2));
  EXPECT_NE(kInvalidLabel, label2);
  feature_processor3.LabelToTokenSpan(label2, &token_span);
  EXPECT_EQ(1, token_span.first);
  EXPECT_EQ(0, token_span.second);

  int label3;
  ASSERT_TRUE(feature_processor3.SpanToLabel({6, 14}, tokens, &label3));
  EXPECT_EQ(label2, label3);
  ASSERT_TRUE(feature_processor3.SpanToLabel({6, 13}, tokens, &label3));
  EXPECT_EQ(label2, label3);
  ASSERT_TRUE(feature_processor3.SpanToLabel({7, 13}, tokens, &label3));
  EXPECT_EQ(label2, label3);
}

TEST(FeatureProcessorTest, SpanToLabelIgnoresPunctuation) {
  CREATE_UNILIB_FOR_TESTING;
  FeatureProcessorOptionsT options;
  options.context_size = 1;
  options.max_selection_span = 1;
  options.snap_label_span_boundaries_to_containing_tokens = false;

  options.tokenization_codepoint_config.emplace_back(
      new TokenizationCodepointRangeT());
  auto& config = options.tokenization_codepoint_config.back();
  config->start = 32;
  config->end = 33;
  config->role = TokenizationCodepointRange_::Role_WHITESPACE_SEPARATOR;

  flatbuffers::DetachedBuffer options_fb = PackFeatureProcessorOptions(options);
  TestingFeatureProcessor feature_processor(
      flatbuffers::GetRoot<FeatureProcessorOptions>(options_fb.data()),
      &unilib);
  std::vector<Token> tokens = feature_processor.Tokenize("one, two, three");
  ASSERT_EQ(3, tokens.size());
  int label;
  ASSERT_TRUE(feature_processor.SpanToLabel({5, 8}, tokens, &label));
  EXPECT_EQ(kInvalidLabel, label);
  ASSERT_TRUE(feature_processor.SpanToLabel({5, 9}, tokens, &label));
  EXPECT_NE(kInvalidLabel, label);
  TokenSpan token_span;
  feature_processor.LabelToTokenSpan(label, &token_span);
  EXPECT_EQ(0, token_span.first);
  EXPECT_EQ(0, token_span.second);

  // Reconfigure with snapping enabled.
  options.snap_label_span_boundaries_to_containing_tokens = true;
  flatbuffers::DetachedBuffer options2_fb =
      PackFeatureProcessorOptions(options);
  TestingFeatureProcessor feature_processor2(
      flatbuffers::GetRoot<FeatureProcessorOptions>(options2_fb.data()),
      &unilib);
  int label2;
  ASSERT_TRUE(feature_processor2.SpanToLabel({5, 8}, tokens, &label2));
  EXPECT_EQ(label, label2);
  ASSERT_TRUE(feature_processor2.SpanToLabel({6, 9}, tokens, &label2));
  EXPECT_EQ(label, label2);
  ASSERT_TRUE(feature_processor2.SpanToLabel({5, 9}, tokens, &label2));
  EXPECT_EQ(label, label2);

  // Cross a token boundary.
  ASSERT_TRUE(feature_processor2.SpanToLabel({4, 9}, tokens, &label2));
  EXPECT_EQ(kInvalidLabel, label2);
  ASSERT_TRUE(feature_processor2.SpanToLabel({5, 10}, tokens, &label2));
  EXPECT_EQ(kInvalidLabel, label2);

  // Multiple tokens.
  options.context_size = 2;
  options.max_selection_span = 2;
  flatbuffers::DetachedBuffer options3_fb =
      PackFeatureProcessorOptions(options);
  TestingFeatureProcessor feature_processor3(
      flatbuffers::GetRoot<FeatureProcessorOptions>(options3_fb.data()),
      &unilib);
  tokens = feature_processor3.Tokenize("zero, one, two, three, four");
  ASSERT_TRUE(feature_processor3.SpanToLabel({6, 15}, tokens, &label2));
  EXPECT_NE(kInvalidLabel, label2);
  feature_processor3.LabelToTokenSpan(label2, &token_span);
  EXPECT_EQ(1, token_span.first);
  EXPECT_EQ(0, token_span.second);

  int label3;
  ASSERT_TRUE(feature_processor3.SpanToLabel({6, 14}, tokens, &label3));
  EXPECT_EQ(label2, label3);
  ASSERT_TRUE(feature_processor3.SpanToLabel({6, 13}, tokens, &label3));
  EXPECT_EQ(label2, label3);
  ASSERT_TRUE(feature_processor3.SpanToLabel({7, 13}, tokens, &label3));
  EXPECT_EQ(label2, label3);
}

TEST(FeatureProcessorTest, CenterTokenFromClick) {
  int token_index;

  // Exactly aligned indices.
  token_index = internal::CenterTokenFromClick(
      {6, 11},
      {Token("Hělló", 0, 5), Token("world", 6, 11), Token("heře!", 12, 17)});
  EXPECT_EQ(token_index, 1);

  // Click is contained in a token.
  token_index = internal::CenterTokenFromClick(
      {13, 17},
      {Token("Hělló", 0, 5), Token("world", 6, 11), Token("heře!", 12, 17)});
  EXPECT_EQ(token_index, 2);

  // Click spans two tokens.
  token_index = internal::CenterTokenFromClick(
      {6, 17},
      {Token("Hělló", 0, 5), Token("world", 6, 11), Token("heře!", 12, 17)});
  EXPECT_EQ(token_index, kInvalidIndex);
}

TEST(FeatureProcessorTest, CenterTokenFromMiddleOfSelection) {
  int token_index;

  // Selection of length 3. Exactly aligned indices.
  token_index = internal::CenterTokenFromMiddleOfSelection(
      {7, 27},
      {Token("Token1", 0, 6), Token("Token2", 7, 13), Token("Token3", 14, 20),
       Token("Token4", 21, 27), Token("Token5", 28, 34)});
  EXPECT_EQ(token_index, 2);

  // Selection of length 1 token. Exactly aligned indices.
  token_index = internal::CenterTokenFromMiddleOfSelection(
      {21, 27},
      {Token("Token1", 0, 6), Token("Token2", 7, 13), Token("Token3", 14, 20),
       Token("Token4", 21, 27), Token("Token5", 28, 34)});
  EXPECT_EQ(token_index, 3);

  // Selection marks sub-token range, with no tokens in it.
  token_index = internal::CenterTokenFromMiddleOfSelection(
      {29, 33},
      {Token("Token1", 0, 6), Token("Token2", 7, 13), Token("Token3", 14, 20),
       Token("Token4", 21, 27), Token("Token5", 28, 34)});
  EXPECT_EQ(token_index, kInvalidIndex);

  // Selection of length 2. Sub-token indices.
  token_index = internal::CenterTokenFromMiddleOfSelection(
      {3, 25},
      {Token("Token1", 0, 6), Token("Token2", 7, 13), Token("Token3", 14, 20),
       Token("Token4", 21, 27), Token("Token5", 28, 34)});
  EXPECT_EQ(token_index, 1);

  // Selection of length 1. Sub-token indices.
  token_index = internal::CenterTokenFromMiddleOfSelection(
      {22, 34},
      {Token("Token1", 0, 6), Token("Token2", 7, 13), Token("Token3", 14, 20),
       Token("Token4", 21, 27), Token("Token5", 28, 34)});
  EXPECT_EQ(token_index, 4);

  // Some invalid ones.
  token_index = internal::CenterTokenFromMiddleOfSelection({7, 27}, {});
  EXPECT_EQ(token_index, -1);
}

TEST(FeatureProcessorTest, SupportedCodepointsRatio) {
  FeatureProcessorOptionsT options;
  options.context_size = 2;
  options.max_selection_span = 2;
  options.snap_label_span_boundaries_to_containing_tokens = false;
  options.feature_version = 2;
  options.embedding_size = 4;
  options.bounds_sensitive_features.reset(
      new FeatureProcessorOptions_::BoundsSensitiveFeaturesT());
  options.bounds_sensitive_features->enabled = true;
  options.bounds_sensitive_features->num_tokens_before = 5;
  options.bounds_sensitive_features->num_tokens_inside_left = 3;
  options.bounds_sensitive_features->num_tokens_inside_right = 3;
  options.bounds_sensitive_features->num_tokens_after = 5;
  options.bounds_sensitive_features->include_inside_bag = true;
  options.bounds_sensitive_features->include_inside_length = true;

  options.tokenization_codepoint_config.emplace_back(
      new TokenizationCodepointRangeT());
  auto& config = options.tokenization_codepoint_config.back();
  config->start = 32;
  config->end = 33;
  config->role = TokenizationCodepointRange_::Role_WHITESPACE_SEPARATOR;

  {
    options.supported_codepoint_ranges.emplace_back(
        new FeatureProcessorOptions_::CodepointRangeT());
    auto& range = options.supported_codepoint_ranges.back();
    range->start = 0;
    range->end = 128;
  }

  {
    options.supported_codepoint_ranges.emplace_back(
        new FeatureProcessorOptions_::CodepointRangeT());
    auto& range = options.supported_codepoint_ranges.back();
    range->start = 10000;
    range->end = 10001;
  }

  {
    options.supported_codepoint_ranges.emplace_back(
        new FeatureProcessorOptions_::CodepointRangeT());
    auto& range = options.supported_codepoint_ranges.back();
    range->start = 20000;
    range->end = 30000;
  }

  flatbuffers::DetachedBuffer options_fb = PackFeatureProcessorOptions(options);
  CREATE_UNILIB_FOR_TESTING;
  TestingFeatureProcessor feature_processor(
      flatbuffers::GetRoot<FeatureProcessorOptions>(options_fb.data()),
      &unilib);
  EXPECT_THAT(feature_processor.SupportedCodepointsRatio(
                  {0, 3}, feature_processor.Tokenize("aaa bbb ccc")),
              FloatEq(1.0));
  EXPECT_THAT(feature_processor.SupportedCodepointsRatio(
                  {0, 3}, feature_processor.Tokenize("aaa bbb ěěě")),
              FloatEq(2.0 / 3));
  EXPECT_THAT(feature_processor.SupportedCodepointsRatio(
                  {0, 3}, feature_processor.Tokenize("ěěě řřř ěěě")),
              FloatEq(0.0));
  EXPECT_FALSE(feature_processor.IsCodepointInRanges(
      -1, feature_processor.supported_codepoint_ranges_));
  EXPECT_TRUE(feature_processor.IsCodepointInRanges(
      0, feature_processor.supported_codepoint_ranges_));
  EXPECT_TRUE(feature_processor.IsCodepointInRanges(
      10, feature_processor.supported_codepoint_ranges_));
  EXPECT_TRUE(feature_processor.IsCodepointInRanges(
      127, feature_processor.supported_codepoint_ranges_));
  EXPECT_FALSE(feature_processor.IsCodepointInRanges(
      128, feature_processor.supported_codepoint_ranges_));
  EXPECT_FALSE(feature_processor.IsCodepointInRanges(
      9999, feature_processor.supported_codepoint_ranges_));
  EXPECT_TRUE(feature_processor.IsCodepointInRanges(
      10000, feature_processor.supported_codepoint_ranges_));
  EXPECT_FALSE(feature_processor.IsCodepointInRanges(
      10001, feature_processor.supported_codepoint_ranges_));
  EXPECT_TRUE(feature_processor.IsCodepointInRanges(
      25000, feature_processor.supported_codepoint_ranges_));

  const std::vector<Token> tokens = {Token("ěěě", 0, 3), Token("řřř", 4, 7),
                                     Token("eee", 8, 11)};

  options.min_supported_codepoint_ratio = 0.0;
  flatbuffers::DetachedBuffer options2_fb =
      PackFeatureProcessorOptions(options);
  TestingFeatureProcessor feature_processor2(
      flatbuffers::GetRoot<FeatureProcessorOptions>(options2_fb.data()),
      &unilib);
  EXPECT_TRUE(feature_processor2.HasEnoughSupportedCodepoints(
      tokens, /*token_span=*/{0, 3}));

  options.min_supported_codepoint_ratio = 0.2;
  flatbuffers::DetachedBuffer options3_fb =
      PackFeatureProcessorOptions(options);
  TestingFeatureProcessor feature_processor3(
      flatbuffers::GetRoot<FeatureProcessorOptions>(options3_fb.data()),
      &unilib);
  EXPECT_TRUE(feature_processor3.HasEnoughSupportedCodepoints(
      tokens, /*token_span=*/{0, 3}));

  options.min_supported_codepoint_ratio = 0.5;
  flatbuffers::DetachedBuffer options4_fb =
      PackFeatureProcessorOptions(options);
  TestingFeatureProcessor feature_processor4(
      flatbuffers::GetRoot<FeatureProcessorOptions>(options4_fb.data()),
      &unilib);
  EXPECT_FALSE(feature_processor4.HasEnoughSupportedCodepoints(
      tokens, /*token_span=*/{0, 3}));
}

TEST(FeatureProcessorTest, InSpanFeature) {
  FeatureProcessorOptionsT options;
  options.context_size = 2;
  options.max_selection_span = 2;
  options.snap_label_span_boundaries_to_containing_tokens = false;
  options.feature_version = 2;
  options.embedding_size = 4;
  options.extract_selection_mask_feature = true;

  flatbuffers::DetachedBuffer options_fb = PackFeatureProcessorOptions(options);
  CREATE_UNILIB_FOR_TESTING;
  TestingFeatureProcessor feature_processor(
      flatbuffers::GetRoot<FeatureProcessorOptions>(options_fb.data()),
      &unilib);

  std::unique_ptr<CachedFeatures> cached_features;

  FakeEmbeddingExecutor embedding_executor;

  const std::vector<Token> tokens = {Token("aaa", 0, 3), Token("bbb", 4, 7),
                                     Token("ccc", 8, 11), Token("ddd", 12, 15)};

  EXPECT_TRUE(feature_processor.ExtractFeatures(
      tokens, /*token_span=*/{0, 4},
      /*selection_span_for_feature=*/{4, 11}, &embedding_executor,
      /*embedding_cache=*/nullptr, /*feature_vector_size=*/5,
      &cached_features));
  std::vector<float> features;
  cached_features->AppendClickContextFeaturesForClick(1, &features);
  ASSERT_EQ(features.size(), 25);
  EXPECT_THAT(features[4], FloatEq(0.0));
  EXPECT_THAT(features[9], FloatEq(0.0));
  EXPECT_THAT(features[14], FloatEq(1.0));
  EXPECT_THAT(features[19], FloatEq(1.0));
  EXPECT_THAT(features[24], FloatEq(0.0));
}

TEST(FeatureProcessorTest, EmbeddingCache) {
  FeatureProcessorOptionsT options;
  options.context_size = 2;
  options.max_selection_span = 2;
  options.snap_label_span_boundaries_to_containing_tokens = false;
  options.feature_version = 2;
  options.embedding_size = 4;
  options.bounds_sensitive_features.reset(
      new FeatureProcessorOptions_::BoundsSensitiveFeaturesT());
  options.bounds_sensitive_features->enabled = true;
  options.bounds_sensitive_features->num_tokens_before = 3;
  options.bounds_sensitive_features->num_tokens_inside_left = 2;
  options.bounds_sensitive_features->num_tokens_inside_right = 2;
  options.bounds_sensitive_features->num_tokens_after = 3;

  flatbuffers::DetachedBuffer options_fb = PackFeatureProcessorOptions(options);
  CREATE_UNILIB_FOR_TESTING;
  TestingFeatureProcessor feature_processor(
      flatbuffers::GetRoot<FeatureProcessorOptions>(options_fb.data()),
      &unilib);

  std::unique_ptr<CachedFeatures> cached_features;

  FakeEmbeddingExecutor embedding_executor;

  const std::vector<Token> tokens = {
      Token("aaa", 0, 3),   Token("bbb", 4, 7),   Token("ccc", 8, 11),
      Token("ddd", 12, 15), Token("eee", 16, 19), Token("fff", 20, 23)};

  // We pre-populate the cache with dummy embeddings, to make sure they are
  // used when populating the features vector.
  const std::vector<float> cached_padding_features = {10.0, -10.0, 10.0, -10.0};
  const std::vector<float> cached_features1 = {1.0, 2.0, 3.0, 4.0};
  const std::vector<float> cached_features2 = {5.0, 6.0, 7.0, 8.0};
  FeatureProcessor::EmbeddingCache embedding_cache = {
      {{kInvalidIndex, kInvalidIndex}, cached_padding_features},
      {{4, 7}, cached_features1},
      {{12, 15}, cached_features2},
  };

  EXPECT_TRUE(feature_processor.ExtractFeatures(
      tokens, /*token_span=*/{0, 6},
      /*selection_span_for_feature=*/{kInvalidIndex, kInvalidIndex},
      &embedding_executor, &embedding_cache, /*feature_vector_size=*/4,
      &cached_features));
  std::vector<float> features;
  cached_features->AppendBoundsSensitiveFeaturesForSpan({2, 4}, &features);
  ASSERT_EQ(features.size(), 40);
  // Check that the dummy embeddings were used.
  EXPECT_THAT(Subvector(features, 0, 4),
              ElementsAreFloat(cached_padding_features));
  EXPECT_THAT(Subvector(features, 8, 12), ElementsAreFloat(cached_features1));
  EXPECT_THAT(Subvector(features, 16, 20), ElementsAreFloat(cached_features2));
  EXPECT_THAT(Subvector(features, 24, 28), ElementsAreFloat(cached_features2));
  EXPECT_THAT(Subvector(features, 36, 40),
              ElementsAreFloat(cached_padding_features));
  // Check that the real embeddings were cached.
  EXPECT_EQ(embedding_cache.size(), 7);
  EXPECT_THAT(Subvector(features, 4, 8),
              ElementsAreFloat(embedding_cache.at({0, 3})));
  EXPECT_THAT(Subvector(features, 12, 16),
              ElementsAreFloat(embedding_cache.at({8, 11})));
  EXPECT_THAT(Subvector(features, 20, 24),
              ElementsAreFloat(embedding_cache.at({8, 11})));
  EXPECT_THAT(Subvector(features, 28, 32),
              ElementsAreFloat(embedding_cache.at({16, 19})));
  EXPECT_THAT(Subvector(features, 32, 36),
              ElementsAreFloat(embedding_cache.at({20, 23})));
}

TEST(FeatureProcessorTest, StripUnusedTokensWithNoRelativeClick) {
  std::vector<Token> tokens_orig{
      Token("0", 0, 0), Token("1", 0, 0), Token("2", 0, 0),  Token("3", 0, 0),
      Token("4", 0, 0), Token("5", 0, 0), Token("6", 0, 0),  Token("7", 0, 0),
      Token("8", 0, 0), Token("9", 0, 0), Token("10", 0, 0), Token("11", 0, 0),
      Token("12", 0, 0)};

  std::vector<Token> tokens;
  int click_index;

  // Try to click first token and see if it gets padded from left.
  tokens = tokens_orig;
  click_index = 0;
  internal::StripOrPadTokens({0, 0}, 2, &tokens, &click_index);
  // clang-format off
  EXPECT_EQ(tokens, std::vector<Token>({Token(),
                                        Token(),
                                        Token("0", 0, 0),
                                        Token("1", 0, 0),
                                        Token("2", 0, 0)}));
  // clang-format on
  EXPECT_EQ(click_index, 2);

  // When we click the second token nothing should get padded.
  tokens = tokens_orig;
  click_index = 2;
  internal::StripOrPadTokens({0, 0}, 2, &tokens, &click_index);
  // clang-format off
  EXPECT_EQ(tokens, std::vector<Token>({Token("0", 0, 0),
                                        Token("1", 0, 0),
                                        Token("2", 0, 0),
                                        Token("3", 0, 0),
                                        Token("4", 0, 0)}));
  // clang-format on
  EXPECT_EQ(click_index, 2);

  // When we click the last token tokens should get padded from the right.
  tokens = tokens_orig;
  click_index = 12;
  internal::StripOrPadTokens({0, 0}, 2, &tokens, &click_index);
  // clang-format off
  EXPECT_EQ(tokens, std::vector<Token>({Token("10", 0, 0),
                                        Token("11", 0, 0),
                                        Token("12", 0, 0),
                                        Token(),
                                        Token()}));
  // clang-format on
  EXPECT_EQ(click_index, 2);
}

TEST(FeatureProcessorTest, StripUnusedTokensWithRelativeClick) {
  std::vector<Token> tokens_orig{
      Token("0", 0, 0), Token("1", 0, 0), Token("2", 0, 0),  Token("3", 0, 0),
      Token("4", 0, 0), Token("5", 0, 0), Token("6", 0, 0),  Token("7", 0, 0),
      Token("8", 0, 0), Token("9", 0, 0), Token("10", 0, 0), Token("11", 0, 0),
      Token("12", 0, 0)};

  std::vector<Token> tokens;
  int click_index;

  // Try to click first token and see if it gets padded from left to maximum
  // context_size.
  tokens = tokens_orig;
  click_index = 0;
  internal::StripOrPadTokens({2, 3}, 2, &tokens, &click_index);
  // clang-format off
  EXPECT_EQ(tokens, std::vector<Token>({Token(),
                                        Token(),
                                        Token("0", 0, 0),
                                        Token("1", 0, 0),
                                        Token("2", 0, 0),
                                        Token("3", 0, 0),
                                        Token("4", 0, 0),
                                        Token("5", 0, 0)}));
  // clang-format on
  EXPECT_EQ(click_index, 2);

  // Clicking to the middle with enough context should not produce any padding.
  tokens = tokens_orig;
  click_index = 6;
  internal::StripOrPadTokens({3, 1}, 2, &tokens, &click_index);
  // clang-format off
  EXPECT_EQ(tokens, std::vector<Token>({Token("1", 0, 0),
                                        Token("2", 0, 0),
                                        Token("3", 0, 0),
                                        Token("4", 0, 0),
                                        Token("5", 0, 0),
                                        Token("6", 0, 0),
                                        Token("7", 0, 0),
                                        Token("8", 0, 0),
                                        Token("9", 0, 0)}));
  // clang-format on
  EXPECT_EQ(click_index, 5);

  // Clicking at the end should pad right to maximum context_size.
  tokens = tokens_orig;
  click_index = 11;
  internal::StripOrPadTokens({3, 1}, 2, &tokens, &click_index);
  // clang-format off
  EXPECT_EQ(tokens, std::vector<Token>({Token("6", 0, 0),
                                        Token("7", 0, 0),
                                        Token("8", 0, 0),
                                        Token("9", 0, 0),
                                        Token("10", 0, 0),
                                        Token("11", 0, 0),
                                        Token("12", 0, 0),
                                        Token(),
                                        Token()}));
  // clang-format on
  EXPECT_EQ(click_index, 5);
}

TEST(FeatureProcessorTest, InternalTokenizeOnScriptChange) {
  CREATE_UNILIB_FOR_TESTING;
  FeatureProcessorOptionsT options;
  options.tokenization_codepoint_config.emplace_back(
      new TokenizationCodepointRangeT());
  {
    auto& config = options.tokenization_codepoint_config.back();
    config->start = 0;
    config->end = 256;
    config->role = TokenizationCodepointRange_::Role_DEFAULT_ROLE;
    config->script_id = 1;
  }
  options.tokenize_on_script_change = false;

  flatbuffers::DetachedBuffer options_fb = PackFeatureProcessorOptions(options);
  TestingFeatureProcessor feature_processor(
      flatbuffers::GetRoot<FeatureProcessorOptions>(options_fb.data()),
      &unilib);

  EXPECT_EQ(feature_processor.Tokenize("앨라배마123웹사이트"),
            std::vector<Token>({Token("앨라배마123웹사이트", 0, 11)}));

  options.tokenize_on_script_change = true;
  flatbuffers::DetachedBuffer options_fb2 =
      PackFeatureProcessorOptions(options);
  TestingFeatureProcessor feature_processor2(
      flatbuffers::GetRoot<FeatureProcessorOptions>(options_fb2.data()),
      &unilib);

  EXPECT_EQ(feature_processor2.Tokenize("앨라배마123웹사이트"),
            std::vector<Token>({Token("앨라배마", 0, 4), Token("123", 4, 7),
                                Token("웹사이트", 7, 11)}));
}

#ifdef LIBTEXTCLASSIFIER_TEST_ICU
TEST(FeatureProcessorTest, ICUTokenize) {
  FeatureProcessorOptionsT options;
  options.tokenization_type = FeatureProcessorOptions_::TokenizationType_ICU;

  flatbuffers::DetachedBuffer options_fb = PackFeatureProcessorOptions(options);
  TestingFeatureProcessor feature_processor(
      flatbuffers::GetRoot<FeatureProcessorOptions>(options_fb.data()));
  std::vector<Token> tokens = feature_processor.Tokenize("พระบาทสมเด็จพระปรมิ");
  ASSERT_EQ(tokens,
            // clang-format off
            std::vector<Token>({Token("พระบาท", 0, 6),
                                Token("สมเด็จ", 6, 12),
                                Token("พระ", 12, 15),
                                Token("ปร", 15, 17),
                                Token("มิ", 17, 19)}));
  // clang-format on
}
#endif

#ifdef LIBTEXTCLASSIFIER_TEST_ICU
TEST(FeatureProcessorTest, ICUTokenizeWithWhitespaces) {
  FeatureProcessorOptionsT options;
  options.tokenization_type = FeatureProcessorOptions_::TokenizationType_ICU;
  options.icu_preserve_whitespace_tokens = true;

  flatbuffers::DetachedBuffer options_fb = PackFeatureProcessorOptions(options);
  TestingFeatureProcessor feature_processor(
      flatbuffers::GetRoot<FeatureProcessorOptions>(options_fb.data()));
  std::vector<Token> tokens =
      feature_processor.Tokenize("พระบาท สมเด็จ พระ ปร มิ");
  ASSERT_EQ(tokens,
            // clang-format off
            std::vector<Token>({Token("พระบาท", 0, 6),
                                Token(" ", 6, 7),
                                Token("สมเด็จ", 7, 13),
                                Token(" ", 13, 14),
                                Token("พระ", 14, 17),
                                Token(" ", 17, 18),
                                Token("ปร", 18, 20),
                                Token(" ", 20, 21),
                                Token("มิ", 21, 23)}));
  // clang-format on
}
#endif

#ifdef LIBTEXTCLASSIFIER_TEST_ICU
TEST(FeatureProcessorTest, MixedTokenize) {
  FeatureProcessorOptionsT options;
  options.tokenization_type = FeatureProcessorOptions_::TokenizationType_MIXED;

  options.tokenization_codepoint_config.emplace_back(
      new TokenizationCodepointRangeT());
  auto& config = options.tokenization_codepoint_config.back();
  config->start = 32;
  config->end = 33;
  config->role = TokenizationCodepointRange_::Role_WHITESPACE_SEPARATOR;

  {
    options.internal_tokenizer_codepoint_ranges.emplace_back(
        new FeatureProcessorOptions_::CodepointRangeT());
    auto& range = options.internal_tokenizer_codepoint_ranges.back();
    range->start = 0;
    range->end = 128;
  }

  {
    options.internal_tokenizer_codepoint_ranges.emplace_back(
        new FeatureProcessorOptions_::CodepointRangeT());
    auto& range = options.internal_tokenizer_codepoint_ranges.back();
    range->start = 128;
    range->end = 256;
  }

  {
    options.internal_tokenizer_codepoint_ranges.emplace_back(
        new FeatureProcessorOptions_::CodepointRangeT());
    auto& range = options.internal_tokenizer_codepoint_ranges.back();
    range->start = 256;
    range->end = 384;
  }

  {
    options.internal_tokenizer_codepoint_ranges.emplace_back(
        new FeatureProcessorOptions_::CodepointRangeT());
    auto& range = options.internal_tokenizer_codepoint_ranges.back();
    range->start = 384;
    range->end = 592;
  }

  flatbuffers::DetachedBuffer options_fb = PackFeatureProcessorOptions(options);
  TestingFeatureProcessor feature_processor(
      flatbuffers::GetRoot<FeatureProcessorOptions>(options_fb.data()));
  std::vector<Token> tokens = feature_processor.Tokenize(
      "こんにちはJapanese-ląnguagę text 世界 http://www.google.com/");
  ASSERT_EQ(tokens,
            // clang-format off
            std::vector<Token>({Token("こんにちは", 0, 5),
                                Token("Japanese-ląnguagę", 5, 22),
                                Token("text", 23, 27),
                                Token("世界", 28, 30),
                                Token("http://www.google.com/", 31, 53)}));
  // clang-format on
}
#endif

TEST(FeatureProcessorTest, IgnoredSpanBoundaryCodepoints) {
  CREATE_UNILIB_FOR_TESTING;
  FeatureProcessorOptionsT options;
  options.ignored_span_boundary_codepoints.push_back('.');
  options.ignored_span_boundary_codepoints.push_back(',');
  options.ignored_span_boundary_codepoints.push_back('[');
  options.ignored_span_boundary_codepoints.push_back(']');

  flatbuffers::DetachedBuffer options_fb = PackFeatureProcessorOptions(options);
  TestingFeatureProcessor feature_processor(
      flatbuffers::GetRoot<FeatureProcessorOptions>(options_fb.data()),
      &unilib);

  const std::string text1_utf8 = "ěščř";
  const UnicodeText text1 = UTF8ToUnicodeText(text1_utf8, /*do_copy=*/false);
  EXPECT_EQ(feature_processor.CountIgnoredSpanBoundaryCodepoints(
                text1.begin(), text1.end(),
                /*count_from_beginning=*/true),
            0);
  EXPECT_EQ(feature_processor.CountIgnoredSpanBoundaryCodepoints(
                text1.begin(), text1.end(),
                /*count_from_beginning=*/false),
            0);

  const std::string text2_utf8 = ".,abčd";
  const UnicodeText text2 = UTF8ToUnicodeText(text2_utf8, /*do_copy=*/false);
  EXPECT_EQ(feature_processor.CountIgnoredSpanBoundaryCodepoints(
                text2.begin(), text2.end(),
                /*count_from_beginning=*/true),
            2);
  EXPECT_EQ(feature_processor.CountIgnoredSpanBoundaryCodepoints(
                text2.begin(), text2.end(),
                /*count_from_beginning=*/false),
            0);

  const std::string text3_utf8 = ".,abčd[]";
  const UnicodeText text3 = UTF8ToUnicodeText(text3_utf8, /*do_copy=*/false);
  EXPECT_EQ(feature_processor.CountIgnoredSpanBoundaryCodepoints(
                text3.begin(), text3.end(),
                /*count_from_beginning=*/true),
            2);
  EXPECT_EQ(feature_processor.CountIgnoredSpanBoundaryCodepoints(
                text3.begin(), text3.end(),
                /*count_from_beginning=*/false),
            2);

  const std::string text4_utf8 = "[abčd]";
  const UnicodeText text4 = UTF8ToUnicodeText(text4_utf8, /*do_copy=*/false);
  EXPECT_EQ(feature_processor.CountIgnoredSpanBoundaryCodepoints(
                text4.begin(), text4.end(),
                /*count_from_beginning=*/true),
            1);
  EXPECT_EQ(feature_processor.CountIgnoredSpanBoundaryCodepoints(
                text4.begin(), text4.end(),
                /*count_from_beginning=*/false),
            1);

  const std::string text5_utf8 = "";
  const UnicodeText text5 = UTF8ToUnicodeText(text5_utf8, /*do_copy=*/false);
  EXPECT_EQ(feature_processor.CountIgnoredSpanBoundaryCodepoints(
                text5.begin(), text5.end(),
                /*count_from_beginning=*/true),
            0);
  EXPECT_EQ(feature_processor.CountIgnoredSpanBoundaryCodepoints(
                text5.begin(), text5.end(),
                /*count_from_beginning=*/false),
            0);

  const std::string text6_utf8 = "012345ěščř";
  const UnicodeText text6 = UTF8ToUnicodeText(text6_utf8, /*do_copy=*/false);
  UnicodeText::const_iterator text6_begin = text6.begin();
  std::advance(text6_begin, 6);
  EXPECT_EQ(feature_processor.CountIgnoredSpanBoundaryCodepoints(
                text6_begin, text6.end(),
                /*count_from_beginning=*/true),
            0);
  EXPECT_EQ(feature_processor.CountIgnoredSpanBoundaryCodepoints(
                text6_begin, text6.end(),
                /*count_from_beginning=*/false),
            0);

  const std::string text7_utf8 = "012345.,ěščř";
  const UnicodeText text7 = UTF8ToUnicodeText(text7_utf8, /*do_copy=*/false);
  UnicodeText::const_iterator text7_begin = text7.begin();
  std::advance(text7_begin, 6);
  EXPECT_EQ(feature_processor.CountIgnoredSpanBoundaryCodepoints(
                text7_begin, text7.end(),
                /*count_from_beginning=*/true),
            2);
  UnicodeText::const_iterator text7_end = text7.begin();
  std::advance(text7_end, 8);
  EXPECT_EQ(feature_processor.CountIgnoredSpanBoundaryCodepoints(
                text7.begin(), text7_end,
                /*count_from_beginning=*/false),
            2);

  // Test not stripping.
  EXPECT_EQ(feature_processor.StripBoundaryCodepoints(
                "Hello [[[Wořld]] or not?", {0, 24}),
            std::make_pair(0, 24));
  // Test basic stripping.
  EXPECT_EQ(feature_processor.StripBoundaryCodepoints(
                "Hello [[[Wořld]] or not?", {6, 16}),
            std::make_pair(9, 14));
  // Test stripping when everything is stripped.
  EXPECT_EQ(
      feature_processor.StripBoundaryCodepoints("Hello [[[]] or not?", {6, 11}),
      std::make_pair(6, 6));
  // Test stripping empty string.
  EXPECT_EQ(feature_processor.StripBoundaryCodepoints("", {0, 0}),
            std::make_pair(0, 0));
}

TEST(FeatureProcessorTest, CodepointSpanToTokenSpan) {
  const std::vector<Token> tokens{Token("Hělló", 0, 5),
                                  Token("fěěbař@google.com", 6, 23),
                                  Token("heře!", 24, 29)};

  // Spans matching the tokens exactly.
  EXPECT_EQ(TokenSpan(0, 1), CodepointSpanToTokenSpan(tokens, {0, 5}));
  EXPECT_EQ(TokenSpan(1, 2), CodepointSpanToTokenSpan(tokens, {6, 23}));
  EXPECT_EQ(TokenSpan(2, 3), CodepointSpanToTokenSpan(tokens, {24, 29}));
  EXPECT_EQ(TokenSpan(0, 2), CodepointSpanToTokenSpan(tokens, {0, 23}));
  EXPECT_EQ(TokenSpan(1, 3), CodepointSpanToTokenSpan(tokens, {6, 29}));
  EXPECT_EQ(TokenSpan(0, 3), CodepointSpanToTokenSpan(tokens, {0, 29}));

  // Snapping to containing tokens has no effect.
  EXPECT_EQ(TokenSpan(0, 1), CodepointSpanToTokenSpan(tokens, {0, 5}, true));
  EXPECT_EQ(TokenSpan(1, 2), CodepointSpanToTokenSpan(tokens, {6, 23}, true));
  EXPECT_EQ(TokenSpan(2, 3), CodepointSpanToTokenSpan(tokens, {24, 29}, true));
  EXPECT_EQ(TokenSpan(0, 2), CodepointSpanToTokenSpan(tokens, {0, 23}, true));
  EXPECT_EQ(TokenSpan(1, 3), CodepointSpanToTokenSpan(tokens, {6, 29}, true));
  EXPECT_EQ(TokenSpan(0, 3), CodepointSpanToTokenSpan(tokens, {0, 29}, true));

  // Span boundaries inside tokens.
  EXPECT_EQ(TokenSpan(1, 2), CodepointSpanToTokenSpan(tokens, {1, 28}));
  EXPECT_EQ(TokenSpan(0, 3), CodepointSpanToTokenSpan(tokens, {1, 28}, true));

  // Tokens adjacent to the span, but not overlapping.
  EXPECT_EQ(TokenSpan(1, 2), CodepointSpanToTokenSpan(tokens, {5, 24}));
  EXPECT_EQ(TokenSpan(1, 2), CodepointSpanToTokenSpan(tokens, {5, 24}, true));
}

}  // namespace
}  // namespace libtextclassifier2
