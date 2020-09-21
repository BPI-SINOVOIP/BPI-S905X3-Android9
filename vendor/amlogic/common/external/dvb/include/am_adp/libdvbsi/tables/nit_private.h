/*****************************************************************************
 * nit_private.h: private NIT structures
 *----------------------------------------------------------------------------
 * (c)2001-2002 VideoLAN
 * $Id: nit_private.h 170 2008-04-24 20:49:16Z jpsaman $
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

#ifndef _DVBPSI_NIT_PRIVATE_H_
#define _DVBPSI_NIT_PRIVATE_H_

/*****************************************************************************
 * dvbpsi_nit_decoder_t
 *****************************************************************************
 * NIT decoder.
 *****************************************************************************/
typedef struct dvbpsi_nit_decoder_s
{
  uint16_t                      i_network_id;

  dvbpsi_nit_callback           pf_callback;
  void *                        p_cb_data;

  dvbpsi_nit_t                  current_nit;
  dvbpsi_nit_t *                p_building_nit;

  int                           b_current_valid;

  uint8_t                       i_last_section_number;
  dvbpsi_psi_section_t *        ap_sections [256];

} dvbpsi_nit_decoder_t;


/*****************************************************************************
 * dvbpsi_GatherNITSections
 *****************************************************************************
 * Callback for the PSI decoder.
 *****************************************************************************/
void dvbpsi_GatherNITSections(dvbpsi_decoder_t* p_psi_decoder,
                              void* p_private_decoder,
                              dvbpsi_psi_section_t* p_section);


/*****************************************************************************
 * dvbpsi_DecodeNITSections
 *****************************************************************************
 * NIT decoder.
 *****************************************************************************/
void dvbpsi_DecodeNITSections(dvbpsi_nit_t* p_nit,
                              dvbpsi_psi_section_t* p_section);


#else
#error "Multiple inclusions of nit_private.h"
#endif

