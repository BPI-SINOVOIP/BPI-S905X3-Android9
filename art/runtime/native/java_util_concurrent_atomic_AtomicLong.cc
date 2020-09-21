/*
 * Copyright (C) 2008 The Android Open Source Project
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

#include "java_util_concurrent_atomic_AtomicLong.h"

#include "nativehelper/jni_macros.h"

#include "arch/instruction_set.h"
#include "base/atomic.h"
#include "base/quasi_atomic.h"
#include "jni_internal.h"
#include "native_util.h"

namespace art {

static jboolean AtomicLong_VMSupportsCS8(JNIEnv*, jclass) {
  return QuasiAtomic::LongAtomicsUseMutexes(kRuntimeISA) ? JNI_FALSE : JNI_TRUE;
}

static JNINativeMethod gMethods[] = {
  NATIVE_METHOD(AtomicLong, VMSupportsCS8, "()Z"),
};

void register_java_util_concurrent_atomic_AtomicLong(JNIEnv* env) {
  REGISTER_NATIVE_METHODS("java/util/concurrent/atomic/AtomicLong");
}

}  // namespace art
