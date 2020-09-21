/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file am_si.h
 * \brief SI Decoder module
 *
 * \author Xia Lei Peng <leipeng.xia@amlogic.com>
 * \date 2010-10-14: create the document
 ***************************************************************************/

#ifndef _AM_SI_H
#define _AM_SI_H

#include "am_types.h"

#include "libdvbsi/descriptor.h"
#include "libdvbsi/dvbpsi.h"
#include "libdvbsi/psi.h"
#include "libdvbsi/demux.h"
#include "libdvbsi/tables/cat.h"
#include "libdvbsi/tables/pat.h"
#include "libdvbsi/tables/pmt.h"
#include "libdvbsi/tables/nit.h"
#include "libdvbsi/tables/sdt.h"
#include "libdvbsi/tables/eit.h"
#include "libdvbsi/tables/tot.h"
#include "libdvbsi/tables/bat.h"
#include "libdvbsi/descriptors/dr.h"
#include "libdvbsi/tables/atsc_mgt.h"
#include "libdvbsi/tables/atsc_vct.h"
#include "libdvbsi/tables/atsc_stt.h"
#include "libdvbsi/tables/atsc_eit.h"
#include "libdvbsi/tables/atsc_ett.h"
#include "libdvbsi/tables/atsc_cea.h"

#include "atsc/atsc_descriptor.h"
#include "atsc/atsc_rrt.h"
#include "atsc/atsc_mgt.h"
#include "atsc/atsc_vct.h"
#include "atsc/atsc_stt.h"
#include "atsc/atsc_eit.h"
#include "atsc/atsc_ett.h"



#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

/**\brief PID define*/
#define AM_SI_PID_PAT	(0x0)
#define AM_SI_PID_CAT	(0x1)
#define AM_SI_PID_NIT	(0x10)
#define AM_SI_PID_SDT	(0x11)
#define AM_SI_PID_BAT	(0x11)
#define AM_SI_PID_EIT	(0x12)
#define AM_SI_PID_TDT	(0x14)
#define AM_SI_PID_TOT	(0x14)
/**\brief ATSC PSIP base pid*/
#define AM_SI_ATSC_BASE_PID	ATSC_BASE_PID

/**\brief Table ID define*/
#define AM_SI_TID_PAT			(0x0)
#define AM_SI_TID_CAT			(0x1)
#define AM_SI_TID_PMT			(0x2)
#define AM_SI_TID_NIT_ACT		(0x40)
#define AM_SI_TID_NIT_OTH		(0X41)
#define AM_SI_TID_SDT_ACT		(0x42)
#define AM_SI_TID_SDT_OTH		(0x46)
#define AM_SI_TID_BAT			(0x4A)
#define AM_SI_TID_EIT_PF_ACT	(0x4E)
#define AM_SI_TID_EIT_PF_OTH	(0x4F)
#define AM_SI_TID_EIT_SCHE_ACT	(0x50) /* 0x50 - 0x5f*/
#define AM_SI_TID_EIT_SCHE_OTH	(0x60) /* 0x60 - 0x6f */
#define AM_SI_TID_TDT			(0x70)
#define AM_SI_TID_TOT			(0x73)

/**\brief  atsc table */
#define AM_SI_TID_PSIP_MGT			ATSC_PSIP_MGT_TID
#define AM_SI_TID_PSIP_TVCT			ATSC_PSIP_TVCT_TID
#define AM_SI_TID_PSIP_CVCT			ATSC_PSIP_CVCT_TID
#define AM_SI_TID_PSIP_RRT			ATSC_PSIP_RRT_TID
#define AM_SI_TID_PSIP_EIT			ATSC_PSIP_EIT_TID
#define AM_SI_TID_PSIP_ETT			ATSC_PSIP_ETT_TID
#define AM_SI_TID_PSIP_STT			ATSC_PSIP_STT_TID
#define AM_SI_TID_PSIP_DCCT			ATSC_PSIP_DCCT_TID
#define AM_SI_TID_PSIP_DCCSCT		ATSC_PSIP_DCCSCT_TID
#define AM_SI_TID_PSIP_CEA			ATSC_PSIP_CEA_TID

