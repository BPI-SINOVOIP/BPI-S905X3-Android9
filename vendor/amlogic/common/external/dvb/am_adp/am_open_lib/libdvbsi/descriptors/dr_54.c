#ifdef _FORTIFY_SOURCE
#undef _FORTIFY_SOURCE
#endif
/*****************************************************************************
 * dr_54.c
 * (c)2005 Andrew John Hughes
 *
 * Authors: Andrew John Hughes <gnu_andrew@member.fsf.org>
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

#include "dr_54.h"


/*****************************************************************************
 * dvbpsi_DecodeContentDr
 *****************************************************************************/
dvbpsi_content_dr_t * dvbpsi_DecodeContentDr(
                                        dvbpsi_descriptor_t * p_descriptor)
{
  dvbpsi_content_dr_t * p_decoded;

  /* Check the tag */
  if (p_descriptor->i_tag != 0x54)
  {
    DVBPSI_ERROR_ARG("dr_54 decoder", "bad tag (0x%x)", p_descriptor->i_tag);
    return NULL;
  }

  /* Don't decode twice */
  if (p_descriptor->p_decoded)
    return p_descriptor->p_decoded;

  /* Allocate memory */
  p_decoded =
        (dvbpsi_content_dr_t*)malloc(sizeof(dvbpsi_content_dr_t));
  if (!p_decoded)
  {
    DVBPSI_ERROR("dr_54 decoder", "out of memory");
    return NULL;
  }

  /* Decode data and check the length */
  if (p_descriptor->i_length < 2)
  {
    DVBPSI_ERROR_ARG("dr_54 decoder", "bad length (%d)",
                     p_descriptor->i_length);
    free(p_decoded);
    return NULL;
  }

  p_decoded->i_nibble_level = p_descriptor->p_data[0];
  p_decoded->i_user_byte = p_descriptor->p_data[1];

  p_descriptor->p_decoded = (void*)p_decoded;

  return p_decoded;
}


/*****************************************************************************
 * dvbpsi_GenContentDr
 *****************************************************************************/
dvbpsi_descriptor_t * dvbpsi_GenContentDr(
                                        dvbpsi_content_dr_t * p_decoded,
                                        int b_duplicate)
{
	/*We don't support yet*/
	return NULL;
}

