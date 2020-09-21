/*
 * Copyright (C) 2014 The Android Open Source Project
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

#ifndef ART_RUNTIME_READ_BARRIER_H_
#define ART_RUNTIME_READ_BARRIER_H_

#include <android-base/logging.h>

#include "base/macros.h"
#include "base/mutex.h"
#include "base/runtime_debug.h"
#include "gc_root.h"
#include "jni.h"
#include "mirror/object_reference.h"
#include "offsets.h"
#include "read_barrier_config.h"

namespace art {
namespace mirror {
class Object;
template<typename MirrorType> class HeapReference;
}  // namespace mirror
class ArtMethod;

class ReadBarrier {
 public:
  // Enable the to-space invariant checks. This is slow and happens very often. Do not enable in
  // fast-debug environment.
  DECLARE_RUNTIME_DEBUG_FLAG(kEnableToSpaceInvariantChecks);

  // Enable the read barrier checks. This is slow and happens very often. Do not enable in
  // fast-debug environment.
  DECLARE_RUNTIME_DEBUG_FLAG(kEnableReadBarrierInvariantChecks);

  // Return the reference at ref_addr, invoking read barrier as appropriate.
  // Ref_addr is an address within obj.
  // It's up to the implementation whether the given field gets updated whereas the return value
  // must be an updated reference unless kAlwaysUpdateField is true.
  template <typename MirrorType,
            bool kIsVolatile,
            ReadBarrierOption kReadBarrierOption = kWithReadBarrier,
            bool kAlwaysUpdateField = false>
  ALWAYS_INLINE static MirrorType* Barrier(
      mirror::Object* obj, MemberOffset offset, mirror::HeapReference<MirrorType>* ref_addr)
      REQUIRES_SHARED(Locks::mutator_lock_);

  // It's up to the implementation whether the given root gets updated
  // whereas the return value must be an updated reference.
  template <typename MirrorType, ReadBarrierOption kReadBarrierOption = kWithReadBarrier>
  ALWAYS_INLINE static MirrorType* BarrierForRoot(MirrorType** root,
                                                  GcRootSource* gc_root_source = nullptr)
      REQUIRES_SHARED(Locks::mutator_lock_);

  // It's up to the implementation whether the given root gets updated
  // whereas the return value must be an updated reference.
  template <typename MirrorType, ReadBarrierOption kReadBarrierOption = kWithReadBarrier>
  ALWAYS_INLINE static MirrorType* BarrierForRoot(mirror::CompressedReference<MirrorType>* root,
                                                  GcRootSource* gc_root_source = nullptr)
      REQUIRES_SHARED(Locks::mutator_lock_);

  // Return the mirror Object if it is marked, or null if not.
  template <typename MirrorType>
  ALWAYS_INLINE static MirrorType* IsMarked(MirrorType* ref)
      REQUIRES_SHARED(Locks::mutator_lock_);

  static bool IsDuringStartup();

  // Without the holder object.
  static void AssertToSpaceInvariant(mirror::Object* ref)
      REQUIRES_SHARED(Locks::mutator_lock_) {
    AssertToSpaceInvariant(nullptr, MemberOffset(0), ref);
  }
  // With the holder object.
  static void AssertToSpaceInvariant(mirror::Object* obj, MemberOffset offset,
                                     mirror::Object* ref)
      REQUIRES_SHARED(Locks::mutator_lock_);
  // With GcRootSource.
  static void AssertToSpaceInvariant(GcRootSource* gc_root_source, mirror::Object* ref)
      REQUIRES_SHARED(Locks::mutator_lock_);

  // Without the holder object, and only with the read barrier configuration (no-op otherwise).
  static void MaybeAssertToSpaceInvariant(mirror::Object* ref)
      REQUIRES_SHARED(Locks::mutator_lock_) {
    if (kUseReadBarrier) {
      AssertToSpaceInvariant(ref);
    }
  }

  // ALWAYS_INLINE on this caused a performance regression b/26744236.
  static mirror::Object* Mark(mirror::Object* obj) REQUIRES_SHARED(Locks::mutator_lock_);

  static constexpr uint32_t WhiteState() {
    return white_state_;
  }
  static constexpr uint32_t GrayState() {
    return gray_state_;
  }

  // fake_address_dependency will be zero which should be bitwise-or'ed with the address of the
  // subsequent load to prevent the reordering of the read barrier bit load and the subsequent
  // object reference load (from one of `obj`'s fields).
  // *fake_address_dependency will be set to 0.
  ALWAYS_INLINE static bool IsGray(mirror::Object* obj, uintptr_t* fake_address_dependency)
      REQUIRES_SHARED(Locks::mutator_lock_);

  // This uses a load-acquire to load the read barrier bit internally to prevent the reordering of
  // the read barrier bit load and the subsequent load.
  ALWAYS_INLINE static bool IsGray(mirror::Object* obj)
      REQUIRES_SHARED(Locks::mutator_lock_);

  static bool IsValidReadBarrierState(uint32_t rb_state) {
    return rb_state == white_state_ || rb_state == gray_state_;
  }

  static constexpr uint32_t white_state_ = 0x0;    // Not marked.
  static constexpr uint32_t gray_state_ = 0x1;     // Marked, but not marked through. On mark stack.
  static constexpr uint32_t rb_state_mask_ = 0x1;  // The low bits for white|gray.
};

}  // namespace art

#endif  // ART_RUNTIME_READ_BARRIER_H_
