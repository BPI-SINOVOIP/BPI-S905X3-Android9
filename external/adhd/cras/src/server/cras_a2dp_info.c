/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <netinet/in.h>
#include <sbc/sbc.h>
#include <syslog.h>

#include "cras_a2dp_info.h"
#include "cras_sbc_codec.h"
#include "cras_types.h"
#include "rtp.h"

int init_a2dp(struct a2dp_info *a2dp, a2dp_sbc_t *sbc)
{
	uint8_t frequency = 0, mode = 0, subbands = 0, allocation, blocks = 0,
		bitpool;

	if (sbc->frequency & SBC_SAMPLING_FREQ_48000)
		frequency = SBC_FREQ_48000;
	else if (sbc->frequency & SBC_SAMPLING_FREQ_44100)
		frequency = SBC_FREQ_44100;
	else if (sbc->frequency & SBC_SAMPLING_FREQ_32000)
		frequency = SBC_FREQ_32000;
	else if (sbc->frequency & SBC_SAMPLING_FREQ_16000)
		frequency = SBC_FREQ_16000;

	if (sbc->channel_mode & SBC_CHANNEL_MODE_JOINT_STEREO)
		mode = SBC_MODE_JOINT_STEREO;
	else if (sbc->channel_mode & SBC_CHANNEL_MODE_STEREO)
		mode = SBC_MODE_STEREO;
	else if (sbc->channel_mode & SBC_CHANNEL_MODE_DUAL_CHANNEL)
		mode = SBC_MODE_DUAL_CHANNEL;
	else if (sbc->channel_mode & SBC_CHANNEL_MODE_MONO)
		mode = SBC_MODE_MONO;

	if (sbc->allocation_method & SBC_ALLOCATION_LOUDNESS)
		allocation = SBC_AM_LOUDNESS;
	else
		allocation = SBC_AM_SNR;

	switch (sbc->subbands) {
	case SBC_SUBBANDS_4:
		subbands = SBC_SB_4;
		break;
	case SBC_SUBBANDS_8:
		subbands = SBC_SB_8;
		break;
	}

	switch (sbc->block_length) {
	case SBC_BLOCK_LENGTH_4:
		blocks = SBC_BLK_4;
		break;
	case SBC_BLOCK_LENGTH_8:
		blocks = SBC_BLK_8;
		break;
	case SBC_BLOCK_LENGTH_12:
		blocks = SBC_BLK_12;
		break;
	case SBC_BLOCK_LENGTH_16:
		blocks = SBC_BLK_16;
		break;
	}

	bitpool = sbc->max_bitpool;

	a2dp->codec = cras_sbc_codec_create(frequency, mode, subbands,
					    allocation, blocks, bitpool);
	if (!a2dp->codec)
		return -1;

	/* SBC info */
	a2dp->codesize = cras_sbc_get_codesize(a2dp->codec);
	a2dp->frame_length = cras_sbc_get_frame_length(a2dp->codec);

	a2dp->a2dp_buf_used = sizeof(struct rtp_header)
			+ sizeof(struct rtp_payload);
	a2dp->frame_count = 0;
	a2dp->seq_num = 0;
	a2dp->samples = 0;

	return 0;
}

void destroy_a2dp(struct a2dp_info *a2dp)
{
	cras_sbc_codec_destroy(a2dp->codec);
}

int a2dp_codesize(struct a2dp_info *a2dp)
{
	return a2dp->codesize;
}

int a2dp_block_size(struct a2dp_info *a2dp, int a2dp_bytes)
{
	return a2dp_bytes / a2dp->frame_length * a2dp->codesize;
}

int a2dp_queued_frames(const struct a2dp_info *a2dp)
{
	return a2dp->samples;
}

void a2dp_drain(struct a2dp_info *a2dp)
{
	a2dp->a2dp_buf_used = sizeof(struct rtp_header)
			+ sizeof(struct rtp_payload);
	a2dp->samples = 0;
	a2dp->seq_num = 0;
	a2dp->frame_count = 0;
}

static int avdtp_write(int stream_fd, struct a2dp_info *a2dp)
{
	int err, samples;
	struct rtp_header *header;
	struct rtp_payload *payload;

	header = (struct rtp_header *)a2dp->a2dp_buf;
	payload = (struct rtp_payload *)(a2dp->a2dp_buf + sizeof(*header));
	memset(a2dp->a2dp_buf, 0, sizeof(*header) + sizeof(*payload));

	payload->frame_count = a2dp->frame_count;
	header->v = 2;
	header->pt = 1;
	header->sequence_number = htons(a2dp->seq_num);
	header->timestamp = htonl(a2dp->nsamples);
	header->ssrc = htonl(1);

	err = send(stream_fd, a2dp->a2dp_buf, a2dp->a2dp_buf_used,
		   MSG_DONTWAIT);
	if (err < 0)
		return -errno;

	/* Returns the number of samples in frame. */
	samples = a2dp->samples;

	/* Reset some data */
	a2dp->a2dp_buf_used = sizeof(*header) + sizeof(*payload);
	a2dp->frame_count = 0;
	a2dp->samples = 0;
	a2dp->seq_num++;

	return samples;
}

int a2dp_encode(struct a2dp_info *a2dp, const void *pcm_buf, int pcm_buf_size,
		int format_bytes, size_t link_mtu)
{
	int processed;
	size_t out_encoded;

	if (link_mtu > A2DP_BUF_SIZE_BYTES)
		link_mtu = A2DP_BUF_SIZE_BYTES;
	if (link_mtu == a2dp->a2dp_buf_used)
		return 0;

	processed = a2dp->codec->encode(a2dp->codec, pcm_buf, pcm_buf_size,
					a2dp->a2dp_buf + a2dp->a2dp_buf_used,
					link_mtu - a2dp->a2dp_buf_used,
					&out_encoded);
	if (processed < 0) {
		syslog(LOG_ERR, "a2dp encode error %d", processed);
		return processed;
	}

	if (a2dp->codesize > 0)
		a2dp->frame_count += processed / a2dp->codesize;
	a2dp->a2dp_buf_used += out_encoded;

	a2dp->samples += processed / format_bytes;
	a2dp->nsamples += processed / format_bytes;

	return processed;
}

int a2dp_write(struct a2dp_info *a2dp, int stream_fd, size_t link_mtu)
{
	/* Do avdtp write when the max number of SBC frames is reached. */
	if (a2dp->a2dp_buf_used + a2dp->frame_length >
	    link_mtu - sizeof(struct rtp_header) - sizeof(struct rtp_payload))
		return avdtp_write(stream_fd, a2dp);

	return 0;
}
