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

#include "tensor-view.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace libtextclassifier2 {
namespace {

TEST(TensorViewTest, TestSize) {
  std::vector<float> data{0.1, 0.2, 0.3, 0.4, 0.5, 0.6};
  const TensorView<float> tensor(data.data(), {3, 1, 2});
  EXPECT_TRUE(tensor.is_valid());
  EXPECT_EQ(tensor.shape(), (std::vector<int>{3, 1, 2}));
  EXPECT_EQ(tensor.data(), data.data());
  EXPECT_EQ(tensor.size(), 6);
  EXPECT_EQ(tensor.dims(), 3);
  EXPECT_EQ(tensor.dim(0), 3);
  EXPECT_EQ(tensor.dim(1), 1);
  EXPECT_EQ(tensor.dim(2), 2);
  std::vector<float> output_data(6);
  EXPECT_TRUE(tensor.copy_to(output_data.data(), output_data.size()));
  EXPECT_EQ(data, output_data);

  // Should not copy when the output is small.
  std::vector<float> small_output_data{-1, -1, -1};
  EXPECT_FALSE(
      tensor.copy_to(small_output_data.data(), small_output_data.size()));
  // The output buffer should not be changed.
  EXPECT_EQ(small_output_data, (std::vector<float>{-1, -1, -1}));

  const TensorView<float> invalid_tensor = TensorView<float>::Invalid();
  EXPECT_FALSE(invalid_tensor.is_valid());
}

}  // namespace
}  // namespace libtextclassifier2
