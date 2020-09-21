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

#include "tensorflow/compiler/tf2xla/literal_util.h"

#include "tensorflow/compiler/tf2xla/shape_util.h"
#include "tensorflow/compiler/tf2xla/type_util.h"
#include "tensorflow/compiler/xla/literal_util.h"
#include "tensorflow/core/common_runtime/dma_helper.h"

namespace tensorflow {

Status HostTensorToLiteral(const Tensor& host_tensor, xla::Literal* literal) {
  xla::Shape literal_shape;
  TF_RETURN_IF_ERROR(TensorShapeToXLAShape(
      host_tensor.dtype(), host_tensor.shape(), &literal_shape));

  *literal = xla::Literal(literal_shape);

  // memcpy over the payload ...
  // TODO(phawkins): handle string types.
  size_t total_bytes = host_tensor.TotalBytes();
  if (total_bytes > 0) {
    void* dst_ptr = literal->untyped_data();
    const void* src_ptr = DMAHelper::base(&host_tensor);
    memcpy(dst_ptr, src_ptr, total_bytes);
  }
  return Status::OK();
}

Status CopyLiteralToHostTensor(const xla::Literal& literal,
                               Tensor* host_tensor) {
  TF_RET_CHECK(xla::ShapeUtil::IsArray(literal.shape()) &&
               xla::ShapeUtil::ElementsIn(literal.shape()) ==
                   host_tensor->NumElements());
  xla::PrimitiveType primitive_type;
  TF_RETURN_IF_ERROR(
      DataTypeToPrimitiveType(host_tensor->dtype(), &primitive_type));
  if (literal.shape().element_type() != primitive_type) {
    return errors::InvalidArgument(
        "Cannot convert literal of type ",
        xla::PrimitiveType_Name(literal.shape().element_type()),
        " to tensor of type ", DataTypeString(host_tensor->dtype()));
  }
  size_t total_bytes = host_tensor->TotalBytes();
  if (total_bytes > 0) {
    const void* src_ptr = literal.untyped_data();
    void* dst_ptr = DMAHelper::base(host_tensor);
    memcpy(dst_ptr, src_ptr, total_bytes);
  }
  return Status::OK();
}

Status LiteralToHostTensor(const xla::Literal& literal, DataType target_type,
                           Tensor* host_tensor) {
  TensorShape shape;
  TF_RETURN_IF_ERROR(XLAShapeToTensorShape(literal.shape(), &shape));
  *host_tensor = Tensor(target_type, shape);
  return CopyLiteralToHostTensor(literal, host_tensor);
}

}  // namespace tensorflow
