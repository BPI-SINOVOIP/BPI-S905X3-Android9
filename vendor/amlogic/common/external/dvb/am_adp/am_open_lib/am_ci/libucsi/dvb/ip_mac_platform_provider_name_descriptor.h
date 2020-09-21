/*
 * section and descriptor parser
 *
 * Copyright (C) 2005 Kenneth Aafloy (kenneth@linuxtv.org)
 * Copyright (C) 2005 Andrew de Quincey (adq_dvb@lidskialf.net)
 * Copyright (C) 2006 Stephane Este-Gracias (sestegra@free.fr)
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

#ifndef _UCSI_DVB_IP_MAC_PLATFORM_PROVIDER_NAME_DESCRIPTOR
#define _UCSI_DVB_IP_MAC_PLATFORM_PROVIDER_NAME_DESCRIPTOR 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/descriptor.h>
#include <libucsi/types.h>

/**
 * dvb_ip_platform_provider_name_descriptor structure.
 */
struct dvb_ip_platform_provider_name_descriptor {
	struct descriptor d;

	iso639lang_t language_code;
	/* uint8_t text[] */
} __ucsi_packed;

/**
 * Process a dvb_ip_platform_provider_name_descriptor.
 *
 * @param d Pointer to a generic descriptor.
 * @return dvb_ip_platform_provider_name_descriptor pointer, or NULL on error.
 */
static inline struct dvb_ip_platform_provider_name_descriptor*
	dvb_ip_platform_provider_name_descriptor_codec(struct descriptor* d)
{
	if (d->len < (sizeof(struct dvb_ip_platform_provider_name_descriptor) - 2))
		return NULL;

	return (struct dvb_ip_platform_provider_name_descriptor*) d;
}

/**
 * Accessor for the text field of a dvb_ip_platform_provider_name_descriptor.
 *
 * @param d dvb_ip_platform_provider_name_descriptor pointer.
 * @return Pointer to the field.
 */
static inline uint8_t *
	dvb_ip_platform_provider_name_descriptor_text(struct dvb_ip_platform_provider_name_descriptor *d)
{
	return (uint8_t *) d + sizeof(struct dvb_ip_platform_provider_name_descriptor);
}

/**
 * Determine the length of the text field of a dvb_ip_platform_provider_name_descriptor.
 *
 * @param d dvb_ip_platform_provider_name_descriptor pointer.
 * @return Length of the field in bytes.
 */
static inline int
	dvb_ip_platform_provider_name_descriptor_text_length(struct dvb_ip_platform_provider_name_descriptor *d)
{
	return d->d.len - 3;
}

#ifdef __cplusplus
}
#endif

#endif
