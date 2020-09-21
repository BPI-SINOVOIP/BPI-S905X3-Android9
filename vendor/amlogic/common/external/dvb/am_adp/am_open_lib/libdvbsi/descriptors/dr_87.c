#ifdef _FORTIFY_SOURCE
#undef _FORTIFY_SOURCE
#endif
/*****************************************************************************
 * dr_87.c
 * (c)2001-2008 VideoLAN
 * $Id: dr_58.c 172 2008-04-26 12:10:54Z jpsaman $
 *
 * Authors: Gong Ke
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

#include "dr_87.h"

/*****************************************************************************
 * dvbpsi_DecodeLogicalChannelNumber83Dr
 *****************************************************************************/
dvbpsi_logical_channel_number_87_dr_t * dvbpsi_DecodeLogicalChannelNumber87Dr(
                                        dvbpsi_descriptor_t * p_descriptor)
{
  dvbpsi_logical_channel_number_87_dr_t * p_decoded;
  uint8_t * p_data, * p_end, * p_list_end;
  dvbpsi_logical_channel_number_87_t * p_current_num;
  dvbpsi_logical_channel_list_87_t * p_current_list;

  /* Check the tag */
  if (p_descriptor->i_tag != 0x87)
  {
    DVBPSI_ERROR_ARG("dr_87 decoder", "bad tag (0x%x)", p_descriptor->i_tag);
    return NULL;
  }

  /* Don't decode twice */
  if (p_descriptor->p_decoded)
    return p_descriptor->p_decoded;

  /* Allocate memory */
  p_decoded =
        (dvbpsi_logical_channel_number_87_dr_t*)malloc(sizeof(dvbpsi_logical_channel_number_87_dr_t));
  if (!p_decoded)
  {
    DVBPSI_ERROR("dr_87 decoder", "out of memory");
    return NULL;
  }

  /* Decode data */
  p_decoded->i_logical_channel_lists_number = 0;
  p_current_list = p_decoded->p_logical_channel_list;
  p_end = p_descriptor->p_data + p_descriptor->i_length;
  p_data = p_descriptor->p_data;
  while (p_data + 6 <= p_end) {
    p_current_list->i_channel_list_id          =   p_data[0];
    p_current_list->i_channel_list_name_length =   p_data[1];
    p_data += 2;

    if (p_current_list->i_channel_list_name_length) {
      if (p_data+p_current_list->i_channel_list_name_length+4 > p_end)
        break;
      memcpy(p_current_list->i_channel_list_name, p_data+2, p_current_list->i_channel_list_name_length);
    }
    p_data += p_current_list->i_channel_list_name_length;

    memcpy(p_current_list->i_country_code, p_data, 3);
    p_list_end = p_data + 4 + p_data[3];
    p_data += 4;

    if (p_list_end > p_end)
      break;

    p_current_list->i_logical_channel_numbers_number = 0;
    p_current_num = p_current_list->p_logical_channel_number;

    while (p_data + 4 <= p_list_end) {
      p_current_num->i_service_id = (p_data[0] << 8) | p_data[1];
      p_current_num->i_visible_service_flag = (p_data[2] & 0x80) ? 1 : 0;
      p_current_num->i_logical_channel_number = ((p_data[2] & 0x03) << 8) | p_data[3];
      p_current_list->i_logical_channel_numbers_number++;
      p_data += 4;
      p_current_num++;
    }

    p_current_list++;
  }

  p_descriptor->p_decoded = (void*)p_decoded;

  return p_decoded;
}


/*****************************************************************************
 * dvbpsi_GenLogicalChannelNumber87Dr
 *****************************************************************************/
dvbpsi_descriptor_t * dvbpsi_GenLogicalChannelNumber87Dr(
                                        dvbpsi_logical_channel_number_87_dr_t * p_decoded,
                                        int b_duplicate)
{
  uint8_t i_list_num, i_num_num;
  dvbpsi_logical_channel_number_87_t * p_current_num;
  dvbpsi_logical_channel_list_87_t * p_current_list;
  uint8_t * p_data;
  uint8_t len = 0;

  p_current_list = p_decoded->p_logical_channel_list;
  for (i_list_num=0; i_list_num<p_decoded->i_logical_channel_lists_number; i_list_num++) {
	  len += 6+p_current_list->i_channel_list_name_length+p_current_list->i_logical_channel_numbers_number*4;
	  p_current_list++;
  }

  /* Create the descriptor */
  dvbpsi_descriptor_t * p_descriptor =
        dvbpsi_NewDescriptor(0x87, len, NULL);

  if (p_descriptor)
  {
    /* Encode data */

    p_current_list = p_decoded->p_logical_channel_list;
    p_data = p_descriptor->p_data;

    for (i_list_num = 0; i_list_num < p_decoded->i_logical_channel_lists_number; i_list_num++) {
      p_data[0]  =   p_current_list->i_channel_list_id;
      p_data[1]  =   p_current_list->i_channel_list_name_length;
      p_data += 2;

      if (p_current_list->i_channel_list_name_length) {
        memcpy(p_data, p_current_list->i_channel_list_name, p_current_list->i_channel_list_name_length);
      }
      p_data += p_current_list->i_channel_list_name_length;

      memcpy(p_data, p_current_list->i_country_code, 3);
      p_data[3]  =   p_current_list->i_logical_channel_numbers_number*4;
      p_data += 4;

      p_current_num = p_current_list->p_logical_channel_number;

      for (i_num_num = 0; i_num_num < p_current_list->i_logical_channel_numbers_number; i_num_num++) {
        p_data[0]  =   (p_current_num->i_service_id >> 8) & 0xff;
        p_data[1]  =    p_current_num->i_service_id & 0xff;
        p_data[2]  =   (p_current_num->i_visible_service_flag?0x80:0)
	             | 0x7C
                     | ((p_current_num->i_logical_channel_number >> 8) & 0x03);
        p_data[3]  =    p_current_num->i_logical_channel_number & 0xff;

        p_data += 4;
	p_current_num++;
      }

      p_current_list++;
    }

    if (b_duplicate)
    {
      /* Duplicate decoded data */
      dvbpsi_logical_channel_number_87_dr_t * p_dup_decoded =
        (dvbpsi_logical_channel_number_87_dr_t*)malloc(sizeof(dvbpsi_logical_channel_number_87_dr_t));
      if (p_dup_decoded)
        memcpy(p_dup_decoded, p_decoded, sizeof(dvbpsi_logical_channel_number_87_dr_t));

      p_descriptor->p_decoded = (void*)p_dup_decoded;
    }
  }

  return p_descriptor;
}
