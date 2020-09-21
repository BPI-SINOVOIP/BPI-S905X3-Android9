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

#include "tensorflow/compiler/tf2xla/lib/util.h"
#include "tensorflow/compiler/tf2xla/xla_helpers.h"
#include "tensorflow/compiler/tf2xla/xla_op_kernel.h"
#include "tensorflow/compiler/tf2xla/xla_op_registry.h"

namespace tensorflow {
namespace {

// Converts 'input' from RGB format to HSV format.
// 'shape' is the shape of the red/green/blue tensors.
std::array<xla::ComputationDataHandle, 3> RGBToHSV(
    XlaOpKernelContext* ctx, xla::ComputationBuilder* b,
    const std::array<xla::ComputationDataHandle, 3>& rgb, DataType dtype,
    const TensorShape& shape) {
  auto zero = XlaHelpers::Zero(b, dtype);
  auto one = XlaHelpers::One(b, dtype);

  auto red = rgb[0];
  auto green = rgb[1];
  auto blue = rgb[2];
  auto value = b->Max(b->Max(red, green), blue);
  auto minimum = b->Min(b->Min(red, green), blue);
  auto range = b->Sub(value, minimum);

  auto zeros = b->Broadcast(zero, shape.dim_sizes());
  auto saturation = b->Select(b->Gt(value, zero), b->Div(range, value), zeros);

  auto norm = b->Div(XlaHelpers::FloatLiteral(b, dtype, 1.0 / 6.0), range);

  auto hue = b->Select(b->Eq(green, value),
                       b->Add(b->Mul(norm, b->Sub(blue, red)),
                              XlaHelpers::FloatLiteral(b, dtype, 2.0 / 6.0)),
                       b->Add(b->Mul(norm, b->Sub(red, green)),
                              XlaHelpers::FloatLiteral(b, dtype, 4.0 / 6.0)));
  hue = b->Select(b->Eq(red, value), b->Mul(norm, b->Sub(green, blue)), hue);
  hue = b->Select(b->Gt(range, zero), hue, zeros);
  hue = b->Select(b->Lt(hue, zero), b->Add(hue, one), hue);
  return {hue, saturation, value};
}

// Converts 'input' from HSV format to RGB format.
std::array<xla::ComputationDataHandle, 3> HSVToRGB(
    xla::ComputationBuilder* b,
    const std::array<xla::ComputationDataHandle, 3>& hsv, DataType dtype) {
  xla::ComputationDataHandle hue = hsv[0];
  xla::ComputationDataHandle saturation = hsv[1];
  xla::ComputationDataHandle value = hsv[2];
  auto zero = XlaHelpers::Zero(b, dtype);
  auto one = XlaHelpers::FloatLiteral(b, dtype, 1.0);
  auto two = XlaHelpers::FloatLiteral(b, dtype, 2.0);
  auto three = XlaHelpers::FloatLiteral(b, dtype, 3.0);
  auto four = XlaHelpers::FloatLiteral(b, dtype, 4.0);
  auto six = XlaHelpers::FloatLiteral(b, dtype, 6.0);

  auto dh = b->Mul(hue, six);
  auto dr = b->Clamp(zero, b->Sub(b->Abs(b->Sub(dh, three)), one), one);
  auto dg = b->Clamp(zero, b->Sub(two, b->Abs(b->Sub(dh, two))), one);
  auto db = b->Clamp(zero, b->Sub(two, b->Abs(b->Sub(dh, four))), one);
  auto one_minus_s = b->Sub(one, saturation);

  auto red = b->Mul(b->Add(one_minus_s, b->Mul(saturation, dr)), value);
  auto green = b->Mul(b->Add(one_minus_s, b->Mul(saturation, dg)), value);
  auto blue = b->Mul(b->Add(one_minus_s, b->Mul(saturation, db)), value);
  return {red, green, blue};
}

class RGBToHSVOp : public XlaOpKernel {
 public:
  explicit RGBToHSVOp(OpKernelConstruction* context) : XlaOpKernel(context) {}

