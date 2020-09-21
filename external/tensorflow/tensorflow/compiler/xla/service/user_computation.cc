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

#include "tensorflow/compiler/xla/service/user_computation.h"

#include <algorithm>
#include <set>
#include <stack>
#include <unordered_map>
#include <utility>
#include <vector>

#include "tensorflow/compiler/xla/layout_util.h"
#include "tensorflow/compiler/xla/literal_util.h"
#include "tensorflow/compiler/xla/ptr_util.h"
#include "tensorflow/compiler/xla/service/hlo_computation.h"
#include "tensorflow/compiler/xla/service/hlo_instruction.h"
#include "tensorflow/compiler/xla/service/hlo_opcode.h"
#include "tensorflow/compiler/xla/service/shape_inference.h"
#include "tensorflow/compiler/xla/shape_util.h"
#include "tensorflow/compiler/xla/status_macros.h"
#include "tensorflow/compiler/xla/types.h"
#include "tensorflow/compiler/xla/util.h"
#include "tensorflow/core/lib/core/errors.h"
#include "tensorflow/core/lib/strings/str_util.h"
#include "tensorflow/core/lib/strings/strcat.h"
#include "tensorflow/core/lib/strings/stringprintf.h"
#include "tensorflow/core/platform/logging.h"
#include "tensorflow/core/platform/protobuf.h"

