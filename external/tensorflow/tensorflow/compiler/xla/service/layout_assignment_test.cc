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

#include "tensorflow/compiler/xla/service/layout_assignment.h"

#include <initializer_list>
#include <memory>
#include <utility>
#include <vector>

#include "tensorflow/compiler/xla/layout_util.h"
#include "tensorflow/compiler/xla/literal_util.h"
#include "tensorflow/compiler/xla/service/algebraic_simplifier.h"
#include "tensorflow/compiler/xla/service/computation_layout.h"
#include "tensorflow/compiler/xla/service/hlo_computation.h"
#include "tensorflow/compiler/xla/service/hlo_instruction.h"
#include "tensorflow/compiler/xla/service/hlo_matchers.h"
#include "tensorflow/compiler/xla/service/hlo_module.h"
#include "tensorflow/compiler/xla/service/hlo_opcode.h"
#include "tensorflow/compiler/xla/shape_layout.h"
#include "tensorflow/compiler/xla/shape_util.h"
#include "tensorflow/compiler/xla/test.h"
#include "tensorflow/compiler/xla/test_helpers.h"
#include "tensorflow/compiler/xla/tests/hlo_test_base.h"
#include "tensorflow/compiler/xla/tests/test_utils.h"
#include "tensorflow/compiler/xla/tools/parser/hlo_parser.h"
#include "tensorflow/compiler/xla/util.h"
#include "tensorflow/compiler/xla/xla_data.pb.h"
#include "tensorflow/core/lib/core/status.h"
#include "tensorflow/core/lib/core/status_test_util.h"
#include "tensorflow/core/lib/gtl/array_slice.h"

namespace op = xla::testing::opcode_matchers;

