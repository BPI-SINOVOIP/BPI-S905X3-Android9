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

#include "tensorflow/compiler/xla/service/bfloat16_normalization.h"
#include "tensorflow/compiler/xla/service/bfloat16_support.h"
#include "tensorflow/compiler/xla/service/hlo_computation.h"
#include "tensorflow/compiler/xla/service/hlo_instruction.h"
#include "tensorflow/compiler/xla/service/hlo_module.h"
#include "tensorflow/compiler/xla/service/hlo_opcode.h"
#include "tensorflow/compiler/xla/shape_util.h"
#include "tensorflow/compiler/xla/test.h"
#include "tensorflow/compiler/xla/test_helpers.h"
#include "tensorflow/compiler/xla/tests/hlo_test_base.h"
#include "tensorflow/compiler/xla/xla_data.pb.h"

namespace xla {

class TestBFloat16Support : public BFloat16Support {
 public:
  TestBFloat16Support() {}
  ~TestBFloat16Support() override {}

  bool SupportsBF16Operand(const HloInstruction& hlo,
                           int64 operand_index) const override {
    if (hlo.opcode() == HloOpcode::kAdd ||
        hlo.opcode() == HloOpcode::kSubtract ||
        hlo.opcode() == HloOpcode::kReduce ||
        hlo.opcode() == HloOpcode::kTuple ||
        hlo.opcode() == HloOpcode::kGetTupleElement) {
      return true;
    }
    return false;
  }

  bool SupportsBF16Output(const HloInstruction& hlo) const override {
    if (hlo.opcode() == HloOpcode::kAdd || hlo.opcode() == HloOpcode::kReduce ||
        hlo.opcode() == HloOpcode::kSubtract ||
        hlo.opcode() == HloOpcode::kTuple ||
        hlo.opcode() == HloOpcode::kGetTupleElement) {
      return true;
    }
    return false;
  }

