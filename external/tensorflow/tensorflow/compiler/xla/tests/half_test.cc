/* Copyright 2018 The TensorFlow Authors. All Rights Reserved.

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

#include <cmath>
#include <vector>

#include "tensorflow/compiler/xla/client/computation.h"
#include "tensorflow/compiler/xla/client/computation_builder.h"
#include "tensorflow/compiler/xla/literal_util.h"
#include "tensorflow/compiler/xla/statusor.h"
#include "tensorflow/compiler/xla/test.h"
#include "tensorflow/compiler/xla/test_helpers.h"
#include "tensorflow/compiler/xla/tests/client_library_test_base.h"
#include "tensorflow/compiler/xla/tests/test_macros.h"
#include "tensorflow/compiler/xla/tests/test_utils.h"

// Tests the handling of the basic mathematics operations with F16 operands.

namespace xla {
namespace {

class HalfTestBase : public ClientLibraryTestBase {
 protected:
  const ErrorSpec error_spec_{0.001, 0.001};
  // Number of elements in the input buffers.
  static const int kNumElements = 4;
};

using UnaryBuildFuncTy =
    std::function<void(ComputationBuilder*, const ComputationDataHandle& src)>;

struct UnaryOpTestParam {
  std::function<half(half)> compute_func;
  UnaryBuildFuncTy build_func;
};

class UnaryOpTest : public HalfTestBase,
                    public ::testing::WithParamInterface<UnaryOpTestParam> {};

XLA_TEST_P(UnaryOpTest, Ops) {
  std::vector<half> x({half(1.4), half(-2.3), half(3.2), half(-4.1)});
  ComputationBuilder builder(client_, TestName());
  ComputationDataHandle x_opnd;
  auto x_data = CreateR1Parameter<half>(x, /*parameter_number=*/0, "x",
                                        &builder, &x_opnd);

  std::function<half(half)> compute_func = GetParam().compute_func;
  std::vector<half> expected;
  for (int64 i = 0; i < x.size(); ++i) {
    expected.push_back(compute_func(x[i]));
  }

  UnaryBuildFuncTy build_func = GetParam().build_func;
  build_func(&builder, x_opnd);

  ComputeAndCompareR1<half>(&builder, expected, {x_data.get()}, error_spec_);
}

half sign_imp(half value) {
  const float x(std::move(value));
  return half((x < .0) ? -1 : (x > .0));
}

half round_imp(half value) {
  return half(round(static_cast<float>(std::move(value))));
}

INSTANTIATE_TEST_CASE_P(
    half, UnaryOpTest,
    ::testing::Values(UnaryOpTestParam{[](half x) { return abs(x); },
                                       &ComputationBuilder::Abs},
                      UnaryOpTestParam{[](half x) { return round_imp(x); },
                                       &ComputationBuilder::Round},
                      UnaryOpTestParam{[](half x) { return ceil(x); },
                                       &ComputationBuilder::Ceil},
                      UnaryOpTestParam{[](half x) { return cos(x); },
                                       &ComputationBuilder::Cos},
                      UnaryOpTestParam{[](half x) { return exp(x); },
                                       &ComputationBuilder::Exp},
                      UnaryOpTestParam{[](half x) { return floor(x); },
                                       &ComputationBuilder::Floor},
                      UnaryOpTestParam{[](half x) { return log(x); },
                                       &ComputationBuilder::Log},
                      UnaryOpTestParam{[](half x) { return -x; },
                                       &ComputationBuilder::Neg},
                      UnaryOpTestParam{[](half x) { return sign_imp(x); },
                                       &ComputationBuilder::Sign},
                      UnaryOpTestParam{[](half x) { return sin(x); },
                                       &ComputationBuilder::Sin},
                      UnaryOpTestParam{[](half x) { return tanh(x); },
                                       &ComputationBuilder::Tanh}

                      ));

struct UnaryPredTestParam {
  std::function<bool(half)> compute_func;
  UnaryBuildFuncTy build_func;
};

class UnaryPredTest : public HalfTestBase,
                      public ::testing::WithParamInterface<UnaryPredTestParam> {
};

XLA_TEST_P(UnaryPredTest, Ops) {
  std::vector<half> x({half(1.4), half(-2.3), half(3.2), half(-4.1)});
  ComputationBuilder builder(client_, TestName());
  ComputationDataHandle x_opnd;
  auto x_data = CreateR1Parameter<half>(x, /*parameter_number=*/0, "x",
                                        &builder, &x_opnd);

  std::function<bool(half)> compute_func = GetParam().compute_func;
  CHECK_EQ(kNumElements, x.size());
  bool expected[kNumElements];
  for (int64 i = 0; i < x.size(); ++i) {
    expected[i] = compute_func(x[i]);
  }

  UnaryBuildFuncTy build_func = GetParam().build_func;
  build_func(&builder, x_opnd);

  ComputeAndCompareR1<bool>(&builder, expected, {x_data.get()});
}

INSTANTIATE_TEST_CASE_P(half, UnaryPredTest,
                        ::testing::Values(UnaryPredTestParam{
                            [](half x) { return isfinite(x); },
                            &ComputationBuilder::IsFinite}));

