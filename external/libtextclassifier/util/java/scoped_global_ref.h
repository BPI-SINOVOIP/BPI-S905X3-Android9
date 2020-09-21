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

#ifndef LIBTEXTCLASSIFIER_UTIL_JAVA_SCOPED_GLOBAL_REF_H_
#define LIBTEXTCLASSIFIER_UTIL_JAVA_SCOPED_GLOBAL_REF_H_

#include <jni.h>
#include <memory>
#include <type_traits>

#include "util/base/logging.h"

namespace libtextclassifier2 {

// A deleter to be used with std::unique_ptr to delete JNI global references.
class GlobalRefDeleter {
 public:
  GlobalRefDeleter() : jvm_(nullptr) {}

  // Style guide violating implicit constructor so that the GlobalRefDeleter
  // is implicitly constructed from the second argument to ScopedGlobalRef.
  GlobalRefDeleter(JavaVM* jvm) : jvm_(jvm) {}  // NOLINT(runtime/explicit)

  GlobalRefDeleter(const GlobalRefDeleter& orig) = default;

  // Copy assignment to allow move semantics in ScopedGlobalRef.
  GlobalRefDeleter& operator=(const GlobalRefDeleter& rhs) {
    TC_CHECK_EQ(jvm_, rhs.jvm_);
    return *this;
  }

  // The delete operator.
  void operator()(jobject object) const {
    JNIEnv* env;
    if (object != nullptr && jvm_ != nullptr &&
        JNI_OK ==
            jvm_->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_4)) {
      env->DeleteGlobalRef(object);
    }
  }

 private:
  // The jvm_ stashed to use for deletion.
  JavaVM* const jvm_;
};

// A smart pointer that deletes a JNI global reference when it goes out
// of scope. Usage is:
// ScopedGlobalRef<jobject> scoped_global(env->JniFunction(), jvm);
template <typename T>
using ScopedGlobalRef =
    std::unique_ptr<typename std::remove_pointer<T>::type, GlobalRefDeleter>;

// A helper to create global references.
template <typename T>
ScopedGlobalRef<T> MakeGlobalRef(T object, JNIEnv* env, JavaVM* jvm) {
  const jobject globalObject = env->NewGlobalRef(object);
  return ScopedGlobalRef<T>(reinterpret_cast<T>(globalObject), jvm);
}

}  // namespace libtextclassifier2

#endif  // LIBTEXTCLASSIFIER_UTIL_JAVA_SCOPED_GLOBAL_REF_H_
