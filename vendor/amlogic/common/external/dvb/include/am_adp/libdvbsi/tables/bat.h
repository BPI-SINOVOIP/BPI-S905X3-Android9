/*****************************************************************************
 * bat.h
 * (c)2001-2008 VideoLAN
 * $Id: bat.h 187 2009-11-18 07:26:10Z md $
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
 *****************************************************************************/

/*!
 * \file <bat.h>
 * \author Johann Hanne
 * \brief Application interface for the BAT decoder and the BAT generator.
 *
 * Application interface for the BAT decoder and the BAT generator.
 * New decoded BAT tables are sent by callback to the application.
 */

#ifndef _DVBPSI_BAT_H_
#define _DVBPSI_BAT_H_

#ifdef __cplusplus
extern "C" {
#endif


/*****************************************************************************
 * dvbpsi_bat_ts_t
 *****************************************************************************/
/*!
 * \struct dvbpsi_bat_ts_s
 * \brief BAT TS structure.
 *
 * This structure is used to store a decoded TS description.
 * (ETSI EN 300 468 section 5.2.1).
 */
/*!
 * \typedef struct dvbpsi_bat_ts_s dvbpsi_bat_ts_t
 * \brief dvbpsi_bat_ts_t type definition.
 */
typedef struct dvbpsi_bat_ts_s
{
  uint16_t                      i_ts_id;                /*!< transport stream id */
  uint16_t                      i_orig_network_id;      /*!< original network id */

  dvbpsi_descriptor_t *         p_first_descriptor;     /*!< descriptor list */

  struct dvbpsi_bat_ts_s *      p_next;                 /*!< next element of
                                                             the list */

} dvbpsi_bat_ts_t;


/*****************************************************************************
 * dvbpsi_bat_t
 *****************************************************************************/
/*!
 * \struct dvbpsi_bat_s
 * \brief BAT structure.
 *
 * This structure is used to store a decoded BAT.
 * (ETSI EN 300 468 section 5.2.1).
 */
/*!
 * \typedef struct dvbpsi_bat_s dvbpsi_bat_t
 * \brief dvbpsi_bat_t type definition.
 */
typedef struct dvbpsi_bat_s
{
  struct dvbpsi_bat_s 		*p_next;			/*!< bat_list */
  uint8_t 					i_table_id;			/*!< table_id */
  uint16_t                  i_bouquet_id;       /*!< bouquet_id */
  uint8_t                   i_version;          /*!< version_number */
  int                       b_current_next;     /*!< current_next_indicator */

  dvbpsi_descriptor_t *     p_first_descriptor; /*!< descriptor list */

  dvbpsi_bat_ts_t *         p_first_ts;         /*!< TS list */

} dvbpsi_bat_t;


/*****************************************************************************
 * dvbpsi_bat_callback
 *****************************************************************************/
/*!
 * \typedef void (* dvbpsi_bat_callback)(void* p_cb_data,
                                         dvbpsi_bat_t* p_new_bat)
 * \brief Callback type definition.
 */
typedef void (* dvbpsi_bat_callback)(void* p_cb_data, dvbpsi_bat_t* p_new_bat);


/*****************************************************************************
 * dvbpsi_AttachBAT
 *****************************************************************************/
/*!
 * \fn void dvbpsi_AttachBAT(dvbpsi_demux_t * p_demux, uint8_t i_table_id,
                             uint16_t i_extension, dvbpsi_bat_callback pf_callback,
                             void* p_cb_data)
 * \brief Creation and initialization of a BAT decoder.
 * \param p_demux Subtable demultiplexor to which the decoder is attached.
 * \param i_table_id Table ID, 0x4E, 0x4F, or 0x50-0x6F.
 * \param i_extension Table ID extension, here service ID.
 * \param pf_callback function to call back on new BAT.
 * \param p_cb_data private data given in argument to the callback.
 * \return 0 if everything went ok.
 */
int dvbpsi_AttachBAT(dvbpsi_decoder_t * p_psi_decoder, uint8_t i_table_id,
                     uint16_t i_extension, dvbpsi_bat_callback pf_callback,
                     void* p_cb_data);


/*****************************************************************************
 * dvbpsi_DetachBAT
 *****************************************************************************/
/*!
 * \fn void dvbpsi_DetachBAT(dvbpsi_demux_t * p_demux, uint8_t i_table_id,
                             uint16_t i_extension)
 * \brief Destroy a BAT decoder.
 * \param p_demux Subtable demultiplexor to which the decoder is attached.
 * \param i_table_id Table ID, 0x4E, 0x4F, or 0x50-0x6F.
 * \param i_extension Table ID extension, here service ID.
 * \return nothing.
 */
void dvbpsi_DetachBAT(dvbpsi_demux_t * p_demux, uint8_t i_table_id,
                      uint16_t i_extension);


/*****************************************************************************
 * dvbpsi_InitBAT/dvbpsi_NewBAT
 *****************************************************************************/
/*!
 * \fn void dvbpsi_InitBAT(dvbpsi_bat_t* p_bat, uint16_t i_bouquet_id,
                           uint8_t i_version, int b_current_next)
 * \brief Initialize a user-allocated dvbpsi_bat_t structure.
 * \param p_bat pointer to the BAT structure
 * \param i_bouquet_id bouquet id
 * \param i_version BAT version
 * \param b_current_next current next indicator
 * \return nothing.
 */
void dvbpsi_InitBAT(dvbpsi_bat_t* p_bat, uint16_t i_bouquet_id,
                    uint8_t i_version, int b_current_next);

/*!
 * \def dvbpsi_NewBAT(p_bat, i_bouquet_id,
                      i_version, b_current_next)
 * \brief Allocate and initialize a new dvbpsi_bat_t structure.
 * \param p_bat pointer to the BAT structure
 * \param i_bouquet_id bouquet id
 * \param i_version BAT version
 * \param b_current_next current next indicator
 * \param i_pcr_pid PCR_PID
 * \return nothing.
 */
#define dvbpsi_NewBAT(p_bat, i_bouquet_id,                              \
                      i_version, b_current_next)                        \