using BinaryBuildFuncTy = std::function<void(
    ComputationBuilder*, const ComputationDataHandle& x,
    const ComputationDataHandle& y, tensorflow::gtl::ArraySlice<int64>)>;

struct BinaryOpTestParam {
  std::function<half(half, half)> compute_func;
  BinaryBuildFuncTy build_func;
};

class BinaryOpTest : public HalfTestBase,
                     public ::testing::WithParamInterface<BinaryOpTestParam> {};

XLA_TEST_P(BinaryOpTest, Ops) {
  std::vector<half> x({half(1.0), half(2.0), half(3.0), half(-4.0)});
  std::vector<half> y({half(0.4), half(-0.3), half(0.2), half(0.1)});
  ComputationBuilder builder(client_, TestName());
  ComputationDataHandle x_opnd;
  auto x_data = CreateR1Parameter<half>(x, /*parameter_number=*/0, "x",
                                        &builder, &x_opnd);

  ComputationDataHandle y_opnd;
  auto y_data = CreateR1Parameter<half>(y, /*parameter_number=*/1, "y",
                                        &builder, &y_opnd);

  std::function<half(half, half)> compute_func = GetParam().compute_func;
  std::vector<half> expected;
  for (int64 i = 0; i < x.size(); ++i) {
    expected.push_back(compute_func(x[i], y[i]));
  }

  BinaryBuildFuncTy build_func = GetParam().build_func;
  build_func(&builder, x_opnd, y_opnd, {});

  ComputeAndCompareR1<half>(&builder, expected, {x_data.get(), y_data.get()},
                            error_spec_);
}

half atan2_imp(half x, half y) {
  return half(atan2(static_cast<float>(std::move(x)),
                    static_cast<float>(std::move(y))));
}

INSTANTIATE_TEST_CASE_P(
    half, BinaryOpTest,
    ::testing::Values(
        BinaryOpTestParam{[](half x, half y) { return x + y; },
                          &ComputationBuilder::Add},
        BinaryOpTestParam{[](half x, half y) { return atan2_imp(x, y); },
                          &ComputationBuilder::Atan2},
        BinaryOpTestParam{[](half x, half y) { return x / y; },
                          &ComputationBuilder::Div},
        BinaryOpTestParam{[](half x, half y) { return max(x, y); },
                          &ComputationBuilder::Max},
        BinaryOpTestParam{[](half x, half y) { return min(x, y); },
                          &ComputationBuilder::Min},
        BinaryOpTestParam{[](half x, half y) { return x * y; },
                          &ComputationBuilder::Mul},
        BinaryOpTestParam{[](half x, half y) { return pow(x, y); },
                          &ComputationBuilder::Pow},
        BinaryOpTestParam{[](half x, half y) { return x - y; },
                          &ComputationBuilder::Sub}

        ));

struct BinaryPredTestParam {
  std::function<bool(half, half)> compute_func;
  BinaryBuildFuncTy build_func;
};

class BinaryPredTest
    : public HalfTestBase,
      public ::testing::WithParamInterface<BinaryPredTestParam> {};

XLA_TEST_P(BinaryPredTest, Ops) {
  std::vector<half> x({half(1.0), half(2.0), half(0.2), half(-4.0)});
  std::vector<half> y({half(0.4), half(-0.3), half(0.2), half(0.1)});
  ComputationBuilder builder(client_, TestName());
  ComputationDataHandle x_opnd;
  auto x_data = CreateR1Parameter<half>(x, /*parameter_number=*/0, "x",
                                        &builder, &x_opnd);

  ComputationDataHandle y_opnd;
  auto y_data = CreateR1Parameter<half>(y, /*parameter_number=*/1, "y",
                                        &builder, &y_opnd);

  std::function<bool(half, half)> compute_func = GetParam().compute_func;
  CHECK_EQ(kNumElements, x.size());
  bool expected[kNumElements];
  for (int64 i = 0; i < x.size(); ++i) {
    expected[i] = compute_func(x[i], y[i]);
  }

  BinaryBuildFuncTy build_func = GetParam().build_func;
  build_func(&builder, x_opnd, y_opnd, {});

  ComputeAndCompareR1<bool>(&builder, expected, {x_data.get(), y_data.get()});
}

INSTANTIATE_TEST_CASE_P(
    half, BinaryPredTest,
    ::testing::Values(BinaryPredTestParam{[](half x, half y) { return x == y; },
                                          &ComputationBuilder::Eq},
                      BinaryPredTestParam{[](half x, half y) { return x != y; },
                                          &ComputationBuilder::Ne},
                      BinaryPredTestParam{[](half x, half y) { return x >= y; },
                                          &ComputationBuilder::Ge},
                      BinaryPredTestParam{[](half x, half y) { return x > y; },
                                          &ComputationBuilder::Gt},
                      BinaryPredTestParam{[](half x, half y) { return x <= y; },
                                          &ComputationBuilder::Le},
                      BinaryPredTestParam{[](half x, half y) { return x < y; },
                                          &ComputationBuilder::Lt}

                      ));

}  // namespace
}  // namespace xla
