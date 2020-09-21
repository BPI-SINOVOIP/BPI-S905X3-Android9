#ifdef _FORTIFY_SOURCE
#undef _FORTIFY_SOURCE
#endif
/*****************************************************************************
 * dr_42.c
 * (c)2001-2002 VideoLAN
 * $Id: dr_42.c 88 2004-02-24 14:31:18Z sam $
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

#include "dr_42.h"


/*****************************************************************************
 * dvbpsi_DecodeStuffingDr
 *****************************************************************************/
dvbpsi_stuffing_dr_t * dvbpsi_DecodeStuffingDr(
                                        dvbpsi_descriptor_t * p_descriptor)
{
  dvbpsi_stuffing_dr_t * p_decoded;

  /* Check the tag */
  if (p_descriptor->i_tag != 0x42)
  {
    DVBPSI_ERROR_ARG("dr_42 decoder", "bad tag (0x%x)", p_descriptor->i_tag);
    return NULL;
  }

  /* Don't decode twice */
  if (p_descriptor->p_decoded)
    return p_descriptor->p_decoded;

  /* Allocate memory */
  p_decoded =
        (dvbpsi_stuffing_dr_t*)malloc(sizeof(dvbpsi_stuffing_dr_t));
  if (!p_decoded)
  {
    DVBPSI_ERROR("dr_42 decoder", "out of memory");
    return NULL;
  }

  /* Decode data */
  p_decoded->i_stuffing_length = p_descriptor->i_length;
  if (p_decoded->i_stuffing_length)
    memcpy(p_decoded->i_stuffing_byte,
           p_descriptor->p_data,
           p_decoded->i_stuffing_length);

  p_descriptor->p_decoded = (void*)p_decoded;

  return p_decoded;
}


/*****************************************************************************
 * dvbpsi_GenStuffingDr
 *****************************************************************************/
dvbpsi_descriptor_t * dvbpsi_GenStuffingDr(
                                        dvbpsi_stuffing_dr_t * p_decoded,
                                        int b_duplicate)
{
  /* Create the descriptor */
  dvbpsi_descriptor_t * p_descriptor =
        dvbpsi_NewDescriptor(0x42, p_decoded->i_stuffing_length, NULL);

  if (p_descriptor)
  {
    /* Encode data */
    if (p_decoded->i_stuffing_length)
      memcpy(p_descriptor->p_data,
             p_decoded->i_stuffing_byte,
             p_decoded->i_stuffing_length);

    if (b_duplicate)
    {
      /* Duplicate decoded data */
      dvbpsi_stuffing_dr_t * p_dup_decoded =
        (dvbpsi_stuffing_dr_t*)malloc(sizeof(dvbpsi_stuffing_dr_t));
      if (p_dup_decoded)
        memcpy(p_dup_decoded, p_decoded, sizeof(dvbpsi_stuffing_dr_t));

      p_descriptor->p_decoded = (void*)p_dup_decoded;
    }
  }

  return p_descriptor;
}

