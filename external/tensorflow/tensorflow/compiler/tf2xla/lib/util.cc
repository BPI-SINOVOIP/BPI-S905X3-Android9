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

#include <memory>
#include <vector>

#include "tensorflow/compiler/xla/literal_util.h"
#include "tensorflow/compiler/xla/shape_util.h"
#include "tensorflow/compiler/xla/status_macros.h"
#include "tensorflow/compiler/xla/statusor.h"
#include "tensorflow/compiler/xla/util.h"
#include "tensorflow/core/lib/core/errors.h"

namespace tensorflow {

xla::ComputationDataHandle Zeros(xla::ComputationBuilder* builder,
                                 const xla::Shape& shape) {
  return builder->Broadcast(
      builder->ConstantLiteral(xla::Literal::Zero(shape.element_type())),
      xla::AsInt64Slice(shape.dimensions()));
}

xla::ComputationDataHandle FloatLiteral(xla::ComputationBuilder* builder,
                                        xla::PrimitiveType type, double value) {
  switch (type) {
    case xla::F16:
      return builder->ConstantR0<xla::half>(static_cast<xla::half>(value));
      break;
    case xla::BF16:
      return builder->ConstantR0<bfloat16>(static_cast<bfloat16>(value));
      break;
    case xla::F32:
      return builder->ConstantR0<float>(static_cast<float>(value));
      break;
    case xla::F64:
      return builder->ConstantR0<double>(value);
      break;
    case xla::C64:
      return builder->ConstantR0<xla::complex64>(value);
      break;
    default:
      LOG(FATAL) << "unhandled element type " << type;
  }
}

xla::ComputationDataHandle IntegerLiteral(xla::ComputationBuilder* builder,
                                          xla::PrimitiveType type,
                                          int64 value) {
  xla::Literal literal;
  switch (type) {
    case xla::U8:
      literal = std::move(*xla::Literal::CreateR0<uint8>(value));
      break;
    case xla::U32:
      literal = std::move(*xla::Literal::CreateR0<uint32>(value));
      break;
    case xla::U64:
      literal = std::move(*xla::Literal::CreateR0<uint64>(value));
      break;
    case xla::S8:
      literal = std::move(*xla::Literal::CreateR0<int8>(value));
      break;
    case xla::S32:
      literal = std::move(*xla::Literal::CreateR0<int32>(value));
      break;
    case xla::S64:
      literal = std::move(*xla::Literal::CreateR0<int64>(value));
      break;
    case xla::F32:
      literal = std::move(*xla::Literal::CreateR0<float>(value));
      break;
    case xla::F64:
      literal = std::move(*xla::Literal::CreateR0<double>(value));
      break;
    case xla::C64:
      literal = std::move(*xla::Literal::CreateR0<complex64>(value));
      break;
    case xla::PRED:
      LOG(FATAL) << "pred element type is not integral";
    case xla::S16:
    case xla::U16:
      LOG(FATAL) << "u16/s16 literals not yet implemented";
    case xla::BF16:
      literal = std::move(
          *xla::Literal::CreateR0<bfloat16>(static_cast<bfloat16>(value)));
      break;
    case xla::F16:
      literal = std::move(
          *xla::Literal::CreateR0<xla::half>(static_cast<xla::half>(value)));
      break;
    case xla::TUPLE:
      LOG(FATAL) << "tuple element type is not integral";
    case xla::OPAQUE:
      LOG(FATAL) << "opaque element type is not integral";
    default:
      LOG(FATAL) << "unhandled element type " << type;
  }
  return builder->ConstantLiteral(literal);
}

xla::StatusOr<xla::ComputationDataHandle> SliceInMinorDims(
    xla::ComputationBuilder* builder, const xla::ComputationDataHandle& x,
    gtl::ArraySlice<int64> start, gtl::ArraySlice<int64> end) {
  TF_RET_CHECK(start.size() == end.size());
  int64 n_minor_dims = start.size();

  TF_ASSIGN_OR_RETURN(std::unique_ptr<xla::Shape> shape, builder->GetShape(x));

  const int64 n_dims = xla::ShapeUtil::Rank(*shape);
  TF_RET_CHECK(n_minor_dims <= n_dims);
  gtl::ArraySlice<int64> major_dims(xla::AsInt64Slice(shape->dimensions()),
                                    /*pos=*/0,
                                    /*len=*/n_dims - n_minor_dims);

  // Prepends 0s in the major dim
  std::vector<int64> padded_start(n_dims, 0);
  std::copy(start.begin(), start.end(),
            padded_start.begin() + major_dims.size());

  // Prepends the shape of the major dims.
  std::vector<int64> padded_end(n_dims);
  std::copy(major_dims.begin(), major_dims.end(), padded_end.begin());
  std::copy(end.begin(), end.end(), padded_end.begin() + major_dims.size());

  std::vector<int64> strides(n_dims, 1);
  return builder->Slice(x, padded_start, padded_end, strides);
}

xla::StatusOr<xla::ComputationDataHandle> UpdateSlice(
    xla::ComputationBuilder* builder, const xla::ComputationDataHandle& x,
    const xla::ComputationDataHandle& update, gtl::ArraySlice<int64> start) {
  // TODO(phawkins): make int64 work on all backends, remove the int32 cast.
  std::vector<int32> start_as_int32(start.begin(), start.end());
  return builder->DynamicUpdateSlice(
      x, update, builder->ConstantR1<int32>(start_as_int32));
}

xla::StatusOr<xla::ComputationDataHandle> UpdateSliceInMinorDims(
    xla::ComputationBuilder* builder, const xla::ComputationDataHandle& x,
    const xla::ComputationDataHandle& update, gtl::ArraySlice<int64> start) {
  TF_ASSIGN_OR_RETURN(std::unique_ptr<xla::Shape> shape, builder->GetShape(x));
  const int64 n_dims = xla::ShapeUtil::Rank(*shape);
  const int64 n_minor_dims = start.size();
  TF_RET_CHECK(n_minor_dims <= n_dims);
  std::vector<int64> padded_start(n_dims, 0);
  std::copy(start.begin(), start.end(),
            padded_start.begin() + (n_dims - n_minor_dims));
  return UpdateSlice(builder, x, update, padded_start);
}

xla::StatusOr<xla::ComputationDataHandle> TransposeInMinorDims(
    xla::ComputationBuilder* builder, const xla::ComputationDataHandle& x) {
  TF_ASSIGN_OR_RETURN(std::unique_ptr<xla::Shape> shape, builder->GetShape(x));
  const int64 n_dims = xla::ShapeUtil::Rank(*shape);
  TF_RET_CHECK(n_dims >= 2);
  std::vector<int64> permutation(n_dims);
  std::iota(permutation.begin(), permutation.end(), 0);
  std::swap(permutation[n_dims - 1], permutation[n_dims - 2]);
  return builder->Transpose(x, permutation);
}

}  // namespace tensorflow
