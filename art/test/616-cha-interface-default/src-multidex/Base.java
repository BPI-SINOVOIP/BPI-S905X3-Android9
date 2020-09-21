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

interface Base {
  default public int foo(int i) {
    if (i != 1) {
      return -2;
    }
    return i + 10;
  }

  // Test default method that's not inlined.
  default public int $noinline$bar() {
    System.out.print("");
    System.out.print("");
    System.out.print("");
    System.out.print("");
    System.out.print("");
    System.out.print("");
    System.out.print("");
    System.out.print("");
    return -1;
  }

  default void printError(String msg) {
    System.out.println(msg);
  }
}
