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

#include "zlib-utils.h"

#include <memory>

#include "model_generated.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace libtextclassifier2 {

TEST(ZlibUtilsTest, CompressModel) {
  ModelT model;
  model.regex_model.reset(new RegexModelT);
  model.regex_model->patterns.emplace_back(new RegexModel_::PatternT);
  model.regex_model->patterns.back()->pattern = "this is a test pattern";
  model.regex_model->patterns.emplace_back(new RegexModel_::PatternT);
  model.regex_model->patterns.back()->pattern = "this is a second test pattern";

  model.datetime_model.reset(new DatetimeModelT);
  model.datetime_model->patterns.emplace_back(new DatetimeModelPatternT);
  model.datetime_model->patterns.back()->regexes.emplace_back(
      new DatetimeModelPattern_::RegexT);
  model.datetime_model->patterns.back()->regexes.back()->pattern =
      "an example datetime pattern";
  model.datetime_model->extractors.emplace_back(new DatetimeModelExtractorT);
  model.datetime_model->extractors.back()->pattern =
      "an example datetime extractor";

  // Compress the model.
  EXPECT_TRUE(CompressModel(&model));

  // Sanity check that uncompressed field is removed.
  EXPECT_TRUE(model.regex_model->patterns[0]->pattern.empty());
  EXPECT_TRUE(model.regex_model->patterns[1]->pattern.empty());
  EXPECT_TRUE(model.datetime_model->patterns[0]->regexes[0]->pattern.empty());
  EXPECT_TRUE(model.datetime_model->extractors[0]->pattern.empty());

  // Pack and load the model.
  flatbuffers::FlatBufferBuilder builder;
  builder.Finish(Model::Pack(builder, &model));
  const Model* compressed_model =
      GetModel(reinterpret_cast<const char*>(builder.GetBufferPointer()));
  ASSERT_TRUE(compressed_model != nullptr);

  // Decompress the fields again and check that they match the original.
  std::unique_ptr<ZlibDecompressor> decompressor = ZlibDecompressor::Instance();
  ASSERT_TRUE(decompressor != nullptr);
  std::string uncompressed_pattern;
  EXPECT_TRUE(decompressor->Decompress(
      compressed_model->regex_model()->patterns()->Get(0)->compressed_pattern(),
      &uncompressed_pattern));
  EXPECT_EQ(uncompressed_pattern, "this is a test pattern");
  EXPECT_TRUE(decompressor->Decompress(
      compressed_model->regex_model()->patterns()->Get(1)->compressed_pattern(),
      &uncompressed_pattern));
  EXPECT_EQ(uncompressed_pattern, "this is a second test pattern");
  EXPECT_TRUE(decompressor->Decompress(compressed_model->datetime_model()
                                           ->patterns()
                                           ->Get(0)
                                           ->regexes()
                                           ->Get(0)
                                           ->compressed_pattern(),
                                       &uncompressed_pattern));
  EXPECT_EQ(uncompressed_pattern, "an example datetime pattern");
  EXPECT_TRUE(decompressor->Decompress(compressed_model->datetime_model()
                                           ->extractors()
                                           ->Get(0)
                                           ->compressed_pattern(),
                                       &uncompressed_pattern));
  EXPECT_EQ(uncompressed_pattern, "an example datetime extractor");

  EXPECT_TRUE(DecompressModel(&model));
  EXPECT_EQ(model.regex_model->patterns[0]->pattern, "this is a test pattern");
  EXPECT_EQ(model.regex_model->patterns[1]->pattern,
            "this is a second test pattern");
  EXPECT_EQ(model.datetime_model->patterns[0]->regexes[0]->pattern,
            "an example datetime pattern");
  EXPECT_EQ(model.datetime_model->extractors[0]->pattern,
            "an example datetime extractor");
}

}  // namespace libtextclassifier2
