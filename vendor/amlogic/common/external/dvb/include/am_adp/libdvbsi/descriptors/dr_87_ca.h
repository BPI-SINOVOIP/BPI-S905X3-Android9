/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package. *
* Description:
*/
/*
dr_87_ca.h

Decode Content Advisory Descriptor.

*/

/*!
 * \file dr_87_ca.h
 * \brief Decode Content Advisory Descriptor.
 */

#ifndef _DR_87_CA_H
#define _DR_87_CA_H

#ifdef __cplusplus
extern "C" {
#endif
/*****************************************************************************
 * dvbpsi_content_advisory_rating_dimension_t
 *****************************************************************************/
/*!
 * \struct dvbpsi_content_advisory_rating_dimension_s
 * \brief Content Advisory Rating Dimension
 *
 * This structure is used to store a decoded Content Advisory Rating Dimension.
 */
/*!
 * \typedef struct dvbpsi_content_advisory_rating_dimension_s dvbpsi_content_advisory_rating_dimension_t
 * \brief dvbpsi_content_advisory_rating_dimension_t type definition.
 */
typedef struct dvbpsi_content_advisory_rating_dimension_s
{
    uint8_t      i_rating_dimension_j;  /*!< Rating dimension*/
    uint8_t      i_rating_value;        /*!< Rating value */
} dvbpsi_content_advisory_rating_dimension_t;

/*****************************************************************************
 * dvbpsi_content_advisory_region_t
 *****************************************************************************/
/*!
 * \struct dvbpsi_content_advisory_region_s
 * \brief Content Advisory Region
 *
 * This structure is used to store a decoded Content Advisory Region.
 */
/*!
 * \typedef struct dvbpsi_content_advisory_region_s dvbpsi_content_advisory_region_t
 * \brief dvbpsi_content_advisory_region_t type definition.
 */
typedef struct dvbpsi_content_advisory_region_s
{
    uint8_t      i_rating_region;          /*!< Rating region */
    uint8_t      i_rated_dimensions;      /*!< Rated dimension */
    struct dvbpsi_content_advisory_rating_dimension_s  dimensions[0xff];    /*!< Rated dimensions */
    uint8_t      i_rating_description_length; /*!< length of the description in bytes */
    uint8_t      i_rating_description[80];     /*!< Description in multiple string structure format */
} dvbpsi_content_advisory_region_t;

/*****************************************************************************
 * dvbpsi_atsc_content_advisory_dr_s
 *****************************************************************************/
/*!
 * \struct dvbpsi_atsc_content_advisory_dr_s
 * \brief Content Advisory Descriptor
 *
 * This structure is used to store a decoded Content Advisory descriptor.
 */
/*!
 * \typedef struct dvbpsi_atsc_content_advisory_dr_s dvbpsi_atsc_content_advisory_dr_t
 * \brief dvbpsi_atsc_content_advisory_dr_t type definition.
 */
typedef struct dvbpsi_atsc_content_advisory_dr_s
{
    uint8_t i_rating_region_count; /*!< Number of Regions */
    dvbpsi_content_advisory_region_t rating_regions[0x2f]; /*!< Rating Region array */
} dvbpsi_atsc_content_advisory_dr_t;

/*****************************************************************************
 * dvbpsi_decode_atsc_content_advisory_dr
 *****************************************************************************/
/*!
 * \fn dvbpsi_atsc_content_advisory_dr_t dvbpsi_decode_atsc_content_advisory_dr(dvbpsi_descriptor_t *p_descriptor)
 * \brief Decode a Content Advisory descriptor (tag 0x86)
 * \param p_descriptor Raw descriptor to decode.
 * \return NULL if the descriptor could not be decoded or a pointer to a
 *         dvbpsi_atsc_content_advisory_dr_t structure.
 */
dvbpsi_atsc_content_advisory_dr_t *dvbpsi_decode_atsc_content_advisory_dr(dvbpsi_descriptor_t *p_descriptor);

#ifdef DVBPSI_USE_DEPRECATED_DR_API
typedef dvbpsi_atsc_content_advisory_dr_t dvbpsi_content_advisory_dr_t ;

__attribute__((deprecated,unused)) static dvbpsi_content_advisory_dr_t* dvbpsi_DecodeCaptionServiceDr (dvbpsi_descriptor_t *dr) {
    return dvbpsi_decode_atsc_content_advisory_dr (dr);
}
#endif

#ifdef __cplusplus
}
#endif

#endif