namespace xla {
namespace {

using ::testing::ElementsAre;

class LayoutAssignmentTest : public HloTestBase {
 protected:
  void AssignLayouts(HloModule* module,
                     ComputationLayout* entry_computation_layout) {
    LayoutAssignment layout_assignment(entry_computation_layout);
    EXPECT_IS_OK(layout_assignment.Run(module).status());
  }
};

TEST_F(LayoutAssignmentTest, ComputationLayout) {
  // Verify the layouts of the root and parameter instructions of a computation
  // match the ComputationLayout for two different layouts.
  std::vector<std::initializer_list<int64>> minor_to_majors = {{0, 1}, {1, 0}};
  for (auto& minor_to_major : minor_to_majors) {
    auto builder = HloComputation::Builder(TestName());
    Shape ashape = ShapeUtil::MakeShape(F32, {42, 12});
    auto param0 = builder.AddInstruction(
        HloInstruction::CreateParameter(0, ashape, "param0"));
    auto param1 = builder.AddInstruction(
        HloInstruction::CreateParameter(1, ashape, "param1"));
    auto add = builder.AddInstruction(
        HloInstruction::CreateBinary(ashape, HloOpcode::kAdd, param0, param1));
    auto module = CreateNewModule();
    HloComputation* computation = module->AddEntryComputation(builder.Build());

    Layout layout = LayoutUtil::MakeLayout(minor_to_major);
    Shape shape(ashape);
    *shape.mutable_layout() = layout;
    const ShapeLayout shape_layout(shape);

    ComputationLayout computation_layout(computation->ComputeProgramShape());
    *computation_layout.mutable_parameter_layout(0) = shape_layout;
    *computation_layout.mutable_parameter_layout(1) = shape_layout;
    *computation_layout.mutable_result_layout() = shape_layout;
    AssignLayouts(module.get(), &computation_layout);
    EXPECT_TRUE(LayoutUtil::Equal(layout, param0->shape().layout()));
    EXPECT_TRUE(LayoutUtil::Equal(layout, param1->shape().layout()));
    EXPECT_TRUE(LayoutUtil::Equal(layout, add->shape().layout()));
  }
}

TEST_F(LayoutAssignmentTest, ComputationLayoutMixedLayout) {
  // Verify the layouts of the root and parameter instructions of a computation
  // match the ComputationLayout which has mixed layout.
  auto builder = HloComputation::Builder(TestName());
  Shape ashape = ShapeUtil::MakeShape(F32, {42, 12});
  auto param0 = builder.AddInstruction(
      HloInstruction::CreateParameter(0, ashape, "param0"));
  auto param1 = builder.AddInstruction(
      HloInstruction::CreateParameter(1, ashape, "param1"));
  builder.AddInstruction(
      HloInstruction::CreateBinary(ashape, HloOpcode::kAdd, param0, param1));
  auto module = CreateNewModule();
  HloComputation* computation = module->AddEntryComputation(builder.Build());

  Layout col_major_layout = LayoutUtil::MakeLayout({1, 0});
  Shape col_major_shape(ashape);
  *col_major_shape.mutable_layout() = col_major_layout;
  const ShapeLayout col_major(col_major_shape);

  Layout row_major_layout = LayoutUtil::MakeLayout({0, 1});
  Shape row_major_shape(ashape);
  *row_major_shape.mutable_layout() = row_major_layout;
  const ShapeLayout row_major(row_major_shape);

  ComputationLayout computation_layout(computation->ComputeProgramShape());
  *computation_layout.mutable_parameter_layout(0) = col_major;
  *computation_layout.mutable_parameter_layout(1) = row_major;
  *computation_layout.mutable_result_layout() = col_major;

  AssignLayouts(module.get(), &computation_layout);
  EXPECT_TRUE(LayoutUtil::Equal(col_major_layout, param0->shape().layout()));
  EXPECT_TRUE(LayoutUtil::Equal(row_major_layout, param1->shape().layout()));
  EXPECT_TRUE(LayoutUtil::Equal(
      col_major_layout, computation->root_instruction()->shape().layout()));
}

TEST_F(LayoutAssignmentTest, FusionInstruction) {
  // Verify that the layout of the fused parameters in a fusion instruction
  // match that of the fusion operands. Other fused instructions should have no
  // layout.
  std::vector<std::initializer_list<int64>> minor_to_majors = {{0, 1}, {1, 0}};
  for (auto& minor_to_major : minor_to_majors) {
    auto builder = HloComputation::Builder(TestName());
    auto constant_literal1 = Literal::CreateR2WithLayout<float>(
        {{1.0, 2.0}, {3.0, 4.0}}, LayoutUtil::MakeLayout(minor_to_major));
    auto constant_literal2 = Literal::CreateR2WithLayout<float>(
        {{5.0, 6.0}, {7.0, 8.0}}, LayoutUtil::MakeLayout(minor_to_major));
    Shape ashape = constant_literal1->shape();

    auto constant1 = builder.AddInstruction(
        HloInstruction::CreateConstant(std::move(constant_literal1)));
    auto constant2 = builder.AddInstruction(
        HloInstruction::CreateConstant(std::move(constant_literal2)));
    auto add = builder.AddInstruction(HloInstruction::CreateBinary(
        ashape, HloOpcode::kAdd, constant1, constant2));
    auto negate1 = builder.AddInstruction(
        HloInstruction::CreateUnary(ashape, HloOpcode::kNegate, add));
    auto negate2 = builder.AddInstruction(
        HloInstruction::CreateUnary(ashape, HloOpcode::kNegate, negate1));

    auto module = CreateNewModule();
    HloComputation* computation = module->AddEntryComputation(builder.Build());

    auto fusion = computation->CreateFusionInstruction(
        {negate2, negate1, add}, HloInstruction::FusionKind::kLoop);

    Layout layout = LayoutUtil::MakeLayout(minor_to_major);
    Shape shape(ashape);
    *shape.mutable_layout() = layout;
    const ShapeLayout shape_layout(shape);

    ComputationLayout computation_layout(computation->ComputeProgramShape());
    *computation_layout.mutable_result_layout() = shape_layout;

    AssignLayouts(module.get(), &computation_layout);

    EXPECT_TRUE(LayoutUtil::Equal(
        layout, fusion->fused_parameter(0)->shape().layout()));
    EXPECT_TRUE(LayoutUtil::Equal(
        layout, fusion->fused_parameter(1)->shape().layout()));
    EXPECT_TRUE(LayoutUtil::Equal(
        layout, fusion->fused_expression_root()->shape().layout()));

    // Inner fused node should not have layout.
    EXPECT_FALSE(LayoutUtil::HasLayout(
        fusion->fused_expression_root()->operand(0)->shape()));
  }
}

TEST_F(LayoutAssignmentTest, TupleLayout) {
  // Verify the layouts of a tuple are assigned properly (the element layouts
  // match their source).
  auto builder = HloComputation::Builder(TestName());
  auto constant0 = builder.AddInstruction(
      HloInstruction::CreateConstant(Literal::CreateR2WithLayout<float>(
          {{1.0, 2.0}, {3.0, 4.0}}, LayoutUtil::MakeLayout({0, 1}))));
  auto constant1 = builder.AddInstruction(
      HloInstruction::CreateConstant(Literal::CreateR2WithLayout<float>(
          {{1.0, 2.0}, {3.0, 4.0}}, LayoutUtil::MakeLayout({1, 0}))));
  auto tuple = builder.AddInstruction(
      HloInstruction::CreateTuple({constant0, constant1}));

  // To avoid having to construct a tuple layout in the ComputationLayout below,
  // make the result of the instruction be an array.
  auto get_element0 = builder.AddInstruction(
      HloInstruction::CreateGetTupleElement(constant0->shape(), tuple, 0));
  auto negate = builder.AddInstruction(HloInstruction::CreateUnary(
      constant0->shape(), HloOpcode::kNegate, get_element0));

  auto module = CreateNewModule();
  module->AddEntryComputation(builder.Build());

  ComputationLayout computation_layout(
      module->entry_computation()->ComputeProgramShape());

  AssignLayouts(module.get(), &computation_layout);

  EXPECT_TRUE(
      LayoutUtil::LayoutsInShapesEqual(constant0->shape(), constant1->shape()));

  EXPECT_TRUE(LayoutUtil::HasLayout(tuple->shape()));
  EXPECT_TRUE(LayoutUtil::LayoutsInShapesEqual(
      negate->shape(), computation_layout.result_layout().shape()));
  EXPECT_TRUE(LayoutUtil::LayoutsInShapesEqual(
      ShapeUtil::GetTupleElementShape(tuple->shape(), 1), constant1->shape()));
}

TEST_F(LayoutAssignmentTest, TupleSelect) {
  // Verify layouts of a select with tuple operands is assigned properly.
  auto builder = HloComputation::Builder(TestName());
  auto constant0 = builder.AddInstruction(
      HloInstruction::CreateConstant(Literal::CreateR2WithLayout<float>(
          {{1.0, 2.0}, {3.0, 4.0}}, LayoutUtil::MakeLayout({0, 1}))));
  auto constant1 = builder.AddInstruction(
      HloInstruction::CreateConstant(Literal::CreateR2WithLayout<float>(
          {{1.0, 2.0}, {3.0, 4.0}}, LayoutUtil::MakeLayout({1, 0}))));
  auto tuple0 = builder.AddInstruction(
      HloInstruction::CreateTuple({constant0, constant1}));
  auto tuple1 = builder.AddInstruction(
      HloInstruction::CreateTuple({constant0, constant1}));

  auto pred = builder.AddInstruction(
      HloInstruction::CreateConstant(Literal::CreateR0<bool>(true)));

  auto select = builder.AddInstruction(HloInstruction::CreateTernary(
      tuple0->shape(), HloOpcode::kSelect, pred, tuple0, tuple1));

  auto module = CreateNewModule();
  module->AddEntryComputation(builder.Build());

  ComputationLayout computation_layout(
      module->entry_computation()->ComputeProgramShape());
  Shape result_shape =
      ShapeUtil::MakeTupleShape({constant0->shape(), constant1->shape()});
  TF_CHECK_OK(computation_layout.mutable_result_layout()->CopyLayoutFromShape(
      result_shape));

  AssignLayouts(module.get(), &computation_layout);

  EXPECT_TRUE(LayoutUtil::LayoutsInShapesEqual(result_shape, select->shape()));
}

TEST_F(LayoutAssignmentTest, ConflictingLayoutTuple) {
  // Construct following computation which has conflicting layouts for two
  // elements of a tuple which share the same source logicalb buffer:
  //
  // %constant = Constant(...)
  // %inner_tuple = Tuple(%constant)
  // %nested_tuple = Tuple(%inner_tuple, %inner_tuple)
  //
  // Result layout col-major for the first element and row-major for the
  // second. This results in the conflict where the element of the inner_tuple
  // needs to be both col and row major. This is resolved by deep-copying the
  // tuple and assigning the layouts of the copied arrays as needed.
  auto builder = HloComputation::Builder(TestName());
  auto constant = builder.AddInstruction(HloInstruction::CreateConstant(
      Literal::CreateR2<float>({{1.0, 2.0}, {3.0, 4.0}})));
  auto inner_tuple =
      builder.AddInstruction(HloInstruction::CreateTuple({constant}));
  auto nested_tuple = builder.AddInstruction(
      HloInstruction::CreateTuple({inner_tuple, inner_tuple}));

  auto module = CreateNewModule();
  module->AddEntryComputation(builder.Build());

  ComputationLayout computation_layout(
      module->entry_computation()->ComputeProgramShape());
  Shape result_shape = nested_tuple->shape();
  *ShapeUtil::GetMutableSubshape(&result_shape, /*index=*/{0, 0}) =
      ShapeUtil::MakeShapeWithLayout(F32, {2, 2}, {1, 0});
  *ShapeUtil::GetMutableSubshape(&result_shape, /*index=*/{1, 0}) =
      ShapeUtil::MakeShapeWithLayout(F32, {2, 2}, {0, 1});
  TF_CHECK_OK(computation_layout.mutable_result_layout()->CopyLayoutFromShape(
      result_shape));

  LayoutAssignment layout_assignment(&computation_layout);
  AssignLayouts(module.get(), &computation_layout);

  // Layout assignment should have deep copied the result of the computation to
  // address the layout conflict. This results in several Tuple() and
  // GetTupleElement() instructions. Running algebraic simplification should
  // clean up the code to something like:
  //
  //  %constant = Constant(...) layout={1,0}
  //  %tuple.0 = Tuple(%constant) layout=({1,0})
  //  %copy = Copy(%constant) layout={0,1}  # layout transposed
  //  %tuple.1 = Tuple(%copy) layout=({0,1})
  //  %tuple.2 = Tuple(%tuple.0, %tuple.1) layout=(({1,0}), ({0,1}))
  //
  EXPECT_TRUE(
      AlgebraicSimplifier(/*is_layout_sensitive=*/true,
                          [](const Shape&, const Shape&) { return false; })
          .Run(module.get())
          .ValueOrDie());
  HloInstruction* root = module->entry_computation()->root_instruction();
  // Verify layout of the root and the root's operands.
  EXPECT_TRUE(ShapeUtil::Equal(result_shape, root->shape()));
  EXPECT_TRUE(ShapeUtil::Equal(ShapeUtil::GetSubshape(result_shape, {0}),
                               root->operand(0)->shape()));
  EXPECT_TRUE(ShapeUtil::Equal(ShapeUtil::GetSubshape(result_shape, {1}),
                               root->operand(1)->shape()));

  // Verify the structure of the HLO graph.
  EXPECT_THAT(root,
              op::Tuple(op::Tuple(constant), op::Tuple(op::Copy(constant))));
}

TEST_F(LayoutAssignmentTest, ElementwiseAndReshape) {
  // param -> log -> reshape -> tanh
  auto builder = HloComputation::Builder(TestName());
  Shape ashape = ShapeUtil::MakeShape(F32, {1, 2, 3, 1});
  Shape bshape = ShapeUtil::MakeShape(F32, {3, 1, 2});
  auto param = builder.AddInstruction(
      HloInstruction::CreateParameter(0, ashape, "param"));
  auto log = builder.AddInstruction(
      HloInstruction::CreateUnary(ashape, HloOpcode::kLog, param));
  auto reshape =
      builder.AddInstruction(HloInstruction::CreateReshape(bshape, log));
  auto tanh = builder.AddInstruction(
      HloInstruction::CreateUnary(bshape, HloOpcode::kTanh, reshape));

  auto module = CreateNewModule();
  HloComputation* computation =
      module->AddEntryComputation(builder.Build(tanh));

  Shape ashape_with_layout(ashape);
  Shape bshape_with_layout(bshape);
  *ashape_with_layout.mutable_layout() = LayoutUtil::MakeLayout({0, 2, 1, 3});
  *bshape_with_layout.mutable_layout() = LayoutUtil::MakeLayout({2, 1, 0});

  ComputationLayout computation_layout(computation->ComputeProgramShape());
  *computation_layout.mutable_parameter_layout(0) =
      ShapeLayout(ashape_with_layout);
  *computation_layout.mutable_result_layout() = ShapeLayout(bshape_with_layout);
  AssignLayouts(module.get(), &computation_layout);

  auto log_minor_to_major =
      AsInt64Slice(log->shape().layout().minor_to_major());
  EXPECT_GT(PositionInContainer(log_minor_to_major, 1),
            PositionInContainer(log_minor_to_major, 2));

  auto reshape_minor_to_major =
      AsInt64Slice(reshape->shape().layout().minor_to_major());
  EXPECT_GT(PositionInContainer(reshape_minor_to_major, 0),
            PositionInContainer(reshape_minor_to_major, 2));
}

// Test whether LayoutAssignment assigns layouts to elementwise operations to
// keep linear indices valid across them, and to transpositions to make them
// bitcasts.
TEST_F(LayoutAssignmentTest, ElementwiseAndTranspose) {
  // param -> log -> transpose -> tanh
  auto builder = HloComputation::Builder(TestName());
  Shape ashape = ShapeUtil::MakeShape(F32, {42, 12});
  Shape bshape = ShapeUtil::MakeShape(F32, {12, 42});
  auto param = builder.AddInstruction(
      HloInstruction::CreateParameter(0, ashape, "param"));
  auto log = builder.AddInstruction(
      HloInstruction::CreateUnary(ashape, HloOpcode::kLog, param));
  auto transpose = builder.AddInstruction(
      HloInstruction::CreateTranspose(bshape, log, {1, 0}));
  auto tanh = builder.AddInstruction(
      HloInstruction::CreateUnary(bshape, HloOpcode::kTanh, transpose));
  auto module = CreateNewModule();
  auto computation = module->AddEntryComputation(builder.Build(tanh));

  Shape ashape_with_layout(ashape);
  Shape bshape_with_layout(bshape);
  *ashape_with_layout.mutable_layout() = LayoutUtil::MakeLayout({1, 0});
  *bshape_with_layout.mutable_layout() = LayoutUtil::MakeLayout({0, 1});

  ComputationLayout computation_layout(computation->ComputeProgramShape());
  *computation_layout.mutable_parameter_layout(0) =
      ShapeLayout(ashape_with_layout);
  *computation_layout.mutable_result_layout() = ShapeLayout(bshape_with_layout);
  AssignLayouts(module.get(), &computation_layout);

  EXPECT_TRUE(
      LayoutUtil::Equal(ashape_with_layout.layout(), log->shape().layout()));
  EXPECT_TRUE(LayoutUtil::Equal(bshape_with_layout.layout(),
                                transpose->shape().layout()));
  EXPECT_TRUE(
      LayoutUtil::Equal(bshape_with_layout.layout(), tanh->shape().layout()));
}

// Test whether LayoutAssignment assigns layouts to transpositions to make them
// bitcasts.
TEST_F(LayoutAssignmentTest, BroadcastAndTranspose) {
  // param -> broadcast -> transpose
  auto builder = HloComputation::Builder(TestName());
  Shape ashape = ShapeUtil::MakeShape(F32, {3, 4});
  Shape bshape = ShapeUtil::MakeShape(F32, {2, 3, 4});
  Shape cshape = ShapeUtil::MakeShape(F32, {4, 3, 2});
  auto param = builder.AddInstruction(
      HloInstruction::CreateParameter(0, ashape, "param"));
  auto broadcast = builder.AddInstruction(
      HloInstruction::CreateBroadcast(bshape, param, {1, 2}));
  auto transpose = builder.AddInstruction(
      HloInstruction::CreateTranspose(cshape, broadcast, {2, 1, 0}));
  auto module = CreateNewModule();
  HloComputation* computation =
      module->AddEntryComputation(builder.Build(transpose));

  Shape input_shape_with_layout(ashape);
  Shape output_shape_with_layout(cshape);
  *input_shape_with_layout.mutable_layout() = LayoutUtil::MakeLayout({1, 0});
  *output_shape_with_layout.mutable_layout() =
      LayoutUtil::MakeLayout({2, 1, 0});

  ComputationLayout computation_layout(computation->ComputeProgramShape());
  *computation_layout.mutable_parameter_layout(0) =
      ShapeLayout(input_shape_with_layout);
  *computation_layout.mutable_result_layout() =
      ShapeLayout(output_shape_with_layout);
  AssignLayouts(module.get(), &computation_layout);

  EXPECT_THAT(broadcast->shape().layout().minor_to_major(),
              ElementsAre(0, 1, 2));
}

TEST_F(LayoutAssignmentTest, ReshapeOperandHasMultipleUsers) {
  // param[4] -> broadcast[3x4] ------> transpose[4x3]-------- -------> tuple
  //                            \                                     /
  //                             \-> tanh[3x4] -> broadcast2[2x3x4] -/
  //
  // The layout of `transpose` is set to {1,0} because it provides a buffer to
  // the computation result which has a fixed layout.. Therefore, `broadcast`
  // (the operand of transpose) is expected to have layout {0,1} so that the
  // transpose is a bitcast. Furthermore, `tanh` is expected to have the same
  // layout as `broadcast` (i.e. {0,1}) because `tanh` is elementwise.
  Shape f32_4 = ShapeUtil::MakeShape(F32, {4});
  Shape f32_34 = ShapeUtil::MakeShape(F32, {3, 4});
  Shape f32_43 = ShapeUtil::MakeShape(F32, {4, 3});
  Shape f32_234 = ShapeUtil::MakeShape(F32, {2, 3, 4});

  auto builder = HloComputation::Builder(TestName());
  auto param = builder.AddInstruction(
      HloInstruction::CreateParameter(0, f32_4, "param"));
  auto broadcast = builder.AddInstruction(
      HloInstruction::CreateBroadcast(f32_34, param, {3}));
  auto transpose = builder.AddInstruction(
      HloInstruction::CreateTranspose(f32_43, broadcast, {1, 0}));
  auto tanh = builder.AddInstruction(
      HloInstruction::CreateUnary(f32_34, HloOpcode::kTanh, broadcast));
  auto broadcast2 = builder.AddInstruction(
      HloInstruction::CreateBroadcast(f32_234, tanh, {2}));
  auto tuple = builder.AddInstruction(
      HloInstruction::CreateTuple({transpose, broadcast2}));
  auto module = CreateNewModule();
  HloComputation* computation =
      module->AddEntryComputation(builder.Build(tuple));

  ComputationLayout computation_layout(computation->ComputeProgramShape());
  Shape param_shape_with_layout(f32_4);
  Shape transpose_shape_with_layout(f32_43);
  Shape broadcast2_shape_with_layout(f32_234);
  *param_shape_with_layout.mutable_layout() = LayoutUtil::MakeLayout({0});
  *transpose_shape_with_layout.mutable_layout() =
      LayoutUtil::MakeLayout({1, 0});
  *broadcast2_shape_with_layout.mutable_layout() =
      LayoutUtil::MakeLayout({2, 1, 0});

  *computation_layout.mutable_parameter_layout(0) =
      ShapeLayout(param_shape_with_layout);
  *computation_layout.mutable_result_layout() =
      ShapeLayout(ShapeUtil::MakeTupleShape(
          {transpose_shape_with_layout, broadcast2_shape_with_layout}));
  AssignLayouts(module.get(), &computation_layout);

  EXPECT_THAT(broadcast->shape().layout().minor_to_major(), ElementsAre(0, 1));
  EXPECT_THAT(transpose->shape().layout().minor_to_major(), ElementsAre(1, 0));
  EXPECT_THAT(tanh->shape().layout().minor_to_major(), ElementsAre(0, 1));
}

class OperandsMustBeTheSameLayoutAssignment : public LayoutAssignment {
 public:
  explicit OperandsMustBeTheSameLayoutAssignment(
      ComputationLayout* entry_computation_layout)
      : LayoutAssignment(entry_computation_layout) {}

