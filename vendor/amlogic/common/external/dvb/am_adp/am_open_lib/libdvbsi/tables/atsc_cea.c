#ifdef _FORTIFY_SOURCE
#undef _FORTIFY_SOURCE
#endif
/*
Copyright (C) 2006-2012  Adam Charrett

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

cea.c

Decode PSIP cable emergency alert msg Table.

*/
#include "config.h"
#define TABLE_AREA

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#if defined(HAVE_INTTYPES_H)
#include <inttypes.h>
#elif defined(HAVE_STDINT_H)
#include <stdint.h>
#endif

#include <assert.h>
#include <am_debug.h>
#include "../dvbpsi.h"
#include "../dvbpsi_private.h"
#include "../psi.h"
#include "../descriptor.h"
#include "../demux.h"
#include "huffman_decode_private.h"
#include "atsc_cea.h"

//#define CEA_DEBUG 1

DEF_SET_DECODE_DESCRIPTOR_CALLBACK(atsc_cea)

static uint32_t atsc_out_puthuffman (uint8_t *out, uint8_t *buf, uint32_t len, uint32_t comp)
{
    int olen;
    olen = psi_atsc_huffman_to_string(out, buf, len, comp - 1);
    return (uint32_t)olen;
}

static uint32_t atsc_out_putucstr (uint8_t *out, uint8_t *buf, uint32_t len)
{
    unsigned char *pin, *pout;
    int left, olen;
    unsigned short uc;

    pin  = buf;
    pout = out;
    left = len;
    olen = 0;

    while (left >= 2) {
        uc = (pin[0] << 8) | pin[1];

        if (uc <= 0x7f) {
            pout[0] = uc;

            pout ++;
            olen ++;
        } else if (uc <= 0x7ff) {
            pout[0] = (uc >> 6) | 0xc0;
            pout[1] = (uc & 0x3f) | 0x80;

            pout += 2;
            olen += 2;
        } else {
            pout[0] = (uc >> 12) | 0xe0;
            pout[1] = ((uc >> 6) & 0x3f) | 0x80;
            pout[2] = (uc & 0x3f) | 0x80;

            pout += 3;
            olen += 3;
        }

        pin  += 2;
        left -= 2;
    }
    return (uint32_t)olen;
}

static uint32_t atsc_out_comp_str(uint8_t *out, uint8_t *buf, uint32_t comp, uint8_t mode, uint32_t num)
{
    if (num > 0) {
        if ((mode == 0) && ((comp == 1) || (comp == 2))) {
            return atsc_out_puthuffman(out, buf, num, comp);
        } else if ((mode>=0x0 && mode <=0x7)
                || (mode >= 0x9 && mode <= 0x10)
                || (mode >= 0x20 && mode <= 0x27)
                || (mode>=0x30 && mode <=0x33)) {
            unsigned char ucb[num * 2], *puc = ucb;
            int i;

            for (i = 0; i < num; i ++) {
                puc[0] = mode;
                puc[1] = buf[i];
                puc += 2;
            }
            return  atsc_out_putucstr(out, ucb, num * 2);
        } else if (mode == 0x3f) {
            return atsc_out_putucstr(out, buf, num);
        } else if (mode == 0x3e) {
            memcpy(out, buf, num);
            return num;
        }
    }
    return 0;
}

/*****************************************************************************
 * dvbpsi_atsc_InitCEA
 *****************************************************************************
 * Initialize a pre-allocated dvbpsi_atsc_cea_t structure.
 *****************************************************************************/
void dvbpsi_atsc_InitCEA(dvbpsi_atsc_cea_t* p_cea, uint8_t i_table_id, uint16_t i_extension,
                         uint8_t i_version, bool b_current_next)
{
    p_cea->i_table_id = i_table_id;
    p_cea->i_extension = i_extension;
    p_cea->i_version = i_version;
    p_cea->b_current_next = b_current_next;
    p_cea->p_first_multi_text = NULL;
    p_cea->p_first_descriptor = NULL;
}

dvbpsi_atsc_cea_t *dvbpsi_atsc_NewCEA(uint8_t i_table_id, uint16_t i_extension,
                                      uint8_t i_version, bool b_current_next)
{
    dvbpsi_atsc_cea_t *p_cea;
    p_cea = (dvbpsi_atsc_cea_t*) malloc(sizeof(dvbpsi_atsc_cea_t));
    memset(p_cea, 0, sizeof(dvbpsi_atsc_cea_t));
#ifdef CEA_DEBUG
    AM_DEBUG(1, "new CEA %d %d %d %d",
                       i_table_id,
                       i_extension,
                       i_version,
                       b_current_next);
#endif
    if (p_cea != NULL)
        dvbpsi_atsc_InitCEA(p_cea, i_table_id, i_extension, i_version,
                            b_current_next);
    return p_cea;
}

