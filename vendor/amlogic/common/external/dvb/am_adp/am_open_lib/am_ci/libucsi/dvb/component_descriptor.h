/*
 * section and descriptor parser
 *
 * Copyright (C) 2005 Kenneth Aafloy (kenneth@linuxtv.org)
 * Copyright (C) 2005 Andrew de Quincey (adq_dvb@lidskialf.net)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef _UCSI_DVB_COMPONENT_DESCRIPTOR
#define _UCSI_DVB_COMPONENT_DESCRIPTOR 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/descriptor.h>
#include <libucsi/endianops.h>
#include <libucsi/types.h>

/**
 * Possible values for stream_content.
 */
enum {
	DVB_STREAM_CONTENT_VIDEO 		= 0x01,
	DVB_STREAM_CONTENT_AUDIO 		= 0x02,
	DVB_STREAM_CONTENT_SUBTITLE 		= 0x03,
	DVB_STREAM_CONTENT_AC3	 		= 0x04,
};

/**
 * Possible values for component_type.
 */
enum {
	DVB_COMPONENT_TYPE_VIDEO_43_25Hz		= 0x01,
	DVB_COMPONENT_TYPE_VIDEO_169_PAN_25Hz		= 0x02,
	DVB_COMPONENT_TYPE_VIDEO_169_NOPAN_25Hz		= 0x03,
	DVB_COMPONENT_TYPE_VIDEO_GT169_25Hz		= 0x04,

	DVB_COMPONENT_TYPE_VIDEO_43_30Hz		= 0x05,
	DVB_COMPONENT_TYPE_VIDEO_169_PAN_30Hz		= 0x06,
	DVB_COMPONENT_TYPE_VIDEO_169_NOPAN_30Hz		= 0x07,
	DVB_COMPONENT_TYPE_VIDEO_GT169_30Hz		= 0x08,

	DVB_COMPONENT_TYPE_HDVIDEO_43_25Hz		= 0x09,
	DVB_COMPONENT_TYPE_HDVIDEO_169_PAN_25Hz		= 0x0a,
	DVB_COMPONENT_TYPE_HDVIDEO_169_NOPAN_25Hz	= 0x0b,
	DVB_COMPONENT_TYPE_HDVIDEO_GT169_25Hz		= 0x0c,

	DVB_COMPONENT_TYPE_HDVIDEO_43_30Hz		= 0x0d,
	DVB_COMPONENT_TYPE_HDVIDEO_169_PAN_30Hz		= 0x0e,
	DVB_COMPONENT_TYPE_HDVIDEO_169_NOPAN_30Hz	= 0x0f,
	DVB_COMPONENT_TYPE_HDVIDEO_GT169_30Hz		= 0x10,

	DVB_COMPONENT_TYPE_AUDIO_SINGLE_MONO		= 0x01,
	DVB_COMPONENT_TYPE_AUDIO_DUAL_MONO		= 0x02,
	DVB_COMPONENT_TYPE_AUDIO_STEREO			= 0x03,
	DVB_COMPONENT_TYPE_AUDIO_MULTI_LINGUAL_MULTI_CHAN= 0x04,
	DVB_COMPONENT_TYPE_AUDIO_SURROUND		= 0x05,
	DVB_COMPONENT_TYPE_AUDIO_VISUAL_IMPAIRED	= 0x40,
	DVB_COMPONENT_TYPE_AUDIO_HARDHEAR		= 0x41,
	DVB_COMPONENT_TYPE_AUDIO_SUPPLEMENTARY		= 0x42,

	DVB_COMPONENT_TYPE_SUBTITLE_TELETEXT		= 0x01,
	DVB_COMPONENT_TYPE_SUBTITLE_ASSOC_TELETEXT	= 0x02,
	DVB_COMPONENT_TYPE_SUBTITLE_VBI			= 0x03,
	DVB_COMPONENT_TYPE_SUBTITLE_DVB			= 0x10,
	DVB_COMPONENT_TYPE_SUBTITLE_DVB_43		= 0x11,
	DVB_COMPONENT_TYPE_SUBTITLE_DVB_169		= 0x12,
	DVB_COMPONENT_TYPE_SUBTITLE_DVB_2211		= 0x13,
	DVB_COMPONENT_TYPE_SUBTITLE_DVB_HARDHEAR	= 0x20,
	DVB_COMPONENT_TYPE_SUBTITLE_DVB_HARDHEAR_43	= 0x21,
	DVB_COMPONENT_TYPE_SUBTITLE_DVB_HARDHEAR_169	= 0x22,
	DVB_COMPONENT_TYPE_SUBTITLE_DVB_HARDHEAR_2211	= 0x23,
};

/**
 * dvb_component_descriptor structure.
 */
struct dvb_component_descriptor {
	struct descriptor d;

  EBIT2(uint8_t reserved	: 4; ,
	uint8_t stream_content	: 4; );
	uint8_t component_type;
	uint8_t component_tag;
	iso639lang_t language_code;
	/* uint8_t text[] */
} __ucsi_packed;

/**
 * Process a dvb_component_descriptor.
 *
 * @param d Pointer to a generic descriptor.
 * @return dvb_component_descriptor pointer, or NULL on error.
 */
static inline struct dvb_component_descriptor*
	dvb_component_descriptor_codec(struct descriptor* d)
{
	if (d->len < (sizeof(struct dvb_component_descriptor) - 2))
		return NULL;

	return (struct dvb_component_descriptor*) d;
}

/**
 * Accessor for the text field of a dvb_component_descriptor.
 *
 * @param d dvb_component_descriptor pointer.
 * @return Pointer to the field.
 */
static inline uint8_t *
	dvb_component_descriptor_text(struct dvb_component_descriptor *d)
{
	return (uint8_t *) d + sizeof(struct dvb_component_descriptor);
}

/**
 * Determine the length of the text field of a dvb_component_descriptor.
 *
 * @param d dvb_component_descriptor pointer.
 * @return Length of the field in bytes.
 */
static inline int
	dvb_component_descriptor_text_length(struct dvb_component_descriptor *d)
{
	return d->d.len - 6;
}

#ifdef __cplusplus
}
#endif

#endif