 protected:
  Status PropagateBufferConstraint(
      const BufferLayoutConstraint& buffer_constraint,
      LayoutConstraints* constraints) override {
    const LogicalBuffer& buffer = buffer_constraint.buffer();
    const HloInstruction* instruction = buffer.instruction();

    // Force the operands' layout to the output layout.
    for (int64 operand_no = 0; operand_no < instruction->operand_count();
         ++operand_no) {
      const HloInstruction* operand = instruction->operand(operand_no);
      if (ShapeUtil::Rank(instruction->shape()) !=
          ShapeUtil::Rank(operand->shape())) {
        continue;
      }
      TF_RETURN_IF_ERROR(constraints->SetArrayOperandLayout(
          buffer_constraint.layout(), instruction, operand_no,
          /*mandatory=*/true));
    }
    return PropagateBufferConstraintToUses(buffer_constraint, constraints);
  }
};

TEST_F(LayoutAssignmentTest, MakeOperandsTheSame) {
  // param0 -> concatenate -> reshape
  // param1   -^
  auto builder = HloComputation::Builder(TestName());
  Shape ashape = ShapeUtil::MakeShape(F32, {50, 1});
  Shape bshape = ShapeUtil::MakeShape(F32, {50, 2});
  Shape cshape = ShapeUtil::MakeShape(F32, {100});
  auto param0 = builder.AddInstruction(
      HloInstruction::CreateParameter(0, ashape, "param"));
  auto param1 = builder.AddInstruction(
      HloInstruction::CreateParameter(1, ashape, "param"));
  auto concatenate = builder.AddInstruction(
      HloInstruction::CreateConcatenate(bshape, {param0, param1}, 1));
  auto reshape = builder.AddInstruction(
      HloInstruction::CreateReshape(cshape, concatenate));
  auto module = CreateNewModule();
  HloComputation* computation =
      module->AddEntryComputation(builder.Build(reshape));

  Shape param0_shape_with_layout(ashape);
  Shape param1_shape_with_layout(ashape);
  *param0_shape_with_layout.mutable_layout() = LayoutUtil::MakeLayout({0, 1});
  *param1_shape_with_layout.mutable_layout() = LayoutUtil::MakeLayout({1, 0});

  ComputationLayout computation_layout(computation->ComputeProgramShape());
  *computation_layout.mutable_parameter_layout(0) =
      ShapeLayout(param0_shape_with_layout);
  *computation_layout.mutable_parameter_layout(1) =
      ShapeLayout(param1_shape_with_layout);
  OperandsMustBeTheSameLayoutAssignment layout_assignment(&computation_layout);
  EXPECT_IS_OK(layout_assignment.Run(module.get()).status());

  EXPECT_EQ(HloOpcode::kCopy, concatenate->operand(0)->opcode());
  EXPECT_THAT(concatenate->operand(0)->shape().layout().minor_to_major(),
              ElementsAre(1, 0));
  EXPECT_THAT(concatenate->operand(1)->shape().layout().minor_to_major(),
              ElementsAre(1, 0));
  EXPECT_THAT(concatenate->shape().layout().minor_to_major(),
              ElementsAre(1, 0));
}

// Test layout assignment of a transpose into a bitcast based on its operand.
TEST_F(LayoutAssignmentTest, TransposeToBitcastFromOperand) {
  auto builder = HloComputation::Builder(TestName());
  Shape input_shape_with_layout =
      ShapeUtil::MakeShapeWithLayout(F32, {3, 5, 6, 7}, {2, 0, 3, 1});
  auto param = builder.AddInstruction(
      HloInstruction::CreateParameter(0, input_shape_with_layout, "param"));
  auto transpose = builder.AddInstruction(HloInstruction::CreateTranspose(
      ShapeUtil::MakeShape(F32, {6, 7, 3, 5}), param, {2, 3, 0, 1}));
  auto module = CreateNewModule();
  HloComputation* computation =
      module->AddEntryComputation(builder.Build(transpose));
  ComputationLayout computation_layout(computation->ComputeProgramShape());
  AssignLayouts(module.get(), &computation_layout);
  EXPECT_TRUE(ShapeUtil::TransposeIsBitcast(transpose->operand(0)->shape(),
                                            transpose->shape(), {2, 3, 0, 1}));
}
// Test layout assignment of a transpose into a bitcast based on its user.
TEST_F(LayoutAssignmentTest, TransposeToBitcastToUser) {
  auto builder = HloComputation::Builder(TestName());
  Shape input_shape = ShapeUtil::MakeShape(F32, {3, 5, 6, 7});
  auto constant = builder.AddInstruction(
      HloInstruction::CreateConstant(Literal::CreateR0<float>(1.0f)));
  auto broadcast = builder.AddInstruction(
      HloInstruction::CreateBroadcast(input_shape, constant, {}));
  auto transpose = builder.AddInstruction(HloInstruction::CreateTranspose(
      ShapeUtil::MakeShape(F32, {6, 7, 3, 5}), broadcast, {2, 3, 0, 1}));
  auto module = CreateNewModule();
  HloComputation* computation =
      module->AddEntryComputation(builder.Build(transpose));
  ComputationLayout computation_layout(computation->ComputeProgramShape());
  AssignLayouts(module.get(), &computation_layout);
  EXPECT_TRUE(ShapeUtil::TransposeIsBitcast(transpose->operand(0)->shape(),
                                            transpose->shape(), {2, 3, 0, 1}));
}

// A GTE inside of a fusion node inherits the layout of its operand (which
// should, if we keep following operands, eventually be a parameter).
TEST_F(LayoutAssignmentTest, GTEInheritsLayoutFromOperand) {
  const char* module_str = R"(
    HloModule test_module

    fused_computation {
      fparam = (f32[2,2,2], (f32[2,2,2], f32[2,2,2])) parameter(0)
      gte0 = f32[2,2,2] get-tuple-element(fparam), index=0
      gte1 = (f32[2,2,2], f32[2,2,2]) get-tuple-element(fparam), index=1
      gte1a = f32[2,2,2] get-tuple-element(gte1), index=0
      gte1b = f32[2,2,2] get-tuple-element(gte1), index=1
      add = f32[2,2,2] add(gte1a, gte1b)
      ROOT fresult = f32[2,2,2] add(gte0, add)
    }

    ENTRY entry_computation {
      param = (f32[2,2,2], (f32[2,2,2], f32[2,2,2])) parameter(0)
      ROOT fusion =
        f32[2,2,2] fusion(param), kind=kLoop, calls=fused_computation
    }
  )";

