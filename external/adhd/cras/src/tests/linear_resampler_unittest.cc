// Copyright (c) 2014 The Chromium OS Autholr. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <limits.h>
#include <stdio.h>
#include <stdint.h>
#include <gtest/gtest.h>

extern "C" {
#include "linear_resampler.h"
}

#define BUF_SIZE 2048

static uint8_t in_buf[BUF_SIZE];
static uint8_t out_buf[BUF_SIZE];

TEST(LinearResampler, ReampleToSlightlyLargerRate) {
	int i, rc;
	unsigned int count;
	unsigned int in_offset = 0;
	unsigned int out_offset = 0;
	struct linear_resampler *lr;


	memset(in_buf, 0, BUF_SIZE);
	memset(out_buf, 0, BUF_SIZE);
	for (i = 0; i < 100; i++) {
		*((int16_t *)(in_buf + i * 4)) = i * 10;
		*((int16_t *)(in_buf + i * 4 + 2)) = i * 20;
	}

	lr = linear_resampler_create(2, 4, 48000, 48001);

	count = 20;
	rc = linear_resampler_resample(lr, in_buf + 4 * in_offset, &count,
				     out_buf + 4 * out_offset, 50);
	EXPECT_EQ(20, rc);
	EXPECT_EQ(20, count);

	in_offset += count;
	out_offset += rc;
	count = 20;
	rc = linear_resampler_resample(lr, in_buf + 4 * in_offset, &count,
				     out_buf + 4 * out_offset, 15);
	EXPECT_EQ(15, rc);
	EXPECT_EQ(15, count);

	/* Assert linear interpotation result. */
	for (i = 0; i < 34; i++) {
		EXPECT_GE(*(int16_t *)(in_buf + 4 * i),
			  *(int16_t *)(out_buf + 4 * i));
		EXPECT_LE(*(int16_t *)(in_buf + 4 * i),
			  *(int16_t *)(out_buf + 4 * (i + 1)));
	}
}

TEST(LinearResampler, ResampleIntegerFractionToLarger) {
	int i, rc;
	unsigned int count;
	unsigned int in_offset = 0;
	unsigned int out_offset = 0;
	struct linear_resampler *lr;

	memset(in_buf, 0, BUF_SIZE);
	memset(out_buf, 0, BUF_SIZE);
	for (i = 0; i < 100; i++) {
		*((int16_t *)(in_buf + i * 4)) = SHRT_MAX - i;
		*((int16_t *)(in_buf + i * 4 + 2)) = SHRT_MAX - i * 10;
	}

	/* Rate 10 -> 11 */
	lr = linear_resampler_create(2, 4, 10, 11);

	count = 5;
	rc = linear_resampler_resample(lr, in_buf + 4 * in_offset, &count,
				     out_buf + 4 * out_offset, 10);
	EXPECT_EQ(5, rc);
	EXPECT_EQ(5, count);

	in_offset += count;
	out_offset += rc;
	count = 6;
	/* Assert source rate + 1 frames resample to destination rate + 1
	 * frames. */
	rc = linear_resampler_resample(lr, in_buf + 4 * in_offset, &count,
				     out_buf + 4 * out_offset, 10);
	EXPECT_EQ(7, rc);
	EXPECT_EQ(6, count);

	in_offset += count;
	out_offset += rc;
	count = 89;
	rc = linear_resampler_resample(lr, in_buf + 4 * in_offset, &count,
				     out_buf + 4 * out_offset, 100);
	EXPECT_EQ(97, rc);
	EXPECT_EQ(89, count);

	/* Assert linear interpotation result. */
	for (i = 0; i < 90; i++) {
		EXPECT_LE(*(int16_t *)(in_buf + 4 * i),
			  *(int16_t *)(out_buf + 4 * i));
		EXPECT_LE(*(int16_t *)(in_buf + 4 * i + 2),
			  *(int16_t *)(out_buf + 4 * i + 2));
	}
}

TEST(LinearResampler, ResampleIntegerFractionToLess) {
	int i, rc;
	unsigned int count;
	unsigned int in_offset = 0;
	unsigned int out_offset = 0;
	struct linear_resampler *lr;

	memset(in_buf, 0, BUF_SIZE);
	memset(out_buf, 0, BUF_SIZE);
	for (i = 0; i < 100; i++) {
		*((int16_t *)(in_buf + i * 4)) = SHRT_MIN + i * 10;
		*((int16_t *)(in_buf + i * 4 + 2)) = SHRT_MIN + i * 20;
	}

	/* Rate 10 -> 9 */
	lr = linear_resampler_create(2, 4, 10, 9);

	count = 6;
	rc = linear_resampler_resample(lr, in_buf + 4 * in_offset, &count,
				     out_buf + 4 * out_offset, 6);
	EXPECT_EQ(5, rc);
	EXPECT_EQ(6, count);

	in_offset += count;
	out_offset += rc;
	count = 4;

	/* Assert source rate frames resample to destination rate frames. */
	rc = linear_resampler_resample(lr, in_buf + 4 * in_offset, &count,
				     out_buf + 4 * out_offset, 4);
	EXPECT_EQ(4, rc);
	EXPECT_EQ(4, count);

	in_offset += count;
	out_offset += rc;
	count = 90;
	rc = linear_resampler_resample(lr, in_buf + 4 * in_offset, &count,
				     out_buf + 4 * out_offset, 90);

	/* Assert linear interpotation result. */
	for (i = 0; i < 90; i++) {
		EXPECT_LE(*(int16_t *)(in_buf + 4 * i),
			  *(int16_t *)(out_buf + 4 * i));
		EXPECT_LE(*(int16_t *)(in_buf + 4 * i + 2),
			  *(int16_t *)(out_buf + 4 * i + 2));
	}
}

TEST(LinearResampler, ResampleIntegerNoSrcBuffer) {
	int rc;
	unsigned int count;
	struct linear_resampler *lr;

	memset(in_buf, 0, BUF_SIZE);
	memset(out_buf, 0, BUF_SIZE);

	/* Rate 10 -> 9 */
	lr = linear_resampler_create(2, 4, 10, 9);

	count = 0;
	rc = linear_resampler_resample(lr, in_buf, &count,
				     out_buf, BUF_SIZE);
	EXPECT_EQ(0, rc);
	EXPECT_EQ(0, count);
}

TEST(LinearResampler, ResampleIntegerNoDstBuffer) {
	int rc;
	unsigned int count;
	struct linear_resampler *lr;

	memset(in_buf, 0, BUF_SIZE);
	memset(out_buf, 0, BUF_SIZE);

	/* Rate 10 -> 9 */
	lr = linear_resampler_create(2, 4, 10, 9);

	count = BUF_SIZE;
	rc = linear_resampler_resample(lr, in_buf, &count,
				     out_buf, 0);
	EXPECT_EQ(0, rc);
	EXPECT_EQ(0, count);
}

extern "C" {

void cras_mix_add_scale_stride(int fmt, uint8_t *dst, uint8_t *src,
			 unsigned int count, unsigned int dst_stride,
			 unsigned int src_stride, float scaler)
{
	unsigned int i;

	for (i = 0; i < count; i++) {
		int32_t sum;
		sum = *(int16_t *)dst + *(int16_t *)src * scaler;
		if (sum > INT16_MAX)
			sum = INT16_MAX;
		else if (sum < INT16_MIN)
			sum = INT16_MIN;
		*(int16_t*)dst = sum;
		dst += dst_stride;
		src += src_stride;
	}
}

}  //  extern "C"

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
