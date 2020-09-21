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

#include <unistd.h>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <limits>

#include "tensorflow/contrib/lite/builtin_op_data.h"
#include "tensorflow/contrib/lite/context.h"
#include "tensorflow/contrib/lite/kernels/activation_functor.h"
#include "tensorflow/contrib/lite/kernels/gemm_support.h"
#include "tensorflow/contrib/lite/kernels/internal/optimized/optimized_ops.h"
#include "tensorflow/contrib/lite/kernels/internal/quantization_util.h"
#include "tensorflow/contrib/lite/kernels/internal/reference/reference_ops.h"
#include "tensorflow/contrib/lite/kernels/internal/tensor.h"
#include "tensorflow/contrib/lite/kernels/internal/tensor_utils.h"
#include "tensorflow/contrib/lite/kernels/kernel_util.h"
#include "tensorflow/contrib/lite/kernels/op_macros.h"

namespace tflite {
namespace ops {
namespace builtin {
namespace fully_connected {

// This file has four implementations of FullyConnected
enum KernelType {
  kReference,
  kGenericOptimized,  // Neon-free
  kNeonOptimized,
  kPie,  // Used by the PIE team
};

struct OpData {
  // The scaling factor from input to output (aka the 'real multiplier') can
  // be represented as a fixed point multipler plus a left shift.
  int32_t output_multiplier;
  int output_shift;
  // The range of the fused activation layer. For example for kNone and
  // uint8_t these would be 0 and 255.
  int32_t output_activation_min;
  int32_t output_activation_max;
};

constexpr int kInputTensor = 0;
constexpr int kWeightsTensor = 1;
constexpr int kBiasTensor = 2;
constexpr int kOutputTensor = 0;

void* Init(TfLiteContext* context, const char* buffer, size_t length) {
  // This is a builtin op, so we don't use the contents in 'buffer', if any.
  // Instead, we allocate a new object to carry information from Prepare() to
  // Eval().
  gemm_support::IncrementUsageCounter(context);
  return new OpData;
}

void Free(TfLiteContext* context, void* buffer) {
  gemm_support::DecrementUsageCounter(context);
  delete reinterpret_cast<OpData*>(buffer);
}

TfLiteStatus Prepare(TfLiteContext* context, TfLiteNode* node) {
  auto* params =
      reinterpret_cast<TfLiteFullyConnectedParams*>(node->builtin_data);
  OpData* data = reinterpret_cast<OpData*>(node->user_data);

  // Check we have all the inputs and outputs we need.
  TF_LITE_ENSURE_EQ(context, node->inputs->size, 3);
  TF_LITE_ENSURE_EQ(context, node->outputs->size, 1);

  TfLiteTensor* input = GetInput(context, node, kInputTensor);
  TfLiteTensor* filter = GetInput(context, node, kWeightsTensor);
  TfLiteTensor* bias = GetOptionalInputTensor(context, node, kBiasTensor);
  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);

  // Check all the parameters of tensor match within themselves and match the
  // input configuration.
  int input_size = 1;
  for (int i = 0; i < input->dims->size; i++) {
    input_size *= input->dims->data[i];
  }

  const int batch_size = input_size / filter->dims->data[1];
  const int num_units = filter->dims->data[0];

  TF_LITE_ASSERT_EQ(input_size, batch_size * filter->dims->data[1]);
  if (bias) {
    TF_LITE_ASSERT_EQ(bias->dims->data[0], num_units);
  }

  TF_LITE_ENSURE_EQ(context, NumDimensions(filter), 2);
  TF_LITE_ENSURE_EQ(context, NumDimensions(bias), 1);

  // Note that quantized inference requires that all tensors have their
  // parameters set. This is usually done during quantized training.
  TfLiteType data_type = input->type;
  if (data_type != kTfLiteFloat32) {
    double real_multiplier = 0.0;
    TF_LITE_ENSURE_STATUS(GetQuantizedConvolutionMultipler(
        context, input, filter, bias, output, &real_multiplier));
    QuantizeMultiplierSmallerThanOne(real_multiplier, &data->output_multiplier,
                                     &data->output_shift);
    CalculateActivationRangeUint8(params->activation, output,
                                  &data->output_activation_min,
                                  &data->output_activation_max);
  }

