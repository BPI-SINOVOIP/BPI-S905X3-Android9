/*
 * Copyright (C) 2015 The Android Open Source Project
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

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "read_elf.h"
#include "workload.h"

static const std::string SLEEP_SEC = "0.001";

void RunWorkloadFunction();
void CreateProcesses(size_t count, std::vector<std::unique_ptr<Workload>>* workloads);

void ParseSymbol(const ElfFileSymbol& symbol, std::map<std::string, ElfFileSymbol>* symbols);
void CheckElfFileSymbols(const std::map<std::string, ElfFileSymbol>& symbols);

bool IsRoot();

#define TEST_IN_ROOT(TestStatement)                                                                \
  do {                                                                                             \
    if (IsRoot()) {                                                                                \
      TestStatement;                                                                               \
    } else {                                                                                       \
      GTEST_LOG_(INFO) << "Didn't test \"" << #TestStatement << "\" requires root privileges";     \
    }                                                                                              \
  } while (0)

#if defined(__ANDROID__)
#define TEST_REQUIRE_HOST_ROOT()
#else
#define TEST_REQUIRE_HOST_ROOT()  if (!IsRoot()) return
#endif

bool IsInNativeAbi();
// Used to skip tests not supposed to run on non-native ABIs.
#define OMIT_TEST_ON_NON_NATIVE_ABIS()  \
  do { \
    if (!IsInNativeAbi()) { \
      GTEST_LOG_(INFO) << "Skip this test as it only runs on native ABIs."; \
      return; \
    } \
  } while (0)
