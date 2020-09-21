// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <gtest/gtest.h>

#include "byte_buffer.h"

namespace {

TEST(ByteBuffer, ReadWrite) {
  struct byte_buffer *b;
  uint8_t *data;
  unsigned int data_size;

  b = byte_buffer_create(100);
  EXPECT_EQ(100, buf_available_bytes(b));
  EXPECT_EQ(0, buf_queued_bytes(b));

  data = buf_read_pointer_size(b, &data_size);
  EXPECT_NE((void *)NULL, data);
  EXPECT_EQ(0, data_size);

  data = buf_write_pointer_size(b, &data_size);
  EXPECT_NE((void *)NULL, data);
  EXPECT_EQ(100, data_size);

  buf_increment_write(b, 50);
  data = buf_read_pointer_size(b, &data_size);
  EXPECT_NE((void *)NULL, data);
  EXPECT_EQ(50, data_size);

  buf_increment_read(b, 40);
  EXPECT_EQ(10, buf_queued_bytes(b));
  EXPECT_EQ(90, buf_available_bytes(b));

  /* Test write to the end of ring buffer. */
  data = buf_write_pointer_size(b, &data_size);
  EXPECT_NE((void *)NULL, data);
  EXPECT_EQ(50, data_size);

  buf_increment_write(b, 50);
  data = buf_write_pointer_size(b, &data_size);
  EXPECT_NE((void *)NULL, data);
  EXPECT_EQ(40, data_size);

  byte_buffer_destroy(b);
}

TEST(ByteBuffer, SetUsedSizeReadWrite) {
  struct byte_buffer *b;
  uint8_t *data;
  unsigned int data_size;

  b = byte_buffer_create(100);
  EXPECT_EQ(100, buf_available_bytes(b));
  EXPECT_EQ(0, buf_queued_bytes(b));

  /* Test set used_size to limit the initial allocated max size. */
  byte_buffer_set_used_size(b, 90);
  EXPECT_EQ(90, buf_available_bytes(b));

  data = buf_write_pointer_size(b, &data_size);
  EXPECT_NE((void *)NULL, data);
  EXPECT_EQ(90, data_size);

  buf_increment_write(b, 90);
  data = buf_read_pointer_size(b, &data_size);
  EXPECT_NE((void *)NULL, data);
  EXPECT_EQ(90, data_size);

  buf_increment_read(b, 50);
  EXPECT_EQ(50, buf_available_bytes(b));
  EXPECT_EQ(40, buf_queued_bytes(b));

  data = buf_write_pointer_size(b, &data_size);
  EXPECT_NE((void *)NULL, data);
  EXPECT_EQ(50, data_size);

  buf_increment_write(b, 50);
  data = buf_write_pointer_size(b, &data_size);
  EXPECT_NE((void *)NULL, data);
  EXPECT_EQ(0, data_size);

  /* Test read to the end of ring buffer. */
  data = buf_read_pointer_size(b, &data_size);
  EXPECT_NE((void *)NULL, data);
  EXPECT_EQ(40, data_size);

  buf_increment_read(b, 40);
  data = buf_read_pointer_size(b, &data_size);
  EXPECT_NE((void *)NULL, data);
  EXPECT_EQ(50, data_size);

  byte_buffer_destroy(b);
}

} // namespace

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}