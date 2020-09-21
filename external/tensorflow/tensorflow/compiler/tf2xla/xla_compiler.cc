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

#include "tensorflow/compiler/tf2xla/xla_compiler.h"

#include <deque>
#include <numeric>

#include "tensorflow/compiler/tf2xla/const_analysis.h"
#include "tensorflow/compiler/tf2xla/dump_graph.h"
#include "tensorflow/compiler/tf2xla/functionalize_control_flow.h"
#include "tensorflow/compiler/tf2xla/graph_compiler.h"
#include "tensorflow/compiler/tf2xla/shape_util.h"
#include "tensorflow/compiler/tf2xla/sharding_util.h"
#include "tensorflow/compiler/tf2xla/tf2xla_util.h"
#include "tensorflow/compiler/tf2xla/type_util.h"
#include "tensorflow/compiler/tf2xla/xla_compilation_device.h"
#include "tensorflow/compiler/tf2xla/xla_context.h"
#include "tensorflow/compiler/tf2xla/xla_op_kernel.h"
#include "tensorflow/compiler/xla/client/client_library.h"
#include "tensorflow/core/common_runtime/device.h"
#include "tensorflow/core/common_runtime/executor.h"
#include "tensorflow/core/common_runtime/function.h"
#include "tensorflow/core/common_runtime/graph_optimizer.h"
#include "tensorflow/core/framework/attr_value_util.h"
#include "tensorflow/core/graph/algorithm.h"
#include "tensorflow/core/graph/graph_constructor.h"
#include "tensorflow/core/graph/node_builder.h"
#include "tensorflow/core/lib/hash/hash.h"
#include "tensorflow/core/platform/logging.h"
#include "tensorflow/core/public/version.h"

