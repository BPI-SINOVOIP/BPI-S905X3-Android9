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

import com.google.clearsilver.jsilver.data.Data;

import java.util.Map;

public class AuxTagInfo extends TagInfo {
  public static final AuxTagInfo[] EMPTY_ARRAY = new AuxTagInfo[0];

  public static AuxTagInfo[] getArray(int size) {
      return size == 0 ? EMPTY_ARRAY : new AuxTagInfo[size];
  }

  private Map<String, String> mArgs;
  private TagInfo[] mValues;

  AuxTagInfo(String name, String kind, SourcePositionInfo position, Map<String, String> args,
      TagInfo[] values) {
    super(name, kind, "", position);
    mArgs = args;
    mValues = values;
  }

  private TagInfo[] valuesTags() {
    return mValues;
  }

  @Override
  public void makeHDF(Data data, String base) {
    super.makeHDF(data, base);
    for (Map.Entry<String, String> entry : mArgs.entrySet()) {
      data.setValue(base + "." + entry.getKey(), entry.getValue());
    }
    TagInfo.makeHDF(data, base + ".values", valuesTags());
  }
}
