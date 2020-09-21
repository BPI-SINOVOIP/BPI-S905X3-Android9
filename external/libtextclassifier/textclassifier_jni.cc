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

// JNI wrapper for the TextClassifier.

#include "textclassifier_jni.h"

#include <jni.h>
#include <type_traits>
#include <vector>

#include "text-classifier.h"
#include "util/base/integral_types.h"
#include "util/java/scoped_local_ref.h"
#include "util/java/string_utils.h"
#include "util/memory/mmap.h"
#include "util/utf8/unilib.h"

using libtextclassifier2::AnnotatedSpan;
using libtextclassifier2::AnnotationOptions;
using libtextclassifier2::ClassificationOptions;
using libtextclassifier2::ClassificationResult;
using libtextclassifier2::CodepointSpan;
using libtextclassifier2::JStringToUtf8String;
using libtextclassifier2::Model;
using libtextclassifier2::ScopedLocalRef;
using libtextclassifier2::SelectionOptions;
using libtextclassifier2::TextClassifier;
#ifdef LIBTEXTCLASSIFIER_UNILIB_JAVAICU
using libtextclassifier2::UniLib;
#endif

namespace libtextclassifier2 {

using libtextclassifier2::CodepointSpan;

namespace {

std::string ToStlString(JNIEnv* env, const jstring& str) {
  std::string result;
  JStringToUtf8String(env, str, &result);
  return result;
}

jobjectArray ClassificationResultsToJObjectArray(
    JNIEnv* env,
    const std::vector<ClassificationResult>& classification_result) {
  const ScopedLocalRef<jclass> result_class(
      env->FindClass(TC_PACKAGE_PATH TC_CLASS_NAME_STR "$ClassificationResult"),
      env);
  if (!result_class) {
    TC_LOG(ERROR) << "Couldn't find ClassificationResult class.";
    return nullptr;
  }
  const ScopedLocalRef<jclass> datetime_parse_class(
      env->FindClass(TC_PACKAGE_PATH TC_CLASS_NAME_STR "$DatetimeResult"), env);
  if (!datetime_parse_class) {
    TC_LOG(ERROR) << "Couldn't find DatetimeResult class.";
    return nullptr;
  }

  const jmethodID result_class_constructor =
      env->GetMethodID(result_class.get(), "<init>",
                       "(Ljava/lang/String;FL" TC_PACKAGE_PATH TC_CLASS_NAME_STR
                       "$DatetimeResult;)V");
  const jmethodID datetime_parse_class_constructor =
      env->GetMethodID(datetime_parse_class.get(), "<init>", "(JI)V");

  const jobjectArray results = env->NewObjectArray(classification_result.size(),
                                                   result_class.get(), nullptr);
  for (int i = 0; i < classification_result.size(); i++) {
    jstring row_string =
        env->NewStringUTF(classification_result[i].collection.c_str());
    jobject row_datetime_parse = nullptr;
    if (classification_result[i].datetime_parse_result.IsSet()) {
      row_datetime_parse = env->NewObject(
          datetime_parse_class.get(), datetime_parse_class_constructor,
          classification_result[i].datetime_parse_result.time_ms_utc,
          classification_result[i].datetime_parse_result.granularity);
    }
    jobject result =
        env->NewObject(result_class.get(), result_class_constructor, row_string,
                       static_cast<jfloat>(classification_result[i].score),
                       row_datetime_parse);
    env->SetObjectArrayElement(results, i, result);
    env->DeleteLocalRef(result);
  }
  return results;
}

template <typename T, typename F>
std::pair<bool, T> CallJniMethod0(JNIEnv* env, jobject object,
                                  jclass class_object, F function,
                                  const std::string& method_name,
                                  const std::string& return_java_type) {
  const jmethodID method = env->GetMethodID(class_object, method_name.c_str(),
                                            ("()" + return_java_type).c_str());
  if (!method) {
    return std::make_pair(false, T());
  }
  return std::make_pair(true, (env->*function)(object, method));
}

SelectionOptions FromJavaSelectionOptions(JNIEnv* env, jobject joptions) {
  if (!joptions) {
    return {};
  }

  const ScopedLocalRef<jclass> options_class(
      env->FindClass(TC_PACKAGE_PATH TC_CLASS_NAME_STR "$SelectionOptions"),
      env);
  const std::pair<bool, jobject> status_or_locales = CallJniMethod0<jobject>(
      env, joptions, options_class.get(), &JNIEnv::CallObjectMethod,
      "getLocales", "Ljava/lang/String;");
  if (!status_or_locales.first) {
    return {};
  }

  SelectionOptions options;
  options.locales =
      ToStlString(env, reinterpret_cast<jstring>(status_or_locales.second));

  return options;
}

template <typename T>
T FromJavaOptionsInternal(JNIEnv* env, jobject joptions,
                          const std::string& class_name) {
  if (!joptions) {
    return {};
  }

  const ScopedLocalRef<jclass> options_class(env->FindClass(class_name.c_str()),
                                             env);
  if (!options_class) {
    return {};
  }

  const std::pair<bool, jobject> status_or_locales = CallJniMethod0<jobject>(
      env, joptions, options_class.get(), &JNIEnv::CallObjectMethod,
      "getLocale", "Ljava/lang/String;");
  const std::pair<bool, jobject> status_or_reference_timezone =
      CallJniMethod0<jobject>(env, joptions, options_class.get(),
                              &JNIEnv::CallObjectMethod, "getReferenceTimezone",
                              "Ljava/lang/String;");
  const std::pair<bool, int64> status_or_reference_time_ms_utc =
      CallJniMethod0<int64>(env, joptions, options_class.get(),
                            &JNIEnv::CallLongMethod, "getReferenceTimeMsUtc",
                            "J");

  if (!status_or_locales.first || !status_or_reference_timezone.first ||
      !status_or_reference_time_ms_utc.first) {
    return {};
  }

  T options;
  options.locales =
      ToStlString(env, reinterpret_cast<jstring>(status_or_locales.second));
  options.reference_timezone = ToStlString(
      env, reinterpret_cast<jstring>(status_or_reference_timezone.second));
  options.reference_time_ms_utc = status_or_reference_time_ms_utc.second;
  return options;
}

ClassificationOptions FromJavaClassificationOptions(JNIEnv* env,
                                                    jobject joptions) {
  return FromJavaOptionsInternal<ClassificationOptions>(
      env, joptions,
      TC_PACKAGE_PATH TC_CLASS_NAME_STR "$ClassificationOptions");
}

AnnotationOptions FromJavaAnnotationOptions(JNIEnv* env, jobject joptions) {
  return FromJavaOptionsInternal<AnnotationOptions>(
      env, joptions, TC_PACKAGE_PATH TC_CLASS_NAME_STR "$AnnotationOptions");
}

CodepointSpan ConvertIndicesBMPUTF8(const std::string& utf8_str,
                                    CodepointSpan orig_indices,
                                    bool from_utf8) {
  const libtextclassifier2::UnicodeText unicode_str =
      libtextclassifier2::UTF8ToUnicodeText(utf8_str, /*do_copy=*/false);

  int unicode_index = 0;
  int bmp_index = 0;

  const int* source_index;
  const int* target_index;
  if (from_utf8) {
    source_index = &unicode_index;
    target_index = &bmp_index;
  } else {
    source_index = &bmp_index;
    target_index = &unicode_index;
  }

  CodepointSpan result{-1, -1};
  std::function<void()> assign_indices_fn = [&result, &orig_indices,
                                             &source_index, &target_index]() {
    if (orig_indices.first == *source_index) {
      result.first = *target_index;
    }

    if (orig_indices.second == *source_index) {
      result.second = *target_index;
    }
  };

  for (auto it = unicode_str.begin(); it != unicode_str.end();
       ++it, ++unicode_index, ++bmp_index) {
    assign_indices_fn();

    // There is 1 extra character in the input for each UTF8 character > 0xFFFF.
    if (*it > 0xFFFF) {
      ++bmp_index;
    }
  }
  assign_indices_fn();

  return result;
}

}  // namespace

CodepointSpan ConvertIndicesBMPToUTF8(const std::string& utf8_str,
                                      CodepointSpan bmp_indices) {
  return ConvertIndicesBMPUTF8(utf8_str, bmp_indices, /*from_utf8=*/false);
}

CodepointSpan ConvertIndicesUTF8ToBMP(const std::string& utf8_str,
                                      CodepointSpan utf8_indices) {
  return ConvertIndicesBMPUTF8(utf8_str, utf8_indices, /*from_utf8=*/true);
}

jint GetFdFromAssetFileDescriptor(JNIEnv* env, jobject afd) {
  // Get system-level file descriptor from AssetFileDescriptor.
  ScopedLocalRef<jclass> afd_class(
      env->FindClass("android/content/res/AssetFileDescriptor"), env);
  if (afd_class == nullptr) {
    TC_LOG(ERROR) << "Couldn't find AssetFileDescriptor.";
    return reinterpret_cast<jlong>(nullptr);
  }
  jmethodID afd_class_getFileDescriptor = env->GetMethodID(
      afd_class.get(), "getFileDescriptor", "()Ljava/io/FileDescriptor;");
  if (afd_class_getFileDescriptor == nullptr) {
    TC_LOG(ERROR) << "Couldn't find getFileDescriptor.";
    return reinterpret_cast<jlong>(nullptr);
  }

  ScopedLocalRef<jclass> fd_class(env->FindClass("java/io/FileDescriptor"),
                                  env);
  if (fd_class == nullptr) {
    TC_LOG(ERROR) << "Couldn't find FileDescriptor.";
    return reinterpret_cast<jlong>(nullptr);
  }
  jfieldID fd_class_descriptor =
      env->GetFieldID(fd_class.get(), "descriptor", "I");
  if (fd_class_descriptor == nullptr) {
    TC_LOG(ERROR) << "Couldn't find descriptor.";
    return reinterpret_cast<jlong>(nullptr);
  }

  jobject bundle_jfd = env->CallObjectMethod(afd, afd_class_getFileDescriptor);
  return env->GetIntField(bundle_jfd, fd_class_descriptor);
}

jstring GetLocalesFromMmap(JNIEnv* env, libtextclassifier2::ScopedMmap* mmap) {
  if (!mmap->handle().ok()) {
    return env->NewStringUTF("");
  }
  const Model* model = libtextclassifier2::ViewModel(
      mmap->handle().start(), mmap->handle().num_bytes());
  if (!model || !model->locales()) {
    return env->NewStringUTF("");
  }
  return env->NewStringUTF(model->locales()->c_str());
}

jint GetVersionFromMmap(JNIEnv* env, libtextclassifier2::ScopedMmap* mmap) {
  if (!mmap->handle().ok()) {
    return 0;
  }
  const Model* model = libtextclassifier2::ViewModel(
      mmap->handle().start(), mmap->handle().num_bytes());
  if (!model) {
    return 0;
  }
  return model->version();
}

jstring GetNameFromMmap(JNIEnv* env, libtextclassifier2::ScopedMmap* mmap) {
  if (!mmap->handle().ok()) {
    return env->NewStringUTF("");
  }
  const Model* model = libtextclassifier2::ViewModel(
      mmap->handle().start(), mmap->handle().num_bytes());
  if (!model || !model->name()) {
    return env->NewStringUTF("");
  }
  return env->NewStringUTF(model->name()->c_str());
}

}  // namespace libtextclassifier2

