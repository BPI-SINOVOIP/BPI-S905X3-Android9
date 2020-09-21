/* Copyright 2017 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/
#include "tensorflow/contrib/lite/kernels/internal/tensor.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace tflite {
namespace {

using ::testing::ElementsAre;

TEST(TensorTest, GetTensorDims4D) {
  Dims<4> d = GetTensorDims({2, 3, 4, 5});
  EXPECT_THAT(d.sizes, ElementsAre(5, 4, 3, 2));
  EXPECT_THAT(d.strides, ElementsAre(1, 5, 20, 60));
}

TEST(TensorTest, GetTensorDims3D) {
  Dims<4> d = GetTensorDims({3, 4, 5});
  EXPECT_THAT(d.sizes, ElementsAre(5, 4, 3, 1));
  EXPECT_THAT(d.strides, ElementsAre(1, 5, 20, 60));
}

TEST(TensorTest, GetTensorDims2D) {
  Dims<4> d = GetTensorDims({4, 5});
  EXPECT_THAT(d.sizes, ElementsAre(5, 4, 1, 1));
  EXPECT_THAT(d.strides, ElementsAre(1, 5, 20, 20));
}

TEST(TensorTest, GetTensorDims1D) {
  Dims<4> d = GetTensorDims({5});
  EXPECT_THAT(d.sizes, ElementsAre(5, 1, 1, 1));
  EXPECT_THAT(d.strides, ElementsAre(1, 5, 5, 5));
}

}  // namespace
}  // namespace tflite

int main(int argc, char** argv) {
  // On Linux, add: tflite::LogToStderr();
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