/*****************************************************************************
 * dvbpsi_atsc_EmptyCEA
 *****************************************************************************
 * Clean a dvbpsi_atsc_cea_t structure.
 *****************************************************************************/
void dvbpsi_atsc_EmptyCEA(dvbpsi_atsc_cea_t* p_cea)
{
  dvbpsi_atsc_cea_multi_str_t* p_multi_text = p_cea->p_first_multi_text;
  AM_DEBUG(1, "free CEA info");
  while (p_multi_text != NULL)
  {
    dvbpsi_atsc_cea_multi_str_t* p_tmp = p_multi_text->p_next;
    free(p_multi_text);
    p_multi_text = p_tmp;
  }
  p_cea->p_first_multi_text = NULL;

  dvbpsi_DeleteDescriptors(p_cea->p_first_descriptor);
  p_cea->p_first_descriptor = NULL;
}

void dvbpsi_atsc_DeleteCEA(dvbpsi_atsc_cea_t *p_cea)
{
    AM_DEBUG(1, "delete CEA info");
    if (p_cea) {
        dvbpsi_atsc_EmptyCEA(p_cea);
        free(p_cea);
        p_cea = NULL;
    }
}
/*****************************************************************************
 * dvbpsi_atsc_setEasCEA
 *****************************************************************************
 * set cea aes info.
 *****************************************************************************/
void dvbpsi_atsc_setEasCEA(dvbpsi_atsc_cea_t* p_cea, uint8_t i_protocol_version, uint16_t i_eas_event_id,
                         uint8_t *i_eas_orig_code, uint8_t i_eas_event_code_len, uint8_t *i_eas_event_code)
{
#ifdef CEA_DEBUG
    AM_DEBUG(1, "set CEA info[%d][%d][%s][%d][%s]",
              i_protocol_version,
              i_eas_event_id,
              i_eas_orig_code,
              i_eas_event_code_len,
              i_eas_event_code);
#endif
    p_cea->i_protocol_version = i_protocol_version;
    p_cea->i_eas_event_id = i_eas_event_id;
    memcpy(p_cea->i_eas_orig_code, i_eas_orig_code, 3);
    p_cea->i_eas_event_code_len = i_eas_event_code_len;
    memcpy(p_cea->i_eas_event_code, i_eas_event_code, i_eas_event_code_len);
}

/*****************************************************************************
 * dvbpsi_atsc_setTimeAndOOBBInfoCEA
 *****************************************************************************
 * set eas time and oob info.
 *****************************************************************************/
void dvbpsi_atsc_setTimeAndOOBBInfoCEA(dvbpsi_atsc_cea_t* p_cea,
                                      uint8_t  i_alert_message_time_remaining,
                                      uint32_t i_event_start_time,
                                      uint16_t i_event_duration,
                                      uint8_t  i_alert_priority,
                                      uint16_t i_details_OOB_source_ID,
                                      uint16_t i_details_major_channel_number,
                                      uint16_t i_details_minor_channel_number,
                                      uint16_t i_audio_OOB_source_ID
                                      )
{
  #ifdef CEA_DEBUG
    AM_DEBUG(1, "CEA set time and oob info remain[%d]start time[%d]dur[%d]priority[%d]oob id[%d]maj[%d]min[%d]audio oob[%d]",
            i_alert_message_time_remaining,
            i_event_start_time,
            i_event_duration,
            i_alert_priority,
            i_details_OOB_source_ID,
            i_details_major_channel_number,
            i_details_minor_channel_number,
            i_audio_OOB_source_ID);
  #endif
    p_cea->i_alert_message_time_remaining = i_alert_message_time_remaining;
    p_cea->i_event_start_time = i_event_start_time;
    p_cea->i_event_duration = i_event_duration;
    p_cea->i_alert_priority = i_alert_priority;

    p_cea->i_details_OOB_source_ID = i_details_OOB_source_ID;
    p_cea->i_details_major_channel_number = i_details_major_channel_number;
    p_cea->i_details_minor_channel_number = i_details_minor_channel_number;
    p_cea->i_audio_OOB_source_ID = i_audio_OOB_source_ID;
}
/*****************************************************************************
 * dvbpsi_CEAAddDescriptor
 *****************************************************************************
 * Add a descriptor in the cea
 * ********************************************/