  void Compile(XlaOpKernelContext* context) override {
    const TensorShape input_shape = context->InputShape(0);
    OP_REQUIRES(context, input_shape.dims() >= 1,
                errors::InvalidArgument("input must be at least 1D",
                                        input_shape.DebugString()));
    int channel_dim = input_shape.dims() - 1;
    int64 channels = input_shape.dim_size(channel_dim);
    OP_REQUIRES(
        context, channels == 3,
        errors::FailedPrecondition("input must have 3 channels but input has ",
                                   channels, " channels."));

    xla::ComputationBuilder* b = context->builder();
    xla::ComputationDataHandle input = context->Input(0);

    xla::ComputationDataHandle red =
        b->SliceInDim(input, /*start_index=*/0, /*limit_index=*/1, /*stride=*/1,
                      /*dimno=*/channel_dim);
    xla::ComputationDataHandle green =
        b->SliceInDim(input, /*start_index=*/1, /*limit_index=*/2, /*stride=*/1,
                      /*dimno=*/channel_dim);
    xla::ComputationDataHandle blue =
        b->SliceInDim(input, /*start_index=*/2, /*limit_index=*/3, /*stride=*/1,
                      /*dimno=*/channel_dim);
    TensorShape channel_shape = input_shape;
    channel_shape.set_dim(channel_dim, 1);
    auto hsv = RGBToHSV(context, b, {red, green, blue}, context->input_type(0),
                        channel_shape);

    context->SetOutput(0, b->ConcatInDim(hsv, channel_dim));
  }
};
REGISTER_XLA_OP(Name("RGBToHSV"), RGBToHSVOp);

class HSVToRGBOp : public XlaOpKernel {
 public:
  explicit HSVToRGBOp(OpKernelConstruction* context) : XlaOpKernel(context) {}

  void Compile(XlaOpKernelContext* context) override {
    const TensorShape input_shape = context->InputShape(0);
    OP_REQUIRES(context, input_shape.dims() >= 1,
                errors::InvalidArgument("input must be at least 1D",
                                        input_shape.DebugString()));
    int channel_dim = input_shape.dims() - 1;
    int64 channels = input_shape.dim_size(channel_dim);
    OP_REQUIRES(
        context, channels == 3,
        errors::FailedPrecondition("input must have 3 channels but input has ",
                                   channels, " channels."));

    xla::ComputationBuilder* b = context->builder();
    xla::ComputationDataHandle input = context->Input(0);
    xla::ComputationDataHandle hue =
        b->SliceInDim(input, /*start_index=*/0, /*limit_index=*/1, /*stride=*/1,
                      /*dimno=*/channel_dim);
    xla::ComputationDataHandle saturation =
        b->SliceInDim(input, /*start_index=*/1, /*limit_index=*/2, /*stride=*/1,
                      /*dimno=*/channel_dim);
    xla::ComputationDataHandle value =
        b->SliceInDim(input, /*start_index=*/2, /*limit_index=*/3, /*stride=*/1,
                      /*dimno=*/channel_dim);

    auto rgb = HSVToRGB(context->builder(), {hue, saturation, value},
                        context->input_type(0));

    context->SetOutput(0, b->ConcatInDim(rgb, channel_dim));
  }
};
REGISTER_XLA_OP(Name("HSVToRGB"), HSVToRGBOp);

class AdjustContrastOpV2 : public XlaOpKernel {
 public:
  explicit AdjustContrastOpV2(OpKernelConstruction* context)
      : XlaOpKernel(context) {}

  void Compile(XlaOpKernelContext* context) override {
    const TensorShape& input_shape = context->InputShape(0);
    const TensorShape& factor_shape = context->InputShape(1);
    OP_REQUIRES(context, input_shape.dims() >= 3,
                errors::InvalidArgument("input must be at least 3-D, got shape",
                                        input_shape.DebugString()));
    int height_dim = input_shape.dims() - 3;
    int width_dim = input_shape.dims() - 2;
    int channel_dim = input_shape.dims() - 1;
    const int64 height = input_shape.dim_size(height_dim);
    const int64 width = input_shape.dim_size(width_dim);

    OP_REQUIRES(context, TensorShapeUtils::IsScalar(factor_shape),
                errors::InvalidArgument("contrast_factor must be scalar: ",
                                        factor_shape.DebugString()));

    xla::ComputationBuilder* b = context->builder();
    xla::ComputationDataHandle input = context->Input(0);
    xla::ComputationDataHandle factor = context->Input(1);

    DataType type = context->input_type(0);

    auto output = b->Reduce(input, /*init_value=*/XlaHelpers::Zero(b, type),
                            /*computation=*/*context->GetOrCreateAdd(type),
                            {height_dim, width_dim});
    output = b->Div(output, XlaHelpers::FloatLiteral(b, type, height * width));

    std::vector<int64> broadcast_dims(input_shape.dims() - 2);
    std::iota(broadcast_dims.begin(), broadcast_dims.end(), 0);
    broadcast_dims.back() = channel_dim;
    output = b->Add(b->Mul(input, factor),
                    b->Mul(output, b->Sub(XlaHelpers::One(b, type), factor)),
                    broadcast_dims);
    context->SetOutput(0, output);
  }
};
REGISTER_XLA_OP(Name("AdjustContrastv2"), AdjustContrastOpV2);

class AdjustSaturationOp : public XlaOpKernel {
 public:
  explicit AdjustSaturationOp(OpKernelConstruction* context)
      : XlaOpKernel(context) {}

