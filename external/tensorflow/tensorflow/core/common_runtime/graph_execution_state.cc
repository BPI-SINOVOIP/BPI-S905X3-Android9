/* Copyright 2016 The TensorFlow Authors. All Rights Reserved.

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

#include "tensorflow/core/common_runtime/graph_execution_state.h"

#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "tensorflow/core/common_runtime/device.h"
#include "tensorflow/core/common_runtime/optimization_registry.h"
#include "tensorflow/core/common_runtime/placer.h"
#include "tensorflow/core/framework/graph.pb_text.h"
#include "tensorflow/core/framework/graph_def_util.h"
#include "tensorflow/core/framework/node_def.pb.h"
#include "tensorflow/core/framework/versions.pb.h"
#include "tensorflow/core/graph/graph.h"
#include "tensorflow/core/graph/graph_constructor.h"
#include "tensorflow/core/graph/subgraph.h"
#include "tensorflow/core/graph/tensor_id.h"
#include "tensorflow/core/graph/validate.h"
#include "tensorflow/core/lib/core/errors.h"
#include "tensorflow/core/lib/core/status.h"
#include "tensorflow/core/lib/strings/stringprintf.h"
#include "tensorflow/core/platform/logging.h"
#include "tensorflow/core/platform/types.h"
#include "tensorflow/core/util/device_name_utils.h"
#include "tensorflow/core/util/util.h"

#ifndef IS_MOBILE_PLATFORM
#include "tensorflow/core/grappler/clusters/utils.h"
#include "tensorflow/core/grappler/clusters/virtual_cluster.h"
#include "tensorflow/core/grappler/grappler_item.h"
#include "tensorflow/core/grappler/optimizers/meta_optimizer.h"
#endif  // IS_MOBILE_PLATFORM

namespace tensorflow {

GraphExecutionState::GraphExecutionState(
    GraphDef* graph_def, const GraphExecutionStateOptions& options)
    : stateful_placements_(options.stateful_placements),
      device_set_(options.device_set),
      session_options_(options.session_options),
      flib_def_(new FunctionLibraryDefinition(OpRegistry::Global(),
                                              graph_def->library())),
      graph_(nullptr) {
  // NOTE(mrry): GraphDef does not have a move constructor, so we pass
  // a non-const pointer and use `Swap()` to transfer the contents
  // without copying.
  original_graph_def_.Swap(graph_def);
  // TODO(mrry): Publish placement visualizations or handle the log
  // placement option.
}

GraphExecutionState::~GraphExecutionState() {
  node_name_to_cost_id_map_.clear();
  delete graph_;
}

/* static */ Status GraphExecutionState::MakeForBaseGraph(
    GraphDef* graph_def, const GraphExecutionStateOptions& options,
    std::unique_ptr<GraphExecutionState>* out_state) {
  std::unique_ptr<GraphExecutionState> ret(
      new GraphExecutionState(graph_def, options));

  TF_RETURN_IF_ERROR(
      AddDefaultAttrsToGraphDef(&ret->original_graph_def_, *ret->flib_def_, 0));
  // TODO(mrry): Refactor InitBaseGraph() so that we don't have to
  // pass an empty BuildGraphOptions (that isn't going to be used when
  // place_pruned_graph is false).
  if (!ret->session_options_->config.graph_options().place_pruned_graph()) {
    TF_RETURN_IF_ERROR(ret->InitBaseGraph(BuildGraphOptions()));
  }
  *out_state = std::move(ret);
  return Status::OK();
}

/* static */ Status GraphExecutionState::MakeForPrunedGraph(
    const FunctionDefLibrary& func_def_lib,
    const GraphExecutionStateOptions& options, const GraphDef& graph_def,
    const BuildGraphOptions& subgraph_options,
    std::unique_ptr<GraphExecutionState>* out_state,
    std::unique_ptr<ClientGraph>* out_client_graph) {
  DCHECK(options.session_options->config.graph_options().place_pruned_graph());
  // NOTE(mrry): This makes a copy of `graph_def`, which is
  // regrettable. We could make `GraphDef` objects sharable between
  // execution states to optimize pruned graph execution, but since
  // this case is primarily used for interactive sessions, we make the
  // bet that graph construction is not performance-critical. (Note
  // also that the previous version used `Extend()`, which is strictly
  // more expensive than copying a `GraphDef`.)
  GraphDef temp(graph_def);
  std::unique_ptr<GraphExecutionState> ret(
      new GraphExecutionState(&temp, options));
  TF_RETURN_IF_ERROR(
      AddDefaultAttrsToGraphDef(&ret->original_graph_def_, *ret->flib_def_, 0));
  TF_RETURN_IF_ERROR(ret->InitBaseGraph(subgraph_options));
  TF_RETURN_IF_ERROR(ret->BuildGraph(subgraph_options, out_client_graph));
  *out_state = std::move(ret);
  return Status::OK();
}

