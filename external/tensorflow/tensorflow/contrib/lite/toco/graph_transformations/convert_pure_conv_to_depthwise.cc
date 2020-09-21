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

bool ConvertPureConvToDepthwise::Run(Model* model, std::size_t op_index) {
  auto conv_it = model->operators.begin() + op_index;
  if (conv_it->get()->type != OperatorType::kConv) {
    return false;
  }
  const auto* conv_op = static_cast<ConvOperator*>(conv_it->get());
  if (conv_op->stride_width != conv_op->stride_height) {
    return false;
  }
  auto& weights_array = model->GetArray(conv_op->inputs[1]);
  if (!weights_array.buffer) {
    // Yield until the weights are resolved as a constant array.
    return false;
  }
  if (weights_array.data_type != ArrayDataType::kFloat) {
    return false;
  }
  if (weights_array.shape().dims(3) != 1) {
    // Not a pure convolution: Conv does accumulation across the depth
    // dimension.
    return false;
  }
  // At this point we know we have a pure conv. Rewrite it as DepthwiseConv.
  AddMessageF(
      "%s is purely convolutional (input/weights depth is 1), replacing it by "
      "a DepthwiseConv.",
      LogName(*conv_op));
  auto* depthwiseconv_op = new DepthwiseConvOperator;
  // Conv and DepthwiseConv take the same inputs
  depthwiseconv_op->inputs = conv_op->inputs;
  // Conv may have a 2nd output for im2col
  depthwiseconv_op->outputs = {conv_op->outputs[0]};
  if (conv_op->outputs.size() > 1) {
    // delete the im2col array.
    model->EraseArray(conv_op->outputs[1]);
  }
  depthwiseconv_op->fused_activation_function =
      conv_op->fused_activation_function;
  // Let PropagateFixedSizes recompute fixed padding, just in case some day it
  // may be different for Conv vs DepthwiseConv.
  depthwiseconv_op->padding.type = conv_op->padding.type;
  depthwiseconv_op->stride_height = conv_op->stride_height;
  depthwiseconv_op->stride_width = conv_op->stride_width;
  depthwiseconv_op->depth_multiplier = weights_array.shape().dims(0);
  // Replace the operator in the graph.
  const auto depthwiseconv_it =
      model->operators.emplace(conv_it, depthwiseconv_op);
  conv_it = depthwiseconv_it + 1;
  CHECK_EQ(conv_it->get(), conv_op);
  model->operators.erase(conv_it);
  // Shuffle the weights.
  const auto& weights_shape = weights_array.shape();
  auto& weights_buffer =
      weights_array.GetMutableBuffer<ArrayDataType::kFloat>();
  const std::vector<float>& conv_weights_data = weights_buffer.data;
  std::vector<float> depthwise_conv_weights_data(conv_weights_data.size());
  const int depth = weights_shape.dims(0);
  const int width = weights_shape.dims(1);
  const int height = weights_shape.dims(2);
  const int width_height = width * height;
  for (int c = 0; c < depth; c++) {
    for (int xy = 0; xy < width_height; xy++) {
      depthwise_conv_weights_data[c + depth * xy] =
          conv_weights_data[xy + width_height * c];
    }
  }
  *weights_array.mutable_shape()->mutable_dims() = {1, width, height, depth};
  weights_buffer.data = depthwise_conv_weights_data;
  return true;
}

}  // namespace toco