dvbpsi_descriptor_t* dvbpsi_CEAAddDescriptor(dvbpsi_atsc_cea_t* p_cea,
                                             uint8_t i_tag, uint8_t i_length,
                                             uint8_t* p_data)
{
  dvbpsi_descriptor_t* p_descriptor
                        = dvbpsi_NewDescriptor(i_tag, i_length, p_data);
  AM_DEBUG(1, "new CEA des info");
  if (p_descriptor)
  {
    if (p_cea->p_first_descriptor == NULL)
    {
      p_cea->p_first_descriptor = p_descriptor;
    }
    else
    {
      dvbpsi_descriptor_t* p_last_descriptor = p_cea->p_first_descriptor;
      while (p_last_descriptor->p_next != NULL)
        p_last_descriptor = p_last_descriptor->p_next;
      p_last_descriptor->p_next = p_descriptor;
    }
  }

  return p_descriptor;
}

/*****************************************************************************
 * dvbpsi_atsc_CEAAddMulti
 *****************************************************************************
 * Add a multi text at the end of the cea.
 *****************************************************************************/
static dvbpsi_atsc_cea_multi_str_t *dvbpsi_atsc_CEAAddMulti(dvbpsi_atsc_cea_t* p_cea,
                                            uint8_t *p_lang,
                                            uint8_t i_type,
                                            uint8_t i_compression_type,
                                            uint8_t i_mode,
                                            uint8_t i_number_bytes,
                                            uint8_t *p_compressed_str)
{

  dvbpsi_atsc_cea_multi_str_t * p_multi_text
                = (dvbpsi_atsc_cea_multi_str_t*)malloc(sizeof(dvbpsi_atsc_cea_multi_str_t));

  if (p_multi_text)
  {
    memset(p_multi_text, 0, sizeof(dvbpsi_atsc_cea_multi_str_t));
    memcpy(p_multi_text->lang, p_lang, 3);
    p_multi_text->i_type = i_type;
    p_multi_text->i_compression_type = i_compression_type;
    p_multi_text->i_mode = i_mode;

    uint32_t i_len = atsc_out_comp_str(p_multi_text->compressed_str,
                                                p_compressed_str,
                                                i_compression_type,
                                                i_mode,
                                                i_number_bytes);
    if (i_len <= 0) {
        memcpy(p_multi_text->compressed_str, p_compressed_str, i_number_bytes);
        p_multi_text->i_number_bytes = i_number_bytes;
    } else {
        p_multi_text->i_number_bytes = i_len;
    }
    #ifdef CEA_DEBUG
    AM_DEBUG(1, "CEA add multi info len %d [%s]", i_len ,p_multi_text->compressed_str);
    #endif
    p_multi_text->p_next = NULL;

    if (p_cea->p_first_multi_text== NULL)
    {
      p_cea->p_first_multi_text = p_multi_text;
    }
    else
    {
      dvbpsi_atsc_cea_multi_str_t * p_last_multi_text = p_cea->p_first_multi_text;
      while (p_last_multi_text->p_next != NULL)
        p_last_multi_text = p_last_multi_text->p_next;
      p_last_multi_text->p_next = p_multi_text;
    }
  }

  return p_multi_text;
}
/*****************************************************************************
 * dvbpsi_atsc_CEAPaserMulti
 *****************************************************************************
 * parse a multi text at the end of the cea.
 *****************************************************************************/
static void dvbpsi_atsc_CEAPaserMulti(dvbpsi_atsc_cea_t* p_cea,
                                            uint8_t i_type,
                                            uint8_t i_len,
                                            uint8_t *buf)
{
    uint8_t *p_byte =  buf;
    uint8_t multi_text_len = p_byte[0];
    uint8_t i = 0;
    /*skip number_strings*/
    p_byte = p_byte + 1;
    for (i = 0;
        i < multi_text_len;
        i ++)
    {
        uint8_t lang[3];

        memset(lang, 0, 3);
        memcpy(lang, p_byte, 3);
        #ifdef CEA_DEBUG
        AM_DEBUG(1, "CEA parse multi info[%c%c%c]",lang[0],lang[1],lang[2]);
        #endif
        uint8_t  i_seg_num = ((uint8_t) p_byte[3]);
        p_byte = p_byte + 4;
        uint8_t j = 0;
        for (j = 0; j < i_seg_num; j++)
        {
            uint8_t i_compression_type = p_byte[0];
            uint8_t i_mode = p_byte[1];
            uint8_t i_number_bytes = p_byte[2];
            p_byte = p_byte + 3;
            dvbpsi_atsc_CEAAddMulti(p_cea,
                                    lang,
                                    i_type,
                                    i_compression_type,
                                    i_mode,
                                    i_number_bytes,
                                    p_byte);
            p_byte = p_byte + i_number_bytes;
        }
    }
  return ;
}
/*****************************************************************************
 * dvbpsi_atsc_CEAPaserLocalAndExcetion
 *****************************************************************************
 * parse a multi text at the end of the cea.
 *****************************************************************************/