using libtextclassifier2::ClassificationResultsToJObjectArray;
using libtextclassifier2::ConvertIndicesBMPToUTF8;
using libtextclassifier2::ConvertIndicesUTF8ToBMP;
using libtextclassifier2::FromJavaAnnotationOptions;
using libtextclassifier2::FromJavaClassificationOptions;
using libtextclassifier2::FromJavaSelectionOptions;
using libtextclassifier2::ToStlString;

JNI_METHOD(jlong, TC_CLASS_NAME, nativeNew)
(JNIEnv* env, jobject thiz, jint fd) {
#ifdef LIBTEXTCLASSIFIER_UNILIB_JAVAICU
  return reinterpret_cast<jlong>(
      TextClassifier::FromFileDescriptor(fd).release(), new UniLib(env));
#else
  return reinterpret_cast<jlong>(
      TextClassifier::FromFileDescriptor(fd).release());
#endif
}

JNI_METHOD(jlong, TC_CLASS_NAME, nativeNewFromPath)
(JNIEnv* env, jobject thiz, jstring path) {
  const std::string path_str = ToStlString(env, path);
#ifdef LIBTEXTCLASSIFIER_UNILIB_JAVAICU
  return reinterpret_cast<jlong>(
      TextClassifier::FromPath(path_str, new UniLib(env)).release());
#else
  return reinterpret_cast<jlong>(TextClassifier::FromPath(path_str).release());
#endif
}

