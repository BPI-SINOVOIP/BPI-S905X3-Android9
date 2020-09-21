/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef LINEAR_RESAMPLER_H_
#define LINEAR_RESAMPLER_H_


struct linear_resampler;

/* Creates a linear resampler.
 * Args:
 *    num_channels - The number of channels in each frames.
 *    format_bytes - The length of one frame in bytes.
 *    src_rate - The source rate to resample from.
 *    dst_rate - The destination rate to resample to.
 */
struct linear_resampler *linear_resampler_create(unsigned int num_channels,
						 unsigned int format_bytes,
						 float src_rate,
						 float dst_rate);

/* Sets the rates for the linear resampler.
 * Args:
 *    from - The rate to resample from.
 *    to - The rate to resample to.
 */
void linear_resampler_set_rates(struct linear_resampler *lr,
				float from,
				float to);

/* Converts the frames count from output rate to input rate. */
unsigned int linear_resampler_out_frames_to_in(struct linear_resampler *lr,
                                               unsigned int frames);

/* Converts the frames count from input rate to output rate. */
unsigned int linear_resampler_in_frames_to_out(struct linear_resampler *lr,
					       unsigned int frames);

/* Returns true if SRC is needed, otherwise return false. */
int linear_resampler_needed(struct linear_resampler *lr);

/* Run linear resample for audio samples.
 * Args:
 *    lr - The linear resampler.
 *    src - The input buffer.
 *    src_frames - The number of frames of input buffer.
 *    dst - The output buffer.
 *    dst_frames - The number of frames of output buffer.
 */
unsigned int linear_resampler_resample(struct linear_resampler *lr,
			     uint8_t *src,
			     unsigned int *src_frames,
			     uint8_t *dst,
			     unsigned dst_frames);

/* Destroy a linear resampler. */
void linear_resampler_destroy(struct linear_resampler *lr);

#endif /* LINEAR_RESAMPLER_H_ */
