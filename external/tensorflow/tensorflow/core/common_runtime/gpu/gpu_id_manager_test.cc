/* Copyright 2015 The TensorFlow Authors. All Rights Reserved.

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

#include "tensorflow/core/common_runtime/gpu/gpu_id_manager.h"

#include "tensorflow/core/common_runtime/gpu/gpu_id.h"
#include "tensorflow/core/platform/test.h"

namespace tensorflow {
namespace test {

TEST(GpuIdManagerTest, Basics) {
  TfGpuId key_0(0);
  CudaGpuId value_0(0);
  GpuIdManager::InsertTfCudaGpuIdPair(key_0, value_0);
  EXPECT_EQ(value_0, GpuIdManager::TfToCudaGpuId(key_0));

  // Multiple calls to map the same value is ok.
  GpuIdManager::InsertTfCudaGpuIdPair(key_0, value_0);
  EXPECT_EQ(value_0, GpuIdManager::TfToCudaGpuId(key_0));

  // Map a different TfGpuId to a different value.
  TfGpuId key_1(3);
  CudaGpuId value_1(2);
  GpuIdManager::InsertTfCudaGpuIdPair(key_1, value_1);
  EXPECT_EQ(value_1, GpuIdManager::TfToCudaGpuId(key_1));

  // Mapping a different TfGpuId to the same value is ok.
  TfGpuId key_2(10);
  GpuIdManager::InsertTfCudaGpuIdPair(key_2, value_1);
  EXPECT_EQ(value_1, GpuIdManager::TfToCudaGpuId(key_2));

  // Mapping the same TfGpuId to a different value will crash the program.
  ASSERT_DEATH(GpuIdManager::InsertTfCudaGpuIdPair(key_2, value_0),
               "Mapping the same TfGpuId to a different CUDA GPU id");

  // Getting an nonexistent mapping will crash the program.
  ASSERT_DEATH(GpuIdManager::TfToCudaGpuId(TfGpuId(100)),
               "Could not find the mapping for TfGpuId");
}

}  // namespace test
}  // namespace tensorflow
