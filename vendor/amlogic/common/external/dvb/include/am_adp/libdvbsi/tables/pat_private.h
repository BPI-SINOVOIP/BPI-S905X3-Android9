/*****************************************************************************
 * pat_private.h: private PAT structures
 *----------------------------------------------------------------------------
 * (c)2001-2002 VideoLAN
 * $Id: pat_private.h 88 2004-02-24 14:31:18Z sam $
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

#ifndef _DVBPSI_PAT_PRIVATE_H_
#define _DVBPSI_PAT_PRIVATE_H_


/*****************************************************************************
 * dvbpsi_pat_decoder_t
 *****************************************************************************
 * PAT decoder.
 *****************************************************************************/
typedef struct dvbpsi_pat_decoder_s
{
  dvbpsi_pat_callback           pf_callback;
  void *                        p_cb_data;

  dvbpsi_pat_t                  current_pat;
  dvbpsi_pat_t *                p_building_pat;

  int                           b_current_valid;

  uint8_t                       i_last_section_number;
  dvbpsi_psi_section_t *        ap_sections [256];

} dvbpsi_pat_decoder_t;


/*****************************************************************************
 * dvbpsi_GatherPATSections
 *****************************************************************************
 * Callback for the PSI decoder.
 *****************************************************************************/
void dvbpsi_GatherPATSections(dvbpsi_decoder_t* p_decoder,
                              dvbpsi_psi_section_t* p_section);


/*****************************************************************************
 * dvbpsi_DecodePATSection
 *****************************************************************************
 * PAT decoder.
 *****************************************************************************/
void dvbpsi_DecodePATSections(dvbpsi_pat_t* p_pat,
                              dvbpsi_psi_section_t* p_section);


#else
#error "Multiple inclusions of pat_private.h"
#endif

