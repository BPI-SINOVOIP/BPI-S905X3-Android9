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

#include <cmath>

#include "tensorflow/compiler/tf2xla/shape_util.h"
#include "tensorflow/compiler/tf2xla/xla_helpers.h"
#include "tensorflow/compiler/tf2xla/xla_op_kernel.h"
#include "tensorflow/compiler/tf2xla/xla_op_registry.h"
#include "tensorflow/compiler/xla/client/lib/arithmetic.h"
#include "tensorflow/core/framework/op_kernel.h"
#include "tensorflow/core/framework/tensor.h"
#include "tensorflow/core/framework/tensor_shape.h"
#include "tensorflow/core/lib/core/casts.h"
#include "tensorflow/core/lib/math/math_util.h"

namespace tensorflow {
namespace {

// Rotates a 32-bit integer 'v' left by 'distance' bits.
xla::ComputationDataHandle RotateLeftS32(xla::ComputationBuilder* builder,
                                         const xla::ComputationDataHandle& v,
                                         int distance) {
  return builder->Or(
      builder->ShiftLeft(v, builder->ConstantR0<int>(distance)),
      builder->ShiftRightLogical(v, builder->ConstantR0<int>(32 - distance)));
}

// TODO(b/65209188): add a primitive XOR to XLA and call it here, rather than
// building XOR out of other bitwise operators.
xla::ComputationDataHandle BitwiseXor(xla::ComputationBuilder* builder,
                                      const xla::ComputationDataHandle& x,
                                      const xla::ComputationDataHandle& y) {
  return builder->Or(builder->And(x, builder->Not(y)),
                     builder->And(builder->Not(x), y));
}

using ThreeFry2x32State = std::array<xla::ComputationDataHandle, 2>;

// Implements the ThreeFry counter-based PRNG algorithm.
// Salmon et al. SC 2011. Parallel random numbers: as easy as 1, 2, 3.
// http://www.thesalmons.org/john/random123/papers/random123sc11.pdf
ThreeFry2x32State ThreeFry2x32(xla::ComputationBuilder* builder,
                               ThreeFry2x32State input, ThreeFry2x32State key) {
  // Rotation distances specified by the Threefry2x32 algorithm.
  constexpr std::array<int, 8> rotations = {13, 15, 26, 6, 17, 29, 16, 24};
  ThreeFry2x32State x;

  std::array<xla::ComputationDataHandle, 3> ks;
  // 0x1BD11BDA is a parity constant specified by the ThreeFry2x32 algorithm.
  ks[2] = builder->ConstantR0<int32>(0x1BD11BDA);
  for (int i = 0; i < 2; ++i) {
    ks[i] = key[i];
    x[i] = input[i];
    ks[2] = BitwiseXor(builder, ks[2], key[i]);
  }

  x[0] = builder->Add(x[0], ks[0]);
  x[1] = builder->Add(x[1], ks[1]);

  // Performs a single round of the Threefry2x32 algorithm, with a rotation
  // amount 'rotation'.
  auto round = [builder](ThreeFry2x32State v, int rotation) {
    v[0] = builder->Add(v[0], v[1]);
    v[1] = RotateLeftS32(builder, v[1], rotation);
    v[1] = BitwiseXor(builder, v[0], v[1]);
    return v;
  };

  // There are no known statistical flaws with 13 rounds of Threefry2x32.
  // We are conservative and use 20 rounds.
  x = round(x, rotations[0]);
  x = round(x, rotations[1]);
  x = round(x, rotations[2]);
  x = round(x, rotations[3]);
  x[0] = builder->Add(x[0], ks[1]);
  x[1] = builder->Add(builder->Add(x[1], ks[2]), builder->ConstantR0<int32>(1));

  x = round(x, rotations[4]);
  x = round(x, rotations[5]);
  x = round(x, rotations[6]);
  x = round(x, rotations[7]);
  x[0] = builder->Add(x[0], ks[2]);
  x[1] = builder->Add(builder->Add(x[1], ks[0]), builder->ConstantR0<int32>(2));

  x = round(x, rotations[0]);
  x = round(x, rotations[1]);
  x = round(x, rotations[2]);
  x = round(x, rotations[3]);
  x[0] = builder->Add(x[0], ks[0]);
  x[1] = builder->Add(builder->Add(x[1], ks[1]), builder->ConstantR0<int32>(3));

  x = round(x, rotations[4]);
  x = round(x, rotations[5]);
  x = round(x, rotations[6]);
  x = round(x, rotations[7]);
  x[0] = builder->Add(x[0], ks[1]);
  x[1] = builder->Add(builder->Add(x[1], ks[2]), builder->ConstantR0<int32>(4));

  x = round(x, rotations[0]);
  x = round(x, rotations[1]);
  x = round(x, rotations[2]);
  x = round(x, rotations[3]);
  x[0] = builder->Add(x[0], ks[2]);
  x[1] = builder->Add(builder->Add(x[1], ks[0]), builder->ConstantR0<int32>(5));

  return x;
}

// Returns a tensor of 'shape' random values uniformly distributed in the range
// [minval, maxval)
xla::ComputationDataHandle RandomUniform(xla::ComputationBuilder* builder,
                                         const xla::ComputationDataHandle& seed,
                                         const TensorShape& shape,
                                         double minval, double maxval) {
  // Split the seed into two 32-bit scalars to form a key.
  auto seed0 = builder->Reshape(builder->Slice(seed, {0}, {1}, {1}), {});
  auto seed1 = builder->Reshape(builder->Slice(seed, {1}, {2}, {1}), {});
  ThreeFry2x32State key = {seed0, seed1};
  const int64 size = shape.num_elements();

  const int64 half_size = MathUtil::CeilOfRatio<int64>(size, 2);
  const bool size_is_odd = (half_size * 2 != size);

  // Fill the generator inputs with unique counter values.
  ThreeFry2x32State inputs;
  TF_CHECK_OK(XlaHelpers::Iota(builder, DT_INT32, half_size, &inputs[0]));
  inputs[1] = builder->Add(inputs[0], builder->ConstantR0<int32>(half_size));
  ThreeFry2x32State outputs = ThreeFry2x32(builder, inputs, key);

  if (size_is_odd) {
    outputs[1] = builder->Slice(outputs[1], {0}, {half_size - 1}, {1});
  }

  auto bits =
      builder->Reshape(builder->ConcatInDim(outputs, 0), shape.dim_sizes());

  // Form 22 random mantissa bits, with a leading 1 bit. The leading 1 bit
  // forces the random bits into the mantissa.
  constexpr int kFloatBits = 32;
  constexpr int kMantissaBits = 23;
  bits = builder->Or(
      builder->ShiftRightLogical(
          bits, builder->ConstantR0<int32>(kFloatBits - kMantissaBits)),
      builder->ConstantR0<int32>(bit_cast<int32>(1.0f)));
  auto floats = builder->BitcastConvertType(bits, xla::F32);

  // We have a floating point number in the range [1.0, 2.0).
  // Subtract 1.0f to shift to the range [0.0, 1.0)
  floats = builder->Sub(floats, builder->ConstantR0<float>(1.0f));
  // Multiply and add to shift to the range [minval, maxval).
  floats = builder->Mul(floats, builder->ConstantR0<float>(maxval - minval));
  floats = builder->Add(floats, builder->ConstantR0<float>(minval));
  return floats;
}

// Approximation for the inverse error function from
//   Giles, M., "Approximating the erfinv function".
// The approximation has the form:
//   w = -log((1 - x) * (1 + x))
//   if ( w < 5 ) {
//     w = w - 2.5
//     p = sum_{i=1}^n lq[i]*w^i
//   } else {
//     w = sqrt(w) - 3
//     p = sum_{i=1}^n gq[i]*w^i
//   }
//   return p*x
xla::ComputationDataHandle ErfInvF32(xla::ComputationBuilder* b,
                                     const xla::ComputationDataHandle& x,
                                     const TensorShape& shape) {
  constexpr int kDegree = 9;
  constexpr std::array<float, 9> w_less_than_5_constants = {
      2.81022636e-08f,  3.43273939e-07f, -3.5233877e-06f,
      -4.39150654e-06f, 0.00021858087f,  -0.00125372503f,
      -0.00417768164f,  0.246640727f,    1.50140941f};
  constexpr std::array<float, 9> w_greater_than_5_constants = {
      -0.000200214257f, 0.000100950558f, 0.00134934322f,
      -0.00367342844f,  0.00573950773f,  -0.0076224613f,
      0.00943887047f,   1.00167406f,     2.83297682f};

  auto one = b->ConstantR0<float>(1.0);
  auto w = b->Neg(b->Log(b->Mul(b->Sub(one, x), b->Add(one, x))));

  auto lt = b->Lt(w, b->ConstantR0<float>(5.0));
  auto coefficient = [&](int i) {
    return b->Select(
        lt,
        b->Broadcast(b->ConstantR0<float>(w_less_than_5_constants[i]),
                     shape.dim_sizes()),
        b->Broadcast(b->ConstantR0<float>(w_greater_than_5_constants[i]),
                     shape.dim_sizes()));
  };
  w = b->Select(lt, b->Sub(w, b->ConstantR0<float>(2.5f)),
                b->Sub(b->SqrtF32(w), b->ConstantR0<float>(3.0f)));
  auto p = coefficient(0);
  for (int i = 1; i < kDegree; ++i) {
    p = b->Add(coefficient(i), b->Mul(p, w));
  }
  return b->Mul(p, x);
}

}  // namespace

class StatelessRandomUniformOp : public XlaOpKernel {
 public:
  explicit StatelessRandomUniformOp(OpKernelConstruction* ctx)
      : XlaOpKernel(ctx) {}