  // Resize output.
  TfLiteIntArray* output_size_array = TfLiteIntArrayCreate(2);
  output_size_array->data[0] = batch_size;
  output_size_array->data[1] = num_units;
  TF_LITE_ENSURE_OK(context,
                    context->ResizeTensor(context, output, output_size_array));
  return kTfLiteOk;
}

TfLiteStatus EvalPie(TfLiteContext* context, TfLiteNode* node,
                     TfLiteFullyConnectedParams* params, OpData* data,
                     TfLiteTensor* input, TfLiteTensor* filter,
                     TfLiteTensor* bias, TfLiteTensor* output) {
  int total_input_size = 1;
  for (int i = 0; i < input->dims->size; i++) {
    total_input_size *= input->dims->data[i];
  }

  int input_size = filter->dims->data[1];
  const int batch_size = total_input_size / filter->dims->data[1];
  const int num_units = filter->dims->data[0];

  // Output = bias if bias tensor exists.
  if (bias) {
    tensor_utils::VectorBatchVectorAssign(bias->data.f, num_units, batch_size,
                                          output->data.f);
  } else {
    tensor_utils::ZeroVector(output->data.f, batch_size * num_units);
  }

  // Compute output += weight * input
  tensor_utils::MatrixBatchVectorMultiplyAccumulate(
      filter->data.f, num_units, input_size, input->data.f, batch_size,
      output->data.f, /*result_stride=*/1);

  // Apply activation function
  tensor_utils::ApplyActivationToVector(output->data.f, batch_size * num_units,
                                        params->activation, output->data.f);

  return kTfLiteOk;
}

#define TF_LITE_MACRO_DISPATCH(macro_name, params, target_namespace) \
  if (params->activation == kTfLiteActNone) {                        \
    macro_name(target_namespace, kNone);                             \
  }                                                                  \
  if (params->activation == kTfLiteActRelu) {                        \
    macro_name(target_namespace, kRelu);                             \
  }                                                                  \
  if (params->activation == kTfLiteActRelu6) {                       \
    macro_name(target_namespace, kRelu6);                            \
  }

template <KernelType kernel_type>
TfLiteStatus EvalQuantized(TfLiteContext* context, TfLiteNode* node,
                           TfLiteFullyConnectedParams* params, OpData* data,
                           TfLiteTensor* input, TfLiteTensor* filter,
                           TfLiteTensor* bias, TfLiteTensor* output) {
  gemmlowp::GemmContext* gemm_context = gemm_support::GetFromContext(context);

  int32_t input_offset = -input->params.zero_point;
  int32_t filter_offset = -filter->params.zero_point;
  int32_t output_offset = output->params.zero_point;
#define TF_LITE_FULLY_CONNECTED(type)                                       \
  type::FullyConnected(                                                     \
      GetTensorData<uint8_t>(input), GetTensorDims(input), input_offset,    \
      GetTensorData<uint8_t>(filter), GetTensorDims(filter), filter_offset, \
      GetTensorData<int32_t>(bias), GetTensorDims(bias), output_offset,     \
      data->output_multiplier, data->output_shift,                          \
      data->output_activation_min, data->output_activation_max,             \
      GetTensorData<uint8_t>(output), GetTensorDims(output), gemm_context)
  if (kernel_type == kReference) {
    TF_LITE_FULLY_CONNECTED(reference_ops);
  } else if (kernel_type == kPie) {
    // TODO(ahentz): we don't have a quantized version of the PIE kernels, so
    // we just defer to the MINI ones.
    TF_LITE_FULLY_CONNECTED(optimized_ops);
  } else {
    TF_LITE_FULLY_CONNECTED(optimized_ops);
  }
#undef TF_LITE_FULLY_CONNECTED

  return kTfLiteOk;
}

template <KernelType kernel_type>
TfLiteStatus EvalFloat(TfLiteContext* context, TfLiteNode* node,
                       TfLiteFullyConnectedParams* params, OpData* data,
                       TfLiteTensor* input, TfLiteTensor* filter,
                       TfLiteTensor* bias, TfLiteTensor* output) {
  float output_activation_min, output_activation_max;
  CalculateActivationRangeFloat(params->activation, &output_activation_min,
                                &output_activation_max);
#define TF_LITE_FULLY_CONNECTED(type)                                       \
  type::FullyConnected(GetTensorData<float>(input), GetTensorDims(input),   \
                       GetTensorData<float>(filter), GetTensorDims(filter), \
                       GetTensorData<float>(bias), GetTensorDims(bias),     \
                       output_activation_min, output_activation_max,        \
                       GetTensorData<float>(output), GetTensorDims(output))
  if (kernel_type == kReference) {
    TF_LITE_FULLY_CONNECTED(reference_ops);
  } else if (kernel_type == kPie) {
    return EvalPie(context, node, params, data, input, filter, bias, output);
  } else {
    TF_LITE_FULLY_CONNECTED(optimized_ops);
  }
#undef TF_LITE_FULLY_CONNECTED

  return kTfLiteOk;
}

#undef TF_LITE_MACRO_DISPATCH

template <KernelType kernel_type>
TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  auto* params =
      reinterpret_cast<TfLiteFullyConnectedParams*>(node->builtin_data);
  OpData* data = reinterpret_cast<OpData*>(node->user_data);