Status GraphExecutionState::Extend(
    const GraphDef& extension_def,
    std::unique_ptr<GraphExecutionState>* out) const {
  GraphDef gdef;

  // 1. Copy the function library.
  TF_RETURN_IF_ERROR(flib_def_->AddLibrary(extension_def.library()));
  *gdef.mutable_library() = flib_def_->ToProto();

  // 2. Build an index of the new node names.
  std::unordered_set<string> new_names;
  for (const NodeDef& node : extension_def.node()) {
    new_names.insert(node.name());
  }

  // 3. Add the non-duplicates from the old graph to the new graph.
  //    Return an error if the same node name appears in both the
  //    old graph and the extension.
  for (const NodeDef& node : original_graph_def_.node()) {
    if (new_names.count(node.name()) == 0) {
      *gdef.add_node() = node;
    } else {
      return errors::InvalidArgument(tensorflow::strings::Printf(
          "GraphDef argument to Extend includes node '%s', which was created "
          "by a previous call to Create or Extend in this session.",
          node.name().c_str()));
    }
  }

  // 4. Merge the versions field.
  int old_node_size = gdef.node_size();
  gdef.mutable_node()->MergeFrom(extension_def.node());
  TF_RETURN_IF_ERROR(
      AddDefaultAttrsToGraphDef(&gdef, *flib_def_, old_node_size));
  // Merge versions
  if (gdef.has_versions()) {
    if (gdef.versions().producer() != extension_def.versions().producer()) {
      return errors::InvalidArgument(
          "Can't extend GraphDef at version ", gdef.versions().producer(),
          " with graph at version ", extension_def.versions().producer());
    }
    VersionDef* versions = gdef.mutable_versions();
    versions->set_min_consumer(std::max(
        versions->min_consumer(), extension_def.versions().min_consumer()));
    if (extension_def.versions().bad_consumers_size()) {
      // Add new bad_consumers that aren't already marked bad.
      //
      // Note: This implementation is quadratic time if there are many calls to
      // ExtendLocked with many bad consumers.  Since this is unlikely, and
      // fixing it would require data structures outside of this routine,
      // quadratic time it is.
      auto* bad_consumers = versions->mutable_bad_consumers();
      const std::unordered_set<int> existing(bad_consumers->begin(),
                                             bad_consumers->end());
      for (const int v : extension_def.versions().bad_consumers()) {
        if (existing.find(v) == existing.end()) {
          bad_consumers->Add(v);
        }
      }
    }

  } else {
    gdef.mutable_versions()->CopyFrom(extension_def.versions());
  }

  // 5. Validate that the final graphdef is valid.
  if (gdef.versions().producer() >= 5) {
    // Validate the graph: we assume that merging two valid graphs
    // should maintain graph validity.
    TF_RETURN_IF_ERROR(graph::ValidateGraphDef(gdef, *flib_def_));
  }

  // 6. Add the extension.
  GraphExecutionStateOptions combined_options;
  combined_options.device_set = device_set_;
  combined_options.session_options = session_options_;
  combined_options.stateful_placements = stateful_placements_;

  // NOTE(mrry): `gdef` is no longer valid after the constructor
  // executes.
  std::unique_ptr<GraphExecutionState> new_execution_state(
      new GraphExecutionState(&gdef, combined_options));

  TF_RETURN_IF_ERROR(AddDefaultAttrsToGraphDef(
      &new_execution_state->original_graph_def_, *flib_def_, 0));
  if (!session_options_->config.graph_options().place_pruned_graph()) {
    // TODO(mrry): Refactor InitBaseGraph() so that we don't have to
    // pass an empty BuildGraphOptions (that isn't going to be used
    // when place_pruned_graph is false).
    TF_RETURN_IF_ERROR(new_execution_state->InitBaseGraph(BuildGraphOptions()));
  }
  *out = std::move(new_execution_state);

  // TODO(mrry): This is likely to be used for non-throughput-sensitive
  // interactive workloads, but in future we may want to transfer other
  // parts of the placement and/or cost model.
  return Status::OK();
}

void GraphExecutionState::SaveStatefulNodes(Graph* graph) {
  for (Node* n : graph->nodes()) {
    if (n->op_def().is_stateful()) {
      VLOG(2) << "Saving " << n->DebugString();
      stateful_placements_[n->name()] = n->assigned_device_name();
    }
  }
}