static int dvbpsi_atsc_CEAPaserLocalAndExcetion(dvbpsi_atsc_cea_t* p_cea,
                                                uint8_t *buf)
{
    uint8_t i_location_count = 0;
    uint8_t i_exception_count = 0;
    uint8_t i = 0;
    uint8_t *p_byte =  buf;
    i_location_count = p_byte[0];
    p_byte = p_byte + 1;

    for (i = 0;
        i < i_location_count;
        i++)
    {
        p_cea->location[i].i_state_code = ((uint8_t) p_byte[0]);
        p_cea->location[i].i_country_subdiv = (uint8_t)((p_byte[1] & 0xf0) >> 4);
        p_cea->location[i].i_country_code = (uint16_t)((p_byte[1] & 0x03) << 8) | ((uint16_t) p_byte[2]);
        #ifdef CEA_DEBUG
        AM_DEBUG(1, "CEA parse local info state code[%d]subdiv[%d]country code[%d]",
          p_cea->location[i].i_state_code,
          p_cea->location[i].i_state_code,
          p_cea->location[i].i_country_code);
        #endif
        p_byte = p_byte + 3;
    }

    i_exception_count = p_byte[0];
    p_byte = p_byte + 1;

    for (i = 0;
        i < i_exception_count;
        i++)
    {
        uint8_t  i_in_band_refer = (uint8_t)(p_byte[0] & 0x01);

        p_cea->exception[i].i_in_band_refer = i_in_band_refer;
        if (i_in_band_refer) {
            p_cea->exception[i].i_exception_major_channel_number = (uint16_t)((p_byte[1] & 0x03) << 8) | ((uint16_t) p_byte[2]);
            p_cea->exception[i].i_exception_minor_channel_number = (uint16_t)((p_byte[3] & 0x03) << 8) | ((uint16_t) p_byte[4]);
            #ifdef CEA_DEBUG
            AM_DEBUG(1, "CEA parse exception info channel min[%d]maj[%d]",
                      p_cea->exception[i].i_exception_minor_channel_number,
                      p_cea->exception[i].i_exception_major_channel_number);
            #endif
        } else {
            p_cea->exception[i].exception_OOB_source_ID = (uint16_t)((p_byte[3]) << 8) | ((uint16_t) p_byte[4]);
            #ifdef CEA_DEBUG
            AM_DEBUG(1, "CEA parse exception info oob id[%d]",
                    p_cea->exception[i].exception_OOB_source_ID);
            #endif
        }
        p_byte = p_byte + 5;
    }

  return i_location_count + i_exception_count + 2;
}

/*****************************************************************************
 * dvbpsi_DecodeCEASection
 *****************************************************************************
 * CEA decoder.
 *****************************************************************************/
