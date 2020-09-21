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

#ifndef _UCSI_DVB_TDT_SECTION_H
#define _UCSI_DVB_TDT_SECTION_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/section.h>
#include <libucsi/dvb/types.h>

/**
 * dvb_tdt_section structure.
 */
struct dvb_tdt_section {
	struct section head;

	dvbdate_t utc_time;
} __ucsi_packed;

/**
 * Process a dvb_tdt_section.
 *
 * @param section Generic section header.
 * @return dvb_tdt_section pointer, or NULL on error.
 */
struct dvb_tdt_section *dvb_tdt_section_codec(struct section *section);

#ifdef __cplusplus
}
#endif

#endif
