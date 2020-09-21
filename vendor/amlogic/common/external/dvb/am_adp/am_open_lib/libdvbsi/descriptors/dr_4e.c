#ifdef _FORTIFY_SOURCE
#undef _FORTIFY_SOURCE
#endif
/*****************************************************************************
 * dr_4e.c
 * (c)2005 VideoLAN
 * $Id: dr_4d.c,v 1.7 2003/07/25 20:20:40 fenrir Exp $
 *
 * Authors: Laurent Aimar <fenrir@via.ecp.fr>
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

#include "dr_4e.h"
#include <am_debug.h>

/*****************************************************************************
 * dvbpsi_DecodeExtendedEventDr
 *****************************************************************************/
dvbpsi_extended_event_dr_t * dvbpsi_DecodeExtendedEventDr(dvbpsi_descriptor_t * p_descriptor)
{
  dvbpsi_extended_event_dr_t * p_decoded;
  int i_len;
  int i_pos;
  uint8_t *p;

 /* Check the tag */
  if(p_descriptor->i_tag != 0x4e ||
     p_descriptor->i_length < 6 )
  {
    DVBPSI_ERROR_ARG("dr_4e decoder", "bad tag or corrupted(0x%x)", p_descriptor->i_tag);
    return NULL;
  }

  /*
   * $:Bug 105997
   */
  int length_of_item = p_descriptor->p_data[4];

  if (p_descriptor->i_length < length_of_item + 6)
  {
	AM_DEBUG(0, "dr_4e decoder check length_of_item error, sub_len:%d, length_of_item:%d",
		p_descriptor->i_length, length_of_item);
    return NULL;
  }

  int text_len = p_descriptor->p_data[5+length_of_item];

  if (p_descriptor->i_length < length_of_item + text_len + 6)
  {
	AM_DEBUG(0, "dr_4e decoder check sub_len error, sub_len:%d, text_len:%d",
		p_descriptor->i_length, text_len);
    return NULL;
  }
  //AM_DEBUG(0, "dr_4e decoder,sub_len:%d, length_of_item:%d, text_len:%d",
  //	p_descriptor->i_length, length_of_item, text_len);
  //check length_of_item
  int check_item = 0;
  for ( p = &p_descriptor->p_data[5]; p < &p_descriptor->p_data[5+length_of_item]; )
  {
      //AM_DEBUG(0, "dr_4e, ==>1 item_description_length:%d",  p[0]);
      p += 1 + p[0];
	  check_item += (p[0]+1);


	  //AM_DEBUG(0, "dr_4e, ==>2 item_length:%d", p[0]);
      p += 1 + p[0];
	  check_item += (p[0]+1);
  }
  //AM_DEBUG(0, "dr_4e, ==> check_item:%d, length_of_item:%d", check_item, length_of_item);
  if (length_of_item < check_item)
  {
	AM_DEBUG(0, "dr_4e check length_of_item error");
	return NULL;
  }


  /* Don't decode twice */
  if (p_descriptor->p_decoded)
    return p_descriptor->p_decoded;

  /* Allocate memory */
  p_decoded = malloc(sizeof(dvbpsi_extended_event_dr_t));
  if (!p_decoded)
  {
    DVBPSI_ERROR("dr_4e decoder", "out of memory");
    return NULL;
  }

  /* Decode */
  p_decoded->i_descriptor_number = (p_descriptor->p_data[0] >> 4)&0xf;
  p_decoded->i_last_descriptor_number = p_descriptor->p_data[0]&0x0f;
  memcpy( &p_decoded->i_iso_639_code[0], &p_descriptor->p_data[1], 3 );
  p_decoded->i_entry_count = 0;
  i_len = p_descriptor->p_data[4];
  i_pos = 0;
  for ( p = &p_descriptor->p_data[5]; p < &p_descriptor->p_data[5+i_len]; )
  {
      int idx = p_decoded->i_entry_count;

      p_decoded->i_item_description_length[idx] = p[0];
      p_decoded->i_item_description[idx] = &p_decoded->i_buffer[i_pos];
      memcpy( &p_decoded->i_buffer[i_pos], &p[1], p[0] );
      i_pos += p[0];
      p += 1 + p[0];

      p_decoded->i_item_length[idx] = p[0];
      p_decoded->i_item[idx] = &p_decoded->i_buffer[i_pos];
      memcpy( &p_decoded->i_buffer[i_pos], &p[1], p[0] );
      i_pos += p[0];
      p += 1 + p[0];

      p_decoded->i_entry_count++;
  }

  p_decoded->i_text_length = p_descriptor->p_data[5+i_len];
  if ( p_decoded->i_text_length > 0 ) {
	memcpy( &p_decoded->i_buffer[i_pos],
              &p_descriptor->p_data[5+i_len+1], p_decoded->i_text_length );
  }
  p_decoded->i_text = &p_decoded->i_buffer[i_pos];

  p_descriptor->p_decoded = (void*)p_decoded;

  return p_decoded;
}


/*****************************************************************************
 * dvbpsi_GenExtendedEventDr
 *****************************************************************************/
dvbpsi_descriptor_t * dvbpsi_GenExtendedEventDr(dvbpsi_extended_event_dr_t * p_decoded,
                                          int b_duplicate)
{
  int i_len;
  int i_len2;
  dvbpsi_descriptor_t * p_descriptor;
  int i;

  i_len2 = 0;
  for ( i = 0; i < p_decoded->i_entry_count; i++ )
      i_len2 += 2 + p_decoded->i_item_description_length[i] + p_decoded->i_item_length[i];
  i_len = 1 + 3 + 1 + i_len2 + 1 + p_decoded->i_text_length;

  /* Create the descriptor */
  p_descriptor = dvbpsi_NewDescriptor(0x4e, i_len, NULL );

  if (p_descriptor)
  {
    uint8_t *p = &p_descriptor->p_data[0];

    /* Encode data */
    p[0] = (p_decoded->i_descriptor_number << 4 ) |
           p_decoded->i_last_descriptor_number;
    memcpy( &p[1], p_decoded->i_iso_639_code, 3 );
    p[4] = i_len2;

    p += 4;

    for ( i = 0; i < p_decoded->i_entry_count; i++ )
    {
        p[0] = p_decoded->i_item_description_length[i];
        memcpy( &p[1], p_decoded->i_item_description[i], p[0] );
        p += 1 + p_decoded->i_item_description_length[i];

        p[0] = p_decoded->i_item_length[i];
        memcpy( &p[1], p_decoded->i_item[i], p[0] );
        p += 1 + p_decoded->i_item_length[i];
    }

    p[0] = p_decoded->i_text_length;
    memcpy( &p[1], p_decoded->i_text, p[0] );

    if (b_duplicate)
    {
      /* Duplicate decoded data */
      dvbpsi_extended_event_dr_t * p_dup_decoded =
                (dvbpsi_extended_event_dr_t*)malloc(sizeof(dvbpsi_extended_event_dr_t));
      if (p_dup_decoded)
        memcpy(p_dup_decoded, p_decoded, sizeof(dvbpsi_extended_event_dr_t));

      p_descriptor->p_decoded = (void*)p_dup_decoded;
    }
  }

  return p_descriptor;
}
