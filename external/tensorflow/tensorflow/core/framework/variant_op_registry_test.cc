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

#define EIGEN_USE_THREADS

#if GOOGLE_CUDA
#define EIGEN_USE_GPU
#endif

#include "tensorflow/core/framework/variant_op_registry.h"

#include "third_party/eigen3/unsupported/Eigen/CXX11/Tensor"
#include "tensorflow/core/framework/op_kernel.h"
#include "tensorflow/core/framework/types.h"
#include "tensorflow/core/lib/core/status_test_util.h"
#include "tensorflow/core/platform/test.h"

namespace tensorflow {

typedef Eigen::ThreadPoolDevice CPUDevice;
typedef Eigen::GpuDevice GPUDevice;

namespace {

struct VariantValue {
  string TypeName() const { return "TEST VariantValue"; }
  static Status ShapeFn(const VariantValue& v, TensorShape* s) {
    if (v.early_exit) {
      return errors::InvalidArgument("early exit!");
    }
    *s = TensorShape({-0xdeadbeef});
    return Status::OK();
  }
  static Status CPUZerosLikeFn(OpKernelContext* ctx, const VariantValue& v,
                               VariantValue* v_out) {
    if (v.early_exit) {
      return errors::InvalidArgument("early exit zeros_like!");
    }
    v_out->value = 1;  // CPU
    return Status::OK();
  }
  static Status GPUZerosLikeFn(OpKernelContext* ctx, const VariantValue& v,
                               VariantValue* v_out) {
    if (v.early_exit) {
      return errors::InvalidArgument("early exit zeros_like!");
    }
    v_out->value = 2;  // GPU
    return Status::OK();
  }
  static Status CPUAddFn(OpKernelContext* ctx, const VariantValue& a,
                         const VariantValue& b, VariantValue* out) {
    if (a.early_exit) {
      return errors::InvalidArgument("early exit add!");
    }
    out->value = a.value + b.value;  // CPU
    return Status::OK();
  }
  static Status GPUAddFn(OpKernelContext* ctx, const VariantValue& a,
                         const VariantValue& b, VariantValue* out) {
    if (a.early_exit) {
      return errors::InvalidArgument("early exit add!");
    }
    out->value = -(a.value + b.value);  // GPU
    return Status::OK();
  }
  static Status CPUToGPUCopyFn(
      const VariantValue& from, VariantValue* to,
      const std::function<Status(const Tensor&, Tensor*)>& copier) {
    TF_RETURN_IF_ERROR(copier(Tensor(), nullptr));
    to->value = 0xdeadbeef;
    return Status::OK();
  }
  bool early_exit;
  int value;
};

REGISTER_UNARY_VARIANT_SHAPE_FUNCTION(VariantValue, "TEST VariantValue",
                                      VariantValue::ShapeFn);

REGISTER_UNARY_VARIANT_DECODE_FUNCTION(VariantValue, "TEST VariantValue");

INTERNAL_REGISTER_UNARY_VARIANT_DEVICE_COPY_FUNCTION(
    VariantValue, VariantDeviceCopyDirection::HOST_TO_DEVICE,
    "TEST VariantValue", VariantValue::CPUToGPUCopyFn);

REGISTER_UNARY_VARIANT_UNARY_OP_FUNCTION(ZEROS_LIKE_VARIANT_UNARY_OP,
                                         DEVICE_CPU, VariantValue,
                                         "TEST VariantValue",
                                         VariantValue::CPUZerosLikeFn);

REGISTER_UNARY_VARIANT_UNARY_OP_FUNCTION(ZEROS_LIKE_VARIANT_UNARY_OP,
                                         DEVICE_GPU, VariantValue,
                                         "TEST VariantValue",
                                         VariantValue::GPUZerosLikeFn);

REGISTER_UNARY_VARIANT_BINARY_OP_FUNCTION(ADD_VARIANT_BINARY_OP, DEVICE_CPU,
                                          VariantValue, "TEST VariantValue",
                                          VariantValue::CPUAddFn);

REGISTER_UNARY_VARIANT_BINARY_OP_FUNCTION(ADD_VARIANT_BINARY_OP, DEVICE_GPU,
                                          VariantValue, "TEST VariantValue",
                                          VariantValue::GPUAddFn);

}  // namespace

TEST(VariantOpShapeRegistryTest, TestBasic) {
  EXPECT_EQ(UnaryVariantOpRegistry::Global()->GetShapeFn("YOU SHALL NOT PASS"),
            nullptr);

  auto* shape_fn =
      UnaryVariantOpRegistry::Global()->GetShapeFn("TEST VariantValue");
  EXPECT_NE(shape_fn, nullptr);
  TensorShape shape;

  VariantValue vv_early_exit{true /* early_exit */};
  Variant v = vv_early_exit;
  Status s0 = (*shape_fn)(v, &shape);
  EXPECT_FALSE(s0.ok());
  EXPECT_TRUE(StringPiece(s0.error_message()).contains("early exit!"));

  VariantValue vv_ok{false /* early_exit */};
  v = vv_ok;
  TF_EXPECT_OK((*shape_fn)(v, &shape));
  EXPECT_EQ(shape, TensorShape({-0xdeadbeef}));
}

TEST(VariantOpShapeRegistryTest, TestDuplicate) {
  UnaryVariantOpRegistry registry;
  UnaryVariantOpRegistry::VariantShapeFn f;
  string kTypeName = "fjfjfj";
  registry.RegisterShapeFn(kTypeName, f);
  EXPECT_DEATH(registry.RegisterShapeFn(kTypeName, f),
               "fjfjfj already registered");
}

TEST(VariantOpDecodeRegistryTest, TestBasic) {
  EXPECT_EQ(UnaryVariantOpRegistry::Global()->GetDecodeFn("YOU SHALL NOT PASS"),
            nullptr);

  auto* decode_fn =
      UnaryVariantOpRegistry::Global()->GetDecodeFn("TEST VariantValue");
  EXPECT_NE(decode_fn, nullptr);

  VariantValue vv{true /* early_exit */};
  Variant v = vv;
  VariantTensorData data;
  v.Encode(&data);
  VariantTensorDataProto proto;
  data.ToProto(&proto);
  Variant encoded = proto;
  EXPECT_TRUE((*decode_fn)(&encoded));
  VariantValue* decoded = encoded.get<VariantValue>();
  EXPECT_NE(decoded, nullptr);
  EXPECT_EQ(decoded->early_exit, true);
}

TEST(VariantOpDecodeRegistryTest, TestDuplicate) {
  UnaryVariantOpRegistry registry;
  UnaryVariantOpRegistry::VariantDecodeFn f;
  string kTypeName = "fjfjfj";
  registry.RegisterDecodeFn(kTypeName, f);
  EXPECT_DEATH(registry.RegisterDecodeFn(kTypeName, f),
               "fjfjfj already registered");
}

TEST(VariantOpCopyToGPURegistryTest, TestBasic) {
  // No registered copy fn for GPU<->GPU.
  EXPECT_EQ(
      UnaryVariantOpRegistry::Global()->GetDeviceCopyFn(
          VariantDeviceCopyDirection::DEVICE_TO_DEVICE, "TEST VariantValue"),
      nullptr);

  auto* copy_to_gpu_fn = UnaryVariantOpRegistry::Global()->GetDeviceCopyFn(
      VariantDeviceCopyDirection::HOST_TO_DEVICE, "TEST VariantValue");
  EXPECT_NE(copy_to_gpu_fn, nullptr);

  VariantValue vv{true /* early_exit */};
  Variant v = vv;
  Variant v_out;
  bool dummy_executed = false;
  auto dummy_copy_fn = [&dummy_executed](const Tensor& from,
                                         Tensor* to) -> Status {
    dummy_executed = true;
    return Status::OK();
  };
  TF_EXPECT_OK((*copy_to_gpu_fn)(v, &v_out, dummy_copy_fn));
  EXPECT_TRUE(dummy_executed);
  VariantValue* copied_value = v_out.get<VariantValue>();
  EXPECT_NE(copied_value, nullptr);
  EXPECT_EQ(copied_value->value, 0xdeadbeef);
}

TEST(VariantOpCopyToGPURegistryTest, TestDuplicate) {
  UnaryVariantOpRegistry registry;
  UnaryVariantOpRegistry::AsyncVariantDeviceCopyFn f;
  string kTypeName = "fjfjfj";
  registry.RegisterDeviceCopyFn(VariantDeviceCopyDirection::HOST_TO_DEVICE,
                                kTypeName, f);
  EXPECT_DEATH(registry.RegisterDeviceCopyFn(
                   VariantDeviceCopyDirection::HOST_TO_DEVICE, kTypeName, f),
               "fjfjfj already registered");
}

TEST(VariantOpZerosLikeRegistryTest, TestBasicCPU) {
  EXPECT_EQ(UnaryVariantOpRegistry::Global()->GetUnaryOpFn(
                ZEROS_LIKE_VARIANT_UNARY_OP, DEVICE_CPU, "YOU SHALL NOT PASS"),
            nullptr);

  VariantValue vv_early_exit{true /* early_exit */, 0 /* value */};
  Variant v = vv_early_exit;
  Variant v_out = VariantValue();

  OpKernelContext* null_context_pointer = nullptr;
  Status s0 = UnaryOpVariant<CPUDevice>(null_context_pointer,
                                        ZEROS_LIKE_VARIANT_UNARY_OP, v, &v_out);
  EXPECT_FALSE(s0.ok());
  EXPECT_TRUE(
      StringPiece(s0.error_message()).contains("early exit zeros_like"));

  VariantValue vv_ok{false /* early_exit */, 0 /* value */};
  v = vv_ok;
  TF_EXPECT_OK(UnaryOpVariant<CPUDevice>(
      null_context_pointer, ZEROS_LIKE_VARIANT_UNARY_OP, v, &v_out));
  VariantValue* vv_out = CHECK_NOTNULL(v_out.get<VariantValue>());
  EXPECT_EQ(vv_out->value, 1);  // CPU
}

#if GOOGLE_CUDA
TEST(VariantOpUnaryOpRegistryTest, TestBasicGPU) {
  EXPECT_EQ(UnaryVariantOpRegistry::Global()->GetUnaryOpFn(
                ZEROS_LIKE_VARIANT_UNARY_OP, DEVICE_GPU, "YOU SHALL NOT PASS"),
            nullptr);

  VariantValue vv_early_exit{true /* early_exit */, 0 /* value */};
  Variant v = vv_early_exit;
  Variant v_out = VariantValue();

  OpKernelContext* null_context_pointer = nullptr;
  Status s0 = UnaryOpVariant<GPUDevice>(null_context_pointer,
                                        ZEROS_LIKE_VARIANT_UNARY_OP, v, &v_out);
  EXPECT_FALSE(s0.ok());
  EXPECT_TRUE(
      StringPiece(s0.error_message()).contains("early exit zeros_like"));

  VariantValue vv_ok{false /* early_exit */, 0 /* value */};
  v = vv_ok;
  TF_EXPECT_OK(UnaryOpVariant<GPUDevice>(
      null_context_pointer, ZEROS_LIKE_VARIANT_UNARY_OP, v, &v_out));
  VariantValue* vv_out = CHECK_NOTNULL(v_out.get<VariantValue>());
  EXPECT_EQ(vv_out->value, 2);  // GPU
}
#endif  // GOOGLE_CUDA

TEST(VariantOpUnaryOpRegistryTest, TestDuplicate) {
  UnaryVariantOpRegistry registry;
  UnaryVariantOpRegistry::VariantUnaryOpFn f;
  string kTypeName = "fjfjfj";

  registry.RegisterUnaryOpFn(ZEROS_LIKE_VARIANT_UNARY_OP, DEVICE_CPU, kTypeName,
                             f);
  EXPECT_DEATH(registry.RegisterUnaryOpFn(ZEROS_LIKE_VARIANT_UNARY_OP,
                                          DEVICE_CPU, kTypeName, f),
               "fjfjfj already registered");

  registry.RegisterUnaryOpFn(ZEROS_LIKE_VARIANT_UNARY_OP, DEVICE_GPU, kTypeName,
                             f);
  EXPECT_DEATH(registry.RegisterUnaryOpFn(ZEROS_LIKE_VARIANT_UNARY_OP,
                                          DEVICE_GPU, kTypeName, f),
               "fjfjfj already registered");
}

TEST(VariantOpAddRegistryTest, TestBasicCPU) {
  return;
  EXPECT_EQ(UnaryVariantOpRegistry::Global()->GetBinaryOpFn(
                ADD_VARIANT_BINARY_OP, DEVICE_CPU, "YOU SHALL NOT PASS"),
            nullptr);

  VariantValue vv_early_exit{true /* early_exit */, 3 /* value */};
  VariantValue vv_other{true /* early_exit */, 4 /* value */};
  Variant v_a = vv_early_exit;
  Variant v_b = vv_other;
  Variant v_out = VariantValue();

  OpKernelContext* null_context_pointer = nullptr;
  Status s0 = BinaryOpVariants<CPUDevice>(
      null_context_pointer, ADD_VARIANT_BINARY_OP, v_a, v_b, &v_out);
  EXPECT_FALSE(s0.ok());
  EXPECT_TRUE(StringPiece(s0.error_message()).contains("early exit add"));

  VariantValue vv_ok{false /* early_exit */, 3 /* value */};
  v_a = vv_ok;
  TF_EXPECT_OK(BinaryOpVariants<CPUDevice>(
      null_context_pointer, ADD_VARIANT_BINARY_OP, v_a, v_b, &v_out));
  VariantValue* vv_out = CHECK_NOTNULL(v_out.get<VariantValue>());
  EXPECT_EQ(vv_out->value, 7);  // CPU
}

#if GOOGLE_CUDA
TEST(VariantOpAddRegistryTest, TestBasicGPU) {
  EXPECT_EQ(UnaryVariantOpRegistry::Global()->GetBinaryOpFn(
                ADD_VARIANT_BINARY_OP, DEVICE_GPU, "YOU SHALL NOT PASS"),
            nullptr);

  VariantValue vv_early_exit{true /* early_exit */, 3 /* value */};
  VariantValue vv_other{true /* early_exit */, 4 /* value */};
  Variant v_a = vv_early_exit;
  Variant v_b = vv_other;
  Variant v_out = VariantValue();

  OpKernelContext* null_context_pointer = nullptr;
  Status s0 = BinaryOpVariants<GPUDevice>(
      null_context_pointer, ADD_VARIANT_BINARY_OP, v_a, v_b, &v_out);
  EXPECT_FALSE(s0.ok());
  EXPECT_TRUE(StringPiece(s0.error_message()).contains("early exit add"));

  VariantValue vv_ok{false /* early_exit */, 3 /* value */};
  v_a = vv_ok;
  TF_EXPECT_OK(BinaryOpVariants<GPUDevice>(
      null_context_pointer, ADD_VARIANT_BINARY_OP, v_a, v_b, &v_out));
  VariantValue* vv_out = CHECK_NOTNULL(v_out.get<VariantValue>());
  EXPECT_EQ(vv_out->value, -7);  // GPU
}
#endif  // GOOGLE_CUDA

TEST(VariantOpAddRegistryTest, TestDuplicate) {
  UnaryVariantOpRegistry registry;
  UnaryVariantOpRegistry::VariantBinaryOpFn f;
  string kTypeName = "fjfjfj";

  registry.RegisterBinaryOpFn(ADD_VARIANT_BINARY_OP, DEVICE_CPU, kTypeName, f);
  EXPECT_DEATH(registry.RegisterBinaryOpFn(ADD_VARIANT_BINARY_OP, DEVICE_CPU,
                                           kTypeName, f),
               "fjfjfj already registered");

  registry.RegisterBinaryOpFn(ADD_VARIANT_BINARY_OP, DEVICE_GPU, kTypeName, f);
  EXPECT_DEATH(registry.RegisterBinaryOpFn(ADD_VARIANT_BINARY_OP, DEVICE_GPU,
                                           kTypeName, f),
               "fjfjfj already registered");
}

}  // namespace tensorflow