  TfLiteTensor* input = GetInput(context, node, kInputTensor);
  TfLiteTensor* filter = GetInput(context, node, kWeightsTensor);
  TfLiteTensor* bias = GetOptionalInputTensor(context, node, kBiasTensor);
  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);

  switch (input->type) {  // Already know in/out types are same.
    case kTfLiteFloat32:
      return EvalFloat<kernel_type>(context, node, params, data, input, filter,
                                    bias, output);
    case kTfLiteUInt8:
      return EvalQuantized<kernel_type>(context, node, params, data, input,
                                        filter, bias, output);
    default:
      context->ReportError(context, "Type not currently supported.");
      return kTfLiteError;
  }
  return kTfLiteOk;
}

}  // namespace fully_connected

TfLiteRegistration* Register_FULLY_CONNECTED_REF() {
  static TfLiteRegistration r = {
      fully_connected::Init, fully_connected::Free, fully_connected::Prepare,
      fully_connected::Eval<fully_connected::kReference>};
  return &r;
}

TfLiteRegistration* Register_FULLY_CONNECTED_NEON_OPT() {
  static TfLiteRegistration r = {
      fully_connected::Init, fully_connected::Free, fully_connected::Prepare,
      fully_connected::Eval<fully_connected::kNeonOptimized>};
  return &r;
}

TfLiteRegistration* Register_FULLY_CONNECTED_GENERIC_OPT() {
  static TfLiteRegistration r = {
      fully_connected::Init, fully_connected::Free, fully_connected::Prepare,
      fully_connected::Eval<fully_connected::kGenericOptimized>};
  return &r;
}

TfLiteRegistration* Register_FULLY_CONNECTED_PIE() {
  static TfLiteRegistration r = {fully_connected::Init, fully_connected::Free,
                                 fully_connected::Prepare,
                                 fully_connected::Eval<fully_connected::kPie>};
  return &r;
}

TfLiteRegistration* Register_FULLY_CONNECTED() {
  // TODO(ahentz): We don't have a dedicated quantized version of the PIE
  // kernel. For now, the quantized version just defer to the corresponding
  // optimized MINI kernel. At some point we will allow different libraries to
  // be built with different kernels, but for now we have to pick one here.
  return Register_FULLY_CONNECTED_PIE();
#ifdef USE_NEON
  return Register_FULLY_CONNECTED_NEON_OPT();
#else
  return Register_FULLY_CONNECTED_GENERIC_OPT();
#endif
}

}  // namespace builtin
}  // namespace ops
}  // namespace tflite
