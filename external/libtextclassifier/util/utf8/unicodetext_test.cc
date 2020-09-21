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

#include "util/utf8/unicodetext.h"

#include "gtest/gtest.h"

namespace libtextclassifier2 {
namespace {

class UnicodeTextTest : public testing::Test {
 protected:
  UnicodeTextTest() : empty_text_() {
    text_.AppendCodepoint(0x1C0);
    text_.AppendCodepoint(0x4E8C);
    text_.AppendCodepoint(0xD7DB);
    text_.AppendCodepoint(0x34);
    text_.AppendCodepoint(0x1D11E);
  }

  UnicodeText empty_text_;
  UnicodeText text_;
};

// Tests for our modifications of UnicodeText.
TEST(UnicodeTextTest, Custom) {
  UnicodeText text = UTF8ToUnicodeText("1234😋hello", /*do_copy=*/false);
  EXPECT_EQ(text.ToUTF8String(), "1234😋hello");
  EXPECT_EQ(text.size_codepoints(), 10);
  EXPECT_EQ(text.size_bytes(), 13);

  auto it_begin = text.begin();
  std::advance(it_begin, 4);
  auto it_end = text.begin();
  std::advance(it_end, 6);
  EXPECT_EQ(text.UTF8Substring(it_begin, it_end), "😋h");
}

TEST(UnicodeTextTest, Ownership) {
  const std::string src = "\u304A\u00B0\u106B";

  UnicodeText alias;
  alias.PointToUTF8(src.data(), src.size());
  EXPECT_EQ(alias.data(), src.data());
  UnicodeText::const_iterator it = alias.begin();
  EXPECT_EQ(*it++, 0x304A);
  EXPECT_EQ(*it++, 0x00B0);
  EXPECT_EQ(*it++, 0x106B);
  EXPECT_EQ(it, alias.end());

  UnicodeText t = alias;  // Copy initialization copies the data.
  EXPECT_NE(t.data(), alias.data());
}

TEST(UnicodeTextTest, Validation) {
  EXPECT_TRUE(UTF8ToUnicodeText("1234😋hello", /*do_copy=*/false).is_valid());
  EXPECT_TRUE(
      UTF8ToUnicodeText("\u304A\u00B0\u106B", /*do_copy=*/false).is_valid());
  EXPECT_TRUE(
      UTF8ToUnicodeText("this is a test😋😋😋", /*do_copy=*/false).is_valid());
  EXPECT_TRUE(
      UTF8ToUnicodeText("\xf0\x9f\x98\x8b", /*do_copy=*/false).is_valid());
  // Too short (string is too short).
  EXPECT_FALSE(UTF8ToUnicodeText("\xf0\x9f", /*do_copy=*/false).is_valid());
  // Too long (too many trailing bytes).
  EXPECT_FALSE(
      UTF8ToUnicodeText("\xf0\x9f\x98\x8b\x8b", /*do_copy=*/false).is_valid());
  // Too short (too few trailing bytes).
  EXPECT_FALSE(
      UTF8ToUnicodeText("\xf0\x9f\x98\x61\x61", /*do_copy=*/false).is_valid());
  // Invalid with context.
  EXPECT_FALSE(
      UTF8ToUnicodeText("hello \xf0\x9f\x98\x61\x61 world1", /*do_copy=*/false)
          .is_valid());
}

class IteratorTest : public UnicodeTextTest {};

TEST_F(IteratorTest, Iterates) {
  UnicodeText::const_iterator iter = text_.begin();
  EXPECT_EQ(0x1C0, *iter);
  EXPECT_EQ(&iter, &++iter);  // operator++ returns *this.
  EXPECT_EQ(0x4E8C, *iter++);
  EXPECT_EQ(0xD7DB, *iter);
  // Make sure you can dereference more than once.
  EXPECT_EQ(0xD7DB, *iter);
  EXPECT_EQ(0x34, *++iter);
  EXPECT_EQ(0x1D11E, *++iter);
  ASSERT_TRUE(iter != text_.end());
  iter++;
  EXPECT_TRUE(iter == text_.end());
}

TEST_F(IteratorTest, MultiPass) {
  // Also tests Default Constructible and Assignable.
  UnicodeText::const_iterator i1, i2;
  i1 = text_.begin();
  i2 = i1;
  EXPECT_EQ(0x4E8C, *++i1);
  EXPECT_TRUE(i1 != i2);
  EXPECT_EQ(0x1C0, *i2);
  ++i2;
  EXPECT_TRUE(i1 == i2);
  EXPECT_EQ(0x4E8C, *i2);
}

TEST_F(IteratorTest, ReverseIterates) {
  UnicodeText::const_iterator iter = text_.end();
  EXPECT_TRUE(iter == text_.end());
  iter--;
  ASSERT_TRUE(iter != text_.end());
  EXPECT_EQ(0x1D11E, *iter--);
  EXPECT_EQ(0x34, *iter);
  EXPECT_EQ(0xD7DB, *--iter);
  // Make sure you can dereference more than once.
  EXPECT_EQ(0xD7DB, *iter);
  --iter;
  EXPECT_EQ(0x4E8C, *iter--);
  EXPECT_EQ(0x1C0, *iter);
  EXPECT_TRUE(iter == text_.begin());
}

TEST_F(IteratorTest, Comparable) {
  UnicodeText::const_iterator i1, i2;
  i1 = text_.begin();
  i2 = i1;
  ++i2;

  EXPECT_TRUE(i1 < i2);
  EXPECT_TRUE(text_.begin() <= i1);
  EXPECT_FALSE(i1 >= i2);
  EXPECT_FALSE(i1 > text_.end());
}

TEST_F(IteratorTest, Advance) {
  UnicodeText::const_iterator iter = text_.begin();
  EXPECT_EQ(0x1C0, *iter);
  std::advance(iter, 4);
  EXPECT_EQ(0x1D11E, *iter);
  ++iter;
  EXPECT_TRUE(iter == text_.end());
}

TEST_F(IteratorTest, Distance) {
  UnicodeText::const_iterator iter = text_.begin();
  EXPECT_EQ(0, std::distance(text_.begin(), iter));
  EXPECT_EQ(5, std::distance(iter, text_.end()));
  ++iter;
  ++iter;
  EXPECT_EQ(2, std::distance(text_.begin(), iter));
  EXPECT_EQ(3, std::distance(iter, text_.end()));
  ++iter;
  ++iter;
  EXPECT_EQ(4, std::distance(text_.begin(), iter));
  ++iter;
  EXPECT_EQ(0, std::distance(iter, text_.end()));
}

class OperatorTest : public UnicodeTextTest {};

TEST_F(OperatorTest, Clear) {
  UnicodeText empty_text(UTF8ToUnicodeText("", /*do_copy=*/false));
  EXPECT_FALSE(text_ == empty_text);
  text_.clear();
  EXPECT_TRUE(text_ == empty_text);
}

TEST_F(OperatorTest, Empty) {
  EXPECT_TRUE(empty_text_.empty());
  EXPECT_FALSE(text_.empty());
  text_.clear();
  EXPECT_TRUE(text_.empty());
}

}  // namespace
}  // namespace libtextclassifier2
