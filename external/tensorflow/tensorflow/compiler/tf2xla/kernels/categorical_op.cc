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

// XLA implementations of Categorical op.

#include "tensorflow/compiler/tf2xla/shape_util.h"
#include "tensorflow/compiler/tf2xla/type_util.h"
#include "tensorflow/compiler/tf2xla/xla_helpers.h"
#include "tensorflow/compiler/tf2xla/xla_op_kernel.h"
#include "tensorflow/compiler/tf2xla/xla_op_registry.h"
#include "tensorflow/compiler/xla/client/lib/arithmetic.h"
#include "tensorflow/core/framework/op_kernel.h"
#include "tensorflow/core/framework/tensor.h"
#include "tensorflow/core/framework/tensor_shape.h"

namespace tensorflow {
namespace {

class CategoricalOp : public XlaOpKernel {
 public:
  explicit CategoricalOp(OpKernelConstruction* ctx) : XlaOpKernel(ctx) {}

  void Compile(XlaOpKernelContext* ctx) override {
    // Get the logits
    const xla::ComputationDataHandle& logits = ctx->Input(0);
    TensorShape logits_shape = ctx->InputShape(0);
    int64 num_samples;
    OP_REQUIRES_OK(ctx, ctx->ConstantInputAsIntScalar(1, &num_samples));
    OP_REQUIRES(ctx, TensorShapeUtils::IsMatrix(logits_shape),
                errors::InvalidArgument("logits should be a matrix, got shape ",
                                        logits_shape.DebugString()));
    OP_REQUIRES(ctx, num_samples >= 0,
                errors::InvalidArgument(
                    "num_samples should be nonnegative, got ", num_samples));

    for (int i = 0; i < 2; i++) {
      const int64 dim = logits_shape.dim_size(i);
      OP_REQUIRES(
          ctx, static_cast<int>(dim) == dim,
          errors::InvalidArgument("logits.shape = ", logits_shape.DebugString(),
                                  " too large for int"));
    }

    const int64 batch_size = logits_shape.dim_size(0);
    const int64 num_classes = logits_shape.dim_size(1);

    xla::ComputationBuilder* builder = ctx->builder();

    std::array<int64, 3> uniform_shape_array = {
        {batch_size, num_samples, num_classes}};
    xla::PrimitiveType uniform_xla_type;
    OP_REQUIRES_OK(ctx,
                   DataTypeToPrimitiveType(input_type(0), &uniform_xla_type));
    xla::Shape uniform_shape =
        xla::ShapeUtil::MakeShape(uniform_xla_type, uniform_shape_array);
    auto uniforms = builder->RngUniform(
        XlaHelpers::Zero(builder, input_type(0)),
        XlaHelpers::One(builder, input_type(0)), uniform_shape);

    // Use Gumbel softmax trick to generate categorical samples.
    // See:
    // https://hips.seas.harvard.edu/blog/2013/04/06/the-gumbel-max-trick-for-discrete-distributions/
    // TODO(b/68769470): Switch to using a cumulative sum approach.
    auto softmax_entries =
        builder->Sub(logits, builder->Log(builder->Neg(builder->Log(uniforms))),
                     /*broadcast_dimensions=*/{0, 2});

    TensorShape softmax_shape(uniform_shape_array);
    xla::ComputationDataHandle argmax;
    OP_REQUIRES_OK(
        ctx,
        XlaHelpers::ArgMax(builder, ctx, softmax_entries, softmax_shape,
                           input_type(0), output_type(0), /*axis=*/2, &argmax));

    ctx->SetOutput(0, argmax);
  }

 private:
  TF_DISALLOW_COPY_AND_ASSIGN(CategoricalOp);
};

// TODO(b/68769717): Rename this sampler to Categorical.
REGISTER_XLA_OP(Name("Multinomial").CompileTimeConstInput("num_samples"),
                CategoricalOp);

}  // anonymous namespace
}  // namespace tensorflow
