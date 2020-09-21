/*
 * Copyright 2016 The Android Open Source Project
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
#include <stdio.h>

#define LOG_TAG "VtsTestBinary"
#include <utils/Log.h>

int main() {
  char* pt = 0;

  printf("crash_app: start");
  ALOGI("crash_app: start");
  *pt = 0xab;  // causes a null pointer exception.
  printf("crash_app: end");
  ALOGI("crash_app: end");
  return 0;
}
