/*
 *  libzvbi -- Video Programming System
 *
 *  Copyright (C) 2000-2004 Michael H. Schimek
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the 
 *  Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 *  Boston, MA  02110-1301  USA.
 */

/* $Id: vps.c,v 1.7 2008/02/19 00:35:22 mschimek Exp $ */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <assert.h>

#include "misc.h"
#include "vps.h"

/**
 * @addtogroup VPS Video Program System Decoder
 * @ingroup LowDec
 * @brief Functions to decode and encode VPS packets (EN 300 231, EN 300 468).
 */

/**
 * @param cni CNI of type VBI_CNI_TYPE_VPS will be stored here.
 * @param buffer VPS packet as defined for @c VBI_SLICED_VPS,
 *   i.e. 13 bytes without clock run-in and start code.
 *
 * Decodes a VPS packet according to EN 300 231, returning the
 * 12 bit Country and Network Identifier in @a cni.
 *
 * The code 0xDC3 is translated according to TR 101 231: "As this
 * code is used for a time in two networks a distinction for automatic
 * tuning systems is given in data line 16 [VPS]: bit 3 of byte 5 = 1
 * for the ARD network / = 0 for the ZDF network."
 *
 * @returns
 * Always @c TRUE, no error checking possible. It may be prudent to
 * wait until two identical CNIs have been received.
 *
 * @since 0.2.20
 */
vbi_bool
vbi_decode_vps_cni		(unsigned int *		cni,
				 const uint8_t		buffer[13])
{
	unsigned int cni_value;

	assert (NULL != cni);
	assert (NULL != buffer);

	cni_value = (+ ((buffer[10] & 0x03) << 10)
		     + ((buffer[11] & 0xC0) << 2)
		     +  (buffer[ 8] & 0xC0)
		     +  (buffer[11] & 0x3F));

	if (unlikely (0x0DC3 == cni_value))
		cni_value = (buffer[2] & 0x10) ?
			0x0DC2 /* ZDF */ : 0x0DC1 /* ARD */;

	*cni = cni_value;

	return TRUE;
}

#if defined ZAPPING8 || 3 == VBI_VERSION_MINOR

/**
 * @param pid PDC data will be stored here.
 * @param buffer VPS packet as defined for @c VBI_SLICED_VPS,
 *   i.e. 13 bytes without clock run-in and start code.
 * 
 * Decodes a VPS datagram according to EN 300 231,
 * storing PDC recording-control data in @a pid.
 *
 * @returns
 * @c FALSE if the buffer contains incorrect data. In this case
 * @a pid remains unmodified.
 */
vbi_bool
vbi_decode_vps_pdc		(vbi_program_id *	pid,
				 const uint8_t		buffer[13])
{
	vbi_pil pil;

	assert (NULL != pid);
	assert (NULL != buffer);

	pil = (+ ((buffer[ 8] & 0x3F) << 14)
	       +  (buffer[ 9]         << 6)
	       +  (buffer[10]         >> 2));

	switch (pil) {
	case VBI_PIL_TIMER_CONTROL:
	case VBI_PIL_INHIBIT_TERMINATE:
	case VBI_PIL_INTERRUPT:
	case VBI_PIL_CONTINUE:
		break;

	default:
		if (unlikely ((unsigned int) VBI_PIL_MONTH (pil) - 1 > 11
			      || (unsigned int) VBI_PIL_DAY (pil) - 1 > 30
			      || (unsigned int) VBI_PIL_HOUR (pil) > 23
			      || (unsigned int) VBI_PIL_MINUTE (pil) > 59))
			return FALSE;
		break;
	}

	CLEAR (*pid);

	pid->channel	= VBI_PID_CHANNEL_VPS;

	pid->cni_type	= VBI_CNI_TYPE_VPS;

	vbi_decode_vps_cni (&pid->cni, buffer);

	pid->pil	= pil;

	pid->month	= VBI_PIL_MONTH (pil);
	pid->day	= VBI_PIL_DAY (pil);
	pid->hour	= VBI_PIL_HOUR (pil); 
	pid->minute	= VBI_PIL_MINUTE (pil);

	pid->pcs_audio	= buffer[2] >> 6;

	pid->pty	= buffer[12];

	return TRUE;
}

