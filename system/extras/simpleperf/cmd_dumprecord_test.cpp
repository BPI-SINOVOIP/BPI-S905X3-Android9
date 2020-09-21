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

#include <gtest/gtest.h>

#include "command.h"
#include "get_test_data.h"

static std::unique_ptr<Command> DumpCmd() {
  return CreateCommandInstance("dump");
}

TEST(cmd_dump, record_file_option) {
  ASSERT_TRUE(DumpCmd()->Run({GetTestData("perf.data")}));
}

TEST(cmd_dump, dump_data_generated_by_linux_perf) {
  ASSERT_TRUE(DumpCmd()->Run({GetTestData(PERF_DATA_GENERATED_BY_LINUX_PERF)}));
}

TEST(cmd_dump, dump_callchain_records) {
  ASSERT_TRUE(DumpCmd()->Run({GetTestData(PERF_DATA_WITH_CALLCHAIN_RECORD)}));
}
