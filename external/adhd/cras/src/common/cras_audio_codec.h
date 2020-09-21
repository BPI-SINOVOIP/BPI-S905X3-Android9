/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef COMMON_CRAS_AUDIO_CODEC_H_
#define COMMON_CRAS_AUDIO_CODEC_H_

/* A audio codec that transforms audio between different formats.
 * decode - Function to decode audio samples. Returns the number of decoded
 *   bytes of input buffer, number of decoded bytes of output buffer
 *   will be filled in count.
 * encode - Function to encode audio samples. Returns the number of encoded
 *   bytes of input buffer, number of encoded bytes of output buffer
 *   will be filled in count.
 * priv_data - Private data for specific use.
 */
struct cras_audio_codec {
	int (*decode)(struct cras_audio_codec *codec, const void *input,
		      size_t input_len, void *output, size_t output_len,
		      size_t *count);
	int (*encode)(struct cras_audio_codec *codec, const void *input,
		      size_t intput_len, void *output, size_t output_len,
		      size_t *count);
	void *priv_data;
};

#endif /* COMMON_CRAS_AUDIO_CODEC_H_ */
