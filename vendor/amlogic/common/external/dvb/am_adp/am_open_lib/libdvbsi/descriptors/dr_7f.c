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
/**\file  dr_7f.c
 * \brief AMLogic descriptor 7f parse
 *
 * \author chen hua ling <hualing.chen@amlogic.com>
 * \date 2017-03-16: create the document
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

#include "dr_7f.h"


/*****************************************************************************
 * dvbpsi_Decode_Exten_Sup_Audio_Dr
 *****************************************************************************/
int dvbpsi_Decode_Exten_Sup_Audio_Dr(dvbpsi_EXTENTION_dr_t * p_decoded, uint8_t * p_data, uint8_t i_length)
{
    AM_DEBUG(1, "dr_7f dvbpsi_Decode_Exten_Sup_Audio_Dr ");
  /*1 5 1 1 8bit mix:1 edit:5 re:1 lang:1*/
  p_decoded->exten_t.sup_audio.mix_type = (p_data[1]&0x80)>7; /*1000 0000*/
  p_decoded->exten_t.sup_audio.editorial_classification = (p_data[1]&0x7C)>2; /*0111 1100*/
  p_decoded->exten_t.sup_audio.lang_code = p_data[1]&0x01; /*0000 0001*/
  if (p_decoded->exten_t.sup_audio.lang_code == 1 && i_length >= 5) {
    p_decoded->exten_t.sup_audio.iso_639_lang_code[0] = p_data[2];
    p_decoded->exten_t.sup_audio.iso_639_lang_code[1] = p_data[3];
    p_decoded->exten_t.sup_audio.iso_639_lang_code[2] = p_data[4];
  }
  /* Convert to lower case if possible */
  dvbpsi_ToLower(p_decoded->exten_t.sup_audio.iso_639_lang_code, 3);
  AM_DEBUG(1, "dr_7f dvbpsi_Decode_Exten_Sup_Audio_Dr: mix:%d edit:%d lang:%d",p_decoded->exten_t.sup_audio.mix_type,p_decoded->exten_t.sup_audio.editorial_classification,p_decoded->exten_t.sup_audio.lang_code );
  return 0;
}

/*****************************************************************************
 * dvbpsi_DecodeEXTENTIONDr
 *****************************************************************************/