JNI_METHOD(jlong, TC_CLASS_NAME, nativeNewFromAssetFileDescriptor)
(JNIEnv* env, jobject thiz, jobject afd, jlong offset, jlong size) {
  const jint fd = libtextclassifier2::GetFdFromAssetFileDescriptor(env, afd);
#ifdef LIBTEXTCLASSIFIER_UNILIB_JAVAICU
  return reinterpret_cast<jlong>(
      TextClassifier::FromFileDescriptor(fd, offset, size, new UniLib(env))
          .release());
#else
  return reinterpret_cast<jlong>(
      TextClassifier::FromFileDescriptor(fd, offset, size).release());
#endif
}

JNI_METHOD(jintArray, TC_CLASS_NAME, nativeSuggestSelection)
(JNIEnv* env, jobject thiz, jlong ptr, jstring context, jint selection_begin,
 jint selection_end, jobject options) {
  if (!ptr) {
    return nullptr;
  }

  TextClassifier* model = reinterpret_cast<TextClassifier*>(ptr);

  const std::string context_utf8 = ToStlString(env, context);
  CodepointSpan input_indices =
      ConvertIndicesBMPToUTF8(context_utf8, {selection_begin, selection_end});
  CodepointSpan selection = model->SuggestSelection(
      context_utf8, input_indices, FromJavaSelectionOptions(env, options));
  selection = ConvertIndicesUTF8ToBMP(context_utf8, selection);

  jintArray result = env->NewIntArray(2);
  env->SetIntArrayRegion(result, 0, 1, &(std::get<0>(selection)));
  env->SetIntArrayRegion(result, 1, 1, &(std::get<1>(selection)));
  return result;
}

