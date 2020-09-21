#ifdef _FORTIFY_SOURCE
#undef _FORTIFY_SOURCE
#endif
/*****************************************************************************
 * bat.c: BAT decoder/generator
 *----------------------------------------------------------------------------
 * (c)2001-2008 VideoLAN
 * $Id: bat.c 187 2009-11-18 07:26:10Z md $
 *
 * Authors: Johann Hanne
 *          heavily based on pmt.c which was written by
 *          Arnaud de Bossoreille de Ribou <bozo@via.ecp.fr>
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
 *----------------------------------------------------------------------------
 *
 *****************************************************************************/

#define TABLE_AREA

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
#include "../psi.h"
#include "../descriptor.h"
#include "../demux.h"
#include "bat.h"
#include "bat_private.h"

DEF_SET_DECODE_DESCRIPTOR_CALLBACK(bat)


/*****************************************************************************
 * dvbpsi_AttachBAT
 *****************************************************************************
 * Initialize a BAT subtable decoder.
 *****************************************************************************/
int dvbpsi_AttachBAT(dvbpsi_decoder_t * p_psi_decoder, uint8_t i_table_id,
                     uint16_t i_extension, dvbpsi_bat_callback pf_callback,
                     void* p_cb_data)
{
  dvbpsi_demux_t* p_demux = (dvbpsi_demux_t*)p_psi_decoder->p_private_decoder;
  dvbpsi_demux_subdec_t* p_subdec;
  dvbpsi_bat_decoder_t*  p_bat_decoder;
  unsigned int i;

  if (dvbpsi_demuxGetSubDec(p_demux, i_table_id, i_extension))
  {
    DVBPSI_ERROR_ARG("BAT decoder",
                     "Already a decoder for (table_id == 0x%02x,"
                     "extension == 0x%02x)",
                     i_table_id, i_extension);

    return 1;
  }

  p_subdec = (dvbpsi_demux_subdec_t*)malloc(sizeof(dvbpsi_demux_subdec_t));
  if (p_subdec == NULL)
  {
    return 1;
  }

  p_bat_decoder = (dvbpsi_bat_decoder_t*)malloc(sizeof(dvbpsi_bat_decoder_t));

  if (p_bat_decoder == NULL)
  {
    free(p_subdec);
    return 1;
  }

  /* subtable decoder configuration */
  p_subdec->pf_callback = &dvbpsi_GatherBATSections;
  p_subdec->p_cb_data = p_bat_decoder;
  p_subdec->i_id = (uint32_t)i_table_id << 16 | (uint32_t)i_extension;
  p_subdec->pf_detach = dvbpsi_DetachBAT;

  /* Attach the subtable decoder to the demux */
  p_subdec->p_next = p_demux->p_first_subdec;
  p_demux->p_first_subdec = p_subdec;

  /* BAT decoder information */
  p_bat_decoder->i_bouquet_id = i_extension;
  p_bat_decoder->pf_callback = pf_callback;
  p_bat_decoder->p_cb_data = p_cb_data;
  /* BAT decoder initial state */
  p_bat_decoder->b_current_valid = 0;
  p_bat_decoder->p_building_bat = NULL;
  for (i = 0; i <= 255; i++)
    p_bat_decoder->ap_sections[i] = NULL;

  return 0;
}


/*****************************************************************************
 * dvbpsi_DetachBAT
 *****************************************************************************
 * Close a BAT decoder.
 *****************************************************************************/
void dvbpsi_DetachBAT(dvbpsi_demux_t * p_demux, uint8_t i_table_id,
                      uint16_t i_extension)
{
  dvbpsi_demux_subdec_t* p_subdec;
  dvbpsi_demux_subdec_t** pp_prev_subdec;
  dvbpsi_bat_decoder_t* p_bat_decoder;

  unsigned int i;

  p_subdec = dvbpsi_demuxGetSubDec(p_demux, i_table_id, i_extension);

  if (p_demux == NULL)
  {
    DVBPSI_ERROR_ARG("BAT Decoder",
                     "No such BAT decoder (table_id == 0x%02x,"
                     "extension == 0x%02x)",
                     i_table_id, i_extension);
    return;
  }

  p_bat_decoder = (dvbpsi_bat_decoder_t*)p_subdec->p_cb_data;

  free(p_bat_decoder->p_building_bat);

  for (i = 0; i <= 255; i++)
  {
    if (p_bat_decoder->ap_sections[i])
      dvbpsi_DeletePSISections(p_bat_decoder->ap_sections[i]);
  }

  free(p_subdec->p_cb_data);

  pp_prev_subdec = &p_demux->p_first_subdec;
  while (*pp_prev_subdec != p_subdec)
    pp_prev_subdec = &(*pp_prev_subdec)->p_next;

  *pp_prev_subdec = p_subdec->p_next;
  free(p_subdec);
}


