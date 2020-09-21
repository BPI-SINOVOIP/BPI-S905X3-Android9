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
/**\file  dr_7a.c
 * \brief AMLogic descriptor 6a parse
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

#include "dr_7a.h"

/*****************************************************************************
 * dvbpsi_DecodeENAC3Dr
 *****************************************************************************/
dvbpsi_ENAC3_dr_t * dvbpsi_DecodeENAC3Dr(dvbpsi_descriptor_t * p_descriptor)
{
  dvbpsi_ENAC3_dr_t * p_decoded;
  AM_DEBUG(1, "dr_7a dvbpsi_DecodeENAC3Dr ");
  /* Check the tag */
  if (p_descriptor->i_tag != 0x7a)
  {
    DVBPSI_ERROR_ARG("dr_7a decoder", "bad tag (0x%x)", p_descriptor->i_tag);
    AM_DEBUG(1, "dr_7a decoder bad tag (0x%x)",p_descriptor->i_tag);
    return NULL;
  }

  /* Don't decode twice */
  if (p_descriptor->p_decoded)
    return p_descriptor->p_decoded;

  /* Allocate memory */
  p_decoded =
        (dvbpsi_ENAC3_dr_t *)malloc(sizeof(dvbpsi_ENAC3_dr_t));
  if (!p_decoded)
  {
    DVBPSI_ERROR("dr_7a decoder", "out of memory");
    AM_DEBUG(1, "dr_7a decoder out of memory");
    return NULL;
  }
  memset(p_decoded, 0, sizeof(dvbpsi_ENAC3_dr_t));
  /* Decode data and check the length */
  p_decoded->i_component_type_flag = (p_descriptor->p_data[0] & AM_ENAC3_CMP_FLAG) ? 1 : 0;
  p_decoded->i_bsid_flag = (p_descriptor->p_data[0] & AM_ENAC3_BSID_FLAG) ? 1 : 0;
  p_decoded->i_mainid_flag = (p_descriptor->p_data[0] & AM_ENAC3_MAINID_FLAG) ? 1 : 0;
  p_decoded->i_asvc_flag = (p_descriptor->p_data[0] & AM_ENAC3_ASVC_FLAG) ? 1 : 0;
  p_decoded->i_mix_info_exist = (p_descriptor->p_data[0] & AM_ENAC3_MIX_EXIST) ? 1 : 0;
  p_decoded->i_sub_stream1_flag = (p_descriptor->p_data[0] & AM_ENAC3_SUB1_FLAG) ? 1 : 0;
  p_decoded->i_sub_stream2_flag = (p_descriptor->p_data[0] & AM_ENAC3_SUB2_FLAG) ? 1 : 0;
  p_decoded->i_sub_stream3_flag = (p_descriptor->p_data[0] & AM_ENAC3_SUB3_FLAG) ? 1 : 0;

  int limit_len = p_decoded->i_component_type_flag
                  +p_decoded->i_bsid_flag
                  +p_decoded->i_mainid_flag
                  +p_decoded->i_asvc_flag
                  +p_decoded->i_sub_stream1_flag
                  +p_decoded->i_sub_stream2_flag
                  +p_decoded->i_sub_stream3_flag;

  if (p_descriptor->i_length < limit_len+1)
  {
    DVBPSI_ERROR_ARG("dr_7a decoder", "bad length (%d)",
                     p_descriptor->i_length);
    AM_DEBUG(1, "dr_7a decoder bad length (%d)",
                     p_descriptor->i_length);
    free(p_decoded);
    return NULL;
  }

  int i_pos = 0;

  if (p_decoded->i_component_type_flag)
  {
    i_pos = i_pos +1;
    p_decoded->i_component_type = p_descriptor->p_data[i_pos];
  }
  if (p_decoded->i_bsid_flag)
  {
    i_pos = i_pos +1;
    p_decoded->i_bsid = p_descriptor->p_data[i_pos];
  }
  if (p_decoded->i_mainid_flag)
  {
    i_pos = i_pos +1;
    p_decoded->i_mainid = p_descriptor->p_data[i_pos];
  }
  if (p_decoded->i_asvc_flag)
  {
    i_pos = i_pos +1;
    p_decoded->i_asvc = p_descriptor->p_data[i_pos];
  }
  if (p_decoded->i_sub_stream1_flag)
  {
    i_pos = i_pos +1;
    p_decoded->i_sub_stream1 = p_descriptor->p_data[i_pos];
  }
  if (p_decoded->i_sub_stream2_flag)
  {
    i_pos = i_pos +1;
    p_decoded->i_sub_stream2 = p_descriptor->p_data[i_pos];
  }
  if (p_decoded->i_sub_stream3_flag)
  {
    i_pos = i_pos +1;
    p_decoded->i_sub_stream3 = p_descriptor->p_data[i_pos];
  }
  p_descriptor->p_decoded = (void *)p_decoded;
  return p_decoded;
}