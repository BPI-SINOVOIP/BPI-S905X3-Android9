// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <gtest/gtest.h>

#include "array.h"

namespace {

struct point {
  int x, y;
};

DECLARE_ARRAY_TYPE(double, double_array);
DECLARE_ARRAY_TYPE(struct point, point_array);

TEST(ArrayTest, Basic) {
  double_array a = ARRAY_INIT;

  /* create an array {1.0, 2.0} */
  ARRAY_APPEND(&a, 1.0);
  double *p = ARRAY_APPEND_ZERO(&a);
  EXPECT_EQ(0.0, *p);
  *p = 2.0;

  EXPECT_EQ(2, ARRAY_COUNT(&a));
  EXPECT_EQ(2, a.count);
  EXPECT_GE(a.size, 2);
  EXPECT_EQ(1.0, *ARRAY_ELEMENT(&a, 0));
  EXPECT_EQ(2.0, *ARRAY_ELEMENT(&a, 1));
  EXPECT_EQ(1.0, a.element[0]);
  EXPECT_EQ(2.0, a.element[1]);
  EXPECT_EQ(0, ARRAY_FIND(&a, 1.0));
  EXPECT_EQ(1, ARRAY_FIND(&a, 2.0));
  EXPECT_EQ(-1, ARRAY_FIND(&a, 0.0));
  EXPECT_EQ(0, ARRAY_INDEX(&a, ARRAY_ELEMENT(&a, 0)));
  EXPECT_EQ(1, ARRAY_INDEX(&a, ARRAY_ELEMENT(&a, 1)));

  ARRAY_FREE(&a);
  EXPECT_EQ(0, ARRAY_COUNT(&a));
  EXPECT_EQ(0, a.count);
  EXPECT_EQ(0, a.size);
  EXPECT_EQ(NULL, a.element);
}

TEST(ArrayTest, StructElement) {
  struct point p = {1, 2};
  struct point q = {3, 4};
  point_array a = ARRAY_INIT;

  ARRAY_APPEND(&a, p);
  ARRAY_APPEND(&a, q);

  EXPECT_EQ(2, ARRAY_COUNT(&a));
  EXPECT_EQ(1, ARRAY_ELEMENT(&a, 0)->x);
  EXPECT_EQ(2, ARRAY_ELEMENT(&a, 0)->y);
  EXPECT_EQ(3, a.element[1].x);
  EXPECT_EQ(4, a.element[1].y);
  ARRAY_ELEMENT(&a, 1)->y = 5;
  EXPECT_EQ(5, a.element[1].y);

  ARRAY_FREE(&a);
  EXPECT_EQ(0, ARRAY_COUNT(&a));
  EXPECT_EQ(0, a.size);
  EXPECT_EQ(NULL, a.element);
}

TEST(ArrayTest, AppendZeroStruct) {
  point_array a = ARRAY_INIT;
  struct point *p, *q;

  p = ARRAY_APPEND_ZERO(&a);
  EXPECT_EQ(0, p->x);
  EXPECT_EQ(0, p->y);
  EXPECT_EQ(1, a.count);

  q = ARRAY_APPEND_ZERO(&a);
  EXPECT_EQ(0, q->x);
  EXPECT_EQ(0, q->y);
  EXPECT_EQ(2, a.count);

  ARRAY_FREE(&a);
}

TEST(ArrayTest, ForLoop) {
  int i;
  double *p;
  double_array a = ARRAY_INIT;

  for (i = 0; i < 100; i++) {
    ARRAY_APPEND(&a, i * 2);
  }

  int expectedIndex = 0;
  double expectedValue = 0;
  FOR_ARRAY_ELEMENT(&a, i, p) {
    EXPECT_EQ(expectedIndex, i);
    EXPECT_EQ(expectedValue, *p);
    expectedIndex++;
    expectedValue += 2;
  }
  EXPECT_EQ(expectedIndex, 100);

  ARRAY_FREE(&a);
  EXPECT_EQ(0, a.count);
  EXPECT_EQ(0, a.size);
  EXPECT_EQ(NULL, a.element);
}

}  //  namespace

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