/**\brief descriptor tag define*/
/**\brief ISO/IEC 13818-1*/
#define AM_SI_DESCR_VIDEO_STREAM			(0x02)
#define AM_SI_DESCR_AUDIO_STREAM			(0x03)
#define AM_SI_DESCR_HIERARCHY				(0x04)
#define AM_SI_DESCR_REGISTRATION			(0x05)
#define AM_SI_DESCR_DS_ALIGNMENT			(0x06)
#define AM_SI_DESCR_TARGET_BG_GRID			(0x07)
#define AM_SI_DESCR_VIDEO_WINDOW			(0x08)
#define AM_SI_DESCR_CA						(0x09)
#define AM_SI_DESCR_ISO639					(0x0a)
#define AM_SI_DESCR_SYSTEM_CLOCK			(0x0b)
#define AM_SI_DESCR_MULX_BUF_UTILIZATION	(0x0c)
#define AM_SI_DESCR_COPYRIGHT				(0x0d)
#define AM_SI_DESCR_MAX_BITRATE				(0x0e)
#define AM_SI_DESCR_PRIVATE_DATA_INDICATOR	(0x0f)

/**\brief EN 300 468*/
#define AM_SI_DESCR_NETWORK_NAME			(0x40)
#define AM_SI_DESCR_SERVICE_LIST			(0x41)
#define AM_SI_DESCR_STUFFING				(0x42)
#define AM_SI_DESCR_SATELLITE_DELIVERY		(0x43)
#define AM_SI_DESCR_CABLE_DELIVERY			(0x44)
#define AM_SI_DESCR_VBI_DATA				(0x45)
#define AM_SI_DESCR_VBI_TELETEXT			(0x46)
#define AM_SI_DESCR_BOUQUET_NAME			(0x47)
#define AM_SI_DESCR_SERVICE					(0x48)
#define AM_SI_DESCR_LINKAGE					(0x4A)
#define AM_SI_DESCR_NVOD_REFERENCE			(0x4B)
#define AM_SI_DESCR_TIME_SHIFTED_SERVICE	(0x4C)
#define AM_SI_DESCR_SHORT_EVENT				(0x4D)
#define AM_SI_DESCR_EXTENDED_EVENT			(0x4E)
#define AM_SI_DESCR_TIME_SHIFTED_EVENT		(0x4F)
#define AM_SI_DESCR_COMPONENT				(0x50)
#define AM_SI_DESCR_MOSAIC					(0x51)
#define AM_SI_DESCR_STREAM_IDENTIFIER		(0x52)
#define AM_SI_DESCR_CA_IDENTIFIER			(0x53)
#define AM_SI_DESCR_CONTENT					(0x54)
#define AM_SI_DESCR_PARENTAL_RATING			(0x55)
#define AM_SI_DESCR_TELETEXT				(0x56)
#define AM_SI_DESCR_TELPHONE				(0x57)
#define AM_SI_DESCR_LOCAL_TIME_OFFSET		(0x58)
#define AM_SI_DESCR_SUBTITLING				(0x59)
#define AM_SI_DESCR_TERRESTRIAL_DELIVERY		(0x5A)
#define AM_SI_DESCR_MULTI_NETWORK_NAME		(0x5B)
#define AM_SI_DESCR_MULTI_BOUQUET_NAME		(0x5C)
#define AM_SI_DESCR_MULTI_SERVICE_NAME		(0x5D)
#define AM_SI_DESCR_MULTI_COMPONENT			(0x5E)
#define AM_SI_DESCR_DATA_BROADCAST			(0x64)
#define AM_SI_DESCR_DATA_BROADCAST_ID		(0x66)
#define AM_SI_DESCR_PDC						(0x69)
#define AM_SI_DESCR_AC3						(0x6A)
#define AM_SI_DESCR_ENHANCED_AC3			(0x7A)
#define AM_SI_DESCR_PSIPENHANCED_AC3		(0xCC)
#define AM_SI_DESCR_PSIP_AUDIOSTREAM_AC3	(0x81)
#define AM_SI_DESCR_DTS						(0x7B)
#define AM_SI_DESCR_AAC						(0x7C)
#define AM_SI_DESCR_EXTENSION				(0x7f)
#define AM_SI_DESCR_DRA                         (0xA0)
#define AM_SI_DESCR_LCN_83                      (0x83)
#define AM_SI_DESCR_LCN_87                      (0x87)
#define AM_SI_DESCR_LCN_88                      (0x88)
#define AM_SI_DESCR_ISDBSUBTITLING				(0xfd)


