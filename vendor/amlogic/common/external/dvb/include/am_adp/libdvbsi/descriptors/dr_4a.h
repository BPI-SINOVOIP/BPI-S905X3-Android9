/*****************************************************************************
 * dr_48.h
 * (c)2001-2002 VideoLAN
 * $Id: dr_48.h 88 2004-02-24 14:31:18Z sam $
 *
 * Authors: Arnaud de Bossoreille de Ribou <bozo@via.ecp.fr>
 *          Johan Bilien <jobi@via.ecp.fr>
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
 *****************************************************************************/

/*!
 * \file <dr_4a.h>
 * \author Johan Bilien <jobi@via.ecp.fr>
 * \brief Application interface for the DVB "linkage"
 * descriptor decoder and generator.
 *
 * Application interface for the DVB "linkage" descriptor
 * decoder and generator. This descriptor's definition can be found in
 * ETSI EN 300 468 section 6.2.30.
 */

#ifndef _DVBPSI_DR_4a_H_
#define _DVBPSI_DR_4a_H_

#ifdef __cplusplus
extern "C" {
#endif


/*****************************************************************************
 * dvbpsi_linkage_dr_t
 *****************************************************************************/
/*!
 * \struct dvbpsi_linkage_dr_s
 * \brief "linkage" descriptor structure.
 *
 * This structure is used to store a decoded "linkage"
 * descriptor. (ETSI EN 300 468 section 6.2.30).
 */
/*!
 * \typedef struct dvbpsi_linkage_dr_s dvbpsi_linkage_dr_t
 * \brief dvbpsi_linkage_dr_t type definition.
 */
typedef struct dvbpsi_linkage_dr_s
{
  uint16_t     i_network_id;           /*!< original network id */
  uint16_t     i_ts_id;           	   /*!< ts id */
  uint16_t     i_service_id;           /*!< service id */
  uint8_t      i_linkage_type;         /*!< linkage type */
  uint8_t      *i_prv_data;			   /*!< Private data */
} dvbpsi_linkage_dr_t;


/*****************************************************************************
 * dvbpsi_DecodeLinkageDataDr
 *****************************************************************************/
/*!
 * \fn dvbpsi_linkage_dr_t * dvbpsi_DecodeLinkageDr(
                                        dvbpsi_descriptor_t * p_descriptor)
 * \brief "linkage" descriptor decoder.
 * \param p_descriptor pointer to the descriptor structure
 * \return a pointer to a new "linkage" descriptor structure
 * which contains the decoded data.
 */
dvbpsi_linkage_dr_t* dvbpsi_DecodeLinkageDr(
                                        dvbpsi_descriptor_t * p_descriptor);



#ifdef __cplusplus
};
#endif

#else
#error "Multiple inclusions of dr_48.h"
#endif

