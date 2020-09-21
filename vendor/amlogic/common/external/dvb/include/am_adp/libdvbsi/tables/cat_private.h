/*****************************************************************************
 * cat_private.h: private CAT structures
 *----------------------------------------------------------------------------
 * (c)2001-2007 VideoLAN
 * $Id: cat_private.h 145 2007-10-05 17:53:35Z jpsaman $
 *
 * Authors: Johann Hanne
 *          heavily based on pmt_private.h which was written by
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

#ifndef _DVBPSI_CAT_PRIVATE_H_
#define _DVBPSI_CAT_PRIVATE_H_

/*****************************************************************************
 * dvbpsi_cat_decoder_t
 *****************************************************************************
 * CAT decoder.
 *****************************************************************************/
typedef struct dvbpsi_cat_decoder_s
{
  dvbpsi_cat_callback           pf_callback;
  void *                        p_cb_data;

  dvbpsi_cat_t                  current_cat;
  dvbpsi_cat_t *                p_building_cat;

  int                           b_current_valid;

  uint8_t                       i_last_section_number;
  dvbpsi_psi_section_t *        ap_sections [256];

} dvbpsi_cat_decoder_t;


/*****************************************************************************
 * dvbpsi_GatherCATSections
 *****************************************************************************
 * Callback for the PSI decoder.
 *****************************************************************************/
void dvbpsi_GatherCATSections(dvbpsi_decoder_t* p_decoder,
                              dvbpsi_psi_section_t* p_section);


/*****************************************************************************
 * dvbpsi_DecodeCATSections
 *****************************************************************************
 * CAT decoder.
 *****************************************************************************/
void dvbpsi_DecodeCATSections(dvbpsi_cat_t* p_cat,
                              dvbpsi_psi_section_t* p_section);


#else
#error "Multiple inclusions of cat_private.h"
#endif

