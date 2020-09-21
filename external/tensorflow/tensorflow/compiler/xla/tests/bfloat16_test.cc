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

#include <cmath>
#include <memory>
#include <vector>

#include "tensorflow/compiler/xla/array2d.h"
#include "tensorflow/compiler/xla/array4d.h"
#include "tensorflow/compiler/xla/client/computation.h"
#include "tensorflow/compiler/xla/client/computation_builder.h"
#include "tensorflow/compiler/xla/client/lib/arithmetic.h"
#include "tensorflow/compiler/xla/client/local_client.h"
#include "tensorflow/compiler/xla/literal_util.h"
#include "tensorflow/compiler/xla/reference_util.h"
#include "tensorflow/compiler/xla/service/hlo_computation.h"
#include "tensorflow/compiler/xla/service/hlo_instruction.h"
#include "tensorflow/compiler/xla/service/hlo_module.h"
#include "tensorflow/compiler/xla/shape_util.h"
#include "tensorflow/compiler/xla/statusor.h"
#include "tensorflow/compiler/xla/test.h"
#include "tensorflow/compiler/xla/test_helpers.h"
#include "tensorflow/compiler/xla/tests/client_library_test_base.h"
#include "tensorflow/compiler/xla/tests/hlo_test_base.h"
#include "tensorflow/compiler/xla/tests/literal_test_util.h"
#include "tensorflow/compiler/xla/tests/test_macros.h"
#include "tensorflow/compiler/xla/tests/test_utils.h"
#include "tensorflow/compiler/xla/util.h"
#include "tensorflow/compiler/xla/xla_data.pb.h"
#include "tensorflow/core/platform/logging.h"
#include "tensorflow/core/platform/test.h"
#include "tensorflow/core/platform/types.h"

namespace xla {
namespace {

class Bfloat16Test : public ClientLibraryTestBase {
 protected:
  const ErrorSpec error_spec_{0.001, 0.001};
};

XLA_TEST_F(Bfloat16Test, ScalarOperation) {
  ComputationBuilder builder(client_, TestName());
  auto x = builder.ConstantR0<bfloat16>(static_cast<bfloat16>(2.0f));
  auto y = builder.ConstantR0<bfloat16>(static_cast<bfloat16>(1.0f));
  builder.Add(x, y);

  ComputeAndCompareR0<bfloat16>(&builder, static_cast<bfloat16>(3.0f), {},
                                error_spec_);
}

XLA_TEST_F(Bfloat16Test, LogOperation) {
  ComputationBuilder builder(client_, TestName());
  auto x = builder.ConstantR0<bfloat16>(static_cast<bfloat16>(4.0f));
  builder.Log(x);

  ComputeAndCompareR0<bfloat16>(&builder, static_cast<bfloat16>(1.387f), {},
                                error_spec_);
}

XLA_TEST_F(Bfloat16Test, NegateScalarF16) {
  ComputationBuilder builder(client_, TestName());
  builder.Neg(builder.ConstantR0<bfloat16>(static_cast<bfloat16>(2.1f)));

  ComputeAndCompareR0<bfloat16>(&builder, static_cast<bfloat16>(-2.1f), {},
                                error_spec_);
}

XLA_TEST_F(Bfloat16Test, BatchNormTraining) {
  const int kFeatureIndex = 2;
  ComputationBuilder builder(client_, TestName());

  auto operand = builder.ConstantR4FromArray4D<bfloat16>(
      {{{{static_cast<bfloat16>(1.f)}, {static_cast<bfloat16>(2.f)}},
        {{static_cast<bfloat16>(3.f)}, {static_cast<bfloat16>(4.f)}}},
       {{{static_cast<bfloat16>(5.f)}, {static_cast<bfloat16>(6.f)}},
        {{static_cast<bfloat16>(7.f)}, {static_cast<bfloat16>(8.f)}}}});

  auto scale = builder.ConstantR1<bfloat16>(
      {static_cast<bfloat16>(2.0f), static_cast<bfloat16>(3.0f)});

  auto offset = builder.ConstantR1<bfloat16>(
      {static_cast<bfloat16>(1.0f), static_cast<bfloat16>(2.0f)});

  auto tuple = builder.BatchNormTraining(operand, scale, offset,
                                         /*epsilon=*/0.001, kFeatureIndex);

  auto expected = Literal::MakeTuple(
      {Literal::CreateR4<bfloat16>(
           {{{{static_cast<bfloat16>(-1.6875f)},
              {static_cast<bfloat16>(-2.04f)}},
             {{static_cast<bfloat16>(0.105f)}, {static_cast<bfloat16>(0.66f)}}},
            {{{static_cast<bfloat16>(1.89f)}, {static_cast<bfloat16>(3.35f)}},
             {{static_cast<bfloat16>(3.7f)}, {static_cast<bfloat16>(6.04f)}}}})
           .get(),
       Literal::CreateR1<bfloat16>(
           {static_cast<bfloat16>(4), static_cast<bfloat16>(5)})
           .get(),
       Literal::CreateR1<bfloat16>(
           {static_cast<bfloat16>(5), static_cast<bfloat16>(5)})
           .get()});

  ComputeAndCompareTuple(&builder, *expected, {}, ErrorSpec(0.01));
}

XLA_TEST_F(Bfloat16Test, BatchNormGrad) {
  const int kFeatureIndex = 2;
  ComputationBuilder builder(client_, TestName());

  auto operand = builder.ConstantR4FromArray4D<bfloat16>(
      Array4D<bfloat16>(2, 2, 2, 1, static_cast<bfloat16>(0.0f)));

  auto scale = builder.ConstantR1<bfloat16>(
      {static_cast<bfloat16>(1.0f), static_cast<bfloat16>(1.0f)});

  auto mean = builder.ConstantR1<bfloat16>(
      {static_cast<bfloat16>(0.0f), static_cast<bfloat16>(0.0f)});

  auto var = builder.ConstantR1<bfloat16>(
      {static_cast<bfloat16>(1.0f), static_cast<bfloat16>(1.0f)});

  auto grad_output = builder.ConstantR4FromArray4D<bfloat16>(
      {{{{static_cast<bfloat16>(1.f)}, {static_cast<bfloat16>(2.f)}},
        {{static_cast<bfloat16>(3.f)}, {static_cast<bfloat16>(4.f)}}},
       {{{static_cast<bfloat16>(5.f)}, {static_cast<bfloat16>(6.f)}},
        {{static_cast<bfloat16>(7.f)}, {static_cast<bfloat16>(8.f)}}}});

  builder.BatchNormGrad(operand, scale, mean, var, grad_output,
                        /*epsilon=*/0.0, kFeatureIndex);

  auto expected = Literal::MakeTuple(
      {Literal::CreateR4<bfloat16>(
           {{{{static_cast<bfloat16>(-3.f)}, {static_cast<bfloat16>(-3.f)}},
             {{static_cast<bfloat16>(-1.f)}, {static_cast<bfloat16>(-1.f)}}},
            {{{static_cast<bfloat16>(1.f)}, {static_cast<bfloat16>(1.f)}},
             {{static_cast<bfloat16>(3.f)}, {static_cast<bfloat16>(3.f)}}}})
           .get(),
       Literal::CreateR1<bfloat16>(
           {static_cast<bfloat16>(0), static_cast<bfloat16>(0)})
           .get(),
       Literal::CreateR1<bfloat16>(
           {static_cast<bfloat16>(16), static_cast<bfloat16>(20)})
           .get()});

  ComputeAndCompareTuple(&builder, *expected, {}, ErrorSpec(0.01));
}

}  // namespace
}  // namespace xla