JNI_METHOD(jobjectArray, TC_CLASS_NAME, nativeClassifyText)
(JNIEnv* env, jobject thiz, jlong ptr, jstring context, jint selection_begin,
 jint selection_end, jobject options) {
  if (!ptr) {
    return nullptr;
  }
  TextClassifier* ff_model = reinterpret_cast<TextClassifier*>(ptr);

  const std::string context_utf8 = ToStlString(env, context);
  const CodepointSpan input_indices =
      ConvertIndicesBMPToUTF8(context_utf8, {selection_begin, selection_end});
  const std::vector<ClassificationResult> classification_result =
      ff_model->ClassifyText(context_utf8, input_indices,
                             FromJavaClassificationOptions(env, options));

  return ClassificationResultsToJObjectArray(env, classification_result);
}

JNI_METHOD(jobjectArray, TC_CLASS_NAME, nativeAnnotate)
(JNIEnv* env, jobject thiz, jlong ptr, jstring context, jobject options) {
  if (!ptr) {
    return nullptr;
  }
  TextClassifier* model = reinterpret_cast<TextClassifier*>(ptr);
  std::string context_utf8 = ToStlString(env, context);
  std::vector<AnnotatedSpan> annotations =
      model->Annotate(context_utf8, FromJavaAnnotationOptions(env, options));

  jclass result_class =
      env->FindClass(TC_PACKAGE_PATH TC_CLASS_NAME_STR "$AnnotatedSpan");
  if (!result_class) {
    TC_LOG(ERROR) << "Couldn't find result class: "
                  << TC_PACKAGE_PATH TC_CLASS_NAME_STR "$AnnotatedSpan";
    return nullptr;
  }

  jmethodID result_class_constructor = env->GetMethodID(
      result_class, "<init>",
      "(II[L" TC_PACKAGE_PATH TC_CLASS_NAME_STR "$ClassificationResult;)V");

  jobjectArray results =
      env->NewObjectArray(annotations.size(), result_class, nullptr);

  for (int i = 0; i < annotations.size(); ++i) {
    CodepointSpan span_bmp =
        ConvertIndicesUTF8ToBMP(context_utf8, annotations[i].span);
    jobject result = env->NewObject(
        result_class, result_class_constructor,
        static_cast<jint>(span_bmp.first), static_cast<jint>(span_bmp.second),
        ClassificationResultsToJObjectArray(env,

                                            annotations[i].classification));
    env->SetObjectArrayElement(results, i, result);
    env->DeleteLocalRef(result);
  }
  env->DeleteLocalRef(result_class);
  return results;
}

