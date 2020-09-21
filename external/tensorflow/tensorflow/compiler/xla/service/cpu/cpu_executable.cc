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

#include "tensorflow/compiler/xla/service/cpu/cpu_executable.h"

#include <stdint.h>
#include <algorithm>
#include <set>
#include <unordered_set>
#include <utility>
#include <vector>

#include "llvm/ExecutionEngine/Orc/IRCompileLayer.h"
#include "tensorflow/compiler/xla/service/buffer_assignment.h"
#include "tensorflow/compiler/xla/service/computation_layout.h"
#include "tensorflow/compiler/xla/service/hlo_computation.h"
#include "tensorflow/compiler/xla/service/hlo_module.h"
#include "tensorflow/compiler/xla/service/logical_buffer.h"
#include "tensorflow/compiler/xla/service/shaped_buffer.h"
#include "tensorflow/compiler/xla/shape_tree.h"
#include "tensorflow/compiler/xla/shape_util.h"
#include "tensorflow/compiler/xla/status_macros.h"
#include "tensorflow/compiler/xla/types.h"
#include "tensorflow/compiler/xla/util.h"
#include "tensorflow/compiler/xla/xla_data.pb.h"
#include "tensorflow/core/lib/strings/str_util.h"
#include "tensorflow/core/lib/strings/strcat.h"
#include "tensorflow/core/lib/strings/stringprintf.h"
#include "tensorflow/core/platform/env.h"
#include "tensorflow/core/platform/logging.h"
#include "tensorflow/core/platform/mem.h"
#include "tensorflow/core/platform/mutex.h"
#include "tensorflow/core/platform/types.h"
#include "tensorflow/stream_executor/host/host_stream.h"

namespace se = ::perftools::gputools;