void GraphExecutionState::RestoreStatefulNodes(Graph* graph) {
  for (Node* n : graph->nodes()) {
    if (n->op_def().is_stateful()) {
      auto iter = stateful_placements_.find(n->name());
      if (iter != stateful_placements_.end()) {
        n->set_assigned_device_name(iter->second);
        VLOG(2) << "Restored " << n->DebugString();
      }
    }
  }
}

Status GraphExecutionState::InitBaseGraph(const BuildGraphOptions& options) {
  const GraphDef* graph_def = &original_graph_def_;

  std::unique_ptr<Graph> new_graph(new Graph(OpRegistry::Global()));
  GraphConstructorOptions opts;
  TF_RETURN_IF_ERROR(ConvertGraphDefToGraph(opts, *graph_def, new_graph.get()));
  for (const Node* n : new_graph->nodes()) {
    VLOG(2) << "Mapping " << n->name() << " to " << n->cost_id();
    node_name_to_cost_id_map_[n->name()] = n->cost_id();
  }
  if (session_options_ &&
      session_options_->config.graph_options().place_pruned_graph()) {
    // Rewrite the graph before placement.
    rewrite_metadata_.reset(new subgraph::RewriteGraphMetadata);
    TF_RETURN_IF_ERROR(subgraph::RewriteGraphForExecution(
        new_graph.get(), options.feed_endpoints, options.fetch_endpoints,
        options.target_nodes, device_set_->client_device()->attributes(),
        options.use_function_convention, rewrite_metadata_.get()));
  }

  // Save stateful placements before placing.
  RestoreStatefulNodes(new_graph.get());

  GraphOptimizationPassOptions optimization_options;
  optimization_options.session_options = session_options_;
  optimization_options.graph = &new_graph;
  optimization_options.flib_def = flib_def_.get();
  optimization_options.device_set = device_set_;

  TF_RETURN_IF_ERROR(OptimizationPassRegistry::Global()->RunGrouping(
      OptimizationPassRegistry::PRE_PLACEMENT, optimization_options));

  Placer placer(new_graph.get(), device_set_, session_options_);
  // TODO(mrry): Consider making the Placer cancelable.
  TF_RETURN_IF_ERROR(placer.Run());

  TF_RETURN_IF_ERROR(OptimizationPassRegistry::Global()->RunGrouping(
      OptimizationPassRegistry::POST_PLACEMENT, optimization_options));

  SaveStatefulNodes(new_graph.get());
  graph_ = new_graph.release();
  return Status::OK();
}

Status GraphExecutionState::OptimizeGraph(
    const BuildGraphOptions& options, std::unique_ptr<Graph>* optimized_graph) {
#ifndef IS_MOBILE_PLATFORM
  if (session_options_->config.graph_options().place_pruned_graph()) {
    return errors::InvalidArgument("Can't optimize a pruned graph");
  }

  const RewriterConfig& rewrite_options =
      session_options_->config.graph_options().rewrite_options();

  if (grappler::MetaOptimizerEnabled(rewrite_options)) {
    // Adding this functionality in steps. The first step is to make sure
    // we don't break dependencies. The second step will be to turn the
    // functionality on by default.
    grappler::GrapplerItem item;
    item.id = "tf_graph";
    graph_->ToGraphDef(&item.graph);

    item.fetch = options.fetch_endpoints;
    item.fetch.insert(item.fetch.end(), options.target_nodes.begin(),
                      options.target_nodes.end());

    if (!options.feed_endpoints.empty()) {
      std::unordered_set<string> feeds;
      for (const string& feed : options.feed_endpoints) {
        TensorId id = ParseTensorName(feed);
        if (id.second != 0) {
          return errors::InvalidArgument("Unsupported feed: ", feed);
        }
        feeds.insert(id.first.ToString());
      }
      for (const NodeDef& node : original_graph_def_.node()) {
        if (feeds.find(node.name()) == feeds.end()) {
          continue;
        }
        if (node.attr().count("dtype") == 0 ||
            node.attr().count("shape") == 0) {
          return errors::InvalidArgument("Missing node shape or type");
        }
        TensorShapeProto shape_proto(node.attr().at("shape").shape());
        // If the shape of the placeholder value is only partially known, we're
        // free to use any dimension we want to feed the placeholder. We choose
        // 1 to minimize the memory impact. Note that this only matters if an
        // optimizer choose to run the graph to build its cost model, which
        // doesn't happen (yet)
        if (shape_proto.unknown_rank()) {
          shape_proto.set_unknown_rank(false);
        }
        for (auto& dim : *shape_proto.mutable_dim()) {
          if (dim.size() < 0) {
            dim.set_size(1);
          }
        }
        TensorShape shape(shape_proto);
        DataType type = node.attr().at("dtype").type();
        Tensor fake_input(type, shape);
        item.feed.emplace_back(node.name(), fake_input);
      }
    }

    std::unordered_map<string, DeviceProperties> device_map;
    Device* cpu_device = nullptr;
    for (const auto& device : device_set_->devices()) {
      DeviceProperties props = grappler::GetDeviceInfo(device->parsed_name());
      if (props.type() == "UNKNOWN") {
        continue;
      }
      device_map[device->name()] = props;
      if (device->parsed_name().id == 0 &&
          StringPiece(device->parsed_name().type) == "CPU" &&
          device->GetAllocator(AllocatorAttributes()) != nullptr) {
        cpu_device = device;
      }
    }
    grappler::VirtualCluster cluster(device_map);
    GraphDef new_graph;
    TF_RETURN_IF_ERROR(grappler::RunMetaOptimizer(
        item, rewrite_options, cpu_device, &cluster, &new_graph));
    GraphConstructorOptions opts;
    opts.allow_internal_ops = true;
    optimized_graph->reset(new Graph(OpRegistry::Global()));
    TF_RETURN_IF_ERROR(
        ConvertGraphDefToGraph(opts, new_graph, optimized_graph->get()));
    // The graph conversion sets the requested device names but not the assigned
    // device names. However, since at this point the graph is placed TF expects
    // an assigned device name for every node. Therefore we copy the requested
    // device into the assigned device field.
    for (Node* node : optimized_graph->get()->nodes()) {
      node->set_assigned_device_name(node->requested_device());
    }
    return Status::OK();
  } else {
    return errors::InvalidArgument("Meta Optimizer disabled");
  }
#else
  return errors::InvalidArgument("Mobile platforms not supported");
#endif  // IS_MOBILE_PLATFORM
}

