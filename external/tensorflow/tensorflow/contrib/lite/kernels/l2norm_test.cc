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
#include <gtest/gtest.h>
#include "tensorflow/contrib/lite/interpreter.h"
#include "tensorflow/contrib/lite/kernels/register.h"
#include "tensorflow/contrib/lite/kernels/test_util.h"
#include "tensorflow/contrib/lite/model.h"

namespace tflite {
namespace {

using ::testing::ElementsAreArray;

class L2NormOpModel : public SingleOpModel {
 public:
  L2NormOpModel(std::initializer_list<int> input_shape,
                ActivationFunctionType activation_type) {
    input_ = AddInput(TensorType_FLOAT32);
    output_ = AddOutput(TensorType_FLOAT32);
    SetBuiltinOp(BuiltinOperator_L2_NORMALIZATION, BuiltinOptions_L2NormOptions,
                 CreateL2NormOptions(builder_, activation_type).Union());
    BuildInterpreter({input_shape});
  }

  void SetInput(std::initializer_list<float> data) {
    PopulateTensor(input_, data);
  }

  std::vector<float> GetOutput() { return ExtractVector<float>(output_); }

 private:
  int input_;
  int output_;
};

TEST(L2NormOpTest, SimpleTest) {
  L2NormOpModel m({1, 1, 1, 6}, ActivationFunctionType_NONE);
  m.SetInput({-1.1, 0.6, 0.7, 1.2, -0.7, 0.1});
  m.Invoke();
  EXPECT_THAT(m.GetOutput(),
              ElementsAreArray({-0.55, 0.3, 0.35, 0.6, -0.35, 0.05}));
}

}  // namespace
}  // namespace tflite

int main(int argc, char** argv) {
  ::tflite::LogToStderr();
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
