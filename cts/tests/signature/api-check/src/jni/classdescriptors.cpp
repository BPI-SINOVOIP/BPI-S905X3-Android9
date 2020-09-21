/*
 * Copyright (C) 2018 The Android Open Source Project
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

#include <jni.h>
#include <jvmti.h>

#include <stdlib.h>
#include <string.h>
#include <type_traits>

#include <iostream>
#include <sstream>

namespace android {
namespace signature {
namespace cts {
namespace api {

static jvmtiEnv* jvmti_env;
static jvmtiError (*get_descriptor_list)(jvmtiEnv* env, jobject loader, jint* cnt, char*** descs);

template <typename T>
static void Dealloc(T* t) {
  jvmti_env->Deallocate(reinterpret_cast<unsigned char*>(t));
}

template <typename T, typename ...Rest>
static void Dealloc(T* t, Rest... rs) {
  Dealloc(t);
  Dealloc(rs...);
}

static void DeallocParams(jvmtiParamInfo* params, jint n_params) {
  for (jint i = 0; i < n_params; i++) {
    Dealloc(params[i].name);
  }
}

static void Cleanup(char** data, jint cnt) {
  for (jint i = 0; i < cnt; i++) {
    Dealloc(data[i]);
  }
  Dealloc(data);
}

template<typename T>
class ScopedJvmtiReference {
 static_assert(std::is_pointer<T>::value, "T must be a pointer type");

 public:
  ScopedJvmtiReference() : ref_(nullptr) {}

  ~ScopedJvmtiReference() {
    if (ref_ != nullptr) {
      Dealloc(ref_);
    }
  }

  // Return the pointer value.
  T Get() { return ref_; };

  // Return a pointer to the pointer value.
  T* GetPtr() { return &ref_; };

 private:
  T ref_;
};

static void abortIfExceptionPending(JNIEnv* env) {
  if (env->ExceptionCheck()) {
    abort();
  }
}

extern "C" JNIEXPORT jint JNICALL Agent_OnAttach(JavaVM* vm,
                                                 __attribute__((unused)) char* options,
                                                 __attribute__((unused)) void* reserved) {
  jint jvmError = vm->GetEnv(reinterpret_cast<void**>(&jvmti_env), JVMTI_VERSION_1_2);
  if (jvmError != JNI_OK) {
    return jvmError;
  }
  return JVMTI_ERROR_NONE;
}

extern "C" JNIEXPORT jobjectArray JNICALL Java_android_signature_cts_api_BootClassPathClassesProvider_getClassloaderDescriptors(
    JNIEnv* env, jclass, jobject loader) {
  if (get_descriptor_list == nullptr) {
    jclass rt_exception = env->FindClass("java/lang/RuntimeException");
    env->ThrowNew(rt_exception, "get_class_loader_class_descriptor extension is not ready.");
    return nullptr;
  }
  char** classes = nullptr;
  jint cnt = -1;
  jvmtiError error = get_descriptor_list(jvmti_env, loader, &cnt, &classes);
  if (error != JVMTI_ERROR_NONE) {
    jclass rt_exception = env->FindClass("java/lang/RuntimeException");
    env->ThrowNew(rt_exception, "Error while executing get_class_loader_class_descriptor.");
    return nullptr;
  }

  jobjectArray arr = env->NewObjectArray(cnt, env->FindClass("java/lang/String"), nullptr);
  if (env->ExceptionCheck()) {
    Cleanup(classes, cnt);
    return nullptr;
  }

  for (jint i = 0; i < cnt; i++) {
    env->SetObjectArrayElement(arr, i, env->NewStringUTF(classes[i]));
    if (env->ExceptionCheck()) {
      Cleanup(classes, cnt);
      return nullptr;
    }
  }
  Cleanup(classes, cnt);
  return arr;
}

extern "C" JNIEXPORT void JNICALL Java_android_signature_cts_api_BootClassPathClassesProvider_initialize(JNIEnv* env, jclass) {
  jint functionInfosCount = 0;
  jvmtiExtensionFunctionInfo* functionInfos = nullptr;

  jvmtiError err = jvmti_env->GetExtensionFunctions(&functionInfosCount, &functionInfos);
  if (err != JVMTI_ERROR_NONE) {
    jclass rt_exception = env->FindClass("java/lang/RuntimeException");
    env->ThrowNew(rt_exception, "Failed to get JVMTI extension APIs");
    return;
  }

  for (jint i = 0; i < functionInfosCount; i++) {
    jvmtiExtensionFunctionInfo* curInfo = &functionInfos[i];
    if (strcmp("com.android.art.class.get_class_loader_class_descriptors", curInfo->id) == 0) {
      get_descriptor_list = reinterpret_cast<jvmtiError (*)(jvmtiEnv*, jobject, jint*, char***)>(curInfo->func);
    }
    DeallocParams(curInfo->params, curInfo->param_count);
    Dealloc(curInfo->id, curInfo->short_description, curInfo->params, curInfo->errors);
  }
  Dealloc(functionInfos);

  if (get_descriptor_list == nullptr) {
    jclass rt_exception = env->FindClass("java/lang/RuntimeException");
    env->ThrowNew(rt_exception, "Failed to find get_class_loader_class_descriptors extension");
    return;
  }
}

extern "C" JNIEXPORT jobjectArray JNICALL
Java_android_signature_cts_api_BootClassPathClassesProvider_getClassMemberNamesAndTypes(
    JNIEnv* env, jclass, jclass klass, jboolean getFields) {
  jvmtiError error;

  jint count;
  ScopedJvmtiReference<jfieldID*> fids;
  ScopedJvmtiReference<jmethodID*> mids;

  // Request a list of field/method IDs using JVMTI.
  error = (getFields != JNI_FALSE) ? jvmti_env->GetClassFields(klass, &count, fids.GetPtr())
                                   : jvmti_env->GetClassMethods(klass, &count, mids.GetPtr());
  if (error != JVMTI_ERROR_NONE) {
    std::stringstream ss;
    ss << "Error while executing "
       << ((getFields != JNI_FALSE) ? "GetClassFields" : "GetClassMethods")
       << ", error code: " << static_cast<unsigned>(error);
    std::string error = ss.str();
    jclass rt_exception = env->FindClass("java/lang/RuntimeException");
    env->ThrowNew(rt_exception, error.c_str());
    return nullptr;
  }

  jobjectArray names = env->NewObjectArray(count, env->FindClass("java/lang/String"), nullptr);
  abortIfExceptionPending(env);
  jobjectArray types = env->NewObjectArray(count, env->FindClass("java/lang/String"), nullptr);
  abortIfExceptionPending(env);

  // Convert IDs to names and types using JVMTI.
  for (jint i = 0; i < count; ++i) {
    ScopedJvmtiReference<char*> name;
    ScopedJvmtiReference<char*> type;

    error = (getFields != JNI_FALSE)
        ? jvmti_env->GetFieldName(klass, fids.Get()[i], name.GetPtr(), type.GetPtr(), nullptr)
        : jvmti_env->GetMethodName(mids.Get()[i], name.GetPtr(), type.GetPtr(), nullptr);
    if (error != JVMTI_ERROR_NONE) {
      std::stringstream ss;
      ss << "Error while executing "
         << ((getFields != JNI_FALSE) ? "GetFieldName" : "GetMethodName")
         << ", error code: " << static_cast<unsigned>(error);
      std::string error = ss.str();
      jclass rt_exception = env->FindClass("java/lang/RuntimeException");
      env->ThrowNew(rt_exception, error.c_str());
      return nullptr;
    }

    env->SetObjectArrayElement(names, i, env->NewStringUTF(name.Get()));
    abortIfExceptionPending(env);
    env->SetObjectArrayElement(types, i, env->NewStringUTF(type.Get()));
    abortIfExceptionPending(env);
  }

  // Return as a array size 2 x count, where result[0] is an array of names and
  // result[1] is an array of types.
  jobjectArray result = env->NewObjectArray(
      /* count */ 2, env->FindClass("[Ljava/lang/String;"), nullptr);
  abortIfExceptionPending(env);
  env->SetObjectArrayElement(result, 0, names);
  abortIfExceptionPending(env);
  env->SetObjectArrayElement(result, 1, types);
  abortIfExceptionPending(env);

  return result;
}

}  // namespace api
}  // namespace cts
}  // namespace signature
}  // namespace android
