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

#include <algorithm>
#include <iterator>
#include <iostream>

#include <gtest/gtest.h>

#include <apdu/apdu.h>

using android::CommandApdu;
using android::ResponseApdu;

/* CommandApdu */

TEST(CommandApduTest, Case1) {
    const CommandApdu apdu{1, 2, 3, 4};
    const std::vector<uint8_t> expected{1, 2, 3, 4};
    ASSERT_EQ(expected.size(), apdu.size());
    ASSERT_TRUE(std::equal(apdu.begin(), apdu.end(), expected.begin(), expected.end()));
}

TEST(CommandApduTest, Case2s) {
    const CommandApdu apdu{4, 3, 2, 1, 0, 3};
    const std::vector<uint8_t> expected{4, 3, 2, 1, 3};
    ASSERT_EQ(expected.size(), apdu.size());
    ASSERT_TRUE(std::equal(apdu.begin(), apdu.end(), expected.begin(), expected.end()));
}

TEST(CommandApduTest, Case2s_maxLe) {
    const CommandApdu apdu{4, 3, 2, 1, 0, 256};
    const std::vector<uint8_t> expected{4, 3, 2, 1, 0};
    ASSERT_EQ(expected.size(), apdu.size());
    ASSERT_TRUE(std::equal(apdu.begin(), apdu.end(), expected.begin(), expected.end()));
}

TEST(CommandApduTest, Case2e) {
    const CommandApdu apdu{5, 6, 7, 8, 0, 258};
    const std::vector<uint8_t> expected{5, 6, 7, 8, 0, 1, 2};
    ASSERT_EQ(expected.size(), apdu.size());
    ASSERT_TRUE(std::equal(apdu.begin(), apdu.end(), expected.begin(), expected.end()));
}

TEST(CommandApduTest, Case2e_maxLe) {
    const CommandApdu apdu{5, 6, 7, 8, 0, 65536};
    const std::vector<uint8_t> expected{5, 6, 7, 8, 0, 0, 0};
    ASSERT_EQ(expected.size(), apdu.size());
    ASSERT_TRUE(std::equal(apdu.begin(), apdu.end(), expected.begin(), expected.end()));
}

TEST(CommandApduTest, Case3s) {
    const CommandApdu apdu{8, 7, 6, 5, 5, 0};
    const std::vector<uint8_t> expected{8, 7, 6, 5, 5, 0, 0, 0, 0, 0};
    ASSERT_EQ(expected.size(), apdu.size());
    ASSERT_TRUE(std::equal(apdu.begin(), apdu.end(), expected.begin(), expected.end()));
}

TEST(CommandApduTest, Case3s_data) {
    CommandApdu apdu{8, 7, 6, 5, 3, 0};
    auto it = apdu.dataBegin();
    *it++ = 10;
    *it++ = 11;
    *it++ = 12;
    ASSERT_EQ(apdu.dataEnd(), it);

    const std::vector<uint8_t> expected{8, 7, 6, 5, 3, 10, 11, 12};
    ASSERT_EQ(expected.size(), apdu.size());
    ASSERT_TRUE(std::equal(apdu.begin(), apdu.end(), expected.begin(), expected.end()));
}

TEST(CommandApduTest, Case3e) {
    const CommandApdu apdu{8, 7, 6, 5, 256, 0};
    std::vector<uint8_t> expected{8, 7, 6, 5, 0, 1, 0};
    expected.resize(expected.size() + 256, 0);
    ASSERT_EQ(expected.size(), apdu.size());
    ASSERT_TRUE(std::equal(apdu.begin(), apdu.end(), expected.begin(), expected.end()));
}

TEST(CommandApduTest, Case3e_data) {
    CommandApdu apdu{8, 7, 6, 5, 65535, 0};
    ASSERT_EQ(size_t{65535}, apdu.dataSize());
    std::fill(apdu.dataBegin(), apdu.dataEnd(), 7);
    std::vector<uint8_t> expected{8, 7, 6, 5, 0, 255, 255};
    expected.resize(expected.size() + 65535, 7);
    ASSERT_EQ(expected.size(), apdu.size());
    ASSERT_TRUE(std::equal(apdu.begin(), apdu.end(), expected.begin(), expected.end()));
}

TEST(CommandApduTest, Case4s) {
    const CommandApdu apdu{1, 3, 5, 7, 2, 3};
    const std::vector<uint8_t> expected{1, 3, 5, 7, 2, 0, 0, 3};
    ASSERT_EQ(expected.size(), apdu.size());
    ASSERT_TRUE(std::equal(apdu.begin(), apdu.end(), expected.begin(), expected.end()));
}