namespace xla {
namespace {

HloOpcode UnaryOperationToHloOpcode(UnaryOperation unop) {
  switch (unop) {
    case UNOP_ABS:
      return HloOpcode::kAbs;
    case UNOP_CEIL:
      return HloOpcode::kCeil;
    case UNOP_COS:
      return HloOpcode::kCos;
    case UNOP_EXP:
      return HloOpcode::kExp;
    case UNOP_FLOOR:
      return HloOpcode::kFloor;
    case UNOP_IMAG:
      return HloOpcode::kImag;
    case UNOP_IS_FINITE:
      return HloOpcode::kIsFinite;
    case UNOP_LOG:
      return HloOpcode::kLog;
    case UNOP_NOT:
      return HloOpcode::kNot;
    case UNOP_NEGATE:
      return HloOpcode::kNegate;
    case UNOP_REAL:
      return HloOpcode::kReal;
    case UNOP_ROUND_NEAREST_AFZ:
      return HloOpcode::kRoundNearestAfz;
    case UNOP_SIGN:
      return HloOpcode::kSign;
    case UNOP_SIN:
      return HloOpcode::kSin;
    case UNOP_SORT:
      return HloOpcode::kSort;
    case UNOP_TANH:
      return HloOpcode::kTanh;
    default:
      LOG(FATAL) << "unhandled operation " << unop;
  }
}

HloOpcode BinaryOperationToHloOpcode(BinaryOperation binop) {
  switch (binop) {
    case BINOP_ATAN2:
      return HloOpcode::kAtan2;
    case BINOP_COMPLEX:
      return HloOpcode::kComplex;
    case BINOP_MUL:
      return HloOpcode::kMultiply;
    case BINOP_ADD:
      return HloOpcode::kAdd;
    case BINOP_SUB:
      return HloOpcode::kSubtract;
    case BINOP_DIV:
      return HloOpcode::kDivide;
    case BINOP_EQ:
      return HloOpcode::kEq;
    case BINOP_GE:
      return HloOpcode::kGe;
    case BINOP_GT:
      return HloOpcode::kGt;
    case BINOP_LE:
      return HloOpcode::kLe;
    case BINOP_LT:
      return HloOpcode::kLt;
    case BINOP_NE:
      return HloOpcode::kNe;
    case BINOP_MAX:
      return HloOpcode::kMaximum;
    case BINOP_MIN:
      return HloOpcode::kMinimum;
    case BINOP_POW:
      return HloOpcode::kPower;
    case BINOP_REM:
      return HloOpcode::kRemainder;
    case BINOP_OR:
      return HloOpcode::kOr;
    case BINOP_AND:
      return HloOpcode::kAnd;
    case BINOP_SHIFT_LEFT:
      return HloOpcode::kShiftLeft;
    case BINOP_SHIFT_RIGHT_ARITHMETIC:
      return HloOpcode::kShiftRightArithmetic;
    case BINOP_SHIFT_RIGHT_LOGICAL:
      return HloOpcode::kShiftRightLogical;
    default:
      LOG(FATAL) << "unhandled operation " << binop;
  }
}

HloOpcode TernaryOperationToHloOpcode(TernaryOperation triop) {
  switch (triop) {
    case TRIOP_CLAMP:
      return HloOpcode::kClamp;
    case TRIOP_SELECT:
      return HloOpcode::kSelect;
    default:
      LOG(FATAL) << "unhandled operation " << triop;
  }
}

HloOpcode VariadicOperationToHloOpcode(VariadicOperation varop) {
  switch (varop) {
    case VAROP_TUPLE:
      return HloOpcode::kTuple;
    default:
      LOG(FATAL) << "unhandled operation " << varop;
  }
}

}  // namespace

/* static */ StatusOr<std::unique_ptr<UserComputation>>
UserComputation::MakeWithRemapping(
    const SessionComputation& session_computation,
    const ComputationHandle& handle,
    const std::map<int64, ComputationHandle>& old_to_new) {
  auto user_computation =
      MakeUnique<UserComputation>(session_computation.name(), handle);
  {
    tensorflow::mutex_lock lock(user_computation->mutex_);
    user_computation->session_computation_ = session_computation;
    user_computation->next_handle_value_ =
        std::max_element(session_computation.requests().begin(),
                         session_computation.requests().end(),
                         [](const std::pair<int64, OperationRequest>& lhs,
                            const std::pair<int64, OperationRequest>& rhs) {
                           return lhs.first < rhs.first;
                         })
            ->first +
        1;
    TF_RETURN_IF_ERROR(user_computation->RemapEmbeddedComputations(old_to_new));
  }

  return std::move(user_computation);
}

UserComputation::UserComputation(const string& name,
                                 const ComputationHandle& handle)
    : name_(name), next_handle_value_(1) {
  *session_computation_.mutable_computation_handle() = handle;
  session_computation_.set_name(name);

  VLOG(1) << "New UserComputation \"" << name
          << "\", handle: " << handle.handle();
}

ComputationDataHandle UserComputation::CreateComputationDataHandle() {
  ComputationDataHandle handle;
  handle.set_handle(next_handle_value_);
  // Handles are used as Version values and *must* be assigned consecutively for
  // computation versioning to work.
  next_handle_value_++;
  return handle;
}

StatusOr<ComputationDataHandle> UserComputation::AddParameterInstruction(
    const ParameterRequest& parameter_request) {
  tensorflow::mutex_lock lock(mutex_);

  int64 parameter_number = parameter_request.parameter();
  if (parameters_.count(parameter_number) != 0) {
    return InvalidArgument("parameter %lld already registered",
                           parameter_number);
  }
  ComputationDataHandle handle = CreateComputationDataHandle();

  const Shape& validated_shape = parameter_request.shape();
  TF_RETURN_IF_ERROR(
      ShapeUtil::ValidateShapeWithOptionalLayout(validated_shape));

  OperationRequest& request =
      (*session_computation_.mutable_requests())[handle.handle()];
  *request.mutable_output_handle() = handle;
  *request.mutable_output_shape() = validated_shape;
  *request.mutable_request()->mutable_parameter_request() = parameter_request;

  parameters_[parameter_number] = &request;

  VLOG(1) << "AddParameterInstruction (" << GetVersionedHandleInternal()
          << "), data handle " << handle.handle() << ": "
          << parameter_request.ShortDebugString();
  return handle;
}

Status UserComputation::AddSendInstruction(const SendRequest& send_request) {
  tensorflow::mutex_lock lock(mutex_);

  // Check if the operand of the instruction is valid.
  TF_RETURN_IF_ERROR(LookUpRequest(send_request.operand()).status());

  // No handle is returned, but a handle must be assigned to this instruction
  // for computation versioning.
  ComputationDataHandle handle = CreateComputationDataHandle();
  OperationRequest& request =
      (*session_computation_.mutable_requests())[handle.handle()];
  *request.mutable_output_handle() = handle;
  *request.mutable_output_shape() = ShapeUtil::MakeNil();
  *request.mutable_request()->mutable_send_request() = send_request;

  VLOG(1) << "AddSendInstruction (" << GetVersionedHandleInternal()
          << "), data handle " << handle.handle() << ": "
          << send_request.ShortDebugString();
  return Status::OK();
}

StatusOr<ComputationDataHandle> UserComputation::AddRecvInstruction(
    const RecvRequest& recv_request) {
  tensorflow::mutex_lock lock(mutex_);

  const Shape& shape = recv_request.shape();
  TF_RETURN_IF_ERROR(ShapeUtil::ValidateShapeWithOptionalLayout(shape));
  ComputationDataHandle handle = CreateComputationDataHandle();
  OperationRequest& request =
      (*session_computation_.mutable_requests())[handle.handle()];
  *request.mutable_output_handle() = handle;
  *request.mutable_output_shape() = shape;
  *request.mutable_request()->mutable_recv_request() = recv_request;

  VLOG(1) << "AddRecvInstruction (" << GetVersionedHandleInternal()
          << "), data handle " << handle.handle() << ": "
          << recv_request.ShortDebugString();
  return handle;
}

StatusOr<ComputationDataHandle> UserComputation::AddPadInstruction(
    const PadRequest& pad_request) {
  tensorflow::mutex_lock lock(mutex_);

  TF_ASSIGN_OR_RETURN(const OperationRequest* operand,
                      LookUpRequest(pad_request.operand()));

  TF_ASSIGN_OR_RETURN(const OperationRequest* padding_value,
                      LookUpRequest(pad_request.padding_value()));

  TF_ASSIGN_OR_RETURN(Shape inferred_shape, ShapeInference::InferPadShape(
                                                operand->output_shape(),
                                                padding_value->output_shape(),
                                                pad_request.padding_config()));

  ComputationDataHandle handle = CreateComputationDataHandle();
  OperationRequest& request =
      (*session_computation_.mutable_requests())[handle.handle()];
  *request.mutable_output_handle() = handle;
  *request.mutable_output_shape() = inferred_shape;
  *request.mutable_request()->mutable_pad_request() = pad_request;

  VLOG(1) << "AddPadInstruction (" << GetVersionedHandleInternal()
          << "), data handle " << handle.handle() << ": "
          << pad_request.ShortDebugString();
  return handle;
}

StatusOr<ComputationDataHandle> UserComputation::AddConstantInstruction(
    const ConstantRequest& constant_request) {
  const Shape& validated_shape = constant_request.literal().shape();
  TF_RETURN_IF_ERROR(
      ShapeUtil::ValidateShapeWithOptionalLayout(validated_shape));

  tensorflow::mutex_lock lock(mutex_);

  ComputationDataHandle handle = CreateComputationDataHandle();

  OperationRequest& request =
      (*session_computation_.mutable_requests())[handle.handle()];
  *request.mutable_output_handle() = handle;
  *request.mutable_output_shape() = validated_shape;
  *request.mutable_request()->mutable_constant_request() = constant_request;

  VLOG(1) << "AddConstantInstruction (" << GetVersionedHandleInternal()
          << "), data handle " << handle.handle();
  return handle;
}

StatusOr<ComputationDataHandle> UserComputation::AddGatherInstruction(
    const GatherRequest& gather_request) {
  tensorflow::mutex_lock lock(mutex_);

  TF_ASSIGN_OR_RETURN(const OperationRequest* input_request,
                      LookUpRequest(gather_request.input()));
  TF_ASSIGN_OR_RETURN(const OperationRequest* gather_indices_request,
                      LookUpRequest(gather_request.gather_indices()));

  TF_ASSIGN_OR_RETURN(
      Shape shape,
      ShapeInference::InferGatherShape(
          input_request->output_shape(), gather_indices_request->output_shape(),
          gather_request.dimension_numbers(),
          AsInt64Slice(gather_request.window_bounds())));

  const ComputationDataHandle handle = CreateComputationDataHandle();

  OperationRequest& request =
      (*session_computation_.mutable_requests())[handle.handle()];
  *request.mutable_output_handle() = handle;
  *request.mutable_output_shape() = shape;
  *request.mutable_request()->mutable_gather_request() = gather_request;

  VLOG(1) << "AddGatherInstruction (" << GetVersionedHandleInternal()
          << "), data handle " << handle.handle() << ": "
          << gather_request.ShortDebugString();
  return handle;
}

StatusOr<ComputationDataHandle> UserComputation::AddGetTupleElementInstruction(
    const GetTupleElementRequest& get_tuple_element_request) {
  tensorflow::mutex_lock lock(mutex_);

  TF_ASSIGN_OR_RETURN(const OperationRequest* operand,
                      LookUpRequest(get_tuple_element_request.operand()));
  if (!ShapeUtil::IsTuple(operand->output_shape())) {
    return InvalidArgument(
        "Operand to GetTupleElement() is not a tuple; got %s",
        ShapeUtil::HumanString(operand->output_shape()).c_str());
  }
  Shape element_shape = ShapeUtil::GetTupleElementShape(
      operand->output_shape(), get_tuple_element_request.index());

  ComputationDataHandle handle = CreateComputationDataHandle();

  OperationRequest& request =
      (*session_computation_.mutable_requests())[handle.handle()];
  *request.mutable_output_handle() = handle;
  *request.mutable_output_shape() = element_shape;
  *request.mutable_request()->mutable_get_tuple_element_request() =
      get_tuple_element_request;

  VLOG(1) << "AddGetTupleElementInstruction (" << GetVersionedHandleInternal()
          << "), data handle " << handle.handle() << ": "
          << get_tuple_element_request.ShortDebugString();
  return handle;
}

Status UserComputation::AddTraceInstruction(const TraceRequest& trace_request) {
  tensorflow::mutex_lock lock(mutex_);

  // Verify that the operand index is valid.
  TF_RETURN_IF_ERROR(LookUpRequest(trace_request.operand()).status());

  ComputationDataHandle handle = CreateComputationDataHandle();
  OperationRequest& request =
      (*session_computation_.mutable_requests())[handle.handle()];
  *request.mutable_output_handle() = handle;
  *request.mutable_output_shape() = ShapeUtil::MakeNil();
  *request.mutable_request()->mutable_trace_request() = trace_request;

  VLOG(1) << "AddTraceInstruction (" << GetVersionedHandleInternal()
          << "), data handle " << handle.handle() << ": "
          << trace_request.ShortDebugString();
  return Status::OK();
}

StatusOr<ComputationDataHandle> UserComputation::AddRngInstruction(
    const RngRequest& rng_request) {
  tensorflow::mutex_lock lock(mutex_);

  // Check the number of parameters per RNG distribution.
  switch (rng_request.distribution()) {
    case RandomDistribution::RNG_NORMAL:
    case RandomDistribution::RNG_UNIFORM:
      if (rng_request.parameter_size() != 2) {
        return InvalidArgument(
            "RNG distribution (%s) expects 2 parameters, but got %d",
            RandomDistribution_Name(rng_request.distribution()).c_str(),
            rng_request.parameter_size());
      }
      break;
    default:
      LOG(FATAL) << "unhandled distribution " << rng_request.distribution();
  }

  // Verify that the parameter indices are valid;
  for (const ComputationDataHandle& param : rng_request.parameter()) {
    TF_RETURN_IF_ERROR(LookUpRequest(param).status());
  }
  const Shape& validated_shape = rng_request.shape();
  TF_RETURN_IF_ERROR(
      ShapeUtil::ValidateShapeWithOptionalLayout(validated_shape));

  ComputationDataHandle handle = CreateComputationDataHandle();

  OperationRequest& request =
      (*session_computation_.mutable_requests())[handle.handle()];
  *request.mutable_output_handle() = handle;
  *request.mutable_output_shape() = validated_shape;
  *request.mutable_request()->mutable_rng_request() = rng_request;

  VLOG(1) << "AddRngInstruction (" << GetVersionedHandleInternal()
          << "), data handle " << handle.handle() << ": "
          << rng_request.ShortDebugString();
  return handle;
}

StatusOr<ComputationDataHandle> UserComputation::AddMapInstruction(
    const MapRequest& map_request,
    const UserComputation& to_apply_computation) {
  tensorflow::mutex_lock lock(mutex_);

  std::vector<const Shape*> operand_shapes;
  for (const ComputationDataHandle& handle : map_request.operands()) {
    TF_ASSIGN_OR_RETURN(const OperationRequest* operand, LookUpRequest(handle));
    operand_shapes.push_back(&operand->output_shape());
  }

  VersionedComputationHandle::Version to_apply_version =
      to_apply_computation.version();
  TF_ASSIGN_OR_RETURN(
      std::shared_ptr<const ProgramShape> to_apply_program_shape,
      to_apply_computation.ComputeProgramShape(to_apply_version));
  TF_ASSIGN_OR_RETURN(
      Shape inferred_shape,
      ShapeInference::InferMapShape(operand_shapes, *to_apply_program_shape,
                                    AsInt64Slice(map_request.dimensions())));

  ComputationDataHandle handle = CreateComputationDataHandle();

  OperationRequest& request =
      (*session_computation_.mutable_requests())[handle.handle()];
  *request.mutable_output_handle() = handle;
  *request.mutable_output_shape() = inferred_shape;
  request.add_embedded_computation_versions(to_apply_version);
  *request.mutable_request()->mutable_map_request() = map_request;

  VLOG(1) << "AddMapInstruction (" << GetVersionedHandleInternal()
          << "), data handle " << handle.handle() << ": "
          << map_request.ShortDebugString();
  return handle;
}

StatusOr<ComputationDataHandle> UserComputation::AddReduceInstruction(
    const ReduceRequest& reduce_request,
    const UserComputation& to_apply_computation) {
  tensorflow::mutex_lock lock(mutex_);

  TF_ASSIGN_OR_RETURN(const OperationRequest* operand,
                      LookUpRequest(reduce_request.operand()));
  TF_ASSIGN_OR_RETURN(const OperationRequest* init_value,
                      LookUpRequest(reduce_request.init_value()));

  VersionedComputationHandle::Version to_apply_version =
      to_apply_computation.version();
  TF_ASSIGN_OR_RETURN(
      std::shared_ptr<const ProgramShape> to_apply_program_shape,
      to_apply_computation.ComputeProgramShape(to_apply_version));

  TF_ASSIGN_OR_RETURN(
      Shape inferred_shape,
      ShapeInference::InferReduceShape(
          operand->output_shape(), init_value->output_shape(),
          AsInt64Slice(reduce_request.dimensions()), *to_apply_program_shape));

  ComputationDataHandle handle = CreateComputationDataHandle();

  OperationRequest& request =
      (*session_computation_.mutable_requests())[handle.handle()];
  *request.mutable_output_handle() = handle;
  *request.mutable_output_shape() = inferred_shape;
  request.add_embedded_computation_versions(to_apply_version);
  *request.mutable_request()->mutable_reduce_request() = reduce_request;

  VLOG(1) << "AddReduceInstruction (" << GetVersionedHandleInternal()
          << "), data handle " << handle.handle() << ": "
          << reduce_request.ShortDebugString();
  return handle;
}

StatusOr<ComputationDataHandle>
UserComputation::AddBatchNormTrainingInstruction(
    const BatchNormTrainingRequest& batch_norm_training_request) {
  tensorflow::mutex_lock lock(mutex_);

  TF_ASSIGN_OR_RETURN(const OperationRequest* operand,
                      LookUpRequest(batch_norm_training_request.operand()));

  TF_ASSIGN_OR_RETURN(const OperationRequest* scale,
                      LookUpRequest(batch_norm_training_request.scale()));

  TF_ASSIGN_OR_RETURN(const OperationRequest* offset,
                      LookUpRequest(batch_norm_training_request.offset()));

  ComputationDataHandle handle = CreateComputationDataHandle();

  OperationRequest& request =
      (*session_computation_.mutable_requests())[handle.handle()];

  TF_ASSIGN_OR_RETURN(
      Shape inferred_shape,
      ShapeInference::InferBatchNormTrainingShape(
          operand->output_shape(), scale->output_shape(),
          offset->output_shape(), batch_norm_training_request.feature_index()));

  *request.mutable_output_shape() = inferred_shape;

  *request.mutable_output_handle() = handle;

  *request.mutable_request()->mutable_batch_norm_training_request() =
      batch_norm_training_request;

  VLOG(1) << "AddBatchNormTrainingInstruction (" << GetVersionedHandleInternal()
          << "), data handle " << handle.handle() << ": "
          << batch_norm_training_request.ShortDebugString();

  return handle;
}

StatusOr<ComputationDataHandle>
UserComputation::AddBatchNormInferenceInstruction(
    const BatchNormInferenceRequest& batch_norm_inference_request) {
  tensorflow::mutex_lock lock(mutex_);

  TF_ASSIGN_OR_RETURN(const OperationRequest* operand,
                      LookUpRequest(batch_norm_inference_request.operand()));

  TF_ASSIGN_OR_RETURN(const OperationRequest* scale,
                      LookUpRequest(batch_norm_inference_request.scale()));

  TF_ASSIGN_OR_RETURN(const OperationRequest* offset,
                      LookUpRequest(batch_norm_inference_request.offset()));

  TF_ASSIGN_OR_RETURN(const OperationRequest* mean,
                      LookUpRequest(batch_norm_inference_request.mean()));

  TF_ASSIGN_OR_RETURN(const OperationRequest* variance,
                      LookUpRequest(batch_norm_inference_request.variance()));

  ComputationDataHandle handle = CreateComputationDataHandle();

  OperationRequest& request =
      (*session_computation_.mutable_requests())[handle.handle()];

  TF_ASSIGN_OR_RETURN(Shape inferred_shape,
                      ShapeInference::InferBatchNormInferenceShape(
                          operand->output_shape(), scale->output_shape(),
                          offset->output_shape(), mean->output_shape(),
                          variance->output_shape(),
                          batch_norm_inference_request.feature_index()));

  *request.mutable_output_shape() = inferred_shape;

  *request.mutable_output_handle() = handle;

  *request.mutable_request()->mutable_batch_norm_inference_request() =
      batch_norm_inference_request;

  VLOG(1) << "AddBatchNormInferenceInstruction ("
          << GetVersionedHandleInternal() << "), data handle "
          << handle.handle() << ": "
          << batch_norm_inference_request.ShortDebugString();

  return handle;
}

StatusOr<ComputationDataHandle> UserComputation::AddBatchNormGradInstruction(
    const BatchNormGradRequest& batch_norm_grad_request) {
  tensorflow::mutex_lock lock(mutex_);

  TF_ASSIGN_OR_RETURN(const OperationRequest* operand,
                      LookUpRequest(batch_norm_grad_request.operand()));

  TF_ASSIGN_OR_RETURN(const OperationRequest* scale,
                      LookUpRequest(batch_norm_grad_request.scale()));

  TF_ASSIGN_OR_RETURN(const OperationRequest* mean,
                      LookUpRequest(batch_norm_grad_request.mean()));

  TF_ASSIGN_OR_RETURN(const OperationRequest* variance,
                      LookUpRequest(batch_norm_grad_request.variance()));

  TF_ASSIGN_OR_RETURN(const OperationRequest* grad_output,
                      LookUpRequest(batch_norm_grad_request.grad_output()));

  ComputationDataHandle handle = CreateComputationDataHandle();

  OperationRequest& request =
      (*session_computation_.mutable_requests())[handle.handle()];

  TF_ASSIGN_OR_RETURN(
      Shape inferred_shape,
      ShapeInference::InferBatchNormGradShape(
          operand->output_shape(), scale->output_shape(), mean->output_shape(),
          variance->output_shape(), grad_output->output_shape(),
          batch_norm_grad_request.feature_index()));

  *request.mutable_output_shape() = inferred_shape;

  *request.mutable_output_handle() = handle;

  *request.mutable_request()->mutable_batch_norm_grad_request() =
      batch_norm_grad_request;

  VLOG(1) << "AddBatchNormGradInstruction (" << GetVersionedHandleInternal()
          << "), data handle " << handle.handle() << ": "
          << batch_norm_grad_request.ShortDebugString();

  return handle;
}

StatusOr<ComputationDataHandle> UserComputation::AddReduceWindowInstruction(
    const ReduceWindowRequest& reduce_window_request,
    const UserComputation& to_apply_computation) {
  tensorflow::mutex_lock lock(mutex_);

  TF_ASSIGN_OR_RETURN(const OperationRequest* operand,
                      LookUpRequest(reduce_window_request.operand()));
  TF_ASSIGN_OR_RETURN(const OperationRequest* init_value,
                      LookUpRequest(reduce_window_request.init_value()));

  VersionedComputationHandle::Version to_apply_version =
      to_apply_computation.version();
  TF_ASSIGN_OR_RETURN(
      std::shared_ptr<const ProgramShape> to_apply_program_shape,
      to_apply_computation.ComputeProgramShape(to_apply_version));

  TF_ASSIGN_OR_RETURN(
      Shape inferred_shape,
      ShapeInference::InferReduceWindowShape(
          operand->output_shape(), init_value->output_shape(),
          reduce_window_request.window(), *to_apply_program_shape));

  ComputationDataHandle handle = CreateComputationDataHandle();

  OperationRequest& request =
      (*session_computation_.mutable_requests())[handle.handle()];
  *request.mutable_output_handle() = handle;
  *request.mutable_output_shape() = inferred_shape;
  request.add_embedded_computation_versions(to_apply_version);
  *request.mutable_request()->mutable_reduce_window_request() =
      reduce_window_request;

  VLOG(1) << "AddReduceWindowInstruction (" << GetVersionedHandleInternal()
          << "), data handle " << handle.handle() << ": "
          << reduce_window_request.ShortDebugString();
  return handle;
}

StatusOr<ComputationDataHandle> UserComputation::AddSelectAndScatterInstruction(
    const SelectAndScatterRequest& select_and_scatter_request,
    const UserComputation& select_computation,
    const UserComputation& scatter_computation) {
  tensorflow::mutex_lock lock(mutex_);

  TF_ASSIGN_OR_RETURN(const OperationRequest* operand,
                      LookUpRequest(select_and_scatter_request.operand()));
  TF_ASSIGN_OR_RETURN(const OperationRequest* source,
                      LookUpRequest(select_and_scatter_request.source()));
  TF_ASSIGN_OR_RETURN(const OperationRequest* init_value,
                      LookUpRequest(select_and_scatter_request.init_value()));

  VersionedComputationHandle::Version select_version =
      select_computation.version();
  TF_ASSIGN_OR_RETURN(std::shared_ptr<const ProgramShape> select_program_shape,
                      select_computation.ComputeProgramShape(select_version));
  VersionedComputationHandle::Version scatter_version =
      scatter_computation.version();
  TF_ASSIGN_OR_RETURN(std::shared_ptr<const ProgramShape> scatter_program_shape,
                      scatter_computation.ComputeProgramShape(scatter_version));

  TF_ASSIGN_OR_RETURN(
      Shape inferred_shape,
      ShapeInference::InferSelectAndScatterShape(
          operand->output_shape(), *select_program_shape,
          select_and_scatter_request.window(), source->output_shape(),
          init_value->output_shape(), *scatter_program_shape));

  ComputationDataHandle handle = CreateComputationDataHandle();

  OperationRequest& request =
      (*session_computation_.mutable_requests())[handle.handle()];
  *request.mutable_output_handle() = handle;
  *request.mutable_output_shape() = inferred_shape;
  request.add_embedded_computation_versions(select_version);
  request.add_embedded_computation_versions(scatter_version);
  *request.mutable_request()->mutable_select_and_scatter_request() =
      select_and_scatter_request;

  VLOG(1) << "AddSelectAndScatterInstruction (" << GetVersionedHandleInternal()
          << "), data handle " << handle.handle() << ": "
          << select_and_scatter_request.ShortDebugString();
  return handle;
}

StatusOr<ComputationDataHandle> UserComputation::AddReverseInstruction(
    const ReverseRequest& reverse_request) {
  tensorflow::mutex_lock lock(mutex_);

  TF_ASSIGN_OR_RETURN(const OperationRequest* operand,
                      LookUpRequest(reverse_request.operand()));
  TF_ASSIGN_OR_RETURN(
      Shape inferred_shape,
      ShapeInference::InferReverseShape(
          operand->output_shape(), AsInt64Slice(reverse_request.dimensions())));

  ComputationDataHandle handle = CreateComputationDataHandle();
  OperationRequest& request =
      (*session_computation_.mutable_requests())[handle.handle()];
  *request.mutable_output_handle() = handle;
  *request.mutable_output_shape() = inferred_shape;
  *request.mutable_request()->mutable_reverse_request() = reverse_request;
  VLOG(1) << "AddReverseInstruction (" << GetVersionedHandleInternal()
          << "), data handle " << handle.handle() << ": "
          << reverse_request.ShortDebugString();
  return handle;
}

StatusOr<ComputationDataHandle> UserComputation::AddWhileInstruction(
    const WhileRequest& while_request,
    const UserComputation& condition_computation,
    const UserComputation& body_computation) {
  tensorflow::mutex_lock lock(mutex_);

  TF_ASSIGN_OR_RETURN(const OperationRequest* init,
                      LookUpRequest(while_request.init()));

  VersionedComputationHandle::Version condition_version =
      condition_computation.version();
  TF_ASSIGN_OR_RETURN(
      std::shared_ptr<const ProgramShape> condition_program_shape,
      condition_computation.ComputeProgramShape(condition_version));

  VersionedComputationHandle::Version body_version = body_computation.version();
  TF_ASSIGN_OR_RETURN(std::shared_ptr<const ProgramShape> body_program_shape,
                      body_computation.ComputeProgramShape(body_version));

  TF_ASSIGN_OR_RETURN(
      Shape inferred_shape,
      ShapeInference::InferWhileShape(
          *condition_program_shape, *body_program_shape, init->output_shape()));

  ComputationDataHandle handle = CreateComputationDataHandle();

  OperationRequest& request =
      (*session_computation_.mutable_requests())[handle.handle()];
  *request.mutable_output_handle() = handle;
  *request.mutable_output_shape() = inferred_shape;
  request.add_embedded_computation_versions(condition_version);
  request.add_embedded_computation_versions(body_version);
  *request.mutable_request()->mutable_while_request() = while_request;

  VLOG(1) << "AddWhileInstruction (" << GetVersionedHandleInternal()
          << "), data handle " << handle.handle() << ": "
          << while_request.ShortDebugString();
  return handle;
}

StatusOr<ComputationDataHandle> UserComputation::AddConditionalInstruction(
    const ConditionalRequest& conditional_request,
    const UserComputation& true_computation,
    const UserComputation& false_computation) {
  tensorflow::mutex_lock lock(mutex_);

  TF_ASSIGN_OR_RETURN(const OperationRequest* pred,
                      LookUpRequest(conditional_request.predicate()));
  TF_ASSIGN_OR_RETURN(const OperationRequest* true_operand,
                      LookUpRequest(conditional_request.true_operand()));
  TF_ASSIGN_OR_RETURN(const OperationRequest* false_operand,
                      LookUpRequest(conditional_request.false_operand()));

  VersionedComputationHandle::Version true_computation_version =
      true_computation.version();
  TF_ASSIGN_OR_RETURN(
      std::shared_ptr<const ProgramShape> true_computation_shape,
      true_computation.ComputeProgramShape(true_computation_version));

  VersionedComputationHandle::Version false_computation_version =
      false_computation.version();
  TF_ASSIGN_OR_RETURN(
      std::shared_ptr<const ProgramShape> false_computation_shape,
      false_computation.ComputeProgramShape(false_computation_version));

  TF_ASSIGN_OR_RETURN(Shape inferred_shape,
                      ShapeInference::InferConditionalShape(
                          pred->output_shape(), true_operand->output_shape(),
                          false_operand->output_shape(),
                          *true_computation_shape, *false_computation_shape));

  ComputationDataHandle handle = CreateComputationDataHandle();

  OperationRequest& request =
      (*session_computation_.mutable_requests())[handle.handle()];
  *request.mutable_output_handle() = handle;
  *request.mutable_output_shape() = inferred_shape;
  request.add_embedded_computation_versions(true_computation_version);
  request.add_embedded_computation_versions(false_computation_version);
  *request.mutable_request()->mutable_conditional_request() =
      conditional_request;

  VLOG(1) << "AddConditionalInstruction (" << GetVersionedHandleInternal()
          << "), data handle " << handle.handle() << ": "
          << conditional_request.ShortDebugString();
  return handle;
}

StatusOr<ComputationDataHandle> UserComputation::AddBroadcastInstruction(
    const BroadcastRequest& broadcast_request) {
  tensorflow::mutex_lock lock(mutex_);

  // Fetches and validates the operand.
  TF_ASSIGN_OR_RETURN(const OperationRequest* operand,
                      LookUpRequest(broadcast_request.operand()));
  TF_ASSIGN_OR_RETURN(Shape inferred_shape,
                      ShapeInference::InferBroadcastShape(
                          operand->output_shape(),
                          AsInt64Slice(broadcast_request.broadcast_sizes())));

  ComputationDataHandle handle = CreateComputationDataHandle();
  OperationRequest& request =
      (*session_computation_.mutable_requests())[handle.handle()];
  *request.mutable_output_handle() = handle;
  *request.mutable_output_shape() = inferred_shape;
  *request.mutable_request()->mutable_broadcast_request() = broadcast_request;

  VLOG(1) << "AddBroadcastInstruction (" << GetVersionedHandleInternal()
          << "), data handle " << handle.handle() << ": "
          << broadcast_request.ShortDebugString();
  return handle;
}

StatusOr<ComputationDataHandle> UserComputation::AddReshapeInstruction(
    const ReshapeRequest& reshape_request) {
  tensorflow::mutex_lock lock(mutex_);

  // Fetches and validates the operand.
  TF_ASSIGN_OR_RETURN(const OperationRequest* operand,
                      LookUpRequest(reshape_request.operand()));

  TF_ASSIGN_OR_RETURN(
      Shape inferred_shape,
      ShapeInference::InferReshapeShape(
          operand->output_shape(), AsInt64Slice(reshape_request.dimensions()),
          AsInt64Slice(reshape_request.new_sizes())));

  ComputationDataHandle handle = CreateComputationDataHandle();

  OperationRequest& request =
      (*session_computation_.mutable_requests())[handle.handle()];
  *request.mutable_output_handle() = handle;
  *request.mutable_output_shape() = inferred_shape;
  *request.mutable_request()->mutable_reshape_request() = reshape_request;

  VLOG(1) << "AddReshapeInstruction (" << GetVersionedHandleInternal()
          << "), data handle " << handle.handle() << ": "
          << reshape_request.ShortDebugString();
  return handle;
}

StatusOr<ComputationDataHandle> UserComputation::AddTransposeInstruction(
    const TransposeRequest& transpose_request) {
  tensorflow::mutex_lock lock(mutex_);

  // Fetches and validates the operand.
  TF_ASSIGN_OR_RETURN(const OperationRequest* operand,
                      LookUpRequest(transpose_request.operand()));

  TF_ASSIGN_OR_RETURN(Shape inferred_shape,
                      ShapeInference::InferTransposeShape(
                          operand->output_shape(),
                          AsInt64Slice(transpose_request.dimensions())));

  ComputationDataHandle handle = CreateComputationDataHandle();

  OperationRequest& request =
      (*session_computation_.mutable_requests())[handle.handle()];
  *request.mutable_output_handle() = handle;
  *request.mutable_output_shape() = inferred_shape;
  *request.mutable_request()->mutable_transpose_request() = transpose_request;

  VLOG(1) << "AddTransposeInstruction (" << GetVersionedHandleInternal()
          << "), data handle " << handle.handle() << ": "
          << transpose_request.ShortDebugString();
  return handle;
}

StatusOr<ComputationDataHandle> UserComputation::AddSliceInstruction(
    const SliceRequest& slice_request) {
  tensorflow::mutex_lock lock(mutex_);

  TF_ASSIGN_OR_RETURN(const OperationRequest* operand,
                      LookUpRequest(slice_request.operand()));

  TF_ASSIGN_OR_RETURN(
      Shape new_shape,
      ShapeInference::InferSliceShape(
          operand->output_shape(), AsInt64Slice(slice_request.start_indices()),
          AsInt64Slice(slice_request.limit_indices()),
          AsInt64Slice(slice_request.strides())));

  ComputationDataHandle handle = CreateComputationDataHandle();

  OperationRequest& request =
      (*session_computation_.mutable_requests())[handle.handle()];
  *request.mutable_output_handle() = handle;
  *request.mutable_output_shape() = new_shape;
  *request.mutable_request()->mutable_slice_request() = slice_request;

  VLOG(1) << "AddSliceInstruction (" << GetVersionedHandleInternal()
          << "), data handle " << handle.handle() << ": "
          << slice_request.ShortDebugString();
  return handle;
}

StatusOr<ComputationDataHandle> UserComputation::AddDynamicSliceInstruction(
    const DynamicSliceRequest& dynamic_slice_request) {
  tensorflow::mutex_lock lock(mutex_);

  TF_ASSIGN_OR_RETURN(const OperationRequest* operand,
                      LookUpRequest(dynamic_slice_request.operand()));

  TF_ASSIGN_OR_RETURN(const OperationRequest* start_indices,
                      LookUpRequest(dynamic_slice_request.start_indices()));

  TF_ASSIGN_OR_RETURN(
      Shape new_shape,
      ShapeInference::InferDynamicSliceShape(
          operand->output_shape(), start_indices->output_shape(),
          AsInt64Slice(dynamic_slice_request.slice_sizes())));

  ComputationDataHandle handle = CreateComputationDataHandle();

  OperationRequest& request =
      (*session_computation_.mutable_requests())[handle.handle()];
  *request.mutable_output_handle() = handle;
  *request.mutable_output_shape() = new_shape;
  *request.mutable_request()->mutable_dynamic_slice_request() =
      dynamic_slice_request;

  VLOG(1) << "AddDynamicSliceInstruction (" << GetVersionedHandleInternal()
          << "), data handle " << handle.handle() << ": "
          << dynamic_slice_request.ShortDebugString();
  return handle;
}

StatusOr<ComputationDataHandle>
UserComputation::AddDynamicUpdateSliceInstruction(
    const DynamicUpdateSliceRequest& dynamic_update_slice_request) {
  tensorflow::mutex_lock lock(mutex_);

  TF_ASSIGN_OR_RETURN(const OperationRequest* operand,
                      LookUpRequest(dynamic_update_slice_request.operand()));

  TF_ASSIGN_OR_RETURN(const OperationRequest* update,
                      LookUpRequest(dynamic_update_slice_request.update()));

  TF_ASSIGN_OR_RETURN(
      const OperationRequest* start_indices,
      LookUpRequest(dynamic_update_slice_request.start_indices()));

  TF_ASSIGN_OR_RETURN(Shape new_shape,
                      ShapeInference::InferDynamicUpdateSliceShape(
                          operand->output_shape(), update->output_shape(),
                          start_indices->output_shape()));

  ComputationDataHandle handle = CreateComputationDataHandle();

  OperationRequest& request =
      (*session_computation_.mutable_requests())[handle.handle()];
  *request.mutable_output_handle() = handle;
  *request.mutable_output_shape() = new_shape;
  *request.mutable_request()->mutable_dynamic_update_slice_request() =
      dynamic_update_slice_request;

  VLOG(1) << "AddDynamicUpdateSliceInstruction ("
          << GetVersionedHandleInternal() << "), data handle "
          << handle.handle() << ": "
          << dynamic_update_slice_request.ShortDebugString();
  return handle;
}

StatusOr<ComputationDataHandle> UserComputation::AddConcatenateInstruction(
    const ConcatenateRequest& concatenate_request) {
  tensorflow::mutex_lock lock(mutex_);

  std::vector<const Shape*> operand_shapes;
  for (const ComputationDataHandle& handle : concatenate_request.operands()) {
    TF_ASSIGN_OR_RETURN(const OperationRequest* operand, LookUpRequest(handle));
    operand_shapes.push_back(&operand->output_shape());
  }

  TF_ASSIGN_OR_RETURN(Shape new_shape,
                      ShapeInference::InferConcatOpShape(
                          operand_shapes, concatenate_request.dimension()));

  ComputationDataHandle handle = CreateComputationDataHandle();

  OperationRequest& request =
      (*session_computation_.mutable_requests())[handle.handle()];
  *request.mutable_output_handle() = handle;
  *request.mutable_output_shape() = new_shape;
  *request.mutable_request()->mutable_concatenate_request() =
      concatenate_request;

  VLOG(1) << "AddConcatenateInstruction (" << GetVersionedHandleInternal()
          << "), data handle " << handle.handle() << ": "
          << concatenate_request.ShortDebugString();
  return handle;
}

StatusOr<ComputationDataHandle> UserComputation::AddConvertInstruction(
    const ConvertRequest& convert_request) {
  tensorflow::mutex_lock lock(mutex_);

  TF_ASSIGN_OR_RETURN(const OperationRequest* operand,
                      LookUpRequest(convert_request.operand()));

  TF_ASSIGN_OR_RETURN(Shape new_shape, ShapeInference::InferConvertShape(
                                           operand->output_shape(),
                                           convert_request.new_element_type()));

  ComputationDataHandle handle = CreateComputationDataHandle();

  OperationRequest& request =
      (*session_computation_.mutable_requests())[handle.handle()];
  *request.mutable_output_handle() = handle;
  *request.mutable_output_shape() = new_shape;
  *request.mutable_request()->mutable_convert_request() = convert_request;

  VLOG(1) << "AddConvertInstruction (" << GetVersionedHandleInternal()
          << "), data handle " << handle.handle() << ": "
          << convert_request.ShortDebugString();
  return handle;
}

StatusOr<ComputationDataHandle> UserComputation::AddBitcastConvertInstruction(
    const ConvertRequest& convert_request) {
  tensorflow::mutex_lock lock(mutex_);

  TF_ASSIGN_OR_RETURN(const OperationRequest* operand,
                      LookUpRequest(convert_request.operand()));

  TF_ASSIGN_OR_RETURN(Shape new_shape, ShapeInference::InferConvertShape(
                                           operand->output_shape(),
                                           convert_request.new_element_type()));

  ComputationDataHandle handle = CreateComputationDataHandle();

  OperationRequest& request =
      (*session_computation_.mutable_requests())[handle.handle()];
  *request.mutable_output_handle() = handle;
  *request.mutable_output_shape() = new_shape;
  *request.mutable_request()->mutable_bitcast_convert_request() =
      convert_request;

  VLOG(1) << "AddBitcastConvertInstruction (" << GetVersionedHandleInternal()
          << "), data handle " << handle.handle() << ": "
          << convert_request.ShortDebugString();
  return handle;
}

StatusOr<ComputationDataHandle> UserComputation::AddReducePrecisionInstruction(
    const ReducePrecisionRequest& reduce_precision_request) {
  tensorflow::mutex_lock lock(mutex_);

  TF_ASSIGN_OR_RETURN(const OperationRequest* operand,
                      LookUpRequest(reduce_precision_request.operand()));

  TF_ASSIGN_OR_RETURN(
      Shape new_shape,
      ShapeInference::InferReducePrecisionShape(
          operand->output_shape(), reduce_precision_request.exponent_bits(),
          reduce_precision_request.mantissa_bits()));

  ComputationDataHandle handle = CreateComputationDataHandle();

  OperationRequest& request =
      (*session_computation_.mutable_requests())[handle.handle()];
  *request.mutable_output_handle() = handle;
  *request.mutable_output_shape() = new_shape;
  *request.mutable_request()->mutable_reduce_precision_request() =
      reduce_precision_request;

  VLOG(1) << "AddReducePrecisionInstruction (" << GetVersionedHandleInternal()
          << "), data handle " << handle.handle() << ": "
          << reduce_precision_request.ShortDebugString();
  return handle;
}

StatusOr<ComputationDataHandle> UserComputation::AddConvolveInstruction(
    const ConvolveRequest& convolve_request) {
  tensorflow::mutex_lock lock(mutex_);

  TF_ASSIGN_OR_RETURN(const OperationRequest* lhs,
                      LookUpRequest(convolve_request.lhs()));
  TF_ASSIGN_OR_RETURN(const OperationRequest* rhs,
                      LookUpRequest(convolve_request.rhs()));
  TF_ASSIGN_OR_RETURN(Shape shape, ShapeInference::InferConvolveShape(
                                       lhs->output_shape(), rhs->output_shape(),
                                       convolve_request.window(),
                                       convolve_request.dimension_numbers()));

  const ComputationDataHandle handle = CreateComputationDataHandle();

  OperationRequest& request =
      (*session_computation_.mutable_requests())[handle.handle()];
  *request.mutable_output_handle() = handle;
  *request.mutable_output_shape() = shape;
  *request.mutable_request()->mutable_convolve_request() = convolve_request;

  VLOG(1) << "AddConvolveInstruction (" << GetVersionedHandleInternal()
          << "), data handle " << handle.handle() << ": "
          << convolve_request.ShortDebugString();
  return handle;
}

StatusOr<ComputationDataHandle> UserComputation::AddFftInstruction(
    const FftRequest& fft_request) {
  tensorflow::mutex_lock lock(mutex_);

  TF_ASSIGN_OR_RETURN(const OperationRequest* operand,
                      LookUpRequest(fft_request.operand()));
  TF_ASSIGN_OR_RETURN(Shape shape,
                      ShapeInference::InferFftShape(
                          operand->output_shape(), fft_request.fft_type(),
                          AsInt64Slice(fft_request.fft_length())));

  const ComputationDataHandle handle = CreateComputationDataHandle();

  OperationRequest& request =
      (*session_computation_.mutable_requests())[handle.handle()];
  *request.mutable_output_handle() = handle;
  *request.mutable_output_shape() = shape;
  *request.mutable_request()->mutable_fft_request() = fft_request;

  VLOG(1) << "AddFftInstruction (" << GetVersionedHandleInternal()
          << "), data handle " << handle.handle() << ": "
          << fft_request.ShortDebugString();
  return handle;
}

StatusOr<ComputationDataHandle> UserComputation::AddCrossReplicaSumInstruction(
    const CrossReplicaSumRequest& cross_replica_sum_request) {
  tensorflow::mutex_lock lock(mutex_);

  TF_ASSIGN_OR_RETURN(const OperationRequest* operand,
                      LookUpRequest(cross_replica_sum_request.operand()));
  TF_ASSIGN_OR_RETURN(Shape shape, ShapeInference::InferCrossReplicaSumShape(
                                       {&operand->output_shape()}));

  ComputationDataHandle handle = CreateComputationDataHandle();

  OperationRequest& request =
      (*session_computation_.mutable_requests())[handle.handle()];
  *request.mutable_output_handle() = handle;
  *request.mutable_output_shape() = shape;
  *request.mutable_request()->mutable_cross_replica_sum_request() =
      cross_replica_sum_request;

  VLOG(1) << "AddCrossreplicaSumInstruction (" << GetVersionedHandleInternal()
          << "), data handle " << handle.handle() << ": "
          << cross_replica_sum_request.ShortDebugString();
  return handle;
}

StatusOr<ComputationDataHandle> UserComputation::AddInfeedInstruction(
    const InfeedRequest& infeed_request) {
  tensorflow::mutex_lock lock(mutex_);

  const Shape& shape = infeed_request.shape();
  if (!LayoutUtil::HasLayout(shape)) {
    return InvalidArgument("Given shape to Infeed must have a layout");
  }

  const ComputationDataHandle handle = CreateComputationDataHandle();

  OperationRequest& request =
      (*session_computation_.mutable_requests())[handle.handle()];
  *request.mutable_output_handle() = handle;
  *request.mutable_output_shape() = shape;
  *request.mutable_request()->mutable_infeed_request() = infeed_request;

  VLOG(1) << "AddInfeedInstruction (" << GetVersionedHandleInternal()
          << "), data handle " << handle.handle() << ": "
          << infeed_request.ShortDebugString();
  return handle;
}

StatusOr<ComputationDataHandle> UserComputation::AddOutfeedInstruction(
    const OutfeedRequest& outfeed_request) {
  tensorflow::mutex_lock lock(mutex_);

  const Shape& shape = outfeed_request.shape();
  if (!LayoutUtil::HasLayout(shape)) {
    return InvalidArgument("Given shape to Outfeed must have a layout");
  }

  // Verify that operand is valid.
  TF_RETURN_IF_ERROR(LookUpRequest(outfeed_request.operand()).status());

  ComputationDataHandle handle = CreateComputationDataHandle();
  OperationRequest& request =
      (*session_computation_.mutable_requests())[handle.handle()];
  *request.mutable_output_handle() = handle;
  *request.mutable_output_shape() = shape;
  *request.mutable_request()->mutable_outfeed_request() = outfeed_request;

  VLOG(1) << "AddOutfeedInstruction (" << GetVersionedHandleInternal()
          << "), data handle " << handle.handle() << ": "
          << outfeed_request.ShortDebugString();
  return handle;
}

StatusOr<ComputationDataHandle> UserComputation::AddCallInstruction(
    const CallRequest& call_request,
    const UserComputation& to_apply_computation) {
  tensorflow::mutex_lock lock(mutex_);

  std::vector<const Shape*> operand_shapes;
  for (const ComputationDataHandle& handle : call_request.operands()) {
    TF_ASSIGN_OR_RETURN(const OperationRequest* operand, LookUpRequest(handle));
    operand_shapes.push_back(&operand->output_shape());
  }

  VersionedComputationHandle::Version to_apply_version =
      to_apply_computation.version();
  TF_ASSIGN_OR_RETURN(
      std::shared_ptr<const ProgramShape> to_apply_program_shape,
      to_apply_computation.ComputeProgramShape(to_apply_version));
  TF_ASSIGN_OR_RETURN(
      Shape inferred_shape,
      ShapeInference::InferCallShape(operand_shapes, *to_apply_program_shape));

  ComputationDataHandle handle = CreateComputationDataHandle();

  OperationRequest& request =
      (*session_computation_.mutable_requests())[handle.handle()];
  *request.mutable_output_handle() = handle;
  *request.mutable_output_shape() = inferred_shape;
  request.add_embedded_computation_versions(to_apply_version);
  *request.mutable_request()->mutable_call_request() = call_request;

  VLOG(1) << "AddCallInstruction (" << GetVersionedHandleInternal()
          << "), data handle " << handle.handle() << ": "
          << call_request.ShortDebugString();
  return handle;
}

StatusOr<ComputationDataHandle> UserComputation::AddCustomCallInstruction(
    const CustomCallRequest& custom_call_request) {
  tensorflow::mutex_lock lock(mutex_);

  for (const ComputationDataHandle& handle : custom_call_request.operands()) {
    TF_RETURN_IF_ERROR(LookUpRequest(handle).status());
  }

  if (tensorflow::StringPiece(custom_call_request.call_target_name())
          .starts_with("$")) {
    return InvalidArgument(
        "Invalid custom_call_target \"%s\": Call targets that start with '$' "
        "are reserved for internal use.",
        custom_call_request.call_target_name().c_str());
  }

  const ComputationDataHandle handle = CreateComputationDataHandle();

  OperationRequest& request =
      (*session_computation_.mutable_requests())[handle.handle()];
  *request.mutable_output_handle() = handle;
  *request.mutable_output_shape() = custom_call_request.shape();
  *request.mutable_request()->mutable_custom_call_request() =
      custom_call_request;

  VLOG(1) << "AddCustomCallInstruction (" << GetVersionedHandleInternal()
          << "), data handle " << handle.handle() << ": "
          << custom_call_request.ShortDebugString();
  return handle;
}

StatusOr<ComputationDataHandle> UserComputation::AddHostComputeInstruction(
    const HostComputeRequest& host_compute_request) {
  tensorflow::mutex_lock lock(mutex_);

  for (const ComputationDataHandle& handle : host_compute_request.operands()) {
    TF_RETURN_IF_ERROR(LookUpRequest(handle).status());
  }

  ComputationDataHandle handle = CreateComputationDataHandle();
  OperationRequest& request =
      (*session_computation_.mutable_requests())[handle.handle()];
  *request.mutable_output_handle() = handle;
  *request.mutable_output_shape() = host_compute_request.shape();
  *request.mutable_request()->mutable_host_compute_request() =
      host_compute_request;

  VLOG(1) << "AddHostComputeInstruction (" << GetVersionedHandleInternal()
          << "), data handle " << handle.handle() << ": "
          << host_compute_request.ShortDebugString();
  return handle;
}

StatusOr<ComputationDataHandle> UserComputation::AddDotInstruction(
    const DotRequest& dot_request) {
  tensorflow::mutex_lock lock(mutex_);

  TF_ASSIGN_OR_RETURN(const OperationRequest* lhs,
                      LookUpRequest(dot_request.lhs()));
  TF_ASSIGN_OR_RETURN(const OperationRequest* rhs,
                      LookUpRequest(dot_request.rhs()));

  TF_ASSIGN_OR_RETURN(Shape shape, ShapeInference::InferDotOpShape(
                                       lhs->output_shape(), rhs->output_shape(),
                                       dot_request.dimension_numbers()));

  const ComputationDataHandle handle = CreateComputationDataHandle();

  OperationRequest& request =
      (*session_computation_.mutable_requests())[handle.handle()];
  *request.mutable_output_handle() = handle;
  *request.mutable_output_shape() = shape;
  *request.mutable_request()->mutable_dot_request() = dot_request;

  VLOG(1) << "AddDotInstruction (" << GetVersionedHandleInternal()
          << "), data handle " << handle.handle() << ": "
          << dot_request.ShortDebugString();
  return handle;
}

StatusOr<ComputationDataHandle> UserComputation::AddUnaryInstruction(
    const UnaryOpRequest& unary_request) {
  tensorflow::mutex_lock lock(mutex_);

  TF_ASSIGN_OR_RETURN(const OperationRequest* operand,
                      LookUpRequest(unary_request.operand()));
  TF_ASSIGN_OR_RETURN(
      Shape shape, ShapeInference::InferUnaryOpShape(unary_request.unop(),
                                                     operand->output_shape()));

  ComputationDataHandle handle = CreateComputationDataHandle();

  OperationRequest& request =
      (*session_computation_.mutable_requests())[handle.handle()];
  *request.mutable_output_handle() = handle;
  *request.mutable_output_shape() = shape;
  *request.mutable_request()->mutable_unary_op_request() = unary_request;

  VLOG(1) << "AddUnaryInstruction (" << GetVersionedHandleInternal()
          << "), data handle " << handle.handle() << ": "
          << unary_request.ShortDebugString();
  return handle;
}

StatusOr<ComputationDataHandle> UserComputation::AddBinaryInstruction(
    const BinaryOpRequest& binary_request) {
  tensorflow::mutex_lock lock(mutex_);

  TF_ASSIGN_OR_RETURN(const OperationRequest* lhs,
                      LookUpRequest(binary_request.lhs()));
  TF_ASSIGN_OR_RETURN(const OperationRequest* rhs,
                      LookUpRequest(binary_request.rhs()));
  TF_ASSIGN_OR_RETURN(
      Shape shape,
      ShapeInference::InferBinaryOpShape(
          binary_request.binop(), lhs->output_shape(), rhs->output_shape(),
          AsInt64Slice(binary_request.broadcast_dimensions())));

  ComputationDataHandle handle = CreateComputationDataHandle();

  OperationRequest& request =
      (*session_computation_.mutable_requests())[handle.handle()];
  *request.mutable_output_handle() = handle;
  *request.mutable_output_shape() = shape;
  *request.mutable_request()->mutable_binary_op_request() = binary_request;

  VLOG(1) << "AddBinaryInstruction (" << GetVersionedHandleInternal()
          << "), data handle " << handle.handle() << ": "
          << binary_request.ShortDebugString();
  return handle;
}

StatusOr<ComputationDataHandle> UserComputation::AddTernaryInstruction(
    const TernaryOpRequest& ternary_request) {
  tensorflow::mutex_lock lock(mutex_);

  TF_ASSIGN_OR_RETURN(const OperationRequest* lhs,
                      LookUpRequest(ternary_request.lhs()));
  TF_ASSIGN_OR_RETURN(const OperationRequest* rhs,
                      LookUpRequest(ternary_request.rhs()));
  TF_ASSIGN_OR_RETURN(const OperationRequest* ehs,
                      LookUpRequest(ternary_request.ehs()));
  TF_ASSIGN_OR_RETURN(Shape shape,
                      ShapeInference::InferTernaryOpShape(
                          ternary_request.triop(), lhs->output_shape(),
                          rhs->output_shape(), ehs->output_shape()));

  ComputationDataHandle handle = CreateComputationDataHandle();

  OperationRequest& request =
      (*session_computation_.mutable_requests())[handle.handle()];
  *request.mutable_output_handle() = handle;
  *request.mutable_output_shape() = shape;
  *request.mutable_request()->mutable_ternary_op_request() = ternary_request;

  VLOG(1) << "AddTernaryInstruction (" << GetVersionedHandleInternal()
          << "), data handle " << handle.handle() << ": "
          << ternary_request.ShortDebugString();
  return handle;
}

StatusOr<ComputationDataHandle> UserComputation::AddVariadicInstruction(
    const VariadicOpRequest& variadic_request) {
  tensorflow::mutex_lock lock(mutex_);

  std::vector<const Shape*> operand_shapes;
  for (const ComputationDataHandle& handle : variadic_request.operands()) {
    TF_ASSIGN_OR_RETURN(const OperationRequest* operand, LookUpRequest(handle));
    operand_shapes.push_back(&operand->output_shape());
  }

  TF_ASSIGN_OR_RETURN(Shape shape,
                      ShapeInference::InferVariadicOpShape(
                          variadic_request.varop(), operand_shapes));

  ComputationDataHandle handle = CreateComputationDataHandle();

  OperationRequest& request =
      (*session_computation_.mutable_requests())[handle.handle()];
  *request.mutable_output_handle() = handle;
  *request.mutable_output_shape() = shape;
  *request.mutable_request()->mutable_variadic_op_request() = variadic_request;

  VLOG(1) << "AddVariadicInstruction (" << GetVersionedHandleInternal()
          << "), data handle " << handle.handle() << ": "
          << variadic_request.ShortDebugString();
  return handle;
}

StatusOr<Shape> UserComputation::GetShape(const ComputationDataHandle& handle) {
  tensorflow::mutex_lock lock(mutex_);

  TF_ASSIGN_OR_RETURN(const OperationRequest* operand, LookUpRequest(handle));
  return operand->output_shape();
}

Status UserComputation::SetOpMetadata(const ComputationDataHandle& handle,
                                      const OpMetadata& metadata) {
  tensorflow::mutex_lock lock(mutex_);

  int64 handle_value = handle.handle();
  if (session_computation_.requests().count(handle_value) == 0) {
    return InvalidArgument("Invalid handle in SetOpMetadata (%lld)",
                           handle_value);
  }
  *session_computation_.mutable_requests()
       ->at(handle_value)
       .mutable_request()
       ->mutable_metadata() = metadata;
  return Status::OK();
}

Status UserComputation::SetOpSharding(const ComputationDataHandle& handle,
                                      const OpSharding& sharding) {
  tensorflow::mutex_lock lock(mutex_);

  int64 handle_value = handle.handle();
  if (session_computation_.requests().count(handle_value) == 0) {
    return InvalidArgument("Invalid handle in SetOpSharding (%lld)",
                           handle_value);
  }
  *session_computation_.mutable_requests()
       ->at(handle_value)
       .mutable_request()
       ->mutable_sharding() = sharding;
  return Status::OK();
}

Status UserComputation::SetReturnValue(const ComputationDataHandle& handle) {
  tensorflow::mutex_lock lock(mutex_);

  if (!(handle.handle() > 0 && handle.handle() < next_handle_value_)) {
    return InvalidArgument("Invalid handle in SetReturnValue");
  }

  handle_to_return_ = handle;

  VLOG(1) << "SetReturnValue of computation \"" << name() << "\" fixed to "
          << GetVersionedHandleInternal();

  return Status::OK();
}

VersionedComputationHandle UserComputation::GetVersionedHandle() const {
  tensorflow::mutex_lock lock(mutex_);
  return GetVersionedHandleInternal();
}

VersionedComputationHandle UserComputation::GetVersionedHandleInternal() const {
  VersionedComputationHandle versioned_handle;
  versioned_handle.handle = session_computation_.computation_handle();

  if (handle_to_return_.handle() > 0) {
    // A specific handle has been requested for the result of the computation.
    versioned_handle.version = handle_to_return_.handle();
  } else {
    // A version value is simply the most recently assigned
    // ComputationDataHandle value, ie the handle value of the root of the
    // computation.
    versioned_handle.version = next_handle_value_ - 1;
  }

  return versioned_handle;
}

VersionedComputationHandle UserComputation::GetVersionedHandleAtOperation(
    const ComputationDataHandle& operation) const {
  tensorflow::mutex_lock lock(mutex_);

  // The version at which an operation was added is simply the handle value of
  // the ComputationDataHandle.
  VersionedComputationHandle versioned_handle;
  versioned_handle.handle = session_computation_.computation_handle();
  versioned_handle.version = operation.handle();
  return versioned_handle;
}

VersionedComputationHandle::Version UserComputation::version() const {
  return GetVersionedHandle().version;
}

namespace {

// Returns true if the operation type corresponding to the given opcase can be
// the root of the computation.
bool CanBeRoot(const OpRequest::OpCase& op_case) {
  switch (op_case) {
    case OpRequest::kTraceRequest:
    case OpRequest::kSendRequest:
    case OpRequest::kOutfeedRequest:
      return false;
    default:
      return true;
  }
}

// Returns a pointer to the operation with the given data handle value in the
// given SessionComputation.
StatusOr<const OperationRequest*> LookUpRequest(
    int64 handle_value, const SessionComputation& session_computation) {
  if (session_computation.requests().count(handle_value) == 0) {
    return InvalidArgument("no ComputationDataHandle value %lld", handle_value);
  }
  return &session_computation.requests().at(handle_value);
}

// Returns the OperationRequest corresponding to the root (result) of the
// session computation.
StatusOr<const OperationRequest*> GetRoot(
    VersionedComputationHandle::Version version,
    const SessionComputation& session_computation) {
  TF_RET_CHECK(version > 0);
  // Not all instructions can be roots. Walk backwards from the operation
  // indicated by this version until a valid root is found.
  const OperationRequest* root_request = nullptr;
  while (version > 0) {
    TF_ASSIGN_OR_RETURN(root_request,
                        LookUpRequest(version, session_computation));
    if (CanBeRoot(root_request->request().op_case())) {
      break;
    }
    version--;
  }
  if (version == 0) {
    return InternalError("Computation contains no root operation");
  }
  return root_request;
}

}  // namespace

StatusOr<std::shared_ptr<const ProgramShape>>
UserComputation::ComputeProgramShape(
    VersionedComputationHandle::Version version) const {
  tensorflow::mutex_lock lock(mutex_);

  TF_RET_CHECK(version > 0 && version < next_handle_value_);

  if (program_shape_ == nullptr || program_shape_version_ != version) {
    // ProgramShape has not been computed yet, or is for different
    // version. Compute it now.
    TF_RETURN_IF_ERROR(CheckParametersAreContiguous(version));

    auto program_shape = MakeUnique<ProgramShape>();
    for (int64 request_num = 1; request_num <= version; ++request_num) {
      const OperationRequest& request =
          session_computation_.requests().at(request_num);
      if (request.request().op_case() == OpRequest::kParameterRequest) {
        const ParameterRequest& parameter_request =
            request.request().parameter_request();
        int64 param_no = parameter_request.parameter();
        // Parameters may be out of order so expand ProgramShape parameters
        // until it is at least large enough to hold the current parameter
        // number.
        while (program_shape->parameters_size() <= param_no) {
          program_shape->add_parameters();
          program_shape->add_parameter_names();
        }
        *program_shape->mutable_parameters(param_no) = request.output_shape();
        *program_shape->mutable_parameter_names(param_no) =
            parameter_request.name();
      }
    }

    // The root determines the output shape.
    TF_ASSIGN_OR_RETURN(const OperationRequest* root_request,
                        GetRoot(version, session_computation_));
    *program_shape->mutable_result() = root_request->output_shape();
    if (ShapeUtil::IsOpaque(program_shape->result())) {
      return Unimplemented("Computation results cannot be opaque");
    }

    program_shape_ = std::move(program_shape);
    program_shape_version_ = version;
  }

  return program_shape_;
}

namespace {

// A visitor which checks whether an operation is pure functional meaning that
// it doesn't depend on any parameter with an index higher then num_parameters.
// The visitor walks the computation starting at a given operation and sets
// is_functional to false iff a parameter or RNG operation is encountered.
void PureFunctionalVisitor(const SessionComputation& session_computation,
                           const ComputationDataHandle& handle,
                           int64 num_parameters, std::set<int64>* visited,
                           bool* is_functional) {
  if (visited->count(handle.handle()) != 0 || !*is_functional) {
    return;
  }

  const OperationRequest& request =
      session_computation.requests().at(handle.handle());
  switch (request.request().op_case()) {
    case OpRequest::kRngRequest:
      *is_functional = false;
      break;

    case OpRequest::kConstantRequest:
      break;

    case OpRequest::kGetTupleElementRequest: {
      const GetTupleElementRequest& get_tuple_element_request =
          request.request().get_tuple_element_request();
      PureFunctionalVisitor(session_computation,
                            get_tuple_element_request.operand(), num_parameters,
                            visited, is_functional);
      break;
    }

    case OpRequest::kSliceRequest: {
      const SliceRequest& slice_request = request.request().slice_request();
      PureFunctionalVisitor(session_computation, slice_request.operand(),
                            num_parameters, visited, is_functional);
      break;
    }

    case OpRequest::kDynamicSliceRequest: {
      const DynamicSliceRequest& dynamic_slice_request =
          request.request().dynamic_slice_request();
      PureFunctionalVisitor(session_computation,
                            dynamic_slice_request.operand(), num_parameters,
                            visited, is_functional);
      PureFunctionalVisitor(session_computation,
                            dynamic_slice_request.start_indices(),
                            num_parameters, visited, is_functional);
      break;
    }

    case OpRequest::kDynamicUpdateSliceRequest: {
      const DynamicUpdateSliceRequest& dynamic_update_slice_request =
          request.request().dynamic_update_slice_request();
      PureFunctionalVisitor(session_computation,
                            dynamic_update_slice_request.operand(),
                            num_parameters, visited, is_functional);
      PureFunctionalVisitor(session_computation,
                            dynamic_update_slice_request.update(),
                            num_parameters, visited, is_functional);
      PureFunctionalVisitor(session_computation,
                            dynamic_update_slice_request.start_indices(),
                            num_parameters, visited, is_functional);
      break;
    }

    case OpRequest::kConcatenateRequest: {
      const ConcatenateRequest& concatenate_request =
          request.request().concatenate_request();
      for (const ComputationDataHandle& handle :
           concatenate_request.operands()) {
        PureFunctionalVisitor(session_computation, handle, num_parameters,
                              visited, is_functional);
      }
      break;
    }

    case OpRequest::kConvolveRequest: {
      const ConvolveRequest& convolve_request =
          request.request().convolve_request();
      PureFunctionalVisitor(session_computation, convolve_request.lhs(),
                            num_parameters, visited, is_functional);
      PureFunctionalVisitor(session_computation, convolve_request.rhs(),
                            num_parameters, visited, is_functional);
      break;
    }

    case OpRequest::kFftRequest: {
      const FftRequest& fft_request = request.request().fft_request();
      PureFunctionalVisitor(session_computation, fft_request.operand(),
                            num_parameters, visited, is_functional);
      break;
    }

    case OpRequest::kCrossReplicaSumRequest: {
      // TODO(b/33009255): Implmement constant folding for cross replica sum.
      *is_functional = false;
      break;
    }

    case OpRequest::kInfeedRequest: {
      *is_functional = false;
      break;
    }

    case OpRequest::kOutfeedRequest: {
      *is_functional = false;
      break;
    }

    case OpRequest::kHostComputeRequest: {
      *is_functional = false;
      break;
    }

    case OpRequest::kCallRequest: {
      const CallRequest& call_request = request.request().call_request();
      for (const ComputationDataHandle& handle : call_request.operands()) {
        PureFunctionalVisitor(session_computation, handle, num_parameters,
                              visited, is_functional);
      }
      // TODO(b/32495713): We aren't checking the to_apply computation itself,
      // so we conservatively say that computations containing the Call op
      // cannot be constant.  We cannot set is_functional=false in other similar
      // cases since we're already relying on IsConstant to return true.
      *is_functional = false;
      break;
    }

    case OpRequest::kCustomCallRequest: {
      *is_functional = false;
      break;
    }

    case OpRequest::kDotRequest: {
      const DotRequest& dot_request = request.request().dot_request();
      PureFunctionalVisitor(session_computation, dot_request.lhs(),
                            num_parameters, visited, is_functional);
      PureFunctionalVisitor(session_computation, dot_request.rhs(),
                            num_parameters, visited, is_functional);
      break;
    }

    case OpRequest::kSendRequest: {
      *is_functional = false;
      break;
    }

    case OpRequest::kRecvRequest: {
      *is_functional = false;
      break;
    }

    case OpRequest::kMapRequest: {
      const MapRequest& map_request = request.request().map_request();
      for (const ComputationDataHandle& handle : map_request.operands()) {
        PureFunctionalVisitor(session_computation, handle, num_parameters,
                              visited, is_functional);
      }
      // TODO(b/32495713): We aren't checking the to_apply computation itself.
      break;
    }

    case OpRequest::kReduceRequest: {
      const ReduceRequest& reduce_request = request.request().reduce_request();
      PureFunctionalVisitor(session_computation, reduce_request.operand(),
                            num_parameters, visited, is_functional);
      PureFunctionalVisitor(session_computation, reduce_request.init_value(),
                            num_parameters, visited, is_functional);
      // TODO(b/32495713): We aren't checking the to_apply computation itself.
      break;
    }

    case OpRequest::kReduceWindowRequest: {
      const ReduceWindowRequest& reduce_window_request =
          request.request().reduce_window_request();
      PureFunctionalVisitor(session_computation,
                            reduce_window_request.operand(), num_parameters,
                            visited, is_functional);
      PureFunctionalVisitor(session_computation,
                            reduce_window_request.init_value(), num_parameters,
                            visited, is_functional);
      // TODO(b/32495713): We aren't checking the to_apply computation itself.
      break;
    }

    case OpRequest::kSelectAndScatterRequest: {
      const SelectAndScatterRequest& select_and_scatter_request =
          request.request().select_and_scatter_request();
      PureFunctionalVisitor(session_computation,
                            select_and_scatter_request.operand(),
                            num_parameters, visited, is_functional);
      PureFunctionalVisitor(session_computation,
                            select_and_scatter_request.source(), num_parameters,
                            visited, is_functional);
      PureFunctionalVisitor(session_computation,
                            select_and_scatter_request.init_value(),
                            num_parameters, visited, is_functional);
      // TODO(b/32495713): We aren't checking the select and scatter
      // computations themselves.
      break;
    }

    case OpRequest::kBroadcastRequest: {
      const BroadcastRequest& broadcast_request =
          request.request().broadcast_request();
      PureFunctionalVisitor(session_computation, broadcast_request.operand(),
                            num_parameters, visited, is_functional);
      break;
    }

    case OpRequest::kReshapeRequest: {
      const ReshapeRequest& reshape_request =
          request.request().reshape_request();
      PureFunctionalVisitor(session_computation, reshape_request.operand(),
                            num_parameters, visited, is_functional);
      break;
    }

    case OpRequest::kReverseRequest: {
      const ReverseRequest& reverse_request =
          request.request().reverse_request();
      PureFunctionalVisitor(session_computation, reverse_request.operand(),
                            num_parameters, visited, is_functional);
      break;
    }

    case OpRequest::kPadRequest: {
      const PadRequest& pad_request = request.request().pad_request();
      PureFunctionalVisitor(session_computation, pad_request.operand(),
                            num_parameters, visited, is_functional);
      PureFunctionalVisitor(session_computation, pad_request.padding_value(),
                            num_parameters, visited, is_functional);
      break;
    }

    case OpRequest::kParameterRequest: {
      const ParameterRequest& parameter_request =
          request.request().parameter_request();
      if (parameter_request.parameter() >= num_parameters) {
        *is_functional = false;
      }
      break;
    }

    case OpRequest::kConvertRequest: {
      const ConvertRequest& convert_request =
          request.request().convert_request();
      PureFunctionalVisitor(session_computation, convert_request.operand(),
                            num_parameters, visited, is_functional);
      break;
    }

    case OpRequest::kBitcastConvertRequest: {
      const ConvertRequest& convert_request =
          request.request().bitcast_convert_request();
      PureFunctionalVisitor(session_computation, convert_request.operand(),
                            num_parameters, visited, is_functional);
      break;
    }

    case OpRequest::kWhileRequest: {
      const WhileRequest& while_request = request.request().while_request();
      PureFunctionalVisitor(session_computation, while_request.init(),
                            num_parameters, visited, is_functional);
      // TODO(b/32495713): We aren't checking the condition and body
      // computations themselves.
      *is_functional = false;
      break;
    }

    case OpRequest::kConditionalRequest: {
      const ConditionalRequest& conditional_request =
          request.request().conditional_request();
      PureFunctionalVisitor(session_computation,
                            conditional_request.predicate(), num_parameters,
                            visited, is_functional);
      PureFunctionalVisitor(session_computation,
                            conditional_request.true_operand(), num_parameters,
                            visited, is_functional);
      PureFunctionalVisitor(session_computation,
                            conditional_request.false_operand(), num_parameters,
                            visited, is_functional);
      // TODO(b/32495713): We aren't checking the true and false computations
      // themselves.
      break;
    }

    case OpRequest::kTernaryOpRequest: {
      const TernaryOpRequest& ternary_op_request =
          request.request().ternary_op_request();
      PureFunctionalVisitor(session_computation, ternary_op_request.lhs(),
                            num_parameters, visited, is_functional);
      PureFunctionalVisitor(session_computation, ternary_op_request.rhs(),
                            num_parameters, visited, is_functional);
      PureFunctionalVisitor(session_computation, ternary_op_request.ehs(),
                            num_parameters, visited, is_functional);
      break;
    }

    case OpRequest::kTransposeRequest: {
      const TransposeRequest& transpose_request =
          request.request().transpose_request();
      PureFunctionalVisitor(session_computation, transpose_request.operand(),
                            num_parameters, visited, is_functional);
      break;
    }

    case OpRequest::kVariadicOpRequest: {
      const VariadicOpRequest& variadic_op_request =
          request.request().variadic_op_request();
      for (const ComputationDataHandle& handle :
           variadic_op_request.operands()) {
        PureFunctionalVisitor(session_computation, handle, num_parameters,
                              visited, is_functional);
      }
      break;
    }

    case OpRequest::kUnaryOpRequest: {
      const UnaryOpRequest& unary_op_request =
          request.request().unary_op_request();
      PureFunctionalVisitor(session_computation, unary_op_request.operand(),
                            num_parameters, visited, is_functional);
      break;
    }

    case OpRequest::kBatchNormTrainingRequest: {
      const BatchNormTrainingRequest& batch_norm_training_request =
          request.request().batch_norm_training_request();
      PureFunctionalVisitor(session_computation,
                            batch_norm_training_request.operand(),
                            num_parameters, visited, is_functional);
      PureFunctionalVisitor(session_computation,
                            batch_norm_training_request.scale(), num_parameters,
                            visited, is_functional);
      PureFunctionalVisitor(session_computation,
                            batch_norm_training_request.offset(),
                            num_parameters, visited, is_functional);
      break;
    }

    case OpRequest::kBatchNormInferenceRequest: {
      const BatchNormInferenceRequest& batch_norm_inference_request =
          request.request().batch_norm_inference_request();
      PureFunctionalVisitor(session_computation,
                            batch_norm_inference_request.operand(),
                            num_parameters, visited, is_functional);
      PureFunctionalVisitor(session_computation,
                            batch_norm_inference_request.scale(),
                            num_parameters, visited, is_functional);
      PureFunctionalVisitor(session_computation,
                            batch_norm_inference_request.offset(),
                            num_parameters, visited, is_functional);
      PureFunctionalVisitor(session_computation,
                            batch_norm_inference_request.mean(), num_parameters,
                            visited, is_functional);
      PureFunctionalVisitor(session_computation,
                            batch_norm_inference_request.variance(),
                            num_parameters, visited, is_functional);
      break;
    }

    case OpRequest::kBatchNormGradRequest: {
      const BatchNormGradRequest& batch_norm_grad_request =
          request.request().batch_norm_grad_request();
      PureFunctionalVisitor(session_computation,
                            batch_norm_grad_request.operand(), num_parameters,
                            visited, is_functional);
      PureFunctionalVisitor(session_computation,
                            batch_norm_grad_request.scale(), num_parameters,
                            visited, is_functional);
      PureFunctionalVisitor(session_computation, batch_norm_grad_request.mean(),
                            num_parameters, visited, is_functional);
      PureFunctionalVisitor(session_computation,
                            batch_norm_grad_request.variance(), num_parameters,
                            visited, is_functional);
      PureFunctionalVisitor(session_computation,
                            batch_norm_grad_request.grad_output(),
                            num_parameters, visited, is_functional);
      break;
    }

    case OpRequest::kBinaryOpRequest: {
      const BinaryOpRequest& binary_op_request =
          request.request().binary_op_request();
      PureFunctionalVisitor(session_computation, binary_op_request.lhs(),
                            num_parameters, visited, is_functional);
      PureFunctionalVisitor(session_computation, binary_op_request.rhs(),
                            num_parameters, visited, is_functional);
      break;
    }

    case OpRequest::kGatherRequest: {
      PureFunctionalVisitor(session_computation,
                            request.request().gather_request().input(),
                            num_parameters, visited, is_functional);
      PureFunctionalVisitor(session_computation,
                            request.request().gather_request().gather_indices(),
                            num_parameters, visited, is_functional);
      break;
    }

    case OpRequest::OP_NOT_SET:
      LOG(FATAL) << "OperationRequest doesn't contain a request";

    default:
      LOG(FATAL) << "Unexpected request type: " << request.request().op_case();
  }
  if (!*is_functional) {
    VLOG(1) << "Non-functional: " << request.request().DebugString();
  }
  visited->insert(handle.handle());
}

}  // namespace

StatusOr<bool> UserComputation::IsConstant(const ComputationDataHandle& handle,
                                           int64 num_parameters) {
  tensorflow::mutex_lock lock(mutex_);

  // Verify that the handle is valid.
  auto operation_status = LookUpRequest(handle);
  if (!operation_status.ok()) {
    return operation_status.status();
  }

  bool is_constant = true;
  std::set<int64> visited;
  PureFunctionalVisitor(session_computation_, handle, num_parameters, &visited,
                        &is_constant);

  return is_constant;
}

std::vector<VersionedComputationHandle>
UserComputation::GetEmbeddedComputations(
    VersionedComputationHandle::Version version) const {
  tensorflow::mutex_lock lock(mutex_);

  VLOG(1)
      << "GetEmbeddedComputations(" << name() << " "
      << VersionedComputationHandle{session_computation_.computation_handle(),
                                    version}
      << ")";
  XLA_VLOG_LINES(3, session_computation_.DebugString());

  std::vector<VersionedComputationHandle> computations;
  std::vector<int64> sorted_handles;
  for (const auto& handle_request : session_computation_.requests()) {
    sorted_handles.push_back(handle_request.first);
  }
  std::sort(sorted_handles.begin(), sorted_handles.end());
  for (int64 handle : sorted_handles) {
    const auto& handle_request = session_computation_.requests().find(handle);
    CHECK(handle_request != session_computation_.requests().end());
    int64 handle_value = handle_request->first;
    if (handle_value <= version) {
      const OperationRequest& request = handle_request->second;
      switch (request.request().op_case()) {
        case OpRequest::kCallRequest: {
          CHECK_EQ(1, request.embedded_computation_versions_size());
          const CallRequest& call_request = request.request().call_request();
          const VersionedComputationHandle versioned_handle = {
              call_request.to_apply(),
              request.embedded_computation_versions(0)};
          computations.push_back(versioned_handle);
          break;
        }

        case OpRequest::kMapRequest: {
          CHECK_EQ(1, request.embedded_computation_versions_size());
          const MapRequest& map_request = request.request().map_request();
          const VersionedComputationHandle versioned_handle = {
              map_request.to_apply(), request.embedded_computation_versions(0)};
          computations.push_back(versioned_handle);
          break;
        }

        case OpRequest::kReduceRequest: {
          CHECK_EQ(1, request.embedded_computation_versions_size());
          const ReduceRequest& reduce_request =
              request.request().reduce_request();
          const VersionedComputationHandle versioned_handle = {
              reduce_request.to_apply(),
              request.embedded_computation_versions(0)};
          computations.push_back(versioned_handle);
          break;
        }

        case OpRequest::kReduceWindowRequest: {
          CHECK_EQ(1, request.embedded_computation_versions_size());
          const ReduceWindowRequest& reduce_window_request =
              request.request().reduce_window_request();
          const VersionedComputationHandle versioned_handle = {
              reduce_window_request.to_apply(),
              request.embedded_computation_versions(0)};
          computations.push_back(versioned_handle);
          break;
        }

        case OpRequest::kSelectAndScatterRequest: {
          CHECK_EQ(2, request.embedded_computation_versions_size());
          const SelectAndScatterRequest& select_and_scatter_request =
              request.request().select_and_scatter_request();
          const VersionedComputationHandle select_versioned_handle = {
              select_and_scatter_request.select(),
              request.embedded_computation_versions(0)};
          computations.push_back(select_versioned_handle);
          const VersionedComputationHandle scatter_versioned_handle = {
              select_and_scatter_request.scatter(),
              request.embedded_computation_versions(1)};
          computations.push_back(scatter_versioned_handle);
          break;
        }

        case OpRequest::kWhileRequest: {
          CHECK_EQ(2, request.embedded_computation_versions_size());
          const WhileRequest& while_request = request.request().while_request();
          const VersionedComputationHandle condition_versioned_handle = {
              while_request.condition(),
              request.embedded_computation_versions(0)};
          computations.push_back(condition_versioned_handle);
          const VersionedComputationHandle body_versioned_handle = {
              while_request.body(), request.embedded_computation_versions(1)};
          computations.push_back(body_versioned_handle);
          break;
        }

        case OpRequest::kConditionalRequest: {
          CHECK_EQ(2, request.embedded_computation_versions_size());
          const ConditionalRequest& conditional_request =
              request.request().conditional_request();
          const VersionedComputationHandle true_computation_versioned_handle = {
              conditional_request.true_computation(),
              request.embedded_computation_versions(0)};
          computations.push_back(true_computation_versioned_handle);
          const VersionedComputationHandle false_computation_versioned_handle =
              {conditional_request.false_computation(),
               request.embedded_computation_versions(1)};
          computations.push_back(false_computation_versioned_handle);
          break;
        }

        default:
          // No embedded computation.
          break;
      }
    }
  }
  VLOG(2) << "Embedded computations: "
          << tensorflow::str_util::Join(
                 computations, ", ",
                 [](string* out, const VersionedComputationHandle& h) {
                   out->append(h.ToString());
                 });
  return computations;
}

StatusOr<const OperationRequest*>
UserComputation::LookUpRequestForErrorReporting(
    const ComputationDataHandle& handle) const {
  tensorflow::mutex_lock lock(mutex_);
  return LookUpRequest(handle);
}

tensorflow::gtl::optional<const OpMetadata*> UserComputation::ParameterMetadata(
    int parameter_number) const {
  tensorflow::mutex_lock lock(mutex_);
  auto it = parameters_.find(parameter_number);
  if (it == parameters_.end()) {
    return tensorflow::gtl::nullopt;
  }
  OperationRequest* op = it->second;
  return &op->request().metadata();
}

Status UserComputation::RemapEmbeddedComputations(
    const std::map<int64, ComputationHandle>& old_to_new) {
  auto update = [&old_to_new](ComputationHandle* to_update) -> Status {
    int64 old = to_update->handle();
    auto it = old_to_new.find(old);
    if (it == old_to_new.end()) {
      string mapping = tensorflow::str_util::Join(
          old_to_new, ", ",
          [](string* out, std::pair<int64, ComputationHandle> element) {
            tensorflow::strings::Appendf(out, "%lld:%lld", element.first,
                                         element.second.handle());
          });
      return NotFound(
          "could not find referenced (old) computation handle in mapping: "
          "%lld; mapping: {%s}",
          old, mapping.c_str());
    }
    VLOG(2) << "remapping " << old << " to " << it->second.handle();
    *to_update = it->second;
    return Status::OK();
  };
  TF_RETURN_IF_ERROR(update(session_computation_.mutable_computation_handle()));
  for (auto& handle_request : *session_computation_.mutable_requests()) {
    OperationRequest& request = handle_request.second;
    switch (request.request().op_case()) {
      case OpRequest::kCallRequest: {
        TF_RET_CHECK(1 == request.embedded_computation_versions_size());
        CallRequest* call_request =
            request.mutable_request()->mutable_call_request();
        TF_RETURN_IF_ERROR(update(call_request->mutable_to_apply()));
        break;
      }
      case OpRequest::kMapRequest: {
        TF_RET_CHECK(1 == request.embedded_computation_versions_size());
        MapRequest* map_request =
            request.mutable_request()->mutable_map_request();
        TF_RETURN_IF_ERROR(update(map_request->mutable_to_apply()));
        break;
      }
      case OpRequest::kReduceRequest: {
        TF_RET_CHECK(1 == request.embedded_computation_versions_size());
        ReduceRequest* reduce_request =
            request.mutable_request()->mutable_reduce_request();
        TF_RETURN_IF_ERROR(update(reduce_request->mutable_to_apply()));
        break;
      }
      case OpRequest::kReduceWindowRequest: {
        TF_RET_CHECK(1 == request.embedded_computation_versions_size());
        ReduceWindowRequest* reduce_window_request =
            request.mutable_request()->mutable_reduce_window_request();
        TF_RETURN_IF_ERROR(update(reduce_window_request->mutable_to_apply()));
        break;
      }
      case OpRequest::kSelectAndScatterRequest: {
        TF_RET_CHECK(2 == request.embedded_computation_versions_size());
        SelectAndScatterRequest* select_and_scatter_request =
            request.mutable_request()->mutable_select_and_scatter_request();
        TF_RETURN_IF_ERROR(
            update(select_and_scatter_request->mutable_select()));
        TF_RETURN_IF_ERROR(
            update(select_and_scatter_request->mutable_scatter()));
        break;
      }
      case OpRequest::kWhileRequest: {
        TF_RET_CHECK(2 == request.embedded_computation_versions_size());
        WhileRequest* while_request =
            request.mutable_request()->mutable_while_request();
        TF_RETURN_IF_ERROR(update(while_request->mutable_condition()));
        TF_RETURN_IF_ERROR(update(while_request->mutable_body()));
        break;
      }
      case OpRequest::kConditionalRequest: {
        TF_RET_CHECK(2 == request.embedded_computation_versions_size());
        ConditionalRequest* conditional_request =
            request.mutable_request()->mutable_conditional_request();
        TF_RETURN_IF_ERROR(
            update(conditional_request->mutable_true_computation()));
        TF_RETURN_IF_ERROR(
            update(conditional_request->mutable_false_computation()));
        break;
      }
      default:
        // No embedded computation.
        TF_RET_CHECK(0 == request.embedded_computation_versions_size());
        break;
    }
  }
  return Status::OK();
}

SessionComputation UserComputation::CloneSessionComputation(
    VersionedComputationHandle::Version version) const {
  tensorflow::mutex_lock lock(mutex_);
  SessionComputation result = session_computation_;
  // Erase all the requests that exceed the version specified.
  // There's no lower_bound method on tensorflow::protobuf::Map so we iterate
  // all the elements.
  auto it = result.mutable_requests()->begin();
  while (it != result.mutable_requests()->end()) {
    if (it->first > version) {
      it = result.mutable_requests()->erase(it);
    } else {
      ++it;
    }
  }
  return result;
}

StatusOr<const OperationRequest*> UserComputation::LookUpRequest(
    const ComputationDataHandle& handle) const {
  int64 handle_value = handle.handle();
  if (session_computation_.requests().count(handle_value) == 0) {
    return InvalidArgument("no ComputationDataHandle value %lld", handle_value);
  }
  return &session_computation_.requests().at(handle_value);
}

Status UserComputation::CheckParametersAreContiguous(
    VersionedComputationHandle::Version version) const {
  TF_RET_CHECK(version > 0 && version < next_handle_value_);

  // Determine number of parameter inputs at the given version.
  std::map<int64, const ParameterRequest*> parameter_requests;
  for (int64 request_num = 1; request_num <= version; ++request_num) {
    const OperationRequest& request =
        session_computation_.requests().at(request_num);

    if (request.request().op_case() == OpRequest::kParameterRequest) {
      const ParameterRequest& parameter_request =
          request.request().parameter_request();
      // Duplicate parameters should be checked when parameter requests are
      // added.
      TF_RET_CHECK(0 ==
                   parameter_requests.count(parameter_request.parameter()));
      parameter_requests[parameter_request.parameter()] = &parameter_request;
    }
  }

  for (int64 i = 0; i < parameter_requests.size(); ++i) {
    auto it = parameter_requests.find(i);
    if (it == parameter_requests.end()) {
      return FailedPrecondition(
          "computation %s does not have all its parameters populated "
          "sequentially, missing parameter %lld",
          name_.c_str(), i);
    }
  }

  return Status::OK();
}

namespace {

// Helper class which builds an HLO computation from a SessionComputation. To
// construct the HLO computation, the SessionComputation graph is walked in
// DFS order lowering each OperationRequest to an HLO instruction.
class ComputationLowerer {
 public:
  static StatusOr<std::unique_ptr<HloComputation>> Lower(
      const string& computation_name,
      const SessionComputation& session_computation,
      VersionedComputationHandle::Version version,
      UserComputation::HloComputationResolver hlo_resolver,
      const DebugOptions& debug_options,
      bool include_unreachable_instructions) {
    ComputationLowerer lowerer(computation_name, session_computation, version,
                               std::move(hlo_resolver), debug_options,
                               include_unreachable_instructions);
    return lowerer.Lower();
  }

