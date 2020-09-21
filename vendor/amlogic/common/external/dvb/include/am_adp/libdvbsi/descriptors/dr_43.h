/*****************************************************************************
 * dr_43.h
 * (c)2001-2008 VideoLAN
 * $Id: dr_43.h 171 2008-04-26 12:09:45Z jpsaman $
 *
 * Authors: Johann Hanne
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
 * \file <dr_43.h>
 * \author Johann Hanne
 * \brief Application interface for the DVB satellite delivery system
 *        descriptor decoder and generator.
 *
 * Application interface for the DVB satellite delivery system descriptor
 * decoder and generator. This descriptor's definition can be found in
 * ETSI EN 300 468 section 6.2.13.2.
 */

#ifndef _DVBPSI_DR_43_H_
#define _DVBPSI_DR_43_H_

#ifdef __cplusplus
extern "C" {
#endif


/*****************************************************************************
 * dvbpsi_sat_deliv_sys_dr_t
 *****************************************************************************/
/*!
 * \struct dvbpsi_sat_deliv_sys_dr_s
 * \brief satellite delivery system descriptor structure.
 *
 * This structure is used to store a decoded satellite delivery system
 * descriptor. (ETSI EN 300 468 section 6.2.13.2).
 */
/*!
 * \typedef struct dvbpsi_sat_deliv_sys_dr_s dvbpsi_sat_deliv_sys_dr_t
 * \brief dvbpsi_sat_deliv_sys_dr_t type definition.
 */
typedef struct dvbpsi_sat_deliv_sys_dr_s
{
  uint32_t     i_frequency;                         /*!< frequency */
  uint16_t     i_orbital_position;                  /*!< orbital position */
  uint8_t      i_west_east_flag;                    /*!< west east flag */
  uint8_t      i_polarization;                      /*!< polarization */
  uint8_t      i_roll_off;                          /*!< roll off */
  uint8_t      i_modulation_system;                 /*!< modulation system */
  uint8_t      i_modulation_type;                   /*!< modulation type */
  uint32_t     i_symbol_rate;                       /*!< symbol rate */
  uint8_t      i_fec_inner;                         /*!< FEC inner */

} dvbpsi_sat_deliv_sys_dr_t;


/*****************************************************************************
 * dvbpsi_DecodeSatDelivSysDr
 *****************************************************************************/
/*!
 * \fn dvbpsi_sat_deliv_sys_dr_t * dvbpsi_DecodeSatDelivSysDr(
                                        dvbpsi_descriptor_t * p_descriptor)
 * \brief satellite delivery system descriptor decoder.
 * \param p_descriptor pointer to the descriptor structure
 * \return a pointer to a new satellite delivery system descriptor structure
 * which contains the decoded data.
 */
dvbpsi_sat_deliv_sys_dr_t* dvbpsi_DecodeSatDelivSysDr(
                                        dvbpsi_descriptor_t * p_descriptor);


/*****************************************************************************
 * dvbpsi_GenSatDelivSysDr
 *****************************************************************************/
/*!
 * \fn dvbpsi_descriptor_t * dvbpsi_GenSatDelivSysDr(
                        dvbpsi_sat_deliv_sys_dr_t * p_decoded, int b_duplicate)
 * \brief satellite delivery system descriptor generator.
 * \param p_decoded pointer to a decoded satellite delivery system descriptor
 * descriptor structure
 * \param b_duplicate if non zero then duplicate the p_decoded structure into
 * the descriptor
 * \return a pointer to a new descriptor structure which contains encoded data.
 */
dvbpsi_descriptor_t * dvbpsi_GenSatDelivSysDr(
                                        dvbpsi_sat_deliv_sys_dr_t * p_decoded,
                                        int b_duplicate);


#ifdef __cplusplus
};
#endif

#else
#error "Multiple inclusions of dr_43.h"
#endif