/**
 * @param pid PDC data will be stored here.
 * @param buffer A DVB PDC descriptor as defined in EN 300 468,
 *   including descriptor_tag and descriptor_length.
 * 
 * Decodes a DVB PDC descriptor as defined in EN 300 468 and EN 300 231,
 * storing PDC recording-control data in @a pid.
 *
 * @returns
 * @c FALSE if the buffer contains an incorrect descriptor_tag,
 * descriptor_length or PIL. In this case @a pid remains unmodified.
 */
vbi_bool
vbi_decode_dvb_pdc_descriptor	(vbi_program_id *	pid,
				 const uint8_t		buffer[5])
{
	vbi_pil pil;

	assert (NULL != pid);
	assert (NULL != buffer);

	/* descriptor_tag [8],
	   descriptor_length [8],
	   reserved_future_use [4],
	   programme_identification_label [20] ->
	     day [5], month [4], hour [5], minute [6] */

	/* EN 300 468 section 6.1, 6.2. */
	if (unlikely (0x69 != buffer[0] || 3 != buffer[1]))
		return FALSE;

	/* EN 300 468 section 6.2.29. */
	pil = (+ ((buffer[2] & 0x0F) << 16)
	       +  (buffer[3]         << 8)
	       +   buffer[4]);

	switch (pil) {
	case VBI_PIL_TIMER_CONTROL:
	case VBI_PIL_INHIBIT_TERMINATE:
	case VBI_PIL_INTERRUPT:
	case VBI_PIL_CONTINUE:
		break;

	default:
		if (unlikely ((unsigned int) VBI_PIL_MONTH (pil) - 1 > 11
			      || (unsigned int) VBI_PIL_DAY (pil) - 1 > 30
			      || (unsigned int) VBI_PIL_HOUR (pil) > 23
			      || (unsigned int) VBI_PIL_MINUTE (pil) > 59))
			return FALSE;
		break;
	}

	CLEAR (*pid);

	pid->channel	= VBI_PID_CHANNEL_PDC_DESCRIPTOR;

	pid->pil	= pil;

	pid->month	= VBI_PIL_MONTH (pil);
	pid->day	= VBI_PIL_DAY (pil);
	pid->hour	= VBI_PIL_HOUR (pil); 
	pid->minute	= VBI_PIL_MINUTE (pil);

	return TRUE;
}

#endif /* 3 == VBI_VERSION_MINOR */

/**
 * @param buffer VPS packet as defined for @c VBI_SLICED_VPS,
 *   i.e. 13 bytes without clock run-in and start code.
 * @param cni CNI of type VBI_CNI_TYPE_VPS.
 *
 * Stores the 12 bit Country and Network Identifier @a cni in
 * a VPS packet according to EN 300 231.
 *
 * @returns
 * @c FALSE if @a cni is invalid. In this case @a buffer remains
 * unmodified.
 *
 * @since 0.2.20
 */
vbi_bool
vbi_encode_vps_cni		(uint8_t		buffer[13],
				 unsigned int		cni)
{
	assert (NULL != buffer);

	if (unlikely (cni > 0x0FFF))
		return FALSE;

	buffer[8] = (buffer[8] & 0x3F) | (cni & 0xC0);
	buffer[10] = (buffer[10] & 0xFC) | (cni >> 10);
	buffer[11] = (cni & 0x3F) | ((cni >> 2) & 0xC0);

	return TRUE;
}

#if defined ZAPPING8 || 3 == VBI_VERSION_MINOR

/**
 * @param buffer VPS packet as defined for @c VBI_SLICED_VPS,
 *   i.e. 13 bytes without clock run-in and start code.
 * @param pid PDC data to encode.
 * 
 * Stores PDC recording-control data (CNI, PIL, PCS audio, PTY) in
 * a VPS datagram according to EN 300 231. If non-zero the function
 * encodes @a pid->pil, otherwise it calculates the PIL from
 * @a pid->month, day, hour and minute.
 *
 * @returns
 * @c FALSE if any of the parameters to encode are invalid. In this
 * case @a buffer remains unmodified.
 */
