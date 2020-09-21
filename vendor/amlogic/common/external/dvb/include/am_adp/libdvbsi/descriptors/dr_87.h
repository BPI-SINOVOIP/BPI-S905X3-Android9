/*****************************************************************************
 * dr_87.h
 * (c)2001-2008 VideoLAN
 * $Id: dr_58.h 172 2008-04-26 12:10:54Z jpsaman $
 *
 * Authors: Gong Ke
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
 * \file <dr_87.h>
 * \author Gong Ke
 * \brief Application interface for the logical channel number
 * descriptor decoder and generator.
 *
 * Application interface for the "logical channel number" descriptor
 * decoder and generator.
 */

#ifndef _DVBPSI_DR_87_H_
#define _DVBPSI_DR_87_H_

#ifdef __cplusplus
extern "C" {
#endif


/*****************************************************************************
 * dvbpsi_logical_channel_number_87_t
 *****************************************************************************/
/*!
 * \struct dvbpsi_logical_channel_number_87_s
 * \brief one logical channel number structure.
 *
 */
/*!
 * \typedef struct dvbpsi_logical_channel_number_87_s dvbpsi_logical_channel_number_87_t
 * \brief dvbpsi_logical_channel_number_87_t type definition.
 */
typedef struct dvbpsi_logical_channel_number_87_s
{
  uint16_t      i_service_id;                 /*!< service_id */
  uint8_t       i_visible_service_flag;       /*!< visible service flag */
  uint16_t      i_logical_channel_number;     /*!< logical channel number*/
} dvbpsi_logical_channel_number_87_t;

/*****************************************************************************
 * dvbpsi_logical_channel_list_87_t
 *****************************************************************************/
/*!
 * \struct dvbpsi_logical_channel_list_87_s
 * \brief one logical channel list structure.
 *
 */
/*!
 * \typedef struct dvbpsi_logical_channel_list_87_s dvbpsi_logical_channel_list_87_t
 * \brief dvbpsi_logical_channel_number_87_t type definition.
 */
typedef struct dvbpsi_logical_channel_list_87_s
{
  uint8_t       i_channel_list_id;
  uint8_t       i_channel_list_name_length;
  uint8_t       i_channel_list_name[249];
  uint8_t       i_country_code[3];
  uint8_t       i_logical_channel_numbers_number;
  dvbpsi_logical_channel_number_87_t p_logical_channel_number[62];
} dvbpsi_logical_channel_list_87_t;


/*****************************************************************************
 * dvbpsi_logical_channel_number_87_dr_t
 *****************************************************************************/
/*!
 * \struct dvbpsi_logical_channel_number_87_dr_s
 * \brief "logical channel number" descriptor structure.
 *
 * This structure is used to store a decoded "logical channel number"
 * descriptor.
 */
/*!
 * \typedef struct dvbpsi_logical_channel_number_87_dr_s dvbpsi_logical_channel_number_87_dr_t
 * \brief dvbpsi_logical_channel_number_87_dr_t type definition.
 */
typedef struct dvbpsi_logical_channel_number_87_dr_s
{
  uint8_t      i_logical_channel_lists_number;
  dvbpsi_logical_channel_list_87_t   p_logical_channel_list[42];
} dvbpsi_logical_channel_number_87_dr_t;


/*****************************************************************************
 * dvbpsi_DecodeLogicalChannelNumberDr
 *****************************************************************************/
/*!
 * \fn dvbpsi_logical_channel_number_87_dr_t * dvbpsi_DecodeLogicalChannelNumber87Dr(
                                        dvbpsi_descriptor_t * p_descriptor)
 * \brief "logical channel number" descriptor decoder.
 * \param p_descriptor pointer to the descriptor structure
 * \return a pointer to a new "logical channel number" descriptor structure
 * which contains the decoded data.
 */
dvbpsi_logical_channel_number_87_dr_t* dvbpsi_DecodeLogicalChannelNumber87Dr(
                                        dvbpsi_descriptor_t * p_descriptor);


/*****************************************************************************
 * dvbpsi_GenLogicalChannelNumber87Dr
 *****************************************************************************/
/*!
 * \fn dvbpsi_descriptor_t * dvbpsi_GenLogicalChannelNumber87Dr(
                        dvbpsi_logical_channel_number_87_dr_t * p_decoded, int b_duplicate)
 * \brief "logical channel number" descriptor generator.
 * \param p_decoded pointer to a decoded "logical channel number" descriptor
 * structure
 * \param b_duplicate if non zero then duplicate the p_decoded structure into
 * the descriptor
 * \return a pointer to a new descriptor structure which contains encoded data.
 */
dvbpsi_descriptor_t * dvbpsi_GenLogicalChannelNumber87Dr(
                                        dvbpsi_logical_channel_number_87_dr_t * p_decoded,
                                        int b_duplicate);


#ifdef __cplusplus
};
#endif

#else
#error "Multiple inclusions of dr_58.h"
#endif
