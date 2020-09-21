#ifdef _FORTIFY_SOURCE
#undef _FORTIFY_SOURCE
#endif
/*****************************************************************************
 * dr_44.c
 * (c)2001-2002 VideoLAN
 * $Id: dr_44.c 110 2005-06-04 12:52:02Z gbazin $
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

#include "dr_44.h"


/*****************************************************************************
 * dvbpsi_DecodeCableDeliveryDr
 *****************************************************************************/
dvbpsi_cable_delivery_dr_t * dvbpsi_DecodeCableDeliveryDr(
                                        dvbpsi_descriptor_t * p_descriptor)
{
  dvbpsi_cable_delivery_dr_t * p_decoded;

  /* Check the tag */
  if (p_descriptor->i_tag != 0x44)
  {
    DVBPSI_ERROR_ARG("dr_44 decoder", "bad tag (0x%x)", p_descriptor->i_tag);
    return NULL;
  }

  /* Don't decode twice */
  if (p_descriptor->p_decoded)
    return p_descriptor->p_decoded;

  /* Allocate memory */
  p_decoded =
        (dvbpsi_cable_delivery_dr_t*)malloc(sizeof(dvbpsi_cable_delivery_dr_t));
  if (!p_decoded)
  {
    DVBPSI_ERROR("dr_44 decoder", "out of memory");
    return NULL;
  }

  /* Decode data and check the length */
  if (p_descriptor->i_length < 11)
  {
    DVBPSI_ERROR_ARG("dr_44 decoder", "bad length (%d)",
                     p_descriptor->i_length);
    free(p_decoded);
    return NULL;
  }

  p_decoded->i_frequency         =   DVBPSI_BCD2VALUE(p_descriptor->p_data[0]) * 100000000 +
									DVBPSI_BCD2VALUE(p_descriptor->p_data[1]) * 1000000 +
									DVBPSI_BCD2VALUE(p_descriptor->p_data[2]) * 10000 +
									DVBPSI_BCD2VALUE(p_descriptor->p_data[3]) * 100;
  p_decoded->i_fec_outer		 = p_descriptor->p_data[5] & 0x0f;
  p_decoded->i_modulation_type	 = p_descriptor->p_data[6];
  p_decoded->i_symbol_rate       =   DVBPSI_BCD2VALUE(p_descriptor->p_data[7]) * 10000000 +
									DVBPSI_BCD2VALUE(p_descriptor->p_data[8]) * 100000 +
									DVBPSI_BCD2VALUE(p_descriptor->p_data[9]) * 1000 +
									((p_descriptor->p_data[10]&0xf0) >> 4) * 100;
  p_decoded->i_fec_inner         =    p_descriptor->p_data[10] & 0x0f;

  p_descriptor->p_decoded = (void*)p_decoded;



  return p_decoded;
}


