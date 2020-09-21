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

// Our general strategy for preventing conflicts between concurrent
// reads and writes of resource variables is to:
// * For read operations, we:
//   - acquire the variable's mutex (in "shared" mode);
//   - make a (shallow) copy of the Tensor object, which increments
//     the reference count on the variable's TensorBuffer;
//   - release the variable's mutex;
//   - use the copy of the Tensor object to do the read.
// * For write operations, we:
//   - acquire the variable's mutex (in "exclusive" mode);
//   - check the reference count of variable's TensorBuffer and
//     if it is >1, make a deep copy of the variable's Tensor;
//   - mutate the variable's Tensor;
//   - and release the variable's mutex.
// This allows several read operations to all use the same
// TensorBuffer without needing to copy. When it comes time to write
// it will only make a copy if there is an outstanding read using the
// buffer. Write operations are serialized by the variable's mutex.
//
// For sparse operations (scatter, gather, sparse optimizer updates),
// we need to avoid copies, since there may not be enough memory for
// to copies of the whole tensor. To support this, we make two
// modifications to the above strategy:
// * For sparse reads (gather), we hold the variable's mutex (still in
//   "shared" mode) for the duration of the whole read. This means
//   that as long as you only do sparse read operations no write will
//   see the reference count >1.
// * For sparse write operations where the user explicitly specifies
//   that they want to perform the write without locks held
//   (use_locking=false), we never copy even if the variable's
//   reference count is >1.

#define EIGEN_USE_THREADS

#if GOOGLE_CUDA
#define EIGEN_USE_GPU
#endif

#include "tensorflow/core/framework/op_kernel.h"
#include "tensorflow/core/framework/register_types.h"
#include "tensorflow/core/framework/resource_mgr.h"
#include "tensorflow/core/framework/tensor_types.h"
#include "tensorflow/core/framework/variant_op_registry.h"
#include "tensorflow/core/kernels/bounds_check.h"
#include "tensorflow/core/kernels/dense_update_functor.h"
#include "tensorflow/core/kernels/gather_functor.h"
#include "tensorflow/core/kernels/scatter_functor.h"
#include "tensorflow/core/kernels/training_op_helpers.h"
#include "tensorflow/core/kernels/variable_ops.h"
#include "tensorflow/core/lib/core/errors.h"
#include "tensorflow/core/platform/mem.h"
#include "tensorflow/core/platform/mutex.h"
#include "tensorflow/core/platform/types.h"
#include "tensorflow/core/util/util.h"

namespace tensorflow {

REGISTER_RESOURCE_HANDLE_KERNEL(Var);

class ReadVariableOp : public OpKernel {
 public:
  explicit ReadVariableOp(OpKernelConstruction* c) : OpKernel(c) {
    OP_REQUIRES_OK(c, c->GetAttr("dtype", &dtype_));
  }

  void Compute(OpKernelContext* ctx) override {
    Var* variable = nullptr;
    ResourceHandle handle = HandleFromInput(ctx, 0);
    const auto status = LookupResource(ctx, handle, &variable);
    OP_REQUIRES(ctx, status.ok(),
                errors::FailedPrecondition(
                    "Error while reading resource variable ", handle.name(),
                    " from Container: ", handle.container(),
                    ". This could mean that the variable was uninitialized. ",
                    status.ToString()));

    core::ScopedUnref s(variable);
    // We're acquiring a reference to the underlying buffer while
    // holding a shared lock to guarantee ordering of reads and
    // writes.
    tf_shared_lock ml(*variable->mu());
    const Tensor& t = *variable->tensor();
    OP_REQUIRES(
        ctx, dtype_ == t.dtype(),
        errors::InvalidArgument(
            "Trying to read variable with wrong dtype. Expected ",
            DataTypeString(dtype_), " got ", DataTypeString(t.dtype())));
    ctx->set_output(0, t);
  }