  bool SupportsMixedPrecisions(const HloInstruction& hlo) const override {
    if (hlo.opcode() == HloOpcode::kAdd || hlo.opcode() == HloOpcode::kTuple ||
        hlo.opcode() == HloOpcode::kGetTupleElement) {
      return true;
    }
    return false;
  }
};

class BFloat16NormalizationTest : public HloTestBase {
 protected:
  bool Normalize(HloModule* module) {
    TestBFloat16Support bfloat16_support_;
    BFloat16Normalization normalization(&bfloat16_support_);
    StatusOr<bool> result = normalization.Run(module);
    EXPECT_IS_OK(result.status());
    return result.ValueOrDie();
  }
};

TEST_F(BFloat16NormalizationTest, NoopIfSupported) {
  auto builder = HloComputation::Builder(TestName());
  Shape f32_shape = ShapeUtil::MakeShape(F32, {2, 4});
  Shape bf16_shape = ShapeUtil::MakeShape(BF16, {2, 4});

  HloInstruction* a = builder.AddInstruction(
      HloInstruction::CreateParameter(0, f32_shape, "a"));
  HloInstruction* b = builder.AddInstruction(
      HloInstruction::CreateParameter(1, bf16_shape, "b"));
  HloInstruction* c = builder.AddInstruction(
      HloInstruction::CreateParameter(2, f32_shape, "c"));

  HloInstruction* add0 = builder.AddInstruction(
      HloInstruction::CreateBinary(bf16_shape, HloOpcode::kAdd, a, b));

  HloInstruction* add1 = builder.AddInstruction(
      HloInstruction::CreateBinary(f32_shape, HloOpcode::kAdd, add0, c));

  auto module = CreateNewModule();
  auto computation = module->AddEntryComputation(builder.Build());

  EXPECT_FALSE(Normalize(module.get()));

  EXPECT_EQ(computation->root_instruction(), add1);
  EXPECT_EQ(add0->shape().element_type(), BF16);
  EXPECT_EQ(add1->shape().element_type(), F32);
}

TEST_F(BFloat16NormalizationTest, ResolveIfUnsupportedBF16) {
  auto builder = HloComputation::Builder(TestName());
  Shape f32_shape = ShapeUtil::MakeShape(F32, {2, 4});
  Shape bf16_shape = ShapeUtil::MakeShape(BF16, {2, 4});

  HloInstruction* a = builder.AddInstruction(
      HloInstruction::CreateParameter(0, f32_shape, "a"));
  HloInstruction* b = builder.AddInstruction(
      HloInstruction::CreateParameter(1, bf16_shape, "b"));
  HloInstruction* c = builder.AddInstruction(
      HloInstruction::CreateParameter(2, f32_shape, "c"));

  HloInstruction* mul0 = builder.AddInstruction(
      HloInstruction::CreateBinary(bf16_shape, HloOpcode::kMultiply, a, b));

  HloInstruction* mul1 = builder.AddInstruction(
      HloInstruction::CreateBinary(bf16_shape, HloOpcode::kMultiply, mul0, c));

  auto module = CreateNewModule();
  auto computation = module->AddEntryComputation(builder.Build());

  EXPECT_TRUE(Normalize(module.get()));

  EXPECT_EQ(computation->root_instruction()->opcode(), HloOpcode::kConvert);
  EXPECT_EQ(computation->root_instruction()->operand(0), mul1);
  EXPECT_EQ(mul0->shape().element_type(), F32);
  EXPECT_EQ(mul1->shape().element_type(), F32);
  EXPECT_EQ(mul1->operand(0)->opcode(), HloOpcode::kConvert);
}

TEST_F(BFloat16NormalizationTest, ResolveUnsupportedMixedPrecisionSubtraction) {
  auto builder = HloComputation::Builder(TestName());
  Shape f32_shape = ShapeUtil::MakeShape(F32, {2, 4});
  Shape bf16_shape = ShapeUtil::MakeShape(BF16, {2, 4});

  HloInstruction* a = builder.AddInstruction(
      HloInstruction::CreateParameter(0, f32_shape, "a"));
  HloInstruction* b = builder.AddInstruction(
      HloInstruction::CreateParameter(1, bf16_shape, "b"));
  HloInstruction* c = builder.AddInstruction(
      HloInstruction::CreateParameter(2, f32_shape, "c"));

  HloInstruction* sub0 = builder.AddInstruction(
      HloInstruction::CreateBinary(bf16_shape, HloOpcode::kSubtract, a, b));

  HloInstruction* sub1 = builder.AddInstruction(
      HloInstruction::CreateBinary(bf16_shape, HloOpcode::kSubtract, sub0, c));

  auto module = CreateNewModule();
  auto computation = module->AddEntryComputation(builder.Build());

  EXPECT_TRUE(Normalize(module.get()));

  EXPECT_EQ(computation->root_instruction()->opcode(), HloOpcode::kConvert);
  EXPECT_EQ(computation->root_instruction()->operand(0), sub1);
  EXPECT_EQ(sub0->shape().element_type(), F32);
  EXPECT_EQ(sub1->shape().element_type(), F32);
  EXPECT_EQ(sub1->operand(0)->opcode(), HloOpcode::kConvert);
}

TEST_F(BFloat16NormalizationTest, ResolveUnsupportedMixedPrecisionReduce) {
  Shape f32_input_shape = ShapeUtil::MakeShape(F32, {2, 4});
  Shape f32_output_shape = ShapeUtil::MakeShape(F32, {4});

  Shape bf16_scalar_shape = ShapeUtil::MakeShape(BF16, {2, 4});

  auto reduce_comp_builder = HloComputation::Builder("reduce_comp");
  auto reduce_comp_param0 = reduce_comp_builder.AddInstruction(
      HloInstruction::CreateParameter(0, bf16_scalar_shape, "param0"));
  auto reduce_comp_param1 = reduce_comp_builder.AddInstruction(
      HloInstruction::CreateParameter(1, bf16_scalar_shape, "param1"));
  reduce_comp_builder.AddInstruction(
      HloInstruction::CreateBinary(bf16_scalar_shape, HloOpcode::kAdd,
                                   reduce_comp_param0, reduce_comp_param1));

  auto module = CreateNewModule();
  auto reduce_computation =
      module->AddEmbeddedComputation(reduce_comp_builder.Build());

  auto builder = HloComputation::Builder(TestName());
  HloInstruction* input = builder.AddInstruction(
      HloInstruction::CreateParameter(0, f32_input_shape, "a"));
  HloInstruction* init = builder.AddInstruction(
      HloInstruction::CreateParameter(1, bf16_scalar_shape, "init"));
  HloInstruction* reduce = builder.AddInstruction(HloInstruction::CreateReduce(
      f32_output_shape, input, init, {0}, reduce_computation));

  auto computation = module->AddEntryComputation(builder.Build());

  EXPECT_TRUE(Normalize(module.get()));

  EXPECT_EQ(computation->root_instruction(), reduce);
  EXPECT_EQ(reduce->called_computations().size(), 1);
  EXPECT_EQ(reduce->called_computations()[0]->num_parameters(), 2);
  EXPECT_EQ(reduce->called_computations()[0]
                ->parameter_instruction(0)
                ->shape()
                .element_type(),
            F32);
  EXPECT_EQ(reduce->called_computations()[0]
                ->parameter_instruction(1)
                ->shape()
                .element_type(),
            F32);
  EXPECT_EQ(reduce->called_computations()[0]
                ->root_instruction()
                ->shape()
                .element_type(),
            F32);
  EXPECT_EQ(reduce->shape().element_type(), F32);
  EXPECT_EQ(reduce->operand(0), input);
  EXPECT_EQ(input->shape().element_type(), F32);
  EXPECT_EQ(reduce->operand(1)->opcode(), HloOpcode::kConvert);
  EXPECT_EQ(reduce->operand(1)->shape().element_type(), F32);
}

TEST_F(BFloat16NormalizationTest, ResolveMixedPrecisionTupleCrossReplicaSum) {
  auto builder = HloComputation::Builder(TestName());
  Shape f32_shape = ShapeUtil::MakeShape(F32, {2, 4});
  Shape bf16_shape = ShapeUtil::MakeShape(BF16, {2, 4});

  HloInstruction* a = builder.AddInstruction(
      HloInstruction::CreateParameter(0, f32_shape, "a"));
  HloInstruction* b = builder.AddInstruction(
      HloInstruction::CreateParameter(1, bf16_shape, "b"));

  HloInstruction* crs =
      builder.AddInstruction(HloInstruction::CreateCrossReplicaSum(
          ShapeUtil::MakeTupleShape({f32_shape, bf16_shape}), {a, b}));
  HloInstruction* gte = builder.AddInstruction(
      HloInstruction::CreateGetTupleElement(bf16_shape, crs, 1));

  auto module = CreateNewModule();
  auto computation = module->AddEntryComputation(builder.Build());

  EXPECT_TRUE(Normalize(module.get()));

  EXPECT_EQ(computation->root_instruction(), gte);
  EXPECT_EQ(gte->shape().element_type(), BF16);
  EXPECT_EQ(crs->operand(1)->shape().element_type(), F32);
  EXPECT_EQ(ShapeUtil::GetSubshape(crs->shape(), {1}).element_type(), F32);
}

}  // namespace xla
