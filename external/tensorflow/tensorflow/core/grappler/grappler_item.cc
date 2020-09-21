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

#include "tensorflow/core/grappler/grappler_item.h"

#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "tensorflow/core/framework/attr_value.pb.h"
#include "tensorflow/core/framework/node_def.pb.h"
#include "tensorflow/core/grappler/op_types.h"
#include "tensorflow/core/grappler/utils.h"

namespace tensorflow {
namespace grappler {

GrapplerItem::GrapplerItem(const GrapplerItem& other, GraphDef&& graphDef) {
  id = other.id;
  feed = other.feed;
  fetch = other.fetch;
  init_ops = other.init_ops;
  expected_init_time = other.expected_init_time;
  save_op = other.save_op;
  restore_op = other.restore_op;
  save_restore_loc_tensor = other.save_restore_loc_tensor;
  queue_runners = other.queue_runners;
  graph.Swap(&graphDef);
}

std::vector<const NodeDef*> GrapplerItem::MainOpsFanin() const {
  return ComputeTransitiveFanin(graph, fetch);
}

std::vector<const NodeDef*> GrapplerItem::EnqueueOpsFanin() const {
  std::vector<string> enqueue_ops;
  for (const auto& queue_runner : queue_runners) {
    for (const string& enqueue_op : queue_runner.enqueue_op_name()) {
      enqueue_ops.push_back(enqueue_op);
    }
  }
  return ComputeTransitiveFanin(graph, enqueue_ops);
}

std::vector<const NodeDef*> GrapplerItem::InitOpsFanin() const {
  return ComputeTransitiveFanin(graph, init_ops);
}

std::vector<const NodeDef*> GrapplerItem::MainVariables() const {
  std::vector<const NodeDef*> fanin = ComputeTransitiveFanin(graph, init_ops);
  std::vector<const NodeDef*> vars;
  for (const NodeDef* node : fanin) {
    if (IsVariable(*node)) {
      vars.push_back(node);
    }
  }
  return vars;
}

std::unordered_set<string> GrapplerItem::NodesToPreserve() const {
  std::unordered_set<string> result;
  for (const string& f : fetch) {
    VLOG(1) << "Add fetch " << f;
    result.insert(NodeName(f));
  }
  for (const auto& f : feed) {
    VLOG(1) << "Add feed " << f.first;
    result.insert(NodeName(f.first));
  }
  for (const auto& node : init_ops) {
    result.insert(NodeName(node));
  }
  if (!save_op.empty()) {
    result.insert(NodeName(save_op));
  }
  if (!restore_op.empty()) {
    result.insert(NodeName(restore_op));
  }
  if (!save_restore_loc_tensor.empty()) {
    result.insert(NodeName(save_restore_loc_tensor));
  }

  for (const auto& queue_runner : queue_runners) {
    for (const string& enqueue_op : queue_runner.enqueue_op_name()) {
      result.insert(NodeName(enqueue_op));
    }
    if (!queue_runner.close_op_name().empty()) {
      result.insert(NodeName(queue_runner.close_op_name()));
    }
    if (!queue_runner.cancel_op_name().empty()) {
      result.insert(NodeName(queue_runner.cancel_op_name()));
    }
  }
  return result;
}

std::vector<const NodeDef*> ComputeTransitiveFanin(
    const GraphDef& graph, const std::vector<string>& terminal_nodes) {
  bool ill_formed = false;
  std::vector<const NodeDef*> result =
      ComputeTransitiveFanin(graph, terminal_nodes, &ill_formed);
  CHECK(!ill_formed);
  return result;
}

std::vector<const NodeDef*> ComputeTransitiveFanin(
    const GraphDef& graph, const std::vector<string>& terminal_nodes,
    bool* ill_formed) {
  *ill_formed = false;
  std::unordered_map<string, const NodeDef*> name_to_node;
  std::unordered_map<string, const NodeDef*> name_to_send;
  for (const auto& node : graph.node()) {
    name_to_node[node.name()] = &node;
    if (node.op() == "_Send") {
      const auto& attr = node.attr();
      name_to_send[attr.at("tensor_name").s()] = &node;
    }
  }

  std::vector<const NodeDef*> queue;
  for (const string& root : terminal_nodes) {
    const NodeDef* node = name_to_node[NodeName(root)];
    if (!node) {
      *ill_formed = true;
      VLOG(2) << "ComputeTransitiveFanin: problem with root node: " << root;
      return {};
    }
    queue.push_back(node);
  }

  std::vector<const NodeDef*> result;
  std::unordered_set<const NodeDef*> visited;

  while (!queue.empty()) {
    const NodeDef* node = queue.back();
    queue.pop_back();
    if (!visited.insert(node).second) {
      // The node has already been visited.
      continue;
    }
    result.push_back(node);
    for (const string& input : node->input()) {
      const NodeDef* in = name_to_node[NodeName(input)];
      if (!in) {
        VLOG(2) << "ComputeTransitiveFanin: problem with node: " << input;
        *ill_formed = true;
        return {};
      }
      queue.push_back(in);
    }
    if (node->op() == "_Recv") {
      const auto& attr = node->attr();
      const NodeDef* send = name_to_send[attr.at("tensor_name").s()];
      if (send) {
        queue.push_back(send);
      }
      // Subgraph after partitioning may have either _Send or _Recv, not both.
      // So, we do not set ill_formed for missing _Send.
    }
  }
  return result;
}

}  // end namespace grappler
}  // end namespace tensorflow