/*****************************************************************************
 * dvbpsi_InitBAT
 *****************************************************************************
 * Initialize a pre-allocated dvbpsi_bat_t structure.
 *****************************************************************************/
void dvbpsi_InitBAT(dvbpsi_bat_t* p_bat, uint16_t i_bouquet_id,
                    uint8_t i_version, int b_current_next)
{
  p_bat->i_bouquet_id = i_bouquet_id;
  p_bat->i_version = i_version;
  p_bat->b_current_next = b_current_next;
  p_bat->p_first_descriptor = NULL;
  p_bat->p_first_ts = NULL;
}


/*****************************************************************************
 * dvbpsi_EmptyBAT
 *****************************************************************************
 * Clean a dvbpsi_bat_t structure.
 *****************************************************************************/
void dvbpsi_EmptyBAT(dvbpsi_bat_t* p_bat)
{
  dvbpsi_bat_ts_t* p_ts = p_bat->p_first_ts;

  dvbpsi_DeleteDescriptors(p_bat->p_first_descriptor);

  while (p_ts != NULL)
  {
    dvbpsi_bat_ts_t* p_tmp = p_ts->p_next;
    dvbpsi_DeleteDescriptors(p_ts->p_first_descriptor);
    free(p_ts);
    p_ts = p_tmp;
  }

  p_bat->p_first_descriptor = NULL;
  p_bat->p_first_ts = NULL;
}


/*****************************************************************************
 * dvbpsi_BATAddDescriptor
 *****************************************************************************
 * Add a descriptor in the BAT.
 *****************************************************************************/
dvbpsi_descriptor_t* dvbpsi_BATAddDescriptor(dvbpsi_bat_t* p_bat,
                                             uint8_t i_tag, uint8_t i_length,
                                             uint8_t* p_data)
{
  dvbpsi_descriptor_t* p_descriptor
                        = dvbpsi_NewDescriptor(i_tag, i_length, p_data);

  if (p_descriptor)
  {
    if (p_bat->p_first_descriptor == NULL)
    {
      p_bat->p_first_descriptor = p_descriptor;
    }
    else
    {
      dvbpsi_descriptor_t* p_last_descriptor = p_bat->p_first_descriptor;
      while (p_last_descriptor->p_next != NULL)
        p_last_descriptor = p_last_descriptor->p_next;
      p_last_descriptor->p_next = p_descriptor;
    }
  }

  return p_descriptor;
}


/*****************************************************************************
 * dvbpsi_BATAddTS
 *****************************************************************************
 * Add an TS in the BAT.
 *****************************************************************************/
dvbpsi_bat_ts_t* dvbpsi_BATAddTS(dvbpsi_bat_t* p_bat,
                                 uint16_t i_ts_id, uint16_t i_orig_network_id)
{
  dvbpsi_bat_ts_t* p_ts = (dvbpsi_bat_ts_t*)malloc(sizeof(dvbpsi_bat_ts_t));

  if (p_ts)
  {
    p_ts->i_ts_id = i_ts_id;
    p_ts->i_orig_network_id = i_orig_network_id;
    p_ts->p_first_descriptor = NULL;
    p_ts->p_next = NULL;

    if (p_bat->p_first_ts == NULL)
    {
      p_bat->p_first_ts = p_ts;
    }
    else
    {
      dvbpsi_bat_ts_t* p_last_ts = p_bat->p_first_ts;
      while (p_last_ts->p_next != NULL)
        p_last_ts = p_last_ts->p_next;
      p_last_ts->p_next = p_ts;
    }
  }

  return p_ts;
}


/*****************************************************************************
 * dvbpsi_BATTSAddDescriptor
 *****************************************************************************
 * Add a descriptor in the BAT TS.
 *****************************************************************************/
