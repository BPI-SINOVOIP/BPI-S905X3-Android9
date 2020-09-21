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

#include "tensorflow/compiler/xla/client/local_client.h"

#include <utility>

#include "llvm/ADT/Triple.h"
#include "tensorflow/compiler/xla/ptr_util.h"
#include "tensorflow/compiler/xla/service/backend.h"
#include "tensorflow/compiler/xla/service/service_executable_run_options.h"
#include "tensorflow/compiler/xla/service/source_map_util.h"
#include "tensorflow/compiler/xla/status_macros.h"

namespace se = ::perftools::gputools;

using xla::source_map_util::InvalidParameterArgument;

namespace xla {

namespace {
StatusOr<Backend::StreamPtr> BorrowStreamForDevice(int device_ordinal,
                                                   Backend* backend) {
  if (device_ordinal < 0) {
    device_ordinal = backend->default_device_ordinal();
  }
  return backend->BorrowStream(device_ordinal);
}
}  // namespace

LocalExecutable::LocalExecutable(std::unique_ptr<Executable> executable,
                                 Backend* backend,
                                 ExecutableBuildOptions build_options)
    : executable_(std::move(executable)),
      backend_(backend),
      build_options_(std::move(build_options)) {
  CHECK_GE(build_options_.device_ordinal(), 0)
      << "Must have a valid device ordinal that the executable was built for.";
}

tensorflow::Status LocalExecutable::ValidateExecutionOptions(
    const tensorflow::gtl::ArraySlice<const ShapedBuffer*> arguments,
    const ExecutableRunOptions& run_options, const Backend& backend) {
  const ComputationLayout& computation_layout =
      executable_->module_config().entry_computation_layout();

  // Check argument number, shapes, and layouts.
  if (arguments.size() != computation_layout.parameter_count()) {
    return InvalidArgument(
        "invalid number of arguments for computation: expected %d, got %zu",
        computation_layout.parameter_count(), arguments.size());
  }
  for (int i = 0; i < arguments.size(); ++i) {
    if (!computation_layout.parameter_layout(i).MatchesLayoutInShape(
            arguments[i]->on_host_shape())) {
      return InvalidParameterArgument(
          executable_.get(), i,
          "Argument does not match shape or layout of computation parameter "
          "%d: want %s, got %s",
          i,
          ShapeUtil::HumanString(computation_layout.parameter_layout(i).shape())
              .c_str(),
          ShapeUtil::HumanString(arguments[i]->on_host_shape()).c_str());
    }
  }

  if (run_options.stream() != nullptr) {
    if (!run_options.stream()->ok()) {
      return InvalidArgument("stream is uninitialized or in an error state");
    }

    // Check stream matches service platform.
    const se::Platform* stream_platform =
        run_options.stream()->parent()->platform();
    if (stream_platform != backend_->platform()) {
      return InvalidArgument(
          "stream is for platform %s, but service targets platform %s",
          stream_platform->Name().c_str(),
          backend_->platform()->Name().c_str());
    }

    // Cannot specify device_ordinal with a stream. The stream determines these
    // values.
    if (run_options.device_ordinal() != -1) {
      return InvalidArgument(
          "cannot set both device ordinal and stream options in "
          "ExecutableRunOptions; the stream determines the device ordinal");
    }
  }

  // Verify that the device the executable was built for is equivalent to the
  // device it will run on.
  int run_device_ordinal = run_options.device_ordinal() == -1
                               ? backend_->default_device_ordinal()
                               : run_options.device_ordinal();
  TF_ASSIGN_OR_RETURN(bool devices_equivalent,
                      backend_->devices_equivalent(
                          run_device_ordinal, build_options_.device_ordinal()));
  if (!devices_equivalent) {
    TF_ASSIGN_OR_RETURN(se::StreamExecutor * run_executor,
                        backend_->stream_executor(run_device_ordinal));
    TF_ASSIGN_OR_RETURN(se::StreamExecutor * build_executor,
                        backend_->stream_executor(build_device_ordinal()));
    return InvalidArgument(
        "executable is built for device %s of type \"%s\"; cannot run it on "
        "device %s of type \"%s\"",
        backend_->device_name(build_device_ordinal()).c_str(),
        build_executor->GetDeviceDescription().name().c_str(),
        backend_->device_name(run_device_ordinal).c_str(),
        run_executor->GetDeviceDescription().name().c_str());
  }

  if (!run_options.allocator()) {
    return InvalidArgument("an allocator must be provided to ExecuteLocally");
  }

  if (run_options.allocator()->platform() != backend.platform()) {
    return InvalidArgument(
        "allocator platform (%s) does not match service platform (%s)",
        run_options.allocator()->platform()->Name().c_str(),
        backend.platform()->Name().c_str());
  }

  return Status::OK();
}

StatusOr<std::unique_ptr<ScopedShapedBuffer>> LocalExecutable::Run(
    const tensorflow::gtl::ArraySlice<const ShapedBuffer*> arguments,
    ExecutableRunOptions run_options) {
  TF_RETURN_IF_ERROR(
      ValidateExecutionOptions(arguments, run_options, *backend_));

  Backend::StreamPtr stream;
  if (run_options.stream() == nullptr) {
    // NB!  The lifetime of `stream` needs to match the lifetime of
    // `actual_options` (otherwise we will end up using a returned stream in
    // ExecuteOnStreamWrapper), which is why it isn't declared in the inner "if"
    // scope.
    TF_ASSIGN_OR_RETURN(
        stream, BorrowStreamForDevice(run_options.device_ordinal(), backend_));
    run_options.set_stream(stream.get());
  }
  if (run_options.allocator() == nullptr) {
    run_options.set_allocator(backend_->memory_allocator());
  }

  // For local client execution on CPU backends:
  // *) The thread pool used for eigen CPU ops is from
  //    ExecutableRunOptions.eigen_intra_op_thread_pool.
  // *) The thread pool used for XLA CPU ops is from
  //    backend_->eigen_intra_op_thread_pool().
  ServiceExecutableRunOptions service_options(
      run_options, backend_->StreamBorrower(),
      backend_->eigen_intra_op_thread_pool());

  if (executable_->dumping()) {
    return ExecuteAndDump(&service_options, arguments);
  }
  TF_ASSIGN_OR_RETURN(
      std::unique_ptr<ShapedBuffer> result,
      executable_->ExecuteOnStreamWrapper(
          &service_options, run_options.execution_profile(), arguments));

  return MakeUnique<ScopedShapedBuffer>(std::move(*result),
                                        run_options.allocator());
}

StatusOr<std::unique_ptr<ScopedShapedBuffer>> LocalExecutable::ExecuteAndDump(
    const ServiceExecutableRunOptions* run_options,
    const tensorflow::gtl::ArraySlice<const ShapedBuffer*> arguments) {
  executable_->session_module()->set_execution_platform(
      backend_->platform()->Name());
  TF_RETURN_IF_ERROR(RecordArguments(arguments, executable_->session_module()));
  TF_ASSIGN_OR_RETURN(
      std::unique_ptr<ShapedBuffer> result,
      executable_->ExecuteOnStream(run_options, arguments,
                                   /*hlo_execution_profile=*/nullptr));
  TF_RETURN_IF_ERROR(RecordResult(result.get(), executable_->session_module()));
  TF_RETURN_IF_ERROR(executable_->DumpSessionModule());
  return ScopedShapedBuffer::MakeScoped(result.get(), run_options->allocator());
}

tensorflow::Status LocalExecutable::RecordArguments(
    const tensorflow::gtl::ArraySlice<const ShapedBuffer*> arguments,
    SessionModule* session_module) {
  session_module->clear_arguments();
  for (const ShapedBuffer* argument : arguments) {
    TF_ASSIGN_OR_RETURN(std::unique_ptr<Literal> literal,
                        LiteralFromShapedBuffer(*argument));
    *session_module->add_arguments() = literal->ToProto();
  }
  return Status::OK();
}

tensorflow::Status LocalExecutable::RecordResult(
    const ShapedBuffer* result, SessionModule* session_module) {
  session_module->clear_result();
  TF_ASSIGN_OR_RETURN(std::unique_ptr<Literal> literal,
                      LiteralFromShapedBuffer(*result));
  *session_module->mutable_result() = literal->ToProto();
  return Status::OK();
}

StatusOr<std::unique_ptr<Literal>> LocalExecutable::LiteralFromShapedBuffer(
    const ShapedBuffer& shaped_buffer) {
  TF_ASSIGN_OR_RETURN(
      se::StreamExecutor * executor,
      backend_->stream_executor(shaped_buffer.device_ordinal()));
  return backend_->transfer_manager()->TransferLiteralFromDevice(executor,
                                                                 shaped_buffer);
}

se::Platform* LocalClient::platform() const {
  return local_service_->backend().platform();
}

int LocalClient::device_count() const {
  return local_service_->backend().device_count();
}

bool LocalClient::device_ordinal_supported(int device_ordinal) const {
  return local_service_->backend().device_ordinal_supported(device_ordinal);
}

int LocalClient::default_device_ordinal() const {
  return local_service_->backend().default_device_ordinal();
}

const Backend& LocalClient::backend() const {
  return local_service_->backend();
}

Backend* LocalClient::mutable_backend() {
  return local_service_->mutable_backend();
}

StatusOr<std::unique_ptr<LocalExecutable>> LocalClient::Compile(
    const Computation& computation,
    const tensorflow::gtl::ArraySlice<const Shape*> argument_layouts,
    const ExecutableBuildOptions& options) {
  ExecutableBuildOptions updated_options = options;
  if (options.device_ordinal() == -1) {
    updated_options.set_device_ordinal(default_device_ordinal());
    VLOG(3) << "Set device ordinal to default value of: "
            << updated_options.device_ordinal();
  }
  TF_ASSIGN_OR_RETURN(
      std::unique_ptr<Executable> executable,
      local_service_->CompileExecutable(computation.handle(), argument_layouts,
                                        updated_options));
  return WrapUnique(new LocalExecutable(std::move(executable),
                                        local_service_->mutable_backend(),
                                        updated_options));
}

StatusOr<std::unique_ptr<ScopedShapedBuffer>>
LocalClient::LiteralToShapedBuffer(const Literal& literal, int device_ordinal,
                                   DeviceMemoryAllocator* allocator) {
  if (allocator == nullptr) {
    allocator = backend().memory_allocator();
  }
  TF_ASSIGN_OR_RETURN(auto scoped_buffer,
                      backend().transfer_manager()->AllocateScopedShapedBuffer(
                          literal.shape(), allocator, device_ordinal));
  TF_ASSIGN_OR_RETURN(se::StreamExecutor * executor,
                      backend().stream_executor(device_ordinal));
  TF_RETURN_IF_ERROR(backend().transfer_manager()->TransferLiteralToDevice(
      executor, literal, *scoped_buffer));
  return std::move(scoped_buffer);
}

StatusOr<std::unique_ptr<Literal>> LocalClient::ShapedBufferToLiteral(
    const ShapedBuffer& shaped_buffer) {
  TF_ASSIGN_OR_RETURN(
      se::StreamExecutor * executor,
      backend().stream_executor(shaped_buffer.device_ordinal()));
  return backend().transfer_manager()->TransferLiteralFromDevice(executor,
                                                                 shaped_buffer);
}

Status LocalClient::TransferToInfeedLocal(const Literal& literal,
                                          int device_ordinal) {
  TF_ASSIGN_OR_RETURN(se::StreamExecutor * executor,
                      backend().stream_executor(device_ordinal));
  return backend().transfer_manager()->TransferLiteralToInfeed(executor,
                                                               literal);
}

StatusOr<std::unique_ptr<Literal>> LocalClient::TransferFromOutfeedLocal(
    const Shape& shape, int device_ordinal) {
  TF_ASSIGN_OR_RETURN(se::StreamExecutor * executor,
                      backend().stream_executor(device_ordinal));
  auto literal = MakeUnique<Literal>();
  TF_RETURN_IF_ERROR(backend().transfer_manager()->TransferLiteralFromOutfeed(
      executor, shape, literal.get()));
  return std::move(literal);
}

StatusOr<int> LocalClient::ReplicaNumberToDeviceOrdinal(int replica_number) {
  return local_service_->ReplicaNumberToDeviceOrdinal(replica_number);
}

}  // namespace xla
