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

#include "tensorflow/compiler/xla/service/gpu/ir_emitter.h"

#include <string>
#include <unordered_map>

#include "tensorflow/core/platform/logging.h"
// IWYU pragma: no_include "llvm/IR/Intrinsics.gen.inc"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "tensorflow/compiler/xla/primitive_util.h"
#include "tensorflow/compiler/xla/service/elemental_ir_emitter.h"
#include "tensorflow/compiler/xla/service/gpu/elemental_ir_emitter.h"
#include "tensorflow/compiler/xla/service/gpu/ir_emitter_nested.h"
#include "tensorflow/compiler/xla/service/gpu/ir_emitter_unnested.h"
#include "tensorflow/compiler/xla/service/gpu/partition_assignment.h"
#include "tensorflow/compiler/xla/service/hlo_computation.h"
#include "tensorflow/compiler/xla/service/llvm_ir/fused_ir_emitter.h"
#include "tensorflow/compiler/xla/service/llvm_ir/ir_array.h"
#include "tensorflow/compiler/xla/service/llvm_ir/llvm_loop.h"
#include "tensorflow/compiler/xla/service/llvm_ir/llvm_util.h"
#include "tensorflow/compiler/xla/service/llvm_ir/loop_emitter.h"
#include "tensorflow/compiler/xla/service/llvm_ir/tuple_ops.h"
#include "tensorflow/compiler/xla/service/name_uniquer.h"
#include "tensorflow/compiler/xla/shape_util.h"
#include "tensorflow/compiler/xla/status_macros.h"
#include "tensorflow/compiler/xla/types.h"
#include "tensorflow/compiler/xla/util.h"
#include "tensorflow/compiler/xla/window_util.h"
#include "tensorflow/core/lib/core/errors.h"

