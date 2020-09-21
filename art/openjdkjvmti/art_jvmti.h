/* Copyright (C) 2016 The Android Open Source Project
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This file implements interfaces from the file jvmti.h. This implementation
 * is licensed under the same terms as the file jvmti.h.  The
 * copyright and license information for the file jvmti.h follows.
 *
 * Copyright (c) 2003, 2011, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.  Oracle designates this
 * particular file as subject to the "Classpath" exception as provided
 * by Oracle in the LICENSE file that accompanied this code.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 */

#ifndef ART_OPENJDKJVMTI_ART_JVMTI_H_
#define ART_OPENJDKJVMTI_ART_JVMTI_H_

#include <memory>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>

#include <jni.h>

#include <android-base/logging.h>

#include "deopt_manager.h"
#include "base/casts.h"
#include "base/macros.h"
#include "base/strlcpy.h"
#include "base/mutex.h"
#include "events.h"
#include "java_vm_ext.h"
#include "jni_env_ext.h"
#include "jvmti.h"
#include "ti_breakpoint.h"

namespace art {
class ArtField;
class ArtMethod;
class ShadowFrame;
}  // namespace art

namespace openjdkjvmti {

class ObjectTagTable;

// A special version that we use to identify special tooling interface versions which mostly matches
// the jvmti spec but everything is best effort. This is used to implement the userdebug
// 'debug-anything' behavior.
//
// This is the value 0x70010200.
static constexpr jint kArtTiVersion = JVMTI_VERSION_1_2 | 0x40000000;

// A structure that is a jvmtiEnv with additional information for the runtime.
struct ArtJvmTiEnv : public jvmtiEnv {
  art::JavaVMExt* art_vm;
  void* local_data;

  // The ti_version we are compatible with. This is only for giving the correct value for GetVersion
  // when running on a userdebug/eng device.
  jint ti_version;

  jvmtiCapabilities capabilities;

  EventMasks event_masks;
  std::unique_ptr<ArtJvmtiEventCallbacks> event_callbacks;

  // Tagging is specific to the jvmtiEnv.
  std::unique_ptr<ObjectTagTable> object_tag_table;

  // Set of watched fields is unique to each jvmtiEnv.
  // TODO It might be good to follow the RI and only let one jvmtiEnv ever have the watch caps so
  // we can record this on the field directly. We could do this either using free access-flag bits
  // or by putting a list in the ClassExt of a field's DeclaringClass.
  // TODO Maybe just have an extension to let one put a watch on every field, that would probably be
  // good enough maybe since you probably want either a few or all/almost all of them.
  std::unordered_set<art::ArtField*> access_watched_fields GUARDED_BY(event_info_mutex_);
  std::unordered_set<art::ArtField*> modify_watched_fields GUARDED_BY(event_info_mutex_);

  // Set of breakpoints is unique to each jvmtiEnv.
  std::unordered_set<Breakpoint> breakpoints GUARDED_BY(event_info_mutex_);
  std::unordered_set<const art::ShadowFrame*> notify_frames GUARDED_BY(event_info_mutex_);

  // RW lock to protect access to all of the event data.
  art::ReaderWriterMutex event_info_mutex_ DEFAULT_MUTEX_ACQUIRED_AFTER;

  ArtJvmTiEnv(art::JavaVMExt* runtime, EventHandler* event_handler, jint ti_version);

  static ArtJvmTiEnv* AsArtJvmTiEnv(jvmtiEnv* env) {
    return art::down_cast<ArtJvmTiEnv*>(env);
  }

  // Top level lock. Nothing can be held when we get this except for mutator lock for full
  // thread-suspension.
  static art::Mutex *gEnvMutex ACQUIRED_AFTER(art::Locks::mutator_lock_);
};

// Macro and constexpr to make error values less annoying to write.
#define ERR(e) JVMTI_ERROR_ ## e
static constexpr jvmtiError OK = JVMTI_ERROR_NONE;

// Special error code for unimplemented functions in JVMTI
static constexpr jvmtiError ERR(NOT_IMPLEMENTED) = JVMTI_ERROR_NOT_AVAILABLE;

static inline JNIEnv* GetJniEnv(jvmtiEnv* env) {
  JNIEnv* ret_value = nullptr;
  jint res = reinterpret_cast<ArtJvmTiEnv*>(env)->art_vm->GetEnv(
      reinterpret_cast<void**>(&ret_value), JNI_VERSION_1_1);
  if (res != JNI_OK) {
    return nullptr;
  }
  return ret_value;
}

template <typename T>
class JvmtiDeleter {
 public:
  JvmtiDeleter() : env_(nullptr) {}
  explicit JvmtiDeleter(jvmtiEnv* env) : env_(env) {}

