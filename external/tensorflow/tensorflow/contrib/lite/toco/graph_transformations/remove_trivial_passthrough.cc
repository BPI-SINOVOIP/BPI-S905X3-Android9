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
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "tensorflow/contrib/lite/toco/graph_transformations/graph_transformations.h"
#include "tensorflow/contrib/lite/toco/model.h"
#include "tensorflow/contrib/lite/toco/tooling_util.h"
#include "tensorflow/core/platform/logging.h"

namespace toco {
namespace {

// Reroute all edges involving a given discardable array to another
// array instead. from_array is assumed to be discardable, and consequently
// this only updates operator edges (since discardable arrays only
// appear there, and not e.g. in model flags).
void RerouteEdges(const string& from_array, const string& to_array,
                  Model* model) {
  for (const auto& op : model->operators) {
    for (auto& output : op->outputs) {
      if (output == from_array) {
        output = to_array;
      }
    }
    for (auto& input : op->inputs) {
      if (input == from_array) {
        input = to_array;
      }
    }
  }
}

}  // namespace

bool RemoveTrivialPassthroughOp(GraphTransformation* transformation,
                                Model* model, std::size_t op_index) {
  const auto passthru_it = model->operators.begin() + op_index;
  auto* passthru_op = passthru_it->get();
  CHECK_EQ(passthru_op->outputs.size(), 1);
  CHECK_GE(passthru_op->inputs.size(), 1);
  int count_nonconstant_input_arrays = 0;
  // We call 'main input' the unique nonconstant input array if there is one,
  // or else the 0-th input.
  int main_input_array_index = 0;
  for (int i = 0; i < passthru_op->inputs.size(); i++) {
    if (!model->GetArray(passthru_op->inputs[i]).buffer) {
      count_nonconstant_input_arrays++;
      main_input_array_index = i;
    }
  }

  const string main_input_name = passthru_op->inputs[main_input_array_index];
  const string output_name = passthru_op->outputs[0];

  // Build the list of all input and output arrays of the passthrough node
  // that we are considering removing. Any of these arrays is a candidate
  // for being removed as well, if nothing else references it. Doing that
  // arrays-removal together with the passthrough-node-removal proved too
  // error-prone.
  std::vector<string> removal_candidates;
  for (const string& input : passthru_op->inputs) {
    removal_candidates.push_back(input);
  }
  removal_candidates.push_back(output_name);

  if (IsDiscardableArray(*model, output_name)) {
    transformation->AddMessageF(
        "Removing %s, keeping its non-constant input array",
        LogName(*passthru_op));
    for (const string& input : passthru_op->inputs) {
      if (IsDiscardableArray(*model, input) && input != main_input_name &&
          CountOpsWithInput(*model, input) == 1) {
      }
    }
    RerouteEdges(output_name, main_input_name, model);
  } else if (IsDiscardableArray(*model, main_input_name)) {
    transformation->AddMessageF("Removing %s, keeping its output array",
                                LogName(*passthru_op));
    for (const string& input : passthru_op->inputs) {
      if (IsDiscardableArray(*model, input) &&
          (input == main_input_name || CountOpsWithInput(*model, input) == 1)) {
      }
    }
    RerouteEdges(main_input_name, output_name, model);
  } else {
    transformation->AddMessageF(
        "Cannot remove %s, neither its main input nor its output may be "
        "discarded",
        LogName(*passthru_op));
    return false;
  }

  // Remove the pass-through node.
  model->operators.erase(passthru_it);

  // Remove any array that is no longer used.
  for (const string& removal_candidate : removal_candidates) {
    bool is_referenced = false;
    for (const auto& op : model->operators) {
      for (const string& input : op->inputs) {
        if (input == removal_candidate) {
          is_referenced = true;
        }
      }
      for (const string& output : op->outputs) {
        if (output == removal_candidate) {
          is_referenced = true;
        }
      }
    }
    if (!is_referenced) {
      model->EraseArray(removal_candidate);
    }
  }

  return true;
}

}  // namespace toco
