#ifdef _FORTIFY_SOURCE
#undef _FORTIFY_SOURCE
#endif
/*****************************************************************************
 * dr_4a.c
 * (c)2001-2002 VideoLAN
 * $Id: dr_4a.c 110 2005-06-04 12:52:02Z gbazin $
 *
 * Authors: Arnaud de Bossoreille de Ribou <bozo@via.ecp.fr>
 *          Johan Bilien <jobi@via.ecp.fr>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 *****************************************************************************/


#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(HAVE_INTTYPES_H)
#include <inttypes.h>
#elif defined(HAVE_STDINT_H)
#include <stdint.h>
#endif

#include "../dvbpsi.h"
#include "../dvbpsi_private.h"
#include "../descriptor.h"

#include "dr_4a.h"


/*****************************************************************************
 * dvbpsi_DecodeLinkageDr
 *****************************************************************************/
dvbpsi_linkage_dr_t * dvbpsi_DecodeLinkageDr(
                                        dvbpsi_descriptor_t * p_descriptor)
{
  dvbpsi_linkage_dr_t * p_decoded;

  /* Check the tag */
  if (p_descriptor->i_tag != 0x4a)
  {
    DVBPSI_ERROR_ARG("dr_4a decoder", "bad tag (0x%x)", p_descriptor->i_tag);
    return NULL;
  }

  /* Don't decode twice */
  if (p_descriptor->p_decoded)
    return p_descriptor->p_decoded;

  /* Allocate memory */
  p_decoded =
        (dvbpsi_linkage_dr_t*)malloc(sizeof(dvbpsi_linkage_dr_t));
  if (!p_decoded)
  {
    DVBPSI_ERROR("dr_4a decoder", "out of memory");
    return NULL;
  }

  /* Decode data and check the length */
  if (p_descriptor->i_length < 7)
  {
    DVBPSI_ERROR_ARG("dr_4a decoder", "bad length (%d)",
                     p_descriptor->i_length);
    free(p_decoded);
    return NULL;
  }

  p_decoded->i_ts_id 	  = (uint16_t)p_descriptor->p_data[0]<<8 | \
							(uint16_t)p_descriptor->p_data[1];
  p_decoded->i_network_id = (uint16_t)p_descriptor->p_data[2]<<8 | \
							(uint16_t)p_descriptor->p_data[3];
  p_decoded->i_service_id = (uint16_t)p_descriptor->p_data[4]<<8 | \
							(uint16_t)p_descriptor->p_data[5];
  p_decoded->i_linkage_type = p_descriptor->p_data[6];

  if (p_descriptor->i_length > 7)
	p_decoded->i_prv_data = p_descriptor->p_data + 7;
  else
	p_decoded->i_prv_data = NULL;

  p_descriptor->p_decoded = (void*)p_decoded;

  return p_decoded;
}


