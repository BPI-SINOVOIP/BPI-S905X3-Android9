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

#include <math.h>
#include <algorithm>
#include <memory>
#include <new>
#include <utility>

#include "tensorflow/compiler/xla/client/computation_builder.h"
#include "tensorflow/compiler/xla/client/local_client.h"
#include "tensorflow/compiler/xla/literal_util.h"
#include "tensorflow/compiler/xla/primitive_util.h"
#include "tensorflow/compiler/xla/ptr_util.h"
#include "tensorflow/compiler/xla/service/hlo_computation.h"
#include "tensorflow/compiler/xla/service/hlo_instruction.h"
#include "tensorflow/compiler/xla/service/hlo_module.h"
#include "tensorflow/compiler/xla/service/hlo_opcode.h"
#include "tensorflow/compiler/xla/service/hlo_runner.h"
#include "tensorflow/compiler/xla/shape_util.h"
#include "tensorflow/compiler/xla/tests/client_library_test_base.h"
#include "tensorflow/compiler/xla/tests/hlo_test_base.h"
#include "tensorflow/compiler/xla/tests/literal_test_util.h"
#include "tensorflow/compiler/xla/tests/test_macros.h"
#include "tensorflow/compiler/xla/tests/test_utils.h"
#include "tensorflow/compiler/xla/xla_data.pb.h"
#include "tensorflow/core/lib/core/status_test_util.h"
#include "tensorflow/core/lib/gtl/array_slice.h"
#include "tensorflow/core/platform/logging.h"
#include "tensorflow/core/platform/protobuf.h"
#include "tensorflow/core/platform/test.h"
#include "tensorflow/core/platform/test_benchmark.h"
#include "tensorflow/core/platform/types.h"

namespace xla {
namespace {

using ::tensorflow::gtl::ArraySlice;

class MultiOutputFusionTest : public HloTestBase {
 protected:
  MultiOutputFusionTest() { error_spec_ = ErrorSpec{0.0001, 1e-2}; }

  void RunTest2D(bool manual_fusion, int64 size) {
    auto builder = HloComputation::Builder(TestName());
    auto hlo_module = CreateNewModule();

    const Shape elem_shape0 = ShapeUtil::MakeShape(F32, {});
    const Shape elem_shape2 = ShapeUtil::MakeShape(F32, {size, size});

    auto const0 = builder.AddInstruction(
        HloInstruction::CreateConstant(Literal::CreateR0<float>(8.0f)));
    auto param0 = builder.AddInstruction(
        HloInstruction::CreateParameter(0, elem_shape0, "0"));

    auto add1 = builder.AddInstruction(HloInstruction::CreateBinary(
        elem_shape0, HloOpcode::kAdd, param0, const0));

    HloInstruction* broadcast = builder.AddInstruction(
        HloInstruction::CreateBroadcast(elem_shape2, add1, {}));

    auto param1 = builder.AddInstruction(
        HloInstruction::CreateParameter(1, elem_shape2, "1"));

    HloInstruction* add2 = builder.AddInstruction(HloInstruction::CreateBinary(
        elem_shape2, HloOpcode::kAdd, broadcast, param1));
    HloInstruction* sub = builder.AddInstruction(HloInstruction::CreateBinary(
        elem_shape2, HloOpcode::kSubtract, param1, broadcast));
    DotDimensionNumbers dot_dnums;
    dot_dnums.add_lhs_contracting_dimensions(1);
    dot_dnums.add_rhs_contracting_dimensions(0);
    HloInstruction* dot = builder.AddInstruction(
        HloInstruction::CreateDot(elem_shape2, sub, add2, dot_dnums));
    auto computation = hlo_module->AddEntryComputation(builder.Build(dot));

    if (manual_fusion) {
      auto tuple = computation->AddInstruction(HloInstruction::CreateTuple(
          ArraySlice<HloInstruction*>({sub, add2}, 0, 2)));
      auto gte0 = computation->AddInstruction(
          HloInstruction::CreateGetTupleElement(elem_shape2, tuple, 0));
      auto gte1 = computation->AddInstruction(
          HloInstruction::CreateGetTupleElement(elem_shape2, tuple, 1));
      TF_CHECK_OK(dot->ReplaceOperandWith(0, gte0));
      TF_CHECK_OK(dot->ReplaceOperandWith(1, gte1));

      CHECK_NE(
          computation->CreateFusionInstruction(
              {tuple, sub, add2, broadcast}, HloInstruction::FusionKind::kLoop),
          nullptr);
    }

    Literal arg1(ShapeUtil::MakeShape(F32, {size, size}));
    arg1.PopulateWithValue<float>(2.5f);

    Literal expect(ShapeUtil::MakeShape(F32, {size, size}));
    expect.PopulateWithValue<float>(size * 1.5f * 3.5f);
    auto actual = ExecuteAndTransfer(
        std::move(hlo_module), {Literal::CreateR0<float>(-9.0f).get(), &arg1});
    LiteralTestUtil::ExpectNear(expect, *actual, error_spec_);
  }

