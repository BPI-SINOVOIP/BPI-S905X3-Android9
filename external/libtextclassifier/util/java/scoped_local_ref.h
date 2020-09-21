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

#ifndef LIBTEXTCLASSIFIER_UTIL_JAVA_SCOPED_LOCAL_REF_H_
#define LIBTEXTCLASSIFIER_UTIL_JAVA_SCOPED_LOCAL_REF_H_

#include <jni.h>
#include <memory>
#include <type_traits>

#include "util/base/logging.h"

namespace libtextclassifier2 {

// A deleter to be used with std::unique_ptr to delete JNI local references.
class LocalRefDeleter {
 public:
  LocalRefDeleter() : env_(nullptr) {}

  // Style guide violating implicit constructor so that the LocalRefDeleter
  // is implicitly constructed from the second argument to ScopedLocalRef.
  LocalRefDeleter(JNIEnv* env) : env_(env) {}  // NOLINT(runtime/explicit)

  LocalRefDeleter(const LocalRefDeleter& orig) = default;

  // Copy assignment to allow move semantics in ScopedLocalRef.
  LocalRefDeleter& operator=(const LocalRefDeleter& rhs) {
    // As the deleter and its state are thread-local, ensure the envs
    // are consistent but do nothing.
    TC_CHECK_EQ(env_, rhs.env_);
    return *this;
  }

  // The delete operator.
  void operator()(jobject object) const {
    if (env_) {
      env_->DeleteLocalRef(object);
    }
  }

 private:
  // The env_ stashed to use for deletion. Thread-local, don't share!
  JNIEnv* const env_;
};

// A smart pointer that deletes a JNI local reference when it goes out
// of scope. Usage is:
// ScopedLocalRef<jobject> scoped_local(env->JniFunction(), env);
//
// Note that this class is not thread-safe since it caches JNIEnv in
// the deleter. Do not use the same jobject across different threads.
template <typename T>
using ScopedLocalRef =
    std::unique_ptr<typename std::remove_pointer<T>::type, LocalRefDeleter>;

}  // namespace libtextclassifier2

#endif  // LIBTEXTCLASSIFIER_UTIL_JAVA_SCOPED_LOCAL_REF_H_