Status GraphExecutionState::BuildGraph(const BuildGraphOptions& options,
                                       std::unique_ptr<ClientGraph>* out) {
  VLOG(1) << "BuildGraph";
  if (!graph_) {
    // It is only valid to call this method directly when the original graph
    // was created with the option `place_pruned_graph == false`.
    return errors::Internal(
        "Attempted to prune a graph that has not been fully initialized.");
  }

  std::unique_ptr<Graph> ng;
  Status s = OptimizeGraph(options, &ng);
  if (!s.ok()) {
    // Simply copy the original graph if we couldn't optimize it.
    ng.reset(new Graph(flib_def_.get()));
    CopyGraph(*graph_, ng.get());
  }

  subgraph::RewriteGraphMetadata rewrite_metadata;
  if (session_options_ == nullptr ||
      !session_options_->config.graph_options().place_pruned_graph()) {
    // Extract the subset of the graph that needs to be run, adding feed/fetch
    // ops as needed.
    TF_RETURN_IF_ERROR(subgraph::RewriteGraphForExecution(
        ng.get(), options.feed_endpoints, options.fetch_endpoints,
        options.target_nodes, device_set_->client_device()->attributes(),
        options.use_function_convention, &rewrite_metadata));
  } else {
    // This GraphExecutionState represents a graph that was
    // pruned when this was constructed, so we copy the metadata from
    // a member variable.
    CHECK(rewrite_metadata_);
    rewrite_metadata = *rewrite_metadata_;
  }

  CHECK_EQ(options.feed_endpoints.size(), rewrite_metadata.feed_types.size());
  CHECK_EQ(options.fetch_endpoints.size(), rewrite_metadata.fetch_types.size());

  // Make a fresh copy of the function library for the client graph.
  std::unique_ptr<FunctionLibraryDefinition> flib(
      new FunctionLibraryDefinition(*flib_def_));

  // TODO(andydavis): Clarify optimization pass requirements around CostModel.
  GraphOptimizationPassOptions optimization_options;
  optimization_options.session_options = session_options_;
  optimization_options.graph = &ng;
  optimization_options.flib_def = flib.get();
  optimization_options.device_set = device_set_;

  TF_RETURN_IF_ERROR(OptimizationPassRegistry::Global()->RunGrouping(
      OptimizationPassRegistry::POST_REWRITE_FOR_EXEC, optimization_options));

  // Copy the extracted graph in order to make its node ids dense,
  // since the local CostModel used to record its stats is sized by
  // the largest node id.
  std::unique_ptr<ClientGraph> dense_copy(
      new ClientGraph(std::move(flib), rewrite_metadata.feed_types,
                      rewrite_metadata.fetch_types));
  CopyGraph(*ng, &dense_copy->graph);

  // TODO(vrv): We should check invariants of the graph here.

  *out = std::move(dense_copy);
  return Status::OK();
}

}  // namespace tensorflow
