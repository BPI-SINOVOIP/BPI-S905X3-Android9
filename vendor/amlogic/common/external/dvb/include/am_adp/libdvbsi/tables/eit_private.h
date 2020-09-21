/*****************************************************************************
 * eit_private.h: private EIT structures
 *----------------------------------------------------------------------------
 * (c)2004 VideoLAN
 * $Id: eit_private.h 88 2004-02-24 14:31:18Z sam $
 *
 * Authors: Christophe Massiot <massiot@via.ecp.fr>
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

#ifndef _DVBPSI_EIT_PRIVATE_H_
#define _DVBPSI_EIT_PRIVATE_H_


/*****************************************************************************
 * dvbpsi_eit_decoder_t
 *****************************************************************************
 * EIT decoder.
 *****************************************************************************/
typedef struct dvbpsi_eit_decoder_s
{
  dvbpsi_eit_callback           pf_callback;
  void *                        p_cb_data;

  dvbpsi_eit_t                  current_eit;
  dvbpsi_eit_t *                p_building_eit;

  int                           b_current_valid;

  uint8_t                       i_last_section_number;
  uint8_t                       i_first_received_section_number;
  dvbpsi_psi_section_t *        ap_sections [256];

} dvbpsi_eit_decoder_t;


/*****************************************************************************
 * dvbpsi_GatherEITSections
 *****************************************************************************
 * Callback for the PSI decoder.
 *****************************************************************************/
void dvbpsi_GatherEITSections(dvbpsi_decoder_t* p_psi_decoder,
		              void* p_private_decoder,
                              dvbpsi_psi_section_t* p_section);


/*****************************************************************************
 * dvbpsi_DecodeEITSection
 *****************************************************************************
 * EIT decoder.
 *****************************************************************************/
void dvbpsi_DecodeEITSections(dvbpsi_eit_t* p_eit,
                              dvbpsi_psi_section_t* p_section);


#else
#error "Multiple inclusions of eit_private.h"
#endif

