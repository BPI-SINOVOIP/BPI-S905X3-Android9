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

#ifndef _UCSI_DVB_SECTION_H
#define _UCSI_DVB_SECTION_H 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libucsi/dvb/bat_section.h>
#include <libucsi/dvb/dit_section.h>
#include <libucsi/dvb/eit_section.h>
#include <libucsi/dvb/nit_section.h>
#include <libucsi/dvb/rst_section.h>
#include <libucsi/dvb/sdt_section.h>
#include <libucsi/dvb/sit_section.h>
#include <libucsi/dvb/st_section.h>
#include <libucsi/dvb/tdt_section.h>
#include <libucsi/dvb/tot_section.h>
#include <libucsi/dvb/tva_container_section.h>
#include <libucsi/dvb/int_section.h>
#include <libucsi/dvb/mpe_fec_section.h>

/**
 * The following are not implemented just now.
 */
/*
#include <libucsi/dvb/tva_related_content_section.h>
#include <libucsi/dvb/tva_content_identifier_section.h>
#include <libucsi/dvb/tva_resolution_provider_notification_section.h>
#include <libucsi/dvb/ait_section.h>
#include <libucsi/dvb/cit_section.h>
#include <libucsi/dvb/rct_section.h>
#include <libucsi/dvb/rnt_section.h>
*/

#define TRANSPORT_NIT_PID 0x10
#define TRANSPORT_SDT_PID 0x11
#define TRANSPORT_BAT_PID 0x11
#define TRANSPORT_EIT_PID 0x12
#define TRANSPORT_CIT_PID 0x12
#define TRANSPORT_RST_PID 0x13
#define TRANSPORT_TDT_PID 0x14
#define TRANSPORT_TOT_PID 0x14
#define TRANSPORT_RNT_PID 0x16
#define TRANSPORT_DIT_PID 0x1e
#define TRANSPORT_SIT_PID 0x1f

/**
 * Enumeration of DVB section tags.
 */
enum dvb_section_tag {
	stag_dvb_network_information_actual			= 0x40,
	stag_dvb_network_information_other			= 0x41,

	stag_dvb_service_description_actual			= 0x42,
	stag_dvb_service_description_other			= 0x46,

	stag_dvb_bouquet_association				= 0x4a,
	stag_dvb_update_notification				= 0x4b, /* same syntax as IP_MAC */
	stag_dvb_ip_mac_notification				= 0x4c,

	stag_dvb_event_information_nownext_actual		= 0x4e,
	stag_dvb_event_information_nownext_other		= 0x4f,
	stag_dvb_event_information_schedule_actual		= 0x50, /* 0x50->0x5f */
	stag_dvb_event_information_schedule_other		= 0x60, /* 0x60->0x6f */

	stag_dvb_time_date					= 0x70,
	stag_dvb_running_status					= 0x71,
	stag_dvb_stuffing					= 0x72,
	stag_dvb_time_offset					= 0x73,
	stag_dvb_application_information			= 0x74,
	stag_dvb_tva_container					= 0x75,
	stag_dvb_tva_related_content				= 0x76,
	stag_dvb_tva_content_identifier				= 0x77,
	stag_dvb_mpe_fec					= 0x78,
	stag_dvb_tva_resolution_provider_notification		= 0x79,

	stag_dvb_discontinuity_information			= 0x7e,
	stag_dvb_selection_information				= 0x7f,

};

#ifdef __cplusplus
}
#endif

#endif
