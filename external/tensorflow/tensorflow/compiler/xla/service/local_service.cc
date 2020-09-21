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

#include "tensorflow/compiler/xla/service/local_service.h"

#include <string>
#include <utility>
#include <vector>

#include "tensorflow/compiler/xla/client/executable_build_options.h"
#include "tensorflow/compiler/xla/execution_options_util.h"
#include "tensorflow/compiler/xla/ptr_util.h"
#include "tensorflow/compiler/xla/service/backend.h"
#include "tensorflow/compiler/xla/service/computation_layout.h"
#include "tensorflow/compiler/xla/service/computation_tracker.h"
#include "tensorflow/compiler/xla/service/executable.h"
#include "tensorflow/compiler/xla/service/hlo_computation.h"
#include "tensorflow/compiler/xla/service/hlo_execution_profile.h"
#include "tensorflow/compiler/xla/service/hlo_module.h"
#include "tensorflow/compiler/xla/service/hlo_module_config.h"
#include "tensorflow/compiler/xla/service/platform_util.h"
#include "tensorflow/compiler/xla/service/user_computation.h"
#include "tensorflow/compiler/xla/service/versioned_computation_handle.h"
#include "tensorflow/compiler/xla/shape_layout.h"
#include "tensorflow/compiler/xla/shape_util.h"
#include "tensorflow/compiler/xla/status_macros.h"
#include "tensorflow/compiler/xla/types.h"
#include "tensorflow/compiler/xla/util.h"
#include "tensorflow/core/lib/gtl/cleanup.h"
#include "tensorflow/core/lib/strings/strcat.h"
#include "tensorflow/core/platform/logging.h"
#include "tensorflow/core/platform/stream_executor_no_cuda.h"

namespace se = ::perftools::gputools;

namespace xla {

/* static */ StatusOr<std::unique_ptr<LocalService>> LocalService::NewService(
    const ServiceOptions& options) {
  perftools::gputools::Platform* platform = options.platform();
  if (platform == nullptr) {
    TF_ASSIGN_OR_RETURN(platform, PlatformUtil::GetDefaultPlatform());
  }

  BackendOptions backend_options;
  backend_options.set_platform(platform).set_intra_op_parallelism_threads(
      options.intra_op_parallelism_threads());
  TF_ASSIGN_OR_RETURN(std::unique_ptr<Backend> backend,
                      Backend::CreateBackend(backend_options));

  std::unique_ptr<LocalService> service(
      new LocalService(options, std::move(backend)));
  return std::move(service);
}

LocalService::LocalService(const ServiceOptions& options,
                           std::unique_ptr<Backend> execute_backend)
    : Service(options, std::move(execute_backend)) {}

StatusOr<std::unique_ptr<Executable>> LocalService::CompileExecutable(
    const ComputationHandle& computation,
    const tensorflow::gtl::ArraySlice<const Shape*> argument_layouts,
    const ExecutableBuildOptions& build_options) {
  TF_ASSIGN_OR_RETURN(UserComputation * user_computation,
                      computation_tracker_.Resolve(computation));
  VersionedComputationHandle versioned_handle =
      user_computation->GetVersionedHandle();

  TF_ASSIGN_OR_RETURN(
      std::shared_ptr<const ProgramShape> program_shape,
      user_computation->ComputeProgramShape(versioned_handle.version));

  // Validate incoming layouts.
  if (argument_layouts.size() != program_shape->parameters_size()) {
    return InvalidArgument(
        "Invalid number of arguments for computation: expected %d, got %zu.",
        program_shape->parameters_size(), argument_layouts.size());
  }
  for (int i = 0; i < argument_layouts.size(); ++i) {
    const Shape& argument_shape = *argument_layouts[i];
    TF_RETURN_IF_ERROR(ShapeUtil::ValidateShape(argument_shape));
    if (!ShapeUtil::Compatible(argument_shape, program_shape->parameters(i))) {
      tensorflow::gtl::optional<const OpMetadata*> metadata =
          user_computation->ParameterMetadata(i);
      auto metadata_string = [&metadata]() -> string {
        if (!metadata.has_value()) {
          return "";
        }
        CHECK(metadata.value() != nullptr);
        const OpMetadata& m = *metadata.value();
        if (!m.source_file().empty()) {
          return tensorflow::strings::Printf(
              " (%s:%d)", m.source_file().c_str(), m.source_line());
        }
        return "";
      };
      return InvalidArgument(
          "Invalid argument shape for argument %d%s, expected %s, got %s.", i,
          metadata_string().c_str(),
          ShapeUtil::HumanString(program_shape->parameters(i)).c_str(),
          ShapeUtil::HumanString(argument_shape).c_str());
    }
  }
  if (build_options.result_layout() != nullptr) {
    TF_RETURN_IF_ERROR(ValidateResultShapeWithLayout(
        *build_options.result_layout(), program_shape->result()));
  }

  ExecutionOptions execution_options = CreateDefaultExecutionOptions();
  if (build_options.generate_hlo_graph().has_value()) {
    execution_options.mutable_debug_options()->set_xla_generate_hlo_graph(
        build_options.generate_hlo_graph().value());
  }
  if (build_options.result_layout() != nullptr) {
    *execution_options.mutable_shape_with_output_layout() =
        *build_options.result_layout();
  } else {
    *execution_options.mutable_shape_with_output_layout() =
        program_shape->result();
    LayoutUtil::SetToDefaultLayout(
        execution_options.mutable_shape_with_output_layout());
  }
  TF_ASSIGN_OR_RETURN(
      std::unique_ptr<HloModuleConfig> module_config,
      CreateModuleConfig(*program_shape, argument_layouts, &execution_options,
                         *user_computation));

  TF_ASSIGN_OR_RETURN(
      se::StreamExecutor * executor,
      execute_backend_->stream_executor(build_options.device_ordinal()));

  return BuildExecutable(versioned_handle, std::move(module_config),
                         execute_backend_.get(), executor,
                         build_options.device_allocator());
}

StatusOr<int> LocalService::ReplicaNumberToDeviceOrdinal(int replica_number) {
  return backend().computation_placer()->DeviceId(
      replica_number, /*computation=*/0, options_.number_of_replicas(),
      /*computation_count=*/1);
}

}  // namespace xla
