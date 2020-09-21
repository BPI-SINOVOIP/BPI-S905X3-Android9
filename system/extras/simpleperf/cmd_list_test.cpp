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

class ListCommandTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    list_cmd = CreateCommandInstance("list");
    ASSERT_TRUE(list_cmd != nullptr);
  }

  std::unique_ptr<Command> list_cmd;
};

TEST_F(ListCommandTest, no_options) {
  ASSERT_TRUE(list_cmd->Run({}));
}

TEST_F(ListCommandTest, one_option) {
  ASSERT_TRUE(list_cmd->Run({"sw"}));
}

TEST_F(ListCommandTest, multiple_options) {
  ASSERT_TRUE(list_cmd->Run({"hw", "tracepoint"}));
}

TEST_F(ListCommandTest, show_features_option) {
  ASSERT_TRUE(list_cmd->Run({"--show-features"}));
}
