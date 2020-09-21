/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef CRAS_AUDIO_FORMAT_H_
#define CRAS_AUDIO_FORMAT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <string.h>

#ifdef __ANDROID__
#include <hardware/audio.h>
#include <tinyalsa/asoundlib.h>
#define PCM_FORMAT_WIDTH(format) pcm_format_to_bits(format)
typedef enum pcm_format snd_pcm_format_t;

/* libasound audio formats. */
#define SND_PCM_FORMAT_UNKNOWN -1
#define SND_PCM_FORMAT_U8       1
#define SND_PCM_FORMAT_S16_LE   2
#define SND_PCM_FORMAT_S24_LE   6
#define SND_PCM_FORMAT_S32_LE  10

static inline int audio_format_to_cras_format(audio_format_t audio_format)
{
    switch (audio_format) {
    case AUDIO_FORMAT_PCM_16_BIT:
        return SND_PCM_FORMAT_S16_LE;
    case AUDIO_FORMAT_PCM_8_BIT:
        return SND_PCM_FORMAT_U8;
    case AUDIO_FORMAT_PCM_32_BIT:
        return SND_PCM_FORMAT_S32_LE;
    case AUDIO_FORMAT_PCM_8_24_BIT:
        return SND_PCM_FORMAT_S24_LE;
    default:
        return SND_PCM_FORMAT_UNKNOWN;
    }
}
#else
#include <alsa/asoundlib.h>
#define PCM_FORMAT_WIDTH(format) snd_pcm_format_physical_width(format)
#endif

/* Identifiers for each channel in audio stream. */
enum CRAS_CHANNEL {
	/* First nine channels matches the
	 * snd_mixer_selem_channel_id_t values.
	 */
	CRAS_CH_FL,
	CRAS_CH_FR,
	CRAS_CH_RL,
	CRAS_CH_RR,
	CRAS_CH_FC,
	CRAS_CH_LFE,
	CRAS_CH_SL,
	CRAS_CH_SR,
	CRAS_CH_RC,
	/* Channels defined both in channel_layout.h and
	 * alsa channel mapping API. */
	CRAS_CH_FLC,
	CRAS_CH_FRC,
	/* Must be the last one */
	CRAS_CH_MAX,
};

/* Audio format. */
struct cras_audio_format {
	snd_pcm_format_t format;
	size_t frame_rate; /* Hz */

	// TODO(hychao): use channel_layout to replace num_channels
	size_t num_channels;

	/* Channel layout whose value represents the index of each
	 * CRAS_CHANNEL in the layout. Value -1 means the channel is
	 * not used. For example: 0,1,2,3,4,5,-1,-1,-1,-1,-1 means the
	 * channel order is FL,FR,RL,RR,FC.
	 */
	int8_t channel_layout[CRAS_CH_MAX];
};

/* Packed version of audio format, for use in messages. We cannot modify
 * the above structure to keep binary compatibility with Chromium.
 * If cras_audio_format ever changes, merge the 2 structures.
 */
struct __attribute__ ((__packed__)) cras_audio_format_packed {
	int32_t format;
	uint32_t frame_rate;
	uint32_t num_channels;
	int8_t channel_layout[CRAS_CH_MAX];
};

static inline void pack_cras_audio_format(struct cras_audio_format_packed* dest,
					  const struct cras_audio_format* src)
{
	dest->format = src->format;
	dest->frame_rate = src->frame_rate;
	dest->num_channels = src->num_channels;
	memcpy(dest->channel_layout, src->channel_layout,
		sizeof(src->channel_layout));
}

static inline void unpack_cras_audio_format(struct cras_audio_format* dest,
				     const struct cras_audio_format_packed* src)
{
	dest->format = (snd_pcm_format_t)src->format;
	dest->frame_rate = src->frame_rate;
	dest->num_channels = src->num_channels;
	memcpy(dest->channel_layout, src->channel_layout,
		sizeof(src->channel_layout));
}

/* Returns the number of bytes per sample.
 * This is bits per smaple / 8 * num_channels.
 */
static inline size_t cras_get_format_bytes(const struct cras_audio_format *fmt)
{
	const int bytes = PCM_FORMAT_WIDTH(fmt->format) / 8;
	return (size_t)bytes * fmt->num_channels;
}

/* Create an audio format structure. */
struct cras_audio_format *cras_audio_format_create(snd_pcm_format_t format,
						   size_t frame_rate,
						   size_t num_channels);

/* Destroy an audio format struct created with cras_audio_format_crate. */
void cras_audio_format_destroy(struct cras_audio_format *fmt);

/* Sets the channel layout for given format.
 *    format - The format structure to carry channel layout info
 *    layout - An integer array representing the position of each
 *        channel in enum CRAS_CHANNEL
 */
int cras_audio_format_set_channel_layout(struct cras_audio_format *format,
					 const int8_t layout[CRAS_CH_MAX]);

/* Allocates an empty channel conversion matrix of given size. */
float** cras_channel_conv_matrix_alloc(size_t in_ch, size_t out_ch);

/* Destroys the channel conversion matrix. */
void cras_channel_conv_matrix_destroy(float **mtx, size_t out_ch);

/* Creates channel conversion matrix for given input and output format.
 * Returns NULL if the conversion is not supported between the channel
 * layouts specified in input/ouput formats.
 */
float **cras_channel_conv_matrix_create(const struct cras_audio_format *in,
					const struct cras_audio_format *out);

#ifdef __cplusplus
}
#endif

#endif /* CRAS_AUDIO_FORMAT_H_ */