vbi_bool
vbi_encode_vps_pdc		(uint8_t		buffer[13],
				 const vbi_program_id *pid)
{
	unsigned int month;
	unsigned int day;
	unsigned int hour;
	unsigned int minute;
	unsigned int pil;

	assert (NULL != buffer);
	assert (NULL != pid);

	if (unlikely ((unsigned int) pid->pty > 0xFF))
		return FALSE;

	if (unlikely ((unsigned int) pid->pcs_audio > 3))
		return FALSE;

	pil = pid->pil;

	switch (pil) {
	case VBI_PIL_TIMER_CONTROL:
	case VBI_PIL_INHIBIT_TERMINATE:
	case VBI_PIL_INTERRUPT:
	case VBI_PIL_CONTINUE:
		break;

	default:
		if (0 == pil) {
			month = pid->month;
			day = pid->day;
			hour = pid->hour;
			minute = pid->minute;

			pil = VBI_PIL (month, day, hour, minute);
		} else {
			month = VBI_PIL_MONTH (pil);
			day = VBI_PIL_DAY (pil);
			hour = VBI_PIL_HOUR (pil);
			minute = VBI_PIL_MINUTE (pil);
		}

		if (unlikely ((month - 1) > 11
			      || (day - 1) > 30
			      || hour > 23
			      || minute > 59))
			return FALSE;

		break;
	}

	if (!vbi_encode_vps_cni (buffer, pid->cni))
		return FALSE;

	buffer[2] = (buffer[2] & 0x3F) | (pid->pcs_audio << 6);
	buffer[8] = (buffer[8] & 0xC0) | ((pil >> 14) & 0x3F);
	buffer[9] = pil >> 6;
	buffer[10] = (buffer[10] & 0x03) | (pil << 2);
	buffer[12] = pid->pty;

	return TRUE;
}

/**
 * @param buffer A DVB PDC descriptor as defined in EN 300 468,
 *   including descriptor_tag and descriptor_length.
 * @param pid PDC data to encode.
 * 
 * Stores PDC recording-control data (PIL only) in a DVB PDC descriptor
 * as defined in EN 300 468 and EN 300 231. If non-zero the function
 * encodes @a pid->pil, otherwise it calculates the PIL from
 * @a pid->month, day, hour and minute.
 *
 * @returns
 * @c FALSE if any of the parameters to encode are invalid. In this
 * case @a buffer remains unmodified.
 */
vbi_bool
vbi_encode_dvb_pdc_descriptor	(uint8_t		buffer[5],
				 const vbi_program_id *pid)
{
	unsigned int month;
	unsigned int day;
	unsigned int hour;
	unsigned int minute;
	unsigned int pil;

	assert (NULL != buffer);
	assert (NULL != pid);

	pil = pid->pil;

	switch (pil) {
	case VBI_PIL_TIMER_CONTROL:
	case VBI_PIL_INHIBIT_TERMINATE:
	case VBI_PIL_INTERRUPT:
	case VBI_PIL_CONTINUE:
		break;

	default:
		if (0 == pil) {
			month = pid->month;
			day = pid->day;
			hour = pid->hour;
			minute = pid->minute;

			pil = VBI_PIL (month, day, hour, minute);
		} else {
			month = VBI_PIL_MONTH (pil);
			day = VBI_PIL_DAY (pil);
			hour = VBI_PIL_HOUR (pil);
			minute = VBI_PIL_MINUTE (pil);
		}

		if (unlikely ((month - 1) > 11
			      || (day - 1) > 30
			      || hour > 23
			      || minute > 59))
			return FALSE;

		break;
	}

	/* descriptor_tag [8],
	   descriptor_length [8],
	   reserved_future_use [4],
	   programme_identification_label [20] ->
	     day [5], month [4], hour [5], minute [6] */

	/* EN 300 468 section 6.1, 6.2. */
	buffer[0] = 0x69;
	buffer[1] = 3;

	/* EN 300 468 section 6.2.29. */

	/* EN 300 468 section 3.1: "Unless otherwise specified within
	   the present document all 'reserved_future_use' bits shall
	   be set to '1'." */
	buffer[2] = 0xF0 | (pil >> 16);

	buffer[3] = pil >> 8;
	buffer[4] = pil;

	return TRUE;
}

#endif /* 3 == VBI_VERSION_MINOR */

/*
Local variables:
c-set-style: K&R
c-basic-offset: 8
End:
*/