/**\brief ATSC Table types*/
#define AM_SI_ATSC_TT_CURRENT_TVCT	0x0
#define AM_SI_ATSC_TT_NEXT_TVCT		0x1
#define AM_SI_ATSC_TT_CURRENT_CVCT	0x2
#define AM_SI_ATSC_TT_NEXT_CVCT		0x3
#define AM_SI_ATSC_TT_ETT			0x4
#define AM_SI_ATSC_TT_DCCSCT		0x5
#define AM_SI_ATSC_TT_EIT0			0x100
#define AM_SI_ATSC_TT_ETT0			0x200
#define AM_SI_ATSC_TT_RRT_RR1		0x301
#define AM_SI_ATSC_TT_DCCT_ID0		0x1400

/**\brief ATSC descriptor*/
#define AM_SI_DESCR_SERVICE_LOCATION		(0xA1)
#define AM_SI_DESCR_CONTENT_ADVISORY		(0x87)
#define AM_SI_DESCR_CAPTION_SERVICE		(0x86)

/**\brief  the head of list provided by SI */
#define AM_SI_LIST_BEGIN(l, v) for ((v)=(l); (v)!=NULL; (v)=(v)->p_next){

/**\brief the hail of list provided by SI*/
#define AM_SI_LIST_END() }

/**\brief Maximum number of single Program supported audio*/
#define AM_SI_MAX_MAIN_AUD_CNT 8
/**\brief Maximum number of single Program supported audio*/
#define AM_SI_MAX_AUD_CNT 32
/**\brief Maximum number of single Program supported subtitle*/
#define AM_SI_MAX_SUB_CNT 32
/**\brief Maximum number of single Program supported teletext*/
#define AM_SI_MAX_TTX_CNT 32
/**\brief Maximum number of single Program supported caption*/
#define AM_SI_MAX_CAP_CNT 32
/**\brief Maximum number of single Program supported isdb subtitle*/
#define AM_SI_MAX_ISDB_SUB_CNT 8

#define SECS_BETWEEN_1JAN1970_6JAN1980 (315964800)

/****************************************************************************
 * Type definitions
 ***************************************************************************/
typedef void* AM_SI_Handle_t;

/**\brief Error code of the si module*/
enum AM_SI_ErrorCode
{
	AM_SI_ERROR_BASE=AM_ERROR_BASE(AM_MOD_SI),
	AM_SI_ERR_INVALID_HANDLE,          /**< Invalid handle*/
	AM_SI_ERR_NOT_SUPPORTED,           /**< not surport action*/
	AM_SI_ERR_NO_MEM,                  /**< out of native memory*/
	AM_SI_ERR_INVALID_SECTION_DATA,		   /**< data error of the section*/
	AM_SI_ERR_END
};

/**\brief section head define*/
typedef struct
{
	uint8_t		table_id;			/**< table_id*/
	uint8_t		syntax_indicator;	/**< section_syntax_indicator*/
	uint8_t		private_indicator;	/**< private_indicator*/
	uint16_t	length;				/**< section_length*/
	uint16_t	extension;			/**< table_id_extension*/
    uint8_t		version;			/**< version_number*/
    uint8_t		cur_next_indicator;	/**< current_next_indicator*/
    uint8_t		sec_num;			/**< section_number*/
    uint8_t		last_sec_num;		/**< last_section_number*/
}AM_SI_SectionHeader_t;

/**\brief audio exten info*/
enum AM_Audio_Exten
{
	AM_Audio_AC3MAIN=1,	/**<ac3 main service*/
	AM_Audio_AC3ASVC,   /**< ac3 ASVC,associated service*/
	AM_Audio_NOUSE
};
/**\brief audio info type,audio info in the ES stream,*/
typedef struct
{
	int		audio_count;		/**<ES stream Contains audio count*/
	int		audio_mainid;		/**<audio mainid increase value from 1 - 8*/
	struct
	{
		int		pid;	/**< audio PID*/
		int		fmt;	/**< audio format*/
		char	lang[10];	/**< audio language*/
		int 	audio_type;   /**<audio type*/
		int 	audio_exten;/**<now used to save ac3 mainid or asvc
												audio_exten is 32 bit
												31:24 bit: exten type,the value is enum AM_Audio_Exten vaue
												23:16:mainid or asvc id,8 bit
												15:0 bit:no use*/
	}audios[AM_SI_MAX_AUD_CNT];/**<audio info*/
}AM_SI_AudioInfo_t;

