/*
 * Copyright (C) 2013 The Android Open Source Project
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

#include <errno.h>
#include <sys/syscall.h>
#include <sys/time.h>

#include "TemporaryFile.h"

// http://b/11383777
TEST(sys_time, utimes_nullptr) {
  TemporaryFile tf;
  ASSERT_EQ(0, utimes(tf.filename, nullptr));
}

TEST(sys_time, utimes_EINVAL) {
  TemporaryFile tf;

  timeval tv[2] = {};

  tv[0].tv_usec = -123;
  ASSERT_EQ(-1, utimes(tf.filename, tv));
  ASSERT_EQ(EINVAL, errno);
  tv[0].tv_usec = 1234567;
  ASSERT_EQ(-1, utimes(tf.filename, tv));
  ASSERT_EQ(EINVAL, errno);

  tv[0].tv_usec = 0;

  tv[1].tv_usec = -123;
  ASSERT_EQ(-1, utimes(tf.filename, tv));
  ASSERT_EQ(EINVAL, errno);
  tv[1].tv_usec = 1234567;
  ASSERT_EQ(-1, utimes(tf.filename, tv));
  ASSERT_EQ(EINVAL, errno);
}

TEST(sys_time, futimes_nullptr) {
  TemporaryFile tf;
  ASSERT_EQ(0, futimes(tf.fd, nullptr));
}

TEST(sys_time, futimes_EINVAL) {
  TemporaryFile tf;

  timeval tv[2] = {};

  tv[0].tv_usec = -123;
  ASSERT_EQ(-1, futimes(tf.fd, tv));
  ASSERT_EQ(EINVAL, errno);
  tv[0].tv_usec = 1234567;
  ASSERT_EQ(-1, futimes(tf.fd, tv));
  ASSERT_EQ(EINVAL, errno);

  tv[0].tv_usec = 0;

  tv[1].tv_usec = -123;
  ASSERT_EQ(-1, futimes(tf.fd, tv));
  ASSERT_EQ(EINVAL, errno);
  tv[1].tv_usec = 1234567;
  ASSERT_EQ(-1, futimes(tf.fd, tv));
  ASSERT_EQ(EINVAL, errno);
}

TEST(sys_time, futimesat_nullptr) {
  TemporaryFile tf;
  ASSERT_EQ(0, futimesat(AT_FDCWD, tf.filename, nullptr));
}

TEST(sys_time, futimesat_EINVAL) {
  TemporaryFile tf;

  timeval tv[2] = {};

  tv[0].tv_usec = -123;
  ASSERT_EQ(-1, futimesat(AT_FDCWD, tf.filename, tv));
  ASSERT_EQ(EINVAL, errno);
  tv[0].tv_usec = 1234567;
  ASSERT_EQ(-1, futimesat(AT_FDCWD, tf.filename, tv));
  ASSERT_EQ(EINVAL, errno);

  tv[0].tv_usec = 0;

  tv[1].tv_usec = -123;
  ASSERT_EQ(-1, futimesat(AT_FDCWD, tf.filename, tv));
  ASSERT_EQ(EINVAL, errno);
  tv[1].tv_usec = 1234567;
  ASSERT_EQ(-1, futimesat(AT_FDCWD, tf.filename, tv));
  ASSERT_EQ(EINVAL, errno);
}

TEST(sys_time, lutimes_nullptr) {
  TemporaryFile tf;
  ASSERT_EQ(0, lutimes(tf.filename, nullptr));
}

TEST(sys_time, lutimes_EINVAL) {
  TemporaryFile tf;

  timeval tv[2] = {};

  tv[0].tv_usec = -123;
  ASSERT_EQ(-1, lutimes(tf.filename, tv));
  ASSERT_EQ(EINVAL, errno);
  tv[0].tv_usec = 1234567;
  ASSERT_EQ(-1, lutimes(tf.filename, tv));
  ASSERT_EQ(EINVAL, errno);

  tv[0].tv_usec = 0;

  tv[1].tv_usec = -123;
  ASSERT_EQ(-1, lutimes(tf.filename, tv));
  ASSERT_EQ(EINVAL, errno);
  tv[1].tv_usec = 1234567;
  ASSERT_EQ(-1, lutimes(tf.filename, tv));
  ASSERT_EQ(EINVAL, errno);
}

TEST(sys_time, gettimeofday) {
  // Try to ensure that our vdso gettimeofday is working.
  timeval tv1;
  ASSERT_EQ(0, gettimeofday(&tv1, NULL));
  timeval tv2;
  ASSERT_EQ(0, syscall(__NR_gettimeofday, &tv2, NULL));

  // What's the difference between the two?
  tv2.tv_sec -= tv1.tv_sec;
  tv2.tv_usec -= tv1.tv_usec;
  if (tv2.tv_usec < 0) {
    --tv2.tv_sec;
    tv2.tv_usec += 1000000;
  }

  // Should be less than (a very generous, to try to avoid flakiness) 2ms (2000us).
  ASSERT_EQ(0, tv2.tv_sec);
  ASSERT_LT(tv2.tv_usec, 2000);
}