dvbpsi_EXTENTION_dr_t * dvbpsi_DecodeEXTENTIONDr(dvbpsi_descriptor_t * p_descriptor)
{
  dvbpsi_EXTENTION_dr_t * p_decoded;
  AM_DEBUG(1, "dr_7f dvbpsi_DecodeEXTENTIONDr ");
  /* Check the tag */
  if (p_descriptor->i_tag != 0x7f)
  {
    DVBPSI_ERROR_ARG("dr_7f decoder", "bad tag (0x%x)", p_descriptor->i_tag);
    AM_DEBUG(1, "dr_7f decoder bad tag (0x%x)",p_descriptor->i_tag);
    return NULL;
  }

  /* Don't decode twice */
  if (p_descriptor->p_decoded)
    return p_descriptor->p_decoded;

  /* Allocate memory */
  p_decoded =
        (dvbpsi_EXTENTION_dr_t *)malloc(sizeof(dvbpsi_EXTENTION_dr_t));
  if (!p_decoded)
  {
    DVBPSI_ERROR("dr_7f decoder", "out of memory");
    AM_DEBUG(1, "dr_7f decoder out of memory");
    return NULL;
  }

  /* Decode data and check the length */
  p_decoded->i_extern_des_tag = (p_descriptor->p_data[0]);

  switch (p_decoded->i_extern_des_tag)
  {
    case AM_SI_EXTEN_DESCR_IMAGE_ICON:
      AM_DEBUG(1, "dr_7f exten tag DESCR_IMAGE_ICON ");
      break;
    case AM_SI_EXTEN_DESCR_CPCM_DELIVERY_SIGNAL:
      AM_DEBUG(1, "dr_7f exten tag AM_SI_EXTEN_DESCR_CPCM_DELIVERY_SIGNAL ");
      break;
    case AM_SI_EXTEN_DESCR_CP:
      AM_DEBUG(1, "dr_7f exten tag AM_SI_EXTEN_DESCR_CP ");
      break;
    case AM_SI_EXTEN_DESCR_CP_IDENTIFITER:
      AM_DEBUG(1, "dr_7f exten tag AM_SI_EXTEN_DESCR_CP_IDENTIFITER ");
      break;
    case AM_SI_EXTEN_DESCR_T2_DELIVERY_SYS:
      AM_DEBUG(1, "dr_7f exten tag AM_SI_EXTEN_DESCR_T2_DELIVERY_SYS ");
      break;
    case AM_SI_EXTEN_DESCR_SH_DELIVERY_SYS:
      AM_DEBUG(1, "dr_7f exten tag AM_SI_EXTEN_DESCR_SH_DELIVERY_SYS ");
      break;
    case AM_SI_EXTEN_DESCR_SUP_AUDIO:
      AM_DEBUG(1, "dr_7f exten tag AM_SI_EXTEN_DESCR_SUP_AUDIO ");
      dvbpsi_Decode_Exten_Sup_Audio_Dr(p_decoded, p_descriptor->p_data, p_descriptor->i_length);
      break;
    case AM_SI_EXTEN_DESCR_NETWORK_CHANGE_NOTIFY:
      AM_DEBUG(1, "dr_7f exten tag AM_SI_EXTEN_DESCR_NETWORK_CHANGE_NOTIFY ");
      break;
    case AM_SI_EXTEN_DESCR_MESSAGE:
      AM_DEBUG(1, "dr_7f exten tag AM_SI_EXTEN_DESCR_MESSAGE ");
      break;
    case AM_SI_EXTEN_DESCR_TARGET_REGION:
      AM_DEBUG(1, "dr_7f exten tag AM_SI_EXTEN_DESCR_TARGET_REGION ");
      break;
    case AM_SI_EXTEN_DESCR_TARGET_REGION_NAME:
      AM_DEBUG(1, "dr_7f exten tag AM_SI_EXTEN_DESCR_TARGET_REGION_NAME ");
      break;
    case AM_SI_EXTEN_DESCR_SERVICE_RELOCATED:
      AM_DEBUG(1, "dr_7f exten tag AM_SI_EXTEN_DESCR_SERVICE_RELOCATED ");
      break;
    case AM_SI_EXTEN_DESCR_XAIT_PID:
      AM_DEBUG(1, "dr_7f exten tag AM_SI_EXTEN_DESCR_XAIT_PID ");
      break;
    case AM_SI_EXTEN_DESCR_C2_DELIVERY:
      AM_DEBUG(1, "dr_7f exten tag AM_SI_EXTEN_DESCR_C2_DELIVERY ");
      break;
    case AM_SI_EXTEN_DESCR_DTSHD_AUDIO:
      AM_DEBUG(1, "dr_7f exten tag AM_SI_EXTEN_DESCR_DTSHD_AUDIO ");
      break;
    case AM_SI_EXTEN_DESCR_DTS_NEURAL:
      AM_DEBUG(1, "dr_7f exten tag AM_SI_EXTEN_DESCR_DTS_NEURAL ");
      break;
    case AM_SI_EXTEN_DESCR_VIDEO_DEPTH_RANGE:
      AM_DEBUG(1, "dr_7f exten tag AM_SI_EXTEN_DESCR_VIDEO_DEPTH_RANGE ");
      break;
    case AM_SI_EXTEN_DESCR_T2MI:
      AM_DEBUG(1, "dr_7f exten tag AM_SI_EXTEN_DESCR_T2MI ");
      break;
    case AM_SI_EXTEN_DESCR_URI_LINKAGE:
      AM_DEBUG(1, "dr_7f exten tag AM_SI_EXTEN_DESCR_URI_LINKAGE ");
      break;
    case AM_SI_EXTEN_DESCR_BCI_ANCILLARY:
      AM_DEBUG(1, "dr_7f exten tag AM_SI_EXTEN_DESCR_BCI_ANCILLARY ");
      break;
    case AM_SI_EXTEN_DESCR_OTHER:
      AM_DEBUG(1, "dr_7f exten tag AM_SI_EXTEN_DESCR_OTHER ");
      break;
    default:
      AM_DEBUG(1, "Scan: Unkown exten tag data, tag 0x%x", p_descriptor->p_data[0]);
      p_decoded->i_extern_des_tag = AM_SI_EXTEN_DESCR_OTHER;
      break;
  }

  p_descriptor->p_decoded = (void *)p_decoded;
  return p_decoded;
}