 private:
  DataType dtype_;
};

REGISTER_KERNEL_BUILDER(Name("ReadVariableOp").Device(DEVICE_CPU),
                        ReadVariableOp);

#if GOOGLE_CUDA
REGISTER_KERNEL_BUILDER(
    Name("ReadVariableOp").Device(DEVICE_GPU).HostMemory("resource"),
    ReadVariableOp);

#define REGISTER_GPU_KERNELS(type)                             \
  namespace functor {                                          \
  template <>                                                  \
  void DenseUpdate<GPUDevice, type, ASSIGN>::operator()(       \
      const GPUDevice& d, typename TTypes<type>::Flat lhs,     \
      typename TTypes<type>::ConstFlat rhs);                   \
  extern template struct DenseUpdate<GPUDevice, type, ASSIGN>; \
  }                                                            \
  REGISTER_KERNEL_BUILDER(Name("VarHandleOp")                  \
                              .Device(DEVICE_GPU)              \
                              .HostMemory("resource")          \
                              .TypeConstraint<type>("dtype"),  \
                          ResourceHandleOp<Var>)

TF_CALL_GPU_ALL_TYPES(REGISTER_GPU_KERNELS);
TF_CALL_int64(REGISTER_GPU_KERNELS);
TF_CALL_variant(REGISTER_GPU_KERNELS);
#undef REGISTER_GPU_KERNELS
#endif  // GOOGLE_CUDA

template <typename T>
class VariableShapeOp : public OpKernel {
 public:
  explicit VariableShapeOp(OpKernelConstruction* c) : OpKernel(c) {}

  void Compute(OpKernelContext* ctx) override {
    Var* variable = nullptr;
    OP_REQUIRES_OK(ctx,
                   LookupResource(ctx, HandleFromInput(ctx, 0), &variable));
    core::ScopedUnref s(variable);
    variable->mu()->lock_shared();
    TensorShape shape = variable->tensor()->shape();
    variable->mu()->unlock_shared();
    Tensor* output;
    OP_REQUIRES_OK(ctx, ctx->allocate_output(0, {shape.dims()}, &output));
    for (int i = 0; i < shape.dims(); ++i) {
      output->flat<T>()(i) = shape.dim_size(i);
    }
  }
};

REGISTER_KERNEL_BUILDER(
    Name("VariableShape").Device(DEVICE_CPU).TypeConstraint<int32>("out_type"),
    VariableShapeOp<int32>);
REGISTER_KERNEL_BUILDER(
    Name("VariableShape").Device(DEVICE_CPU).TypeConstraint<int64>("out_type"),
    VariableShapeOp<int64>);

#if GOOGLE_CUDA

REGISTER_KERNEL_BUILDER(Name("VariableShape")
                            .Device(DEVICE_GPU)
                            .TypeConstraint<int32>("out_type")
                            .HostMemory("output")
                            .HostMemory("input"),
                        VariableShapeOp<int32>);
REGISTER_KERNEL_BUILDER(Name("VariableShape")
                            .Device(DEVICE_GPU)
                            .TypeConstraint<int64>("out_type")
                            .HostMemory("output")
                            .HostMemory("input"),
                        VariableShapeOp<int64>);

#endif  // GOOGLE_CUDA

class DestroyResourceOp : public OpKernel {
 public:
  explicit DestroyResourceOp(OpKernelConstruction* ctx) : OpKernel(ctx) {
    OP_REQUIRES_OK(ctx,
                   ctx->GetAttr("ignore_lookup_error", &ignore_lookup_error_));
  }

  void Compute(OpKernelContext* ctx) override {
    const ResourceHandle& p = HandleFromInput(ctx, 0);
    Status status = DeleteResource(ctx, p);
    if (ignore_lookup_error_ && errors::IsNotFound(status)) {
      return;
    }
    OP_REQUIRES_OK(ctx, status);
  }

 private:
  bool ignore_lookup_error_;
};

REGISTER_KERNEL_BUILDER(Name("DestroyResourceOp").Device(DEVICE_CPU),
                        DestroyResourceOp);
REGISTER_KERNEL_BUILDER(
    Name("DestroyResourceOp").Device(DEVICE_GPU).HostMemory("resource"),
    DestroyResourceOp);

template <typename Device, typename T>
class AssignVariableOp : public OpKernel {
 public:
  explicit AssignVariableOp(OpKernelConstruction* c) : OpKernel(c) {
    OP_REQUIRES_OK(c, c->GetAttr("dtype", &dtype_));
  }