  auto module = tools::Parse(module_str).ValueOrDie();
  ComputationLayout computation_layout(
      module->entry_computation()->ComputeProgramShape());
  Shape param_shape = ShapeUtil::MakeTupleShape(
      {ShapeUtil::MakeShapeWithLayout(F32, {2, 2, 2}, {0, 1, 2}),
       ShapeUtil::MakeTupleShape({
           ShapeUtil::MakeShapeWithLayout(F32, {2, 2, 2}, {1, 2, 0}),
           ShapeUtil::MakeShapeWithLayout(F32, {2, 2, 2}, {2, 0, 1}),
       })});
  TF_ASSERT_OK(
      computation_layout.mutable_parameter_layout(0)->CopyLayoutFromShape(
          param_shape));
  computation_layout.mutable_result_layout()->ResetLayout(
      LayoutUtil::MakeLayout({2, 1, 0}));
  AssignLayouts(module.get(), &computation_layout);

  HloComputation* fused_computation = *std::find_if(
      module->computations().begin(), module->computations().end(),
      [](const HloComputation* c) { return c->name() == "fused_computation"; });

  auto fused_instr = [&](const string& name) {
    auto it = std::find_if(
        fused_computation->instructions().begin(),
        fused_computation->instructions().end(),
        [&](const HloInstruction* i) { return i->name() == name; });
    CHECK(it != fused_computation->instructions().end());
    return *it;
  };

