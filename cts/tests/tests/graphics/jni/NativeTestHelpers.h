/*
 * Copyright 2018 The Android Open Source Project
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
 *
 */

#ifndef ANDROID_NATIVETESTHELPERS_H
#define ANDROID_NATIVETESTHELPERS_H

#include <android/log.h>
#include <jni.h>

#define ASSERT(condition, format, args...)                                     \
  if (!(condition)) {                                                          \
    fail(env, format, ##args);                                                 \
    return;                                                                    \
  }

#define ASSERT_TRUE(a) ASSERT((a), "assert failed on (" #a ") at " __FILE__ ":%d", __LINE__)
#define ASSERT_FALSE(a) ASSERT(!(a), "assert failed on (!" #a ") at " __FILE__ ":%d", __LINE__)
#define ASSERT_EQ(a, b) \
        ASSERT((a) == (b), "assert failed on (" #a " == " #b ") at " __FILE__ ":%d", __LINE__)
#define ASSERT_NE(a, b) \
        ASSERT((a) != (b), "assert failed on (" #a " != " #b ") at " __FILE__ ":%d", __LINE__)
#define ASSERT_GT(a, b) \
        ASSERT((a) > (b), "assert failed on (" #a " > " #b ") at " __FILE__ ":%d", __LINE__)
#define ASSERT_GE(a, b) \
        ASSERT((a) >= (b), "assert failed on (" #a " >= " #b ") at " __FILE__ ":%d", __LINE__)
#define ASSERT_LT(a, b) \
        ASSERT((a) < (b), "assert failed on (" #a " < " #b ") at " __FILE__ ":%d", __LINE__)
#define ASSERT_LE(a, b) \
        ASSERT((a) <= (b), "assert failed on (" #a " <= " #b ") at " __FILE__ ":%d", __LINE__)

#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Raises a java exception.
void fail(JNIEnv *env, const char *format, ...);

#endif  // ANDROID_NATIVETESTHELPERS_H

