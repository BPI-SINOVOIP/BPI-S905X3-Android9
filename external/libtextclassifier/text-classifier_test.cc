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

#include "text-classifier.h"

#include <fstream>
#include <iostream>
#include <memory>
#include <string>

#include "model_generated.h"
#include "types-test-util.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace libtextclassifier2 {
namespace {

using testing::ElementsAreArray;
using testing::IsEmpty;
using testing::Pair;
using testing::Values;

std::string FirstResult(const std::vector<ClassificationResult>& results) {
  if (results.empty()) {
    return "<INVALID RESULTS>";
  }
  return results[0].collection;
}

MATCHER_P3(IsAnnotatedSpan, start, end, best_class, "") {
  return testing::Value(arg.span, Pair(start, end)) &&
         testing::Value(FirstResult(arg.classification), best_class);
}

std::string ReadFile(const std::string& file_name) {
  std::ifstream file_stream(file_name);
  return std::string(std::istreambuf_iterator<char>(file_stream), {});
}

std::string GetModelPath() {
  return LIBTEXTCLASSIFIER_TEST_DATA_DIR;
}

TEST(TextClassifierTest, EmbeddingExecutorLoadingFails) {
  CREATE_UNILIB_FOR_TESTING;
  std::unique_ptr<TextClassifier> classifier =
      TextClassifier::FromPath(GetModelPath() + "wrong_embeddings.fb", &unilib);
  EXPECT_FALSE(classifier);
}

class TextClassifierTest : public ::testing::TestWithParam<const char*> {};

INSTANTIATE_TEST_CASE_P(ClickContext, TextClassifierTest,
                        Values("test_model_cc.fb"));
INSTANTIATE_TEST_CASE_P(BoundsSensitive, TextClassifierTest,
                        Values("test_model.fb"));

TEST_P(TextClassifierTest, ClassifyText) {
  CREATE_UNILIB_FOR_TESTING;
  std::unique_ptr<TextClassifier> classifier =
      TextClassifier::FromPath(GetModelPath() + GetParam(), &unilib);
  ASSERT_TRUE(classifier);

  EXPECT_EQ("other",
            FirstResult(classifier->ClassifyText(
                "this afternoon Barack Obama gave a speech at", {15, 27})));
  EXPECT_EQ("phone", FirstResult(classifier->ClassifyText(
                         "Call me at (800) 123-456 today", {11, 24})));

  // More lines.
  EXPECT_EQ("other",
            FirstResult(classifier->ClassifyText(
                "this afternoon Barack Obama gave a speech at|Visit "
                "www.google.com every today!|Call me at (800) 123-456 today.",
                {15, 27})));
  EXPECT_EQ("phone",
            FirstResult(classifier->ClassifyText(
                "this afternoon Barack Obama gave a speech at|Visit "
                "www.google.com every today!|Call me at (800) 123-456 today.",
                {90, 103})));

  // Single word.
  EXPECT_EQ("other", FirstResult(classifier->ClassifyText("obama", {0, 5})));
  EXPECT_EQ("other", FirstResult(classifier->ClassifyText("asdf", {0, 4})));
  EXPECT_EQ("<INVALID RESULTS>",
            FirstResult(classifier->ClassifyText("asdf", {0, 0})));

  // Junk.
  EXPECT_EQ("<INVALID RESULTS>",
            FirstResult(classifier->ClassifyText("", {0, 0})));
  EXPECT_EQ("<INVALID RESULTS>", FirstResult(classifier->ClassifyText(
                                     "a\n\n\n\nx x x\n\n\n\n\n\n", {1, 5})));
  // Test invalid utf8 input.
  EXPECT_EQ("<INVALID RESULTS>", FirstResult(classifier->ClassifyText(
                                     "\xf0\x9f\x98\x8b\x8b", {0, 0})));
}

TEST_P(TextClassifierTest, ClassifyTextDisabledFail) {
  CREATE_UNILIB_FOR_TESTING;
  const std::string test_model = ReadFile(GetModelPath() + GetParam());
  std::unique_ptr<ModelT> unpacked_model = UnPackModel(test_model.c_str());

  unpacked_model->classification_model.clear();
  unpacked_model->triggering_options.reset(new ModelTriggeringOptionsT);
  unpacked_model->triggering_options->enabled_modes = ModeFlag_SELECTION;

  flatbuffers::FlatBufferBuilder builder;
  builder.Finish(Model::Pack(builder, unpacked_model.get()));

  std::unique_ptr<TextClassifier> classifier =
      TextClassifier::FromUnownedBuffer(
          reinterpret_cast<const char*>(builder.GetBufferPointer()),
          builder.GetSize(), &unilib);

  // The classification model is still needed for selection scores.
  ASSERT_FALSE(classifier);
}

TEST_P(TextClassifierTest, ClassifyTextDisabled) {
  CREATE_UNILIB_FOR_TESTING;
  const std::string test_model = ReadFile(GetModelPath() + GetParam());
  std::unique_ptr<ModelT> unpacked_model = UnPackModel(test_model.c_str());

  unpacked_model->triggering_options.reset(new ModelTriggeringOptionsT);
  unpacked_model->triggering_options->enabled_modes =
      ModeFlag_ANNOTATION_AND_SELECTION;

  flatbuffers::FlatBufferBuilder builder;
  builder.Finish(Model::Pack(builder, unpacked_model.get()));

  std::unique_ptr<TextClassifier> classifier =
      TextClassifier::FromUnownedBuffer(
          reinterpret_cast<const char*>(builder.GetBufferPointer()),
          builder.GetSize(), &unilib);
  ASSERT_TRUE(classifier);

  EXPECT_THAT(
      classifier->ClassifyText("Call me at (800) 123-456 today", {11, 24}),
      IsEmpty());
}

TEST_P(TextClassifierTest, ClassifyTextFilteredCollections) {
  CREATE_UNILIB_FOR_TESTING;
  const std::string test_model = ReadFile(GetModelPath() + GetParam());

  std::unique_ptr<TextClassifier> classifier =
      TextClassifier::FromUnownedBuffer(test_model.c_str(), test_model.size(),
                                        &unilib);
  ASSERT_TRUE(classifier);

  EXPECT_EQ("phone", FirstResult(classifier->ClassifyText(
                         "Call me at (800) 123-456 today", {11, 24})));

  std::unique_ptr<ModelT> unpacked_model = UnPackModel(test_model.c_str());
  unpacked_model->output_options.reset(new OutputOptionsT);

  // Disable phone classification
  unpacked_model->output_options->filtered_collections_classification.push_back(
      "phone");

  flatbuffers::FlatBufferBuilder builder;
  builder.Finish(Model::Pack(builder, unpacked_model.get()));

  classifier = TextClassifier::FromUnownedBuffer(
      reinterpret_cast<const char*>(builder.GetBufferPointer()),
      builder.GetSize(), &unilib);
  ASSERT_TRUE(classifier);

  EXPECT_EQ("other", FirstResult(classifier->ClassifyText(
                         "Call me at (800) 123-456 today", {11, 24})));

  // Check that the address classification still passes.
  EXPECT_EQ("address", FirstResult(classifier->ClassifyText(
                           "350 Third Street, Cambridge", {0, 27})));
}

std::unique_ptr<RegexModel_::PatternT> MakePattern(
    const std::string& collection_name, const std::string& pattern,
    const bool enabled_for_classification, const bool enabled_for_selection,
    const bool enabled_for_annotation, const float score) {
  std::unique_ptr<RegexModel_::PatternT> result(new RegexModel_::PatternT);
  result->collection_name = collection_name;
  result->pattern = pattern;
  // We cannot directly operate with |= on the flag, so use an int here.
  int enabled_modes = ModeFlag_NONE;
  if (enabled_for_annotation) enabled_modes |= ModeFlag_ANNOTATION;
  if (enabled_for_classification) enabled_modes |= ModeFlag_CLASSIFICATION;
  if (enabled_for_selection) enabled_modes |= ModeFlag_SELECTION;
  result->enabled_modes = static_cast<ModeFlag>(enabled_modes);
  result->target_classification_score = score;
  result->priority_score = score;
  return result;
}

#ifdef LIBTEXTCLASSIFIER_UNILIB_ICU
TEST_P(TextClassifierTest, ClassifyTextRegularExpression) {
  CREATE_UNILIB_FOR_TESTING;
  const std::string test_model = ReadFile(GetModelPath() + GetParam());
  std::unique_ptr<ModelT> unpacked_model = UnPackModel(test_model.c_str());

  // Add test regex models.
  unpacked_model->regex_model->patterns.push_back(MakePattern(
      "person", "Barack Obama", /*enabled_for_classification=*/true,
      /*enabled_for_selection=*/false, /*enabled_for_annotation=*/false, 1.0));
  unpacked_model->regex_model->patterns.push_back(MakePattern(
      "flight", "[a-zA-Z]{2}\\d{2,4}", /*enabled_for_classification=*/true,
      /*enabled_for_selection=*/false, /*enabled_for_annotation=*/false, 0.5));

  flatbuffers::FlatBufferBuilder builder;
  builder.Finish(Model::Pack(builder, unpacked_model.get()));

  std::unique_ptr<TextClassifier> classifier =
      TextClassifier::FromUnownedBuffer(
          reinterpret_cast<const char*>(builder.GetBufferPointer()),
          builder.GetSize(), &unilib);
  ASSERT_TRUE(classifier);

  EXPECT_EQ("flight",
            FirstResult(classifier->ClassifyText(
                "Your flight LX373 is delayed by 3 hours.", {12, 17})));
  EXPECT_EQ("person",
            FirstResult(classifier->ClassifyText(
                "this afternoon Barack Obama gave a speech at", {15, 27})));
  EXPECT_EQ("email",
            FirstResult(classifier->ClassifyText("you@android.com", {0, 15})));
  EXPECT_EQ("email", FirstResult(classifier->ClassifyText(
                         "Contact me at you@android.com", {14, 29})));

  EXPECT_EQ("url", FirstResult(classifier->ClassifyText(
                       "Visit www.google.com every today!", {6, 20})));

  EXPECT_EQ("flight", FirstResult(classifier->ClassifyText("LX 37", {0, 5})));
  EXPECT_EQ("flight", FirstResult(classifier->ClassifyText("flight LX 37 abcd",
                                                           {7, 12})));

  // More lines.
  EXPECT_EQ("url",
            FirstResult(classifier->ClassifyText(
                "this afternoon Barack Obama gave a speech at|Visit "
                "www.google.com every today!|Call me at (800) 123-456 today.",
                {51, 65})));
}
#endif  // LIBTEXTCLASSIFIER_UNILIB_ICU

#ifdef LIBTEXTCLASSIFIER_UNILIB_ICU
TEST_P(TextClassifierTest, SuggestSelectionRegularExpression) {
  CREATE_UNILIB_FOR_TESTING;
  const std::string test_model = ReadFile(GetModelPath() + GetParam());
  std::unique_ptr<ModelT> unpacked_model = UnPackModel(test_model.c_str());

  // Add test regex models.
  unpacked_model->regex_model->patterns.push_back(MakePattern(
      "person", " (Barack Obama) ", /*enabled_for_classification=*/false,
      /*enabled_for_selection=*/true, /*enabled_for_annotation=*/false, 1.0));
  unpacked_model->regex_model->patterns.push_back(MakePattern(
      "flight", "([a-zA-Z]{2} ?\\d{2,4})", /*enabled_for_classification=*/false,
      /*enabled_for_selection=*/true, /*enabled_for_annotation=*/false, 1.0));
  unpacked_model->regex_model->patterns.back()->priority_score = 1.1;

  flatbuffers::FlatBufferBuilder builder;
  builder.Finish(Model::Pack(builder, unpacked_model.get()));

  std::unique_ptr<TextClassifier> classifier =
      TextClassifier::FromUnownedBuffer(
          reinterpret_cast<const char*>(builder.GetBufferPointer()),
          builder.GetSize(), &unilib);
  ASSERT_TRUE(classifier);

  // Check regular expression selection.
  EXPECT_EQ(classifier->SuggestSelection(
                "Your flight MA 0123 is delayed by 3 hours.", {12, 14}),
            std::make_pair(12, 19));
  EXPECT_EQ(classifier->SuggestSelection(
                "this afternoon Barack Obama gave a speech at", {15, 21}),
            std::make_pair(15, 27));
}
#endif  // LIBTEXTCLASSIFIER_UNILIB_ICU

#ifdef LIBTEXTCLASSIFIER_UNILIB_ICU
TEST_P(TextClassifierTest,
       SuggestSelectionRegularExpressionConflictsModelWins) {
  const std::string test_model = ReadFile(GetModelPath() + GetParam());
  std::unique_ptr<ModelT> unpacked_model = UnPackModel(test_model.c_str());

  // Add test regex models.
  unpacked_model->regex_model->patterns.push_back(MakePattern(
      "person", " (Barack Obama) ", /*enabled_for_classification=*/false,
      /*enabled_for_selection=*/true, /*enabled_for_annotation=*/false, 1.0));
  unpacked_model->regex_model->patterns.push_back(MakePattern(
      "flight", "([a-zA-Z]{2} ?\\d{2,4})", /*enabled_for_classification=*/false,
      /*enabled_for_selection=*/true, /*enabled_for_annotation=*/false, 1.0));
  unpacked_model->regex_model->patterns.back()->priority_score = 0.5;

  flatbuffers::FlatBufferBuilder builder;
  builder.Finish(Model::Pack(builder, unpacked_model.get()));

  std::unique_ptr<TextClassifier> classifier =
      TextClassifier::FromUnownedBuffer(
          reinterpret_cast<const char*>(builder.GetBufferPointer()),
          builder.GetSize());
  ASSERT_TRUE(classifier);

  // Check conflict resolution.
  EXPECT_EQ(
      classifier->SuggestSelection(
          "saw Barack Obama today .. 350 Third Street, Cambridge, MA 0123",
          {55, 57}),
      std::make_pair(26, 62));
}
#endif  // LIBTEXTCLASSIFIER_UNILIB_ICU

#ifdef LIBTEXTCLASSIFIER_UNILIB_ICU
TEST_P(TextClassifierTest,
       SuggestSelectionRegularExpressionConflictsRegexWins) {
  const std::string test_model = ReadFile(GetModelPath() + GetParam());
  std::unique_ptr<ModelT> unpacked_model = UnPackModel(test_model.c_str());

  // Add test regex models.
  unpacked_model->regex_model->patterns.push_back(MakePattern(
      "person", " (Barack Obama) ", /*enabled_for_classification=*/false,
      /*enabled_for_selection=*/true, /*enabled_for_annotation=*/false, 1.0));
  unpacked_model->regex_model->patterns.push_back(MakePattern(
      "flight", "([a-zA-Z]{2} ?\\d{2,4})", /*enabled_for_classification=*/false,
      /*enabled_for_selection=*/true, /*enabled_for_annotation=*/false, 1.0));
  unpacked_model->regex_model->patterns.back()->priority_score = 1.1;

  flatbuffers::FlatBufferBuilder builder;
  builder.Finish(Model::Pack(builder, unpacked_model.get()));

  std::unique_ptr<TextClassifier> classifier =
      TextClassifier::FromUnownedBuffer(
          reinterpret_cast<const char*>(builder.GetBufferPointer()),
          builder.GetSize());
  ASSERT_TRUE(classifier);

  // Check conflict resolution.
  EXPECT_EQ(
      classifier->SuggestSelection(
          "saw Barack Obama today .. 350 Third Street, Cambridge, MA 0123",
          {55, 57}),
      std::make_pair(55, 62));
}
#endif  // LIBTEXTCLASSIFIER_UNILIB_ICU

#ifdef LIBTEXTCLASSIFIER_UNILIB_ICU
TEST_P(TextClassifierTest, AnnotateRegex) {
  CREATE_UNILIB_FOR_TESTING;
  const std::string test_model = ReadFile(GetModelPath() + GetParam());
  std::unique_ptr<ModelT> unpacked_model = UnPackModel(test_model.c_str());

  // Add test regex models.
  unpacked_model->regex_model->patterns.push_back(MakePattern(
      "person", " (Barack Obama) ", /*enabled_for_classification=*/false,
      /*enabled_for_selection=*/false, /*enabled_for_annotation=*/true, 1.0));
  unpacked_model->regex_model->patterns.push_back(MakePattern(
      "flight", "([a-zA-Z]{2} ?\\d{2,4})", /*enabled_for_classification=*/false,
      /*enabled_for_selection=*/false, /*enabled_for_annotation=*/true, 0.5));
  flatbuffers::FlatBufferBuilder builder;
  builder.Finish(Model::Pack(builder, unpacked_model.get()));

  std::unique_ptr<TextClassifier> classifier =
      TextClassifier::FromUnownedBuffer(
          reinterpret_cast<const char*>(builder.GetBufferPointer()),
          builder.GetSize(), &unilib);
  ASSERT_TRUE(classifier);

  const std::string test_string =
      "& saw Barack Obama today .. 350 Third Street, Cambridge\nand my phone "
      "number is 853 225 3556";
  EXPECT_THAT(classifier->Annotate(test_string),
              ElementsAreArray({
                  IsAnnotatedSpan(6, 18, "person"),
                  IsAnnotatedSpan(19, 24, "date"),
                  IsAnnotatedSpan(28, 55, "address"),
                  IsAnnotatedSpan(79, 91, "phone"),
              }));
}
#endif  // LIBTEXTCLASSIFIER_UNILIB_ICU

TEST_P(TextClassifierTest, PhoneFiltering) {
  CREATE_UNILIB_FOR_TESTING;
  std::unique_ptr<TextClassifier> classifier =
      TextClassifier::FromPath(GetModelPath() + GetParam(), &unilib);
  ASSERT_TRUE(classifier);

  EXPECT_EQ("phone", FirstResult(classifier->ClassifyText(
                         "phone: (123) 456 789", {7, 20})));
  EXPECT_EQ("phone", FirstResult(classifier->ClassifyText(
                         "phone: (123) 456 789,0001112", {7, 25})));
  EXPECT_EQ("other", FirstResult(classifier->ClassifyText(
                         "phone: (123) 456 789,0001112", {7, 28})));
}

TEST_P(TextClassifierTest, SuggestSelection) {
  CREATE_UNILIB_FOR_TESTING;
  std::unique_ptr<TextClassifier> classifier =
      TextClassifier::FromPath(GetModelPath() + GetParam(), &unilib);
  ASSERT_TRUE(classifier);

  EXPECT_EQ(classifier->SuggestSelection(
                "this afternoon Barack Obama gave a speech at", {15, 21}),
            std::make_pair(15, 21));

  // Try passing whole string.
  // If more than 1 token is specified, we should return back what entered.
  EXPECT_EQ(
      classifier->SuggestSelection("350 Third Street, Cambridge", {0, 27}),
      std::make_pair(0, 27));

  // Single letter.
  EXPECT_EQ(classifier->SuggestSelection("a", {0, 1}), std::make_pair(0, 1));

  // Single word.
  EXPECT_EQ(classifier->SuggestSelection("asdf", {0, 4}), std::make_pair(0, 4));

  EXPECT_EQ(
      classifier->SuggestSelection("call me at 857 225 3556 today", {11, 14}),
      std::make_pair(11, 23));

  // Unpaired bracket stripping.
  EXPECT_EQ(
      classifier->SuggestSelection("call me at (857) 225 3556 today", {11, 16}),
      std::make_pair(11, 25));
  EXPECT_EQ(classifier->SuggestSelection("call me at (857 today", {11, 15}),
            std::make_pair(12, 15));
  EXPECT_EQ(classifier->SuggestSelection("call me at 3556) today", {11, 16}),
            std::make_pair(11, 15));
  EXPECT_EQ(classifier->SuggestSelection("call me at )857( today", {11, 16}),
            std::make_pair(12, 15));

  // If the resulting selection would be empty, the original span is returned.
  EXPECT_EQ(classifier->SuggestSelection("call me at )( today", {11, 13}),
            std::make_pair(11, 13));
  EXPECT_EQ(classifier->SuggestSelection("call me at ( today", {11, 12}),
            std::make_pair(11, 12));
  EXPECT_EQ(classifier->SuggestSelection("call me at ) today", {11, 12}),
            std::make_pair(11, 12));
}

TEST_P(TextClassifierTest, SuggestSelectionDisabledFail) {
  CREATE_UNILIB_FOR_TESTING;
  const std::string test_model = ReadFile(GetModelPath() + GetParam());
  std::unique_ptr<ModelT> unpacked_model = UnPackModel(test_model.c_str());

  // Disable the selection model.
  unpacked_model->selection_model.clear();
  unpacked_model->triggering_options.reset(new ModelTriggeringOptionsT);
  unpacked_model->triggering_options->enabled_modes = ModeFlag_ANNOTATION;

  flatbuffers::FlatBufferBuilder builder;
  builder.Finish(Model::Pack(builder, unpacked_model.get()));

  std::unique_ptr<TextClassifier> classifier =
      TextClassifier::FromUnownedBuffer(
          reinterpret_cast<const char*>(builder.GetBufferPointer()),
          builder.GetSize(), &unilib);
  // Selection model needs to be present for annotation.
  ASSERT_FALSE(classifier);
}

TEST_P(TextClassifierTest, SuggestSelectionDisabled) {
  CREATE_UNILIB_FOR_TESTING;
  const std::string test_model = ReadFile(GetModelPath() + GetParam());
  std::unique_ptr<ModelT> unpacked_model = UnPackModel(test_model.c_str());

  // Disable the selection model.
  unpacked_model->selection_model.clear();
  unpacked_model->triggering_options.reset(new ModelTriggeringOptionsT);
  unpacked_model->triggering_options->enabled_modes = ModeFlag_CLASSIFICATION;
  unpacked_model->enabled_modes = ModeFlag_CLASSIFICATION;

  flatbuffers::FlatBufferBuilder builder;
  builder.Finish(Model::Pack(builder, unpacked_model.get()));

  std::unique_ptr<TextClassifier> classifier =
      TextClassifier::FromUnownedBuffer(
          reinterpret_cast<const char*>(builder.GetBufferPointer()),
          builder.GetSize(), &unilib);
  ASSERT_TRUE(classifier);

  EXPECT_EQ(
      classifier->SuggestSelection("call me at 857 225 3556 today", {11, 14}),
      std::make_pair(11, 14));

  EXPECT_EQ("phone", FirstResult(classifier->ClassifyText(
                         "call me at (800) 123-456 today", {11, 24})));

  EXPECT_THAT(classifier->Annotate("call me at (800) 123-456 today"),
              IsEmpty());
}

TEST_P(TextClassifierTest, SuggestSelectionFilteredCollections) {
  CREATE_UNILIB_FOR_TESTING;
  const std::string test_model = ReadFile(GetModelPath() + GetParam());

  std::unique_ptr<TextClassifier> classifier =
      TextClassifier::FromUnownedBuffer(test_model.c_str(), test_model.size(),
                                        &unilib);
  ASSERT_TRUE(classifier);

  EXPECT_EQ(
      classifier->SuggestSelection("call me at 857 225 3556 today", {11, 14}),
      std::make_pair(11, 23));

  std::unique_ptr<ModelT> unpacked_model = UnPackModel(test_model.c_str());
  unpacked_model->output_options.reset(new OutputOptionsT);

  // Disable phone selection
  unpacked_model->output_options->filtered_collections_selection.push_back(
      "phone");
  // We need to force this for filtering.
  unpacked_model->selection_options->always_classify_suggested_selection = true;

  flatbuffers::FlatBufferBuilder builder;
  builder.Finish(Model::Pack(builder, unpacked_model.get()));

  classifier = TextClassifier::FromUnownedBuffer(
      reinterpret_cast<const char*>(builder.GetBufferPointer()),
      builder.GetSize(), &unilib);
  ASSERT_TRUE(classifier);

  EXPECT_EQ(
      classifier->SuggestSelection("call me at 857 225 3556 today", {11, 14}),
      std::make_pair(11, 14));

  // Address selection should still work.
  EXPECT_EQ(classifier->SuggestSelection("350 Third Street, Cambridge", {4, 9}),
            std::make_pair(0, 27));
}

TEST_P(TextClassifierTest, SuggestSelectionsAreSymmetric) {
  CREATE_UNILIB_FOR_TESTING;
  std::unique_ptr<TextClassifier> classifier =
      TextClassifier::FromPath(GetModelPath() + GetParam(), &unilib);
  ASSERT_TRUE(classifier);

  EXPECT_EQ(classifier->SuggestSelection("350 Third Street, Cambridge", {0, 3}),
            std::make_pair(0, 27));
  EXPECT_EQ(classifier->SuggestSelection("350 Third Street, Cambridge", {4, 9}),
            std::make_pair(0, 27));
  EXPECT_EQ(
      classifier->SuggestSelection("350 Third Street, Cambridge", {10, 16}),
      std::make_pair(0, 27));
  EXPECT_EQ(classifier->SuggestSelection("a\nb\nc\n350 Third Street, Cambridge",
                                         {16, 22}),
            std::make_pair(6, 33));
}

TEST_P(TextClassifierTest, SuggestSelectionWithNewLine) {
  CREATE_UNILIB_FOR_TESTING;
  std::unique_ptr<TextClassifier> classifier =
      TextClassifier::FromPath(GetModelPath() + GetParam(), &unilib);
  ASSERT_TRUE(classifier);

  EXPECT_EQ(classifier->SuggestSelection("abc\n857 225 3556", {4, 7}),
            std::make_pair(4, 16));
  EXPECT_EQ(classifier->SuggestSelection("857 225 3556\nabc", {0, 3}),
            std::make_pair(0, 12));

  SelectionOptions options;
  EXPECT_EQ(classifier->SuggestSelection("857 225\n3556\nabc", {0, 3}, options),
            std::make_pair(0, 7));
}

TEST_P(TextClassifierTest, SuggestSelectionWithPunctuation) {
  CREATE_UNILIB_FOR_TESTING;
  std::unique_ptr<TextClassifier> classifier =
      TextClassifier::FromPath(GetModelPath() + GetParam(), &unilib);
  ASSERT_TRUE(classifier);

  // From the right.
  EXPECT_EQ(classifier->SuggestSelection(
                "this afternoon BarackObama, gave a speech at", {15, 26}),
            std::make_pair(15, 26));

  // From the right multiple.
  EXPECT_EQ(classifier->SuggestSelection(
                "this afternoon BarackObama,.,.,, gave a speech at", {15, 26}),
            std::make_pair(15, 26));

  // From the left multiple.
  EXPECT_EQ(classifier->SuggestSelection(
                "this afternoon ,.,.,,BarackObama gave a speech at", {21, 32}),
            std::make_pair(21, 32));

  // From both sides.
  EXPECT_EQ(classifier->SuggestSelection(
                "this afternoon !BarackObama,- gave a speech at", {16, 27}),
            std::make_pair(16, 27));
}

TEST_P(TextClassifierTest, SuggestSelectionNoCrashWithJunk) {
  CREATE_UNILIB_FOR_TESTING;
  std::unique_ptr<TextClassifier> classifier =
      TextClassifier::FromPath(GetModelPath() + GetParam(), &unilib);
  ASSERT_TRUE(classifier);

  // Try passing in bunch of invalid selections.
  EXPECT_EQ(classifier->SuggestSelection("", {0, 27}), std::make_pair(0, 27));
  EXPECT_EQ(classifier->SuggestSelection("", {-10, 27}),
            std::make_pair(-10, 27));
  EXPECT_EQ(classifier->SuggestSelection("Word 1 2 3 hello!", {0, 27}),
            std::make_pair(0, 27));
  EXPECT_EQ(classifier->SuggestSelection("Word 1 2 3 hello!", {-30, 300}),
            std::make_pair(-30, 300));
  EXPECT_EQ(classifier->SuggestSelection("Word 1 2 3 hello!", {-10, -1}),
            std::make_pair(-10, -1));
  EXPECT_EQ(classifier->SuggestSelection("Word 1 2 3 hello!", {100, 17}),
            std::make_pair(100, 17));

  // Try passing invalid utf8.
  EXPECT_EQ(classifier->SuggestSelection("\xf0\x9f\x98\x8b\x8b", {-1, -1}),
            std::make_pair(-1, -1));
}

TEST_P(TextClassifierTest, SuggestSelectionSelectSpace) {
  CREATE_UNILIB_FOR_TESTING;
  std::unique_ptr<TextClassifier> classifier =
      TextClassifier::FromPath(GetModelPath() + GetParam(), &unilib);
  ASSERT_TRUE(classifier);

  EXPECT_EQ(
      classifier->SuggestSelection("call me at 857 225 3556 today", {14, 15}),
      std::make_pair(11, 23));
  EXPECT_EQ(
      classifier->SuggestSelection("call me at 857 225 3556 today", {10, 11}),
      std::make_pair(10, 11));
  EXPECT_EQ(
      classifier->SuggestSelection("call me at 857 225 3556 today", {23, 24}),
      std::make_pair(23, 24));
  EXPECT_EQ(
      classifier->SuggestSelection("call me at 857 225 3556, today", {23, 24}),
      std::make_pair(23, 24));
  EXPECT_EQ(classifier->SuggestSelection("call me at 857   225 3556, today",
                                         {14, 17}),
            std::make_pair(11, 25));
  EXPECT_EQ(
      classifier->SuggestSelection("call me at 857-225 3556, today", {14, 17}),
      std::make_pair(11, 23));
  EXPECT_EQ(
      classifier->SuggestSelection(
          "let's meet at 350 Third Street Cambridge and go there", {30, 31}),
      std::make_pair(14, 40));
  EXPECT_EQ(classifier->SuggestSelection("call me today", {4, 5}),
            std::make_pair(4, 5));
  EXPECT_EQ(classifier->SuggestSelection("call me today", {7, 8}),
            std::make_pair(7, 8));

  // With a punctuation around the selected whitespace.
  EXPECT_EQ(
      classifier->SuggestSelection(
          "let's meet at 350 Third Street, Cambridge and go there", {31, 32}),
      std::make_pair(14, 41));

  // When all's whitespace, should return the original indices.
  EXPECT_EQ(classifier->SuggestSelection("      ", {0, 1}),
            std::make_pair(0, 1));
  EXPECT_EQ(classifier->SuggestSelection("      ", {0, 3}),
            std::make_pair(0, 3));
  EXPECT_EQ(classifier->SuggestSelection("      ", {2, 3}),
            std::make_pair(2, 3));
  EXPECT_EQ(classifier->SuggestSelection("      ", {5, 6}),
            std::make_pair(5, 6));
}

TEST(TextClassifierTest, SnapLeftIfWhitespaceSelection) {
  CREATE_UNILIB_FOR_TESTING;
  UnicodeText text;

  text = UTF8ToUnicodeText("abcd efgh", /*do_copy=*/false);
  EXPECT_EQ(internal::SnapLeftIfWhitespaceSelection({4, 5}, text, unilib),
            std::make_pair(3, 4));
  text = UTF8ToUnicodeText("abcd     ", /*do_copy=*/false);
  EXPECT_EQ(internal::SnapLeftIfWhitespaceSelection({4, 5}, text, unilib),
            std::make_pair(3, 4));

  // Nothing on the left.
  text = UTF8ToUnicodeText("     efgh", /*do_copy=*/false);
  EXPECT_EQ(internal::SnapLeftIfWhitespaceSelection({4, 5}, text, unilib),
            std::make_pair(4, 5));
  text = UTF8ToUnicodeText("     efgh", /*do_copy=*/false);
  EXPECT_EQ(internal::SnapLeftIfWhitespaceSelection({0, 1}, text, unilib),
            std::make_pair(0, 1));

  // Whitespace only.
  text = UTF8ToUnicodeText("     ", /*do_copy=*/false);
  EXPECT_EQ(internal::SnapLeftIfWhitespaceSelection({2, 3}, text, unilib),
            std::make_pair(2, 3));
  text = UTF8ToUnicodeText("     ", /*do_copy=*/false);
  EXPECT_EQ(internal::SnapLeftIfWhitespaceSelection({4, 5}, text, unilib),
            std::make_pair(4, 5));
  text = UTF8ToUnicodeText("     ", /*do_copy=*/false);
  EXPECT_EQ(internal::SnapLeftIfWhitespaceSelection({0, 1}, text, unilib),
            std::make_pair(0, 1));
}

TEST_P(TextClassifierTest, Annotate) {
  CREATE_UNILIB_FOR_TESTING;
  std::unique_ptr<TextClassifier> classifier =
      TextClassifier::FromPath(GetModelPath() + GetParam(), &unilib);
  ASSERT_TRUE(classifier);

  const std::string test_string =
      "& saw Barack Obama today .. 350 Third Street, Cambridge\nand my phone "
      "number is 853 225 3556";
  EXPECT_THAT(classifier->Annotate(test_string),
              ElementsAreArray({
#ifdef LIBTEXTCLASSIFIER_UNILIB_ICU
                  IsAnnotatedSpan(19, 24, "date"),
#endif
                  IsAnnotatedSpan(28, 55, "address"),
                  IsAnnotatedSpan(79, 91, "phone"),
              }));

  AnnotationOptions options;
  EXPECT_THAT(classifier->Annotate("853 225 3556", options),
              ElementsAreArray({IsAnnotatedSpan(0, 12, "phone")}));
  EXPECT_TRUE(classifier->Annotate("853 225\n3556", options).empty());

  // Try passing invalid utf8.
  EXPECT_TRUE(
      classifier->Annotate("853 225 3556\n\xf0\x9f\x98\x8b\x8b", options)
          .empty());
}

TEST_P(TextClassifierTest, AnnotateSmallBatches) {
  CREATE_UNILIB_FOR_TESTING;
  const std::string test_model = ReadFile(GetModelPath() + GetParam());
  std::unique_ptr<ModelT> unpacked_model = UnPackModel(test_model.c_str());

  // Set the batch size.
  unpacked_model->selection_options->batch_size = 4;
  flatbuffers::FlatBufferBuilder builder;
  builder.Finish(Model::Pack(builder, unpacked_model.get()));

  std::unique_ptr<TextClassifier> classifier =
      TextClassifier::FromUnownedBuffer(
          reinterpret_cast<const char*>(builder.GetBufferPointer()),
          builder.GetSize(), &unilib);
  ASSERT_TRUE(classifier);

  const std::string test_string =
      "& saw Barack Obama today .. 350 Third Street, Cambridge\nand my phone "
      "number is 853 225 3556";
  EXPECT_THAT(classifier->Annotate(test_string),
              ElementsAreArray({
#ifdef LIBTEXTCLASSIFIER_UNILIB_ICU
                  IsAnnotatedSpan(19, 24, "date"),
#endif
                  IsAnnotatedSpan(28, 55, "address"),
                  IsAnnotatedSpan(79, 91, "phone"),
              }));

  AnnotationOptions options;
  EXPECT_THAT(classifier->Annotate("853 225 3556", options),
              ElementsAreArray({IsAnnotatedSpan(0, 12, "phone")}));
  EXPECT_TRUE(classifier->Annotate("853 225\n3556", options).empty());
}

#ifdef LIBTEXTCLASSIFIER_UNILIB_ICU
TEST_P(TextClassifierTest, AnnotateFilteringDiscardAll) {
  CREATE_UNILIB_FOR_TESTING;
  const std::string test_model = ReadFile(GetModelPath() + GetParam());
  std::unique_ptr<ModelT> unpacked_model = UnPackModel(test_model.c_str());

  unpacked_model->triggering_options.reset(new ModelTriggeringOptionsT);
  // Add test threshold.
  unpacked_model->triggering_options->min_annotate_confidence =
      2.f;  // Discards all results.
  flatbuffers::FlatBufferBuilder builder;
  builder.Finish(Model::Pack(builder, unpacked_model.get()));

  std::unique_ptr<TextClassifier> classifier =
      TextClassifier::FromUnownedBuffer(
          reinterpret_cast<const char*>(builder.GetBufferPointer()),
          builder.GetSize(), &unilib);
  ASSERT_TRUE(classifier);

  const std::string test_string =
      "& saw Barack Obama today .. 350 Third Street, Cambridge\nand my phone "
      "number is 853 225 3556";

  EXPECT_EQ(classifier->Annotate(test_string).size(), 1);
}
#endif

TEST_P(TextClassifierTest, AnnotateFilteringKeepAll) {
  CREATE_UNILIB_FOR_TESTING;
  const std::string test_model = ReadFile(GetModelPath() + GetParam());
  std::unique_ptr<ModelT> unpacked_model = UnPackModel(test_model.c_str());

  // Add test thresholds.
  unpacked_model->triggering_options.reset(new ModelTriggeringOptionsT);
  unpacked_model->triggering_options->min_annotate_confidence =
      0.f;  // Keeps all results.
  unpacked_model->triggering_options->enabled_modes = ModeFlag_ALL;
  flatbuffers::FlatBufferBuilder builder;
  builder.Finish(Model::Pack(builder, unpacked_model.get()));

  std::unique_ptr<TextClassifier> classifier =
      TextClassifier::FromUnownedBuffer(
          reinterpret_cast<const char*>(builder.GetBufferPointer()),
          builder.GetSize(), &unilib);
  ASSERT_TRUE(classifier);

  const std::string test_string =
      "& saw Barack Obama today .. 350 Third Street, Cambridge\nand my phone "
      "number is 853 225 3556";
#ifdef LIBTEXTCLASSIFIER_UNILIB_ICU
  EXPECT_EQ(classifier->Annotate(test_string).size(), 3);
#else
  // In non-ICU mode there is no "date" result.
  EXPECT_EQ(classifier->Annotate(test_string).size(), 2);
#endif
}

TEST_P(TextClassifierTest, AnnotateDisabled) {
  CREATE_UNILIB_FOR_TESTING;
  const std::string test_model = ReadFile(GetModelPath() + GetParam());
  std::unique_ptr<ModelT> unpacked_model = UnPackModel(test_model.c_str());

  // Disable the model for annotation.
  unpacked_model->enabled_modes = ModeFlag_CLASSIFICATION_AND_SELECTION;
  flatbuffers::FlatBufferBuilder builder;
  builder.Finish(Model::Pack(builder, unpacked_model.get()));

  std::unique_ptr<TextClassifier> classifier =
      TextClassifier::FromUnownedBuffer(
          reinterpret_cast<const char*>(builder.GetBufferPointer()),
          builder.GetSize(), &unilib);
  ASSERT_TRUE(classifier);
  const std::string test_string =
      "& saw Barack Obama today .. 350 Third Street, Cambridge\nand my phone "
      "number is 853 225 3556";
  EXPECT_THAT(classifier->Annotate(test_string), IsEmpty());
}

TEST_P(TextClassifierTest, AnnotateFilteredCollections) {
  CREATE_UNILIB_FOR_TESTING;
  const std::string test_model = ReadFile(GetModelPath() + GetParam());

  std::unique_ptr<TextClassifier> classifier =
      TextClassifier::FromUnownedBuffer(test_model.c_str(), test_model.size(),
                                        &unilib);
  ASSERT_TRUE(classifier);

  const std::string test_string =
      "& saw Barack Obama today .. 350 Third Street, Cambridge\nand my phone "
      "number is 853 225 3556";

  EXPECT_THAT(classifier->Annotate(test_string),
              ElementsAreArray({
#ifdef LIBTEXTCLASSIFIER_UNILIB_ICU
                  IsAnnotatedSpan(19, 24, "date"),
#endif
                  IsAnnotatedSpan(28, 55, "address"),
                  IsAnnotatedSpan(79, 91, "phone"),
              }));

  std::unique_ptr<ModelT> unpacked_model = UnPackModel(test_model.c_str());
  unpacked_model->output_options.reset(new OutputOptionsT);

  // Disable phone annotation
  unpacked_model->output_options->filtered_collections_annotation.push_back(
      "phone");

  flatbuffers::FlatBufferBuilder builder;
  builder.Finish(Model::Pack(builder, unpacked_model.get()));

  classifier = TextClassifier::FromUnownedBuffer(
      reinterpret_cast<const char*>(builder.GetBufferPointer()),
      builder.GetSize(), &unilib);
  ASSERT_TRUE(classifier);

  EXPECT_THAT(classifier->Annotate(test_string),
              ElementsAreArray({
#ifdef LIBTEXTCLASSIFIER_UNILIB_ICU
                  IsAnnotatedSpan(19, 24, "date"),
#endif
                  IsAnnotatedSpan(28, 55, "address"),
              }));
}

#ifdef LIBTEXTCLASSIFIER_UNILIB_ICU
TEST_P(TextClassifierTest, AnnotateFilteredCollectionsSuppress) {
  CREATE_UNILIB_FOR_TESTING;
  const std::string test_model = ReadFile(GetModelPath() + GetParam());

  std::unique_ptr<TextClassifier> classifier =
      TextClassifier::FromUnownedBuffer(test_model.c_str(), test_model.size(),
                                        &unilib);
  ASSERT_TRUE(classifier);

  const std::string test_string =
      "& saw Barack Obama today .. 350 Third Street, Cambridge\nand my phone "
      "number is 853 225 3556";

  EXPECT_THAT(classifier->Annotate(test_string),
              ElementsAreArray({
#ifdef LIBTEXTCLASSIFIER_UNILIB_ICU
                  IsAnnotatedSpan(19, 24, "date"),
#endif
                  IsAnnotatedSpan(28, 55, "address"),
                  IsAnnotatedSpan(79, 91, "phone"),
              }));

  std::unique_ptr<ModelT> unpacked_model = UnPackModel(test_model.c_str());
  unpacked_model->output_options.reset(new OutputOptionsT);

  // We add a custom annotator that wins against the phone classification
  // below and that we subsequently suppress.
  unpacked_model->output_options->filtered_collections_annotation.push_back(
      "suppress");

  unpacked_model->regex_model->patterns.push_back(MakePattern(
      "suppress", "(\\d{3} ?\\d{4})",
      /*enabled_for_classification=*/false,
      /*enabled_for_selection=*/false, /*enabled_for_annotation=*/true, 2.0));

  flatbuffers::FlatBufferBuilder builder;
  builder.Finish(Model::Pack(builder, unpacked_model.get()));

  classifier = TextClassifier::FromUnownedBuffer(
      reinterpret_cast<const char*>(builder.GetBufferPointer()),
      builder.GetSize(), &unilib);
  ASSERT_TRUE(classifier);

  EXPECT_THAT(classifier->Annotate(test_string),
              ElementsAreArray({
                  IsAnnotatedSpan(19, 24, "date"),
                  IsAnnotatedSpan(28, 55, "address"),
              }));
}
#endif

#ifdef LIBTEXTCLASSIFIER_CALENDAR_ICU
TEST_P(TextClassifierTest, ClassifyTextDate) {
  std::unique_ptr<TextClassifier> classifier =
      TextClassifier::FromPath(GetModelPath() + GetParam());
  EXPECT_TRUE(classifier);

  std::vector<ClassificationResult> result;
  ClassificationOptions options;

  options.reference_timezone = "Europe/Zurich";
  result = classifier->ClassifyText("january 1, 2017", {0, 15}, options);

  ASSERT_EQ(result.size(), 1);
  EXPECT_THAT(result[0].collection, "date");
  EXPECT_EQ(result[0].datetime_parse_result.time_ms_utc, 1483225200000);
  EXPECT_EQ(result[0].datetime_parse_result.granularity,
            DatetimeGranularity::GRANULARITY_DAY);
  result.clear();

  options.reference_timezone = "America/Los_Angeles";
  result = classifier->ClassifyText("march 1, 2017", {0, 13}, options);
  ASSERT_EQ(result.size(), 1);
  EXPECT_THAT(result[0].collection, "date");
  EXPECT_EQ(result[0].datetime_parse_result.time_ms_utc, 1488355200000);
  EXPECT_EQ(result[0].datetime_parse_result.granularity,
            DatetimeGranularity::GRANULARITY_DAY);
  result.clear();

  options.reference_timezone = "America/Los_Angeles";
  result = classifier->ClassifyText("2018/01/01 10:30:20", {0, 19}, options);
  ASSERT_EQ(result.size(), 1);
  EXPECT_THAT(result[0].collection, "date");
  EXPECT_EQ(result[0].datetime_parse_result.time_ms_utc, 1514831420000);
  EXPECT_EQ(result[0].datetime_parse_result.granularity,
            DatetimeGranularity::GRANULARITY_SECOND);
  result.clear();

  // Date on another line.
  options.reference_timezone = "Europe/Zurich";
  result = classifier->ClassifyText(
      "hello world this is the first line\n"
      "january 1, 2017",
      {35, 50}, options);
  ASSERT_EQ(result.size(), 1);
  EXPECT_THAT(result[0].collection, "date");
  EXPECT_EQ(result[0].datetime_parse_result.time_ms_utc, 1483225200000);
  EXPECT_EQ(result[0].datetime_parse_result.granularity,
            DatetimeGranularity::GRANULARITY_DAY);
}
#endif  // LIBTEXTCLASSIFIER_UNILIB_ICU

#ifdef LIBTEXTCLASSIFIER_CALENDAR_ICU
TEST_P(TextClassifierTest, ClassifyTextDatePriorities) {
  std::unique_ptr<TextClassifier> classifier =
      TextClassifier::FromPath(GetModelPath() + GetParam());
  EXPECT_TRUE(classifier);

  std::vector<ClassificationResult> result;
  ClassificationOptions options;

  result.clear();
  options.reference_timezone = "Europe/Zurich";
  options.locales = "en-US";
  result = classifier->ClassifyText("03.05.1970", {0, 10}, options);

  ASSERT_EQ(result.size(), 1);
  EXPECT_THAT(result[0].collection, "date");
  EXPECT_EQ(result[0].datetime_parse_result.time_ms_utc, 5439600000);
  EXPECT_EQ(result[0].datetime_parse_result.granularity,
            DatetimeGranularity::GRANULARITY_DAY);

  result.clear();
  options.reference_timezone = "Europe/Zurich";
  options.locales = "de";
  result = classifier->ClassifyText("03.05.1970", {0, 10}, options);

  ASSERT_EQ(result.size(), 1);
  EXPECT_THAT(result[0].collection, "date");
  EXPECT_EQ(result[0].datetime_parse_result.time_ms_utc, 10537200000);
  EXPECT_EQ(result[0].datetime_parse_result.granularity,
            DatetimeGranularity::GRANULARITY_DAY);
}
#endif  // LIBTEXTCLASSIFIER_UNILIB_ICU

#ifdef LIBTEXTCLASSIFIER_CALENDAR_ICU
TEST_P(TextClassifierTest, SuggestTextDateDisabled) {
  CREATE_UNILIB_FOR_TESTING;
  const std::string test_model = ReadFile(GetModelPath() + GetParam());
  std::unique_ptr<ModelT> unpacked_model = UnPackModel(test_model.c_str());

  // Disable the patterns for selection.
  for (int i = 0; i < unpacked_model->datetime_model->patterns.size(); i++) {
    unpacked_model->datetime_model->patterns[i]->enabled_modes =
        ModeFlag_ANNOTATION_AND_CLASSIFICATION;
  }
  flatbuffers::FlatBufferBuilder builder;
  builder.Finish(Model::Pack(builder, unpacked_model.get()));

  std::unique_ptr<TextClassifier> classifier =
      TextClassifier::FromUnownedBuffer(
          reinterpret_cast<const char*>(builder.GetBufferPointer()),
          builder.GetSize(), &unilib);
  ASSERT_TRUE(classifier);
  EXPECT_EQ("date",
            FirstResult(classifier->ClassifyText("january 1, 2017", {0, 15})));
  EXPECT_EQ(classifier->SuggestSelection("january 1, 2017", {0, 7}),
            std::make_pair(0, 7));
  EXPECT_THAT(classifier->Annotate("january 1, 2017"),
              ElementsAreArray({IsAnnotatedSpan(0, 15, "date")}));
}
#endif  // LIBTEXTCLASSIFIER_UNILIB_ICU

class TestingTextClassifier : public TextClassifier {
 public:
  TestingTextClassifier(const std::string& model, const UniLib* unilib)
      : TextClassifier(ViewModel(model.data(), model.size()), unilib) {}

