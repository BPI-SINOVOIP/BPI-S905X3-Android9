/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package. *
* Description:
*/
/*****************************************************************************
 * dr_7f.h
 * (c)2016 amlogic
 *
 *****************************************************************************/

/*!
 * \file <dr_7f.h>
 * \author chen hua ling <hualing.chen@amlogic.com>
 * \brief Application interface for the extention des
 * descriptor decoder and generator.
 *
 * Application interface for the MPEG 2 TS extention des descriptor decoder and generator
 * This descriptor's definition can be found in ETSC EN 300 468
 */

#ifndef _DVBPSI_DR_7F_H_
#define _DVBPSI_DR_7F_H_

#ifdef __cplusplus
extern "C" {
#endif


/*****************************************************************************
 * dvbpsi_EXTENTION_dr_t
 *****************************************************************************/
/*!
 * \struct dvbpsi_EXTENTION_dr_t
 * \brief EXTENTION descriptor structure.
 *
 * This structure is used to store a decoded EXTENTION descriptor.
 * (ETSI EN 300 468).
 */
/*!
 * \typedef struct dvbpsi_EXTENTION_dr_s dvbpsi_EXTENTION_dr_t
 * \brief dvbpsi_EXTENTION_dr_t type definition.
 */

typedef uint8_t language_code_t[3];

typedef struct dvbpsi_EXTENTION_sup_audio_s
{
	uint8_t		mix_type;					/*!< audio mix type */
	uint8_t		editorial_classification;	/*!< audio edit classificatio */
	uint8_t		lang_code;					/*!< lang code,is set 1,need used lang code replace iso 639 code */
	language_code_t	iso_639_lang_code;		/*!< ISO_639_language_code */
} dvbpsi_EXTENTION_sup_audio_t;


typedef struct dvbpsi_EXTENTION_dr_s
{
	uint8_t   i_extern_des_tag;          /*!< exten des tag */
	union {
		dvbpsi_EXTENTION_sup_audio_t sup_audio;
	} exten_t;							/*!< exten tag info */
} dvbpsi_EXTENTION_dr_t;

#define AM_AUDIO_MIX_DEPENDENT	(0)
#define AM_AUDIO_MIX_COMPLETE	(1)

#define AM_AUDIO_EDIT_CLASS_MAIN	(0x0)
#define AM_AUDIO_EDIT_CLASS_SUB	(0x01)
#define AM_AUDIO_EDIT_CLASS_CLEANAUDIO	(0x02)
#define AM_AUDIO_EDIT_CLASS_SPOKENSUB	(0x03)
#define AM_AUDIO_EDIT_CLASS_EXTEN	(0x04)

/**\brief exten descriptor tag define*/
/**\brief ISO/IEC 300 468*/
#define AM_SI_EXTEN_DESCR_IMAGE_ICON				(0x00)
#define AM_SI_EXTEN_DESCR_CPCM_DELIVERY_SIGNAL		(0x01)
#define AM_SI_EXTEN_DESCR_CP						(0x02)
#define AM_SI_EXTEN_DESCR_CP_IDENTIFITER			(0x03)
#define AM_SI_EXTEN_DESCR_T2_DELIVERY_SYS			(0x04)
#define AM_SI_EXTEN_DESCR_SH_DELIVERY_SYS			(0x05)
#define AM_SI_EXTEN_DESCR_SUP_AUDIO					(0x06)
#define AM_SI_EXTEN_DESCR_NETWORK_CHANGE_NOTIFY		(0x07)
#define AM_SI_EXTEN_DESCR_MESSAGE					(0x08)
#define AM_SI_EXTEN_DESCR_TARGET_REGION				(0x09)
#define AM_SI_EXTEN_DESCR_TARGET_REGION_NAME		(0x0A)
#define AM_SI_EXTEN_DESCR_SERVICE_RELOCATED			(0x0B)
#define AM_SI_EXTEN_DESCR_XAIT_PID					(0x0C)
#define AM_SI_EXTEN_DESCR_C2_DELIVERY				(0x0D)
#define AM_SI_EXTEN_DESCR_DTSHD_AUDIO				(0x0E)
#define AM_SI_EXTEN_DESCR_DTS_NEURAL				(0x0F)
#define AM_SI_EXTEN_DESCR_VIDEO_DEPTH_RANGE			(0x10)
#define AM_SI_EXTEN_DESCR_T2MI						(0x11)
#define AM_SI_EXTEN_DESCR_RESERVED					(0x12)
#define AM_SI_EXTEN_DESCR_URI_LINKAGE				(0x13)
#define AM_SI_EXTEN_DESCR_BCI_ANCILLARY				(0x14)
#define AM_SI_EXTEN_DESCR_OTHER						(0x15)
/*****************************************************************************
 * dvbpsi_DecodeEXTENTIONDr
 *****************************************************************************/
/*!
 * \fn dvbpsi_EXTENTION_dr_t * dvbpsi_DecodeEXTENTIONDr(
                                        dvbpsi_descriptor_t * p_descriptor)
 * \brief EXTENTION descriptor decoder.
 * \param p_descriptor pointer to the descriptor structure
 * \return a pointer to a new "EXTENTION audio" descriptor structure which
 * contains the decoded data.
 */
dvbpsi_EXTENTION_dr_t* dvbpsi_DecodeEXTENTIONDr(dvbpsi_descriptor_t * p_descriptor);


#ifdef __cplusplus
};
#endif

#else
#error "Multiple inclusions of dr_7f.h"
#endif

