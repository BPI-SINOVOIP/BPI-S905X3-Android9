/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package. *
* Description:
*/
/*****************************************************************************
 * dr_6a.h
 * (c)2016 amlogic
 *
 *****************************************************************************/

/*!
 * \file <dr_7a.h>
 * \author chen hua ling <hualing.chen@amlogic.com>
 * \brief Application interface for the Enhanced AC3
 * descriptor decoder and generator.
 *
 * Application interface for the MPEG 2 TS Enhanced Ac3 descriptor decoder and generator
 * This descriptor's definition can be found in ETSC EN 300 468
 */

#ifndef _DVBPSI_DR_7A_H_
#define _DVBPSI_DR_7A_H_

#ifdef __cplusplus
extern "C" {
#endif


/*****************************************************************************
 * dvbpsi_ENAC3_dr_t
 *****************************************************************************/
/*!
 * \struct dvbpsi_ENAC3_dr_t
 * \brief EN AC3 descriptor structure.
 *
 * This structure is used to store a decoded EN AC3 descriptor.
 * (ETSI EN 300 468).
 */
/*!
 * \typedef struct dvbpsi_ENAC3_dr_s dvbpsi_ENAC3_dr_t
 * \brief dvbpsi_ENAC3_dr_t type definition.
 */
typedef struct dvbpsi_ENAC3_dr_s
{
	uint8_t   i_component_type_flag;          /*!< component type flag */
	uint8_t   i_bsid_flag;          			/*!< bsid flag */
	uint8_t   i_mainid_flag;          /*!< mainid flag */
	uint8_t   i_asvc_flag;          /*!< asvc flag */
	uint8_t	  i_mix_info_exist;		/*!< mix info exist */
	uint8_t	  i_sub_stream1_flag;	/*!< sub stream1 flag */
	uint8_t	  i_sub_stream2_flag;	/*!< sub stream2 flag */
	uint8_t	  i_sub_stream3_flag;	/*!< sub stream3 flag */

	uint8_t   i_component_type;          /*!< component type */
	uint8_t   i_bsid;          /*!< bsid */
	uint8_t   i_mainid;          /*!< mainid */
	uint8_t   i_asvc;          /*!< asvc */
	uint8_t   i_sub_stream1;          /*!< sub stream 1 */
	uint8_t   i_sub_stream2;          /*!< sub stream 2 */
	uint8_t   i_sub_stream3;          /*!< sub stream 3 */
} dvbpsi_ENAC3_dr_t;

#define AM_ENAC3_CMP_FLAG		(0x80) /*1000 0000*/
#define AM_ENAC3_BSID_FLAG		(0x40)  /*0100 0000*/
#define AM_ENAC3_MAINID_FLAG	(0x20)  /*0010 0000*/
#define AM_ENAC3_ASVC_FLAG		(0x10)  /*0001 0000*/
#define AM_ENAC3_MIX_EXIST		(0x08) /*0000 1000 */
#define AM_ENAC3_SUB1_FLAG		(0x04)  /*0000 0100 */
#define AM_ENAC3_SUB2_FLAG		(0x02)  /*0000 0010 */
#define AM_ENAC3_SUB3_FLAG		(0x01)  /*0000 0001 */
/*****************************************************************************
 * dvbpsi_DecodeENAC3Dr
 *****************************************************************************/
/*!
 * \fn dvbpsi_ENAC3_dr_t * dvbpsi_DecodeENAC3Dr(
                                        dvbpsi_descriptor_t * p_descriptor)
 * \brief EN ac3 descriptor decoder.
 * \param p_descriptor pointer to the descriptor structure
 * \return a pointer to a new "EN ac3 audio" descriptor structure which
 * contains the decoded data.
 */
dvbpsi_ENAC3_dr_t* dvbpsi_DecodeENAC3Dr(dvbpsi_descriptor_t * p_descriptor);


#ifdef __cplusplus
};
#endif

#else
#error "Multiple inclusions of dr_7a.h"
#endif