void dvbpsi_atsc_DecodeCEASections(dvbpsi_atsc_cea_t* p_cea,
                              dvbpsi_psi_section_t* p_section)
{
  uint8_t *p_byte, *p_end;
  AM_DEBUG(1, "CEA decode cea section info");
  while (p_section)
  {
    uint8_t i_protocol_version = 0;
    uint16_t i_eas_event_id = 0;
    uint8_t i_eas_orig_code[4];
    uint8_t i_eas_event_code_len;
    uint8_t i_eas_event_code[256];

    memset(i_eas_orig_code, 0, 4);
    memset(i_eas_event_code, 0, 256);
    /*parse eas info and protocol version*/
    p_byte = p_section->p_payload_start;
    i_protocol_version = p_byte[0];
    AM_DEBUG(1, "CEA decode cea section info i_protocol_version %d", i_protocol_version);
    i_eas_event_id = (uint16_t)(p_byte[1] << 8) | ((uint16_t) p_byte[2]);
    i_eas_orig_code[0] = (uint8_t) p_byte[3];
    i_eas_orig_code[1] = (uint8_t) p_byte[4];
    i_eas_orig_code[2] = (uint8_t) p_byte[5];
    i_eas_event_code_len = (uint8_t) p_byte[6];
    p_byte = p_section->p_payload_start + 6;
    if (i_eas_event_code_len > 0) {
      memcpy(i_eas_event_code, p_byte + 1, i_eas_event_code_len);
      dvbpsi_atsc_setEasCEA(p_cea,
                             i_protocol_version,
                             i_eas_event_id,
                             i_eas_orig_code,
                             i_eas_event_code_len,
                             i_eas_event_code
                              );
      p_byte = p_byte + i_eas_event_code_len;
    }
    /*parse nature text multi info*/
    p_byte = p_byte + 1;
    uint8_t nature_text_len = p_byte[0];
    p_byte = p_byte + 1;
    dvbpsi_atsc_CEAPaserMulti(p_cea, AM_EAS_MULTI_NATURE, nature_text_len, p_byte);
    p_byte = p_byte + nature_text_len;
    /*time priority and oob*/
    uint8_t    i_alert_message_time_remaining;   /*!< alert msg remain time */
    uint32_t   i_event_start_time;   /*!< eas event start time */
    uint16_t   i_event_duration;   /*!< event dur */
    uint8_t    i_alert_priority;   /*!< alert priority */
    uint16_t   i_details_OOB_source_ID;   /*!< details oob source id */
    uint16_t   i_details_major_channel_number;   /*!< details major channel num */
    uint16_t   i_details_minor_channel_number;   /*!< details minor channel num */
    uint16_t   i_audio_OOB_source_ID;   /*!< audio oob source id */

    i_alert_message_time_remaining = (uint8_t) p_byte[0];
    i_event_start_time = (uint32_t)(p_byte[1] << 24) |
                         (uint32_t)(p_byte[2] << 16) |
                         (uint32_t)(p_byte[3] << 8) |
                         (uint32_t)(p_byte[4]);
    i_event_duration = (uint16_t)(p_byte[5] << 8) |
                       (uint16_t) p_byte[6];
    /*p_byte[7] reserved*/
    i_alert_priority = (uint8_t) p_byte[8] & 0x0f;
    p_byte = p_byte + 9;/*details_OOB_source_ID*/
    i_details_OOB_source_ID = (uint16_t)(p_byte[0] << 8) |
                              (uint16_t) p_byte[1];
    /* 111111** ******** 10 bit */
    i_details_major_channel_number = (uint16_t)((p_byte[2] & 0x03) << 8) |
                                     (uint16_t) p_byte[3];
    /* 111111** ******** 10 bit */
    i_details_minor_channel_number = (uint16_t)((p_byte[4] & 0x03) << 8) |
                                     (uint16_t) p_byte[5];
    i_audio_OOB_source_ID = (uint16_t)(p_byte[6] << 8) |
                            (uint16_t) p_byte[7];
    dvbpsi_atsc_setTimeAndOOBBInfoCEA(p_cea,
                           i_alert_message_time_remaining,
                           i_event_start_time,
                           i_event_duration,
                           i_alert_priority,
                           i_details_OOB_source_ID,
                           i_details_major_channel_number,
                           i_details_minor_channel_number,
                           i_audio_OOB_source_ID
                            );
    p_byte = p_byte + 8;/*alert_text_len*/
    /*parse nature text multi info*/
    uint8_t alert_text_len = p_byte[0] << 8 | p_byte[1];
    p_byte = p_byte + 2;
    if (alert_text_len > 0) {
      dvbpsi_atsc_CEAPaserMulti(p_cea, AM_EAS_MULTI_ALERT, alert_text_len, p_byte);
      p_byte = p_byte + alert_text_len;
    }
    /*parse local code and excetion*/
    uint8_t i_local_len = dvbpsi_atsc_CEAPaserLocalAndExcetion(p_cea, p_byte);
    p_byte = p_byte + i_local_len;
    // /*des info*/
    // uint16_t i_des_length = ((uint16_t)(p_byte[0] & 0x03) << 8) | p_byte[1];
    // uint8_t *p_end = p_byte + i_des_length;
    // uint8_t i_tag = p_byte[2];
    // uint8_t i_length = p_byte[3];

    // if (i_length + 2 <= p_end - p_byte) {
    //   dvbpsi_CEAAddDescriptor(p_cea, i_tag, i_length, p_byte + 4);
    // }
    p_section = p_section->p_next;
  }
  AM_DEBUG(1, "CEA decode cea section info end");
}