  void Compute(OpKernelContext* context) override {
    OP_REQUIRES(context, dtype_ == context->input(1).dtype(),
                errors::InvalidArgument(
                    "Variable and value dtypes don't match; respectively, ",
                    dtype_, " and ", context->input(1).dtype()));
    Var* variable = nullptr;
    OP_REQUIRES_OK(
        context,
        LookupOrCreateResource<Var>(
            context, HandleFromInput(context, 0), &variable,
            [this, context](Var** ptr) {
              *ptr = new Var(dtype_);
              PersistentTensor unused;
              Tensor* tmp;
              AllocatorAttributes attr;
              attr.set_gpu_compatible(true);
              attr.set_nic_compatible(true);
              TF_RETURN_IF_ERROR(context->allocate_persistent(
                  dtype_, context->input(1).shape(), &unused, &tmp, attr));
              *(*ptr)->tensor() = *tmp;
              return Status::OK();
            }));
    core::ScopedUnref s(variable);

    OP_REQUIRES(context, variable->tensor()->dtype() == dtype_,
                errors::InvalidArgument(
                    "Trying to assign variable with wrong dtype. Expected ",
                    DataTypeString(variable->tensor()->dtype()), " got ",
                    DataTypeString(dtype_)));

    const Tensor& value = context->input(1);
    AllocatorAttributes attr;
    attr.set_gpu_compatible(true);
    attr.set_nic_compatible(true);

    // Copying is unnecessary if we are the last user of the value
    // tensor, we can just adopt the input tensor's buffer instead.
    std::unique_ptr<Tensor> input_alias =
        context->forward_input(1, dtype_, value.shape(), DEVICE_MEMORY, attr);
    mutex_lock ml(*variable->mu());
    if (input_alias) {
      *variable->tensor() = *input_alias;
      return;
    }

    // Need to copy, but maybe we can re-use variable's buffer?
    if (!variable->tensor()->RefCountIsOne() ||
        !variable->tensor()->shape().IsSameSize(value.shape())) {
      // Copy to new buffer
      PersistentTensor unused;
      Tensor* tmp;
      OP_REQUIRES_OK(context, context->allocate_persistent(
                                  dtype_, value.shape(), &unused, &tmp, attr));
      *variable->tensor() = *tmp;
    }
    functor::DenseUpdate<Device, T, ASSIGN> copy_functor;
    copy_functor(context->eigen_device<Device>(), variable->tensor()->flat<T>(),
                 value.flat<T>());
  }

