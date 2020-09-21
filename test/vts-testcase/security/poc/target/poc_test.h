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

#ifndef __VTS_TESTCASES_SECURITY_POC_TARGET_POC_TEST_H__
#define __VTS_TESTCASES_SECURITY_POC_TARGET_POC_TEST_H__

#include <map>
#include <string>

/* define poc_test exit codes */
#define POC_TEST_PASS 0
#define POC_TEST_FAIL 1
#define POC_TEST_SKIP 2

typedef enum {
  NEXUS_5,
  NEXUS_5X,
  NEXUS_6,
  NEXUS_6P,
  PIXEL,
  PIXEL_XL,
  OTHER
} DeviceModel;

typedef struct {
  DeviceModel device_model;
  std::map<std::string, std::string> params;
} VtsHostInput;

extern VtsHostInput ParseVtsHostFlags(int argc, char *argv[]);

#endif  // __VTS_TESTCASES_SECURITY_POC_TARGET_POC_TEST_H__
