#ifdef _FORTIFY_SOURCE
#undef _FORTIFY_SOURCE
#endif
/***************************************************************************
 * Copyright (C) 2017 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * Description:
 */
/**\file  dr_cc.c
 * \brief AMLogic descriptor cc parse
 *
 * \author chen hua ling <hualing.chen@amlogic.com>
 * \date 2016-06-22: create the document
 ***************************************************************************/


#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(HAVE_INTTYPES_H)
#include <inttypes.h>
#elif defined(HAVE_STDINT_H)
#include <stdint.h>
#endif
#include <am_debug.h>
#include "../dvbpsi.h"
#include "../dvbpsi_private.h"
#include "../descriptor.h"

#include "dr_cc.h"
#define DEBUG_DRCC 0
/*****************************************************************************
 * dvbpsi_DecodePSIPENAC3Dr
 *****************************************************************************/
dvbpsi_PSIPENAC3_dr_t * dvbpsi_DecodePSIPENAC3Dr(dvbpsi_descriptor_t * p_descriptor)
{
  dvbpsi_PSIPENAC3_dr_t * p_decoded;
  AM_DEBUG(1, "dr_cc dvbpsi_DecodePSIPENAC3Dr ");
  /* Check the tag */
  if (p_descriptor->i_tag != 0xCC)
  {
    DVBPSI_ERROR_ARG("dr_CC decoder", "bad tag (0x%x)", p_descriptor->i_tag);
    AM_DEBUG(1, "dr_CC decoder bad tag (0x%x)",p_descriptor->i_tag);
    return NULL;
  }

  /* Don't decode twice */
  if (p_descriptor->p_decoded)
    return p_descriptor->p_decoded;

  /* Allocate memory */
  p_decoded =
        (dvbpsi_PSIPENAC3_dr_t *)malloc(sizeof(dvbpsi_PSIPENAC3_dr_t));
  if (!p_decoded)
  {
    DVBPSI_ERROR("dr_cc decoder", "out of memory");
    AM_DEBUG(1, "dr_cc decoder out of memory");
    return NULL;
  }
  memset(p_decoded, 0, sizeof(dvbpsi_PSIPENAC3_dr_t));
  /* Decode data and check the length */
  if (DEBUG_DRCC) {
    int i = 0;
    AM_DEBUG(1, "dr_cc vaule: ");
    for (i = 0; i < p_descriptor->i_length; i++) {
      AM_DEBUG(1, " %2x", p_descriptor->p_data[i]);
    }
    AM_DEBUG(1, "dr_cc vaule end");
  }
  if (p_descriptor->i_length < 3)
  {
    DVBPSI_ERROR_ARG("dr_cc decoder", "bad length (%d)",
                     p_descriptor->i_length);
    AM_DEBUG(1, "dr_cc decoder bad length (%d)",
                     p_descriptor->i_length);
    free(p_decoded);
    return NULL;
  }
  //p_decoded->i_component_type_flag = (p_descriptor->p_data[0] & AM_ENAC3_CMP_FLAG) ? 1 : 0;
  p_decoded->i_bsid_flag = (p_descriptor->p_data[0] & AM_ENAC3_BSID_FLAG) ? 1 : 0;
  p_decoded->i_mainid_flag = (p_descriptor->p_data[0] & AM_ENAC3_MAINID_FLAG) ? 1 : 0;
  p_decoded->i_asvc_flag = (p_descriptor->p_data[0] & AM_ENAC3_ASVC_FLAG) ? 1 : 0;
  p_decoded->i_mix_info_exist = (p_descriptor->p_data[0] & AM_ENAC3_MIX_EXIST) ? 1 : 0;
  p_decoded->i_sub_stream1_flag = (p_descriptor->p_data[0] & AM_ENAC3_SUB1_FLAG) ? 1 : 0;
  p_decoded->i_sub_stream2_flag = (p_descriptor->p_data[0] & AM_ENAC3_SUB2_FLAG) ? 1 : 0;
  p_decoded->i_sub_stream3_flag = (p_descriptor->p_data[0] & AM_ENAC3_SUB3_FLAG) ? 1 : 0;

  /*0100 0000 1bit*/
  p_decoded->i_full_service_flag = (p_descriptor->p_data[1] & 0x40) ? 1 : 0;
  /*0011 1000 3bit*/
  p_decoded->i_audio_service_type = (p_descriptor->p_data[1] & 0x38) >> 3;
  /*0000 0111 3bit*/
  p_decoded->i_number_channels = p_descriptor->p_data[1] & 0x07;
  /*1000 0000 1bit*/
  p_decoded->i_lang_flag_1 = p_descriptor->p_data[2] & 0x80 ? 1 : 0;
  /*0100 0000 1bit*/
  p_decoded->i_lang_flag_2 = p_descriptor->p_data[2] & 0x40 ? 1 : 0;
  AM_DEBUG(1, "dr_cc decoder length (%d)p_decoded->i_mainid_flag(%d)asvc[%d]sub[%d]sub[%d]sub[%d]lang[%d]lang[%d]",
                     p_descriptor->i_length, p_decoded->i_mainid_flag,
                     p_decoded->i_asvc_flag, p_decoded->i_sub_stream1_flag,
                     p_decoded->i_sub_stream2_flag, p_decoded->i_sub_stream3_flag,
                     p_decoded->i_lang_flag_1, p_decoded->i_lang_flag_2
                     );
  int limit_len = p_decoded->i_mainid_flag
                  + p_decoded->i_asvc_flag
                  + p_decoded->i_sub_stream1_flag * 4
                  + p_decoded->i_sub_stream2_flag * 4
                  + p_decoded->i_sub_stream3_flag * 4
                  + p_decoded->i_lang_flag_1 * 3
                  + p_decoded->i_lang_flag_2 * 3;

  if (p_descriptor->i_length < limit_len + 3)
  {
    DVBPSI_ERROR_ARG("dr_cc decoder", "bad length (%d)",
                     p_descriptor->i_length);
    AM_DEBUG(1, "dr_cc decoder bad length (%d)limit_len(%d)",
                     p_descriptor->i_length, limit_len);
    free(p_decoded);
    return NULL;
  }

  int i_pos = 2;
  /*0001 1111 5bit*/
  if (p_decoded->i_bsid_flag) {
   p_decoded->i_bsid = p_descriptor->p_data[i_pos] & 0x1f;
   if (DEBUG_DRCC)
     AM_DEBUG(1, "dr_cc decoder i_bsid (%d)", p_decoded->i_bsid);
  }

  if (p_decoded->i_mainid_flag)
  {
    i_pos = i_pos +1;
    /*0001 1000 2bit*/
    p_decoded->i_priority = p_descriptor->p_data[i_pos] & 0x18;
    /*0000 0111 3bit*/
    p_decoded->i_mainid = p_descriptor->p_data[i_pos] & 0x07;
    if (DEBUG_DRCC)
      AM_DEBUG(1, "dr_cc decoder i_mainid (%d)", p_decoded->i_mainid);
  }
  if (p_decoded->i_asvc_flag)
  {
    i_pos = i_pos +1;
    p_decoded->i_asvc = p_descriptor->p_data[i_pos];
    if (DEBUG_DRCC)
      AM_DEBUG(1, "dr_cc decoder i_asvc (%d)", p_decoded->i_asvc);
  }
  if (p_decoded->i_sub_stream1_flag)
  {
    i_pos = i_pos +1;
    p_decoded->i_sub_stream1 = p_descriptor->p_data[i_pos];
    if (DEBUG_DRCC)
      AM_DEBUG(1, "dr_cc decoder i_sub_stream1 (%d)", p_decoded->i_sub_stream1);
  }
  if (p_decoded->i_sub_stream2_flag)
  {
    i_pos = i_pos +1;
    p_decoded->i_sub_stream2 = p_descriptor->p_data[i_pos];
    if (DEBUG_DRCC)
      AM_DEBUG(1, "dr_cc decoder i_sub_stream2 (%d)", p_decoded->i_sub_stream2);
  }
  if (p_decoded->i_sub_stream3_flag)
  {
    i_pos = i_pos +1;
    p_decoded->i_sub_stream3 = p_descriptor->p_data[i_pos];
    if (DEBUG_DRCC)
      AM_DEBUG(1, "dr_cc decoder i_sub_stream3 (%d)", p_decoded->i_sub_stream3);
  }

  if (p_decoded->i_lang_flag_1)
  {
    i_pos = i_pos +1;
    memcpy(p_decoded->i_lang1, &p_descriptor->p_data[i_pos], 3);
    i_pos = i_pos +2;
    if (DEBUG_DRCC)
      AM_DEBUG(1, "dr_cc decoder i_lang1 (%s)", p_decoded->i_lang1);
  }
  if (p_decoded->i_lang_flag_2)
  {
    i_pos = i_pos +1;
    memcpy(p_decoded->i_lang2, &p_descriptor->p_data[i_pos], 3);
    i_pos = i_pos +2;
    if (DEBUG_DRCC)
      AM_DEBUG(1, "dr_cc decoder i_lang2 (%s)", p_decoded->i_lang2);
  }

  if (p_decoded->i_sub_stream1_flag)
  {
    i_pos = i_pos +1;
    memcpy(p_decoded->i_substream1_lang, &p_descriptor->p_data[i_pos], 3);
    i_pos = i_pos +2;
    if (DEBUG_DRCC)
      AM_DEBUG(1, "dr_cc decoder i_substream1_lang (%s)", p_decoded->i_substream1_lang);
  }
  if (p_decoded->i_sub_stream2_flag)
  {
    i_pos = i_pos +1;
    memcpy(p_decoded->i_substream2_lang, &p_descriptor->p_data[i_pos], 3);
    i_pos = i_pos +2;
    if (DEBUG_DRCC)
      AM_DEBUG(1, "dr_cc decoder i_substream2_lang (%s)", p_decoded->i_substream2_lang);
  }
  if (p_decoded->i_sub_stream3_flag)
  {
    i_pos = i_pos +1;
    memcpy(p_decoded->i_substream3_lang, &p_descriptor->p_data[i_pos], 3);
    i_pos = i_pos +2;
    if (DEBUG_DRCC)
      AM_DEBUG(1, "dr_cc decoder i_substream3_lang (%s)", p_decoded->i_substream3_lang);
  }
  p_descriptor->p_decoded = (void *)p_decoded;
  return p_decoded;
}