 private:
  DataType dtype_;
};

template <typename Device>
Status VariantCopyFn(OpKernelContext* context, const Tensor& from, Tensor* to);

#define CPU_DENSE_COPY(T)                                                \
  case DataTypeToEnum<T>::value: {                                       \
    functor::DenseUpdate<CPUDevice, T, ASSIGN> copy_functor_;            \
    copy_functor_(context->eigen_device<CPUDevice>(), tensor->flat<T>(), \
                  from.flat<T>());                                       \
    break;                                                               \
  }

#define INSTANTIATE_GET_VARIANT_COPY_FN(Device, TYPE_CALLER, TYPE_DENSE_COPY) \
  template <>                                                                 \
  Status VariantCopyFn<Device>(OpKernelContext * context, const Tensor& from, \
                               Tensor* to) {                                  \
    PersistentTensor tmp;                                                     \
    Tensor* tensor;                                                           \
    AllocatorAttributes attr;                                                 \
    attr.set_gpu_compatible(true);                                            \
    attr.set_nic_compatible(true);                                            \
    TF_RETURN_IF_ERROR(context->allocate_persistent(                          \
        from.dtype(), from.shape(), &tmp, &tensor, attr));                    \
    switch (from.dtype()) {                                                   \
      TYPE_CALLER(TYPE_DENSE_COPY);                                           \
      default:                                                                \
        return errors::InvalidArgument(                                       \
            "VariantCopyFn: Could not perform a deep copy of variant "        \
            "element of type: ",                                              \
            DataTypeString(from.dtype()),                                     \
            " using device: ", context->device()->name());                    \
    }                                                                         \
    *to = *tensor;                                                            \
    return Status::OK();                                                      \
  }

INSTANTIATE_GET_VARIANT_COPY_FN(CPUDevice, TF_CALL_ALL_TYPES, CPU_DENSE_COPY);

#if GOOGLE_CUDA
#define GPU_DENSE_COPY(T)                                                \
  case DataTypeToEnum<T>::value: {                                       \
    functor::DenseUpdate<GPUDevice, T, ASSIGN> copy_functor_;            \
    copy_functor_(context->eigen_device<GPUDevice>(), tensor->flat<T>(), \
                  from.flat<T>());                                       \
    break;                                                               \
  }
#define TF_CALL_GPU_AND_ADDITIONAL_TYPES(T) \
  TF_CALL_GPU_ALL_TYPES(T);                 \
  TF_CALL_int32(T);                         \
  TF_CALL_int64(T);
INSTANTIATE_GET_VARIANT_COPY_FN(GPUDevice, TF_CALL_GPU_AND_ADDITIONAL_TYPES,
                                GPU_DENSE_COPY);
#undef TF_CALL_GPU_AND_ADDITIONAL_TYPES
#undef GPU_DENSE_COPY
#endif  // GOOGLE_CUDA

#undef CPU_DENSE_COPY
#undef INSTANTIATE_GET_VARIANT_COPY_FN

template <typename Device>
class AssignVariableOp<Device, Variant> : public OpKernel {
 public:
  explicit AssignVariableOp(OpKernelConstruction* c) : OpKernel(c) {
    OP_REQUIRES_OK(c, c->GetAttr("dtype", &dtype_));
    OP_REQUIRES(c, dtype_ == DT_VARIANT,
                errors::Internal("Variant kernel called with dtype: ",
                                 DataTypeString(dtype_)));
  }

  void Compute(OpKernelContext* context) override {
    const Tensor& value = context->input(1);
    Var* variable = nullptr;
    OP_REQUIRES_OK(context, LookupOrCreateResource<Var>(
                                context, HandleFromInput(context, 0), &variable,
                                [this, context](Var** ptr) {
                                  // Created on host.
                                  *ptr = new Var(DT_VARIANT);
                                  return Status::OK();
                                }));
    core::ScopedUnref s(variable);
    OP_REQUIRES(context, variable->tensor()->dtype() == DT_VARIANT,
                errors::InvalidArgument(
                    "Trying to assign variable with wrong dtype. Expected ",
                    DataTypeString(variable->tensor()->dtype()), " got ",
                    DataTypeString(DT_VARIANT)));

    mutex_lock ml(*variable->mu());

    *variable->tensor() = Tensor(DT_VARIANT, value.shape());
    const auto elements_in = value.flat<Variant>();
    auto elements_out = variable->tensor()->flat<Variant>();
    auto copy_fn = std::bind(&VariantCopyFn<Device>, context,
                             std::placeholders::_1, std::placeholders::_2);
    for (int64 i = 0; i < elements_in.size(); ++i) {
      OP_REQUIRES_OK(context, VariantDeviceCopy(
                                  VariantDeviceCopyDirection::DEVICE_TO_DEVICE,
                                  elements_in(i), &elements_out(i), copy_fn));
    };
  }

 private:
  DataType dtype_;
};

#define REGISTER_KERNELS(type)                                \
  REGISTER_KERNEL_BUILDER(Name("AssignVariableOp")            \
                              .Device(DEVICE_CPU)             \
                              .TypeConstraint<type>("dtype"), \
                          AssignVariableOp<Eigen::ThreadPoolDevice, type>);

TF_CALL_ALL_TYPES(REGISTER_KERNELS);
TF_CALL_QUANTIZED_TYPES(REGISTER_KERNELS);
#undef REGISTER_KERNELS

#if GOOGLE_CUDA
#define REGISTER_GPU_KERNELS(type)                           \
  REGISTER_KERNEL_BUILDER(Name("AssignVariableOp")           \
                              .Device(DEVICE_GPU)            \
                              .TypeConstraint<type>("dtype") \
                              .HostMemory("resource"),       \
                          AssignVariableOp<GPUDevice, type>);

TF_CALL_GPU_ALL_TYPES(REGISTER_GPU_KERNELS);
TF_CALL_int64(REGISTER_GPU_KERNELS);
TF_CALL_variant(REGISTER_GPU_KERNELS);
#undef REGISTER_GPU_KERNELS
#endif  // GOOGLE_CUDA

template <typename Device, typename T, DenseUpdateType Op>
class AssignUpdateVariableOp : public OpKernel {
 public:
  explicit AssignUpdateVariableOp(OpKernelConstruction* c) : OpKernel(c) {}