/**\brief scte27 info type*/
typedef struct
{
	int subtitle_count;	/**<subtitle count*/
	struct
	{
		int pid;					/**<subtitle stream pid*/
	}subtitles[AM_SI_MAX_SUB_CNT];/**<subtitle info*/
}AM_SI_Scte27SubtitleInfo_t;

/**\brief subtitle info type*/
typedef struct
{
	int subtitle_count;	/**<subtitle count*/
	struct
	{
		int pid;					/**<subtitle stream pid*/
		int type;					/**<subtitle type*/
		int comp_page_id; /**<subtitle composition page id*/
		int anci_page_id; /**<subtitle ancillary page id*/
		char lang[16];		/**<the language of subtitle*/
	}subtitles[AM_SI_MAX_SUB_CNT];/**<subtitle info*/
}AM_SI_SubtitleInfo_t;
/**\brief teletext info type*/
typedef struct
{
	int teletext_count;	/**<teletext count*/
	struct
	{
		int pid;					/**<teletext stream pid*/
		int type;					/**<teletext type*/
		int magazine_no;	/**<teletext magazine number*/
		int page_no;			/**<teletext page number*/
		char lang[16];		/**<the language of teletext*/
	}teletexts[AM_SI_MAX_TTX_CNT];/**<teletext info*/
}AM_SI_TeletextInfo_t;
/**\brief isdb subtitle info type*/
typedef struct
{
	int isdb_count;	/**<isdb subtitle count*/
	struct
	{
		int pid;					/**<isdb subtitle stream pid*/
		int type;					/**<isdb subtitle type*/
		char lang[16];		/**<the language of isdb subtitle*/
	}isdbs[AM_SI_MAX_ISDB_SUB_CNT];/**<isdb subtitle info*/
}AM_SI_IsdbsubtitleInfo_t;
/**\brief caption info type*/
typedef struct
{
	int caption_count;	/**<caption count*/
	struct
	{
		int type;			/**< 1: digital cc, 0: analog */
		int service_number;	/**<caption service number*/
		int pid_or_line21;  /**<line21 for analog / es pid for digital*/
		int flags;                  /**<easy reader(mask:0x80) / wide aspect ratio(mask:0x40) for digital*/
		unsigned int private_data;
		char lang[16];		/**<the language of caption for digital*/
	}captions[AM_SI_MAX_CAP_CNT];/**<caption info*/
}AM_SI_CaptionInfo_t;

/****************************************************************************
 * Function prototypes
 ***************************************************************************/

/**\brief creat a parser of parse si
 * \param [out] handle the handle of parser
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_SI_Create(AM_SI_Handle_t *handle);

/**\brief destroy a parser of parse si
 * \param handle the handle of parser
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_SI_Destroy(AM_SI_Handle_t handle);

/**\brief parse a section,surport si table:
 * CAT(dvbpsi_cat_t) PAT(dvbpsi_pat_t) PMT(dvbpsi_pmt_t)
 * SDT(dvbpsi_sdt_t) EIT(dvbpsi_eit_t) TOT(dvbpsi_tot_t)
 *  NIT(dvbpsi_nit_t).
 * e.g. parse a PAT section:
 * 	dvbpsi_pat_t *pat_sec;
 * 	AM_SI_DecodeSection(hSI, AM_SI_PID_PAT, pat_buf, len, &pat_sec);
 *
 * \param handle the handle of parser
 * \param pid section pid
 * \param [in] buf section original data
 * \param len section original data length
 * \param [out] sec parsered section data
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_SI_DecodeSection(AM_SI_Handle_t handle, uint16_t pid, uint8_t *buf, uint16_t len, void **sec);

/**\brief release a section that got from AM_SI_DecodeSection()
 * \param handle the handle of parser
 * \param table_id table id
 * \param [in] sec release section
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_SI_ReleaseSection(AM_SI_Handle_t handle, uint8_t table_id, void *sec);

/**\brief get the head info of a section
 * \param handle the handle of parser
 * \param [in] buf section original data
 * \param len section original data length
 * \param [out] sec_header store parsered section header info
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_SI_GetSectionHeader(AM_SI_Handle_t handle, uint8_t *buf, uint16_t len, AM_SI_SectionHeader_t *sec_header);

/**\brief set default text code,you can call this function when the first
 * character does not specify the encoding method.
 * \param [in] coding e.g GB2312 、BIG5 and so on.
 * \return none
 */