  void Compile(XlaOpKernelContext* ctx) override {
    xla::ComputationBuilder* builder = ctx->builder();

    TensorShape shape;
    OP_REQUIRES_OK(ctx, ctx->ConstantInputAsShape(0, &shape));

    TensorShape seed_shape = ctx->InputShape(1);
    OP_REQUIRES(ctx, seed_shape.dims() == 1 && seed_shape.dim_size(0) == 2,
                errors::InvalidArgument("seed must have shape [2], not ",
                                        seed_shape.DebugString()));
    xla::ComputationDataHandle seed = ctx->Input(1);
    ctx->SetOutput(0, RandomUniform(builder, seed, shape, 0.0, 1.0));
  }

 private:
  TF_DISALLOW_COPY_AND_ASSIGN(StatelessRandomUniformOp);
};

// TODO(phawkins): generalize to non-float, non-int32 seed types.
REGISTER_XLA_OP(Name("StatelessRandomUniform")
                    .TypeConstraint("dtype", DT_FLOAT)
                    .TypeConstraint("Tseed", DT_INT32),
                StatelessRandomUniformOp);

class StatelessRandomNormalOp : public XlaOpKernel {
 public:
  explicit StatelessRandomNormalOp(OpKernelConstruction* ctx)
      : XlaOpKernel(ctx) {}