  void Compute(OpKernelContext* context) override {
    Var* variable = nullptr;
    OP_REQUIRES_OK(context, LookupResource(context, HandleFromInput(context, 0),
                                           &variable));
    core::ScopedUnref s(variable);

    const Tensor& value = context->input(1);
    // TODO(apassos): We could possibly avoid the copy done by
    // PrepareToUpdateVariable() for commutative operations like Op ==
    // ADD if value's refcount was 1.
    mutex_lock ml(*variable->mu());
    Tensor* var_tensor = variable->tensor();
    OP_REQUIRES_OK(context,
                   PrepareToUpdateVariable<Device, T>(context, var_tensor));
    functor::DenseUpdate<Device, T, Op> update_functor;
    update_functor(context->eigen_device<Device>(), var_tensor->flat<T>(),
                   value.flat<T>());
  }
};

#define REGISTER_KERNELS(type)                                     \
  REGISTER_KERNEL_BUILDER(                                         \
      Name("AssignAddVariableOp")                                  \
          .Device(DEVICE_CPU)                                      \
          .TypeConstraint<type>("dtype"),                          \
      AssignUpdateVariableOp<Eigen::ThreadPoolDevice, type, ADD>); \
  REGISTER_KERNEL_BUILDER(                                         \
      Name("AssignSubVariableOp")                                  \
          .Device(DEVICE_CPU)                                      \
          .TypeConstraint<type>("dtype"),                          \
      AssignUpdateVariableOp<Eigen::ThreadPoolDevice, type, SUB>);

TF_CALL_NUMBER_TYPES(REGISTER_KERNELS);
#undef REGISTER_KERNELS

#if GOOGLE_CUDA
#define REGISTER_GPU_KERNELS(type)                                       \
  REGISTER_KERNEL_BUILDER(Name("AssignAddVariableOp")                    \
                              .Device(DEVICE_GPU)                        \
                              .HostMemory("resource")                    \
                              .TypeConstraint<type>("dtype"),            \
                          AssignUpdateVariableOp<GPUDevice, type, ADD>); \
  REGISTER_KERNEL_BUILDER(Name("AssignSubVariableOp")                    \
                              .Device(DEVICE_GPU)                        \
                              .HostMemory("resource")                    \
                              .TypeConstraint<type>("dtype"),            \
                          AssignUpdateVariableOp<GPUDevice, type, SUB>);

TF_CALL_GPU_NUMBER_TYPES(REGISTER_GPU_KERNELS);
TF_CALL_int64(REGISTER_GPU_KERNELS);
#undef REGISTER_GPU_KERNELS
#endif  // GOOGLE_CUDA

REGISTER_KERNEL_BUILDER(Name("VarIsInitializedOp").Device(DEVICE_CPU),
                        IsResourceInitialized<Var>);

#if GOOGLE_CUDA
REGISTER_KERNEL_BUILDER(Name("VarIsInitializedOp")
                            .Device(DEVICE_GPU)
                            .HostMemory("resource")
                            .HostMemory("is_initialized"),
                        IsResourceInitialized<Var>);
#endif  // GOOGLE_CUDA

template <typename Device, typename T, typename Index>
class ResourceGatherOp : public OpKernel {
 public:
  explicit ResourceGatherOp(OpKernelConstruction* c) : OpKernel(c) {}