 private:
  ComputationLowerer(const string& computation_name,
                     const SessionComputation& session_computation,
                     VersionedComputationHandle::Version version,
                     UserComputation::HloComputationResolver hlo_resolver,
                     const DebugOptions& debug_options,
                     bool include_unreachable_instructions)
      : hlo_builder_(computation_name),
        session_computation_(session_computation),
        version_(version),
        hlo_resolver_(std::move(hlo_resolver)),
        debug_options_(debug_options),
        include_unreachable_instructions_(include_unreachable_instructions) {}

  // Build an HLO computation from the SessionComputation at the given
  // version.
  StatusOr<std::unique_ptr<HloComputation>> Lower();

 private:
  // Traverses the computation 'root' using a DFS, calling 'visit' in postorder.
  void TraversePostorder(
      const ComputationDataHandle& root,
      std::unordered_map<int64, HloInstruction*>* visited,
      const std::function<void(const ComputationDataHandle&)>& visit);

  // DFS visitor of the UserComputation operations which lowers the operations
  // to HLO instructions.
  void Visit(const ComputationDataHandle& handle,
             std::unordered_map<int64, HloInstruction*>* instructions);

  // Resolves a ComputationHandle and Version to a previously lowered
  // HloComputation using the hlo_resolver_ function.
  HloComputation* ResolveComputation(
      const ComputationHandle& handle,
      VersionedComputationHandle::Version version);