namespace xla {
namespace cpu {

CpuExecutable::CpuExecutable(
    std::unique_ptr<SimpleOrcJIT> jit,
    std::unique_ptr<const BufferAssignment> assignment,
    std::unique_ptr<const HloModule> hlo_module,
    const string& entry_function_name,
    std::unique_ptr<HloProfilePrinterData> hlo_profile_printer_data,
    std::unique_ptr<HloProfileIndexMap> hlo_profile_index_map)
    : Executable(std::move(hlo_module), std::move(hlo_profile_printer_data),
                 std::move(hlo_profile_index_map)),
      jit_(std::move(jit)),
      assignment_(std::move(assignment)) {
  // Resolve symbols in the constructor rather than at execution time to avoid
  // races because FindSymbol is not thread safe.
  llvm::JITSymbol sym = jit_->FindCompiledSymbol(entry_function_name);
  // We expect to find the symbol provided with entry_function_name; otherwise
  // this is an internal error.
  CHECK(sym) << "Symbol " << entry_function_name << " not found.";
  // getAddress can do work under the hood in the jit, so it needs to be
  // guarded by the mutex.
  compute_function_ =
      reinterpret_cast<ComputeFunctionType>(cantFail(sym.getAddress()));
}

Status CpuExecutable::AllocateBuffers(
    DeviceMemoryAllocator* memory_allocator, int device_ordinal,
    std::vector<perftools::gputools::DeviceMemoryBase>* buffers) {
  CHECK_EQ(buffers->size(), assignment_->Allocations().size());
  VLOG(3) << "Allocating " << assignment_->Allocations().size()
          << " allocations for module " << module().name();
  for (BufferAllocation::Index i = 0; i < assignment_->Allocations().size();
       ++i) {
    auto& allocation = assignment_->GetAllocation(i);

    VLOG(3) << allocation.ToString();

    if (allocation.is_entry_computation_parameter()) {
      VLOG(3) << "allocation #" << i << " is a parameter";
      continue;
    }

    if (allocation.is_thread_local()) {
      VLOG(3) << "buffer #" << i << " is thread-local";
      continue;
    }

    int64 buffer_size = allocation.size();
    if (!(*buffers)[i].is_null()) {
      VLOG(3) << "buffer #" << i
              << " is in the preallocated result ShapedBuffer";
    } else {
      TF_ASSIGN_OR_RETURN((*buffers)[i], memory_allocator->Allocate(
                                             device_ordinal, buffer_size));

      VLOG(3) << "buffer #" << i << " allocated " << buffer_size << " bytes ["
              << (*buffers)[i].opaque() << "]";
    }

    // Since the output buffer and all the temporary buffers were written into
    // by the JITed code, msan has no way of knowing their memory was
    // initialized. Mark them initialized so that msan doesn't flag loads from
    // these buffers.
    TF_ANNOTATE_MEMORY_IS_INITIALIZED((*buffers)[i].opaque(), buffer_size);
  }

  TF_ASSIGN_OR_RETURN(const BufferAllocation::Slice result_slice,
                      assignment_->GetUniqueTopLevelOutputSlice());
  VLOG(3) << "result index: " << result_slice.index();

  return Status::OK();
}

Status CpuExecutable::ExecuteComputeFunction(
    const ExecutableRunOptions* run_options,
    tensorflow::gtl::ArraySlice<const ShapedBuffer*> arguments,
    tensorflow::gtl::ArraySlice<se::DeviceMemoryBase> buffers,
    HloExecutionProfile* hlo_execution_profile) {
  // The calling convention for JITed functions is:
  //
  //  void function(void* result, const void* run_options, void** args_array,
  //                void** temps_array)
  //
  // result: Points at the result.
  // run_options: the ExecutableRunOptions object.
  // args_array: An array of pointers, each of which points to a parameter.
  //               The size of this array is determined by the function's arity
  //               (ProgramShape).
  // temps_array:  An array of pointers, each of which points to a temporary
  //               buffer the computation needs. The size of this array is
  //               determined by buffer analysis.
  //
  std::vector<const void*> args_array;
  for (const ShapedBuffer* argument : arguments) {
    args_array.push_back(argument->root_buffer().opaque());
  }

  uint64 start_micros = tensorflow::Env::Default()->NowMicros();

  size_t profile_counters_size =
      hlo_execution_profile ? hlo_execution_profile->profile_counters().size()
                            : 0;
  int64* profile_counters =
      hlo_execution_profile
          ? hlo_execution_profile->mutable_profile_counters()->data()
          : nullptr;

  // Call the computation function following the calling convention.
  std::vector<void*> buffer_pointers;
  for (auto& buffer : buffers) {
    buffer_pointers.push_back(const_cast<void*>(buffer.opaque()));
  }
  TF_ASSIGN_OR_RETURN(const BufferAllocation::Slice result_slice,
                      assignment_->GetUniqueTopLevelOutputSlice());
  void* result_buffer = buffer_pointers[result_slice.index()];
  if (VLOG_IS_ON(3)) {
    VLOG(3) << "Executing compute function:";
    VLOG(3) << tensorflow::strings::Printf(
        "  func(void* result, void* params[%zu], void* temps[%zu], "
        "uint64 profile_counters[%zu])",
        args_array.size(), buffer_pointers.size(), profile_counters_size);
    VLOG(3) << tensorflow::strings::Printf("    result = %p", result_buffer);
    auto ptr_printer = [](string* out, const void* p) {
      tensorflow::strings::StrAppend(out, tensorflow::strings::Printf("%p", p));
    };
    VLOG(3) << tensorflow::strings::Printf(
        "    params = [%s]",
        tensorflow::str_util::Join(args_array, ", ", ptr_printer).c_str());
    VLOG(3) << tensorflow::strings::Printf(
        "    temps = [%s]",
        tensorflow::str_util::Join(buffer_pointers, ", ", ptr_printer).c_str());
    VLOG(3) << tensorflow::strings::Printf("    profile_counters = %p",
                                           profile_counters);
  }

  compute_function_(result_buffer, run_options, args_array.data(),
                    buffer_pointers.data(), profile_counters);

  uint64 end_micros = tensorflow::Env::Default()->NowMicros();

  {
    tensorflow::mutex_lock lock(mutex_);
    const double nanoseconds = (end_micros - start_micros) * 1000.0;
    execution_profile_.set_compute_time_ns(std::max(nanoseconds, 1.0));
    // If hlo profiling was disabled then the cycle count is left empty.
    if (hlo_execution_profile) {
      execution_profile_.set_compute_cycle_count(
          hlo_execution_profile->total_cycles_executed(
              *module().entry_computation()));
    }
  }

  return Status::OK();
}

static void LogLiveAddresses(
    tensorflow::gtl::ArraySlice<se::DeviceMemoryBase> buffers,
    const std::vector<bool>& buffers_in_result) {
  if (!VLOG_IS_ON(3)) {
    return;
  }

  CHECK_EQ(buffers.size(), buffers_in_result.size());
  std::vector<const void*> live_out_buffers;
  for (int i = 0; i < buffers.size(); ++i) {
    if (buffers_in_result[i]) {
      live_out_buffers.push_back(buffers[i].opaque());
    }
  }
  VLOG(3) << "Live addresses in output marking found "
          << live_out_buffers.size() << " addresses:\n"
          << tensorflow::str_util::Join(
                 live_out_buffers, ", ", [](string* out, const void* address) {
                   tensorflow::strings::StrAppend(
                       out, tensorflow::strings::Printf("%p", address));
                 });
}

static Status DeallocateTempBuffers(
    DeviceMemoryAllocator* allocator, se::Stream* stream,
    tensorflow::gtl::ArraySlice<se::DeviceMemoryBase> buffers,
    const std::vector<bool>& buffers_in_result) {
  // Keep those buffers in the output of the marked live because they are needed
  // by the service. They will be deallocated by the service.
  for (size_t i = 0; i < buffers.size(); ++i) {
    se::DeviceMemoryBase alloc = buffers[i];
    if (!buffers_in_result[i] && !alloc.is_null()) {
      VLOG(3) << "CpuExecutable deallocating buffer #" << i << " ["
              << alloc.opaque() << "]";
      TF_RETURN_IF_ERROR(
          allocator->Deallocate(stream->parent()->device_ordinal(), &alloc));
    }
  }

  return Status::OK();
}

StatusOr<std::unique_ptr<ShapedBuffer>> CpuExecutable::CreateResultShapedBuffer(
    const ServiceExecutableRunOptions* run_options,
    tensorflow::gtl::ArraySlice<perftools::gputools::DeviceMemoryBase>
        allocated_buffers,
    std::vector<bool>* buffers_in_result) {
  se::Stream* stream = run_options->stream();
  auto result_buffer = MakeUnique<ShapedBuffer>(
      /*on_host_shape=*/result_shape(), /*on_device_shape=*/result_shape(),
      stream->parent()->platform(), stream->parent()->device_ordinal());

  // Copy DeviceMemoryBase values which contain the array(s) of the result into
  // the respective location in ShapedBuffer which is returned to the caller.
  TF_RETURN_IF_ERROR(result_buffer->buffers().ForEachMutableElementWithStatus(
      [&](const ShapeIndex& index, se::DeviceMemoryBase* device_memory) {
        const auto& sources = this->GetRootPointsToSet().element(index);
        // The points to set is unambiguous so the set should be a
        // singleton.
        CHECK_EQ(1, sources.size());
        const LogicalBuffer* buffer_source = sources[0];
        HloInstruction* src = buffer_source->instruction();

        // The source for this result buffer can be a nested buffer such as
        // a tuple element. The source instruction should have a
        // non-parameter buffer assigned.
        TF_ASSIGN_OR_RETURN(
            const BufferAllocation::Slice slice,
            this->assignment_->GetUniqueSlice(src, buffer_source->index()));
        CHECK(!slice.allocation()->is_entry_computation_parameter());

        const BufferAllocation::Index buffer_index = slice.index();
        const se::DeviceMemoryBase& buffer = allocated_buffers[buffer_index];
        CHECK(!buffer.is_null() || buffer.size() == 0);
        *device_memory = buffer;
        (*buffers_in_result)[buffer_index] = true;
        return Status::OK();
      }));
  return std::move(result_buffer);
}

StatusOr<std::unique_ptr<ShapedBuffer>> CpuExecutable::ExecuteOnStream(
    const ServiceExecutableRunOptions* run_options,
    tensorflow::gtl::ArraySlice<const ShapedBuffer*> arguments,
    HloExecutionProfile* hlo_execution_profile) {
  if (GetRootPointsToSet().IsAmbiguous()) {
    return Unimplemented("Points-to set of root instruction is ambiguous");
  }

  se::Stream* stream = run_options->stream();
  DeviceMemoryAllocator* memory_allocator = run_options->allocator();
  std::vector<se::DeviceMemoryBase> buffers(assignment_->Allocations().size());

  TF_RETURN_IF_ERROR(AllocateBuffers(
      memory_allocator, stream->parent()->device_ordinal(), &buffers));
  TF_RETURN_IF_ERROR(ExecuteComputeFunction(
      &run_options->run_options(), arguments, buffers, hlo_execution_profile));

  std::vector<bool> buffers_in_result(assignment_->Allocations().size(), false);
  TF_ASSIGN_OR_RETURN(
      std::unique_ptr<ShapedBuffer> result_buffer,
      CreateResultShapedBuffer(run_options, buffers, &buffers_in_result));

  // Free all buffers not in the result.
  TF_RETURN_IF_ERROR(DeallocateTempBuffers(memory_allocator, stream, buffers,
                                           buffers_in_result));

  return std::move(result_buffer);
}

StatusOr<std::unique_ptr<ShapedBuffer>> CpuExecutable::ExecuteAsyncOnStream(
    const ServiceExecutableRunOptions* run_options,
    tensorflow::gtl::ArraySlice<const ShapedBuffer*> arguments) {
  if (hlo_profiling_enabled()) {
    return Unimplemented(
        "Asynchronous execution on stream with hlo profiling is not yet "
        "supported on CPU.");
  }

  auto* host_stream = dynamic_cast<perftools::gputools::host::HostStream*>(
      run_options->stream()->implementation());
  se::Stream* stream = run_options->stream();
  DeviceMemoryAllocator* memory_allocator = run_options->allocator();
  std::vector<se::DeviceMemoryBase> buffers(assignment_->Allocations().size());

  TF_RETURN_IF_ERROR(AllocateBuffers(
      memory_allocator, stream->parent()->device_ordinal(), &buffers));

  std::vector<bool> buffers_in_result(assignment_->Allocations().size(), false);
  TF_ASSIGN_OR_RETURN(
      std::unique_ptr<ShapedBuffer> result_buffer,
      CreateResultShapedBuffer(run_options, buffers, &buffers_in_result));

  LogLiveAddresses(buffers, buffers_in_result);

  host_stream->EnqueueTask([this, run_options, arguments, buffers,
                            buffers_in_result, memory_allocator, stream]() {
    // Failing a CHECK here is not great, but I don't see an obvious way to
    // return a failed Status asynchronously.
    TF_CHECK_OK(ExecuteComputeFunction(&run_options->run_options(), arguments,
                                       buffers,
                                       /*hlo_execution_profile=*/nullptr));
    TF_CHECK_OK(DeallocateTempBuffers(memory_allocator, stream, buffers,
                                      buffers_in_result));
  });

  return std::move(result_buffer);
}

/*static*/ int64 CpuExecutable::ShapeSizeBytes(const Shape& shape) {
  // On the cpu, opaques are pointers.
  if (ShapeUtil::IsOpaque(shape)) {
    return sizeof(void*);
  }
  return ShapeUtil::ByteSizeOf(shape, sizeof(void*));
}

const PointsToSet& CpuExecutable::GetRootPointsToSet() const {
  return assignment_->points_to_analysis().GetPointsToSet(
      module().entry_computation()->root_instruction());
}

}  // namespace cpu
}  // namespace xla