TEST(CommandApduTest, Case4s_data) {
    CommandApdu apdu{1, 3, 5, 7, 1, 90};
    auto it = apdu.dataBegin();
    *it++ = 8;
    ASSERT_EQ(apdu.dataEnd(), it);

    const std::vector<uint8_t> expected{1, 3, 5, 7, 1, 8, 90};
    ASSERT_EQ(expected.size(), apdu.size());
    ASSERT_TRUE(std::equal(apdu.begin(), apdu.end(), expected.begin(), expected.end()));
}

TEST(CommandApduTest, Case4s_maxLe) {
    const CommandApdu apdu{1, 3, 5, 7, 2, 256};
    const std::vector<uint8_t> expected{1, 3, 5, 7, 2, 0, 0, 0};
    ASSERT_EQ(expected.size(), apdu.size());
    ASSERT_TRUE(std::equal(apdu.begin(), apdu.end(), expected.begin(), expected.end()));
}

TEST(CommandApduTest, Case4e) {
    const CommandApdu apdu{1, 3, 5, 7, 527, 349};
    std::vector<uint8_t> expected{1, 3, 5, 7, 0, 2, 15};
    expected.resize(expected.size() + 527, 0);
    expected.push_back(1);
    expected.push_back(93);
    ASSERT_EQ(expected.size(), apdu.size());
    ASSERT_TRUE(std::equal(apdu.begin(), apdu.end(), expected.begin(), expected.end()));
}

TEST(CommandApduTest, Case4e_maxLe) {
    const CommandApdu apdu{1, 3, 5, 7, 20, 65536};
    std::vector<uint8_t> expected{1, 3, 5, 7, 0, 0, 20};
    expected.resize(expected.size() + 20, 0);
    expected.push_back(0);
    expected.push_back(0);
    ASSERT_EQ(expected.size(), apdu.size());
    ASSERT_TRUE(std::equal(apdu.begin(), apdu.end(), expected.begin(), expected.end()));
}

/* ResponseApdu */

TEST(ResponseApduTest, bad) {
    const std::vector<uint8_t> empty{};
    const ResponseApdu<std::vector<uint8_t>> apdu{empty};
    ASSERT_FALSE(apdu.ok());
}

TEST(ResponseApduTest, statusOnly) {
    const std::vector<uint8_t> statusOnly{0x90, 0x37};
    const ResponseApdu<std::vector<uint8_t>> apdu{statusOnly};
    ASSERT_TRUE(apdu.ok());
    ASSERT_EQ(0x90, apdu.sw1());
    ASSERT_EQ(0x37, apdu.sw2());
    ASSERT_EQ(0x9037, apdu.status());
    ASSERT_EQ(size_t{0}, apdu.dataSize());
}

TEST(ResponseApduTest, data) {
    const std::vector<uint8_t> data{1, 2, 3, 9, 8, 7, 0x3a, 0xbc};
    const ResponseApdu<std::vector<uint8_t>> apdu{data};
    ASSERT_TRUE(apdu.ok());
    ASSERT_EQ(0x3abc, apdu.status());
    ASSERT_EQ(size_t{6}, apdu.dataSize());

    const uint8_t expected[] = {1, 2, 3, 9, 8, 7};
    ASSERT_TRUE(std::equal(apdu.dataBegin(), apdu.dataEnd(),
                           std::begin(expected), std::end(expected)));
}

TEST(ResponseApduTest, remainingBytes) {
    const std::vector<uint8_t> remainingBytes{0x61, 23};
    const ResponseApdu<std::vector<uint8_t>> apdu{remainingBytes};
    ASSERT_EQ(23, apdu.remainingBytes());
    ASSERT_FALSE(apdu.isWarning());
    ASSERT_FALSE(apdu.isExecutionError());
    ASSERT_FALSE(apdu.isCheckingError());
    ASSERT_FALSE(apdu.isError());
}

TEST(ResponseApduTest, warning) {
    const std::vector<uint8_t> warning{0x62, 0};
    const ResponseApdu<std::vector<uint8_t>> apdu{warning};
    ASSERT_TRUE(apdu.isWarning());
    ASSERT_FALSE(apdu.isExecutionError());
    ASSERT_FALSE(apdu.isCheckingError());
    ASSERT_FALSE(apdu.isError());
}

TEST(ResponseApduTest, executionError) {
    const std::vector<uint8_t> executionError{0x66, 0};
    const ResponseApdu<std::vector<uint8_t>> apdu{executionError};
    ASSERT_FALSE(apdu.isWarning());
    ASSERT_TRUE(apdu.isExecutionError());
    ASSERT_FALSE(apdu.isCheckingError());
    ASSERT_TRUE(apdu.isError());
}

TEST(ResponseApduTest, checkingError) {
    const std::vector<uint8_t> checkingError{0x67, 0};
    const ResponseApdu<std::vector<uint8_t>> apdu{checkingError};
    ASSERT_FALSE(apdu.isWarning());
    ASSERT_FALSE(apdu.isExecutionError());
    ASSERT_TRUE(apdu.isCheckingError());
    ASSERT_TRUE(apdu.isError());
}
