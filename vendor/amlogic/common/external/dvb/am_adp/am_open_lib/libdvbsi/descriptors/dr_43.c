#ifdef _FORTIFY_SOURCE
#undef _FORTIFY_SOURCE
#endif
/*****************************************************************************
 * dr_43.c
 * (c)2001-2008 VideoLAN
 * $Id: dr_43.c 171 2008-04-26 12:09:45Z jpsaman $
 *
 * Authors: Johann Hanne
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

#include "dr_43.h"


/*****************************************************************************
 * dvbpsi_DecodeSatDelivSysDr
 *****************************************************************************/
dvbpsi_sat_deliv_sys_dr_t * dvbpsi_DecodeSatDelivSysDr(
                                        dvbpsi_descriptor_t * p_descriptor)
{
  dvbpsi_sat_deliv_sys_dr_t * p_decoded;

  /* Check the tag */
  if (p_descriptor->i_tag != 0x43)
  {
    DVBPSI_ERROR_ARG("dr_43 decoder", "bad tag (0x%x)", p_descriptor->i_tag);
    return NULL;
  }

  /* Don't decode twice */
  if (p_descriptor->p_decoded)
    return p_descriptor->p_decoded;

  /* Allocate memory */
  p_decoded =
        (dvbpsi_sat_deliv_sys_dr_t*)malloc(sizeof(dvbpsi_sat_deliv_sys_dr_t));
  if (!p_decoded)
  {
    DVBPSI_ERROR("dr_43 decoder", "out of memory");
    return NULL;
  }

  /* Decode data */
  p_decoded->i_frequency         =   DVBPSI_BCD2VALUE(p_descriptor->p_data[0]) * 10000000 +
									DVBPSI_BCD2VALUE(p_descriptor->p_data[1]) * 100000 +
									DVBPSI_BCD2VALUE(p_descriptor->p_data[2]) * 1000 +
									DVBPSI_BCD2VALUE(p_descriptor->p_data[3]) * 10;

  p_decoded->i_orbital_position  =   DVBPSI_BCD2VALUE(p_descriptor->p_data[4]) * 100 +
									 DVBPSI_BCD2VALUE(p_descriptor->p_data[5]);

  p_decoded->i_west_east_flag    =   (p_descriptor->p_data[6] >> 7) & 0x01;
  p_decoded->i_polarization      =   (p_descriptor->p_data[6] >> 5) & 0x03;
  p_decoded->i_roll_off          =   (p_descriptor->p_data[6] >> 3) & 0x03;
  p_decoded->i_modulation_system =   (p_descriptor->p_data[6] >> 2) & 0x01;
  p_decoded->i_modulation_type   =    p_descriptor->p_data[6] & 0x03;

  p_decoded->i_symbol_rate       =   DVBPSI_BCD2VALUE(p_descriptor->p_data[7]) * 10000000 +
									DVBPSI_BCD2VALUE(p_descriptor->p_data[8]) * 100000 +
									DVBPSI_BCD2VALUE(p_descriptor->p_data[9]) * 1000 +
									((p_descriptor->p_data[10]&0xf0) >> 4) * 100;

  p_decoded->i_fec_inner         =    p_descriptor->p_data[10] & 0x0f;

  p_descriptor->p_decoded = (void*)p_decoded;

  return p_decoded;
}


/*****************************************************************************
 * dvbpsi_GenSatDelivSysDr
 *****************************************************************************/
dvbpsi_descriptor_t * dvbpsi_GenSatDelivSysDr(
                                        dvbpsi_sat_deliv_sys_dr_t * p_decoded,
                                        int b_duplicate)
{
  /* Create the descriptor */
  dvbpsi_descriptor_t * p_descriptor =
        dvbpsi_NewDescriptor(0x43, 11, NULL);

  if (p_descriptor)
  {
    /* Encode data */
    p_descriptor->p_data[0]  =     (p_decoded->i_frequency >> 24)       & 0xff;
    p_descriptor->p_data[1]  =     (p_decoded->i_frequency >> 16)       & 0xff;
    p_descriptor->p_data[2]  =     (p_decoded->i_frequency >>  8)       & 0xff;
    p_descriptor->p_data[3]  =      p_decoded->i_frequency              & 0xff;
    p_descriptor->p_data[4]  =     (p_decoded->i_orbital_position >> 8) & 0xff;
    p_descriptor->p_data[5]  =      p_decoded->i_orbital_position       & 0xff;
    p_descriptor->p_data[6]  =    ((p_decoded->i_west_east_flag         & 0x01) << 7)
                                | ((p_decoded->i_polarization           & 0x03) << 5)
                                | ((p_decoded->i_roll_off               & 0x03) << 3)
                                | ((p_decoded->i_modulation_system      & 0x01) << 2)
                                |  (p_decoded->i_modulation_type        & 0x03);
    p_descriptor->p_data[7]  =     (p_decoded->i_symbol_rate >> 20)     & 0xff;
    p_descriptor->p_data[8]  =     (p_decoded->i_symbol_rate >> 12)     & 0xff;
    p_descriptor->p_data[9]  =     (p_decoded->i_symbol_rate >>  4)     & 0xff;
    p_descriptor->p_data[10] =    ((p_decoded->i_symbol_rate <<  4)     & 0xf0)
                                |  (p_decoded->i_fec_inner              & 0x0f);

    if (b_duplicate)
    {
      /* Duplicate decoded data */
      dvbpsi_sat_deliv_sys_dr_t * p_dup_decoded =
        (dvbpsi_sat_deliv_sys_dr_t*)malloc(sizeof(dvbpsi_sat_deliv_sys_dr_t));
      if (p_dup_decoded)
        memcpy(p_dup_decoded, p_decoded, sizeof(dvbpsi_sat_deliv_sys_dr_t));

      p_descriptor->p_decoded = (void*)p_dup_decoded;
    }
  }

  return p_descriptor;
}