  void Compute(OpKernelContext* c) override {
    Var* v = nullptr;
    OP_REQUIRES_OK(c, LookupResource(c, HandleFromInput(c, 0), &v));
    // NOTE: We hold the lock for the whole gather operation instead
    // of increasing the reference count of v->tensor() to avoid a
    // situation where a write to the same variable will see a
    // reference count greater than one and make a copy of the
    // (potentially very large) tensor buffer.
    tf_shared_lock ml(*v->mu());
    const Tensor& params = *v->tensor();
    const Tensor& indices = c->input(1);
    OP_REQUIRES(
        c, TensorShapeUtils::IsVectorOrHigher(params.shape()),
        errors::InvalidArgument("params must be at least 1 dimensional"));

    // Check that we have enough index space
    const int64 N = indices.NumElements();
    OP_REQUIRES(
        c, params.dim_size(0) <= std::numeric_limits<Index>::max(),
        errors::InvalidArgument("params.shape[0] too large for ",
                                DataTypeString(DataTypeToEnum<Index>::v()),
                                " indexing: ", params.dim_size(0), " > ",
                                std::numeric_limits<Index>::max()));

    // The result shape is indices.shape + params.shape[1:].
    TensorShape result_shape = indices.shape();
    for (int i = 1; i < params.dims(); i++) {
      result_shape.AddDim(params.dim_size(i));
    }

    Tensor* out = nullptr;
    OP_REQUIRES_OK(c, c->allocate_output(0, result_shape, &out));
    if (N > 0) {
      const int64 gather_dim_size = params.dim_size(0);
      int64 inner_size = 1;
      for (int i = 1; i < params.dims(); i++) {
        inner_size *= params.dim_size(i);
      }
      auto params_flat = params.shaped<T, 3>({1, gather_dim_size, inner_size});
      auto indices_flat = indices.flat<Index>();
      auto out_flat = out->shaped<T, 3>({1, N, out->NumElements() / N});

      functor::GatherFunctor<Device, T, Index> functor;
      int64 bad_i = functor(c, params_flat, indices_flat, out_flat);

      OP_REQUIRES(
          c, bad_i < 0,
          errors::InvalidArgument(
              "indices", SliceDebugString(indices.shape(), bad_i), " = ",
              indices_flat(bad_i), " is not in [0, ", params.dim_size(0), ")"));
    }
  }
};

#define REGISTER_GATHER_FULL(dev, type, index_type)                    \
  REGISTER_KERNEL_BUILDER(Name("ResourceGather")                       \
                              .Device(DEVICE_##dev)                    \
                              .HostMemory("resource")                  \
                              .TypeConstraint<type>("dtype")           \
                              .TypeConstraint<index_type>("Tindices"), \
                          ResourceGatherOp<dev##Device, type, index_type>)

#define REGISTER_GATHER_ALL_INDICES(dev, type) \
  REGISTER_GATHER_FULL(dev, type, int32);      \
  REGISTER_GATHER_FULL(dev, type, int64)

#define REGISTER_GATHER_CPU(type) REGISTER_GATHER_ALL_INDICES(CPU, type)

// Registration of the CPU implementations.
TF_CALL_ALL_TYPES(REGISTER_GATHER_CPU);
TF_CALL_QUANTIZED_TYPES(REGISTER_GATHER_CPU);

// Registers GPU kernels.
#if GOOGLE_CUDA
#define REGISTER_GATHER_GPU(type) REGISTER_GATHER_ALL_INDICES(GPU, type)

TF_CALL_GPU_NUMBER_TYPES_NO_HALF(REGISTER_GATHER_GPU);

#endif  // GOOGLE_CUDA

#undef REGISTER_GATHER_CPU
#undef REGISTER_GATHER_GPU
#undef REGISTER_GATHER_ALL_INDICES
#undef REGISTER_GATHER_FULL

template <typename Device, typename T, typename Index, scatter_op::UpdateOp op>
class ResourceScatterUpdateOp : public OpKernel {
 public:
  explicit ResourceScatterUpdateOp(OpKernelConstruction* c) : OpKernel(c) {}

  void Compute(OpKernelContext* c) override {
    Var* v = nullptr;
    OP_REQUIRES_OK(c, LookupResource(c, HandleFromInput(c, 0), &v));
    core::ScopedUnref unref_v(v);
    mutex_lock ml(*v->mu());
    Tensor* params = v->tensor();
    OP_REQUIRES_OK(c, PrepareToUpdateVariable<Device, T>(c, params));
    const Tensor& indices = c->input(1);
    const Tensor& updates = c->input(2);

    // Check that we have enough index space
    const int64 N_big = indices.NumElements();
    OP_REQUIRES(
        c, N_big <= std::numeric_limits<Index>::max(),
        errors::InvalidArgument("indices has too many elements for ",
                                DataTypeString(DataTypeToEnum<Index>::v()),
                                " indexing: ", N_big, " > ",
                                std::numeric_limits<Index>::max()));
    const Index N = static_cast<Index>(indices.NumElements());
    OP_REQUIRES(
        c, params->dim_size(0) <= std::numeric_limits<Index>::max(),
        errors::InvalidArgument("params.shape[0] too large for ",
                                DataTypeString(DataTypeToEnum<Index>::v()),
                                " indexing: ", params->dim_size(0), " > ",
                                std::numeric_limits<Index>::max()));

    if (N > 0) {
      auto indices_flat = indices.flat<Index>();
      auto params_flat = params->flat_outer_dims<T>();
      auto updates_flat = updates.shaped<T, 2>({N, updates.NumElements() / N});

      functor::ScatterFunctor<Device, T, Index, op> functor;
      const Index bad_i = functor(c, c->template eigen_device<Device>(),
                                  params_flat, updates_flat, indices_flat);
      OP_REQUIRES(c, bad_i < 0,
                  errors::InvalidArgument(
                      "indices", SliceDebugString(indices.shape(), bad_i),
                      " = ", indices_flat(bad_i), " is not in [0, ",
                      params->dim_size(0), ")"));
    }
  }
};

#define REGISTER_SCATTER_KERNEL_INDEX(type, index_type, dev, name, op) \
  REGISTER_KERNEL_BUILDER(                                             \
      Name(name)                                                       \
          .Device(DEVICE_##dev)                                        \
          .HostMemory("resource")                                      \
          .TypeConstraint<type>("dtype")                               \
          .TypeConstraint<index_type>("Tindices"),                     \
      ResourceScatterUpdateOp<dev##Device, type, index_type, op>)

#define REGISTER_SCATTER_KERNEL(type, dev, name, op)         \
  REGISTER_SCATTER_KERNEL_INDEX(type, int32, dev, name, op); \
  REGISTER_SCATTER_KERNEL_INDEX(type, int64, dev, name, op);

// TODO(apassos) add the other types here.
#define REGISTER_SCATTER_ARITHEMTIC(type, dev)                \
  REGISTER_SCATTER_KERNEL(type, dev, "ResourceScatterAdd",    \
                          scatter_op::UpdateOp::ADD);         \
  REGISTER_SCATTER_KERNEL(type, dev, "ResourceScatterUpdate", \
                          scatter_op::UpdateOp::ASSIGN);

// Registers CPU kernels.
#define REGISTER_SCATTER_ARITHEMTIC_CPU(type) \
  REGISTER_SCATTER_ARITHEMTIC(type, CPU);

TF_CALL_NUMBER_TYPES(REGISTER_SCATTER_ARITHEMTIC_CPU);

REGISTER_SCATTER_KERNEL(string, CPU, "ResourceScatterUpdate",
                        scatter_op::UpdateOp::ASSIGN);

// Registers GPU kernels.
#if GOOGLE_CUDA
#define REGISTER_SCATTER_ARITHEMTIC_GPU(type) \
  REGISTER_SCATTER_ARITHEMTIC(type, GPU);

#define REGISTER_SCATTER_UPDATE_GPU(type) REGISTER_SCATTER_UPDATE(type, GPU);

TF_CALL_GPU_NUMBER_TYPES_NO_HALF(REGISTER_SCATTER_ARITHEMTIC_GPU);

#endif  // GOOGLE_CUDA

#undef REGISTER_SCATTER_ARITHEMTIC
#undef REGISTER_SCATTER_ARITHEMTIC_CPU
#undef REGISTER_SCATTER_KERNEL
#undef REGISTER_SCATTER_KERNEL_INDEX

}  // namespace tensorflow
