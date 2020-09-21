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

#include "tensorflow/compiler/tf2xla/type_util.h"
#include "tensorflow/compiler/tf2xla/xla_helpers.h"
#include "tensorflow/compiler/tf2xla/xla_op_kernel.h"
#include "tensorflow/compiler/tf2xla/xla_op_registry.h"
#include "tensorflow/compiler/xla/primitive_util.h"
#include "tensorflow/core/framework/kernel_def_builder.h"

namespace tensorflow {
namespace {

class CastOp : public XlaOpKernel {
 public:
  explicit CastOp(OpKernelConstruction* ctx) : XlaOpKernel(ctx) {
    OP_REQUIRES_OK(ctx, ctx->GetAttr("SrcT", &src_dtype_));
    OP_REQUIRES_OK(ctx, ctx->GetAttr("DstT", &dst_dtype_));
    OP_REQUIRES_OK(ctx, DataTypeToPrimitiveType(src_dtype_, &src_type_));
    OP_REQUIRES_OK(ctx, DataTypeToPrimitiveType(dst_dtype_, &dst_type_));
  }

  void Compile(XlaOpKernelContext* ctx) override {
    xla::ComputationBuilder* builder = ctx->builder();
    xla::ComputationDataHandle input = ctx->Input(0);
    xla::ComputationDataHandle output;

    if (src_dtype_ == dst_dtype_) {
      output = input;
    } else if (dst_dtype_ == DT_BOOL) {
      output = builder->Ne(input, XlaHelpers::Zero(builder, src_dtype_));
    } else if (xla::primitive_util::IsComplexType(src_type_) &&
               !xla::primitive_util::IsComplexType(dst_type_)) {
      // As in cast_op.h, we replicate the numpy behavior of truncating the
      // imaginary part.
      output = builder->ConvertElementType(builder->Real(input), dst_type_);
    } else {
      output = builder->ConvertElementType(input, dst_type_);
    }

    ctx->SetOutput(0, output);
  }

 protected:
  DataType src_dtype_, dst_dtype_;
  xla::PrimitiveType src_type_, dst_type_;

  TF_DISALLOW_COPY_AND_ASSIGN(CastOp);
};

REGISTER_XLA_OP(Name("Cast"), CastOp);

}  // anonymous namespace
}  // namespace tensorflow