do {                                                                    \
  p_bat = (dvbpsi_bat_t*)malloc(sizeof(dvbpsi_bat_t));                  \
  if(p_bat != NULL)                                                     \
    dvbpsi_InitBAT(p_bat, i_bouquet_id, i_version, b_current_next);     \
} while(0);


/*****************************************************************************
 * dvbpsi_EmptyBAT/dvbpsi_DeleteBAT
 *****************************************************************************/
/*!
 * \fn void dvbpsi_EmptyBAT(dvbpsi_bat_t* p_bat)
 * \brief Clean a dvbpsi_bat_t structure.
 * \param p_bat pointer to the BAT structure
 * \return nothing.
 */
void dvbpsi_EmptyBAT(dvbpsi_bat_t* p_bat);

/*!
 * \def dvbpsi_DeleteBAT(p_bat)
 * \brief Clean and free a dvbpsi_bat_t structure.
 * \param p_bat pointer to the BAT structure
 * \return nothing.
 */
#define dvbpsi_DeleteBAT(p_bat)                                         \
do {                                                                    \
  dvbpsi_EmptyBAT(p_bat);                                               \
  free(p_bat);                                                          \
} while(0);


/*****************************************************************************
 * dvbpsi_BATAddDescriptor
 *****************************************************************************/
/*!
 * \fn dvbpsi_descriptor_t* dvbpsi_BATAddDescriptor(dvbpsi_bat_t* p_bat,
                                                    uint8_t i_tag,
                                                    uint8_t i_length,
                                                    uint8_t* p_data)
 * \brief Add a descriptor in the BAT.
 * \param p_bat pointer to the BAT structure
 * \param i_tag descriptor's tag
 * \param i_length descriptor's length
 * \param p_data descriptor's data
 * \return a pointer to the added descriptor.
 */
dvbpsi_descriptor_t* dvbpsi_BATAddDescriptor(dvbpsi_bat_t* p_bat,
                                             uint8_t i_tag, uint8_t i_length,
                                             uint8_t* p_data);


/*****************************************************************************
 * dvbpsi_BATAddTS
 *****************************************************************************/
/*!
 * \fn dvbpsi_bat_ts_t* dvbpsi_BATAddTS(dvbpsi_bat_t* p_bat,
                                        uint8_t i_type, uint16_t i_pid)
 * \brief Add an TS in the BAT.
 * \param p_bat pointer to the BAT structure
 * \param i_type type of TS
 * \param i_pid PID of the TS
 * \return a pointer to the added TS.
 */
dvbpsi_bat_ts_t* dvbpsi_BATAddTS(dvbpsi_bat_t* p_bat,
                                 uint16_t i_ts_id, uint16_t i_orig_network_id);


/*****************************************************************************
 * dvbpsi_BATTSAddDescriptor
 *****************************************************************************/
/*!
 * \fn dvbpsi_descriptor_t* dvbpsi_BATTSAddDescriptor(dvbpsi_bat_ts_t* p_ts,
                                                      uint8_t i_tag,
                                                      uint8_t i_length,
                                                      uint8_t* p_data)
 * \brief Add a descriptor in the BAT TS.
 * \param p_ts pointer to the TS structure
 * \param i_tag descriptor's tag
 * \param i_length descriptor's length
 * \param p_data descriptor's data
 * \return a pointer to the added descriptor.
 */
dvbpsi_descriptor_t* dvbpsi_BATTSAddDescriptor(dvbpsi_bat_ts_t* p_ts,
                                               uint8_t i_tag, uint8_t i_length,
                                               uint8_t* p_data);


/*****************************************************************************
 * dvbpsi_GenBATSections
 *****************************************************************************/
/*!
 * \fn dvbpsi_psi_section_t* dvbpsi_GenBATSections(dvbpsi_bat_t* p_bat,
                                                   uint8_t i_table_id)
 * \brief BAT generator
 * \param p_bat BAT structure
 * \param i_table_id table id, 0x4a
 * \return a pointer to the list of generated PSI sections.
 *
 * Generate BAT sections based on the dvbpsi_bat_t structure.
 */
dvbpsi_psi_section_t* dvbpsi_GenBATSections(dvbpsi_bat_t* p_bat,
                                            uint8_t i_table_id);


#ifdef __cplusplus
};
#endif

#else
#error "Multiple inclusions of bat.h"
#endif