  void Compile(XlaOpKernelContext* context) override {
    const TensorShape& input_shape = context->InputShape(0);
    const TensorShape& scale_shape = context->InputShape(1);
    OP_REQUIRES(context, input_shape.dims() >= 3,
                errors::InvalidArgument("input must be at least 3-D, got shape",
                                        input_shape.DebugString()));
    OP_REQUIRES(context, TensorShapeUtils::IsScalar(scale_shape),
                errors::InvalidArgument("scale must be scalar: ",
                                        scale_shape.DebugString()));
    const int channel_dim = input_shape.dims() - 1;
    const int64 channels = input_shape.dim_size(channel_dim);
    OP_REQUIRES(
        context, channels == 3,
        errors::InvalidArgument("input must have 3 channels but instead has ",
                                channels, " channels."));

    xla::ComputationBuilder* b = context->builder();
    xla::ComputationDataHandle input = context->Input(0);
    xla::ComputationDataHandle scale = context->Input(1);

    DataType type = context->input_type(0);

    xla::ComputationDataHandle red =
        b->SliceInDim(input, /*start_index=*/0, /*limit_index=*/1, /*stride=*/1,
                      /*dimno=*/channel_dim);
    xla::ComputationDataHandle green =
        b->SliceInDim(input, /*start_index=*/1, /*limit_index=*/2, /*stride=*/1,
                      /*dimno=*/channel_dim);
    xla::ComputationDataHandle blue =
        b->SliceInDim(input, /*start_index=*/2, /*limit_index=*/3, /*stride=*/1,
                      /*dimno=*/channel_dim);
    TensorShape channel_shape = input_shape;
    channel_shape.set_dim(channel_dim, 1);
    auto hsv = RGBToHSV(context, b, {red, green, blue}, context->input_type(0),
                        channel_shape);

    hsv[1] = b->Clamp(XlaHelpers::Zero(b, type), b->Mul(hsv[1], scale),
                      XlaHelpers::One(b, type));

    auto rgb = HSVToRGB(context->builder(), hsv, context->input_type(0));

    context->SetOutput(0, b->ConcatInDim(rgb, channel_dim));
  }
};
REGISTER_XLA_OP(Name("AdjustSaturation"), AdjustSaturationOp);

class AdjustHueOp : public XlaOpKernel {
 public:
  explicit AdjustHueOp(OpKernelConstruction* context) : XlaOpKernel(context) {}

  void Compile(XlaOpKernelContext* context) override {
    const TensorShape& input_shape = context->InputShape(0);
    const TensorShape& delta_shape = context->InputShape(1);
    OP_REQUIRES(context, input_shape.dims() >= 3,
                errors::InvalidArgument("input must be at least 3-D, got shape",
                                        input_shape.DebugString()));
    OP_REQUIRES(context, TensorShapeUtils::IsScalar(delta_shape),
                errors::InvalidArgument("delta must be scalar: ",
                                        delta_shape.DebugString()));
    const int channel_dim = input_shape.dims() - 1;
    const int64 channels = input_shape.dim_size(channel_dim);
    OP_REQUIRES(
        context, channels == 3,
        errors::InvalidArgument("input must have 3 channels but instead has ",
                                channels, " channels."));

    xla::ComputationBuilder* b = context->builder();
    xla::ComputationDataHandle input = context->Input(0);
    xla::ComputationDataHandle delta = context->Input(1);

    DataType type = context->input_type(0);

    xla::ComputationDataHandle red =
        b->SliceInDim(input, /*start_index=*/0, /*limit_index=*/1, /*stride=*/1,
                      /*dimno=*/channel_dim);
    xla::ComputationDataHandle green =
        b->SliceInDim(input, /*start_index=*/1, /*limit_index=*/2, /*stride=*/1,
                      /*dimno=*/channel_dim);
    xla::ComputationDataHandle blue =
        b->SliceInDim(input, /*start_index=*/2, /*limit_index=*/3, /*stride=*/1,
                      /*dimno=*/channel_dim);
    TensorShape channel_shape = input_shape;
    channel_shape.set_dim(channel_dim, 1);
    auto hsv = RGBToHSV(context, b, {red, green, blue}, context->input_type(0),
                        channel_shape);

    auto zero = XlaHelpers::Zero(b, type);
    auto one = XlaHelpers::One(b, type);

    auto& hue = hsv[0];
    hue = b->Rem(b->Add(hsv[0], delta), one);
    hue = b->Select(b->Lt(hue, zero), b->Rem(b->Add(one, hue), one), hue);

    auto rgb = HSVToRGB(context->builder(), hsv, context->input_type(0));

    context->SetOutput(0, b->ConcatInDim(rgb, channel_dim));
  }
};
REGISTER_XLA_OP(Name("AdjustHue"), AdjustHueOp);

}  // namespace
}  // namespace tensorflow
