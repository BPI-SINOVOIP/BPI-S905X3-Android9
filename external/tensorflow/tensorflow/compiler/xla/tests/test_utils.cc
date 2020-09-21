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

#include "tensorflow/compiler/xla/tests/test_utils.h"
#include "tensorflow/compiler/xla/primitive_util.h"
#include "tensorflow/compiler/xla/service/hlo_dataflow_analysis.h"
#include "tensorflow/compiler/xla/service/hlo_verifier.h"
#include "tensorflow/compiler/xla/service/transfer_manager.h"

namespace xla {

namespace {

template <typename FloatT>
void PopulateWithRandomFloatingPointData(Literal* literal,
                                         std::minstd_rand0* engine) {
  CHECK_EQ(literal->shape().element_type(),
           primitive_util::NativeToPrimitiveType<FloatT>());
  // Create uniform numbers between 1 and 1.125 to avoid creating denormal
  // numbers.
  std::uniform_real_distribution<FloatT> generator(1.0f, 1.125f);
  const bool should_index_bias = ShapeUtil::ElementsIn(literal->shape()) > 1000;
  TF_CHECK_OK(literal->Populate<FloatT>(
      [&](tensorflow::gtl::ArraySlice<int64> indices) {
        // Generate a random uniform number from -0.0625 and 0.0625 and bias it
        // with a position dependent number with mean 0.037109375. These number
        // should allow for long chains of accumulation without being too close
        // to zero or too large to accumulate all numbers accurately. Only do
        // this for large literals where the number of elements is much greater
        // than 47 otherwise only negative values are produced.
        //
        // The value is positionally biased using a product of the indices. Add
        // one to each index value to avoid collapsing to zero if any of the
        // indices are zero.
        int64 index_product = 1;
        for (int64 i : indices) {
          index_product *= (1 + i);
        }
        const int64 negative_bias = should_index_bias ? 47 : 0;
        FloatT index_bias =
            static_cast<FloatT>(index_product % 113 - negative_bias) /
            static_cast<FloatT>(256.0f);
        return (generator(*engine) - 1.0625) + index_bias;
      }));
}

// The standard library does not have a case for bfloat16, unsurprisingly, so we
// handle that one specially.
template <>
void PopulateWithRandomFloatingPointData<bfloat16>(Literal* literal,
                                                   std::minstd_rand0* engine) {
  CHECK_EQ(literal->shape().element_type(), BF16);
  std::uniform_real_distribution<float> generator(-0.9f, 1.0f);
  TF_CHECK_OK(literal->Populate<bfloat16>(
      [&](tensorflow::gtl::ArraySlice<int64> /*indices*/) {
        return static_cast<bfloat16>(generator(*engine));
      }));
}

template <typename IntT>
void PopulateWithRandomIntegralData(Literal* literal,
                                    std::minstd_rand0* engine) {
  CHECK_EQ(literal->shape().element_type(),
           primitive_util::NativeToPrimitiveType<IntT>());
  std::uniform_int_distribution<IntT> generator(
      std::numeric_limits<IntT>::lowest(), std::numeric_limits<IntT>::max());
  TF_CHECK_OK(literal->Populate<IntT>(
      [&](tensorflow::gtl::ArraySlice<int64> /*indices*/) {
        return generator(*engine);
      }));
}

// Similar to MakeFakeLiteral but takes a random number generator engine to
// enable reusing the engine across randomly generated literals.
StatusOr<std::unique_ptr<Literal>> MakeFakeLiteralInternal(
    const Shape& shape, std::minstd_rand0* engine) {
  if (ShapeUtil::IsTuple(shape)) {
    std::vector<std::unique_ptr<Literal>> elements;
    for (const Shape& element_shape : shape.tuple_shapes()) {
      TF_ASSIGN_OR_RETURN(std::unique_ptr<Literal> element,
                          MakeFakeLiteralInternal(element_shape, engine));
      elements.push_back(std::move(element));
    }
    return Literal::MakeTupleOwned(std::move(elements));
  }
  std::unique_ptr<Literal> literal = Literal::CreateFromShape(shape);
  switch (shape.element_type()) {
    case BF16:
      PopulateWithRandomFloatingPointData<bfloat16>(literal.get(), engine);
      break;
    case F32:
      PopulateWithRandomFloatingPointData<float>(literal.get(), engine);
      break;
    case F64:
      PopulateWithRandomFloatingPointData<double>(literal.get(), engine);
      break;
    case S8:
      PopulateWithRandomIntegralData<int8>(literal.get(), engine);
      break;
    case U8:
      PopulateWithRandomIntegralData<uint8>(literal.get(), engine);
      break;
    case S16:
      PopulateWithRandomIntegralData<int16>(literal.get(), engine);
      break;
    case U16:
      PopulateWithRandomIntegralData<uint16>(literal.get(), engine);
      break;
    case S32:
      PopulateWithRandomIntegralData<int32>(literal.get(), engine);
      break;
    case U32:
      PopulateWithRandomIntegralData<uint32>(literal.get(), engine);
      break;
    case S64:
      PopulateWithRandomIntegralData<int64>(literal.get(), engine);
      break;
    case U64:
      PopulateWithRandomIntegralData<uint64>(literal.get(), engine);
      break;
    case PRED: {
      std::uniform_int_distribution<int> generator(0, 1);
      TF_CHECK_OK(literal->Populate<bool>(
          [&](tensorflow::gtl::ArraySlice<int64> /*indices*/) {
            return generator(*engine);
          }));
      break;
    }
    default:
      return Unimplemented("Unsupported type for fake literal generation: %s",
                           ShapeUtil::HumanString(shape).c_str());
  }
  return std::move(literal);
}

// Matches binary addition computations.
bool LooksLikeSum(const HloComputation& computation) {
  const HloInstruction* const root = computation.root_instruction();
  return root->opcode() == HloOpcode::kAdd &&
         computation.num_parameters() == 2 &&
         root->operand(0)->opcode() == HloOpcode::kParameter &&
         root->operand(1)->opcode() == HloOpcode::kParameter &&
         root->operand(0) != root->operand(1);
}

// Reduce, ReduceWindow, and SelectAndScatter ops may use binary addition,
// which requires an init_value of 0 rather than a random value.
bool NeedsZeroInitValue(const HloUse& use) {
  const HloInstruction* const instruction = use.instruction;
  const HloOpcode opcode = instruction->opcode();
  const int64 op_num = use.operand_number;
  return (
      ((opcode == HloOpcode::kReduce || opcode == HloOpcode::kReduceWindow) &&
       op_num == 1 && LooksLikeSum(*instruction->to_apply())) ||
      (opcode == HloOpcode::kSelectAndScatter && op_num == 2 &&
       LooksLikeSum(*instruction->scatter())));
}

// Generate random values that are constrained to the input_shape minus the
// output_shape so as not to produce wrapping slices, for instance.
std::unique_ptr<Literal> MakeRandomNonwrappingSliceIndex(
    const Shape& input_shape, const Shape& slice_shape,
    std::minstd_rand0* engine) {
  const int64 rank = ShapeUtil::Rank(input_shape);
  std::vector<int32> start_indices(rank);
  for (int i = 0; i < rank; ++i) {
    const int32 upper_bound = ShapeUtil::GetDimension(input_shape, i) -
                              ShapeUtil::GetDimension(slice_shape, i);
    std::uniform_int_distribution<int32> generator(0, upper_bound);
    start_indices[i] = generator(*engine);
  }
  return Literal::CreateR1<int32>(start_indices);
}

// Use dataflow analysis on each parameter to see if there are uses that would
// be problematic when generating input data.  Returns the list of instructions
// that correspond to their uses.
//
// Should be paired with the CreateLiteralForConstrainedUses() function below.
std::vector<HloInstruction*> FindConstrainedUses(
    const HloDataflowAnalysis& dataflow, const HloInstruction& param) {
  std::vector<HloInstruction*> constrained_uses;
  for (const auto& pair : dataflow.GetInstructionValueSet(&param)) {
    const HloValue& value = dataflow.GetUniqueValueAt(&param, pair.first);
    for (const HloUse& use : value.uses()) {
      HloInstruction* instruction = use.instruction;
      const HloOpcode opcode = instruction->opcode();
      const int64 op_num = use.operand_number;
      if ((opcode == HloOpcode::kDynamicSlice && op_num == 1) ||
          (opcode == HloOpcode::kDynamicUpdateSlice && op_num == 2)) {
        constrained_uses.push_back(instruction);
      } else if (opcode == HloOpcode::kFusion) {
        const HloInstruction* const to_analyze =
            instruction->fused_parameter(op_num);
        auto fused_uses = FindConstrainedUses(dataflow, *to_analyze);
        constrained_uses.insert(constrained_uses.end(), fused_uses.begin(),
                                fused_uses.end());
      } else if (NeedsZeroInitValue(use)) {
        constrained_uses.push_back(instruction);
      } else if (opcode == HloOpcode::kConvert ||
                 opcode == HloOpcode::kReducePrecision) {
        auto converted_uses = FindConstrainedUses(dataflow, *instruction);
        constrained_uses.insert(constrained_uses.end(), converted_uses.begin(),
                                converted_uses.end());
      }
    }
  }
  return constrained_uses;
}

// Given a parameter, generate a random Literal to use as input if there exist
// no constrained uses in the dataflow graph.  If such constraints exist,
// generate a constrained literal (either bounded in the case of indices, or
// zero in the case of init_values for reductions).
StatusOr<std::unique_ptr<Literal>> CreateLiteralForConstrainedUses(
    const tensorflow::gtl::ArraySlice<HloInstruction*> constrained_uses,
    const HloInstruction& param, std::minstd_rand0* engine) {
  HloInstruction* needs_index = nullptr;
  HloInstruction* needs_zero = nullptr;
  for (HloInstruction* use : constrained_uses) {
    switch (use->opcode()) {
      case HloOpcode::kDynamicSlice:
      case HloOpcode::kDynamicUpdateSlice:
        if (needs_index != nullptr &&
            !ShapeUtil::Equal(needs_index->shape(), use->shape())) {
          return Unimplemented(
              "Conflicting operand generation slice index constraints\n");
        }
        needs_index = use;
        break;

      case HloOpcode::kReduce:
      case HloOpcode::kReduceWindow:
      case HloOpcode::kSelectAndScatter:
        needs_zero = use;
        break;

      default:
        return Unimplemented(
            "Constrained operand generation not implemented for %s.",
            use->ToString().c_str());
    }
  }
  if (needs_index != nullptr && needs_zero != nullptr) {
    return Unimplemented(
        "Conflicting operand generation constraints.\nNeeds index: %s\nNeeds "
        "zero: %s\n",
        needs_index->ToString().c_str(), needs_zero->ToString().c_str());
  }
  if (needs_index != nullptr) {
    return MakeRandomNonwrappingSliceIndex(needs_index->operand(0)->shape(),
                                           needs_index->shape(), engine);
  } else if (needs_zero != nullptr) {
    return Literal::CreateFromShape(param.shape());
  } else {
    return MakeFakeLiteralInternal(param.shape(), engine);
  }
}

// Given a module entry parameter, use the dataflow analysis to see if a
// special case literal must be created, or if we can generate fake data.
StatusOr<std::unique_ptr<Literal>> MakeConstrainedArgument(
    const HloDataflowAnalysis& dataflow, const HloInstruction& param,
    std::minstd_rand0* engine) {
  const auto constrained_uses = FindConstrainedUses(dataflow, param);
  return CreateLiteralForConstrainedUses(constrained_uses, param, engine);
}

}  // namespace

StatusOr<std::unique_ptr<Literal>> MakeFakeLiteral(const Shape& shape) {
  std::minstd_rand0 engine;
  return MakeFakeLiteralInternal(shape, &engine);
}

StatusOr<std::vector<std::unique_ptr<Literal>>> MakeFakeArguments(
    HloModule* const module) {
  TF_ASSIGN_OR_RETURN(auto dataflow, HloDataflowAnalysis::Run(*module));
  const auto params = module->entry_computation()->parameter_instructions();
  std::minstd_rand0 engine;
  std::vector<std::unique_ptr<Literal>> arguments(params.size());
  for (int i = 0; i < params.size(); ++i) {
    TF_ASSIGN_OR_RETURN(
        arguments[i], MakeConstrainedArgument(*dataflow, *params[i], &engine));
  }
  return std::move(arguments);
}

Status VerifyHloModule(const perftools::gputools::Platform& platform,
                       HloModule* const module) {
  return HloVerifier().Run(module).status();
}

}  // namespace xla
