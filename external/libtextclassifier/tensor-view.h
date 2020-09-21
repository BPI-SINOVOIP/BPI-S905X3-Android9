/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef LIBTEXTCLASSIFIER_TENSOR_VIEW_H_
#define LIBTEXTCLASSIFIER_TENSOR_VIEW_H_

#include <algorithm>
#include <vector>

namespace libtextclassifier2 {
namespace internal {
// Computes the number of elements in a tensor of given shape.
int NumberOfElements(const std::vector<int>& shape);
}  // namespace internal

// View of a tensor of given type.
// NOTE: Does not own the underlying memory, so the contract about its validity
// needs to be specified on the interface that returns it.
template <typename T>
class TensorView {
 public:
  TensorView(const T* data, const std::vector<int>& shape)
      : data_(data), shape_(shape), size_(internal::NumberOfElements(shape)) {}

  static TensorView Invalid() {
    static std::vector<int>& invalid_shape =
        *[]() { return new std::vector<int>(0); }();
    return TensorView(nullptr, invalid_shape);
  }

  bool is_valid() const { return data_ != nullptr; }

  const std::vector<int>& shape() const { return shape_; }

  int dim(int i) const { return shape_[i]; }

  int dims() const { return shape_.size(); }

  const T* data() const { return data_; }

  int size() const { return size_; }

  bool copy_to(T* dest, int dest_size) const {
    if (dest_size < size_) {
      return false;
    }
    std::copy(data_, data_ + size_, dest);
    return true;
  }

 private:
  const T* data_ = nullptr;
  const std::vector<int> shape_;
  const int size_;
};

}  // namespace libtextclassifier2

#endif  // LIBTEXTCLASSIFIER_TENSOR_VIEW_H_
