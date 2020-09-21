/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <sbc/sbc.h>
#include <stdlib.h>

#include "cras_sbc_codec.h"

/* SBC library encodes one PCM input block to one SBC output block. This
 * structure holds related info about the SBC codec.
 * Members:
 *    sbc - The main structure for SBC codec.
 *    codesize - The size of one PCM input block in bytes.
 *    frame_length - The size of one SBC output block in bytes.
 */
struct cras_sbc_data {
	sbc_t sbc;
	unsigned int codesize;
	unsigned int frame_length;
};

int cras_sbc_decode(struct cras_audio_codec *codec, const void *input,
		    size_t input_len, void *output, size_t output_len,
		    size_t *count) {
	struct cras_sbc_data *data = (struct cras_sbc_data *)codec->priv_data;
	size_t written;
	ssize_t decoded;
	int processed = 0;
	int result = 0;

	/* Proceed decode when there is buffer left in input and room in
	 * output.
	 */
	while (input_len > processed && output_len > result) {
		decoded = sbc_decode(&data->sbc,
				     input + processed,
				     input_len - processed,
				     output + result,
				     output_len - result,
				     &written);
		if (decoded <= 0)
			break;

		processed += decoded;
		result += written;
	}
	*count = result;
	return processed;
}

int cras_sbc_encode(struct cras_audio_codec *codec, const void *input,
		    size_t input_len, void *output, size_t output_len,
		    size_t *count) {
	struct cras_sbc_data *data = (struct cras_sbc_data *)codec->priv_data;
	ssize_t written, encoded;
	int processed = 0, result = 0;

	/* Proceed encode when input buffer has at least one input block and
	 * there is still room in output buffer.
	 */
	while (input_len - processed >= data->codesize &&
			output_len >= result) {
		encoded = sbc_encode(&data->sbc,
				     input + processed,
				     data->codesize,
				     output + result,
				     output_len - result,
				     &written);
		if (encoded == -ENOSPC)
			break;
		else if (encoded < 0)
			return encoded;

		processed += encoded;
		result += written;
	}
	*count = result;
	return processed;
}

int cras_sbc_get_codesize(struct cras_audio_codec *codec)
{
	struct cras_sbc_data *data = (struct cras_sbc_data *)codec->priv_data;
	return data->codesize;
}

int cras_sbc_get_frame_length(struct cras_audio_codec *codec)
{
	struct cras_sbc_data *data = (struct cras_sbc_data *)codec->priv_data;
	return data->frame_length;
}

struct cras_audio_codec *cras_sbc_codec_create(uint8_t freq,
		   uint8_t mode, uint8_t subbands, uint8_t alloc,
		   uint8_t blocks, uint8_t bitpool) {
	struct cras_audio_codec *codec;
	struct cras_sbc_data *data;

	codec = (struct cras_audio_codec *)calloc(1, sizeof(*codec));
	if (!codec)
		return NULL;

	codec->priv_data = (struct cras_sbc_data *)calloc(1,
			sizeof(struct cras_sbc_data));
	if (!codec->priv_data)
		goto create_error;

	data = (struct cras_sbc_data *)codec->priv_data;
	sbc_init(&data->sbc, 0L);
	data->sbc.endian = SBC_LE;
	data->sbc.frequency = freq;
	data->sbc.mode = mode;
	data->sbc.subbands = subbands;
	data->sbc.allocation = alloc;
	data->sbc.blocks = blocks;
	data->sbc.bitpool = bitpool;
	data->codesize = sbc_get_codesize(&data->sbc);
	data->frame_length = sbc_get_frame_length(&data->sbc);

	codec->decode = cras_sbc_decode;
	codec->encode = cras_sbc_encode;
	return codec;

create_error:
	free(codec);
	return NULL;
}

void cras_sbc_codec_destroy(struct cras_audio_codec *codec)
{
	sbc_finish(&((struct cras_sbc_data *)codec->priv_data)->sbc);
	free(codec->priv_data);
	free(codec);
}
