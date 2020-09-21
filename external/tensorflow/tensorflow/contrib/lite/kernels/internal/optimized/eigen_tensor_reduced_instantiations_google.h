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
#ifndef TENSORFLOW_CONTRIB_LITE_KERNELS_INTERNAL_OPTIMIZED_EIGEN_TENSOR_REDUCED_INSTANTIATIONS_GOOGLE_H_
#define TENSORFLOW_CONTRIB_LITE_KERNELS_INTERNAL_OPTIMIZED_EIGEN_TENSOR_REDUCED_INSTANTIATIONS_GOOGLE_H_

#define EIGEN_USE_CUSTOM_THREAD_POOL
#define EIGEN_USE_THREADS

// clang-format off

#include <stdint.h>

#include <cstddef>
#include <cstring>
#include <cmath>
#include <random>
#include <atomic>
#include <condition_variable>  // NOLINT(build/c++11)
#include <mutex>  // NOLINT(build/c++11)
#include <thread>  // NOLINT(build/c++11)
#include <functional>

#ifdef _WIN32
#include <winbase.h>
#elif defined(__APPLE__)
#include <mach/mach_time.h>
#else
#include <time.h>
#endif


// Because some programs may link Eigen in through other frameworks with
// different flags, we can run into multiple definition issues if we don't have
// a private namespace for our versions. This is a nasty hack, but a similar
// approach is used elsewhere to handle the problem, so it should be stable.
#define Eigen EigenForTFLite

#include "Eigen/src/Core/util/StaticAssert.h"
#include "unsupported/Eigen/CXX11/Core"
#include "unsupported/Eigen/SpecialFunctions"

#include "Eigen/src/Core/util/DisableStupidWarnings.h"

#include "Eigen/Core"

// Beware: the order of the include matters to some compilers. For example
// TensorIndexList.h should be included before TensorDimensions.h in order to
// use index lists to encode tensor dimensions when compiling with llvm.
// We're defining this ourselves rather than using the Eigen Tensor header file
// so that we can alter the macro definition of TENSOR_CONTRACTION_DISPATCH to
// reduce binary size.
#include "unsupported/Eigen/CXX11/src/Tensor/TensorForwardDeclarations.h"
#include "unsupported/Eigen/CXX11/src/Tensor/TensorMeta.h"
#include "unsupported/Eigen/CXX11/src/Tensor/TensorCostModel.h"
#include "unsupported/Eigen/CXX11/src/Tensor/ThreadPoolInterface.h"
#include "unsupported/Eigen/CXX11/src/Tensor/TensorDeviceType.h"
#include "unsupported/Eigen/CXX11/src/Tensor/TensorNonBlockingThreadPool.h"
#include "unsupported/Eigen/CXX11/src/Tensor/TensorIndexList.h"
#include "unsupported/Eigen/CXX11/src/Tensor/TensorDimensionList.h"
#include "unsupported/Eigen/CXX11/src/Tensor/TensorDimensions.h"
#include "unsupported/Eigen/CXX11/src/Tensor/TensorInitializer.h"
#include "unsupported/Eigen/CXX11/src/Tensor/TensorTraits.h"
#include "unsupported/Eigen/CXX11/src/Tensor/TensorRandom.h"
#include "unsupported/Eigen/CXX11/src/Tensor/TensorFunctors.h"
#include "unsupported/Eigen/CXX11/src/Tensor/TensorUInt128.h"
#include "unsupported/Eigen/CXX11/src/Tensor/TensorIntDiv.h"
#include "unsupported/Eigen/CXX11/src/Tensor/TensorBlock.h"
#include "unsupported/Eigen/CXX11/src/Tensor/TensorGlobalFunctions.h"

#include "unsupported/Eigen/CXX11/src/Tensor/TensorStats.h"

#include "unsupported/Eigen/CXX11/src/Tensor/TensorBase.h"

#include "unsupported/Eigen/CXX11/src/Tensor/TensorEvaluator.h"
#include "unsupported/Eigen/CXX11/src/Tensor/TensorExpr.h"
#include "unsupported/Eigen/CXX11/src/Tensor/TensorReduction.h"
#include "unsupported/Eigen/CXX11/src/Tensor/TensorArgMax.h"