extern void AM_SI_SetDefaultDVBTextCoding(const char *coding);

/**\brief convert text to UTF-8 code text
 * \param [in] in_code/in Character data that needs to be converted
 * \param [in] in_len the lenght of in_code/in
 * \param [out] out_code/out Converted character data
 * \param [in] out_len out_code/out buf length
 * \param [in] coding coding of the @in
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_SI_ConvertDVBTextCode(char *in_code,int in_len,char *out_code,int out_len);
extern AM_ErrorCode_t AM_SI_ConvertDVBTextCodeEx(char *in, int in_len, char *out, int out_len, char *coding);

/**\brief get audio and video info from ES stream
 * \param [in] es ES stream
 * \param [out] vid video PID
 * \param [out] vfmt video format
 * \param [out] aud_info audio info
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_SI_ExtractAVFromES(dvbpsi_pmt_es_t *es, int *vid, int *vfmt, AM_SI_AudioInfo_t *aud_info);

/**\brief Extract audio and video from a visual channel ATSC according to ATSC standard
 * \param [in] vcinfo Channel information
 * \param [out] vid video PID
 * \param [out] vfmt video format
 * \param [out] aud_info audio info
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_SI_ExtractAVFromATSCVC(vct_channel_info_t *vcinfo, int *vid, int *vfmt, AM_SI_AudioInfo_t *aud_info);
extern AM_ErrorCode_t AM_SI_ExtractAVFromVC(dvbpsi_atsc_vct_channel_t *vcinfo, int *vid, int *vfmt, AM_SI_AudioInfo_t *aud_info);
/**\brief get subtitle info from ES stream
 * \param [in] es ES stream
 * \param [out] sub_info subtitle info
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_SI_ExtractDVBSubtitleFromES(dvbpsi_pmt_es_t *es, AM_SI_SubtitleInfo_t *sub_info);
/**\brief get Teletext info from ES stream
 * \param [in] es ES stream
 * \param [out] ttx_info Teletext info
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_SI_ExtractDVBTeletextFromES(dvbpsi_pmt_es_t *es, AM_SI_TeletextInfo_t *ttx_info);
/**\brief get Caption info from ES-stream
 * \param [in] es ES-stream
 * \param [out] cap_info Caption info
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_SI_ExtractATSCCaptionFromES(dvbpsi_pmt_es_t *es, AM_SI_CaptionInfo_t *cap_info);
/**\brief get Rating/Rating/Caption String from ContentAdvisoryDescriptor/descriptor/descriptor
 * \param [in] decoded content advisory descriptor/decoded descriptors/decoded descriptors
 * \param [out] json string
 * \param [in] buffer size
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_SI_GetRatingString(dvbpsi_atsc_content_advisory_dr_t *pcad, char *buf, int buf_size);
extern AM_ErrorCode_t AM_SI_GetRatingStringFromDescriptors(dvbpsi_descriptor_t *p_descriptor, char *buf, int buf_size);
extern AM_ErrorCode_t AM_SI_GetATSCCaptionStringFromDescriptors(dvbpsi_descriptor_t *p_descriptor, char *buf, int buf_size);
/**\brief get the coding and data of the given DVB-Text
 * \param [in] in DVB-Text data
 * \param [in] in_len length of the data
 * \param [out] coding buf to store the coding-string
 * \param [in] coding_len length of the buf @out
 * \param [out] offset the raw data offset in the DVB-Text
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_SI_GetDVBTextCodingAndData(char *in, int in_len, char *coding, int coding_len, int *offset);
/**\brief convert the data to utf-8 coding format
 * \param [in] in the data
 * \param [in] in_len the data length
 * \param [out] out the output data
 * \param [in] out_len the size of the buf @out
 * \param [in] the original coding of the data
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_SI_ConvertToUTF8(char *in, int in_len, char *out, int out_len, char *coding);


#ifdef __cplusplus
}
#endif

#endif