  using TextClassifier::ResolveConflicts;
};

AnnotatedSpan MakeAnnotatedSpan(CodepointSpan span,
                                const std::string& collection,
                                const float score) {
  AnnotatedSpan result;
  result.span = span;
  result.classification.push_back({collection, score});
  return result;
}

TEST(TextClassifierTest, ResolveConflictsTrivial) {
  CREATE_UNILIB_FOR_TESTING;
  TestingTextClassifier classifier("", &unilib);

  std::vector<AnnotatedSpan> candidates{
      {MakeAnnotatedSpan({0, 1}, "phone", 1.0)}};

  std::vector<int> chosen;
  classifier.ResolveConflicts(candidates, /*context=*/"", /*cached_tokens=*/{},
                              /*interpreter_manager=*/nullptr, &chosen);
  EXPECT_THAT(chosen, ElementsAreArray({0}));
}

TEST(TextClassifierTest, ResolveConflictsSequence) {
  CREATE_UNILIB_FOR_TESTING;
  TestingTextClassifier classifier("", &unilib);

  std::vector<AnnotatedSpan> candidates{{
      MakeAnnotatedSpan({0, 1}, "phone", 1.0),
      MakeAnnotatedSpan({1, 2}, "phone", 1.0),
      MakeAnnotatedSpan({2, 3}, "phone", 1.0),
      MakeAnnotatedSpan({3, 4}, "phone", 1.0),
      MakeAnnotatedSpan({4, 5}, "phone", 1.0),
  }};

  std::vector<int> chosen;
  classifier.ResolveConflicts(candidates, /*context=*/"", /*cached_tokens=*/{},
                              /*interpreter_manager=*/nullptr, &chosen);
  EXPECT_THAT(chosen, ElementsAreArray({0, 1, 2, 3, 4}));
}

TEST(TextClassifierTest, ResolveConflictsThreeSpans) {
  CREATE_UNILIB_FOR_TESTING;
  TestingTextClassifier classifier("", &unilib);

  std::vector<AnnotatedSpan> candidates{{
      MakeAnnotatedSpan({0, 3}, "phone", 1.0),
      MakeAnnotatedSpan({1, 5}, "phone", 0.5),  // Looser!
      MakeAnnotatedSpan({3, 7}, "phone", 1.0),
  }};

  std::vector<int> chosen;
  classifier.ResolveConflicts(candidates, /*context=*/"", /*cached_tokens=*/{},
                              /*interpreter_manager=*/nullptr, &chosen);
  EXPECT_THAT(chosen, ElementsAreArray({0, 2}));
}

TEST(TextClassifierTest, ResolveConflictsThreeSpansReversed) {
  CREATE_UNILIB_FOR_TESTING;
  TestingTextClassifier classifier("", &unilib);

  std::vector<AnnotatedSpan> candidates{{
      MakeAnnotatedSpan({0, 3}, "phone", 0.5),  // Looser!
      MakeAnnotatedSpan({1, 5}, "phone", 1.0),
      MakeAnnotatedSpan({3, 7}, "phone", 0.6),  // Looser!
  }};

  std::vector<int> chosen;
  classifier.ResolveConflicts(candidates, /*context=*/"", /*cached_tokens=*/{},
                              /*interpreter_manager=*/nullptr, &chosen);
  EXPECT_THAT(chosen, ElementsAreArray({1}));
}

TEST(TextClassifierTest, ResolveConflictsFiveSpans) {
  CREATE_UNILIB_FOR_TESTING;
  TestingTextClassifier classifier("", &unilib);

  std::vector<AnnotatedSpan> candidates{{
      MakeAnnotatedSpan({0, 3}, "phone", 0.5),
      MakeAnnotatedSpan({1, 5}, "other", 1.0),  // Looser!
      MakeAnnotatedSpan({3, 7}, "phone", 0.6),
      MakeAnnotatedSpan({8, 12}, "phone", 0.6),  // Looser!
      MakeAnnotatedSpan({11, 15}, "phone", 0.9),
  }};

  std::vector<int> chosen;
  classifier.ResolveConflicts(candidates, /*context=*/"", /*cached_tokens=*/{},
                              /*interpreter_manager=*/nullptr, &chosen);
  EXPECT_THAT(chosen, ElementsAreArray({0, 2, 4}));
}

#ifdef LIBTEXTCLASSIFIER_UNILIB_ICU
TEST_P(TextClassifierTest, LongInput) {
  CREATE_UNILIB_FOR_TESTING;
  std::unique_ptr<TextClassifier> classifier =
      TextClassifier::FromPath(GetModelPath() + GetParam(), &unilib);
  ASSERT_TRUE(classifier);

  for (const auto& type_value_pair :
       std::vector<std::pair<std::string, std::string>>{
           {"address", "350 Third Street, Cambridge"},
           {"phone", "123 456-7890"},
           {"url", "www.google.com"},
           {"email", "someone@gmail.com"},
           {"flight", "LX 38"},
           {"date", "September 1, 2018"}}) {
    const std::string input_100k = std::string(50000, ' ') +
                                   type_value_pair.second +
                                   std::string(50000, ' ');
    const int value_length = type_value_pair.second.size();

    EXPECT_THAT(classifier->Annotate(input_100k),
                ElementsAreArray({IsAnnotatedSpan(50000, 50000 + value_length,
                                                  type_value_pair.first)}));
    EXPECT_EQ(classifier->SuggestSelection(input_100k, {50000, 50001}),
              std::make_pair(50000, 50000 + value_length));
    EXPECT_EQ(type_value_pair.first,
              FirstResult(classifier->ClassifyText(
                  input_100k, {50000, 50000 + value_length})));
  }
}
#endif  // LIBTEXTCLASSIFIER_UNILIB_ICU

#ifdef LIBTEXTCLASSIFIER_UNILIB_ICU
// These coarse tests are there only to make sure the execution happens in
// reasonable amount of time.
TEST_P(TextClassifierTest, LongInputNoResultCheck) {
  CREATE_UNILIB_FOR_TESTING;
  std::unique_ptr<TextClassifier> classifier =
      TextClassifier::FromPath(GetModelPath() + GetParam(), &unilib);
  ASSERT_TRUE(classifier);

  for (const std::string& value :
       std::vector<std::string>{"http://www.aaaaaaaaaaaaaaaaaaaa.com "}) {
    const std::string input_100k =
        std::string(50000, ' ') + value + std::string(50000, ' ');
    const int value_length = value.size();

    classifier->Annotate(input_100k);
    classifier->SuggestSelection(input_100k, {50000, 50001});
    classifier->ClassifyText(input_100k, {50000, 50000 + value_length});
  }
}
#endif  // LIBTEXTCLASSIFIER_UNILIB_ICU

#ifdef LIBTEXTCLASSIFIER_UNILIB_ICU
TEST_P(TextClassifierTest, MaxTokenLength) {
  CREATE_UNILIB_FOR_TESTING;
  const std::string test_model = ReadFile(GetModelPath() + GetParam());
  std::unique_ptr<ModelT> unpacked_model = UnPackModel(test_model.c_str());

  std::unique_ptr<TextClassifier> classifier;

  // With unrestricted number of tokens should behave normally.
  unpacked_model->classification_options->max_num_tokens = -1;

  flatbuffers::FlatBufferBuilder builder;
  builder.Finish(Model::Pack(builder, unpacked_model.get()));
  classifier = TextClassifier::FromUnownedBuffer(
      reinterpret_cast<const char*>(builder.GetBufferPointer()),
      builder.GetSize(), &unilib);
  ASSERT_TRUE(classifier);

  EXPECT_EQ(FirstResult(classifier->ClassifyText(
                "I live at 350 Third Street, Cambridge.", {10, 37})),
            "address");

  // Raise the maximum number of tokens to suppress the classification.
  unpacked_model->classification_options->max_num_tokens = 3;

  flatbuffers::FlatBufferBuilder builder2;
  builder2.Finish(Model::Pack(builder2, unpacked_model.get()));
  classifier = TextClassifier::FromUnownedBuffer(
      reinterpret_cast<const char*>(builder2.GetBufferPointer()),
      builder2.GetSize(), &unilib);
  ASSERT_TRUE(classifier);

  EXPECT_EQ(FirstResult(classifier->ClassifyText(
                "I live at 350 Third Street, Cambridge.", {10, 37})),
            "other");
}
#endif  // LIBTEXTCLASSIFIER_UNILIB_ICU

#ifdef LIBTEXTCLASSIFIER_UNILIB_ICU
TEST_P(TextClassifierTest, MinAddressTokenLength) {
  CREATE_UNILIB_FOR_TESTING;
  const std::string test_model = ReadFile(GetModelPath() + GetParam());
  std::unique_ptr<ModelT> unpacked_model = UnPackModel(test_model.c_str());

  std::unique_ptr<TextClassifier> classifier;

  // With unrestricted number of address tokens should behave normally.
  unpacked_model->classification_options->address_min_num_tokens = 0;

  flatbuffers::FlatBufferBuilder builder;
  builder.Finish(Model::Pack(builder, unpacked_model.get()));
  classifier = TextClassifier::FromUnownedBuffer(
      reinterpret_cast<const char*>(builder.GetBufferPointer()),
      builder.GetSize(), &unilib);
  ASSERT_TRUE(classifier);

  EXPECT_EQ(FirstResult(classifier->ClassifyText(
                "I live at 350 Third Street, Cambridge.", {10, 37})),
            "address");

  // Raise number of address tokens to suppress the address classification.
  unpacked_model->classification_options->address_min_num_tokens = 5;

  flatbuffers::FlatBufferBuilder builder2;
  builder2.Finish(Model::Pack(builder2, unpacked_model.get()));
  classifier = TextClassifier::FromUnownedBuffer(
      reinterpret_cast<const char*>(builder2.GetBufferPointer()),
      builder2.GetSize(), &unilib);
  ASSERT_TRUE(classifier);

  EXPECT_EQ(FirstResult(classifier->ClassifyText(
                "I live at 350 Third Street, Cambridge.", {10, 37})),
            "other");
}
#endif  // LIBTEXTCLASSIFIER_UNILIB_ICU

}  // namespace
}  // namespace libtextclassifier2