dvbpsi_descriptor_t* dvbpsi_BATTSAddDescriptor(dvbpsi_bat_ts_t* p_ts,
                                               uint8_t i_tag, uint8_t i_length,
                                               uint8_t* p_data)
{
  dvbpsi_descriptor_t* p_descriptor
                        = dvbpsi_NewDescriptor(i_tag, i_length, p_data);

  if (p_descriptor)
  {
    if (p_ts->p_first_descriptor == NULL)
    {
      p_ts->p_first_descriptor = p_descriptor;
    }
    else
    {
      dvbpsi_descriptor_t* p_last_descriptor = p_ts->p_first_descriptor;
      while (p_last_descriptor->p_next != NULL)
        p_last_descriptor = p_last_descriptor->p_next;
      p_last_descriptor->p_next = p_descriptor;
    }
  }

  return p_descriptor;
}


/*****************************************************************************
 * dvbpsi_GatherBATSections
 *****************************************************************************
 * Callback for the PSI decoder.
 *****************************************************************************/
void dvbpsi_GatherBATSections(dvbpsi_decoder_t * p_decoder,
                              void * p_private_decoder,
                              dvbpsi_psi_section_t * p_section)
{
  dvbpsi_bat_decoder_t* p_bat_decoder
                        = (dvbpsi_bat_decoder_t*)p_private_decoder;
  int b_append = 1;
  int b_reinit = 0;
  unsigned int i;

  DVBPSI_DEBUG_ARG("BAT decoder",
                   "Table version %2d, " "i_extension %5d, "
                   "section %3d up to %3d, " "current %1d",
                   p_section->i_version, p_section->i_extension,
                   p_section->i_number, p_section->i_last_number,
                   p_section->b_current_next);

  if (p_section->i_table_id != 0x40 && p_section->i_table_id != 0x41)
  {
    /* Invalid table_id value */
    DVBPSI_ERROR_ARG("BAT decoder",
                     "invalid section (table_id == 0x%02x)",
                     p_section->i_table_id);
    b_append = 0;
  }

  if (b_append && !p_section->b_syntax_indicator)
  {
    /* Invalid section_syntax_indicator */
    DVBPSI_ERROR("BAT decoder",
                 "invalid section (section_syntax_indicator == 0)");
    b_append = 0;
  }

  /* Now if b_append is true then we have a valid BAT section */
  if (b_append && (p_bat_decoder->i_bouquet_id != p_section->i_extension))
  {
    /* Invalid program_number */
#if 0
    DVBPSI_ERROR("BAT decoder", "'bouquet_id' don't match");
#endif
    b_append = 0;
  }

  if (b_append)
  {
    /* TS discontinuity check */
    if (p_decoder->b_discontinuity)
    {
      b_reinit = 1;
      p_decoder->b_discontinuity = 0;
    }
    else
    {
      /* Perform some few sanity checks */
      if (p_bat_decoder->p_building_bat)
      {
        if (p_bat_decoder->p_building_bat->i_version != p_section->i_version)
        {
          /* version_number */
          DVBPSI_ERROR("BAT decoder",
                       "'version_number' differs"
                       " whereas no discontinuity has occured");
          b_reinit = 1;
        }
        else if(p_bat_decoder->i_last_section_number
                                                != p_section->i_last_number)
        {
          /* last_section_number */
          DVBPSI_ERROR("BAT decoder",
                       "'last_section_number' differs"
                       " whereas no discontinuity has occured");
          b_reinit = 1;
        }
      }
      else
      {
        if (    (p_bat_decoder->b_current_valid)
            && (p_bat_decoder->current_bat.i_version == p_section->i_version)
            && (p_bat_decoder->current_bat.b_current_next ==
                                           p_section->b_current_next))
        {
          /* Don't decode since this version is already decoded */
          b_append = 0;
        }
      }
    }
  }

  /* Reinit the decoder if wanted */
  if (b_reinit)
  {
    /* Force redecoding */
    p_bat_decoder->b_current_valid = 0;
    /* Free structures */
    if (p_bat_decoder->p_building_bat)
    {
      free(p_bat_decoder->p_building_bat);
      p_bat_decoder->p_building_bat = NULL;
    }
    /* Clear the section array */
    for (i = 0; i <= 255; i++)
    {
      if (p_bat_decoder->ap_sections[i] != NULL)
      {
        dvbpsi_DeletePSISections(p_bat_decoder->ap_sections[i]);
        p_bat_decoder->ap_sections[i] = NULL;
      }
    }
  }

  /* Append the section to the list if wanted */
  if (b_append)
  {
    int b_complete;

    /* Initialize the structures if it's the first section received */
    if (!p_bat_decoder->p_building_bat)
    {
      p_bat_decoder->p_building_bat =
                                (dvbpsi_bat_t*)malloc(sizeof(dvbpsi_bat_t));
      dvbpsi_InitBAT(p_bat_decoder->p_building_bat,
                     p_bat_decoder->i_bouquet_id,
                     p_section->i_version,
                     p_section->b_current_next);
      p_bat_decoder->i_last_section_number = p_section->i_last_number;
    }

    /* Fill the section array */
    if (p_bat_decoder->ap_sections[p_section->i_number] != NULL)
    {
      DVBPSI_DEBUG_ARG("BAT decoder", "overwrite section number %d",
                       p_section->i_number);
      dvbpsi_DeletePSISections(p_bat_decoder->ap_sections[p_section->i_number]);
    }
    p_bat_decoder->ap_sections[p_section->i_number] = p_section;

    /* Check if we have all the sections */
    b_complete = 0;
    for (i = 0; i <= p_bat_decoder->i_last_section_number; i++)
    {
      if (!p_bat_decoder->ap_sections[i])
        break;

      if (i == p_bat_decoder->i_last_section_number)
        b_complete = 1;
    }

    if (b_complete)
    {
      /* Save the current information */
      p_bat_decoder->current_bat = *p_bat_decoder->p_building_bat;
      p_bat_decoder->b_current_valid = 1;
      /* Chain the sections */
      if (p_bat_decoder->i_last_section_number)
      {
        for (i = 0; (int)i <= p_bat_decoder->i_last_section_number - 1; i++)
          p_bat_decoder->ap_sections[i]->p_next =
                                        p_bat_decoder->ap_sections[i + 1];
      }
      /* Decode the sections */
      dvbpsi_DecodeBATSections(p_bat_decoder->p_building_bat,
                               p_bat_decoder->ap_sections[0]);
      /* Delete the sections */
      dvbpsi_DeletePSISections(p_bat_decoder->ap_sections[0]);
      /* signal the new BAT */
      p_bat_decoder->pf_callback(p_bat_decoder->p_cb_data,
                                 p_bat_decoder->p_building_bat);
      /* Reinitialize the structures */
      p_bat_decoder->p_building_bat = NULL;
      for (i = 0; i <= p_bat_decoder->i_last_section_number; i++)
        p_bat_decoder->ap_sections[i] = NULL;
    }
  }
  else
  {
    dvbpsi_DeletePSISections(p_section);
  }
}


