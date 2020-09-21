/*
 * Copyright (C) 2010 Google Inc.
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

public class LiteralTagInfo extends TagInfo {
  public static final LiteralTagInfo[] EMPTY_ARRAY = new LiteralTagInfo[0];

  public static LiteralTagInfo[] getArray(int size) {
      return size == 0 ? EMPTY_ARRAY : new LiteralTagInfo[size];
  }

  private static String encode(String t) {
    t = t.replace("&", "&amp;");
    t = t.replace("<", "&lt;");
    t = t.replace(">", "&gt;");
    return t;
  }

  public LiteralTagInfo(String text, SourcePositionInfo sp) {
    super("Text", "Text", encode(text), sp);
  }
}
