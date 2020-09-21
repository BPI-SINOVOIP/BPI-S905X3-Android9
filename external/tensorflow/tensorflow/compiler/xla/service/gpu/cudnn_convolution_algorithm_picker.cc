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

#include "tensorflow/compiler/xla/service/gpu/cudnn_convolution_algorithm_picker.h"
#include "tensorflow/compiler/xla/service/gpu/convolution_thunk.h"
#include "tensorflow/compiler/xla/service/gpu/ir_emission_utils.h"
#include "tensorflow/core/lib/gtl/optional.h"
#include "tensorflow/core/lib/strings/numbers.h"
#include "tensorflow/core/lib/strings/strcat.h"

namespace xla {
namespace gpu {
namespace {

namespace se = perftools::gputools;

using se::DeviceMemoryBase;
using se::dnn::AlgorithmConfig;
using se::dnn::AlgorithmDesc;
using tensorflow::gtl::nullopt;
using tensorflow::gtl::optional;

class ScratchAllocator : public se::ScratchAllocator {
 public:
  ScratchAllocator(int device_ordinal, DeviceMemoryAllocator* memory_allocator)
      : device_ordinal_(device_ordinal), memory_allocator_(memory_allocator) {}

  ~ScratchAllocator() override;

  int64 GetMemoryLimitInBytes(se::Stream* stream) override {
    return 1LL << 32;  // 4GB.  TODO(jlebar): Tune this?
  }
  int64 TotalAllocatedBytes() { return total_allocated_bytes_; }

  se::port::StatusOr<se::DeviceMemory<uint8>> AllocateBytes(
      se::Stream* stream, int64 byte_size) override;

