/*****************************************************************************
 * tot_private.h: private TDT/TOT structures
 *----------------------------------------------------------------------------
 * (c)2001-2008 VideoLAN
 * $Id: tot_private.h 178 2008-07-14 09:03:13Z sam $
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

#ifndef _DVBPSI_TOT_PRIVATE_H_
#define _DVBPSI_TOT_PRIVATE_H_

/*****************************************************************************
 * dvbpsi_tot_decoder_t
 *****************************************************************************
 * TDT/TOT decoder.
 *****************************************************************************/
typedef struct dvbpsi_tot_decoder_s
{
  dvbpsi_tot_callback           pf_callback;
  void *                        p_cb_data;

} dvbpsi_tot_decoder_t;


/*****************************************************************************
 * dvbpsi_GatherTOTSections
 *****************************************************************************
 * Callback for the PSI decoder.
 *****************************************************************************/
void dvbpsi_GatherTOTSections(dvbpsi_decoder_t* p_decoder,
                              void * p_private_decoder,
                              dvbpsi_psi_section_t* p_section);


/*****************************************************************************
 * dvbpsi_ValidTOTSection
 *****************************************************************************
 * Check the CRC_32 if the section has b_syntax_indicator set.
 *****************************************************************************/
int dvbpsi_ValidTOTSection(dvbpsi_psi_section_t* p_section);

/*****************************************************************************
 * dvbpsi_DecodeTOTSections
 *****************************************************************************
 * TDT/TOT decoder.
 *****************************************************************************/
void dvbpsi_DecodeTOTSections(dvbpsi_tot_t* p_tot,
                              dvbpsi_psi_section_t* p_section);


#else
#error "Multiple inclusions of tot_private.h"
#endif
