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

package android.location.cts.asn1.base;

/**
 * Tag class of an ASN.1 type.
 */
public enum Asn1TagClass {
  UNIVERSAL(0),
  APPLICATION(1),
  CONTEXT(2),
  PRIVATE(3);

  private final int value;

  private Asn1TagClass(int value) {
    this.value = value;
  }

  public static Asn1TagClass fromValue(int tagClass) {
    switch (tagClass) {
      case 0: return UNIVERSAL;
      case 1: return APPLICATION;
      case 2: return CONTEXT;
      case 3: return PRIVATE;
      default:
        throw new IllegalArgumentException("Invalid tag class: " + tagClass);
    }
  }

  public int getValue() {
    return value;
  }
}