  void Compile(XlaOpKernelContext* ctx) override {
    TensorShape shape;
    OP_REQUIRES_OK(ctx, ctx->ConstantInputAsShape(0, &shape));

    TensorShape seed_shape = ctx->InputShape(1);
    OP_REQUIRES(ctx, seed_shape == TensorShape({2}),
                errors::InvalidArgument("seed must have shape [2], not ",
                                        seed_shape.DebugString()));
    xla::ComputationDataHandle seed = ctx->Input(1);
    xla::ComputationBuilder* builder = ctx->builder();
    auto uniform = RandomUniform(builder, seed, shape, -1.0, 1.0);
    // Convert uniform distribution to normal distribution by computing
    // sqrt(2) * erfinv(x)
    auto normal = builder->Mul(builder->ConstantR0<float>(std::sqrt(2.0)),
                               ErfInvF32(builder, uniform, shape));
    ctx->SetOutput(0, normal);
  }

 private:
  TF_DISALLOW_COPY_AND_ASSIGN(StatelessRandomNormalOp);
};

// TODO(phawkins): generalize to non-float, non-int32 seed types.
REGISTER_XLA_OP(Name("StatelessRandomNormal")
                    .TypeConstraint("dtype", DT_FLOAT)
                    .TypeConstraint("Tseed", DT_INT32),
                StatelessRandomNormalOp);

}  // namespace tensorflow