  JvmtiDeleter(JvmtiDeleter&) = default;
  JvmtiDeleter(JvmtiDeleter&&) = default;
  JvmtiDeleter& operator=(const JvmtiDeleter&) = default;

  void operator()(T* ptr) const {
    CHECK(env_ != nullptr);
    jvmtiError ret = env_->Deallocate(reinterpret_cast<unsigned char*>(ptr));
    CHECK(ret == ERR(NONE));
  }

 private:
  mutable jvmtiEnv* env_;
};

template <typename T>
class JvmtiDeleter<T[]> {
 public:
  JvmtiDeleter() : env_(nullptr) {}
  explicit JvmtiDeleter(jvmtiEnv* env) : env_(env) {}

  JvmtiDeleter(JvmtiDeleter&) = default;
  JvmtiDeleter(JvmtiDeleter&&) = default;
  JvmtiDeleter& operator=(const JvmtiDeleter&) = default;

  template <typename U>
  void operator()(U* ptr) const {
    CHECK(env_ != nullptr);
    jvmtiError ret = env_->Deallocate(reinterpret_cast<unsigned char*>(ptr));
    CHECK(ret == ERR(NONE));
  }

 private:
  mutable jvmtiEnv* env_;
};

template <typename T>
using JvmtiUniquePtr = std::unique_ptr<T, JvmtiDeleter<T>>;

template <typename T>
ALWAYS_INLINE
static inline JvmtiUniquePtr<T> MakeJvmtiUniquePtr(jvmtiEnv* env, T* mem) {
  return JvmtiUniquePtr<T>(mem, JvmtiDeleter<T>(env));
}

template <typename T>
ALWAYS_INLINE
static inline JvmtiUniquePtr<T> MakeJvmtiUniquePtr(jvmtiEnv* env, unsigned char* mem) {
  return JvmtiUniquePtr<T>(reinterpret_cast<T*>(mem), JvmtiDeleter<T>(env));
}

template <typename T>
ALWAYS_INLINE
static inline JvmtiUniquePtr<T> AllocJvmtiUniquePtr(jvmtiEnv* env, jvmtiError* error) {
  unsigned char* tmp;
  *error = env->Allocate(sizeof(T), &tmp);
  if (*error != ERR(NONE)) {
    return JvmtiUniquePtr<T>();
  }
  return JvmtiUniquePtr<T>(tmp, JvmtiDeleter<T>(env));
}

template <typename T>
ALWAYS_INLINE
static inline JvmtiUniquePtr<T> AllocJvmtiUniquePtr(jvmtiEnv* env,
                                                    size_t count,
                                                    jvmtiError* error) {
  unsigned char* tmp;
  *error = env->Allocate(sizeof(typename std::remove_extent<T>::type) * count, &tmp);
  if (*error != ERR(NONE)) {
    return JvmtiUniquePtr<T>();
  }
  return JvmtiUniquePtr<T>(reinterpret_cast<typename std::remove_extent<T>::type*>(tmp),
                           JvmtiDeleter<T>(env));
}

ALWAYS_INLINE
static inline jvmtiError CopyDataIntoJvmtiBuffer(ArtJvmTiEnv* env,
                                                 const unsigned char* source,
                                                 jint len,
                                                 /*out*/unsigned char** dest) {
  jvmtiError res = env->Allocate(len, dest);
  if (res != OK) {
    return res;
  }
  memcpy(reinterpret_cast<void*>(*dest),
         reinterpret_cast<const void*>(source),
         len);
  return OK;
}

ALWAYS_INLINE
static inline JvmtiUniquePtr<char[]> CopyString(jvmtiEnv* env, const char* src, jvmtiError* error) {
  if (src == nullptr) {
    JvmtiUniquePtr<char[]> ret = AllocJvmtiUniquePtr<char[]>(env, 0, error);
    return ret;
  }
  size_t len = strlen(src) + 1;
  JvmtiUniquePtr<char[]> ret = AllocJvmtiUniquePtr<char[]>(env, len, error);
  if (ret != nullptr) {
    strlcpy(ret.get(), src, len);
  }
  return ret;
}

const jvmtiCapabilities kPotentialCapabilities = {
    .can_tag_objects                                 = 1,
    .can_generate_field_modification_events          = 1,
    .can_generate_field_access_events                = 1,
    .can_get_bytecodes                               = 1,
    .can_get_synthetic_attribute                     = 1,
    .can_get_owned_monitor_info                      = 1,
    .can_get_current_contended_monitor               = 1,
    .can_get_monitor_info                            = 1,
    .can_pop_frame                                   = 0,
    .can_redefine_classes                            = 1,
    .can_signal_thread                               = 1,
    .can_get_source_file_name                        = 1,
    .can_get_line_numbers                            = 1,
    .can_get_source_debug_extension                  = 1,
    .can_access_local_variables                      = 1,
    .can_maintain_original_method_order              = 1,
    .can_generate_single_step_events                 = 1,
    .can_generate_exception_events                   = 1,
    .can_generate_frame_pop_events                   = 1,
    .can_generate_breakpoint_events                  = 1,
    .can_suspend                                     = 1,
    .can_redefine_any_class                          = 0,
    .can_get_current_thread_cpu_time                 = 0,
    .can_get_thread_cpu_time                         = 0,
    .can_generate_method_entry_events                = 1,
    .can_generate_method_exit_events                 = 1,
    .can_generate_all_class_hook_events              = 0,
    .can_generate_compiled_method_load_events        = 0,
    .can_generate_monitor_events                     = 1,
    .can_generate_vm_object_alloc_events             = 1,
    .can_generate_native_method_bind_events          = 1,
    .can_generate_garbage_collection_events          = 1,
    .can_generate_object_free_events                 = 1,
    .can_force_early_return                          = 0,
    .can_get_owned_monitor_stack_depth_info          = 1,
    .can_get_constant_pool                           = 0,
    .can_set_native_method_prefix                    = 0,
    .can_retransform_classes                         = 1,
    .can_retransform_any_class                       = 0,
    .can_generate_resource_exhaustion_heap_events    = 0,
    .can_generate_resource_exhaustion_threads_events = 0,
};

// These are capabilities that are disabled if we were loaded without being debuggable.
//
// This includes the following capabilities:
//   can_retransform_any_class:
//   can_retransform_classes:
//   can_redefine_any_class:
//   can_redefine_classes:
//     We need to ensure that inlined code is either not present or can always be deoptimized. This
//     is not guaranteed for non-debuggable processes since we might have inlined bootclasspath code
//     on a threads stack.
const jvmtiCapabilities kNonDebuggableUnsupportedCapabilities = {
    .can_tag_objects                                 = 0,
    .can_generate_field_modification_events          = 0,
    .can_generate_field_access_events                = 0,
    .can_get_bytecodes                               = 0,
    .can_get_synthetic_attribute                     = 0,
    .can_get_owned_monitor_info                      = 0,
    .can_get_current_contended_monitor               = 0,
    .can_get_monitor_info                            = 0,
    .can_pop_frame                                   = 0,
    .can_redefine_classes                            = 1,
    .can_signal_thread                               = 0,
    .can_get_source_file_name                        = 0,
    .can_get_line_numbers                            = 0,
    .can_get_source_debug_extension                  = 0,
    .can_access_local_variables                      = 0,
    .can_maintain_original_method_order              = 0,
    .can_generate_single_step_events                 = 0,
    .can_generate_exception_events                   = 0,
    .can_generate_frame_pop_events                   = 0,
    .can_generate_breakpoint_events                  = 0,
    .can_suspend                                     = 0,
    .can_redefine_any_class                          = 1,
    .can_get_current_thread_cpu_time                 = 0,
    .can_get_thread_cpu_time                         = 0,
    .can_generate_method_entry_events                = 0,
    .can_generate_method_exit_events                 = 0,
    .can_generate_all_class_hook_events              = 0,
    .can_generate_compiled_method_load_events        = 0,
    .can_generate_monitor_events                     = 0,
    .can_generate_vm_object_alloc_events             = 0,
    .can_generate_native_method_bind_events          = 0,
    .can_generate_garbage_collection_events          = 0,
    .can_generate_object_free_events                 = 0,
    .can_force_early_return                          = 0,
    .can_get_owned_monitor_stack_depth_info          = 0,
    .can_get_constant_pool                           = 0,
    .can_set_native_method_prefix                    = 0,
    .can_retransform_classes                         = 1,
    .can_retransform_any_class                       = 1,
    .can_generate_resource_exhaustion_heap_events    = 0,
    .can_generate_resource_exhaustion_threads_events = 0,
};

}  // namespace openjdkjvmti

#endif  // ART_OPENJDKJVMTI_ART_JVMTI_H_