 private:
  const int device_ordinal_;
  DeviceMemoryAllocator* memory_allocator_;
  std::vector<se::DeviceMemoryBase> allocated_buffers_;
  int64 total_allocated_bytes_ = 0;
};

ScratchAllocator::~ScratchAllocator() {
  for (auto& allocated_buffer : allocated_buffers_) {
    if (!memory_allocator_->Deallocate(device_ordinal_, &allocated_buffer)
             .ok()) {
      // The program can still continue with failed deallocation.
      LOG(ERROR) << "Failed to deallocate the allocated buffer: "
                 << allocated_buffer.opaque();
    }
  }
}

se::port::StatusOr<se::DeviceMemory<uint8>> ScratchAllocator::AllocateBytes(
    se::Stream* stream, int64 byte_size) {
  CHECK_GE(byte_size, 0) << "byte_size must be positive.";
  if (byte_size > GetMemoryLimitInBytes(stream)) {
    return se::port::Status(
        se::port::error::RESOURCE_EXHAUSTED,
        tensorflow::strings::Printf(
            "Allocating %lld bytes exceeds the memory limit of %lld bytes.",
            byte_size, GetMemoryLimitInBytes(stream)));
  }

  auto status_or_memory =
      memory_allocator_->Allocate(device_ordinal_, byte_size,
                                  /*retry_on_failure=*/false);
  if (!status_or_memory.ok()) {
    return se::port::Status(se::port::error::RESOURCE_EXHAUSTED,
                            tensorflow::strings::Printf(
                                "Failed to allocate %lld bytes on device %d.",
                                byte_size, device_ordinal_));
  }
  se::DeviceMemoryBase allocated_buffer = status_or_memory.ValueOrDie();
  allocated_buffers_.push_back(allocated_buffer);
  total_allocated_bytes_ += byte_size;
  return se::DeviceMemory<uint8>(allocated_buffer);
}

// Determines whether we can safely perform a winograd non-fused convolution for
// the given input and output shapes.  This works around b/68264959, an integer
// overflow in cuDNNv5 and cuDNNv6.
//
// TODO(jlebar): We shouldn't need this check for cuDNNv7.
bool ShouldIncludeWinogradNonfusedAlgo(
    const Shape& input_shape, const Shape& output_shape,
    const ConvolutionDimensionNumbers& dnums) {
  int64 batch = input_shape.dimensions(dnums.input_batch_dimension());
  int64 in_depths = input_shape.dimensions(dnums.input_feature_dimension());
  int64 in_rows = input_shape.dimensions(dnums.input_spatial_dimensions(0));
  int64 in_cols =
      dnums.input_spatial_dimensions_size() == 1
          ? 1
          : input_shape.dimensions(dnums.input_spatial_dimensions(1));
  int64 out_depths = output_shape.dimensions(dnums.output_feature_dimension());

  int64 total_size = CeilOfRatio(batch, int64{16}) *
                     std::max(in_depths, out_depths) * in_cols * in_rows *
                     sizeof(float);

  const int64 threshold = 1L << 31;
  return total_size < threshold;
}

std::vector<AlgorithmDesc> GetAlgorithms(CudnnConvKind kind,
                                         bool with_winograd_nonfused,
                                         se::StreamExecutor* stream_exec_) {
  std::vector<AlgorithmDesc> algorithms;
  switch (kind) {
    case CudnnConvKind::kBackwardFilter:
      CHECK(stream_exec_->GetConvolveBackwardFilterAlgorithms(
          with_winograd_nonfused, &algorithms));
      break;
    case CudnnConvKind::kBackwardInput:
      CHECK(stream_exec_->GetConvolveBackwardDataAlgorithms(
          with_winograd_nonfused, &algorithms));
      break;
    case CudnnConvKind::kForward:
      CHECK(stream_exec_->GetConvolveAlgorithms(with_winograd_nonfused,
                                                &algorithms));
      break;
  }

  return algorithms;
}

string AlgorithmToString(const AlgorithmDesc& algo) {
  if (algo.tensor_ops_enabled()) {
    return tensorflow::strings::StrCat(algo.algo_id(), "+TC");
  }
  return tensorflow::strings::StrCat(algo.algo_id());
}

string NumBytesToString(int64 bytes) {
  return tensorflow::strings::StrCat(
      tensorflow::strings::HumanReadableNumBytes(bytes), " (", bytes, "B)");
}

}  // anonymous namespace

// We could have caching here so that we don't redo this work for two identical
// convolutions.  Unfortunately our cache key would have to be a tuple
// containing the protos passed to this function, and we have no utility for
// hashing protos.  We could write our own hash functions, but they'd silently
// break if we ever added a field to one of the protos.  Perhaps we could hack
// using the binary-encoded proto as the hash key, on the assumption that two
// protos being binary-equal is a sufficient, if not necessary, condition for
// proper equality.  But that would still leave us open to having unnecessary
// cache misses and doing extra work.  Overall, caching doesn't seem worth the
// trouble, but we may want to revisit this if we ever find a model where
// caching would speed up compilation a lot.
optional<std::tuple<int64, bool, int64>>
CudnnConvolutionAlgorithmPicker::PickBestAlgorithm(
    CudnnConvKind kind, const Shape& input_shape, const Shape& filter_shape,
    const Shape& output_shape, const Window& window,
    const ConvolutionDimensionNumbers& dnums, HloInstruction* instr) {
  // Create a stream for us to do our work on.
  se::Stream stream{stream_exec_};
  stream.Init();
  const auto device_ordinal = stream_exec_->device_ordinal();

  // allocator either points to this->allocator_ or, if that's null, to a
  // StreamExecutorMemoryAllocator for stream_exec_.
  DeviceMemoryAllocator* allocator;
  optional<StreamExecutorMemoryAllocator> se_allocator;
  if (allocator_ != nullptr) {
    allocator = allocator_;
  } else {
    se_allocator.emplace(
        stream_exec_->platform(),
        tensorflow::gtl::ArraySlice<se::StreamExecutor*>({stream_exec_}));
    allocator = &*se_allocator;
  }

  // Allocate space for the input, filter, and output of the convolution.  We
  // use a ScratchAllocator for this instead of calling allocator_ directly so
  // that our allocations don't leak.
  //
  // We don't put any data in these buffers, because (in theory, anyway) the
  // speed of a conv isn't affected by the data being convolved.
  ScratchAllocator input_output_allocator(device_ordinal, allocator);
  se::port::StatusOr<DeviceMemoryBase> input_buf =
      input_output_allocator.AllocateBytes(&stream,
                                           ShapeUtil::ByteSizeOf(input_shape));
  se::port::StatusOr<DeviceMemoryBase> filter_buf =
      input_output_allocator.AllocateBytes(&stream,
                                           ShapeUtil::ByteSizeOf(filter_shape));
  se::port::StatusOr<DeviceMemoryBase> output_buf =
      input_output_allocator.AllocateBytes(&stream,
                                           ShapeUtil::ByteSizeOf(output_shape));
  if (!input_buf.ok() || !filter_buf.ok() || !output_buf.ok()) {
    LOG(WARNING)
        << "Couldn't allocate space for input/filter/output of convolution "
        << instr->ToString() << ".  Falling back to default algorithm.";
    return nullopt;
  }

  const bool use_winograd_nonfused =
      ShouldIncludeWinogradNonfusedAlgo(input_shape, output_shape, dnums);
  se::dnn::ProfileResult best_result;
  int64 best_result_bytes_used = 0;

  for (const AlgorithmDesc& alg :
       GetAlgorithms(kind, use_winograd_nonfused, stream_exec_)) {
    ScratchAllocator scratch_allocator(device_ordinal, allocator);
    se::dnn::ProfileResult profile_result;
    VLOG(3) << "Trying algorithm " << AlgorithmToString(alg) << " for "
            << instr->ToString();

    bool launch_ok = RunCudnnConvolution(
                         kind, input_shape, filter_shape, output_shape,
                         input_buf.ValueOrDie(), filter_buf.ValueOrDie(),
                         output_buf.ValueOrDie(), &scratch_allocator, window,
                         dnums, AlgorithmConfig(alg), &stream, &profile_result)
                         .ok();

    if (launch_ok && profile_result.is_valid()) {
      int64 scratch_bytes_used = scratch_allocator.TotalAllocatedBytes();
      VLOG(3) << "Run of algorithm " << AlgorithmToString(alg)
              << " succeeded, taking " << profile_result.elapsed_time_in_ms()
              << "ms and using " << NumBytesToString(scratch_bytes_used)
              << " of scratch (Best result: "
              << best_result.elapsed_time_in_ms() << "ms, "
              << NumBytesToString(best_result_bytes_used) << " of scratch)";
      if (profile_result.elapsed_time_in_ms() <
          best_result.elapsed_time_in_ms()) {
        best_result = profile_result;
        best_result_bytes_used = scratch_bytes_used;
      }
    } else {
      VLOG(3) << "Run of algorithm " << AlgorithmToString(alg) << " failed.";
    }
  }
  if (best_result.is_valid()) {
    VLOG(2) << "Best algorithm for " << instr->ToString() << ": "
            << AlgorithmToString(best_result.algorithm()) << ", takes "
            << best_result.elapsed_time_in_ms() << "ms, and uses "
            << best_result_bytes_used << "B of scratch memory.";
    return std::make_tuple(best_result.algorithm().algo_id(),
                           best_result.algorithm().tensor_ops_enabled(),
                           best_result_bytes_used);
  }

  LOG(WARNING) << "All algorithms tried for convolution " << instr->ToString()
               << " failed.  Falling back to default algorithm.";
  return nullopt;
}

StatusOr<bool> CudnnConvolutionAlgorithmPicker::RunOnInstruction(
    HloInstruction* instr) {
  CHECK(IsCustomCallToDnnConvolution(*instr));

  const auto& call_target = instr->custom_call_target();
  const auto& lhs_shape = instr->operand(0)->shape();
  const auto& rhs_shape = instr->operand(1)->shape();
  const auto& conv_result_shape = instr->shape().tuple_shapes(0);
  optional<std::tuple<int64, bool, int64>> alg_scratch_and_tc;
  if (call_target == kCudnnConvForwardCallTarget) {
    alg_scratch_and_tc = PickBestAlgorithm(
        CudnnConvKind::kForward, /*input_shape=*/lhs_shape,
        /*filter_shape=*/rhs_shape, /*output_shape=*/conv_result_shape,
        instr->window(), instr->convolution_dimension_numbers(), instr);
  } else if (call_target == kCudnnConvBackwardInputCallTarget) {
    alg_scratch_and_tc = PickBestAlgorithm(
        CudnnConvKind::kBackwardInput, /*input_shape=*/conv_result_shape,
        /*filter_shape=*/rhs_shape, /*output_shape=*/lhs_shape, instr->window(),
        instr->convolution_dimension_numbers(), instr);
  } else if (call_target == kCudnnConvBackwardFilterCallTarget) {
    alg_scratch_and_tc = PickBestAlgorithm(
        CudnnConvKind::kBackwardFilter, /*input_shape=*/lhs_shape,
        /*filter_shape=*/conv_result_shape, /*output_shape=*/rhs_shape,
        instr->window(), instr->convolution_dimension_numbers(), instr);
  } else {
    LOG(FATAL) << "Unknown custom call target for cudnn conv: "
               << instr->ToString();
  }

  if (!alg_scratch_and_tc.has_value()) {
    return false;
  }

  int64 algorithm;
  bool tensor_ops_enabled;
  int64 scratch_bytes;

  std::tie(algorithm, tensor_ops_enabled, scratch_bytes) = *alg_scratch_and_tc;

  VLOG(1) << "Setting cudnn conv to use algorithm " << algorithm << " and "
          << NumBytesToString(scratch_bytes)
          << " of scratch memory: " << instr->ToString()
          << " tensor_ops_enabled: " << tensor_ops_enabled;

  // Replace instr with a new CustomCall which has the correct algorithm, and
  // whose output shape has the appropriate amount of scratch memory.
  HloComputation* computation = instr->parent();
  Shape new_call_shape =
      ShapeUtil::MakeTupleShape({instr->shape().tuple_shapes(0),
                                 ShapeUtil::MakeShape(U8, {scratch_bytes})});
  HloInstruction* algorithm_hlo = computation->AddInstruction(
      HloInstruction::CreateConstant(Literal::CreateR0<int64>(algorithm)));
  HloInstruction* tensor_ops_enabled_hlo =
      computation->AddInstruction(HloInstruction::CreateConstant(
          Literal::CreateR0<bool>(tensor_ops_enabled)));

  HloInstruction* new_call =
      computation->AddInstruction(HloInstruction::CreateCustomCall(
          new_call_shape,
          {instr->mutable_operand(0), instr->mutable_operand(1), algorithm_hlo,
           tensor_ops_enabled_hlo},
          instr->custom_call_target()));
  new_call->set_window(instr->window());
  new_call->set_convolution_dimension_numbers(
      instr->convolution_dimension_numbers());

  // Repackage new_call so it has the same shape as the original call, namely
  // (conv_result, u8[0]).
  HloInstruction* new_tuple =
      computation->AddInstruction(HloInstruction::CreateTuple(
          {computation->AddInstruction(HloInstruction::CreateGetTupleElement(
               new_call_shape.tuple_shapes(0), new_call, 0)),
           computation->AddInstruction(
               HloInstruction::CreateConstant(Literal::CreateR1<uint8>({})))}));

  TF_RETURN_IF_ERROR(instr->parent()->ReplaceInstruction(instr, new_tuple));
  return true;
}

StatusOr<bool> CudnnConvolutionAlgorithmPicker::RunOnComputation(
    HloComputation* computation) {
  std::vector<HloInstruction*> convs;
  for (auto* instr : computation->instructions()) {
    if (IsCustomCallToDnnConvolution(*instr)) {
      convs.push_back(instr);
    }
  }

  bool changed = false;
  for (auto* instr : convs) {
    TF_ASSIGN_OR_RETURN(bool result, RunOnInstruction(instr));
    changed |= result;
  }
  return changed;
}

StatusOr<bool> CudnnConvolutionAlgorithmPicker::Run(HloModule* module) {
  bool changed = false;
  for (HloComputation* computation : module->MakeNonfusionComputations()) {
    TF_ASSIGN_OR_RETURN(bool result, RunOnComputation(computation));
    changed |= result;
  }
  return changed;
}

}  // namespace gpu
}  // namespace xla
