#ifdef _FORTIFY_SOURCE
#undef _FORTIFY_SOURCE
#endif
/*****************************************************************************
 * dr_0a.c
 * (c)2001-2002 VideoLAN
 * $Id: dr_0a.c 132 2006-07-18 08:34:12Z sigmunau $
 *
 * Authors: Arnaud de Bossoreille de Ribou <bozo@via.ecp.fr>
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

#include "dr_0a.h"


/*****************************************************************************
 * dvbpsi_DecodeISO639Dr
 *****************************************************************************/
dvbpsi_iso639_dr_t * dvbpsi_DecodeISO639Dr(dvbpsi_descriptor_t * p_descriptor)
{
  dvbpsi_iso639_dr_t * p_decoded;
  int i;

  /* Check the tag */
  if (p_descriptor->i_tag != 0x0a)
  {
    DVBPSI_ERROR_ARG("dr_0a decoder", "bad tag (0x%x)", p_descriptor->i_tag);
    return NULL;
  }

  /* Don't decode twice */
  if (p_descriptor->p_decoded)
    return p_descriptor->p_decoded;

  /* Allocate memory */
  p_decoded = (dvbpsi_iso639_dr_t*)malloc(sizeof(dvbpsi_iso639_dr_t));
  if (!p_decoded)
  {
    DVBPSI_ERROR("dr_0a decoder", "out of memory");
    return NULL;
  }

  /* Decode data and check the length */
  if ((p_descriptor->i_length < 1) || (p_descriptor->i_length % 4 != 0))
  {
    DVBPSI_ERROR_ARG("dr_0a decoder", "bad length (%d)",
                     p_descriptor->i_length);
    free(p_decoded);
    return NULL;
  }

  p_decoded->i_code_count = p_descriptor->i_length / 4;
  i = 0;
  while ( i < p_decoded->i_code_count ) {
    p_decoded->code[i].iso_639_code[0] = p_descriptor->p_data[i*4];
    p_decoded->code[i].iso_639_code[1] = p_descriptor->p_data[i*4+1];
    p_decoded->code[i].iso_639_code[2] = p_descriptor->p_data[i*4+2];

    /* Convert to lower case if possible */
    dvbpsi_ToLower(p_decoded->code[i].iso_639_code, 3);

    p_decoded->code[i].i_audio_type = p_descriptor->p_data[i*4+3];
    i++;
  }
  p_descriptor->p_decoded = (void*)p_decoded;

  return p_decoded;
}


/*****************************************************************************
 * dvbpsi_GenISO639Dr
 *****************************************************************************/
dvbpsi_descriptor_t * dvbpsi_GenISO639Dr(dvbpsi_iso639_dr_t * p_decoded,
                                         int b_duplicate)
{
  /* Create the descriptor */
  dvbpsi_descriptor_t * p_descriptor =
        dvbpsi_NewDescriptor(0x0a, p_decoded->i_code_count * 4, NULL);

  if (p_descriptor)
  {
    /* Encode data */
    int i = 0;
    while ( i < p_decoded->i_code_count ) {
      p_descriptor->p_data[i*4] = p_decoded->code[i].iso_639_code[0];
      p_descriptor->p_data[i*4+1] = p_decoded->code[i].iso_639_code[1];
      p_descriptor->p_data[i*4+2] = p_decoded->code[i].iso_639_code[2];
      p_descriptor->p_data[i*4+3] = p_decoded->code[i].i_audio_type;
      i++;
    }
    if (b_duplicate)
    {
      /* Duplicate decoded data */
      dvbpsi_iso639_dr_t * p_dup_decoded =
                        (dvbpsi_iso639_dr_t*)malloc(sizeof(dvbpsi_iso639_dr_t));
      if (p_dup_decoded)
        memcpy(p_dup_decoded, p_decoded, sizeof(dvbpsi_iso639_dr_t));

      p_descriptor->p_decoded = (void*)p_dup_decoded;
    }
  }

  return p_descriptor;
}

