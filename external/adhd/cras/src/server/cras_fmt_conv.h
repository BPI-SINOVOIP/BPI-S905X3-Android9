/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Used to convert from one audio format to another.  Currently only supports
 * sample rate conversion with the speex backend.
 */
#ifndef CRAS_FMT_CONV_H_
#define CRAS_FMT_CONV_H_

#include <stdint.h>
#include <stdlib.h>

#include "cras_types.h"

struct cras_audio_format;
struct cras_fmt_conv;

/* Create and destroy format converters. */
struct cras_fmt_conv *cras_fmt_conv_create(const struct cras_audio_format *in,
					   const struct cras_audio_format *out,
					   size_t max_frames,
					   size_t pre_linear_resample);
void cras_fmt_conv_destroy(struct cras_fmt_conv *conv);

/* Creates the format converter for channel remixing. The conversion takes
 * a N by N float matrix, to multiply each N-channels sample.
 * Args:
 *    num_channels - Number of channels of PCM data.
 *    coefficient - Float array of length N * N representing the conversion
 *        matrix, where matrix[i][j] corresponds to coefficient[i * N + j]
 */
struct cras_fmt_conv *cras_channel_remix_conv_create(
		unsigned int num_channels,
		const float *coefficient);

/* Converts nframes of sample from in_buf, using given remix converter.
 * Args:
 *    conv - The format converter.
 *    fmt - The format of the buffer to convert.
 *    in_buf - The buffer to convert.
 *    nframes - The number of frames to convert.
 */
void cras_channel_remix_convert(struct cras_fmt_conv *conv,
				const struct cras_audio_format *fmt,
				uint8_t *in_buf,
				size_t nframes);

/* Get the input format of the converter. */
const struct cras_audio_format *cras_fmt_conv_in_format(
		const struct cras_fmt_conv *conv);

/* Get the output format of the converter. */
const struct cras_audio_format *cras_fmt_conv_out_format(
		const struct cras_fmt_conv *conv);

/* Get the number of output frames that will result from converting in_frames */
size_t cras_fmt_conv_in_frames_to_out(struct cras_fmt_conv *conv,
				      size_t in_frames);
/* Get the number of input frames that will result from converting out_frames */
size_t cras_fmt_conv_out_frames_to_in(struct cras_fmt_conv *conv,
				      size_t out_frames);
/* Sets the input and output rate to the linear resampler. */
void cras_fmt_conv_set_linear_resample_rates(struct cras_fmt_conv *conv,
					     float from,
					     float to);
/* Converts in_frames samples from in_buf, storing the results in out_buf.
 * Args:
 *    conv - The format converter returned from cras_fmt_conv_create().
 *    in_buf - Samples to convert.
 *    out_buf - Converted samples are placed here.
 *    in_frames - Number of frames from in_buf to convert.
 *    out_frames - Maximum number of frames to store in out_buf.  If there isn't
 *      any format conversion, out_frames must be >= in_frames.  When doing
 *      format conversion out_frames should be able to hold all the converted
 *      frames, this can be checked with cras_fmt_conv_in_frames_to_out().
 * Return number of frames put in out_buf. */
size_t cras_fmt_conv_convert_frames(struct cras_fmt_conv *conv,
				    const uint8_t *in_buf,
				    uint8_t *out_buf,
				    unsigned int *in_frames,
				    size_t out_frames);

/* Checks if format conversion is needed for a fmt converter.
 * Args:
 *    conv - The format convert to check.
 *  Returns:
 *    Non-zero if a format conversion is needed.
 */
int cras_fmt_conversion_needed(const struct cras_fmt_conv *conv);

/* If the server cannot provide the requested format, configures an audio format
 * converter that handles transforming the input format to the format used by
 * the server.
 * Args:
 *    conv - filled with the new converter if needed.
 *    dir - the stream direction the new converter used for.
 *    from - Format to convert from.
 *    to - Format to convert to.
 *    frames - size of buffer.
 */
int config_format_converter(struct cras_fmt_conv **conv,
			    enum CRAS_STREAM_DIRECTION dir,
			    const struct cras_audio_format *from,
			    const struct cras_audio_format *to,
			    unsigned int frames);

#endif /* CRAS_FMT_CONV_H_ */
