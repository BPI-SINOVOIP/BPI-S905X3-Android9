/*****************************************************************************
 * dr_54.h
 * (c)2005 Andrew John Hughes
 *
 * Authors: Andrew John Hughes <gnu_andrew@member.fsf.org>
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
 * \file <dr_54.h>
 * \author Andrew John Hughes <gnu_andrew@member.fsf.org>
 * \brief Application interface for the DVB "content"
 * descriptor decoder and generator.
 *
 * Application interface for the DVB "content" descriptor
 * decoder and generator. This descriptor's definition can be found in
 * ETSI EN 300 468 section 6.2.37.
 */

#ifndef _DVBPSI_DR_54_H_
#define _DVBPSI_DR_54_H_

#ifdef __cplusplus
extern "C" {
#endif


/*****************************************************************************
 * dvbpsi_service_dr_t
 *****************************************************************************/
/*!
 * \struct dvbpsi_content_dr_s
 * \brief "content" descriptor structure.
 *
 * This structure is used to store a decoded "content"
 * descriptor. (ETSI EN 300 468 section 6.2.37).
 */
/*!
 * \typedef struct dvbpsi_service_dr_s dvbpsi_content_dr_t
 * \brief dvbpsi_content_dr_t type definition.
 */
typedef struct dvbpsi_content_dr_s
{
  uint8_t      i_nibble_level;             /*!< content_nibble_level, composed by content_nibble_level1(4 bits) and content_nibble_level2 (4 bits)*/
  uint8_t      i_user_byte;			/**!< the user byte defined by broadcaster*/
} dvbpsi_content_dr_t;


/*****************************************************************************
 * dvbpsi_DecodeContentDr
 *****************************************************************************/
/*!
 * \fn dvbpsi_content_dr_t * dvbpsi_DecodeContentDr(
                                        dvbpsi_descriptor_t * p_descriptor)
 * \brief "content" descriptor decoder.
 * \param p_descriptor pointer to the descriptor structure
 * \return a pointer to a new "content" descriptor structure
 * which contains the decoded data.
 */
dvbpsi_content_dr_t* dvbpsi_DecodeContentDr(
						      dvbpsi_descriptor_t * p_descriptor);


/*****************************************************************************
 * dvbpsi_GenContentDr
 *****************************************************************************/
/*!
 * \fn dvbpsi_descriptor_t * dvbpsi_GenContentDr(
                        dvbpsi_service_dr_t * p_decoded, int b_duplicate)
 * \brief "content" descriptor generator.
 * \param p_decoded pointer to a decoded "content" descriptor
 * structure
 * \param b_duplicate if non zero then duplicate the p_decoded structure into
 * the descriptor
 * \return a pointer to a new descriptor structure which contains encoded data.
 */
dvbpsi_descriptor_t * dvbpsi_GenContentDr(
                                        dvbpsi_content_dr_t * p_decoded,
                                        int b_duplicate);


#ifdef __cplusplus
};
#endif

#else
#error "Multiple inclusions of dr_54.h"
#endif

