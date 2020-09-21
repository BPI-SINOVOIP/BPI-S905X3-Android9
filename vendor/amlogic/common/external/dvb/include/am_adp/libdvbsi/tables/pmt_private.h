/*****************************************************************************
 * pmt_private.h: private PMT structures
 *----------------------------------------------------------------------------
 * (c)2001-2002 VideoLAN
 * $Id: pmt_private.h 88 2004-02-24 14:31:18Z sam $
 *
 * Authors: Arnaud de Bossoreille de Ribou <bozo@via.ecp.fr>
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

#ifndef _DVBPSI_PMT_PRIVATE_H_
#define _DVBPSI_PMT_PRIVATE_H_

/*****************************************************************************
 * dvbpsi_pmt_decoder_t
 *****************************************************************************
 * PMT decoder.
 *****************************************************************************/
typedef struct dvbpsi_pmt_decoder_s
{
  uint16_t                      i_program_number;

  dvbpsi_pmt_callback           pf_callback;
  void *                        p_cb_data;

  dvbpsi_pmt_t                  current_pmt;
  dvbpsi_pmt_t *                p_building_pmt;

  int                           b_current_valid;

  uint8_t                       i_last_section_number;
  dvbpsi_psi_section_t *        ap_sections [256];

} dvbpsi_pmt_decoder_t;


/*****************************************************************************
 * dvbpsi_GatherPMTSections
 *****************************************************************************
 * Callback for the PSI decoder.
 *****************************************************************************/
void dvbpsi_GatherPMTSections(dvbpsi_decoder_t* p_decoder,
                              dvbpsi_psi_section_t* p_section);


/*****************************************************************************
 * dvbpsi_DecodePMTSections
 *****************************************************************************
 * PMT decoder.
 *****************************************************************************/
void dvbpsi_DecodePMTSections(dvbpsi_pmt_t* p_pmt,
                              dvbpsi_psi_section_t* p_section);


#else
#error "Multiple inclusions of pmt_private.h"
#endif