namespace xla {

using llvm_ir::IrName;
using llvm_ir::SetToFirstInsertPoint;

namespace gpu {

IrEmitter::IrEmitter(const HloModuleConfig& hlo_module_config,
                     IrEmitterContext* ir_emitter_context, bool is_nested)
    : ir_emitter_context_(ir_emitter_context),
      module_(ir_emitter_context->llvm_module()),
      ir_builder_(module_->getContext()),
      bindings_(ir_emitter_context->hlo_module(),
                &ir_emitter_context->buffer_assignment(), &ir_builder_, module_,
                is_nested),
      hlo_module_config_(hlo_module_config) {
  ir_builder_.setFastMathFlags(llvm_ir::GetFastMathFlags(
      /*fast_math_enabled=*/hlo_module_config.debug_options()
          .xla_enable_fast_math()));
}

Status IrEmitter::DefaultAction(HloInstruction* hlo) {
  ElementalIrEmitter::HloToElementGeneratorMap operand_to_generator;
  for (const HloInstruction* operand : hlo->operands()) {
    operand_to_generator[operand] = [=](const llvm_ir::IrArray::Index& index) {
      return GetIrArray(*operand, *hlo)
          .EmitReadArrayElement(index, &ir_builder_);
    };
  }
  return EmitTargetElementLoop(
      *hlo, GpuElementalIrEmitter(hlo_module_config_, module_, &ir_builder_,
                                  GetNestedComputer())
                .MakeElementGenerator(hlo, operand_to_generator));
}

Status IrEmitter::HandleConstant(HloInstruction* constant) {
  const Literal& literal = constant->literal();
  llvm::Constant* initializer =
      llvm_ir::ConvertLiteralToIrConstant(literal, module_);
  llvm::GlobalVariable* global_for_const = new llvm::GlobalVariable(
      *module_, initializer->getType(),
      /*isConstant=*/true, llvm::GlobalValue::PrivateLinkage, initializer,
      /*Name=*/"");
  VLOG(2) << "HandleConstant: " << constant->ToString() << std::endl
          << "  emitted_value: " << llvm_ir::DumpToString(*global_for_const)
          << std::endl
          << "  its type: "
          << llvm_ir::DumpToString(*global_for_const->getType());
  bindings_.BindHloToIrValue(*constant, global_for_const);
  return Status::OK();
}

Status IrEmitter::HandleBitcast(HloInstruction* bitcast) {
  VLOG(2) << "HandleBitcast: " << bitcast->ToString();
  const HloInstruction* operand = bitcast->operand(0);
  // Bitcast is a no-op, but we still want to bind it to an llvm::Value
  // sometimes, e.g., when it's operand is a constant or a bitcast of a
  // constant.
  if (bindings_.BoundToIrValue(*operand)) {
    bindings_.BindHloToIrValue(*bitcast, GetBasePointer(*operand));
  }
  return Status::OK();
}

Status IrEmitter::HandleGetTupleElement(HloInstruction* get_tuple_element) {
  auto operand = get_tuple_element->operand(0);
  CHECK(bindings_.BoundToIrValue(*operand));
  bindings_.BindHloToIrValue(
      *get_tuple_element,
      llvm_ir::EmitGetTupleElement(
          get_tuple_element->shape(), get_tuple_element->tuple_index(),
          // TODO(b/26344050): tighten the alignment here
          // based on the real element type.
          /*alignment=*/1, GetBasePointer(*operand), &ir_builder_, module_));
  return Status::OK();
}

Status IrEmitter::HandleSort(HloInstruction*) {
  // TODO(b/26783907): Implement sort on GPU.
  return Unimplemented("sort");
}

Status IrEmitter::HandleSend(HloInstruction*) {
  return Unimplemented("Send is not implemented on GPU");
}

Status IrEmitter::HandleSendDone(HloInstruction*) {
  return Unimplemented("Send-Done is not implemented on GPU");
}

Status IrEmitter::HandleRecv(HloInstruction*) {
  return Unimplemented("Recv is not implemented on GPU");
}

Status IrEmitter::HandleRecvDone(HloInstruction*) {
  return Unimplemented("Recv-done is not implemented on GPU");
}

Status IrEmitter::HandleTuple(HloInstruction* tuple) {
  std::vector<llvm::Value*> base_ptrs;
  for (const HloInstruction* operand : tuple->operands()) {
    base_ptrs.push_back(GetBasePointer(*operand));
  }
  llvm_ir::EmitTuple(GetIrArray(*tuple, *tuple), base_ptrs, &ir_builder_,
                     module_);
  return Status::OK();
}

Status IrEmitter::EmitCallToNestedComputation(
    const HloComputation& nested_computation,
    tensorflow::gtl::ArraySlice<llvm::Value*> operands, llvm::Value* output) {
  TF_RET_CHECK(nested_computation.num_parameters() > 0);
  llvm::Function*& emitted_function =
      computation_to_ir_function_[&nested_computation];
  if (emitted_function == nullptr) {
    IrEmitterNested ir_emitter_nested(hlo_module_config_, nested_computation,
                                      ir_emitter_context_);
    TF_RETURN_IF_ERROR(
        nested_computation.root_instruction()->Accept(&ir_emitter_nested));
    emitted_function = ir_emitter_nested.GetEmittedFunction();
  }

  std::vector<llvm::Value*> arguments(operands.begin(), operands.end());
  arguments.push_back(output);
  arguments.push_back(bindings_.GetTempBufferBase());
  ir_builder_.CreateCall(emitted_function, arguments);

  return Status::OK();
}

bool IrEmitter::MaybeEmitDirectAtomicOperation(
    const HloComputation& computation, llvm::Value* output_address,
    llvm::Value* source_address) {
  CHECK_EQ(2, computation.num_parameters());

  if (computation.instruction_count() != 3) {
    // We special-case only computations with one computing instruction for now.
    // Such computation has exactly three instructions given it has two
    // parameters.
    return false;
  }

  HloOpcode root_opcode = computation.root_instruction()->opcode();
  PrimitiveType element_type =
      computation.root_instruction()->shape().element_type();
  llvm::Value* source = ir_builder_.CreateLoad(source_address, "source");
  if (root_opcode == HloOpcode::kAdd) {
    // NVPTX supports atomicAdd on F32 and integer types.
    if (element_type == F32) {
      // F32 + F32
      llvm_ir::EmitCallToIntrinsic(llvm::Intrinsic::nvvm_atomic_load_add_f32,
                                   {output_address, source},
                                   {output_address->getType()}, &ir_builder_);
      return true;
    }
    if (primitive_util::IsIntegralType(element_type)) {
      // integral + integral
      ir_builder_.CreateAtomicRMW(llvm::AtomicRMWInst::Add, output_address,
                                  source,
                                  llvm::AtomicOrdering::SequentiallyConsistent);
      return true;
    }
  }

  // NVPTX supports atomicMax and atomicMin on only integer types.
  if (root_opcode == HloOpcode::kMaximum &&
      primitive_util::IsIntegralType(element_type)) {
    // max(integral, integral)
    auto opcode = primitive_util::IsSignedIntegralType(element_type)
                      ? llvm::AtomicRMWInst::Max
                      : llvm::AtomicRMWInst::UMax;
    ir_builder_.CreateAtomicRMW(opcode, output_address, source,
                                llvm::AtomicOrdering::SequentiallyConsistent);
    return true;
  }

  if (root_opcode == HloOpcode::kMinimum &&
      primitive_util::IsIntegralType(element_type)) {
    // min(integral, integral)
    auto opcode = primitive_util::IsSignedIntegralType(element_type)
                      ? llvm::AtomicRMWInst::Min
                      : llvm::AtomicRMWInst::UMin;
    ir_builder_.CreateAtomicRMW(opcode, output_address, source,
                                llvm::AtomicOrdering::SequentiallyConsistent);
    return true;
  }

  return false;
}

// Implements atomic binary operations using atomic compare-and-swap
// (atomicCAS) as follows:
//   1. Reads the value from the memory pointed to by output_address and
//     records it as old_output.
//   2. Uses old_output as one of the source operand to perform the binary
//     operation and stores the result in new_output.
//   3. Calls atomicCAS which implements compare-and-swap as an atomic
//     operation. In particular, atomicCAS reads the value from the memory
//     pointed to by output_address, and compares the value with old_output. If
//     the two values equal, new_output is written to the same memory location
//     and true is returned to indicate that the atomic operation succeeds.
//     Otherwise, the new value read from the memory is returned. In this case,
//     the new value is copied to old_output, and steps 2. and 3. are repeated
//     until atomicCAS succeeds.
//
// On Nvidia GPUs, atomicCAS can only operate on 32 bit and 64 bit integers. If
// the element type of the binary operation is 32 bits or 64 bits, the integer
// type of the same size is used for the atomicCAS operation. On the other hand,
// if the element type is smaller than 32 bits, int32 is used for the atomicCAS
// operation. In this case, atomicCAS reads and writes 32 bit values from
// the memory, which is larger than the memory size required by the original
// atomic binary operation. We mask off the last two bits of the output_address
// and use the result as an address to read the 32 bit values from the memory.
// This can avoid out of bound memory accesses if tensor buffers are 4 byte
// aligned and have a size of 4N, an assumption that the runtime can guarantee.
//
// The pseudo code is shown below. Variables *_address are pointers to a memory
// region with a size equal to the size of the atomicCAS operation, with the
// exception that new_output_address is a pointer to a memory region with a size
// equal to the element size of the binary operation.
//
//   element_size = sizeof(element_type);
//   atomic_size = max(32, element_size);
//   cas_new_output_address = alloca(atomic_size);
//   cas_old_output_address = alloca(atomic_size);
//   if (atomic_size != element_size) {
//     atomic_address = output_address & ((int64)(-4));
//     new_output_address = cas_new_output_address + (output_address & 3);
//   } else {
//     atomic_address = output_address;
//     new_output_address = cas_new_output_address;
//   }
//
//   *cas_old_output_address = *atomic_address;
//   do {
//     *cas_new_output_address = *cas_old_output_address;
//     *new_output_address = operation(*new_output_address, *source_address);
//     (*cas_old_output_address, success) =
//       atomicCAS(atomic_address, *cas_old_output_address,
//       *cas_new_output_address);
//   } while (!success);
//
Status IrEmitter::EmitAtomicOperationUsingCAS(const HloComputation& computation,
                                              llvm::Value* output_address,
                                              llvm::Value* source_address) {
  llvm::PointerType* output_address_type =
      llvm::dyn_cast<llvm::PointerType>(output_address->getType());
  CHECK_NE(output_address_type, nullptr);

  // element_type is the data type for the binary operation.
  llvm::Type* element_type = output_address_type->getPointerElementType();
  int element_size = llvm_ir::GetSizeInBits(element_type);
  llvm::Type* element_address_type = element_type->getPointerTo();

  int atomic_size = (element_size < 32) ? 32 : element_size;
  llvm::Type* atomic_type = ir_builder_.getIntNTy(atomic_size);
  llvm::Type* atomic_address_type =
      atomic_type->getPointerTo(output_address_type->getPointerAddressSpace());

  // cas_old_output_address and cas_new_output_address point to the scratch
  // memory where we store the old and new values for the repeated atomicCAS
  // operations.
  llvm::Value* cas_old_output_address = ir_builder_.CreateAlloca(
      atomic_type, /*ArraySize=*/nullptr, "cas_old_output_address");
  llvm::Value* cas_new_output_address = ir_builder_.CreateAlloca(
      atomic_type, /*ArraySize=*/nullptr, "cas_new_output_address");

  // Emit preparation code to the preheader.
  llvm::BasicBlock* loop_preheader_bb = ir_builder_.GetInsertBlock();

  llvm::Value* atomic_memory_address;
  // binop_output_address points to the scratch memory that stores the
  // result of the binary operation.
  llvm::Value* binop_output_address;
  if (element_size < 32) {
    // Assume the element size is an integer number of bytes.
    CHECK_EQ((element_size % sizeof(char)), 0);
    llvm::Type* address_int_type =
        module_->getDataLayout().getIntPtrType(output_address_type);
    atomic_memory_address =
        ir_builder_.CreatePtrToInt(output_address, address_int_type);
    llvm::Value* mask = llvm::ConstantInt::get(address_int_type, 3);
    llvm::Value* offset = ir_builder_.CreateAnd(atomic_memory_address, mask);
    mask = llvm::ConstantInt::get(address_int_type, -4);
    atomic_memory_address = ir_builder_.CreateAnd(atomic_memory_address, mask);
    atomic_memory_address =
        ir_builder_.CreateIntToPtr(atomic_memory_address, atomic_address_type);
    binop_output_address = ir_builder_.CreateAdd(
        ir_builder_.CreatePtrToInt(cas_new_output_address, address_int_type),
        offset);
    binop_output_address =
        ir_builder_.CreateIntToPtr(binop_output_address, element_address_type);
  } else {
    atomic_memory_address =
        ir_builder_.CreateBitCast(output_address, atomic_address_type);
    binop_output_address =
        ir_builder_.CreateBitCast(cas_new_output_address, element_address_type);
  }

  // Use the value from the memory that atomicCAS operates on to initialize
  // cas_old_output.
  llvm::Value* cas_old_output =
      ir_builder_.CreateLoad(atomic_memory_address, "cas_old_output");
  ir_builder_.CreateStore(cas_old_output, cas_old_output_address);

  llvm::BasicBlock* loop_exit_bb = loop_preheader_bb->splitBasicBlock(
      ir_builder_.GetInsertPoint(), "atomic_op_loop_exit");
  llvm::BasicBlock* loop_body_bb =
      llvm::BasicBlock::Create(ir_builder_.getContext(), "atomic_op_loop_body",
                               ir_builder_.GetInsertBlock()->getParent());
  ir_builder_.SetInsertPoint(loop_body_bb);
  // Change preheader's successor from loop_exit_bb to loop_body_bb.
  loop_preheader_bb->getTerminator()->setSuccessor(0, loop_body_bb);

  // Emit the body of the loop that repeatedly invokes atomicCAS.
  //
  // Use cas_old_output to initialize cas_new_output.
  cas_old_output =
      ir_builder_.CreateLoad(cas_old_output_address, "cas_old_output");
  ir_builder_.CreateStore(cas_old_output, cas_new_output_address);
  // Emits code to calculate new_output = operation(old_output, source);
  TF_RETURN_IF_ERROR(EmitCallToNestedComputation(
      computation, {binop_output_address, source_address},
      binop_output_address));

  llvm::Value* cas_new_output =
      ir_builder_.CreateLoad(cas_new_output_address, "cas_new_output");

  // Emit code to perform the atomicCAS operation
  // (cas_old_output, success) = atomicCAS(memory_address, cas_old_output,
  //                                       cas_new_output);
  llvm::Value* ret_value = ir_builder_.CreateAtomicCmpXchg(
      atomic_memory_address, cas_old_output, cas_new_output,
      llvm::AtomicOrdering::SequentiallyConsistent,
      llvm::AtomicOrdering::SequentiallyConsistent);

  // Extract the memory value returned from atomicCAS and store it as
  // cas_old_output.
  ir_builder_.CreateStore(
      ir_builder_.CreateExtractValue(ret_value, 0, "cas_old_output"),
      cas_old_output_address);
  // Extract the success bit returned from atomicCAS and generate a
  // conditional branch on the success bit.
  ir_builder_.CreateCondBr(
      ir_builder_.CreateExtractValue(ret_value, 1, "success"), loop_exit_bb,
      loop_body_bb);

  // Set the insertion point to the exit basic block so that the caller of
  // this method can continue emitting code to the right place.
  SetToFirstInsertPoint(loop_exit_bb, &ir_builder_);
  return Status::OK();
}

Status IrEmitter::EmitAtomicOperationForNestedComputation(
    const HloComputation& computation, llvm::Value* output_address,
    llvm::Value* source_address) {
  if (computation.num_parameters() != 2) {
    // TODO(b/30258929): We only accept binary computations so far.
    return Unimplemented(
        "We only support atomic functions with exactly two parameters, but "
        "computation %s has %lld.",
        computation.name().c_str(), computation.num_parameters());
  }

  if (MaybeEmitDirectAtomicOperation(computation, output_address,
                                     source_address)) {
    return Status::OK();
  }

  return EmitAtomicOperationUsingCAS(computation, output_address,
                                     source_address);
}

Status IrEmitter::HandleSelect(HloInstruction* select) {
  auto pred = select->operand(0);
  auto on_true = select->operand(1);
  auto on_false = select->operand(2);
  TF_RET_CHECK(pred->shape().element_type() == PRED);

  if (ShapeUtil::IsTuple(select->shape())) {
    llvm_ir::EmitTupleSelect(GetIrArray(*select, *select),
                             GetIrArray(*pred, *select),
                             GetBasePointer(*on_true),
                             GetBasePointer(*on_false), &ir_builder_, module_);
    return Status::OK();
  }

  // We must not call the subclass `DefaultAction` method, lest its
  // `HandleSelect` call `IrEmitter::HandleSelect` and its `DefaultAction`
  // assume no handler has already been called.
  return IrEmitter::DefaultAction(select);
}

Status IrEmitter::HandleDot(HloInstruction* dot) {
  auto lhs_instruction = dot->operand(0);
  auto rhs_instruction = dot->operand(1);
  const llvm_ir::IrArray& target_array = GetIrArray(*dot, *dot);
  const llvm_ir::IrArray& lhs_array = GetIrArray(*lhs_instruction, *dot);
  const llvm_ir::IrArray& rhs_array = GetIrArray(*rhs_instruction, *dot);

  const Shape& lhs_shape = lhs_instruction->shape();
  const Shape& rhs_shape = rhs_instruction->shape();

  if (ShapeUtil::IsScalar(lhs_shape) && ShapeUtil::IsScalar(rhs_shape)) {
    // If the operands are scalar, don't emit any loops.
    llvm::Value* lhs_value =
        lhs_array.EmitReadArrayElement(/*index=*/{}, &ir_builder_);
    llvm::Value* rhs_value =
        rhs_array.EmitReadArrayElement(/*index=*/{}, &ir_builder_);
    llvm::Value* result;
    if (ShapeUtil::ElementIsComplex(lhs_shape)) {
      auto real = [&](llvm::Value* x) {
        return ir_builder_.CreateExtractValue(x, {0});
      };
      auto imag = [&](llvm::Value* x) {
        return ir_builder_.CreateExtractValue(x, {1});
      };
      llvm::Value* real_result = ir_builder_.CreateFSub(
          ir_builder_.CreateFMul(real(lhs_value), real(rhs_value)),
          ir_builder_.CreateFMul(imag(lhs_value), imag(rhs_value)));
      llvm::Value* imag_result = ir_builder_.CreateFAdd(
          ir_builder_.CreateFMul(real(lhs_value), imag(rhs_value)),
          ir_builder_.CreateFMul(imag(lhs_value), real(rhs_value)));
      result = llvm::ConstantAggregateZero::get(lhs_array.GetElementLlvmType());
      result = ir_builder_.CreateInsertValue(result, real_result, {0});
      result = ir_builder_.CreateInsertValue(result, imag_result, {1});
    } else {
      result = ir_builder_.CreateFMul(lhs_value, rhs_value);
    }
    target_array.EmitWriteArrayElement(/*index=*/{}, result, &ir_builder_);
    return Status::OK();
  }

  // "Scalar dot non-scalar" or "non-scalar dot scalar" is invalid. See
  // the semantics of Dot in the XLA documentation for details.
  TF_RET_CHECK(!ShapeUtil::IsScalar(lhs_shape) &&
               !ShapeUtil::IsScalar(rhs_shape));

  // Reduce along the last dimension of the LHS and the second-to-last dimension
  // of the RHS. Vectors are a special case where the reduction dimension is 0
  // for both LHS and RHS. This results in a vector dot product producing a
  // scalar.
  const int64 lhs_reduction_dimension =
      ShapeUtil::GetDimensionNumber(lhs_shape, -1);
  const int64 rhs_reduction_dimension =
      ShapeUtil::Rank(rhs_shape) >= 2
          ? ShapeUtil::GetDimensionNumber(rhs_shape, -2)
          : 0;

  // Verify the reduction dimension in the two operands are the same size.
  TF_RET_CHECK(lhs_shape.dimensions(lhs_reduction_dimension) ==
               rhs_shape.dimensions(rhs_reduction_dimension));

  // Create loop nests which loop through the LHS operand dimensions and the RHS
  // operand dimensions. The reduction dimension of the LHS and RHS are handled
  // in a separate innermost loop which performs the sum of products.
  llvm_ir::ForLoopNest loop_nest(IrName(dot), &ir_builder_);
  llvm_ir::IrArray::Index lhs_index = EmitOperandArrayLoopNest(
      lhs_array, lhs_reduction_dimension, "lhs", &loop_nest);
  llvm_ir::IrArray::Index rhs_index = EmitOperandArrayLoopNest(
      rhs_array, rhs_reduction_dimension, "rhs", &loop_nest);

  // Create the reduction loop which does the sum of products reduction.
  std::unique_ptr<llvm_ir::ForLoop> reduction_loop = loop_nest.AddLoop(
      /*start_index=*/0,
      /*end_index=*/lhs_shape.dimensions(lhs_reduction_dimension),
      /*suffix=*/"reduction");

  // The final entry in the rhs and lhs indexes is the indvar of the reduction
  // loop.
  lhs_index[lhs_reduction_dimension] = reduction_loop->GetIndVarValue();
  rhs_index[rhs_reduction_dimension] = reduction_loop->GetIndVarValue();

  // For computing the sum of products we alloca a single location to store the
  // dot product result as we accumulate it within the reduction loop. After the
  // reduction loop we load the result and store into the output array.
  llvm::Type* accum_type = target_array.GetElementLlvmType();
  llvm::Value* accum_address = llvm_ir::EmitAllocaAtFunctionEntry(
      accum_type,       // The pointee type of the alloca instruction.
      "accum_address",  // The name of the alloca instruction.
      &ir_builder_);

  // Initialize the accumulator in the preheader to zero.
  new llvm::StoreInst(
      llvm::Constant::getNullValue(lhs_array.GetElementLlvmType()),  // init 0
      accum_address,  // The address.
      reduction_loop->GetPreheaderBasicBlock()
          ->getTerminator());  // The instruction this store is inserted before.

  // Emit the body of the reduction loop:
  //   accum = *accum_address
  //   updated_accum = accum + lhs_element * rhs_element
  //   *accum_address = updated_accum
  TF_RET_CHECK(!reduction_loop->GetBodyBasicBlock()->empty());
  ir_builder_.SetInsertPoint(
      &*reduction_loop->GetBodyBasicBlock()->getFirstInsertionPt());
  llvm::Value* lhs_element =
      lhs_array.EmitReadArrayElement(lhs_index, &ir_builder_);
  llvm::Value* rhs_element =
      rhs_array.EmitReadArrayElement(rhs_index, &ir_builder_);
  llvm::Value* accum = ir_builder_.CreateLoad(accum_address);
  llvm::Value* updated_accum;
  if (ShapeUtil::ElementIsComplex(lhs_shape)) {
#define REAL(x) ir_builder_.CreateExtractValue(x, {0})
#define IMAG(x) ir_builder_.CreateExtractValue(x, {1})
    llvm::Value* product_real = ir_builder_.CreateFSub(
        ir_builder_.CreateFMul(REAL(lhs_element), REAL(rhs_element)),
        ir_builder_.CreateFMul(IMAG(lhs_element), IMAG(rhs_element)));
    llvm::Value* product_imag = ir_builder_.CreateFAdd(
        ir_builder_.CreateFMul(REAL(lhs_element), IMAG(rhs_element)),
        ir_builder_.CreateFMul(IMAG(lhs_element), REAL(rhs_element)));
    updated_accum = ir_builder_.CreateInsertValue(
        accum, ir_builder_.CreateFAdd(REAL(accum), product_real), {0});
    updated_accum = ir_builder_.CreateInsertValue(
        updated_accum, ir_builder_.CreateFAdd(IMAG(accum), product_imag), {1});
#undef IMAG
#undef REAL
  } else {
    llvm::Value* product = ir_builder_.CreateFMul(lhs_element, rhs_element);
    updated_accum = ir_builder_.CreateFAdd(accum, product);
  }
  ir_builder_.CreateStore(updated_accum, accum_address);

  // After the reduction loop exits, store the accumulator into the target
  // address. The index into the target address is the concatenation of the rhs
  // and lhs indexes with the reduction dimensions removed. The terms from the
  // rhs index are the lower dimensions in the index so we add them first.
  llvm_ir::IrArray::Index target_index;
  for (size_t dimension = 0; dimension < lhs_index.size(); ++dimension) {
    if (dimension != lhs_reduction_dimension) {
      target_index.push_back(lhs_index[dimension]);
    }
  }
  for (size_t dimension = 0; dimension < rhs_index.size(); ++dimension) {
    if (dimension != rhs_reduction_dimension) {
      target_index.push_back(rhs_index[dimension]);
    }
  }
  SetToFirstInsertPoint(reduction_loop->GetExitBasicBlock(), &ir_builder_);
  target_array.EmitWriteArrayElement(
      target_index,
      ir_builder_.CreateLoad(
          accum_address),  // The value written to the target array.
      &ir_builder_);

  // Set the IR builder insert point to the exit basic block of the outer most
  // loop. This ensures later instructions are inserted after this loop nest.
  ir_builder_.SetInsertPoint(loop_nest.GetOuterLoopExitBasicBlock());

  return Status::OK();
}

Status IrEmitter::HandleConvolution(HloInstruction* convolution) {
  if (ShapeUtil::HasZeroElements(convolution->shape())) {
    // Emit no code for an empty output.
    return Status::OK();
  }
  // TODO(b/31409998): Support convolution with dilation.
  return Unimplemented(
      "Hit a case for convolution that is not implemented on GPU.");
}

Status IrEmitter::HandleFft(HloInstruction* fft) {
  if (ShapeUtil::HasZeroElements(fft->shape())) {
    // Emit no code for an empty output.
    return Status::OK();
  }
  return Unimplemented("Hit a case for fft that is not implemented on GPU.");
}

Status IrEmitter::HandleCrossReplicaSum(HloInstruction* crs) {
  // TODO(b/33011107): Support cross replica sum on GPU.
  return Unimplemented("CrossReplicaSum is not implemented on GPU.");
}

Status IrEmitter::HandleParameter(HloInstruction* parameter) {
  return Status::OK();
}

Status IrEmitter::HandleReduce(HloInstruction* reduce) {
  auto arg = reduce->operand(0);
  auto init_value = reduce->operand(1);
  tensorflow::gtl::ArraySlice<int64> dimensions(reduce->dimensions());
  HloComputation* function = reduce->to_apply();
  return EmitTargetElementLoop(
      *reduce,
      [=](const llvm_ir::IrArray::Index& index) -> StatusOr<llvm::Value*> {
        // Initialize an accumulator with init_value.
        llvm::AllocaInst* accumulator_addr =
            ir_builder_.CreateAlloca(llvm_ir::PrimitiveTypeToIrType(
                reduce->shape().element_type(), module_));
        ir_builder_.CreateStore(
            ir_builder_.CreateLoad(GetBasePointer(*init_value)),
            accumulator_addr);

        // The enclosing loops go over all the target elements. Now we have to
        // compute the actual target element. For this, we build a new loop nest
        // to iterate over all the reduction dimensions in the argument.
        // AddLoopsForShapeOnDimensions will return an Index where induction
        // Value*s are placed for each dimension in dimensions, and all the rest
        // are nullptrs.
        llvm_ir::ForLoopNest loops(IrName(reduce, "inner"), &ir_builder_);
        const llvm_ir::IrArray::Index reduced_dims_index =
            loops.AddLoopsForShapeOnDimensions(arg->shape(), dimensions,
                                               "reduction_dim");

        SetToFirstInsertPoint(loops.GetInnerLoopBodyBasicBlock(), &ir_builder_);

        // Build a full index for the input argument, using reduced_dims_index
        // as the base. In reduced_dims_index only the reduction dimensions are
        // filled in. We fill in the rest of the dimensions with induction
        // Value*s taken from 'index' which iterates over the target array.
        // See the high-level description in the XLA documentation for details.
        llvm_ir::IrArray::Index input_index = reduced_dims_index;
        llvm_ir::IrArray::Index::const_iterator it = index.begin();

        for (size_t i = 0; i < input_index.size(); ++i) {
          if (input_index[i] == nullptr) {
            input_index[i] = *it++;
          }
        }
        CHECK(index.end() == it);

        // Apply the reduction function to the loaded value.
        llvm::Value* input_address =
            GetIrArray(*arg, *reduce)
                .EmitArrayElementAddress(input_index, &ir_builder_);
        TF_RETURN_IF_ERROR(EmitCallToNestedComputation(
            *function, {accumulator_addr, input_address}, accumulator_addr));

        SetToFirstInsertPoint(loops.GetOuterLoopExitBasicBlock(), &ir_builder_);
        return ir_builder_.CreateLoad(accumulator_addr);
      });
}

Status IrEmitter::HandleFusion(HloInstruction* fusion) {
  // kFusion for library calls should be handled by
  // IrEmitterUnnested::HandleFusion.
  CHECK(HloInstruction::FusionKind::kLoop == fusion->fusion_kind());

  std::vector<llvm_ir::IrArray> parameter_arrays;
  for (HloInstruction* operand : fusion->operands()) {
    parameter_arrays.push_back(GetIrArray(*operand, *fusion));
  }
  GpuElementalIrEmitter elemental_emitter(hlo_module_config_, module_,
                                          &ir_builder_, GetNestedComputer());
  FusedIrEmitter fused_emitter(parameter_arrays, &elemental_emitter);
  TF_RETURN_IF_ERROR(fusion->fused_expression_root()->Accept(&fused_emitter));

  return EmitTargetElementLoop(*fusion, fused_emitter.GetRootGenerator());
}

Status IrEmitter::HandleCall(HloInstruction* call) {
  std::vector<llvm::Value*> operand_addresses;
  for (HloInstruction* operand : call->operands()) {
    operand_addresses.push_back(GetBasePointer(*operand));
  }
  return EmitCallToNestedComputation(*call->to_apply(), operand_addresses,
                                     GetBasePointer(*call));
}

Status IrEmitter::HandleCustomCall(HloInstruction*) {
  return Unimplemented("custom-call");
}

Status IrEmitter::HandleInfeed(HloInstruction*) {
  // TODO(b/30467474): Implement infeed on GPU.
  return Unimplemented("Infeed is not supported on GPU.");
}

Status IrEmitter::HandleOutfeed(HloInstruction*) {
  // TODO(b/34359662): Implement outfeed on GPU.
  return Unimplemented("Outfeed is not supported on GPU.");
}

Status IrEmitter::HandleRng(HloInstruction* random) {
  ElementalIrEmitter::HloToElementGeneratorMap operand_to_generator;
  for (const HloInstruction* operand : random->operands()) {
    operand_to_generator[operand] = [=](const llvm_ir::IrArray::Index& index) {
      return GetIrArray(*operand, *random)
          .EmitReadArrayElement(index, &ir_builder_);
    };
  }
  // Emits a single-threaded loop because the loop body generated by the element
  // generator for Rng can't be parallelized (b/32333178).
  return llvm_ir::LoopEmitter(
             GpuElementalIrEmitter(hlo_module_config_, module_, &ir_builder_,
                                   GetNestedComputer())
                 .MakeElementGenerator(random, operand_to_generator),
             GetIrArray(*random, *random), &ir_builder_)
      .EmitLoop(IrName(random));
}

Status IrEmitter::HandleBatchNormInference(HloInstruction*) {
  return Unimplemented(
      "The GPU backend does not implement BatchNormInference directly.  It "
      "should be lowered before IR emission to HLO-soup using "
      "BatchNormRewriter or to a cudnn CustomCall using "
      "CudnnBatchNormRewriter.");
}

Status IrEmitter::HandleBatchNormTraining(HloInstruction*) {
  return Unimplemented(
      "The GPU backend does not implement BatchNormTraining directly.  It "
      "should be lowered before IR emission to HLO-soup using "
      "BatchNormRewriter or to a cudnn CustomCall using "
      "CudnnBatchNormRewriter.");
}

Status IrEmitter::HandleBatchNormGrad(HloInstruction*) {
  return Unimplemented(
      "The GPU backend does not implement BatchNormGrad directly.  It should "
      "be lowered before IR emission to HLO-soup (using BatchNormRewriter) or "
      "to a cudnn CustomCall using CudnnBatchNormRewriter.");
}

llvm_ir::IrArray::Index IrEmitter::EmitOperandArrayLoopNest(
    const llvm_ir::IrArray& operand_array, int64 reduction_dimension,
    tensorflow::StringPiece name_suffix, llvm_ir::ForLoopNest* loop_nest) {
  // Prepares the dimension list we will use to emit the loop nest. Outermost
  // loops are added first. Add loops in major-to-minor order, and skip the
  // reduction dimension.
  std::vector<int64> dimensions;
  const Shape& shape = operand_array.GetShape();
  for (int i = 0; i < LayoutUtil::MinorToMajor(shape).size(); ++i) {
    int64 dimension = LayoutUtil::Major(shape.layout(), i);
    if (dimension != reduction_dimension) {
      dimensions.push_back(dimension);
    }
  }

  // Create loop nest with one for-loop for each dimension of the
  // output.
  llvm_ir::IrArray::Index index =
      loop_nest->AddLoopsForShapeOnDimensions(shape, dimensions, name_suffix);
  // Verify every dimension except the reduction dimension was set in the index.
  for (size_t dimension = 0; dimension < index.size(); ++dimension) {
    if (dimension == reduction_dimension) {
      DCHECK_EQ(nullptr, index[dimension]);
    } else {
      DCHECK_NE(nullptr, index[dimension]);
    }
  }
  return index;
}

StatusOr<llvm::Value*> IrEmitter::ComputeNestedElement(
    const HloComputation& computation,
    tensorflow::gtl::ArraySlice<llvm::Value*> parameter_elements) {
  llvm::Value* return_buffer = llvm_ir::EmitAllocaAtFunctionEntry(
      llvm_ir::PrimitiveTypeToIrType(
          computation.root_instruction()->shape().element_type(), module_),
      "return_buffer", &ir_builder_);
  std::vector<llvm::Value*> parameter_buffers;
  for (llvm::Value* parameter_element : parameter_elements) {
    parameter_buffers.push_back(llvm_ir::EmitAllocaAtFunctionEntry(
        parameter_element->getType(), "parameter_buffer", &ir_builder_));
    ir_builder_.CreateStore(parameter_element, parameter_buffers.back());
  }
  TF_RETURN_IF_ERROR(EmitCallToNestedComputation(computation, parameter_buffers,
                                                 return_buffer));
  return ir_builder_.CreateLoad(return_buffer);
}

}  // namespace gpu
}  // namespace xla
