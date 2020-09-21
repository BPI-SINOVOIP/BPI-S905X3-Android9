/*
 * Copyright (C) 2017 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.google.doclava;

public interface AuxSource {
  public TagInfo[] classAuxTags(ClassInfo clazz);
  public TagInfo[] fieldAuxTags(FieldInfo field);
  public TagInfo[] methodAuxTags(MethodInfo method);
  public TagInfo[] paramAuxTags(MethodInfo method, ParameterInfo param, String comment);
  public TagInfo[] returnAuxTags(MethodInfo method);
}

class EmptyAuxSource implements AuxSource {
  @Override
  public TagInfo[] classAuxTags(ClassInfo clazz) {
    return TagInfo.EMPTY_ARRAY;
  }

  @Override
  public TagInfo[] fieldAuxTags(FieldInfo field) {
    return TagInfo.EMPTY_ARRAY;
  }

  @Override
  public TagInfo[] methodAuxTags(MethodInfo method) {
    return TagInfo.EMPTY_ARRAY;
  }

  @Override
  public TagInfo[] paramAuxTags(MethodInfo method, ParameterInfo param, String comment) {
    return TagInfo.EMPTY_ARRAY;
  }

  @Override
  public TagInfo[] returnAuxTags(MethodInfo method) {
    return TagInfo.EMPTY_ARRAY;
  }
}