  void RunTest1D(bool manual_fusion, int size) {
    auto builder = HloComputation::Builder(TestName());
    auto hlo_module = CreateNewModule();

    const Shape elem_shape_F32 = ShapeUtil::MakeShape(F32, {size});
    const Shape elem_shape_U8 = ShapeUtil::MakeShape(F64, {size});
    auto param0 = builder.AddInstruction(
        HloInstruction::CreateParameter(0, elem_shape_F32, "0"));
    auto param1 = builder.AddInstruction(
        HloInstruction::CreateParameter(1, elem_shape_U8, "1"));

    HloInstruction* param0_U8 = builder.AddInstruction(
        HloInstruction::CreateConvert(elem_shape_U8, param0));
    HloInstruction* param1_F32 = builder.AddInstruction(
        HloInstruction::CreateConvert(elem_shape_F32, param1));
    HloInstruction* add = builder.AddInstruction(HloInstruction::CreateBinary(
        elem_shape_F32, HloOpcode::kAdd, param0, param1_F32));
    HloInstruction* sub_U8 =
        builder.AddInstruction(HloInstruction::CreateBinary(
            elem_shape_U8, HloOpcode::kSubtract, param0_U8, param1));
    HloInstruction* sub = builder.AddInstruction(
        HloInstruction::CreateConvert(elem_shape_F32, sub_U8));

    HloInstruction* reshape =
        builder.AddInstruction(HloInstruction::CreateReshape(
            ShapeUtil::MakeShape(F32, {size, 1}), add));
    DotDimensionNumbers dot_dnums;
    dot_dnums.add_lhs_contracting_dimensions(0);
    dot_dnums.add_rhs_contracting_dimensions(0);
    HloInstruction* dot = builder.AddInstruction(HloInstruction::CreateDot(
        ShapeUtil::MakeShape(F32, {1}), sub, reshape, dot_dnums));
    auto computation = hlo_module->AddEntryComputation(builder.Build(dot));

    if (manual_fusion) {
      auto tuple = computation->AddInstruction(HloInstruction::CreateTuple(
          ArraySlice<HloInstruction*>({sub_U8, add}, 0, 2)));

      auto gte0 = computation->AddInstruction(
          HloInstruction::CreateGetTupleElement(elem_shape_U8, tuple, 0));
      auto gte1 = computation->AddInstruction(
          HloInstruction::CreateGetTupleElement(elem_shape_F32, tuple, 1));
      TF_CHECK_OK(sub->ReplaceOperandWith(0, gte0));
      TF_CHECK_OK(reshape->ReplaceOperandWith(0, gte1));

      CHECK_NE(computation->CreateFusionInstruction(
                   {tuple, sub_U8, add, param0_U8, param1_F32},
                   HloInstruction::FusionKind::kLoop),
               nullptr);
    }

    Literal input0(ShapeUtil::MakeShape(F32, {size}));
    input0.PopulateWithValue(2.5f);
    Literal input1(ShapeUtil::MakeShape(F64, {size}));
    input1.PopulateWithValue(1.);

    Literal expect = std::move(*Literal::CreateR1<float>({size * 1.5f * 3.5f}));
    auto actual = ExecuteAndTransfer(std::move(hlo_module), {&input0, &input1});
    LiteralTestUtil::ExpectNear(expect, *actual, error_spec_);
  }
};

XLA_TEST_F(MultiOutputFusionTest, 2DNofusion) { RunTest2D(false, 5); }
XLA_TEST_F(MultiOutputFusionTest, 2DFusion) { RunTest2D(true, 5); }
XLA_TEST_F(MultiOutputFusionTest, 2DFusionSize129) { RunTest2D(true, 129); }
XLA_TEST_F(MultiOutputFusionTest, DiffentTypesNoFusion) { RunTest1D(false, 8); }
XLA_TEST_F(MultiOutputFusionTest, DiffentTypesFusion) { RunTest1D(true, 8); }

XLA_TEST_F(MultiOutputFusionTest, FusionNodeIsRoot) {
  const char* testcase = R"(
    HloModule m

    fused_computation {
      x.param_0 = (((s32[]), f32[]), (f32[], s32[])) parameter(0)
      gte.3 = ((s32[]), f32[]) get-tuple-element(x.param_0), index=0
      gte.2 = (s32[]) get-tuple-element(gte.3), index=0
      gte.4 = s32[] get-tuple-element(gte.2), index=0
      copy = s32[] copy(gte.4)
      ROOT tuple = (s32[]) tuple(copy)
    }

    ENTRY thing.v3 {
      x = (((s32[]), f32[]), (f32[], s32[])) parameter(0)
      ROOT fusion = (s32[]) fusion(x), kind=kLoop, calls=fused_computation
    }
  )";
  auto module =
      HloRunner::CreateModuleFromString(testcase, GetDebugOptionsForTest())
          .ValueOrDie();
  auto param = Literal::MakeTupleOwned(
      Literal::MakeTupleOwned(
          Literal::MakeTupleOwned(Literal::CreateR0<int32>(42)),
          Literal::CreateR0<float>(1.0)),
      Literal::MakeTupleOwned(Literal::CreateR0<float>(3.0),
                              Literal::CreateR0<int32>(4)));
  TF_ASSERT_OK_AND_ASSIGN(auto result,
                          Execute(std::move(module), {param.get()}));
  EXPECT_TRUE(LiteralTestUtil::Equal(
      *result, *Literal::MakeTupleOwned(Literal::CreateR0<int32>(42))));
}

}  // namespace
}  // namespace xla
