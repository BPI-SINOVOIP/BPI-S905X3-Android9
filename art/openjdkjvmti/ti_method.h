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

#ifndef ART_OPENJDKJVMTI_TI_METHOD_H_
#define ART_OPENJDKJVMTI_TI_METHOD_H_

#include "dex/primitive.h"
#include "jni.h"
#include "jvmti.h"

namespace openjdkjvmti {

class EventHandler;

class MethodUtil {
 public:
  static void Register(EventHandler* event_handler);
  static void Unregister();

  static jvmtiError GetBytecodes(jvmtiEnv* env,
                                 jmethodID method,
                                 jint* count_ptr,
                                 unsigned char** bytecodes);

  static jvmtiError GetArgumentsSize(jvmtiEnv* env, jmethodID method, jint* size_ptr);

  static jvmtiError GetMaxLocals(jvmtiEnv* env, jmethodID method, jint* max_ptr);

  static jvmtiError GetMethodName(jvmtiEnv* env,
                                  jmethodID method,
                                  char** name_ptr,
                                  char** signature_ptr,
                                  char** generic_ptr);

  static jvmtiError GetMethodDeclaringClass(jvmtiEnv* env,
                                            jmethodID method,
                                            jclass* declaring_class_ptr);

  static jvmtiError GetMethodLocation(jvmtiEnv* env,
                                      jmethodID method,
                                      jlocation* start_location_ptr,
                                      jlocation* end_location_ptr);

  static jvmtiError GetMethodModifiers(jvmtiEnv* env,
                                       jmethodID method,
                                       jint* modifiers_ptr);

  static jvmtiError GetLineNumberTable(jvmtiEnv* env,
                                       jmethodID method,
                                       jint* entry_count_ptr,
                                       jvmtiLineNumberEntry** table_ptr);

  static jvmtiError IsMethodNative(jvmtiEnv* env, jmethodID method, jboolean* is_native_ptr);
  static jvmtiError IsMethodObsolete(jvmtiEnv* env, jmethodID method, jboolean* is_obsolete_ptr);
  static jvmtiError IsMethodSynthetic(jvmtiEnv* env, jmethodID method, jboolean* is_synthetic_ptr);
  static jvmtiError GetLocalVariableTable(jvmtiEnv* env,
                                          jmethodID method,
                                          jint* entry_count_ptr,
                                          jvmtiLocalVariableEntry** table_ptr);

  template<typename T>
  static jvmtiError SetLocalVariable(jvmtiEnv* env, jthread thread, jint depth, jint slot, T data);

  template<typename T>
  static jvmtiError GetLocalVariable(jvmtiEnv* env, jthread thread, jint depth, jint slot, T* data);

  static jvmtiError GetLocalInstance(jvmtiEnv* env, jthread thread, jint depth, jobject* data);

 private:
  static jvmtiError SetLocalVariableGeneric(jvmtiEnv* env,
                                            jthread thread,
                                            jint depth,
                                            jint slot,
                                            art::Primitive::Type type,
                                            jvalue value);
  static jvmtiError GetLocalVariableGeneric(jvmtiEnv* env,
                                            jthread thread,
                                            jint depth,
                                            jint slot,
                                            art::Primitive::Type type,
                                            jvalue* value);
};

}  // namespace openjdkjvmti

#endif  // ART_OPENJDKJVMTI_TI_METHOD_H_
