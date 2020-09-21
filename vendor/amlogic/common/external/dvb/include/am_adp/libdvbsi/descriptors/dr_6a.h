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
 * \file <dr_6a.h>
 * \author chen hua ling <hualing.chen@amlogic.com>
 * \brief Application interface for the AC3
 * descriptor decoder and generator.
 *
 * Application interface for the MPEG 2 TS Ac3 descriptor decoder and generator
 * This descriptor's definition can be found in ETSC EN 300 468
 */

#ifndef _DVBPSI_DR_6A_H_
#define _DVBPSI_DR_6A_H_

#ifdef __cplusplus
extern "C" {
#endif


/*****************************************************************************
 * dvbpsi_AC3_dr_t
 *****************************************************************************/
/*!
 * \struct dvbpsi_AC3_dr_t
 * \brief AC3 descriptor structure.
 *
 * This structure is used to store a decoded AC3 descriptor.
 * (ETSI EN 300 468).
 */
/*!
 * \typedef struct dvbpsi_AC3_dr_s dvbpsi_AC3_dr_t
 * \brief dvbpsi_AC3_dr_t type definition.
 */
typedef struct dvbpsi_AC3_dr_s
{
	uint8_t   i_component_type_flag;          /*!< component type flag */
	uint8_t   i_bsid_flag;          			/*!< bsid flag */
	uint8_t   i_mainid_flag;          /*!< mainid flag */
	uint8_t   i_asvc_flag;          /*!< asvc flag */
	uint8_t   i_component_type;          /*!< component type */
	uint8_t   i_bsid;          /*!< bsid */
	uint8_t   i_mainid;          /*!< mainid */
	uint8_t   i_asvc;          /*!< asvc */

} dvbpsi_AC3_dr_t;

#define AM_AC3_CMP_FLAG			(0x80) /*1000 0000*/
#define AM_AC3_BSID_FLAG		(0x40)  /*0100 0000*/
#define AM_AC3_MAINID_FLAG		(0x20)  /*0010 0000*/
#define AM_AC3_ASVC_FLAG		(0x10)  /*0001 0000*/
/*****************************************************************************
 * dvbpsi_DecodeAC3Dr
 *****************************************************************************/
/*!
 * \fn dvbpsi_AC3_dr_t * dvbpsi_DecodeAC3Dr(
                                        dvbpsi_descriptor_t * p_descriptor)
 * \brief ac3 descriptor decoder.
 * \param p_descriptor pointer to the descriptor structure
 * \return a pointer to a new "ac3 audio" descriptor structure which
 * contains the decoded data.
 */
dvbpsi_AC3_dr_t* dvbpsi_DecodeAC3Dr(dvbpsi_descriptor_t * p_descriptor);


#ifdef __cplusplus
};
#endif

#else
#error "Multiple inclusions of dr_6a.h"
#endif