/*****************************************************************************
 * dvbpsi_DecodeBATSections
 *****************************************************************************
 * BAT decoder.
 *****************************************************************************/
void dvbpsi_DecodeBATSections(dvbpsi_bat_t* p_bat,
                              dvbpsi_psi_section_t* p_section)
{
  uint8_t* p_byte, * p_end, * p_end2;

  while (p_section)
  {
    /* - BAT descriptors */
    p_byte = p_section->p_payload_start + 2;
    p_end = p_byte + (   ((uint16_t)(p_section->p_payload_start[0] & 0x0f) << 8)
                       | p_section->p_payload_start[1]);

    while (p_byte + 2 <= p_end)
    {
      uint8_t i_tag = p_byte[0];
      uint8_t i_length = p_byte[1];
      if (i_length + 2 <= p_end - p_byte)
        dvbpsi_BATAddDescriptor(p_bat, i_tag, i_length, p_byte + 2);
      p_byte += 2 + i_length;
    }

    p_end = p_byte + (   ((uint16_t)(p_byte[0] & 0x0f) << 8)
                       | p_byte[1]);
    if (p_end > p_section->p_payload_end)
    {
        p_end = p_section->p_payload_end;
    }
    p_byte += 2;

    /* - TSs */
    for (; p_byte + 6 <= p_end;)
    {
      uint16_t i_ts_id = ((uint16_t)p_byte[0] << 8) | p_byte[1];
      uint16_t i_orig_network_id = ((uint16_t)p_byte[2] << 8) | p_byte[3];
      uint16_t i_ts_length = ((uint16_t)(p_byte[4] & 0x0f) << 8) | p_byte[5];
      dvbpsi_bat_ts_t* p_ts = dvbpsi_BATAddTS(p_bat, i_ts_id, i_orig_network_id);
      /* - TS descriptors */
      p_byte += 6;
      p_end2 = p_byte + i_ts_length;
      if ( p_end2 > p_section->p_payload_end )
      {
            p_end2 = p_section->p_payload_end;
      }
      while (p_byte + 2 <= p_end2)
      {
        uint8_t i_tag = p_byte[0];
        uint8_t i_length = p_byte[1];
        if (i_length + 2 <= p_end2 - p_byte)
          dvbpsi_BATTSAddDescriptor(p_ts, i_tag, i_length, p_byte + 2);
        p_byte += 2 + i_length;
      }
    }

    p_section = p_section->p_next;
  }
}


