/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef CRAS_AUDIO_AREA_H_
#define CRAS_AUDIO_AREA_H_

#include <stdint.h>

#include "cras_audio_format.h"

/*
 * Descriptor of the memory area holding a channel of audio.
 * Members:
 *    ch_set - Bit set of channels this channel area could map to.
 *    step_bytes - The number of bytes between adjacent samples.
 *    buf - A pointer to the start address of this area.
 */
struct cras_channel_area {
	unsigned int ch_set;
	unsigned int step_bytes;
	uint8_t *buf;
};

/*
 * Descriptor of the memory area that provides various access to audio channels.
 * Members:
 *    frames - The size of the audio buffer in frames.
 *    num_channels - The number of channels in the audio area.
 *    channels - array of channel areas.
 */
struct cras_audio_area {
	unsigned int frames;
	unsigned int num_channels;
	struct cras_channel_area channels[];
};

/*
 * Sets the channel bit for a channel area.
 * Args:
 *    ca - the channel area to set channel bit set.
 *    channel - the channel bit to add to the channel area.
 */
static
inline void channel_area_set_channel(struct cras_channel_area *ca,
				     enum CRAS_CHANNEL channel)
{
	ca->ch_set |= (1 << channel);
}

/*
 * Creates a cras_audio_area.
 * Args:
 *    num_channels - The number of channels in the audio area.
*/
struct cras_audio_area *cras_audio_area_create(int num_channels);

/*
 * Copies a cras_audio_area to another cras_audio_area with given offset.
 * Args:
 *    dst - The destination audio area.
 *    dst_offset - The offset of dst audio area in frames.
 *    format - The format of dst area.
 *    src - The source audio area.
 *    src_offset - The offset of src audio area in frames.
 *    software_gain_scaler - The software gain scaler needed.
 * Returns the number of frames copied.
 */
unsigned int cras_audio_area_copy(const struct cras_audio_area *dst,
				  unsigned int dst_offset,
				  const struct cras_audio_format *dst_fmt,
				  const struct cras_audio_area *src,
				  unsigned int src_offset,
				  float software_gain_scaler);

/*
 * Destroys a cras_audio_area.
 * Args:
 *    area - the audio area to destroy
 */
void cras_audio_area_destroy(struct cras_audio_area *area);

/*
 * Configures the channel types based on the audio format.
 * Args:
 *    area - The audio area created with cras_audio_area_create.
 *    fmt - The format to use to configure the channels.
 */
void cras_audio_area_config_channels(struct cras_audio_area *area,
				     const struct cras_audio_format *fmt);

/*
 * Sets the buffer pointers for each channel.
 */
void cras_audio_area_config_buf_pointers(struct cras_audio_area *area,
					 const struct cras_audio_format *fmt,
					 uint8_t *base_buffer);

#endif /* CRAS_AUDIO_AREA_H_ */
