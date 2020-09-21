/*
Copyright (C) 2006  Adam Charrett

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

cea.h

*/

/*!
 * \file atsc_cea.h
 * \author Adam Charrett
 * \brief Decode PSIP Event Information Table (ATSC cea).
 * \cable_emergency_alert()
 */

#ifndef _ATSC_CEA_H
#define _ATSC_CEA_H

#ifdef __cplusplus
extern "C" {
#endif

enum AM_EAS_MULTI_TYPE {
   AM_EAS_MULTI_NATURE,
   AM_EAS_MULTI_ALERT,
};
/*****************************************************************************
 * dvbpsi_atsc_cea_exception_t
 *****************************************************************************/
/*!
 * \struct dvbpsi_atsc_cea_exception_s
 * \brief ATSC cea location code  structure.
 *
 * This structure is used to store decoded cea exception information.
 */
/*!
 * \typedef struct dvbpsi_atsc_cea_exception_s dvbpsi_atsc_cea_exception_t
 * \brief dvbpsi_atsc_cea_exception_t type definition.
 */
typedef struct dvbpsi_atsc_cea_exception_s
{
    uint8_t  i_in_band_refer;
    uint16_t i_exception_major_channel_number;    /**<the exception major channel num*/
    uint16_t i_exception_minor_channel_number;    /**<the exception minor channel num*/
    uint16_t exception_OOB_source_ID;       /**<the exception oob source id*/
} dvbpsi_atsc_cea_exception_t;

/*****************************************************************************
 * dvbpsi_atsc_cea_location_code_t
 *****************************************************************************/
/*!
 * \struct dvbpsi_atsc_cea_location_code_s
 * \brief ATSC cea location code  structure.
 *
 * This structure is used to store decoded cea location code information.
 */
/*!
 * \typedef struct dvbpsi_atsc_cea_location_code_s dvbpsi_atsc_cea_location_code_t
 * \brief dvbpsi_atsc_cea_location_code_t type definition.
 */
typedef struct dvbpsi_atsc_cea_location_code_s
{
    uint8_t  i_state_code;
    uint8_t  i_country_subdiv;    /**<the language of mlti str*/
    uint16_t i_country_code;

} dvbpsi_atsc_cea_location_code_t;

/*****************************************************************************
 * dvbpsi_atsc_cea_multi_str_t
 *****************************************************************************/
/*!
 * \struct dvbpsi_atsc_cea_multi_str_s
 * \brief ATSC cea nulti string  structure.
 *
 * This structure is used to store decoded cea multi string information.
 */
/*!
 * \typedef struct dvbpsi_atsc_cea_multi_str_s dvbpsi_atsc_cea_multi_str_t
 * \brief dvbpsi_atsc_cea_multi_str_t type definition.
 */
typedef struct dvbpsi_atsc_cea_multi_str_s
{
    uint8_t   lang[3];    /**<the language of mlti str*/
    uint8_t   i_type;    /**<the str type, alert or nature*/
    uint8_t   i_compression_type;      /*!< compression type */
    uint8_t   i_mode;      /*!< mode */
    uint32_t  i_number_bytes;      /*!< number bytes */
    uint8_t   compressed_str[4096];    /**<the compressed str*/
    struct dvbpsi_atsc_cea_multi_str_s   *p_next;/*!< Next multi str information. */
} dvbpsi_atsc_cea_multi_str_t;
/*****************************************************************************
 * dvbpsi_atsc_cea_t
 *****************************************************************************/
/*!
 * \struct dvbpsi_atsc_cea_s
 * \brief ATSC cea table  structure.
 *
 * This structure is used to store decoded cea information.
 */
/*!
 * \typedef struct dvbpsi_atsc_cea_s dvbpsi_atsc_cea_t
 * \brief dvbpsi_atsc_cea_t type definition.
 */
typedef struct dvbpsi_atsc_cea_s
{
    uint8_t    i_table_id;         /*!< table id */
    uint16_t   i_extension;        /*!< subtable id */
    uint8_t    i_version;          /*!< version_number */
    bool       b_current_next;     /*!< current_next_indicator */
    uint8_t    i_sequence_num;     /*!< sequence version*/
    uint8_t    i_protocol_version; /*!< protocol version*/
    uint16_t   i_eas_event_id;     /*!< eas event id */
    uint8_t    i_eas_orig_code[3]; /*!< eas event orig code */
    uint8_t    i_eas_event_code_len;   /*!< eas event code len */
    uint8_t    i_eas_event_code[256];   /*!< eas event code */
    uint8_t    i_alert_message_time_remaining;   /*!< alert msg remain time */
    uint32_t   i_event_start_time;   /*!< eas event start time */
    uint16_t   i_event_duration;   /*!< event dur */
    uint8_t    i_alert_priority;   /*!< alert priority */
    uint16_t   i_details_OOB_source_ID;   /*!< details oob source id */
    uint16_t   i_details_major_channel_number;   /*!< details major channel num */
    uint16_t   i_details_minor_channel_number;   /*!< details minor channel num */
    uint16_t   i_audio_OOB_source_ID;   /*!< audio oob source id */
    uint8_t    i_location_count;  /*!< location count */
    uint8_t    i_exception_count; /*!< exception count */

    dvbpsi_atsc_cea_location_code_t location[256];  /*!< location info */
    dvbpsi_atsc_cea_exception_t   exception[256];  /*!< exception info */

    dvbpsi_descriptor_t     *p_first_descriptor;  /*!< First descriptor structure. */
    struct dvbpsi_atsc_cea_multi_str_s   *p_first_multi_text;  /*!< nature and alert multi str information structure. */
    struct dvbpsi_atsc_cea_s *p_next;
} dvbpsi_atsc_cea_t;

/*****************************************************************************
 * dvbpsi_cea_callback
 *****************************************************************************/
/*!
 * \typedef void (* dvbpsi_atsc_cea_callback)(void* p_cb_data,
                                         dvbpsi_atsc_cea_t* p_new_cea)
 * \brief Callback type definition.
 */
typedef void (* dvbpsi_atsc_cea_callback)(void* p_cb_data, dvbpsi_atsc_cea_t* p_new_cea);

/*****************************************************************************
 * dvbpsi_atsc_InitCEA/dvbpsi_atsc_NewCEA
 *****************************************************************************/
/*!
 * \fn void dvbpsi_atsc_InitCEA(dvbpsi_atsc_cea_t* p_cea, uint8_t i_table_id, uint16_t i_extension,
                                uint8_t i_version, uint8_t i_protocol);
 * \brief Initialize a user-allocated dvbpsi_atsc_cea_t structure.
 * \param p_cea pointer to the CEA structure
 * \param i_table_id Table ID, 0xD8.
 * \param i_extension Table ID extension,  shall be set 0x0000.
 * \param i_version CEA version
 * \param b_current_next.
 * \return nothing.
 */
void dvbpsi_atsc_InitCEA(dvbpsi_atsc_cea_t* p_cea, uint8_t i_table_id, uint16_t i_extension,
                         uint8_t i_version, bool b_current_next);

/*!
 * \fn dvbpsi_atsc_cea_t *dvbpsi_atsc_NewCEA(uint8_t i_table_id, uint16_t i_extension,
                                             uint8_t i_version, uint8_t i_protocol,
                                             uint16_t i_source_id, bool b_current_next)
 * \brief Allocate and initialize a new dvbpsi_cea_t structure. Use ObjectRefDec to delete it.
 * \param i_table_id Table ID, 0xD8.
 * \param i_extension Table ID extension,  shall be set 0x0000.
 * \param i_version CEA version
 * \param b_current_next.
 * \return p_cea pointer to the CEA structure or NULL on error
 */
dvbpsi_atsc_cea_t *dvbpsi_atsc_NewCEA(uint8_t i_table_id, uint16_t i_extension,
                                      uint8_t i_version, bool b_current_next);

/*****************************************************************************
 * dvbpsi_atsc_EmptyCEA
 *****************************************************************************/
/*!
 * \fn void dvbpsi_atsc_EmptyCEA(dvbpsi_atsc_cea_t* p_cea)
 * \brief Clean a dvbpsi_cea_t structure.
 * \param p_cea pointer to the CEA structure
 * \return nothing.
 */
void dvbpsi_atsc_EmptyCEA(dvbpsi_atsc_cea_t *p_cea);

/*!
 * \fn void dvbpsi_atsc_DeleteCEA(dvbpsi_atsc_cea_t *p_cea)
 * \brief Clean and free a dvbpsi_cea_t structure.
 * \param p_cea pointer to the CEA structure
 * \return nothing.
 */
void dvbpsi_atsc_DeleteCEA(dvbpsi_atsc_cea_t *p_cea);

void dvbpsi_atsc_DecodeCEASections(dvbpsi_atsc_cea_t* p_cea,
                              dvbpsi_psi_section_t* p_section);
#ifdef __cplusplus
};
#endif

#endif


