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

#ifndef TENSORFLOW_KERNELS_SNAPSHOT_OP_H_
#define TENSORFLOW_KERNELS_SNAPSHOT_OP_H_

#if GOOGLE_CUDA
#define EIGEN_USE_GPU
#endif

#define EIGEN_USE_THREADS

#include "third_party/eigen3/unsupported/Eigen/CXX11/Tensor"
#include "tensorflow/core/framework/op_kernel.h"

namespace tensorflow {

template <typename Device, typename Scalar>
class SnapshotOp : public OpKernel {
 public:
  explicit SnapshotOp(OpKernelConstruction* context) : OpKernel(context) {}

  void Compute(OpKernelContext* context) override {
    const Tensor& input = context->input(0);
    Tensor* output = nullptr;
    // Try to use buffer forwarding to avoid an explicit copy.
    OP_REQUIRES_OK(context, context->forward_input_or_allocate_output(
                                {0}, 0, input.shape(), &output));
    if (!output->SharesBufferWith(input)) {
      // We had to allocate a new buffer since the refcount on the input was
      // greater than 1. Copy the input to the new buffer.
      const Device& device = context->eigen_device<Device>();
      device.memcpy(output->template flat<Scalar>().data(),
                    input.template flat<Scalar>().data(),
                    input.NumElements() * sizeof(Scalar));
    }
  }
};

}  // namespace tensorflow

#endif  // TENSORFLOW_KERNELS_SNAPSHOT_OP_H_
