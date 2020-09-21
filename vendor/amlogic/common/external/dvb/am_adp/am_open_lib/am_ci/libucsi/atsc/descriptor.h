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

#ifndef _UCSI_ATSC_DESCRIPTOR_H
#define _UCSI_ATSC_DESCRIPTOR_H 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/endianops.h>
#include <libucsi/atsc/stuffing_descriptor.h>
#include <libucsi/atsc/ac3_descriptor.h>
#include <libucsi/atsc/caption_service_descriptor.h>
#include <libucsi/atsc/component_name_descriptor.h>
#include <libucsi/atsc/content_advisory_descriptor.h>
#include <libucsi/atsc/dcc_arriving_request_descriptor.h>
#include <libucsi/atsc/dcc_departing_request_descriptor.h>
#include <libucsi/atsc/extended_channel_name_descriptor.h>
#include <libucsi/atsc/genre_descriptor.h>
#include <libucsi/atsc/rc_descriptor.h>
#include <libucsi/atsc/service_location_descriptor.h>
#include <libucsi/atsc/time_shifted_service_descriptor.h>

/**
 * Enumeration of ATSC descriptor tags.
 */
enum atsc_descriptor_tag {
	dtag_atsc_stuffing			= 0x80,
	dtag_atsc_ac3_audio			= 0x81,
	dtag_atsc_caption_service		= 0x86,
	dtag_atsc_content_advisory		= 0x87,
	dtag_atsc_extended_channel_name		= 0xa0,
	dtag_atsc_service_location		= 0xa1,
	dtag_atsc_time_shifted_service		= 0xa2,
	dtag_atsc_component_name		= 0xa3,
	dtag_atsc_dcc_departing_request		= 0xa8,
	dtag_atsc_dcc_arriving_request		= 0xa9,
	dtag_atsc_redistribution_control	= 0xaa,
	dtag_atsc_private_information		= 0xad,
	dtag_atsc_content_identifier		= 0xb6,
	dtag_atsc_genre				= 0xab,
};

#ifdef __cplusplus
}
#endif

#endif
