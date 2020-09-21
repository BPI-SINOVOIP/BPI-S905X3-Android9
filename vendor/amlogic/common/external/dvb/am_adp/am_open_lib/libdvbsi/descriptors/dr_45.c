#ifdef _FORTIFY_SOURCE
#undef _FORTIFY_SOURCE
#endif
/*****************************************************************************
 * dr_45.c
 * (c)2004-2007 VideoLAN
 * $Id: dr_45.c 174 2008-05-22 16:55:20Z jpsaman $
 *
 * Authors: Jean-Paul Saman <jpsaman at videolan dot org>
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

#include "dr_45.h"

/*****************************************************************************
 * dvbpsi_DecodeVBIDataDr
 *****************************************************************************/
dvbpsi_vbi_dr_t * dvbpsi_DecodeVBIDataDr(
                                        dvbpsi_descriptor_t * p_descriptor)
{
  int i;
  int left;
  dvbpsi_vbi_dr_t * p_decoded;
  uint8_t *pdata;

  /* Check the tag */
  if ( p_descriptor->i_tag != 0x45 )
  {
    DVBPSI_ERROR_ARG("dr_45 decoder", "bad tag (0x%x)", p_descriptor->i_tag);
    return NULL;
  }

  /* Don't decode twice */
  if (p_descriptor->p_decoded)
    return p_descriptor->p_decoded;

#if 0
  /* Decode data and check the length */
  if (p_descriptor->i_length < 3)
  {
    DVBPSI_ERROR_ARG("dr_45 decoder", "bad length (%d)",
                     p_descriptor->i_length);
    return NULL;
  }

  if (p_descriptor->i_length % 2)
  {
    DVBPSI_ERROR_ARG("dr_45 decoder", "length not multiple of 3(%d)",
                     p_descriptor->i_length);
    return NULL;
  }
#endif

  /* Allocate memory */
  p_decoded =
        (dvbpsi_vbi_dr_t*)malloc(sizeof(dvbpsi_vbi_dr_t));
  if (!p_decoded)
  {
    DVBPSI_ERROR("dr_45 decoder", "out of memory");
    return NULL;
  }

  i     = 0;
  left  = p_descriptor->i_length;
  pdata = p_descriptor->p_data + 2;

  while (left >= 2)
  {
    int n, i_lines = 0, i_data_service_id;

    i_data_service_id = ((uint8_t)pdata[0]);
    p_decoded->p_services[i].i_data_service_id = i_data_service_id;

    i_lines = ((uint8_t)pdata[1]);
    p_decoded->p_services[i].i_lines = i_lines;

	left  -= 2;
	pdata += 2;

	if (left < i_lines)
		break;

    for (n=0; n < i_lines; n++ )
    {
      if ( (i_data_service_id >= 0x01) && (i_data_service_id <= 0x07) )
      {
        p_decoded->p_services[i].p_lines[n].i_parity =
               ((uint8_t)((pdata[n])&0x20)>>5);
        p_decoded->p_services[i].p_lines[n].i_line_offset =
               ((uint8_t)(pdata[n])&0x1f);
      }
    }

	left  -= i_lines;
	pdata += i_lines;
    i++;
  }

  p_decoded->i_services_number = i;

  p_descriptor->p_decoded = (void*)p_decoded;

  return p_decoded;
}


/*****************************************************************************
 * dvbpsi_GenVBIDataDr
 *****************************************************************************/
dvbpsi_descriptor_t * dvbpsi_GenVBIDataDr(
                                        dvbpsi_vbi_dr_t * p_decoded,
                                        int b_duplicate)
{
  int i;

  /* Create the descriptor */
  dvbpsi_descriptor_t * p_descriptor =
      dvbpsi_NewDescriptor(0x45, p_decoded->i_services_number * 5 , NULL);

  if (p_descriptor)
  {
    /* Encode data */
    for (i=0; i < p_decoded->i_services_number; i++ )
    {
      int n;
      p_descriptor->p_data[5 * i + 3] =
                    ( (uint8_t) p_decoded->p_services[i].i_data_service_id );

      p_descriptor->p_data[5 * i + 4] = p_decoded->p_services[i].i_lines;
      for (n=0; n < p_decoded->p_services[i].i_lines; n++ )
      {
         if ( (p_decoded->p_services[i].i_data_service_id >= 0x01) &&
             (p_decoded->p_services[i].i_data_service_id <= 0x07) )
         {
            p_descriptor->p_data[5 * i + 4 + n] = (uint8_t)
                ( (((uint8_t) p_decoded->p_services[i].p_lines[n].i_parity)&0x20)<<5) |
                ( ((uint8_t) p_decoded->p_services[i].p_lines[n].i_line_offset)&0x1f);
         }
         else p_descriptor->p_data[5 * i + 3 + n] = 0xFF; /* Stuffing byte */
      }
    }

    if (b_duplicate)
    {
      /* Duplicate decoded data */
      dvbpsi_vbi_dr_t * p_dup_decoded =
        (dvbpsi_vbi_dr_t*)malloc(sizeof(dvbpsi_vbi_dr_t));
      if (p_dup_decoded)
        memcpy(p_dup_decoded, p_decoded, sizeof(dvbpsi_vbi_dr_t));

      p_descriptor->p_decoded = (void*)p_dup_decoded;
    }
  }

  return p_descriptor;
}
