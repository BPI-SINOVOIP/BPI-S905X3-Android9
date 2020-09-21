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

#include "tensorflow/compiler/xla/service/cpu/cpu_instruction_fusion.h"

#include <algorithm>
#include <set>

#include "tensorflow/compiler/xla/service/hlo_matchers.h"
#include "tensorflow/compiler/xla/service/transpose_folding.h"
#include "tensorflow/compiler/xla/tests/hlo_test_base.h"
#include "tensorflow/core/lib/gtl/array_slice.h"

namespace op = xla::testing::opcode_matchers;

namespace xla {
namespace cpu {
namespace {

using InstructionFusionTest = HloTestBase;

std::unique_ptr<HloInstruction> MakeDot(const Shape& shape, HloInstruction* lhs,
                                        HloInstruction* rhs) {
  DotDimensionNumbers dot_dnums;
  dot_dnums.add_lhs_contracting_dimensions(1);
  dot_dnums.add_rhs_contracting_dimensions(0);
  return HloInstruction::CreateDot(shape, lhs, rhs, dot_dnums);
}

TEST_F(InstructionFusionTest, DotOperationFusion_Basic_0) {
  HloComputation::Builder builder(TestName());
  HloInstruction* arg0 = builder.AddInstruction(HloInstruction::CreateParameter(
      0, ShapeUtil::MakeShape(F32, {1024, 256}), "arg0"));
  HloInstruction* arg1 = builder.AddInstruction(HloInstruction::CreateParameter(
      1, ShapeUtil::MakeShape(F32, {256, 1}), "arg1"));

  HloInstruction* exp0 = builder.AddInstruction(HloInstruction::CreateUnary(
      ShapeUtil::MakeShape(S32, {1024, 256}), HloOpcode::kExp, arg0));
  HloInstruction* dot = builder.AddInstruction(
      MakeDot(ShapeUtil::MakeShape(F32, {1024, 1}), exp0, arg1));

  auto module = CreateNewModule();
  auto computation = module->AddEntryComputation(builder.Build());
  EXPECT_EQ(dot, computation->root_instruction());
  EXPECT_TRUE(CpuInstructionFusion().Run(module.get()).ValueOrDie());
  EXPECT_THAT(computation->root_instruction(), op::Fusion());
}

TEST_F(InstructionFusionTest, DotOperationFusion_Basic_1) {
  HloComputation::Builder builder(TestName());
  HloInstruction* arg0 = builder.AddInstruction(HloInstruction::CreateParameter(
      0, ShapeUtil::MakeShape(F32, {1, 256}), "arg0"));
  HloInstruction* arg1 = builder.AddInstruction(HloInstruction::CreateParameter(
      1, ShapeUtil::MakeShape(F32, {256, 1024}), "arg1"));

  HloInstruction* exp1 = builder.AddInstruction(HloInstruction::CreateUnary(
      ShapeUtil::MakeShape(S32, {256, 1024}), HloOpcode::kExp, arg1));
  HloInstruction* dot = builder.AddInstruction(
      MakeDot(ShapeUtil::MakeShape(F32, {1, 1024}), arg0, exp1));

  auto module = CreateNewModule();
  auto computation = module->AddEntryComputation(builder.Build());
  EXPECT_EQ(dot, computation->root_instruction());
  EXPECT_TRUE(CpuInstructionFusion().Run(module.get()).ValueOrDie());
  EXPECT_THAT(computation->root_instruction(), op::Fusion());
}

TEST_F(InstructionFusionTest, DotOperationFusion_Bitcast) {
  HloComputation::Builder builder(TestName());
  HloInstruction* arg0 = builder.AddInstruction(HloInstruction::CreateParameter(
      0, ShapeUtil::MakeShape(F32, {2, 512, 2, 128}), "arg0"));
  HloInstruction* arg1 = builder.AddInstruction(HloInstruction::CreateParameter(
      1, ShapeUtil::MakeShape(F32, {256, 1}), "arg1"));

  HloInstruction* exp0 = builder.AddInstruction(HloInstruction::CreateUnary(
      ShapeUtil::MakeShape(S32, {2, 512, 2, 128}), HloOpcode::kExp, arg0));
  HloInstruction* bitcast0 = builder.AddInstruction(HloInstruction::CreateUnary(
      ShapeUtil::MakeShape(S32, {1024, 256}), HloOpcode::kBitcast, exp0));
  HloInstruction* dot = builder.AddInstruction(
      MakeDot(ShapeUtil::MakeShape(F32, {1024, 1}), bitcast0, arg1));

  auto module = CreateNewModule();
  auto computation = module->AddEntryComputation(builder.Build());
  EXPECT_EQ(dot, computation->root_instruction());
  EXPECT_TRUE(CpuInstructionFusion().Run(module.get()).ValueOrDie());
  EXPECT_THAT(computation->root_instruction(), op::Fusion());
}

TEST_F(InstructionFusionTest, DotOperationFusion_Reshape) {
  HloComputation::Builder builder(TestName());
  HloInstruction* arg0 = builder.AddInstruction(HloInstruction::CreateParameter(
      0, ShapeUtil::MakeShape(F32, {2, 512, 2, 128}), "arg0"));
  HloInstruction* arg1 = builder.AddInstruction(HloInstruction::CreateParameter(
      1, ShapeUtil::MakeShape(F32, {256, 1}), "arg1"));

  HloInstruction* exp0 = builder.AddInstruction(HloInstruction::CreateUnary(
      ShapeUtil::MakeShape(S32, {2, 512, 2, 128}), HloOpcode::kExp, arg0));
  HloInstruction* reshape0 =
      builder.AddInstruction(HloInstruction::CreateReshape(
          ShapeUtil::MakeShape(S32, {1024, 256}), exp0));
  HloInstruction* dot = builder.AddInstruction(
      MakeDot(ShapeUtil::MakeShape(F32, {1024, 1}), reshape0, arg1));

  auto module = CreateNewModule();
  auto computation = module->AddEntryComputation(builder.Build());
  EXPECT_EQ(dot, computation->root_instruction());
  EXPECT_TRUE(CpuInstructionFusion().Run(module.get()).ValueOrDie());
  EXPECT_THAT(computation->root_instruction(), op::Fusion());
}

TEST_F(InstructionFusionTest, DotOperationFusion_TooLarge) {
  HloComputation::Builder builder(TestName());
  HloInstruction* arg0 = builder.AddInstruction(HloInstruction::CreateParameter(
      0, ShapeUtil::MakeShape(F32, {1, 32 * 1024}), "arg0"));
  HloInstruction* arg1 = builder.AddInstruction(HloInstruction::CreateParameter(
      1, ShapeUtil::MakeShape(F32, {256, 32 * 1024}), "arg1"));

  HloInstruction* exp1 = builder.AddInstruction(HloInstruction::CreateUnary(
      ShapeUtil::MakeShape(S32, {256, 32 * 1024}), HloOpcode::kExp, arg1));
  HloInstruction* dot = builder.AddInstruction(
      MakeDot(ShapeUtil::MakeShape(F32, {1, 32 * 1024}), arg0, exp1));

  auto module = CreateNewModule();
  auto computation = module->AddEntryComputation(builder.Build());
  EXPECT_EQ(dot, computation->root_instruction());
  EXPECT_FALSE(CpuInstructionFusion().Run(module.get()).ValueOrDie());
  EXPECT_EQ(dot, computation->root_instruction());
}

TEST_F(InstructionFusionTest, DotOperationFusion_ElementReuse) {
  HloComputation::Builder builder(TestName());
  HloInstruction* arg0 = builder.AddInstruction(HloInstruction::CreateParameter(
      0, ShapeUtil::MakeShape(F32, {2, 256}), "arg0"));
  HloInstruction* arg1 = builder.AddInstruction(HloInstruction::CreateParameter(
      1, ShapeUtil::MakeShape(F32, {256, 1024}), "arg1"));

  HloInstruction* exp1 = builder.AddInstruction(HloInstruction::CreateUnary(
      ShapeUtil::MakeShape(S32, {256, 1024}), HloOpcode::kExp, arg1));
  HloInstruction* dot = builder.AddInstruction(
      MakeDot(ShapeUtil::MakeShape(F32, {2, 1024}), arg0, exp1));

  auto module = CreateNewModule();
  auto computation = module->AddEntryComputation(builder.Build());
  EXPECT_EQ(dot, computation->root_instruction());
  EXPECT_FALSE(CpuInstructionFusion().Run(module.get()).ValueOrDie());
  EXPECT_EQ(dot, computation->root_instruction());
}

TEST_F(InstructionFusionTest, DotOperationFusion_TransposeFusion) {
  HloComputation::Builder builder(TestName());
  HloInstruction* arg0 = builder.AddInstruction(HloInstruction::CreateParameter(
      0, ShapeUtil::MakeShape(F32, {1, 256}), "arg0"));
  HloInstruction* arg1 = builder.AddInstruction(HloInstruction::CreateParameter(
      1, ShapeUtil::MakeShape(F32, {1024, 256}), "arg1"));

  HloInstruction* exp1 = builder.AddInstruction(HloInstruction::CreateUnary(
      ShapeUtil::MakeShape(S32, {1024, 256}), HloOpcode::kExp, arg1));
  HloInstruction* transpose1 =
      builder.AddInstruction(HloInstruction::CreateTranspose(
          ShapeUtil::MakeShape(S32, {256, 1024}), exp1, {1, 0}));
  builder.AddInstruction(
      MakeDot(ShapeUtil::MakeShape(F32, {1, 1024}), arg0, transpose1));

  auto module = CreateNewModule();
  auto computation = module->AddEntryComputation(builder.Build());
  TransposeFolding transpose_folding(
      [](const HloInstruction& dot,
         const TransposeFolding::OperandIndices& candidate_operands) {
        return candidate_operands;
      },
      TransposeFolding::NeverFoldTranspose);
  EXPECT_TRUE(transpose_folding.Run(module.get()).ValueOrDie());
  EXPECT_EQ(computation->root_instruction()->opcode(), HloOpcode::kFusion);
  EXPECT_EQ(computation->root_instruction()->fusion_kind(),
            HloInstruction::FusionKind::kTransposeDot);
  EXPECT_FALSE(CpuInstructionFusion().Run(module.get()).ValueOrDie());
  EXPECT_EQ(computation->root_instruction()->opcode(), HloOpcode::kFusion);
  EXPECT_EQ(computation->root_instruction()->fusion_kind(),
            HloInstruction::FusionKind::kTransposeDot);
}

class OpcodeFusionTest : public InstructionFusionTest {
 protected:
  // Runs CPU instruction fusion on the given module, and tests that the result
  // contains a fused op at the root with exactly the given multiset of opcodes.
  void RunFusionAndCheckOpcodesWereFused(
      HloModule* module, const std::multiset<HloOpcode>& expected_opcodes,
      HloInstruction::FusionKind fusion_kind =
          HloInstruction::FusionKind::kLoop) {
    auto computation = module->entry_computation();
    auto did_fusion = CpuInstructionFusion().Run(module);
    ASSERT_TRUE(did_fusion.ok());
    EXPECT_TRUE(did_fusion.ValueOrDie());

    HloInstruction* root = computation->root_instruction();
    ASSERT_THAT(root, op::Fusion());
    EXPECT_EQ(root->fusion_kind(), fusion_kind);

    std::vector<HloOpcode> fused_opcodes(root->fused_instruction_count());
    std::transform(root->fused_instructions().begin(),
                   root->fused_instructions().end(), fused_opcodes.begin(),
                   [](const HloInstruction* hlo) { return hlo->opcode(); });

    EXPECT_EQ(
        std::multiset<HloOpcode>(fused_opcodes.begin(), fused_opcodes.end()),
        expected_opcodes);
  }