namespace tensorflow {
namespace {

// Checks that arguments `args` match types `types`.
Status CheckSignature(const DataTypeVector& types,
                      const std::vector<XlaCompiler::Argument>& args) {
  if (args.size() != types.size()) {
    return errors::Internal("Compilation arguments have ", args.size(),
                            " elements while function has ", types.size());
  }
  for (int i = 0; i < types.size(); ++i) {
    if (types[i] != args[i].type && types[i] != DT_RESOURCE) {
      return errors::Internal(
          "Argument ", i, " has declared type ", DataTypeString(args[i].type),
          " but function parameter has type ", DataTypeString(types[i]));
    }
  }
  return Status::OK();
}

}  // namespace

bool XlaCompiler::Argument::operator==(
    const XlaCompiler::Argument& other) const {
  if (std::tie(kind, resource_kind, type, name, initialized, tensor_array_size,
               tensor_array_gradients) !=
      std::tie(other.kind, other.resource_kind, other.type, other.name,
               other.initialized, other.tensor_array_size,
               other.tensor_array_gradients)) {
    return false;
  }
  if (shape != other.shape) {
    return false;
  }
  if (constant_value.shape() != other.constant_value.shape()) {
    return false;
  }
  return constant_value.tensor_data() == other.constant_value.tensor_data();
}

XlaCompiler::XlaCompiler(XlaCompiler::Options options)
    : options_(options),
      initialization_status_(Status::OK()),
      next_step_id_(1),
      device_(
          new XlaCompilationDevice(SessionOptions(), *options_.device_type)),
      device_mgr_({device_}) {
  // We no longer need the device_type.
  options_.device_type = nullptr;

  if (options_.populate_resource_manager) {
    initialization_status_ =
        (*options_.populate_resource_manager)(device_->resource_manager());
  }

  local_flib_def_.reset(new FunctionLibraryDefinition(OpRegistry::Global(),
                                                      FunctionDefLibrary{}));
  local_pflr_.reset(new ProcessFunctionLibraryRuntime(
      &device_mgr_, Env::Default(), options.graph_def_version,
      local_flib_def_.get(), OptimizerOptions(),
      nullptr /* custom_kernel_creator */));
  pflr_.reset(new ProcessFunctionLibraryRuntime(
      &device_mgr_, Env::Default(), options.graph_def_version, options.flib_def,
      OptimizerOptions(), nullptr /* custom_kernel_creator */));

  local_flib_runtime_ = local_pflr_->GetFLR(device_->name());
  flib_runtime_ = pflr_->GetFLR(device_->name());

  // The default variable representation shape is the identity function.
  if (!options_.variable_representation_shape_fn) {
    options_.variable_representation_shape_fn =
        [](const TensorShape& shape, DataType type) { return shape; };
  }
}

XlaCompiler::~XlaCompiler() = default;

int64 XlaCompiler::NextStepId() { return next_step_id_++; }

uint64 XlaCompiler::SignatureHash::operator()(
    const std::pair<string, std::vector<Argument>>& signature) const {
  return std::hash<string>()(signature.first);
}

static Status GetFunctionBody(const NameAttrList& function,
                              FunctionLibraryRuntime* flib_runtime,
                              const FunctionBody** fbody) {
  FunctionLibraryRuntime::Handle handle;
  TF_RETURN_IF_ERROR(flib_runtime->Instantiate(
      function.name(), AttrSlice(&function.attr()), &handle));

  *fbody = flib_runtime->GetFunctionBody(handle);
  TF_RET_CHECK(*fbody);
  return Status::OK();
}

Status XlaCompiler::FindFunctionBody(const NameAttrList& function,
                                     const FunctionBody** fbody) {
  // The function may be in either the local_flib_runtime_ or flib_runtime_.
  // Look up the function in local first and if it is not found then look up the
  // function in flib_runtime_.
  auto status = GetFunctionBody(function, local_flib_runtime_, fbody);
  if (!status.ok()) {
    if (!errors::IsNotFound(status)) {
      return status;
    }
    TF_RETURN_WITH_CONTEXT_IF_ERROR(
        GetFunctionBody(function, flib_runtime_, fbody),
        "Local lookup failed with: ", status.error_message());
  }
  return Status::OK();
}

std::unique_ptr<Graph> XlaCompiler::GetGraph(const FunctionBody* fbody) {
  std::unique_ptr<Graph> graph(new Graph(options_.flib_def));
  CopyGraph(*fbody->graph, graph.get());
  OptimizerOptions opts;
  opts.set_opt_level(OptimizerOptions::L0);
  opts.set_do_common_subexpression_elimination(false);
  opts.set_do_function_inlining(true);
  opts.set_do_constant_folding(true);
  GraphOptimizer optimizer(opts);
  optimizer.Optimize(flib_runtime_, flib_runtime_->env(),
                     /*device=*/nullptr, &graph, /*shape_map=*/nullptr);

  return graph;
}

Status XlaCompiler::CompileFunction(const XlaCompiler::CompileOptions& options,
                                    const NameAttrList& function,
                                    std::vector<XlaCompiler::Argument> args,
                                    XlaCompiler::CompilationResult* result) {
  const string function_id =
      Canonicalize(function.name(), AttrSlice(&function.attr()));
  VLOG(1) << "XlaCompiler::CompileFunction " << function_id;

  auto it = cache_.find({function_id, args});
  if (it != cache_.end()) {
    *result = it->second;
    return Status::OK();
  }

  const FunctionBody* fbody;
  TF_RETURN_IF_ERROR(FindFunctionBody(function, &fbody));

  TF_RETURN_WITH_CONTEXT_IF_ERROR(
      CheckSignature(fbody->arg_types, args),
      "Signature check failure while compiling: ", function.name());

  std::unique_ptr<Graph> graph = GetGraph(fbody);

  // _Arg and _Retval nodes don't exist in the stored subgraph for the function;
  // they are added by the function body looked up.  Therefore, they don't have
  // core assignments here.
  // Attempt to assign a core to each _Retval and _Arg. Chooses the
  // lowest-numbered core that consumes the argument. We choose the
  // lowest-numbered core so the assignment is deterministic.
  for (Node* n : graph->nodes()) {
    if (StringPiece(n->type_string()) == "_Arg") {
      TF_RETURN_IF_ERROR(SetNodeShardingFromNeighbors(n, /*out_edges=*/true));
    }
  }
  // Do _Retval as a second loop, in case the retval's input is an _Arg (which
  // may have gotten a device assignment from the first loop).
  for (Node* n : graph->nodes()) {
    if (StringPiece(n->type_string()) == "_Retval") {
      TF_RETURN_IF_ERROR(SetNodeShardingFromNeighbors(n, /*out_edges=*/false));
    }
  }

  if (VLOG_IS_ON(2)) {
    VLOG(2) << "XlaCompiler::CompileFunction: "
            << dump_graph::DumpGraphToFile(
                   strings::StrCat("xla_compile_function_", function_id),
                   *graph);
  }

  VLOG(1) << "====================================================";
  TF_RETURN_IF_ERROR(
      CompileGraph(options, function_id, std::move(graph), args, result));
  VLOG(1) << "====================================================";

  cache_[{function_id, args}] = *result;
  return Status::OK();
}

// Computes the XLA shape for argument 'arg'.
Status XlaCompiler::XLAShapeForArgument(const XlaCompiler::Argument& arg,
                                        xla::Shape* xla_shape) {
  switch (arg.kind) {
    case XlaCompiler::Argument::kConstant:
      return TensorShapeToXLAShape(arg.type, arg.constant_value.shape(),
                                   xla_shape);
    case XlaCompiler::Argument::kParameter:
      return TensorShapeToXLAShape(arg.type, arg.shape, xla_shape);
    case XlaCompiler::Argument::kResource: {
      TF_RET_CHECK(arg.initialized);

      switch (arg.resource_kind) {
        case XlaResource::kVariable: {
          TensorShape representation_shape =
              options_.variable_representation_shape_fn(arg.shape, arg.type);
          return TensorShapeToXLAShape(arg.type, representation_shape,
                                       xla_shape);
        }
        case XlaResource::kTensorArray: {
          if (arg.tensor_array_size < 0) {
            return errors::InvalidArgument(
                "Negative tensor_array_size in XLAShapeForArgument");
          }
          TensorShape shape;
          shape.AddDim(arg.tensor_array_size);
          shape.AppendShape(arg.shape);
          TF_RETURN_IF_ERROR(TensorShapeToXLAShape(arg.type, shape, xla_shape));

          if (!arg.tensor_array_gradients.empty()) {
            std::vector<xla::Shape> tuple_shape(
                arg.tensor_array_gradients.size() + 1, *xla_shape);
            *xla_shape = xla::ShapeUtil::MakeTupleShape(tuple_shape);
          }
          return Status::OK();
        }
        case XlaResource::kStack: {
          if (arg.tensor_array_size < 0) {
            return errors::InvalidArgument(
                "Negative tensor_array_size in XLAShapeForArgument");
          }
          TensorShape shape;
          shape.AddDim(arg.tensor_array_size);
          shape.AppendShape(arg.shape);
          xla::Shape buffer_shape;
          TF_RETURN_IF_ERROR(
              TensorShapeToXLAShape(arg.type, shape, &buffer_shape));
          *xla_shape = xla::ShapeUtil::MakeTupleShape(
              {buffer_shape, xla::ShapeUtil::MakeShape(xla::S32, {})});
          return Status::OK();
        }

        case XlaResource::kInvalid:
          return errors::Internal(
              "Invalid resource type in XLAShapeForArgument()");
      }
    }
    case XlaCompiler::Argument::kInvalid:
      return errors::Internal("Invalid argument type in XLAShapeForArgument()");
  }
}

namespace {

Status ExecuteGraph(XlaContext* xla_context, std::unique_ptr<Graph> graph,
                    XlaCompilationDevice* device, FunctionLibraryRuntime* flib,
                    int64 step_id) {
  // Resource cleanup is a bit messy. XlaContext is a ref-countd resource; the
  // resource manager takes ownership via Create, and unrefs via Cleanup.  We
  // explicitly add a reference to ensure the refcount at entry is maintained at
  // all exit points; Create and Cleanup are always called in this function.
  //
  // The Executor requires us to use ScopedStepContainer. We wrap it in a
  // unique_ptr so we can capture the cleanup status in the end.
  xla_context->Ref();
  Status status;
  auto step_container = xla::MakeUnique<ScopedStepContainer>(
      step_id, [&status, device](const string& name) {
        status = device->resource_manager()->Cleanup(name);
      });
  TF_RETURN_IF_ERROR(device->resource_manager()->Create(
      step_container->name(), XlaContext::kXlaContextResourceName,
      xla_context));

  GraphCompiler graph_compiler(xla_context, device, graph.get(), flib,
                               step_container.get());
  TF_RETURN_IF_ERROR(graph_compiler.Compile());
  // Explicitly clean up the step container, to capture the cleanup status.
  step_container.reset();
  return Status::OK();
}

// Builds the XLA computation.
//
// `retvals` is the list of retvals produced by _Retval operators, in index
// order. `variable_map` is a map from variable ID numbers to XlaOpContext
// variable states, generated by the symbolic evaluation.
// If `return_updated_values_for_all_resources` is true, all resources will be
// included in `resource_updates`, regardless of whether their value changed.
// Sets `*num_nonconst_outputs` to the number of outputs of the `computation`.
// Sets `*resource_updates` to a description of resources whose values are
// written by the computation; the variable writes are the last
// `resource_updates.size()` return values from the computation. Each entry in
// `resource_updates` is a (input_index, type) pair, where `input_index` is the
// index of a resource variable argument to the computation, and `type` is the
// type of the final output.
Status BuildComputation(
    const std::vector<XlaCompiler::Argument>& args,
    const std::vector<int>& arg_cores,
    const std::vector<XlaExpression>& retvals,
    const std::vector<std::unique_ptr<XlaResource>>& resources,
    bool return_updated_values_for_all_resources,
    xla::ComputationBuilder* builder, xla::Computation* computation,
    int* num_computation_outputs, int* num_nonconst_outputs,
    std::vector<XlaCompiler::ResourceUpdate>* resource_updates) {
  std::vector<xla::ComputationDataHandle> elems;
  elems.reserve(retvals.size());
  for (const XlaExpression& retval : retvals) {
    if (!retval.has_constant_value()) {
      elems.push_back(retval.handle());
    }
  }
  *num_nonconst_outputs = elems.size();

  // Add return values for resources whose values have changed.
  std::vector<const XlaResource*> arg_resources;
  arg_resources.reserve(resources.size());
  for (const auto& resource : resources) {
    if (resource->arg_num() >= 0) {
      arg_resources.push_back(resource.get());
    }
  }
  std::sort(arg_resources.begin(), arg_resources.end(),
            [](const XlaResource* a, const XlaResource* b) {
              return a->arg_num() < b->arg_num();
            });

  for (const XlaResource* resource : arg_resources) {
    const XlaCompiler::Argument& arg = args[resource->arg_num()];
    const int core = arg_cores[resource->arg_num()];
    DCHECK_LT(resource->arg_num(), arg_cores.size());
    bool modified =
        resource->value().handle() != resource->initial_value().handle();
    // TensorArray gradients were modified if their values changed or there are
    // any newly created gradients.
    for (const auto& grad : resource->tensor_array_gradients()) {
      modified = modified ||
                 grad.second->value().handle() !=
                     grad.second->initial_value().handle() ||
                 arg.tensor_array_gradients.count(grad.first) == 0;
    }
    if (return_updated_values_for_all_resources || modified) {
      resource_updates->emplace_back();
      XlaCompiler::ResourceUpdate& update = resource_updates->back();
      update.input_index = resource->arg_num();
      update.type = resource->type();
      update.shape = resource->shape();
      update.modified = modified;
      for (const auto& grad : resource->tensor_array_gradients()) {
        update.tensor_array_gradients_accessed.insert(grad.first);
      }

      // Request that the value be returned on a specific core.
      xla::ScopedShardingAssignment assign_sharding(
          builder, core == -1 ? tensorflow::gtl::optional<xla::OpSharding>()
                              : xla::sharding_builder::AssignDevice(core));

      xla::ComputationDataHandle handle;
      TF_RETURN_IF_ERROR(resource->Pack(&handle, builder));

      // Since we can't change the sharding metadata of <value> as this point,
      // create a tuple/get-tuple-element combination so that sharding
      // assignment will be placed on this value, which will cause the resource
      // update to be returned from the same device that provided the resource.
      handle = builder->GetTupleElement(builder->Tuple({handle}), 0);

      elems.push_back(handle);
    }
  }

  *num_computation_outputs = elems.size();

  // Builds the XLA computation.
  builder->Tuple(elems);
  xla::StatusOr<xla::Computation> computation_status = builder->Build();
  if (!computation_status.ok()) {
    return computation_status.status();
  }
  *computation = computation_status.ConsumeValueOrDie();
  return Status::OK();
}

}  // namespace

// Builds XLA computations for each of the arguments to the computation.
// `args` are the arguments to the computation.
Status XlaCompiler::BuildArguments(
    const Graph& graph, const std::vector<XlaCompiler::Argument>& args,
    bool use_tuple_arg, xla::ComputationBuilder* builder, XlaContext* context,
    std::vector<int>* arg_cores, std::vector<XlaExpression>* arg_expressions,
    std::vector<int>* input_mapping, std::vector<xla::Shape>* input_shapes,
    bool is_entry_computation) {
  arg_expressions->resize(args.size());
  *arg_cores = std::vector<int>(args.size(), -1);

  // Argument numbers of arguments and resources that are to be passed to the
  // XLA computation as runtime parameters.
  input_mapping->clear();
  input_mapping->reserve(args.size());
  std::vector<int> resources;
  resources.reserve(args.size());

  // Fills in constant arguments, and computes non-constant argument order.
  for (std::vector<XlaCompiler::Argument>::size_type i = 0; i < args.size();
       ++i) {
    const XlaCompiler::Argument& arg = args[i];
    XlaExpression& arg_expression = (*arg_expressions)[i];
    switch (arg.kind) {
      case XlaCompiler::Argument::kResource:
        TF_RET_CHECK(arg.resource_kind != XlaResource::kInvalid);
        // TODO(phawkins): this code assumes that resource arguments do not
        // alias.
        XlaResource* resource;
        TF_RETURN_IF_ERROR(context->CreateResource(
            arg.resource_kind, i, arg.name, arg.type, arg.shape,
            xla::ComputationDataHandle(),
            /*tensor_array_size=*/arg.tensor_array_size,
            /*tensor_array_gradients=*/arg.tensor_array_gradients, &resource));
        arg_expression.set_resource(resource);
        if (arg.initialized) {
          resources.push_back(i);
        }
        break;
      case XlaCompiler::Argument::kParameter: {
        input_mapping->push_back(i);
        break;
      }
      case XlaCompiler::Argument::kConstant:
        arg_expression.set_constant_value(arg.constant_value);
        break;
      case XlaCompiler::Argument::kInvalid:
        return errors::Internal("Unreachable case in BuildArguments()");
    }
  }

  // Append parameters containing variable values after the other runtime
  // parameters.
  input_mapping->insert(input_mapping->end(), resources.begin(),
                        resources.end());
  if (input_mapping->empty()) {
    return Status::OK();
  }

  std::vector<xla::Shape> arg_shapes(input_mapping->size());
  for (std::vector<int>::size_type i = 0; i < input_mapping->size(); ++i) {
    // Computes the shapes of non-constant arguments.
    TF_RETURN_IF_ERROR(
        XLAShapeForArgument(args[(*input_mapping)[i]], &arg_shapes[i]));
  }

  if (use_tuple_arg) {
    input_shapes->push_back(xla::ShapeUtil::MakeTupleShape(arg_shapes));
  } else {
    *input_shapes = arg_shapes;
  }

  // Use the _Arg nodes in the graph to resolve core assignments.
  for (const Node* n : graph.nodes()) {
    if (StringPiece(n->type_string()) != "_Arg") continue;
    int index;
    TF_RETURN_IF_ERROR(GetNodeAttr(n->attrs(), "index", &index));
    TF_RET_CHECK(index >= 0 && index < args.size())
        << "_Arg out of bounds: " << index << " vs " << args.size();
    TF_ASSIGN_OR_RETURN(
        auto sharding,
        ParseShardingFromDevice(*n, std::numeric_limits<int32>::max()));
    if (sharding.has_value()) {
      TF_RET_CHECK(sharding.value().type() ==
                   xla::OpSharding::Type::OpSharding_Type_MAXIMAL);
      const int core = sharding.value().tile_assignment_devices(0);
      if ((*arg_cores)[index] == -1 || core < (*arg_cores)[index]) {
        (*arg_cores)[index] = core;
      }
    }
  }

  // Build parameter handles for non-constant arguments.
  std::vector<xla::ComputationDataHandle> arg_handles(input_mapping->size());
  if (use_tuple_arg) {
    xla::ComputationDataHandle tuple;
    if (is_entry_computation) {
      xla::OpSharding tuple_sharding;
      tuple_sharding.set_type(xla::OpSharding::Type::OpSharding_Type_TUPLE);
      for (int64 parameter : *input_mapping) {
        const int core = (*arg_cores)[parameter];
        const int root_device = 0;
        *tuple_sharding.add_tuple_shardings() =
            core == -1 ? xla::sharding_builder::AssignDevice(root_device)
                       : xla::sharding_builder::AssignDevice(core);
      }
      xla::ScopedShardingAssignment assign_tuple_sharding(builder,
                                                          tuple_sharding);
      tuple = builder->Parameter(0, (*input_shapes)[0], "arg_tuple");
    } else {
      tuple = builder->Parameter(0, (*input_shapes)[0], "arg_tuple");
    }
    for (std::vector<int>::size_type i = 0; i < input_mapping->size(); ++i) {
      const int core = (*arg_cores)[input_mapping->at(i)];
      xla::ScopedShardingAssignment assign_sharding(
          builder, core == -1 ? tensorflow::gtl::optional<xla::OpSharding>()
                              : xla::sharding_builder::AssignDevice(core));
      arg_handles[i] = builder->GetTupleElement(tuple, i);
    }
  } else {
    for (std::vector<int>::size_type i = 0; i < input_mapping->size(); ++i) {
      const int core = (*arg_cores)[input_mapping->at(i)];
      xla::ScopedShardingAssignment assign_sharding(
          builder, core == -1 ? tensorflow::gtl::optional<xla::OpSharding>()
                              : xla::sharding_builder::AssignDevice(core));
      arg_handles[i] =
          builder->Parameter(i, (*input_shapes)[i], strings::StrCat("arg", i));
    }
  }

  // Fill in the handles in non-constant arguments.
  VLOG(2) << "XLA computation inputs:";
  for (std::vector<int>::size_type i = 0; i < input_mapping->size(); ++i) {
    const XlaCompiler::Argument& arg = args[input_mapping->at(i)];
    VLOG(2) << "  XLA arg " << i
            << " shape: " << xla::ShapeUtil::HumanString(arg_shapes[i])
            << " name: " << arg.name << " TF arg " << input_mapping->at(i);
    XlaExpression& arg_expression = (*arg_expressions)[input_mapping->at(i)];
    switch (arg.kind) {
      case XlaCompiler::Argument::kResource: {
        TF_RET_CHECK(arg.initialized);
        XlaResource* resource = arg_expression.resource();
        TF_RETURN_IF_ERROR(resource->SetFromPack(arg.tensor_array_gradients,
                                                 arg_handles[i], builder));
        VLOG(2) << "    resource: num_gradients: "
                << arg.tensor_array_gradients.size();
        break;
      }
      case XlaCompiler::Argument::kParameter:
        arg_expression.set_handle(arg_handles[i]);
        break;
      case XlaCompiler::Argument::kConstant:
      case XlaCompiler::Argument::kInvalid:
        return errors::Internal("Unreachable case in BuildArguments()");
    }
  }

  return Status::OK();
}

Status XlaCompiler::CompileGraph(const XlaCompiler::CompileOptions& options,
                                 string const& name,
                                 std::unique_ptr<Graph> graph,
                                 const std::vector<XlaCompiler::Argument>& args,
                                 CompilationResult* result) {
  VLOG(1) << "Executing graph symbolically to populate ComputationBuilder.";

  if (VLOG_IS_ON(2)) {
    VLOG(2) << "XlaCompiler::CompileGraph: "
            << dump_graph::DumpGraphToFile(
                   strings::StrCat("xla_compile_graph_", name), *graph);
  }

  // Report the error here if initialization failed.
  TF_RETURN_IF_ERROR(initialization_status_);

  // Converts Tensorflow's graph control-flow constructs into functional
  // control-flow that can be compiled into XLA code.
  TF_RETURN_IF_ERROR(
      FunctionalizeControlFlow(graph.get(), local_flib_def_.get()));

  xla::ComputationBuilder builder(client(), name);
  XlaContext* context =
      new XlaContext(this, &builder, options_.allow_cpu_custom_calls,
                     options.resolve_compile_time_constants,
                     &options_.variable_representation_shape_fn);
  core::ScopedUnref context_unref(context);

  std::vector<XlaExpression> arg_expressions;
  std::vector<int> arg_cores;
  TF_RETURN_IF_ERROR(
      BuildArguments(*graph, args, options.use_tuple_arg, &builder, context,
                     &arg_cores, &arg_expressions, &result->input_mapping,
                     &result->xla_input_shapes, options.is_entry_computation));
  context->set_args(std::move(arg_expressions));

  TF_RETURN_IF_ERROR(ExecuteGraph(context, std::move(graph), device_,
                                  flib_runtime_, NextStepId()));

  int num_nonconst_outputs;
  int num_computation_outputs;
  result->computation = std::make_shared<xla::Computation>();
  TF_RETURN_IF_ERROR(BuildComputation(
      args, arg_cores, context->retvals(), context->resources(),
      options.return_updated_values_for_all_resources, &builder,
      result->computation.get(), &num_computation_outputs,
      &num_nonconst_outputs, &result->resource_updates));

  VLOG(2) << "Outputs: total: " << context->retvals().size()
          << " nonconstant: " << num_nonconst_outputs;
  result->outputs.resize(context->retvals().size());
  for (std::vector<XlaExpression>::size_type i = 0;
       i < context->retvals().size(); ++i) {
    const XlaExpression& retval = context->retvals()[i];
    if (retval.has_constant_value()) {
      OutputDescription& output = result->outputs[i];
      output.shape = retval.constant_value().shape();
      output.is_constant = true;
      output.constant_value = retval.constant_value();
    }
  }

  // Compute the output shapes, if there is a computation with non-constant
  // outputs.
  auto computation_shape = client()->GetComputationShape(*result->computation);
  if (!computation_shape.ok()) {
    return computation_shape.status();
  }

  result->xla_output_shape.Swap(
      computation_shape.ValueOrDie()->mutable_result());
  VLOG(2) << "XLA output shape: "
          << xla::ShapeUtil::HumanString(result->xla_output_shape);

  // Tensorflow expects a major-to-minor order of results.
  xla::LayoutUtil::SetToDefaultLayout(&result->xla_output_shape);

  // Converts the output shapes to TensorShapes.
  int computation_output = 0;
  for (std::vector<XlaExpression>::size_type i = 0;
       i < context->retvals().size(); ++i) {
    const XlaExpression& retval = context->retvals()[i];
    if (!retval.has_constant_value()) {
      TF_RET_CHECK(computation_output < num_computation_outputs)
          << "Computation has more outputs than expected";
      OutputDescription& output = result->outputs[i];
      output.is_constant = false;
      TF_RETURN_IF_ERROR(XLAShapeToTensorShape(
          xla::ShapeUtil::GetTupleElementShape(result->xla_output_shape,
                                               computation_output),
          &output.shape));
      ++computation_output;
    }
  }
  return Status::OK();
}

Status XlaCompiler::GetChannelHandle(const string& key,
                                     xla::ChannelHandle* channel) {
  auto result = channels_.emplace(key, xla::ChannelHandle());
  if (result.second) {
    TF_ASSIGN_OR_RETURN(result.first->second, client()->CreateChannelHandle());
  }
  *channel = result.first->second;
  VLOG(1) << "Channel: " << key << " " << channel->DebugString();
  return Status::OK();
}

}  // namespace tensorflow