/*****************************************************************************
 * dvbpsi_GenBATSections
 *****************************************************************************
 * Generate BAT sections based on the dvbpsi_bat_t structure.
 *****************************************************************************/
dvbpsi_psi_section_t* dvbpsi_GenBATSections(dvbpsi_bat_t* p_bat,
                                            uint8_t i_table_id)
{
  dvbpsi_psi_section_t* p_result = dvbpsi_NewPSISection(1024);
  dvbpsi_psi_section_t* p_current = p_result;
  dvbpsi_psi_section_t* p_prev;
  dvbpsi_descriptor_t* p_descriptor = p_bat->p_first_descriptor;
  dvbpsi_bat_ts_t* p_ts = p_bat->p_first_ts;
  uint16_t i_bouquet_descriptors_length, i_transport_stream_loop_length;
  uint8_t * p_transport_stream_loop_length;

  p_current->i_table_id = i_table_id;
  p_current->b_syntax_indicator = 1;
  p_current->b_private_indicator = 0;
  p_current->i_length = 13;                     /* including CRC_32 */
  p_current->i_extension = p_bat->i_bouquet_id;
  p_current->i_version = p_bat->i_version;
  p_current->b_current_next = p_bat->b_current_next;
  p_current->i_number = 0;
  p_current->p_payload_end += 10;
  p_current->p_payload_start = p_current->p_data + 8;

  /* BAT descriptors */
  while (p_descriptor != NULL)
  {
    /* New section if needed */
    /* written_data_length + descriptor_length + 2 > 1024 - CRC_32_length */
    if (   (p_current->p_payload_end - p_current->p_data)
                                + p_descriptor->i_length > 1018)
    {
      /* bouquet_descriptors_length */
      i_bouquet_descriptors_length = (p_current->p_payload_end - p_current->p_payload_start) - 2;
      p_current->p_data[8] = (i_bouquet_descriptors_length >> 8) | 0xf0;
      p_current->p_data[9] = i_bouquet_descriptors_length;

      /* transport_stream_loop_length */
      p_current->p_payload_end[0] = 0;
      p_current->p_payload_end[1] = 0;
      p_current->p_payload_end += 2;

      p_prev = p_current;
      p_current = dvbpsi_NewPSISection(1024);
      p_prev->p_next = p_current;

      p_current->i_table_id = i_table_id;
      p_current->b_syntax_indicator = 1;
      p_current->b_private_indicator = 0;
      p_current->i_length = 13;                 /* including CRC_32 */
      p_current->i_extension = p_bat->i_bouquet_id;
      p_current->i_version = p_bat->i_version;
      p_current->b_current_next = p_bat->b_current_next;
      p_current->i_number = p_prev->i_number + 1;
      p_current->p_payload_end += 10;
      p_current->p_payload_start = p_current->p_data + 8;
    }

    /* p_payload_end is where the descriptor begins */
    p_current->p_payload_end[0] = p_descriptor->i_tag;
    p_current->p_payload_end[1] = p_descriptor->i_length;
    memcpy(p_current->p_payload_end + 2,
           p_descriptor->p_data,
           p_descriptor->i_length);

    /* Increase length by descriptor_length + 2 */
    p_current->p_payload_end += p_descriptor->i_length + 2;
    p_current->i_length += p_descriptor->i_length + 2;

    p_descriptor = p_descriptor->p_next;
  }

  /* bouquet_descriptors_length */
  i_bouquet_descriptors_length = (p_current->p_payload_end - p_current->p_payload_start) - 2;
  p_current->p_data[8] = (i_bouquet_descriptors_length >> 8) | 0xf0;
  p_current->p_data[9] = i_bouquet_descriptors_length;

  /* Store the position of the transport_stream_loop_length field
     and reserve two bytes for it */
  p_transport_stream_loop_length = p_current->p_payload_end;
  p_current->p_payload_end += 2;

  /* BAT TSs */
  while (p_ts != NULL)
  {
    uint8_t* p_ts_start = p_current->p_payload_end;
    uint16_t i_ts_length = 5;

    /* Can the current section carry all the descriptors ? */
    p_descriptor = p_ts->p_first_descriptor;
    while (    (p_descriptor != NULL)
           && ((p_ts_start - p_current->p_data) + i_ts_length <= 1020))
    {
      i_ts_length += p_descriptor->i_length + 2;
      p_descriptor = p_descriptor->p_next;
    }

    /* If _no_ and the current section isn't empty and an empty section
       may carry one more descriptor
       then create a new section */
    if (    (p_descriptor != NULL)
        && (p_ts_start - p_current->p_data != 12)
        && (i_ts_length <= 1008))
    {
      /* transport_stream_loop_length */
      i_transport_stream_loop_length = (p_current->p_payload_end - p_transport_stream_loop_length) - 2;
      p_transport_stream_loop_length[0] = (i_transport_stream_loop_length >> 8) | 0xf0;
      p_transport_stream_loop_length[1] = i_transport_stream_loop_length;

      /* will put more descriptors in an empty section */
      DVBPSI_DEBUG("BAT generator",
                   "create a new section to carry more TS descriptors");
      p_prev = p_current;
      p_current = dvbpsi_NewPSISection(1024);
      p_prev->p_next = p_current;

      p_current->i_table_id = i_table_id;
      p_current->b_syntax_indicator = 1;
      p_current->b_private_indicator = 0;
      p_current->i_length = 13;                 /* including CRC_32 */
      p_current->i_extension = p_bat->i_bouquet_id;
      p_current->i_version = p_bat->i_version;
      p_current->b_current_next = p_bat->b_current_next;
      p_current->i_number = p_prev->i_number + 1;
      p_current->p_payload_end += 10;
      p_current->p_payload_start = p_current->p_data + 8;

      /* bouquet_descriptors_length = 0 */
      p_current->p_data[8] = 0xf0;
      p_current->p_data[9] = 0x00;

      /* Store the position of the transport_stream_loop_length field
         and reserve two bytes for it */
      p_transport_stream_loop_length = p_current->p_payload_end;
      p_current->p_payload_end += 2;

      p_ts_start = p_current->p_payload_end;
    }

    /* p_ts_start is where the TS begins */
    p_ts_start[0] = p_ts->i_ts_id >> 8;
    p_ts_start[1] = p_ts->i_ts_id & 0xff;
    p_ts_start[2] = p_ts->i_orig_network_id >> 8;
    p_ts_start[3] = p_ts->i_orig_network_id & 0xff;

    /* Increase the length by 6 */
    p_current->p_payload_end += 6;
    p_current->i_length += 6;

    /* TS descriptors */
    p_descriptor = p_ts->p_first_descriptor;
    while (    (p_descriptor != NULL)
           && (   (p_current->p_payload_end - p_current->p_data)
                + p_descriptor->i_length <= 1018))
    {
      /* p_payload_end is where the descriptor begins */
      p_current->p_payload_end[0] = p_descriptor->i_tag;
      p_current->p_payload_end[1] = p_descriptor->i_length;
      memcpy(p_current->p_payload_end + 2,
             p_descriptor->p_data,
             p_descriptor->i_length);

      /* Increase length by descriptor_length + 2 */
      p_current->p_payload_end += p_descriptor->i_length + 2;
      p_current->i_length += p_descriptor->i_length + 2;

      p_descriptor = p_descriptor->p_next;
    }

    if (p_descriptor != NULL)
      DVBPSI_ERROR("BAT generator", "unable to carry all the TS descriptors");

    /* TS_info_length */
    i_ts_length = p_current->p_payload_end - p_ts_start - 5;
    p_ts_start[4] = (i_ts_length >> 8) | 0xf0;
    p_ts_start[5] = i_ts_length;

    p_ts = p_ts->p_next;
  }

  /* transport_stream_loop_length */
  i_transport_stream_loop_length = (p_current->p_payload_end - p_transport_stream_loop_length) - 2;
  p_transport_stream_loop_length[0] = (i_transport_stream_loop_length >> 8) | 0xf0;
  p_transport_stream_loop_length[1] = i_transport_stream_loop_length;

  /* Finalization */
  p_prev = p_result;
  while (p_prev != NULL)
  {
    p_prev->i_last_number = p_current->i_number;
    dvbpsi_BuildPSISection(p_prev);
    p_prev = p_prev->p_next;
  }

  return p_result;
}

