/*
 * Copyright 2018 The Android Open Source Project
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
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <gtest/gtest.h>
#define LOG_TAG "VtsSelfTestFlakyTest"
#include <utils/Log.h>

class VtsSelfTestFlakyTest : public ::testing::Test {
 public:
  virtual void SetUp() override {
    struct timeval time;
    gettimeofday(&time, NULL);

    srand((time.tv_sec * 1000) + (time.tv_usec / 1000));
  }

  virtual void TearDown() override {}
};

/**
 * Always passing.
 */
TEST_F(VtsSelfTestFlakyTest, TestAlwaysPassing1) {
  printf("PASS (with 100%% chance)\n");
}

TEST_F(VtsSelfTestFlakyTest, TestAlwaysPassing2) {
  printf("PASS (with 100%% chance)\n");
}

/**
 * Fails with 50% of chance.
 */
TEST_F(VtsSelfTestFlakyTest, TestFlaky1) {
  int number = abs(rand());
  printf("number: %d\n", number);
  ASSERT_TRUE((number % 2) == 0);
}

TEST_F(VtsSelfTestFlakyTest, TestFlaky2) {
  int number = abs(rand());
  printf("number: %d\n", number);
  ASSERT_TRUE((number % 2) == 0);
}

TEST_F(VtsSelfTestFlakyTest, TestFlaky3) {
  int number = abs(rand());
  printf("number: %d\n", number);
  ASSERT_TRUE((number % 2) == 0);
}

TEST_F(VtsSelfTestFlakyTest, TestFlaky4) {
  int number = abs(rand());
  printf("number: %d\n", number);
  ASSERT_TRUE((number % 2) == 0);
}

TEST_F(VtsSelfTestFlakyTest, TestFlaky5) {
  int number = abs(rand());
  printf("number: %d\n", number);
  ASSERT_TRUE((number % 2) == 0);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