JNI_METHOD(void, TC_CLASS_NAME, nativeClose)
(JNIEnv* env, jobject thiz, jlong ptr) {
  TextClassifier* model = reinterpret_cast<TextClassifier*>(ptr);
  delete model;
}

JNI_METHOD(jstring, TC_CLASS_NAME, nativeGetLanguage)
(JNIEnv* env, jobject clazz, jint fd) {
  TC_LOG(WARNING) << "Using deprecated getLanguage().";
  return JNI_METHOD_NAME(TC_CLASS_NAME, nativeGetLocales)(env, clazz, fd);
}

JNI_METHOD(jstring, TC_CLASS_NAME, nativeGetLocales)
(JNIEnv* env, jobject clazz, jint fd) {
  const std::unique_ptr<libtextclassifier2::ScopedMmap> mmap(
      new libtextclassifier2::ScopedMmap(fd));
  return GetLocalesFromMmap(env, mmap.get());
}

JNI_METHOD(jstring, TC_CLASS_NAME, nativeGetLocalesFromAssetFileDescriptor)
(JNIEnv* env, jobject thiz, jobject afd, jlong offset, jlong size) {
  const jint fd = libtextclassifier2::GetFdFromAssetFileDescriptor(env, afd);
  const std::unique_ptr<libtextclassifier2::ScopedMmap> mmap(
      new libtextclassifier2::ScopedMmap(fd, offset, size));
  return GetLocalesFromMmap(env, mmap.get());
}

JNI_METHOD(jint, TC_CLASS_NAME, nativeGetVersion)
(JNIEnv* env, jobject clazz, jint fd) {
  const std::unique_ptr<libtextclassifier2::ScopedMmap> mmap(
      new libtextclassifier2::ScopedMmap(fd));
  return GetVersionFromMmap(env, mmap.get());
}

JNI_METHOD(jint, TC_CLASS_NAME, nativeGetVersionFromAssetFileDescriptor)
(JNIEnv* env, jobject thiz, jobject afd, jlong offset, jlong size) {
  const jint fd = libtextclassifier2::GetFdFromAssetFileDescriptor(env, afd);
  const std::unique_ptr<libtextclassifier2::ScopedMmap> mmap(
      new libtextclassifier2::ScopedMmap(fd, offset, size));
  return GetVersionFromMmap(env, mmap.get());
}

JNI_METHOD(jstring, TC_CLASS_NAME, nativeGetName)
(JNIEnv* env, jobject clazz, jint fd) {
  const std::unique_ptr<libtextclassifier2::ScopedMmap> mmap(
      new libtextclassifier2::ScopedMmap(fd));
  return GetNameFromMmap(env, mmap.get());
}

JNI_METHOD(jstring, TC_CLASS_NAME, nativeGetNameFromAssetFileDescriptor)
(JNIEnv* env, jobject thiz, jobject afd, jlong offset, jlong size) {
  const jint fd = libtextclassifier2::GetFdFromAssetFileDescriptor(env, afd);
  const std::unique_ptr<libtextclassifier2::ScopedMmap> mmap(
      new libtextclassifier2::ScopedMmap(fd, offset, size));
  return GetNameFromMmap(env, mmap.get());
}