  // This function takes an input value which is being implicitly broadcast into
  // an output shape and figures out the right kBroadcast instruction(s)
  // necessary to replicate the implicit broadcast semantics explicitly.
  HloInstruction* ImplicitBroadcastToExplicitBroadcast(
      HloInstruction* operand, const Shape& output_shape);

  HloComputation::Builder hlo_builder_;
  const SessionComputation& session_computation_;
  const VersionedComputationHandle::Version version_;
  const UserComputation::HloComputationResolver hlo_resolver_;
  const DebugOptions& debug_options_;
  const bool include_unreachable_instructions_;
};

// Calls 'apply' on each operand of 'request'.
static void ForEachOperand(
    const OperationRequest& request,
    const std::function<void(const ComputationDataHandle& param)>& apply) {
  switch (request.request().op_case()) {
    case OpRequest::kRngRequest: {
      const RngRequest& rng_request = request.request().rng_request();
      for (const ComputationDataHandle& param : rng_request.parameter()) {
        apply(param);
      }
      break;
    }

    case OpRequest::kConstantRequest:
      break;
    case OpRequest::kGetTupleElementRequest: {
      const GetTupleElementRequest& get_tuple_element_request =
          request.request().get_tuple_element_request();
      apply(get_tuple_element_request.operand());
      break;
    }

    case OpRequest::kSliceRequest: {
      const SliceRequest& slice_request = request.request().slice_request();
      apply(slice_request.operand());
      break;
    }

    case OpRequest::kDynamicSliceRequest: {
      const DynamicSliceRequest& dynamic_slice_request =
          request.request().dynamic_slice_request();
      apply(dynamic_slice_request.operand());
      apply(dynamic_slice_request.start_indices());
      break;
    }

    case OpRequest::kDynamicUpdateSliceRequest: {
      const DynamicUpdateSliceRequest& dynamic_update_slice_request =
          request.request().dynamic_update_slice_request();
      apply(dynamic_update_slice_request.operand());
      apply(dynamic_update_slice_request.update());
      apply(dynamic_update_slice_request.start_indices());
      break;
    }

    case OpRequest::kConcatenateRequest: {
      const ConcatenateRequest& concatenate_request =
          request.request().concatenate_request();
      for (const ComputationDataHandle& handle :
           concatenate_request.operands()) {
        apply(handle);
      }
      break;
    }

    case OpRequest::kConvolveRequest: {
      const ConvolveRequest& convolve_request =
          request.request().convolve_request();
      apply(convolve_request.lhs());
      apply(convolve_request.rhs());
      break;
    }

    case OpRequest::kFftRequest: {
      const FftRequest& fft_request = request.request().fft_request();
      apply(fft_request.operand());
      break;
    }

    case OpRequest::kBatchNormTrainingRequest: {
      const BatchNormTrainingRequest& batch_norm_training_request =
          request.request().batch_norm_training_request();

      apply(batch_norm_training_request.operand());
      apply(batch_norm_training_request.scale());
      apply(batch_norm_training_request.offset());
      break;
    }

    case OpRequest::kBatchNormInferenceRequest: {
      const BatchNormInferenceRequest& batch_norm_inference_request =
          request.request().batch_norm_inference_request();

      apply(batch_norm_inference_request.operand());
      apply(batch_norm_inference_request.scale());
      apply(batch_norm_inference_request.offset());
      apply(batch_norm_inference_request.mean());
      apply(batch_norm_inference_request.variance());
      break;
    }

    case OpRequest::kBatchNormGradRequest: {
      const BatchNormGradRequest& batch_norm_grad_request =
          request.request().batch_norm_grad_request();

      apply(batch_norm_grad_request.operand());
      apply(batch_norm_grad_request.scale());
      apply(batch_norm_grad_request.mean());
      apply(batch_norm_grad_request.variance());
      apply(batch_norm_grad_request.grad_output());
      break;
    }

    case OpRequest::kCrossReplicaSumRequest: {
      const CrossReplicaSumRequest& cross_replica_sum_request =
          request.request().cross_replica_sum_request();
      apply(cross_replica_sum_request.operand());
      break;
    }

    case OpRequest::kInfeedRequest:
      break;

    case OpRequest::kOutfeedRequest: {
      const OutfeedRequest& outfeed_request =
          request.request().outfeed_request();
      apply(outfeed_request.operand());
      break;
    }

    case OpRequest::kMapRequest: {
      const MapRequest& map_request = request.request().map_request();
      for (const ComputationDataHandle& handle : map_request.operands()) {
        apply(handle);
      }
      break;
    }

    case OpRequest::kReduceRequest: {
      const ReduceRequest& reduce_request = request.request().reduce_request();
      apply(reduce_request.operand());
      apply(reduce_request.init_value());
      break;
    }

    case OpRequest::kReduceWindowRequest: {
      const ReduceWindowRequest& reduce_window_request =
          request.request().reduce_window_request();
      apply(reduce_window_request.operand());
      apply(reduce_window_request.init_value());
      break;
    }

    case OpRequest::kSelectAndScatterRequest: {
      const SelectAndScatterRequest& select_and_scatter_request =
          request.request().select_and_scatter_request();
      apply(select_and_scatter_request.operand());
      apply(select_and_scatter_request.source());
      apply(select_and_scatter_request.init_value());

      break;
    }

    case OpRequest::kBroadcastRequest: {
      const BroadcastRequest& broadcast_request =
          request.request().broadcast_request();
      apply(broadcast_request.operand());
      break;
    }

    case OpRequest::kReshapeRequest: {
      const ReshapeRequest& reshape_request =
          request.request().reshape_request();
      apply(reshape_request.operand());
      break;
    }

    case OpRequest::kTransposeRequest: {
      const TransposeRequest& transpose_request =
          request.request().transpose_request();
      apply(transpose_request.operand());
      break;
    }

    case OpRequest::kReverseRequest: {
      const ReverseRequest& reverse_request =
          request.request().reverse_request();
      apply(reverse_request.operand());
      break;
    }

    case OpRequest::kPadRequest: {
      const PadRequest& pad_request = request.request().pad_request();
      apply(pad_request.operand());
      apply(pad_request.padding_value());
      break;
    }

    case OpRequest::kRecvRequest:
    case OpRequest::kParameterRequest:
      break;

    case OpRequest::kConvertRequest: {
      const ConvertRequest& convert_request =
          request.request().convert_request();
      apply(convert_request.operand());
      break;
    }

    case OpRequest::kBitcastConvertRequest: {
      const ConvertRequest& convert_request =
          request.request().bitcast_convert_request();
      apply(convert_request.operand());
      break;
    }

    case OpRequest::kWhileRequest: {
      const WhileRequest& while_request = request.request().while_request();
      apply(while_request.init());
      break;
    }

    case OpRequest::kConditionalRequest: {
      const ConditionalRequest& conditional_request =
          request.request().conditional_request();
      apply(conditional_request.predicate());
      apply(conditional_request.true_operand());
      apply(conditional_request.false_operand());
      break;
    }

    case OpRequest::kTernaryOpRequest: {
      const TernaryOpRequest& ternary_op_request =
          request.request().ternary_op_request();
      apply(ternary_op_request.lhs());
      apply(ternary_op_request.rhs());
      apply(ternary_op_request.ehs());
      break;
    }

    case OpRequest::kVariadicOpRequest: {
      const VariadicOpRequest& variadic_op_request =
          request.request().variadic_op_request();
      for (const ComputationDataHandle& handle :
           variadic_op_request.operands()) {
        apply(handle);
      }
      break;
    }

    case OpRequest::kCallRequest: {
      const CallRequest& call_request = request.request().call_request();
      for (const ComputationDataHandle& handle : call_request.operands()) {
        apply(handle);
      }
      break;
    }

    case OpRequest::kCustomCallRequest: {
      const CustomCallRequest& cc_request =
          request.request().custom_call_request();
      for (const ComputationDataHandle& operand : cc_request.operands()) {
        apply(operand);
      }
      break;
    }

    case OpRequest::kHostComputeRequest: {
      const HostComputeRequest& hc_request =
          request.request().host_compute_request();
      for (const ComputationDataHandle& operand : hc_request.operands()) {
        apply(operand);
      }
      break;
    }

    case OpRequest::kDotRequest: {
      const DotRequest& dot_request = request.request().dot_request();
      apply(dot_request.rhs());
      apply(dot_request.lhs());
      break;
    }

    case OpRequest::kUnaryOpRequest: {
      const UnaryOpRequest& unary_op_request =
          request.request().unary_op_request();
      apply(unary_op_request.operand());
      break;
    }

    case OpRequest::kBinaryOpRequest: {
      const BinaryOpRequest& binary_op_request =
          request.request().binary_op_request();
      apply(binary_op_request.rhs());
      apply(binary_op_request.lhs());
      break;
    }

    case OpRequest::kReducePrecisionRequest: {
      const ReducePrecisionRequest& reduce_precision_request =
          request.request().reduce_precision_request();
      apply(reduce_precision_request.operand());
      break;
    }

    case OpRequest::kTraceRequest: {
      const TraceRequest& trace_request = request.request().trace_request();
      apply(trace_request.operand());
      break;
    }

    case OpRequest::kSendRequest: {
      const SendRequest& send_request = request.request().send_request();
      apply(send_request.operand());
      break;
    }

    case OpRequest::kGatherRequest: {
      const GatherRequest& gather_request = request.request().gather_request();
      apply(gather_request.input());
      apply(gather_request.gather_indices());
      break;
    }

    case OpRequest::OP_NOT_SET:
      LOG(FATAL) << "OperationRequest doesn't contain a request";

    default:
      LOG(FATAL) << "Unexpected request type: " << request.request().op_case();
  }
}

void ComputationLowerer::TraversePostorder(
    const ComputationDataHandle& root,
    std::unordered_map<int64, HloInstruction*>* visited,
    const std::function<void(const ComputationDataHandle&)>& visit) {
  // Stack containing {handle, enter} pairs. The 'enter' value describes whether
  // we are entering or leaving 'handle'.
  std::stack<std::pair<ComputationDataHandle, bool>> work;
  work.push({root, true});
  while (!work.empty()) {
    ComputationDataHandle handle;
    bool enter;
    std::tie(handle, enter) = work.top();
    work.pop();

    if (enter) {
      // We are entering 'handle'. The first time we enter 'handle', we add it
      // to 'visited' with a nullptr value. If 'handle' is already in 'visited',
      // we do not visit it again. This algorithm only uses the presence of
      // a handle in 'visited', but we use a map so we can use the same data
      // structure to store the HloInstruction outputs.
      if (visited->emplace(handle.handle(), nullptr).second) {
        const OperationRequest& request =
            session_computation_.requests().at(handle.handle());
        // Push the corresponding 'leave' action onto the stack, followed by
        // the operands.
        work.push({handle, false});
        ForEachOperand(request, [&work](const ComputationDataHandle& child) {
          work.push({child, true});
        });
      }
    } else {
      // We are leaving 'handle'. We have visited the operands of 'handle', and
      // now can visit the 'handle' itself.
      visit(handle);
    }
  }
}

StatusOr<std::unique_ptr<HloComputation>> ComputationLowerer::Lower() {
  // Map from ComputationDataHandle to HLO instruction. Serves as a record of
  // which operations have been visited as well as a cache for looking up
  // ComputationDataHandles as HloInstructions.
  std::unordered_map<int64, HloInstruction*> instructions;

  TF_ASSIGN_OR_RETURN(const OperationRequest* root_request,
                      GetRoot(version_, session_computation_));

  auto visit = [&](const ComputationDataHandle& handle) {
    Visit(handle, &instructions);
  };
  TraversePostorder(root_request->output_handle(), &instructions, visit);
  HloInstruction* hlo_root =
      instructions.at(root_request->output_handle().handle());

  if (include_unreachable_instructions_) {
    // Iterate through all computation data handles, and visit any unvisited
    // operations.
    for (int64 request_num = 1; request_num <= version_; ++request_num) {
      TF_ASSIGN_OR_RETURN(const OperationRequest* request,
                          LookUpRequest(request_num, session_computation_));
      TraversePostorder(request->output_handle(), &instructions, visit);
    }
  }

  return hlo_builder_.Build(hlo_root);
}

HloComputation* ComputationLowerer::ResolveComputation(
    const ComputationHandle& handle,
    VersionedComputationHandle::Version version) {
  const VersionedComputationHandle checked_handle = {handle, version};
  return hlo_resolver_(checked_handle);
}

HloInstruction* ComputationLowerer::ImplicitBroadcastToExplicitBroadcast(
    HloInstruction* operand, const Shape& output_shape) {
  auto fadd = [this](std::unique_ptr<HloInstruction> x) {
    return hlo_builder_.AddInstruction(std::move(x));
  };
  return fadd(
      HloInstruction::CreateBroadcastSequence(output_shape, operand, fadd));
}

void ComputationLowerer::Visit(
    const ComputationDataHandle& handle,
    std::unordered_map<int64, HloInstruction*>* instructions) {
  CHECK_LE(handle.handle(), version_);
  CHECK(instructions->at(handle.handle()) == nullptr);
  const OperationRequest& request =
      session_computation_.requests().at(handle.handle());
  auto add_instruction = [&](std::unique_ptr<HloInstruction> instruction) {
    HloInstruction* hlo_instruction =
        hlo_builder_.AddInstruction(std::move(instruction));
    hlo_instruction->set_metadata(request.request().metadata());
    if (request.request().has_sharding()) {
      OpSharding op_sharding = request.request().sharding();
      hlo_instruction->set_sharding(
          HloSharding::FromProto(op_sharding).ValueOrDie());
    }
    return hlo_instruction;
  };
  auto lookup_instruction = [&](const ComputationDataHandle& handle) {
    return instructions->at(handle.handle());
  };
  HloInstruction* hlo_instruction;
  switch (request.request().op_case()) {
    case OpRequest::kRngRequest: {
      const RngRequest& rng_request = request.request().rng_request();
      std::vector<HloInstruction*> parameters;
      for (const ComputationDataHandle& param : rng_request.parameter()) {
        parameters.push_back(lookup_instruction(param));
      }
      hlo_instruction = add_instruction(HloInstruction::CreateRng(
          request.output_shape(), rng_request.distribution(), parameters));
      break;
    }

    case OpRequest::kConstantRequest: {
      const ConstantRequest& constant_request =
          request.request().constant_request();
      hlo_instruction = add_instruction(HloInstruction::CreateConstant(
          Literal::CreateFromProto(constant_request.literal())
              .ConsumeValueOrDie()));
      break;
    }

    case OpRequest::kGetTupleElementRequest: {
      const GetTupleElementRequest& get_tuple_element_request =
          request.request().get_tuple_element_request();
      HloInstruction* operand =
          lookup_instruction(get_tuple_element_request.operand());
      hlo_instruction = add_instruction(HloInstruction::CreateGetTupleElement(
          request.output_shape(), operand, get_tuple_element_request.index()));
      break;
    }

    case OpRequest::kSliceRequest: {
      const SliceRequest& slice_request = request.request().slice_request();
      HloInstruction* operand = lookup_instruction(slice_request.operand());
      hlo_instruction = add_instruction(HloInstruction::CreateSlice(
          request.output_shape(), operand,
          AsInt64Slice(slice_request.start_indices()),
          AsInt64Slice(slice_request.limit_indices()),
          AsInt64Slice(slice_request.strides())));
      break;
    }

    case OpRequest::kDynamicSliceRequest: {
      const DynamicSliceRequest& dynamic_slice_request =
          request.request().dynamic_slice_request();
      HloInstruction* operand =
          lookup_instruction(dynamic_slice_request.operand());
      HloInstruction* start_indices =
          lookup_instruction(dynamic_slice_request.start_indices());

      hlo_instruction = add_instruction(HloInstruction::CreateDynamicSlice(
          request.output_shape(), operand, start_indices,
          AsInt64Slice(dynamic_slice_request.slice_sizes())));
      break;
    }

    case OpRequest::kDynamicUpdateSliceRequest: {
      const DynamicUpdateSliceRequest& dynamic_update_slice_request =
          request.request().dynamic_update_slice_request();
      HloInstruction* operand =
          lookup_instruction(dynamic_update_slice_request.operand());
      HloInstruction* update =
          lookup_instruction(dynamic_update_slice_request.update());
      HloInstruction* start_indices =
          lookup_instruction(dynamic_update_slice_request.start_indices());
      hlo_instruction =
          add_instruction(HloInstruction::CreateDynamicUpdateSlice(
              request.output_shape(), operand, update, start_indices));
      break;
    }

    case OpRequest::kConcatenateRequest: {
      const ConcatenateRequest& concatenate_request =
          request.request().concatenate_request();
      std::vector<HloInstruction*> operands;
      for (const ComputationDataHandle& handle :
           concatenate_request.operands()) {
        HloInstruction* operand = lookup_instruction(handle);
        operands.push_back(operand);
      }
      hlo_instruction = add_instruction(HloInstruction::CreateConcatenate(
          request.output_shape(), operands, concatenate_request.dimension()));
      break;
    }

    case OpRequest::kConvolveRequest: {
      const ConvolveRequest& convolve_request =
          request.request().convolve_request();
      HloInstruction* lhs = lookup_instruction(convolve_request.lhs());
      HloInstruction* rhs = lookup_instruction(convolve_request.rhs());
      hlo_instruction = add_instruction(HloInstruction::CreateConvolve(
          request.output_shape(), lhs, rhs, convolve_request.window(),
          convolve_request.dimension_numbers()));
      break;
    }

    case OpRequest::kFftRequest: {
      const FftRequest& fft_request = request.request().fft_request();
      HloInstruction* operand = lookup_instruction(fft_request.operand());
      hlo_instruction = add_instruction(HloInstruction::CreateFft(
          request.output_shape(), operand, fft_request.fft_type(),
          AsInt64Slice(fft_request.fft_length())));
      break;
    }

    case OpRequest::kDotRequest: {
      const DotRequest& dot_request = request.request().dot_request();
      HloInstruction* lhs = lookup_instruction(dot_request.lhs());
      HloInstruction* rhs = lookup_instruction(dot_request.rhs());
      hlo_instruction = add_instruction(HloInstruction::CreateDot(
          request.output_shape(), lhs, rhs, dot_request.dimension_numbers()));
      break;
    }

    case OpRequest::kCrossReplicaSumRequest: {
      const CrossReplicaSumRequest& cross_replica_sum_request =
          request.request().cross_replica_sum_request();
      HloInstruction* operand =
          lookup_instruction(cross_replica_sum_request.operand());
      hlo_instruction = add_instruction(HloInstruction::CreateCrossReplicaSum(
          request.output_shape(), {operand}));
      break;
    }

    case OpRequest::kInfeedRequest: {
      const InfeedRequest& infeed_request = request.request().infeed_request();
      hlo_instruction = add_instruction(HloInstruction::CreateInfeed(
          request.output_shape(), infeed_request.config()));
      break;
    }

    case OpRequest::kOutfeedRequest: {
      const OutfeedRequest& outfeed_request =
          request.request().outfeed_request();
      HloInstruction* operand = lookup_instruction(outfeed_request.operand());
      hlo_instruction = add_instruction(HloInstruction::CreateOutfeed(
          outfeed_request.shape(), operand, outfeed_request.outfeed_config()));
      break;
    }

    case OpRequest::kMapRequest: {
      const MapRequest& map_request = request.request().map_request();
      std::vector<HloInstruction*> operands;
      for (const ComputationDataHandle& handle : map_request.operands()) {
        HloInstruction* operand = lookup_instruction(handle);
        operands.push_back(operand);
      }
      CHECK_EQ(1, request.embedded_computation_versions_size());
      VersionedComputationHandle::Version map_version =
          request.embedded_computation_versions(0);
      HloComputation* map_computation =
          ResolveComputation(map_request.to_apply(), map_version);
      hlo_instruction = add_instruction(HloInstruction::CreateMap(
          request.output_shape(), operands, map_computation));
      break;
    }

    case OpRequest::kReduceRequest: {
      const ReduceRequest& reduce_request = request.request().reduce_request();
      HloInstruction* operand = lookup_instruction(reduce_request.operand());
      HloInstruction* init_value =
          lookup_instruction(reduce_request.init_value());
      CHECK_EQ(1, request.embedded_computation_versions_size());
      VersionedComputationHandle::Version reduce_version =
          request.embedded_computation_versions(0);
      HloComputation* reduce_computation =
          ResolveComputation(reduce_request.to_apply(), reduce_version);
      hlo_instruction = add_instruction(HloInstruction::CreateReduce(
          request.output_shape(), operand, init_value,
          AsInt64Slice(reduce_request.dimensions()), reduce_computation));
      break;
    }

    case OpRequest::kReduceWindowRequest: {
      const ReduceWindowRequest& reduce_window_request =
          request.request().reduce_window_request();
      HloInstruction* operand =
          lookup_instruction(reduce_window_request.operand());
      HloInstruction* init_value =
          lookup_instruction(reduce_window_request.init_value());
      CHECK_EQ(1, request.embedded_computation_versions_size());
      VersionedComputationHandle::Version reduce_window_version =
          request.embedded_computation_versions(0);
      HloComputation* reduce_window_computation = ResolveComputation(
          reduce_window_request.to_apply(), reduce_window_version);
      hlo_instruction = add_instruction(HloInstruction::CreateReduceWindow(
          request.output_shape(), operand, init_value,
          reduce_window_request.window(), reduce_window_computation));
      break;
    }

    case OpRequest::kSelectAndScatterRequest: {
      const SelectAndScatterRequest& select_and_scatter_request =
          request.request().select_and_scatter_request();
      HloInstruction* operand =
          lookup_instruction(select_and_scatter_request.operand());
      HloInstruction* source =
          lookup_instruction(select_and_scatter_request.source());
      HloInstruction* init_value =
          lookup_instruction(select_and_scatter_request.init_value());
      CHECK_EQ(2, request.embedded_computation_versions_size());
      VersionedComputationHandle::Version select_version =
          request.embedded_computation_versions(0);
      VersionedComputationHandle::Version scatter_version =
          request.embedded_computation_versions(1);
      HloComputation* select_computation = ResolveComputation(
          select_and_scatter_request.select(), select_version);
      HloComputation* scatter_computation = ResolveComputation(
          select_and_scatter_request.scatter(), scatter_version);
      hlo_instruction = add_instruction(HloInstruction::CreateSelectAndScatter(
          request.output_shape(), operand, select_computation,
          select_and_scatter_request.window(), source, init_value,
          scatter_computation));
      break;
    }

    case OpRequest::kBatchNormTrainingRequest: {
      const BatchNormTrainingRequest& batch_norm_training_request =
          request.request().batch_norm_training_request();
      HloInstruction* operand =
          lookup_instruction(batch_norm_training_request.operand());
      HloInstruction* scale =
          lookup_instruction(batch_norm_training_request.scale());
      HloInstruction* offset =
          lookup_instruction(batch_norm_training_request.offset());

      hlo_instruction = add_instruction(HloInstruction::CreateBatchNormTraining(
          request.output_shape(), operand, scale, offset,
          batch_norm_training_request.epsilon(),
          batch_norm_training_request.feature_index()));
      break;
    }

    case OpRequest::kBatchNormInferenceRequest: {
      const BatchNormInferenceRequest& batch_norm_inference_request =
          request.request().batch_norm_inference_request();
      HloInstruction* operand =
          lookup_instruction(batch_norm_inference_request.operand());
      HloInstruction* scale =
          lookup_instruction(batch_norm_inference_request.scale());
      HloInstruction* offset =
          lookup_instruction(batch_norm_inference_request.offset());
      HloInstruction* mean =
          lookup_instruction(batch_norm_inference_request.mean());
      HloInstruction* variance =
          lookup_instruction(batch_norm_inference_request.variance());

      hlo_instruction =
          add_instruction(HloInstruction::CreateBatchNormInference(
              request.output_shape(), operand, scale, offset, mean, variance,
              batch_norm_inference_request.epsilon(),
              batch_norm_inference_request.feature_index()));
      break;
    }

    case OpRequest::kBatchNormGradRequest: {
      const BatchNormGradRequest& batch_norm_grad_request =
          request.request().batch_norm_grad_request();

      HloInstruction* operand =
          lookup_instruction(batch_norm_grad_request.operand());
      HloInstruction* scale =
          lookup_instruction(batch_norm_grad_request.scale());
      HloInstruction* mean = lookup_instruction(batch_norm_grad_request.mean());
      HloInstruction* variance =
          lookup_instruction(batch_norm_grad_request.variance());
      HloInstruction* grad_output =
          lookup_instruction(batch_norm_grad_request.grad_output());

      hlo_instruction = add_instruction(HloInstruction::CreateBatchNormGrad(
          request.output_shape(), operand, scale, mean, variance, grad_output,
          batch_norm_grad_request.epsilon(),
          batch_norm_grad_request.feature_index()));
      break;
    }

    case OpRequest::kBroadcastRequest: {
      const BroadcastRequest& broadcast_request =
          request.request().broadcast_request();
      HloInstruction* operand = lookup_instruction(broadcast_request.operand());
      std::vector<int64> broadcast_dimensions;
      // The client-level broadcast instruction just appends dimensions on the
      // left (adds lowest numbered dimensions). The HLO broadcast op is more
      // flexible and can add new dimensions anywhere. The broadcast_dimensions
      // maps operand dimensions to dimensions in the broadcast output, so
      // to append dimensions on the left the broadcast_dimensions should just
      // be the n highest dimension numbers of the output shape where n is
      // the number of input dimensions.
      broadcast_dimensions.reserve(ShapeUtil::Rank(operand->shape()));
      for (int i = 0; i < ShapeUtil::Rank(operand->shape()); ++i) {
        broadcast_dimensions.push_back(i +
                                       ShapeUtil::Rank(request.output_shape()) -
                                       ShapeUtil::Rank(operand->shape()));
      }
      hlo_instruction = add_instruction(HloInstruction::CreateBroadcast(
          request.output_shape(), operand, broadcast_dimensions));
      break;
    }

    case OpRequest::kReshapeRequest: {
      const ReshapeRequest& reshape_request =
          request.request().reshape_request();
      HloInstruction* operand = lookup_instruction(reshape_request.operand());
      HloInstruction* transposed;
      if (IsIdentityPermutation(AsInt64Slice(reshape_request.dimensions()))) {
        transposed = operand;
      } else {
        transposed = add_instruction(HloInstruction::CreateTranspose(
            ShapeUtil::PermuteDimensions(
                InversePermutation(AsInt64Slice(reshape_request.dimensions())),
                operand->shape()),
            operand, AsInt64Slice(reshape_request.dimensions())));
      }
      hlo_instruction = add_instruction(
          HloInstruction::CreateReshape(request.output_shape(), transposed));
      break;
    }

    case OpRequest::kTransposeRequest: {
      const TransposeRequest& transpose_request =
          request.request().transpose_request();
      HloInstruction* operand = lookup_instruction(transpose_request.operand());
      hlo_instruction = add_instruction(HloInstruction::CreateTranspose(
          ShapeUtil::PermuteDimensions(
              InversePermutation(AsInt64Slice(transpose_request.dimensions())),
              operand->shape()),
          operand, AsInt64Slice(transpose_request.dimensions())));
      break;
    }

    case OpRequest::kReverseRequest: {
      const ReverseRequest& reverse_request =
          request.request().reverse_request();
      HloInstruction* operand = lookup_instruction(reverse_request.operand());
      hlo_instruction = add_instruction(HloInstruction::CreateReverse(
          request.output_shape(), operand,
          AsInt64Slice(reverse_request.dimensions())));
      break;
    }

    case OpRequest::kPadRequest: {
      const PadRequest& pad_request = request.request().pad_request();
      HloInstruction* operand = lookup_instruction(pad_request.operand());
      HloInstruction* padding_value =
          lookup_instruction(pad_request.padding_value());
      hlo_instruction = add_instruction(HloInstruction::CreatePad(
          request.output_shape(), operand, padding_value,
          pad_request.padding_config()));
      break;
    }

    case OpRequest::kRecvRequest: {
      const RecvRequest& recv_request = request.request().recv_request();
      HloInstruction* recv = add_instruction(HloInstruction::CreateRecv(
          request.output_shape(), recv_request.channel_handle().handle()));
      hlo_instruction = add_instruction(HloInstruction::CreateRecvDone(recv));
      break;
    }

    case OpRequest::kParameterRequest: {
      const ParameterRequest& parameter_request =
          request.request().parameter_request();
      hlo_instruction = add_instruction(HloInstruction::CreateParameter(
          parameter_request.parameter(), request.output_shape(),
          parameter_request.name()));
      break;
    }

    case OpRequest::kConvertRequest: {
      const ConvertRequest& convert_request =
          request.request().convert_request();
      HloInstruction* operand = lookup_instruction(convert_request.operand());
      hlo_instruction = add_instruction(
          HloInstruction::CreateConvert(request.output_shape(), operand));
      break;
    }

    case OpRequest::kBitcastConvertRequest: {
      const ConvertRequest& convert_request =
          request.request().bitcast_convert_request();
      HloInstruction* operand = lookup_instruction(convert_request.operand());
      hlo_instruction = add_instruction(HloInstruction::CreateBitcastConvert(
          request.output_shape(), operand));
      break;
    }

    case OpRequest::kWhileRequest: {
      const WhileRequest& while_request = request.request().while_request();
      CHECK_EQ(2, request.embedded_computation_versions_size());
      VersionedComputationHandle::Version condition_version =
          request.embedded_computation_versions(0);
      HloComputation* condition =
          ResolveComputation(while_request.condition(), condition_version);
      VersionedComputationHandle::Version body_version =
          request.embedded_computation_versions(1);
      HloComputation* body =
          ResolveComputation(while_request.body(), body_version);
      HloInstruction* init = lookup_instruction(while_request.init());
      hlo_instruction = add_instruction(HloInstruction::CreateWhile(
          request.output_shape(), condition, body, init));
      break;
    }

    case OpRequest::kConditionalRequest: {
      const ConditionalRequest& conditional_request =
          request.request().conditional_request();
      CHECK_EQ(2, request.embedded_computation_versions_size());
      VersionedComputationHandle::Version true_computation_version =
          request.embedded_computation_versions(0);
      HloComputation* true_computation = ResolveComputation(
          conditional_request.true_computation(), true_computation_version);
      VersionedComputationHandle::Version false_computation_version =
          request.embedded_computation_versions(1);
      HloComputation* false_computation = ResolveComputation(
          conditional_request.false_computation(), false_computation_version);
      HloInstruction* predicate =
          lookup_instruction(conditional_request.predicate());
      HloInstruction* true_operand =
          lookup_instruction(conditional_request.true_operand());
      HloInstruction* false_operand =
          lookup_instruction(conditional_request.false_operand());
      hlo_instruction = add_instruction(HloInstruction::CreateConditional(
          request.output_shape(), predicate, true_operand, true_computation,
          false_operand, false_computation));
      break;
    }

    case OpRequest::kTernaryOpRequest: {
      const TernaryOpRequest& ternary_op_request =
          request.request().ternary_op_request();
      HloInstruction* lhs = lookup_instruction(ternary_op_request.lhs());
      HloInstruction* rhs = lookup_instruction(ternary_op_request.rhs());
      HloInstruction* ehs = lookup_instruction(ternary_op_request.ehs());
      auto hlo_opcode = TernaryOperationToHloOpcode(ternary_op_request.triop());

      if (debug_options_.xla_eliminate_hlo_implicit_broadcast()) {
        if (!ShapeUtil::SameDimensions(request.output_shape(), lhs->shape())) {
          // lhs side is being implicitly broadcast. Change to explicit.
          lhs =
              ImplicitBroadcastToExplicitBroadcast(lhs, request.output_shape());
        }

        if (!ShapeUtil::SameDimensions(request.output_shape(), rhs->shape())) {
          rhs =
              ImplicitBroadcastToExplicitBroadcast(rhs, request.output_shape());
        }

        if (!ShapeUtil::SameDimensions(request.output_shape(), ehs->shape())) {
          ehs =
              ImplicitBroadcastToExplicitBroadcast(ehs, request.output_shape());
        }
      }

      hlo_instruction = add_instruction(HloInstruction::CreateTernary(
          request.output_shape(), hlo_opcode, lhs, rhs, ehs));
      break;
    }

    case OpRequest::kVariadicOpRequest: {
      const VariadicOpRequest& variadic_op_request =
          request.request().variadic_op_request();
      std::vector<HloInstruction*> operands;
      for (const ComputationDataHandle& handle :
           variadic_op_request.operands()) {
        HloInstruction* operand = lookup_instruction(handle);
        operands.push_back(operand);
      }
      auto hlo_opcode =
          VariadicOperationToHloOpcode(variadic_op_request.varop());
      hlo_instruction = add_instruction(HloInstruction::CreateVariadic(
          request.output_shape(), hlo_opcode, operands));
      break;
    }

    case OpRequest::kCallRequest: {
      const CallRequest& call_request = request.request().call_request();
      std::vector<HloInstruction*> operands;
      for (const ComputationDataHandle& handle : call_request.operands()) {
        operands.push_back(lookup_instruction(handle));
      }
      CHECK_EQ(1, request.embedded_computation_versions_size());
      VersionedComputationHandle::Version call_version =
          request.embedded_computation_versions(0);
      HloComputation* call_computation =
          ResolveComputation(call_request.to_apply(), call_version);
      hlo_instruction = add_instruction(HloInstruction::CreateCall(
          request.output_shape(), operands, call_computation));
      break;
    }

    case OpRequest::kCustomCallRequest: {
      const CustomCallRequest& cc_request =
          request.request().custom_call_request();
      std::vector<HloInstruction*> operands;
      for (const ComputationDataHandle& operand : cc_request.operands()) {
        operands.push_back(lookup_instruction(operand));
      }
      hlo_instruction = add_instruction(HloInstruction::CreateCustomCall(
          cc_request.shape(), operands, cc_request.call_target_name()));
      break;
    }

    case OpRequest::kHostComputeRequest: {
      const HostComputeRequest& host_compute_request =
          request.request().host_compute_request();
      std::vector<HloInstruction*> operands;
      for (const ComputationDataHandle& operand :
           host_compute_request.operands()) {
        operands.push_back(lookup_instruction(operand));
      }
      auto output_shape = host_compute_request.shape();
      auto channel_name = host_compute_request.channel_name();
      auto cost_estimate_ns = host_compute_request.cost_estimate_ns();
      hlo_instruction = add_instruction(HloInstruction::CreateHostCompute(
          output_shape, operands, channel_name, cost_estimate_ns));
      break;
    }

    case OpRequest::kUnaryOpRequest: {
      const UnaryOpRequest& unary_op_request =
          request.request().unary_op_request();
      HloInstruction* operand = lookup_instruction(unary_op_request.operand());
      auto hlo_opcode = UnaryOperationToHloOpcode(unary_op_request.unop());
      hlo_instruction = add_instruction(HloInstruction::CreateUnary(
          request.output_shape(), hlo_opcode, operand));
      break;
    }

    case OpRequest::kBinaryOpRequest: {
      const BinaryOpRequest& binary_op_request =
          request.request().binary_op_request();
      HloInstruction* lhs = lookup_instruction(binary_op_request.lhs());
      HloInstruction* rhs = lookup_instruction(binary_op_request.rhs());
      auto hlo_opcode = BinaryOperationToHloOpcode(binary_op_request.binop());
      if (binary_op_request.broadcast_dimensions_size() > 0 &&
          ShapeUtil::Rank(lhs->shape()) != ShapeUtil::Rank(rhs->shape())) {
        // Emit a broadcast instruction to perform the "broadcast in dimension"
        // operation.
        HloInstruction* operand_to_broadcast =
            ShapeUtil::Rank(lhs->shape()) < ShapeUtil::Rank(rhs->shape()) ? lhs
                                                                          : rhs;
        CHECK_EQ(ShapeUtil::Rank(operand_to_broadcast->shape()),
                 binary_op_request.broadcast_dimensions().size());

        // Construct the bounds of the shape of the kBroadcast instruction
        // responsible for the in-dimension broadcast.
        std::vector<int64> output_dimensions;
        for (int64 size : request.output_shape().dimensions()) {
          output_dimensions.push_back(size);
        }
        for (int64 operand_dim = 0;
             operand_dim < ShapeUtil::Rank(operand_to_broadcast->shape());
             ++operand_dim) {
          int64 output_dim =
              binary_op_request.broadcast_dimensions()[operand_dim];
          output_dimensions[output_dim] =
              operand_to_broadcast->shape().dimensions(operand_dim);
        }

        Shape broadcast_shape = ShapeUtil::MakeShape(
            operand_to_broadcast->shape().element_type(), output_dimensions);

        // The broadcast semantics of a client-level binary op broadcast is
        // identical to the HLO broadcast semantics so the broadcast_dimensions
        // field can just be passed to the instruction builder.
        HloInstruction* broadcasted_operand =
            add_instruction(HloInstruction::CreateBroadcast(
                broadcast_shape, operand_to_broadcast,
                AsInt64Slice(binary_op_request.broadcast_dimensions())));

        lhs = (lhs == operand_to_broadcast) ? broadcasted_operand : lhs;
        rhs = (rhs == operand_to_broadcast) ? broadcasted_operand : rhs;
      }
      if (debug_options_.xla_eliminate_hlo_implicit_broadcast()) {
        if (!ShapeUtil::SameDimensions(request.output_shape(), lhs->shape())) {
          // lhs side is being implicitly broadcast. Change to explicit.
          lhs =
              ImplicitBroadcastToExplicitBroadcast(lhs, request.output_shape());
        }

        if (!ShapeUtil::SameDimensions(request.output_shape(), rhs->shape())) {
          rhs =
              ImplicitBroadcastToExplicitBroadcast(rhs, request.output_shape());
        }
      }
      hlo_instruction = add_instruction(HloInstruction::CreateBinary(
          request.output_shape(), hlo_opcode, lhs, rhs));
      break;
    }

    case OpRequest::kReducePrecisionRequest: {
      const ReducePrecisionRequest& reduce_precision_request =
          request.request().reduce_precision_request();
      HloInstruction* operand =
          lookup_instruction(reduce_precision_request.operand());
      auto exponent_bits = reduce_precision_request.exponent_bits();
      auto mantissa_bits = reduce_precision_request.mantissa_bits();
      hlo_instruction = add_instruction(HloInstruction::CreateReducePrecision(
          request.output_shape(), operand, exponent_bits, mantissa_bits));
      break;
    }

    case OpRequest::kTraceRequest: {
      const TraceRequest& trace_request = request.request().trace_request();
      HloInstruction* operand = lookup_instruction(trace_request.operand());
      hlo_instruction = add_instruction(
          HloInstruction::CreateTrace(trace_request.tag(), operand));
      operand->set_tracing(hlo_instruction);
      break;
    }

    case OpRequest::kSendRequest: {
      const SendRequest& send_request = request.request().send_request();
      HloInstruction* operand = lookup_instruction(send_request.operand());
      HloInstruction* send = add_instruction(HloInstruction::CreateSend(
          operand, send_request.channel_handle().handle()));
      hlo_instruction = add_instruction(HloInstruction::CreateSendDone(send));
      break;
    }

    case OpRequest::kGatherRequest: {
      const GatherRequest& gather_request = request.request().gather_request();
      HloInstruction* input_operand =
          lookup_instruction(gather_request.input());
      HloInstruction* gather_indices_operand =
          lookup_instruction(gather_request.gather_indices());
      std::vector<int64> window_bounds;
      c_copy(gather_request.window_bounds(), std::back_inserter(window_bounds));
      hlo_instruction = add_instruction(HloInstruction::CreateGather(
          request.output_shape(), input_operand, gather_indices_operand,
          gather_request.dimension_numbers(), window_bounds));
      break;
    }

    case OpRequest::OP_NOT_SET:
      LOG(FATAL) << "OperationRequest doesn't contain a request";

    default:
      LOG(FATAL) << "Unexpected request type: " << request.request().op_case();
  }
  (*instructions)[handle.handle()] = hlo_instruction;
}  // NOLINT(readability/fn_size)

}  // namespace

StatusOr<std::unique_ptr<HloComputation>> UserComputation::BuildHloComputation(
    VersionedComputationHandle::Version version,
    HloComputationResolver hlo_resolver, const DebugOptions& debug_options,
    bool include_unreachable_instructions) const {
  tensorflow::mutex_lock lock(mutex_);

  VLOG(2) << "Building HloComputation from UserComputation " << name_
          << " at version " << version;
  XLA_VLOG_LINES(3, session_computation_.DebugString());

  TF_ASSIGN_OR_RETURN(
      std::unique_ptr<HloComputation> hlo_computation,
      ComputationLowerer::Lower(
          tensorflow::strings::StrCat(name(), ".v", version),
          session_computation_, version, std::move(hlo_resolver), debug_options,
          include_unreachable_instructions));

  return std::move(hlo_computation);
}

}  // namespace xla