#include "unsupported/Eigen/CXX11/src/Tensor/TensorConcatenation.h"
#include "unsupported/Eigen/CXX11/src/Tensor/TensorContractionMappers.h"
#include "unsupported/Eigen/CXX11/src/Tensor/TensorContractionBlocking.h"
#include "unsupported/Eigen/CXX11/src/Tensor/TensorContraction.h"
#undef TENSOR_CONTRACTION_DISPATCH
#define TENSOR_CONTRACTION_DISPATCH(METHOD, ALIGNMENT, ARGS)    \
  if (this->m_lhs_inner_dim_contiguous &&                       \
      this->m_rhs_inner_dim_contiguous &&                       \
      !this->m_rhs_inner_dim_reordered) {                       \
    METHOD<true, true, false, ALIGNMENT> ARGS;                  \
  } else {                                                      \
    eigen_assert(false && "Unsupported contraction formats");   \
  }

#include "unsupported/Eigen/CXX11/src/Tensor/TensorContractionThreadPool.h"
#include "unsupported/Eigen/CXX11/src/Tensor/TensorContractionCuda.h"
#include "unsupported/Eigen/CXX11/src/Tensor/TensorConversion.h"
#include "unsupported/Eigen/CXX11/src/Tensor/TensorConvolution.h"
#include "unsupported/Eigen/CXX11/src/Tensor/TensorFFT.h"
#include "unsupported/Eigen/CXX11/src/Tensor/TensorPatch.h"
#include "unsupported/Eigen/CXX11/src/Tensor/TensorImagePatch.h"
#include "unsupported/Eigen/CXX11/src/Tensor/TensorVolumePatch.h"
#include "unsupported/Eigen/CXX11/src/Tensor/TensorBroadcasting.h"
#include "unsupported/Eigen/CXX11/src/Tensor/TensorChipping.h"
#include "unsupported/Eigen/CXX11/src/Tensor/TensorInflation.h"
#include "unsupported/Eigen/CXX11/src/Tensor/TensorLayoutSwap.h"
#include "unsupported/Eigen/CXX11/src/Tensor/TensorMorphing.h"
#include "unsupported/Eigen/CXX11/src/Tensor/TensorPadding.h"
#include "unsupported/Eigen/CXX11/src/Tensor/TensorReverse.h"
#include "unsupported/Eigen/CXX11/src/Tensor/TensorShuffling.h"
#include "unsupported/Eigen/CXX11/src/Tensor/TensorStriding.h"
#include "unsupported/Eigen/CXX11/src/Tensor/TensorCustomOp.h"
#include "unsupported/Eigen/CXX11/src/Tensor/TensorEvalTo.h"
#include "unsupported/Eigen/CXX11/src/Tensor/TensorForcedEval.h"
#include "unsupported/Eigen/CXX11/src/Tensor/TensorGenerator.h"
#include "unsupported/Eigen/CXX11/src/Tensor/TensorAssign.h"
#include "unsupported/Eigen/CXX11/src/Tensor/TensorScan.h"

#include "unsupported/Eigen/CXX11/src/Tensor/TensorExecutor.h"
#include "unsupported/Eigen/CXX11/src/Tensor/TensorDevice.h"

#include "unsupported/Eigen/CXX11/src/Tensor/TensorStorage.h"
#include "unsupported/Eigen/CXX11/src/Tensor/Tensor.h"
#include "unsupported/Eigen/CXX11/src/Tensor/TensorFixedSize.h"
#include "unsupported/Eigen/CXX11/src/Tensor/TensorMap.h"
#include "unsupported/Eigen/CXX11/src/Tensor/TensorRef.h"

#include "unsupported/Eigen/CXX11/src/Tensor/TensorReductionCuda.h"

#include "unsupported/Eigen/CXX11/src/Tensor/TensorIO.h"

#include "Eigen/src/Core/util/ReenableStupidWarnings.h"
#endif  // TENSORFLOW_CONTRIB_LITE_KERNELS_INTERNAL_OPTIMIZED_EIGEN_TENSOR_REDUCED_INSTANTIATIONS_H