  EXPECT_THAT(fused_instr("gte0")->shape().layout().minor_to_major(),
              ElementsAre(0, 1, 2));
  EXPECT_THAT(
      fused_instr("gte1")->shape().tuple_shapes(0).layout().minor_to_major(),
      ElementsAre(1, 2, 0));
  EXPECT_THAT(
      fused_instr("gte1")->shape().tuple_shapes(1).layout().minor_to_major(),
      ElementsAre(2, 0, 1));
  EXPECT_THAT(fused_instr("gte1a")->shape().layout().minor_to_major(),
              ElementsAre(1, 2, 0));
  EXPECT_THAT(fused_instr("gte1b")->shape().layout().minor_to_major(),
              ElementsAre(2, 0, 1));
  EXPECT_THAT(fused_instr("fresult")->shape().layout().minor_to_major(),
              ElementsAre(2, 1, 0));
}

TEST_F(LayoutAssignmentTest, ConditionalAsymmetricLayout) {
  auto builder = HloComputation::Builder(TestName());
  auto module = CreateNewModule();
  Shape shape = ShapeUtil::MakeShape(F32, {128, 8});
  Shape tshape = ShapeUtil::MakeTupleShape({shape, shape});
  Shape result_tshape = ShapeUtil::MakeTupleShape({shape});

  auto param0 = builder.AddInstruction(
      HloInstruction::CreateParameter(0, shape, "param0"));
  auto param1 = builder.AddInstruction(
      HloInstruction::CreateParameter(1, shape, "param1"));
  auto pred = builder.AddInstruction(HloInstruction::CreateParameter(
      2, ShapeUtil::MakeShape(PRED, {}), "param2"));
  auto tuple =
      builder.AddInstruction(HloInstruction::CreateTuple({param0, param1}));

  auto true_builder = HloComputation::Builder(TestName() + "_TrueBranch");
  {
    auto param = true_builder.AddInstruction(
        HloInstruction::CreateParameter(0, tshape, "param"));
    auto gte0 = true_builder.AddInstruction(
        HloInstruction::CreateGetTupleElement(shape, param, 0));
    auto gte1 = true_builder.AddInstruction(
        HloInstruction::CreateGetTupleElement(shape, param, 1));
    auto add = true_builder.AddInstruction(
        HloInstruction::CreateBinary(shape, HloOpcode::kAdd, gte0, gte1));
    true_builder.AddInstruction(HloInstruction::CreateTuple({add}));
  }
  HloComputation* true_computation =
      module->AddEmbeddedComputation(true_builder.Build());

  auto false_builder = HloComputation::Builder(TestName() + "_FalseBranch");
  {
    Shape xshape = ShapeUtil::MakeShapeWithLayout(F32, {128, 8}, {0, 1});
    false_builder.AddInstruction(
        HloInstruction::CreateParameter(0, tshape, "param"));
    // Using infeed as layout assignment does not mess up with it.
    auto infeed =
        false_builder.AddInstruction(HloInstruction::CreateInfeed(xshape, ""));
    false_builder.AddInstruction(HloInstruction::CreateTuple({infeed}));
  }
  HloComputation* false_computation =
      module->AddEmbeddedComputation(false_builder.Build());
  builder.AddInstruction(HloInstruction::CreateConditional(
      result_tshape, pred, tuple, true_computation, tuple, false_computation));

  HloComputation* computation = module->AddEntryComputation(builder.Build());
  ComputationLayout computation_layout(computation->ComputeProgramShape());

  AssignLayouts(module.get(), &computation_layout);

  const HloInstruction* true_root = true_computation->root_instruction();
  const HloInstruction* false_root = false_computation->root_instruction();
  EXPECT_THAT(true_root->opcode(), HloOpcode::kTuple);
  EXPECT_THAT(false_root->opcode(), HloOpcode::kTuple);

  const HloInstruction* true_result = true_root->operand(0);
  const HloInstruction* false_result = false_root->operand(0);
  EXPECT_TRUE(LayoutUtil::Equal(true_result->shape().layout(),
                                false_result->shape().layout()));
  EXPECT_THAT(false_result->opcode(), HloOpcode::kCopy);
}

}  // namespace
}  // namespace xla