  HloComputation* CreateAdderToOne(HloModule* module) {
    HloComputation::Builder builder(TestName());
    HloInstruction* arg0 =
        builder.AddInstruction(HloInstruction::CreateParameter(
            0, ShapeUtil::MakeShape(F32, {}), "arg0"));
    HloInstruction* one = builder.AddInstruction(
        HloInstruction::CreateConstant(Literal::CreateR0<float>(1.0)));
    builder.AddInstruction(HloInstruction::CreateBinary(
        ShapeUtil::MakeShape(F32, {}), HloOpcode::kAdd, arg0, one));
    return module->AddEmbeddedComputation(builder.Build());
  }

  HloComputation* CreateMax(HloModule* module) {
    HloComputation::Builder builder(TestName());
    HloInstruction* arg0 =
        builder.AddInstruction(HloInstruction::CreateParameter(
            0, ShapeUtil::MakeShape(F32, {}), "arg0"));
    HloInstruction* arg1 =
        builder.AddInstruction(HloInstruction::CreateParameter(
            1, ShapeUtil::MakeShape(F32, {}), "arg1"));
    builder.AddInstruction(HloInstruction::CreateBinary(
        ShapeUtil::MakeShape(F32, {}), HloOpcode::kMaximum, arg0, arg1));
    return module->AddEmbeddedComputation(builder.Build());
  }
};

TEST_F(OpcodeFusionTest, Exponential_Bitcast_Negate) {
  HloComputation::Builder builder(TestName());
  Shape param_shape = ShapeUtil::MakeShape(F32, {1, 4});
  Shape result_shape = ShapeUtil::MakeShape(F32, {4});
  HloInstruction* param0 = builder.AddInstruction(
      HloInstruction::CreateParameter(0, param_shape, "param"));
  // InstructionFusion::ShouldFuse() precludes fusing a bitcast whose operand
  // is a parameter, so create an operand between the parameter and bitcast.
  HloInstruction* exp1 = builder.AddInstruction(
      HloInstruction::CreateUnary(param_shape, HloOpcode::kExp, param0));
  HloInstruction* bitcast2 = builder.AddInstruction(
      HloInstruction::CreateUnary(result_shape, HloOpcode::kBitcast, exp1));
  builder.AddInstruction(
      HloInstruction::CreateUnary(result_shape, HloOpcode::kNegate, bitcast2));

  auto module = CreateNewModule();
  module->AddEntryComputation(builder.Build());

  RunFusionAndCheckOpcodesWereFused(
      module.get(), {HloOpcode::kNegate, HloOpcode::kBitcast, HloOpcode::kExp,
                     HloOpcode::kParameter});
}

TEST_F(OpcodeFusionTest, Broadcast_Bitcast_DynamicSlice_Tanh) {
  HloComputation::Builder builder(TestName());
  Shape param_shape = ShapeUtil::MakeShape(F32, {8});
  Shape starts_shape = ShapeUtil::MakeShape(F32, {2});
  Shape broadcast_shape = ShapeUtil::MakeShape(F32, {1, 8, 8});
  Shape bitcast_shape = ShapeUtil::MakeShape(F32, {8, 8});
  Shape dynamic_slice_shape = ShapeUtil::MakeShape(F32, {4, 4});
  HloInstruction* param0 = builder.AddInstruction(
      HloInstruction::CreateParameter(0, param_shape, "param"));
  HloInstruction* param1 = builder.AddInstruction(
      HloInstruction::CreateParameter(1, starts_shape, "starts"));
  HloInstruction* broadcast2 = builder.AddInstruction(
      HloInstruction::CreateBroadcast(broadcast_shape, param0, {1}));
  HloInstruction* bitcast3 = builder.AddInstruction(HloInstruction::CreateUnary(
      bitcast_shape, HloOpcode::kBitcast, broadcast2));
  HloInstruction* dynamic_slice4 =
      builder.AddInstruction(HloInstruction::CreateDynamicSlice(
          dynamic_slice_shape, bitcast3, param1, {4, 4}));
  builder.AddInstruction(HloInstruction::CreateUnary(
      dynamic_slice_shape, HloOpcode::kTanh, dynamic_slice4));

  auto module = CreateNewModule();
  module->AddEntryComputation(builder.Build());

  RunFusionAndCheckOpcodesWereFused(
      module.get(),
      {HloOpcode::kTanh, HloOpcode::kDynamicSlice, HloOpcode::kBitcast,
       HloOpcode::kBroadcast, HloOpcode::kParameter, HloOpcode::kParameter});
}

TEST_F(OpcodeFusionTest, Broadcast_Negate) {
  HloComputation::Builder builder(TestName());
  Shape param_shape = ShapeUtil::MakeShape(F32, {8});
  Shape result_shape = ShapeUtil::MakeShape(F32, {8, 8});
  HloInstruction* param0 = builder.AddInstruction(
      HloInstruction::CreateParameter(0, param_shape, "param"));
  HloInstruction* broadcast1 = builder.AddInstruction(
      HloInstruction::CreateBroadcast(result_shape, param0, {1}));
  builder.AddInstruction(HloInstruction::CreateUnary(
      result_shape, HloOpcode::kNegate, broadcast1));

  auto module = CreateNewModule();
  module->AddEntryComputation(builder.Build());

  RunFusionAndCheckOpcodesWereFused(
      module.get(),
      {HloOpcode::kNegate, HloOpcode::kBroadcast, HloOpcode::kParameter});
}

TEST_F(OpcodeFusionTest, DynamicSlice_Negate) {
  HloComputation::Builder builder(TestName());
  Shape param_shape = ShapeUtil::MakeShape(F32, {4});
  Shape slice_shape = ShapeUtil::MakeShape(F32, {1});
  Shape result_shape = ShapeUtil::MakeShape(F32, {2});
  HloInstruction* param0 = builder.AddInstruction(
      HloInstruction::CreateParameter(0, param_shape, "param"));
  HloInstruction* param1 = builder.AddInstruction(
      HloInstruction::CreateParameter(1, slice_shape, "starts"));
  HloInstruction* dynamic_slice2 = builder.AddInstruction(
      HloInstruction::CreateDynamicSlice(result_shape, param0, param1, {2}));
  builder.AddInstruction(HloInstruction::CreateUnary(
      result_shape, HloOpcode::kNegate, dynamic_slice2));

  auto module = CreateNewModule();
  module->AddEntryComputation(builder.Build());

  RunFusionAndCheckOpcodesWereFused(
      module.get(), {HloOpcode::kNegate, HloOpcode::kDynamicSlice,
                     HloOpcode::kParameter, HloOpcode::kParameter});
}

TEST_F(OpcodeFusionTest, Exponential_Negate) {
  HloComputation::Builder builder(TestName());
  Shape param_shape = ShapeUtil::MakeShape(F32, {4});
  HloInstruction* param0 = builder.AddInstruction(
      HloInstruction::CreateParameter(0, param_shape, "param"));
  HloInstruction* exp1 = builder.AddInstruction(
      HloInstruction::CreateUnary(param_shape, HloOpcode::kExp, param0));
  builder.AddInstruction(
      HloInstruction::CreateUnary(param_shape, HloOpcode::kNegate, exp1));

  auto module = CreateNewModule();
  module->AddEntryComputation(builder.Build());

  RunFusionAndCheckOpcodesWereFused(
      module.get(),
      {HloOpcode::kNegate, HloOpcode::kExp, HloOpcode::kParameter});
}

TEST_F(OpcodeFusionTest, Reshape_Negate) {
  HloComputation::Builder builder(TestName());
  Shape param_shape = ShapeUtil::MakeShape(F32, {4, 4});
  Shape result_shape = ShapeUtil::MakeShape(F32, {16});
  HloInstruction* param0 = builder.AddInstruction(
      HloInstruction::CreateParameter(0, param_shape, "param"));
  HloInstruction* reshape1 = builder.AddInstruction(
      HloInstruction::CreateReshape(result_shape, param0));
  builder.AddInstruction(
      HloInstruction::CreateUnary(result_shape, HloOpcode::kNegate, reshape1));

  auto module = CreateNewModule();
  module->AddEntryComputation(builder.Build());

  RunFusionAndCheckOpcodesWereFused(
      module.get(),
      {HloOpcode::kNegate, HloOpcode::kReshape, HloOpcode::kParameter});
}

TEST_F(OpcodeFusionTest, Reverse_Negate) {
  HloComputation::Builder builder(TestName());
  Shape param_shape = ShapeUtil::MakeShape(F32, {8});
  HloInstruction* param0 = builder.AddInstruction(
      HloInstruction::CreateParameter(0, param_shape, "param"));
  HloInstruction* reverse1 = builder.AddInstruction(
      HloInstruction::CreateReverse(param_shape, param0, {0}));
  builder.AddInstruction(
      HloInstruction::CreateUnary(param_shape, HloOpcode::kNegate, reverse1));

  auto module = CreateNewModule();
  module->AddEntryComputation(builder.Build());

  RunFusionAndCheckOpcodesWereFused(
      module.get(),
      {HloOpcode::kNegate, HloOpcode::kReverse, HloOpcode::kParameter});
}

TEST_F(OpcodeFusionTest, Slice_Negate) {
  HloComputation::Builder builder(TestName());
  Shape param_shape = ShapeUtil::MakeShape(F32, {4});
  Shape slice_shape = ShapeUtil::MakeShape(F32, {2});
  HloInstruction* param0 = builder.AddInstruction(
      HloInstruction::CreateParameter(0, param_shape, "param"));
  HloInstruction* slice1 = builder.AddInstruction(
      HloInstruction::CreateSlice(slice_shape, param0, {0}, {4}, {2}));
  builder.AddInstruction(HloInstruction::CreateUnary(
      ShapeUtil::MakeShape(S32, {2}), HloOpcode::kNegate, slice1));

  auto module = CreateNewModule();
  module->AddEntryComputation(builder.Build());

  RunFusionAndCheckOpcodesWereFused(
      module.get(),
      {HloOpcode::kNegate, HloOpcode::kSlice, HloOpcode::kParameter});
}

TEST_F(OpcodeFusionTest, Exponential_Transpose_Negate) {
  HloComputation::Builder builder(TestName());
  Shape param_shape = ShapeUtil::MakeShape(F32, {3, 4});
  Shape result_shape = ShapeUtil::MakeShape(F32, {4, 3});
  HloInstruction* param0 = builder.AddInstruction(
      HloInstruction::CreateParameter(0, param_shape, "param"));
  // InstructionFusion::ShouldFuse() precludes fusing a transpose whose operand
  // is a parameter, so create an operand between the parameter and transpose.
  HloInstruction* exp1 = builder.AddInstruction(
      HloInstruction::CreateUnary(param_shape, HloOpcode::kExp, param0));
  HloInstruction* transpose2 = builder.AddInstruction(
      HloInstruction::CreateTranspose(result_shape, exp1, {1, 0}));
  builder.AddInstruction(HloInstruction::CreateUnary(
      result_shape, HloOpcode::kNegate, transpose2));

  auto module = CreateNewModule();
  module->AddEntryComputation(builder.Build());

  RunFusionAndCheckOpcodesWereFused(
      module.get(), {HloOpcode::kNegate, HloOpcode::kTranspose, HloOpcode::kExp,
                     HloOpcode::kParameter});
}

TEST_F(OpcodeFusionTest, UnaryMapOfExp) {
  auto module = CreateNewModule();

  HloComputation::Builder builder(TestName());
  Shape shape = ShapeUtil::MakeShape(F32, {3, 4});
  HloInstruction* param0 = builder.AddInstruction(
      HloInstruction::CreateParameter(0, shape, "param"));

  HloInstruction* exp = builder.AddInstruction(
      HloInstruction::CreateUnary(shape, HloOpcode::kExp, param0));
  builder.AddInstruction(HloInstruction::CreateMap(
      shape, {exp}, CreateAdderToOne(module.get()), /*static_operands=*/{}));

  module->AddEntryComputation(builder.Build());

  RunFusionAndCheckOpcodesWereFused(
      module.get(), {HloOpcode::kParameter, HloOpcode::kExp, HloOpcode::kMap});
}

TEST_F(OpcodeFusionTest, BinaryMapOfExps) {
  auto module = CreateNewModule();

  HloComputation::Builder builder(TestName());
  Shape shape = ShapeUtil::MakeShape(F32, {3, 4});
  HloInstruction* param0 = builder.AddInstruction(
      HloInstruction::CreateParameter(0, shape, "param"));
  HloInstruction* param1 = builder.AddInstruction(
      HloInstruction::CreateParameter(1, shape, "param"));

  HloInstruction* exp0 = builder.AddInstruction(
      HloInstruction::CreateUnary(shape, HloOpcode::kExp, param0));
  HloInstruction* exp1 = builder.AddInstruction(
      HloInstruction::CreateUnary(shape, HloOpcode::kExp, param1));

  builder.AddInstruction(HloInstruction::CreateMap(
      shape, {exp0, exp1}, CreateMax(module.get()), /*static_operands=*/{}));

  module->AddEntryComputation(builder.Build());

  RunFusionAndCheckOpcodesWereFused(
      module.get(), {HloOpcode::kParameter, HloOpcode::kParameter,
                     HloOpcode::kExp, HloOpcode::kExp, HloOpcode::kMap});
}

TEST_F(OpcodeFusionTest, DynamicSliceWithDynamicUpdateSlice) {
  auto module = CreateNewModule();

  HloComputation::Builder builder(TestName());
  Shape full_shape = ShapeUtil::MakeShape(F32, {10, 100, 1000});
  Shape slice_shape = ShapeUtil::MakeShape(F32, {10, 1, 1000});

  HloInstruction* slice =
      builder.AddInstruction(HloInstruction::CreateDynamicSlice(
          slice_shape,
          builder.AddInstruction(
              HloInstruction::CreateParameter(0, full_shape, "slice_from")),
          builder.AddInstruction(HloInstruction::CreateParameter(
              1, ShapeUtil::MakeShape(U32, {3}), "slice_indices")),
          /*slice_sizes=*/{10, 1, 1000}));

  builder.AddInstruction(HloInstruction::CreateDynamicUpdateSlice(
      full_shape,
      builder.AddInstruction(
          HloInstruction::CreateParameter(2, full_shape, "to_update")),
      slice,
      builder.AddInstruction(HloInstruction::CreateParameter(
          3, ShapeUtil::MakeShape(U32, {3}), "update_indices"))));

  module->AddEntryComputation(builder.Build());
  RunFusionAndCheckOpcodesWereFused(
      module.get(), {HloOpcode::kDynamicSlice, HloOpcode::kDynamicUpdateSlice,
                     HloOpcode::kParameter, HloOpcode::kParameter,
                     HloOpcode::kParameter, HloOpcode::kParameter});
}

TEST_F(OpcodeFusionTest, MessOfFusileNodes) {
  auto module = CreateNewModule();
  HloComputation::Builder builder(TestName());

  Shape full_shape = ShapeUtil::MakeShape(F32, {4, 100, 10, 100, 50});

  auto loop_idx = builder.AddInstruction(HloInstruction::CreateReshape(
      ShapeUtil::MakeShape(S32, {1}),
      builder.AddInstruction(HloInstruction::CreateParameter(
          0, ShapeUtil::MakeShape(S32, {}), "param0"))));

  auto param1 = builder.AddInstruction(HloInstruction::CreateParameter(
      1, ShapeUtil::MakeShape(S32, {1}), "param1"));
  auto concat = builder.AddInstruction(HloInstruction::CreateConcatenate(
      ShapeUtil::MakeShape(S32, {5}),
      {loop_idx, param1, param1, param1, param1}, /*dimension=*/0));

  auto idx_choice = builder.AddInstruction(HloInstruction::CreateDynamicSlice(
      ShapeUtil::MakeShape(S32, {1}),
      builder.AddInstruction(HloInstruction::CreateParameter(
          2, ShapeUtil::MakeShape(S32, {4}), "param2")),
      loop_idx,
      /*slice_sizes=*/{1}));

  PaddingConfig padding_config;
  padding_config.add_dimensions()->set_edge_padding_high(4);
  auto pad = builder.AddInstruction(HloInstruction::CreatePad(
      ShapeUtil::MakeShape(S32, {5}), idx_choice,
      builder.AddInstruction(
          HloInstruction::CreateConstant(Literal::CreateR0(0))),
      padding_config));

  auto slice = builder.AddInstruction(HloInstruction::CreateDynamicSlice(
      ShapeUtil::MakeShape(F32, {1, 100, 10, 100, 50}),
      builder.AddInstruction(HloInstruction::CreateParameter(
          3, ShapeUtil::MakeShape(F32, {100, 100, 10, 100, 50}), "param3")),
      pad, /*slice_sizes=*/{1, 100, 10, 100, 50}));

  builder.AddInstruction(HloInstruction::CreateDynamicUpdateSlice(
      full_shape,
      builder.AddInstruction(
          HloInstruction::CreateParameter(4, full_shape, "param4")),
      slice, concat));

  module->AddEntryComputation(builder.Build());
  RunFusionAndCheckOpcodesWereFused(
      module.get(),
      {HloOpcode::kConcatenate, HloOpcode::kPad, HloOpcode::kDynamicSlice,
       HloOpcode::kDynamicSlice, HloOpcode::kDynamicUpdateSlice,
       HloOpcode::kParameter, HloOpcode::kParameter, HloOpcode::kParameter,
       HloOpcode::kParameter, HloOpcode::kParameter, HloOpcode::kParameter});
}

// Tests that we do not fuse instructions in cases where instructions in the
// fusion would reuse elements from its operand due to an implicit broadcast.
TEST_F(OpcodeFusionTest, ReuseViaImplicitBroadcastUnary) {
  Shape small_shape = ShapeUtil::MakeShape(F32, {1, 4});
  Shape large_shape = ShapeUtil::MakeShape(F32, {3, 4});

  HloComputation::Builder builder(TestName());

  HloInstruction* small_param =
      builder.AddInstruction(HloInstruction::CreateParameter(
          /*parameter_number=*/0, small_shape, "param"));
  HloInstruction* small_exp = builder.AddInstruction(
      HloInstruction::CreateUnary(small_shape, HloOpcode::kExp, small_param));
  builder.AddInstruction(
      HloInstruction::CreateUnary(large_shape, HloOpcode::kExp, small_exp));

  std::unique_ptr<HloModule> module = CreateNewModule();
  module->AddEntryComputation(builder.Build());

  auto did_fusion = CpuInstructionFusion().Run(module.get());
  ASSERT_TRUE(did_fusion.ok());
  EXPECT_FALSE(did_fusion.ValueOrDie());
  ASSERT_THAT(module->entry_computation()->root_instruction(),
              Not(op::Fusion()));
}

// Like ReuseViaImplicitBroadcastUnary but with a binary operation.
TEST_F(OpcodeFusionTest, ReuseViaImplicitBroadcastBinary) {
  Shape small_shape = ShapeUtil::MakeShape(F32, {1, 4});
  Shape large_shape = ShapeUtil::MakeShape(F32, {3, 4});

  HloComputation::Builder builder(TestName());

  HloInstruction* small_param =
      builder.AddInstruction(HloInstruction::CreateParameter(
          /*parameter_number=*/0, small_shape, "param"));
  HloInstruction* large_param =
      builder.AddInstruction(HloInstruction::CreateParameter(
          /*parameter_number=*/1, large_shape, "param"));
  HloInstruction* small_exp = builder.AddInstruction(
      HloInstruction::CreateUnary(small_shape, HloOpcode::kExp, small_param));

  builder.AddInstruction(HloInstruction::CreateBinary(
      large_shape, HloOpcode::kAdd, small_exp, large_param));

  std::unique_ptr<HloModule> module = CreateNewModule();
  module->AddEntryComputation(builder.Build());

  auto did_fusion = CpuInstructionFusion().Run(module.get());
  ASSERT_TRUE(did_fusion.ok());
  EXPECT_FALSE(did_fusion.ValueOrDie());
  ASSERT_THAT(module->entry_computation()->root_instruction(),
              Not(op::Fusion()));
}

void CreateComputationForDotAddOutputFusionTest(const string& test_name,
                                                HloModule* module, int m, int k,
                                                int n,
                                                bool add_extra_use_for_dot) {
  HloComputation::Builder builder(test_name);

  Shape dot_lhs_shape = ShapeUtil::MakeShape(F32, {m, k});
  Shape dot_rhs_shape = ShapeUtil::MakeShape(F32, {k, n});
  Shape dot_shape = ShapeUtil::MakeShape(F32, {m, n});

  auto* dot_lhs = builder.AddInstruction(
      HloInstruction::CreateParameter(0, dot_lhs_shape, "param0"));
  auto* dot_rhs = builder.AddInstruction(
      HloInstruction::CreateParameter(1, dot_rhs_shape, "param1"));
  auto* addend = builder.AddInstruction(
      HloInstruction::CreateParameter(2, dot_shape, "param2"));

  auto* dot = builder.AddInstruction(
      HloInstruction::CreateCanonicalDot(dot_shape, dot_lhs, dot_rhs));
  builder.AddInstruction(
      HloInstruction::CreateBinary(dot_shape, HloOpcode::kAdd, dot, addend));

  if (add_extra_use_for_dot) {
    builder.AddInstruction(
        HloInstruction::CreateOutfeed(dot_shape, dot, "no_config"));
  }

  module->AddEntryComputation(builder.Build());
}

TEST_F(OpcodeFusionTest, DotAddOutputFusion_1x50x19) {
  auto module = CreateNewModule();
  CreateComputationForDotAddOutputFusionTest(TestName(), module.get(), /*m=*/1,
                                             /*k=*/50, /*n=*/19,
                                             /*add_extra_use_for_dot=*/false);

  RunFusionAndCheckOpcodesWereFused(
      module.get(),
      {HloOpcode::kDot, HloOpcode::kAdd, HloOpcode::kParameter,
       HloOpcode::kParameter, HloOpcode::kParameter},
      HloInstruction::FusionKind::kOutput);
}

TEST_F(OpcodeFusionTest, DotAddOutputFusion_19x50x1) {
  auto module = CreateNewModule();
  CreateComputationForDotAddOutputFusionTest(TestName(), module.get(), /*m=*/19,
                                             /*k=*/50, /*n=*/1,
                                             /*add_extra_use_for_dot=*/false);

  RunFusionAndCheckOpcodesWereFused(
      module.get(),
      {HloOpcode::kDot, HloOpcode::kAdd, HloOpcode::kParameter,
       HloOpcode::kParameter, HloOpcode::kParameter},
      HloInstruction::FusionKind::kOutput);
}

TEST_F(OpcodeFusionTest, DotAddOutputFusion_19x50x19) {
  auto module = CreateNewModule();
  CreateComputationForDotAddOutputFusionTest(TestName(), module.get(), /*m=*/19,
                                             /*k=*/50, /*n=*/19,
                                             /*add_extra_use_for_dot=*/false);

  TF_ASSERT_OK_AND_ASSIGN(bool fused_something,
                          CpuInstructionFusion().Run(module.get()));
  EXPECT_FALSE(fused_something);
  EXPECT_THAT(module->entry_computation()->root_instruction(),
              Not(op::Fusion()));
}

TEST_F(OpcodeFusionTest, DotAddOutputFusion_19x50x1_multi_use) {
  auto module = CreateNewModule();
  CreateComputationForDotAddOutputFusionTest(TestName(), module.get(), /*m=*/19,
                                             /*k=*/50, /*n=*/1,
                                             /*add_extra_use_for_dot=*/true);

  TF_ASSERT_OK_AND_ASSIGN(bool fused_something,
                          CpuInstructionFusion().Run(module.get()));
  EXPECT_FALSE(fused_something);
  EXPECT_THAT(module->entry_computation()->root_instruction(),
              Not(op::Fusion()));
}

}  // namespace
}  // namespace cpu
}  // namespace xla
