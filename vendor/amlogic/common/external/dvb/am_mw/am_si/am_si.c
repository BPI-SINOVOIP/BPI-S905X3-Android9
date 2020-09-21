#ifdef _FORTIFY_SOURCE
#undef _FORTIFY_SOURCE
#endif
/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file am_si.c
 * \brief SI Decoder 模块
 *
 * \author Xia Lei Peng <leipeng.xia@amlogic.com>
 * \date 2010-10-15: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 5

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <am_debug.h>
#include "am_si.h"
#include "am_si_internal.h"
#include <am_mem.h>
#include <am_av.h>
#include <am_iconv.h>
#include "am_misc.h"
#include <errno.h>
#include <freesat.h>
#include <amports/vformat.h>

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

/*增加一个描述符及其对应的解析函数*/
#define SI_ADD_DESCR_DECODE_FUNC(_tag, _func)\
		case _tag:\
			_func(descr);\
			break;
/**\brief 解析ATSC表*/
#define si_decode_psip_table(p_table, tab, _type, _buf, _len)\
	AM_MACRO_BEGIN\
		_type *p_sec = atsc_psip_new_##tab##_info();\
		if (p_sec == NULL){\
			ret = AM_SI_ERR_NO_MEM;\
		} else {\
			ret = atsc_psip_parse_##tab(_buf,(uint32_t)_len, p_sec);\
		}\
		if (ret == AM_SUCCESS){\
			p_sec->i_table_id = _buf[0];\
			p_table = (void*)p_sec;\
		} else if (p_sec){\
			atsc_psip_free_##tab##_info(p_sec);\
			p_table = NULL;\
		}\
	AM_MACRO_END


/****************************************************************************
 * Static data
 ***************************************************************************/

extern void dvbpsi_DecodePATSections(dvbpsi_pat_t *p_pat,dvbpsi_psi_section_t* p_section);
extern void dvbpsi_DecodePMTSections(dvbpsi_pmt_t *p_pmt,dvbpsi_psi_section_t* p_section);
extern void dvbpsi_DecodeCATSections(dvbpsi_cat_t *p_cat,dvbpsi_psi_section_t* p_section);
extern void dvbpsi_DecodeNITSections(dvbpsi_nit_t *p_nit,dvbpsi_psi_section_t* p_section);
extern void dvbpsi_DecodeSDTSections(dvbpsi_sdt_t *p_sdt,dvbpsi_psi_section_t* p_section);
extern void dvbpsi_DecodeEITSections(dvbpsi_eit_t *p_eit,dvbpsi_psi_section_t* p_section);
extern void dvbpsi_DecodeTOTSections(dvbpsi_tot_t *p_tot,dvbpsi_psi_section_t* p_section);
extern void dvbpsi_DecodeBATSections(dvbpsi_bat_t *p_tot,dvbpsi_psi_section_t* p_section);

extern void dvbpsi_atsc_DecodeMGTSections(dvbpsi_atsc_mgt_t *p_mgt,dvbpsi_psi_section_t* p_section);
extern void dvbpsi_atsc_DecodeVCTSections(dvbpsi_atsc_vct_t *p_vct,dvbpsi_psi_section_t* p_section);
extern void dvbpsi_atsc_DecodeSTTSections(dvbpsi_atsc_stt_t *p_stt,dvbpsi_psi_section_t* p_section);
extern void dvbpsi_atsc_DecodeEITSections(dvbpsi_atsc_eit_t *p_eit,dvbpsi_psi_section_t* p_section);
extern void dvbpsi_atsc_DecodeETTSections(dvbpsi_atsc_ett_t *p_ett,dvbpsi_psi_section_t* p_section);

extern int dvbpsi_bat_Set_DecodeDescriptor_Callback(DVBpsi_Decode_Descriptor cb, void *user);
extern int dvbpsi_eit_Set_DecodeDescriptor_Callback(DVBpsi_Decode_Descriptor cb, void *user);
extern int dvbpsi_cat_Set_DecodeDescriptor_Callback(DVBpsi_Decode_Descriptor cb, void *user);
extern int dvbpsi_nit_Set_DecodeDescriptor_Callback(DVBpsi_Decode_Descriptor cb, void *user);
extern int dvbpsi_pmt_Set_DecodeDescriptor_Callback(DVBpsi_Decode_Descriptor cb, void *user);
extern int dvbpsi_sdt_Set_DecodeDescriptor_Callback(DVBpsi_Decode_Descriptor cb, void *user);
extern int dvbpsi_tot_Set_DecodeDescriptor_Callback(DVBpsi_Decode_Descriptor cb, void *user);
extern int dvbpsi_atsc_eit_Set_DecodeDescriptor_Callback(DVBpsi_Decode_Descriptor cb, void *user);
extern int dvbpsi_atsc_ett_Set_DecodeDescriptor_Callback(DVBpsi_Decode_Descriptor cb, void *user);
extern int dvbpsi_atsc_mgt_Set_DecodeDescriptor_Callback(DVBpsi_Decode_Descriptor cb, void *user);
extern int dvbpsi_atsc_vct_Set_DecodeDescriptor_Callback(DVBpsi_Decode_Descriptor cb, void *user);
extern int dvbpsi_atsc_stt_Set_DecodeDescriptor_Callback(DVBpsi_Decode_Descriptor cb, void *user);

/*DVB字符默认编码,在进行DVB字符转码时会强制使用该编码为输入编码*/
static char forced_dvb_text_coding[32] = {0};

static const char * const si_prv_data = "AM SI Decoder";


/**\brief 从dvbpsi_psi_section_t结构创建PAT表*/
static AM_ErrorCode_t si_decode_pat(void **p_table, dvbpsi_psi_section_t *p_section)
{
	dvbpsi_pat_t *p_pat;

	assert(p_table && p_section);

	/*Allocate a new table*/
	p_pat = (dvbpsi_pat_t*)malloc(sizeof(dvbpsi_pat_t));
	if (p_pat == NULL)
	{
		*p_table = NULL;
		return AM_SI_ERR_NO_MEM;
	}

	/*Init the p_pat*/
	dvbpsi_InitPAT(p_pat,
			p_section->i_extension,
			p_section->i_version,
			p_section->b_current_next);
	/*Decode*/
	dvbpsi_DecodePATSections(p_pat, p_section);

	p_pat->i_table_id = p_section->i_table_id;
	*p_table = (void*)p_pat;

	return AM_SUCCESS;
}

/**\brief 从dvbpsi_psi_section_t结构创建PMT表*/
static AM_ErrorCode_t si_decode_pmt(void **p_table, dvbpsi_psi_section_t *p_section)
{
	dvbpsi_pmt_t *p_pmt;

	assert(p_table && p_section);

	/*Allocate a new table*/
	p_pmt = (dvbpsi_pmt_t*)malloc(sizeof(dvbpsi_pmt_t));
	/*need init p_pmt*/
	memset(p_pmt, 0, sizeof(dvbpsi_pmt_t));
	if (p_pmt == NULL)
	{
		*p_table = NULL;
		return AM_SI_ERR_NO_MEM;
	}

	/*Init the p_pmt*/
	dvbpsi_InitPMT(p_pmt,
			p_section->i_extension,
			p_section->i_version,
			p_section->b_current_next,
			((uint16_t)(p_section->p_payload_start[0] & 0x1f) << 8)\
			| (uint16_t)p_section->p_payload_start[1]);
	/*Decode*/
	dvbpsi_DecodePMTSections(p_pmt, p_section);

	p_pmt->i_table_id = p_section->i_table_id;
	*p_table = (void*)p_pmt;

	return AM_SUCCESS;
}

/**\brief 从dvbpsi_psi_section_t结构创建CAT表*/
static AM_ErrorCode_t si_decode_cat(void **p_table, dvbpsi_psi_section_t *p_section)
{
	dvbpsi_cat_t *p_cat;

	assert(p_table && p_section);

	/*Allocate a new table*/
	p_cat = (dvbpsi_cat_t*)malloc(sizeof(dvbpsi_cat_t));
	if (p_cat == NULL)
	{
		*p_table = NULL;
		return AM_SI_ERR_NO_MEM;
	}

	/*Init the p_cat*/
	dvbpsi_InitCAT(p_cat,
			p_section->i_version,
			p_section->b_current_next);
	/*Decode*/
	dvbpsi_DecodeCATSections(p_cat, p_section);

	p_cat->i_table_id = p_section->i_table_id;
	*p_table = (void*)p_cat;

	return AM_SUCCESS;
}

/**\brief 从dvbpsi_psi_section_t结构创建NIT表*/
static AM_ErrorCode_t si_decode_nit(void **p_table, dvbpsi_psi_section_t *p_section)
{
	dvbpsi_nit_t *p_nit;

	assert(p_table && p_section);

	/*Allocate a new table*/
	p_nit = (dvbpsi_nit_t*)malloc(sizeof(dvbpsi_nit_t));
	if (p_nit == NULL)
	{
		*p_table = NULL;
		return AM_SI_ERR_NO_MEM;
	}

	/*Init the p_nit*/
	dvbpsi_InitNIT(p_nit,
			p_section->i_extension,
			p_section->i_version,
			p_section->b_current_next);
	/*Decode*/
	dvbpsi_DecodeNITSections(p_nit, p_section);

	p_nit->i_table_id = p_section->i_table_id;
	*p_table = (void*)p_nit;

	return AM_SUCCESS;
}

/**\brief 从dvbpsi_psi_section_t结构创建BAT表*/
static AM_ErrorCode_t si_decode_bat(void **p_table, dvbpsi_psi_section_t *p_section)
{
	dvbpsi_bat_t *p_bat;

	assert(p_table && p_section);

	/*Allocate a new table*/
	p_bat = (dvbpsi_bat_t*)malloc(sizeof(dvbpsi_bat_t));
	if (p_bat == NULL)
	{
		*p_table = NULL;
		return AM_SI_ERR_NO_MEM;
	}

	/*Init the p_bat*/
	dvbpsi_InitBAT(p_bat,
			p_section->i_extension,
			p_section->i_version,
			p_section->b_current_next);
	/*Decode*/
	dvbpsi_DecodeBATSections(p_bat, p_section);

	p_bat->i_table_id = p_section->i_table_id;
	*p_table = (void*)p_bat;

	return AM_SUCCESS;
}


/**\brief 从dvbpsi_psi_section_t结构创建SDT表*/
static AM_ErrorCode_t si_decode_sdt(void **p_table, dvbpsi_psi_section_t *p_section)
{
	dvbpsi_sdt_t *p_sdt;

	assert(p_table && p_section);

	/*Allocate a new table*/
	p_sdt = (dvbpsi_sdt_t*)malloc(sizeof(dvbpsi_sdt_t));
	if (p_sdt == NULL)
	{
		*p_table = NULL;
		return AM_SI_ERR_NO_MEM;
	}

	/*Init the p_sdt*/
	dvbpsi_InitSDT(p_sdt,
			p_section->i_extension,
			p_section->i_version,
			p_section->b_current_next,
			((uint16_t)(p_section->p_payload_start[0]) << 8)\
			| (uint16_t)p_section->p_payload_start[1]);
	/*Decode*/
	dvbpsi_DecodeSDTSections(p_sdt, p_section);

	p_sdt->i_table_id = p_section->i_table_id;
	*p_table = (void*)p_sdt;

	return AM_SUCCESS;
}

/**\brief 从dvbpsi_psi_section_t结构创建EIT表*/
static AM_ErrorCode_t si_decode_eit(void **p_table, dvbpsi_psi_section_t *p_section)
{
	dvbpsi_eit_t *p_eit;

	assert(p_table && p_section);

	/*Allocate a new table*/
	p_eit = (dvbpsi_eit_t*)malloc(sizeof(dvbpsi_eit_t));
	if (p_eit == NULL)
	{
		*p_table = NULL;
		return AM_SI_ERR_NO_MEM;
	}

	/*Init the p_eit*/
	dvbpsi_InitEIT(p_eit,
			p_section->i_extension,
			p_section->i_version,
			p_section->b_current_next,
			((uint16_t)(p_section->p_payload_start[0]) << 8)\
			| (uint16_t)p_section->p_payload_start[1],
			((uint16_t)(p_section->p_payload_start[2]) << 8)\
			| (uint16_t)p_section->p_payload_start[3],
			p_section->p_payload_start[4],
			p_section->p_payload_start[5]);
	/*Decode*/
	dvbpsi_DecodeEITSections(p_eit, p_section);

	p_eit->i_table_id = p_section->i_table_id;
	*p_table = (void*)p_eit;

	return AM_SUCCESS;
}

/**\brief 从dvbpsi_psi_section_t结构创建TOT表*/
static AM_ErrorCode_t si_decode_tot(void **p_table, dvbpsi_psi_section_t *p_section)
{
	dvbpsi_tot_t *p_tot;

	assert(p_table && p_section);

	/*Allocate a new table*/
	p_tot = (dvbpsi_tot_t*)malloc(sizeof(dvbpsi_tot_t));
	if (p_tot == NULL)
	{
		*p_table = NULL;
		return AM_SI_ERR_NO_MEM;
	}

	/*Init the p_tot*/
	dvbpsi_InitTOT(p_tot,
			((uint64_t)p_section->p_payload_start[0] << 32)\
			| ((uint64_t)p_section->p_payload_start[1] << 24)\
			| ((uint64_t)p_section->p_payload_start[2] << 16)\
			| ((uint64_t)p_section->p_payload_start[3] <<  8)\
			|  (uint64_t)p_section->p_payload_start[4]);
	/*Decode*/
	dvbpsi_DecodeTOTSections(p_tot, p_section);

	p_tot->i_table_id = p_section->i_table_id;
	*p_table = (void*)p_tot;

	return AM_SUCCESS;
}

static AM_ErrorCode_t si_decode_atsc_mgt(void **p_table, dvbpsi_psi_section_t *p_section)
{
	dvbpsi_atsc_mgt_t *p_mgt;

	assert(p_table && p_section);

	/*Allocate a new table*/
	p_mgt = (dvbpsi_atsc_mgt_t*)malloc(sizeof(dvbpsi_atsc_mgt_t));
	if (p_mgt == NULL)
	{
		*p_table = NULL;
		return AM_SI_ERR_NO_MEM;
	}

	/*Init the p_mgt*/
	dvbpsi_atsc_InitMGT(p_mgt,
			p_section->i_table_id,
			p_section->i_extension,
			p_section->i_version,
			p_section->p_payload_start[0],
			p_section->b_current_next);

	/*Decode*/
	dvbpsi_atsc_DecodeMGTSections(p_mgt, p_section);

	p_mgt->i_table_id = p_section->i_table_id;
	*p_table = (void*)p_mgt;

	return AM_SUCCESS;
}

static AM_ErrorCode_t si_decode_atsc_vct(void **p_table, dvbpsi_psi_section_t *p_section)
{
	dvbpsi_atsc_vct_t *p_vct;

	assert(p_table && p_section);

	/*Allocate a new table*/
	p_vct = (dvbpsi_atsc_vct_t*)malloc(sizeof(dvbpsi_atsc_vct_t));
	if (p_vct == NULL)
	{
		*p_table = NULL;
		return AM_SI_ERR_NO_MEM;
	}

	/*Init the p_vct*/
	dvbpsi_atsc_InitVCT(p_vct,
			p_section->i_table_id,
			p_section->i_extension,
			p_section->p_payload_start[0],
			p_section->i_table_id == 0xC9,
			p_section->i_version,
			p_section->b_current_next);

	/*Decode*/
	dvbpsi_atsc_DecodeVCTSections(p_vct, p_section);

	p_vct->i_table_id = p_section->i_table_id;
	*p_table = (void*)p_vct;

	return AM_SUCCESS;
}

static AM_ErrorCode_t si_decode_atsc_stt(void **p_table, dvbpsi_psi_section_t *p_section)
{
	dvbpsi_atsc_stt_t *p_stt;

	assert(p_table && p_section);

	/*Allocate a new table*/
	p_stt = (dvbpsi_atsc_stt_t*)malloc(sizeof(dvbpsi_atsc_stt_t));
	if (p_stt == NULL)
	{
		*p_table = NULL;
		return AM_SI_ERR_NO_MEM;
	}

	/*Init the p_stt*/
	dvbpsi_atsc_InitSTT(p_stt,
			p_section->i_table_id,
			p_section->i_extension,
			p_section->i_version,
			p_section->b_current_next);

	/*Decode*/
	dvbpsi_atsc_DecodeSTTSections(p_stt, p_section);

	p_stt->i_table_id = p_section->i_table_id;
	*p_table = (void*)p_stt;

	return AM_SUCCESS;
}

static AM_ErrorCode_t si_decode_atsc_eit(void **p_table, dvbpsi_psi_section_t *p_section)
{
	dvbpsi_atsc_eit_t *p_eit;

	assert(p_table && p_section);

	/*Allocate a new table*/
	p_eit = (dvbpsi_atsc_eit_t*)malloc(sizeof(dvbpsi_atsc_eit_t));
	if (p_eit == NULL)
	{
		*p_table = NULL;
		return AM_SI_ERR_NO_MEM;
	}

	/*Init the p_eit*/
	dvbpsi_atsc_InitEIT(p_eit,
			p_section->i_table_id,
			p_section->i_extension,
			p_section->i_version,
			p_section->p_payload_start[0],
			p_section->i_extension,
			p_section->b_current_next);

	/*Decode*/
	dvbpsi_atsc_DecodeEITSections(p_eit, p_section);

	p_eit->i_table_id = p_section->i_table_id;
	*p_table = (void*)p_eit;

	return AM_SUCCESS;
}

static AM_ErrorCode_t si_decode_atsc_cea(void **p_table, dvbpsi_psi_section_t *p_section)
{
	dvbpsi_atsc_cea_t *p_cea;

	assert(p_table && p_section);

	/*Allocate a new table*/
	p_cea = (dvbpsi_atsc_cea_t*)malloc(sizeof(dvbpsi_atsc_cea_t));
	if (p_cea == NULL)
	{
		*p_table = NULL;
		return AM_SI_ERR_NO_MEM;
	}

	/*Init the p_cea*/
	dvbpsi_atsc_InitCEA(p_cea,
			p_section->i_table_id,
			p_section->i_extension,
			p_section->i_version,
			p_section->b_current_next);

	/*Decode*/
	dvbpsi_atsc_DecodeCEASections(p_cea, p_section);

	p_cea->i_table_id = p_section->i_table_id;
	*p_table = (void*)p_cea;
	return AM_SUCCESS;
}

static AM_ErrorCode_t si_decode_atsc_ett(void **p_table, dvbpsi_psi_section_t *p_section)
{
	dvbpsi_atsc_ett_t *p_ett;

	assert(p_table && p_section);

	/*Allocate a new table*/
	p_ett = (dvbpsi_atsc_ett_t*)malloc(sizeof(dvbpsi_atsc_ett_t));
	if (p_ett == NULL)
	{
		*p_table = NULL;
		return AM_SI_ERR_NO_MEM;
	}

	/*Init the p_ett*/
	uint32_t i_etm_id = ((uint32_t)p_section->p_payload_start[1] << 24) |
		((uint32_t)p_section->p_payload_start[2] << 16) |
		((uint32_t)p_section->p_payload_start[3] << 8)  |
		((uint32_t)p_section->p_payload_start[4] << 0);

	dvbpsi_atsc_InitETT(p_ett,
			p_section->i_table_id,
			p_section->i_extension,
			p_section->i_version,
			p_section->p_payload_start[0],
			i_etm_id,
			p_section->b_current_next);

	/*Decode*/
	dvbpsi_atsc_DecodeETTSections(p_ett, p_section);

	p_ett->i_table_id = p_section->i_table_id;
	*p_table = (void*)p_ett;

	return AM_SUCCESS;
}


/**\brief 检查句柄是否有效*/
static AM_INLINE AM_ErrorCode_t si_check_handle(AM_SI_Handle_t handle)
{
	if (handle && ((SI_Decoder_t*)handle)->allocated &&
		(((SI_Decoder_t*)handle)->prv_data == si_prv_data))
		return AM_SUCCESS;

	AM_DEBUG(1, "SI Decoder got invalid handle");
	return AM_SI_ERR_INVALID_HANDLE;
}

/**\brief 解析一个section头*/
static AM_ErrorCode_t si_get_section_header(uint8_t *buf, AM_SI_SectionHeader_t *sec_header)
{
	assert(buf && sec_header);

	sec_header->table_id = buf[0];
	sec_header->syntax_indicator = (buf[1] & 0x80) >> 7;
	sec_header->private_indicator = (buf[1] & 0x40) >> 6;
	sec_header->length = (((uint16_t)(buf[1] & 0x0f)) << 8) | (uint16_t)buf[2];
	sec_header->extension = ((uint16_t)buf[3] << 8) | (uint16_t)buf[4];
	sec_header->version = (buf[5] & 0x3e) >> 1;
	sec_header->cur_next_indicator = buf[5] & 0x1;
	sec_header->sec_num = buf[6];
	sec_header->last_sec_num = buf[7];

	return AM_SUCCESS;
}

/**\brief 从section原始数据生成dvbpsi_psi_section_t类型的数据*/
static AM_ErrorCode_t si_gen_dvbpsi_section(uint8_t *buf, uint16_t len, dvbpsi_psi_section_t **psi_sec)
{
	dvbpsi_psi_section_t * p_section;
	AM_SI_SectionHeader_t header;

	assert(buf && psi_sec);

	/*Check the section header*/
	AM_TRY(si_get_section_header(buf, &header));
	if ((header.length + 3) != len)
	{
		AM_DEBUG(1, "Invalid section header");
		return AM_SI_ERR_INVALID_SECTION_DATA;
	}

	/* Allocate the dvbpsi_psi_section_t structure */
	p_section  = (dvbpsi_psi_section_t*)malloc(sizeof(dvbpsi_psi_section_t));
	if (p_section == NULL)
	{
		AM_DEBUG(1, "Cannot alloc new psi section, no enough memory");
		return AM_SI_ERR_NO_MEM;
	}

	/*Fill the p_section*/
	p_section->i_table_id = header.table_id;
	p_section->b_syntax_indicator = header.syntax_indicator;
	p_section->b_private_indicator = header.private_indicator;
	p_section->i_length = header.length;
	p_section->i_extension = header.extension;
	p_section->i_version = header.version;
	p_section->b_current_next = header.cur_next_indicator;
	p_section->i_number = header.sec_num;
	p_section->i_last_number = header.last_sec_num;
	p_section->p_data = buf;
	if (header.table_id == AM_SI_TID_TDT || header.table_id == AM_SI_TID_TOT)
	{
		p_section->p_payload_start = buf + 3;
		p_section->p_payload_end = buf + len;
	}
	else
	{
		p_section->p_payload_start = buf + 8;
		p_section->p_payload_end = buf + len - 4;
	}
	p_section->p_next = NULL;

 	*psi_sec = p_section;

 	return AM_SUCCESS;
}

void si_decode_descriptor_ex(dvbpsi_descriptor_t *descr, SI_Descriptor_Flag_t flag)
{
	assert(descr);
	/*Decode*/
	switch (descr->i_tag)
	{
		SI_ADD_DESCR_DECODE_FUNC(AM_SI_DESCR_VIDEO_STREAM,	dvbpsi_DecodeVStreamDr)
		SI_ADD_DESCR_DECODE_FUNC(AM_SI_DESCR_AUDIO_STREAM,	dvbpsi_DecodeAStreamDr)
		SI_ADD_DESCR_DECODE_FUNC(AM_SI_DESCR_HIERARCHY,	  	dvbpsi_DecodeHierarchyDr)
		SI_ADD_DESCR_DECODE_FUNC(AM_SI_DESCR_REGISTRATION,	dvbpsi_DecodeRegistrationDr)
		SI_ADD_DESCR_DECODE_FUNC(AM_SI_DESCR_DS_ALIGNMENT,	dvbpsi_DecodeDSAlignmentDr)
		SI_ADD_DESCR_DECODE_FUNC(AM_SI_DESCR_TARGET_BG_GRID,dvbpsi_DecodeTargetBgGridDr)
		SI_ADD_DESCR_DECODE_FUNC(AM_SI_DESCR_VIDEO_WINDOW,	dvbpsi_DecodeVWindowDr)
		SI_ADD_DESCR_DECODE_FUNC(AM_SI_DESCR_CA,			dvbpsi_DecodeCADr)
		SI_ADD_DESCR_DECODE_FUNC(AM_SI_DESCR_ISO639,		dvbpsi_DecodeISO639Dr)
		SI_ADD_DESCR_DECODE_FUNC(AM_SI_DESCR_SYSTEM_CLOCK,	dvbpsi_DecodeSystemClockDr)
		SI_ADD_DESCR_DECODE_FUNC(AM_SI_DESCR_MULX_BUF_UTILIZATION,  dvbpsi_DecodeMxBuffUtilizationDr)
		SI_ADD_DESCR_DECODE_FUNC(AM_SI_DESCR_COPYRIGHT,		dvbpsi_DecodeCopyrightDr)
		SI_ADD_DESCR_DECODE_FUNC(AM_SI_DESCR_MAX_BITRATE,	dvbpsi_DecodeMaxBitrateDr)
		SI_ADD_DESCR_DECODE_FUNC(AM_SI_DESCR_PRIVATE_DATA_INDICATOR,dvbpsi_DecodePrivateDataDr)
		SI_ADD_DESCR_DECODE_FUNC(AM_SI_DESCR_NETWORK_NAME, 	dvbpsi_DecodeNetworkNameDr)
		SI_ADD_DESCR_DECODE_FUNC(AM_SI_DESCR_SERVICE_LIST, 	dvbpsi_DecodeServiceListDr)
		SI_ADD_DESCR_DECODE_FUNC(AM_SI_DESCR_STUFFING, 		dvbpsi_DecodeStuffingDr)
		SI_ADD_DESCR_DECODE_FUNC(AM_SI_DESCR_SATELLITE_DELIVERY, dvbpsi_DecodeSatDelivSysDr)
		SI_ADD_DESCR_DECODE_FUNC(AM_SI_DESCR_CABLE_DELIVERY, dvbpsi_DecodeCableDeliveryDr)
		SI_ADD_DESCR_DECODE_FUNC(AM_SI_DESCR_VBI_DATA, 		dvbpsi_DecodeVBIDataDr)
		/*SI_ADD_DESCR_DECODE_FUNC(AM_SI_DESCR_VBI_TELETEXT, 			NULL)*/
		SI_ADD_DESCR_DECODE_FUNC(AM_SI_DESCR_BOUQUET_NAME, 	dvbpsi_DecodeBouquetNameDr)
		SI_ADD_DESCR_DECODE_FUNC(AM_SI_DESCR_SERVICE, 		dvbpsi_DecodeServiceDr)
		SI_ADD_DESCR_DECODE_FUNC(AM_SI_DESCR_LINKAGE, 		dvbpsi_DecodeLinkageDr)
		/*SI_ADD_DESCR_DECODE_FUNC(AM_SI_DESCR_NVOD_REFERENCE, 		NULL)*/
		/*SI_ADD_DESCR_DECODE_FUNC(AM_SI_DESCR_TIME_SHIFTED_SERVICE,	NULL)*/
		SI_ADD_DESCR_DECODE_FUNC(AM_SI_DESCR_SHORT_EVENT, 	dvbpsi_DecodeShortEventDr)
		SI_ADD_DESCR_DECODE_FUNC(AM_SI_DESCR_EXTENDED_EVENT,dvbpsi_DecodeExtendedEventDr)
		/*SI_ADD_DESCR_DECODE_FUNC(AM_SI_DESCR_TIME_SHIFTED_EVENT, 	NULL)*/
		/*SI_ADD_DESCR_DECODE_FUNC(AM_SI_DESCR_COMPONENT, 			NULL)*/
		/*SI_ADD_DESCR_DECODE_FUNC(AM_SI_DESCR_MOSAIC, 				NULL)*/
		SI_ADD_DESCR_DECODE_FUNC(AM_SI_DESCR_STREAM_IDENTIFIER, dvbpsi_DecodeStreamIdentifierDr)
		/*SI_ADD_DESCR_DECODE_FUNC(AM_SI_DESCR_CA_IDENTIFIER, 		NULL)*/
		SI_ADD_DESCR_DECODE_FUNC(AM_SI_DESCR_CONTENT, 				dvbpsi_DecodeContentDr)
		SI_ADD_DESCR_DECODE_FUNC(AM_SI_DESCR_PARENTAL_RATING, dvbpsi_DecodeParentalRatingDr)
		SI_ADD_DESCR_DECODE_FUNC(AM_SI_DESCR_TELETEXT, 		dvbpsi_DecodeTeletextDr)
		/*SI_ADD_DESCR_DECODE_FUNC(AM_SI_DESCR_TELPHONE, 				NULL)*/
		SI_ADD_DESCR_DECODE_FUNC(AM_SI_DESCR_LOCAL_TIME_OFFSET,	dvbpsi_DecodeLocalTimeOffsetDr)
		SI_ADD_DESCR_DECODE_FUNC(AM_SI_DESCR_SUBTITLING, 		dvbpsi_DecodeSubtitlingDr)
		SI_ADD_DESCR_DECODE_FUNC(AM_SI_DESCR_TERRESTRIAL_DELIVERY,dvbpsi_DecodeTerrDelivSysDr)

		/*SI_ADD_DESCR_DECODE_FUNC(AM_SI_DESCR_MULTI_NETWORK_NAME, 	NULL)*/
		/*SI_ADD_DESCR_DECODE_FUNC(AM_SI_DESCR_MULTI_BOUQUET_NAME, 	NULL)*/
		SI_ADD_DESCR_DECODE_FUNC(AM_SI_DESCR_MULTI_SERVICE_NAME, 	dvbpsi_DecodeMultiServiceNameDr)
		/*SI_ADD_DESCR_DECODE_FUNC(AM_SI_DESCR_MULTI_COMPONENT, 		NULL)*/
		/*SI_ADD_DESCR_DECODE_FUNC(AM_SI_DESCR_DATA_BROADCAST, 		NULL)*/
		/*SI_ADD_DESCR_DECODE_FUNC(AM_SI_DESCR_DATA_BROADCAST_ID, 	NULL)*/
		SI_ADD_DESCR_DECODE_FUNC(AM_SI_DESCR_PDC, 			dvbpsi_DecodePDCDr)
		SI_ADD_DESCR_DECODE_FUNC(AM_SI_DESCR_LCN_83, 			dvbpsi_DecodeLogicalChannelNumber83Dr)
		SI_ADD_DESCR_DECODE_FUNC(AM_SI_DESCR_LCN_88, 			dvbpsi_DecodeLogicalChannelNumber88Dr)
		SI_ADD_DESCR_DECODE_FUNC(AM_SI_DESCR_AC3, 			dvbpsi_DecodeAC3Dr)
		SI_ADD_DESCR_DECODE_FUNC(AM_SI_DESCR_ENHANCED_AC3, 	dvbpsi_DecodeENAC3Dr)
		SI_ADD_DESCR_DECODE_FUNC(AM_SI_DESCR_PSIPENHANCED_AC3, 	dvbpsi_DecodePSIPENAC3Dr)
		SI_ADD_DESCR_DECODE_FUNC(AM_SI_DESCR_PSIP_AUDIOSTREAM_AC3, 	dvbpsi_decode_atsc_ac3_audio_dr)
		SI_ADD_DESCR_DECODE_FUNC(AM_SI_DESCR_EXTENSION, 	dvbpsi_DecodeEXTENTIONDr)
		SI_ADD_DESCR_DECODE_FUNC(AM_SI_DESCR_CAPTION_SERVICE,  dvbpsi_decode_atsc_caption_service_dr)
		SI_ADD_DESCR_DECODE_FUNC(AM_SI_DESCR_SERVICE_LOCATION, dvbpsi_decode_atsc_service_location_dr)
		default:
			break;
	}

	switch (descr->i_tag) {
	case AM_SI_DESCR_CONTENT_ADVISORY: {
		//AM_DEBUG(1, "PMT [%x][%x][%x]", flag, SI_DESCR_87_LCN, SI_DESCR_87_CA);
		if ((flag & SI_DESCR_87_LCN) == SI_DESCR_87_LCN)
			dvbpsi_DecodeLogicalChannelNumber87Dr(descr);
		else if ((flag & SI_DESCR_87_CA) == SI_DESCR_87_CA)
			dvbpsi_decode_atsc_content_advisory_dr(descr);
		else
			dvbpsi_decode_atsc_content_advisory_dr(descr);
		} break;
	default:
		break;
	}
}

/**\brief 解析一个描述符,自行查找解析函数,为libdvbsi调用*/
void si_decode_descriptor(dvbpsi_descriptor_t *descr, void *user)
{
#ifdef ANDROID
	assert(des);
#endif
	SI_Descriptor_Flag_t flag = (SI_Descriptor_Flag_t)(long)user;

	si_decode_descriptor_ex(descr, flag);
}


/**\brief 将ISO6937转换为UTF-8编码*/
static AM_ErrorCode_t si_convert_iso6937_to_utf8(const char *src, int src_len, char *dest, int *dest_len)
{
	uint16_t ch;
	long dlen, i;
	uint8_t b;
	char *ucs2 = NULL;

#define READ_BYTE()\
	({\
		uint8_t ret;\
		if (i >= src_len) ret=0;\
		else ret = (uint8_t)src[i];\
		i++;\
		ret;\
	})

	if (!src || !dest || !dest_len || src_len <= 0)
		return -1;

	/* first covert to UCS-2, then iconv to utf8 */
	ucs2 = (char *)malloc(src_len*2);
	if (!ucs2)
		return -1;
	dlen = 0;
	i=0;
	b = READ_BYTE();
	if (b < 0x20)
	{
		/* ISO 6937 encoding must start with character between 0x20 and 0xFF
		 otherwise it is dfferent encoding table
		 for example 0x05 means encoding table 8859-9 */
		return -1;
	}

	while (b != 0)
	{
		ch = 0x00;
		switch (b)
		{
			/* at first single byte characters */
			case 0xA8: ch = 0x00A4; break;
			case 0xA9: ch = 0x2018; break;
			case 0xAA: ch = 0x201C; break;
			case 0xAC: ch = 0x2190; break;
			case 0xAD: ch = 0x2191; break;
			case 0xAE: ch = 0x2192; break;
			case 0xAF: ch = 0x2193; break;
			case 0xB4: ch = 0x00D7; break;
			case 0xB8: ch = 0x00F7; break;
			case 0xB9: ch = 0x2019; break;
			case 0xBA: ch = 0x201D; break;
			case 0xD0: ch = 0x2014; break;
			case 0xD1: ch = 0xB9; break;
			case 0xD2: ch = 0xAE; break;
			case 0xD3: ch = 0xA9; break;
			case 0xD4: ch = 0x2122; break;
			case 0xD5: ch = 0x266A; break;
			case 0xD6: ch = 0xAC; break;
			case 0xD7: ch = 0xA6; break;
			case 0xDC: ch = 0x215B; break;
			case 0xDD: ch = 0x215C; break;
			case 0xDE: ch = 0x215D; break;
			case 0xDF: ch = 0x215E; break;
			case 0xE0: ch = 0x2126; break;
			case 0xE1: ch = 0xC6; break;
			case 0xE2: ch = 0xD0; break;
			case 0xE3: ch = 0xAA; break;
			case 0xE4: ch = 0x126; break;
			case 0xE6: ch = 0x132; break;
			case 0xE7: ch = 0x013F; break;
			case 0xE8: ch = 0x141; break;
			case 0xE9: ch = 0xD8; break;
			case 0xEA: ch = 0x152; break;
			case 0xEB: ch = 0xBA; break;
			case 0xEC: ch = 0xDE; break;
			case 0xED: ch = 0x166; break;
			case 0xEE: ch = 0x014A; break;
			case 0xEF: ch = 0x149; break;
			case 0xF0: ch = 0x138; break;
			case 0xF1: ch = 0xE6; break;
			case 0xF2: ch = 0x111; break;
			case 0xF3: ch = 0xF0; break;
			case 0xF4: ch = 0x127; break;
			case 0xF5: ch = 0x131; break;
			case 0xF6: ch = 0x133; break;
			case 0xF7: ch = 0x140; break;
			case 0xF8: ch = 0x142; break;
			case 0xF9: ch = 0xF8; break;
			case 0xFA: ch = 0x153; break;
			case 0xFB: ch = 0xDF; break;
			case 0xFC: ch = 0xFE; break;
			case 0xFD: ch = 0x167; break;
			case 0xFE: ch = 0x014B; break;
			case 0xFF: ch = 0xAD; break;
			/* multibyte */
			case 0xC1:
				b = READ_BYTE();
				switch (b)
				{
					case 0x41: ch = 0xC0; break;
					case 0x45: ch = 0xC8; break;
					case 0x49: ch = 0xCC; break;
					case 0x4F: ch = 0xD2; break;
					case 0x55: ch = 0xD9; break;
					case 0x61: ch = 0xE0; break;
					case 0x65: ch = 0xE8; break;
					case 0x69: ch = 0xEC; break;
					case 0x6F: ch = 0xF2; break;
					case 0x75: ch = 0xF9; break;
					// unknown character --> fallback
					default: ch = b; break;
				}
				break;
			case 0xC2:
				b = READ_BYTE();
				switch (b)
				{
					case 0x20: ch = 0xB4; break;
					case 0x41: ch = 0xC1; break;
					case 0x43: ch = 0x106; break;
					case 0x45: ch = 0xC9; break;
					case 0x49: ch = 0xCD; break;
					case 0x4C: ch = 0x139; break;
					case 0x4E: ch = 0x143; break;
					case 0x4F: ch = 0xD3; break;
					case 0x52: ch = 0x154; break;
					case 0x53: ch = 0x015A; break;
					case 0x55: ch = 0xDA; break;
					case 0x59: ch = 0xDD; break;
					case 0x5A: ch = 0x179; break;
					case 0x61: ch = 0xE1; break;
					case 0x63: ch = 0x107; break;
					case 0x65: ch = 0xE9; break;
					case 0x69: ch = 0xED; break;
					case 0x6C: ch = 0x013A; break;
					case 0x6E: ch = 0x144; break;
					case 0x6F: ch = 0xF3; break;
					case 0x72: ch = 0x155; break;
					case 0x73: ch = 0x015B; break;
					case 0x75: ch = 0xFA; break;
					case 0x79: ch = 0xFD; break;
					case 0x7A: ch = 0x017A; break;
					// unknown character --> fallback
					default: ch = b; break;
				}
				break;

			case 0xC3:
				b = READ_BYTE();
				switch (b)
				{
					case 0x41: ch = 0xC2; break;
					case 0x43: ch = 0x108; break;
					case 0x45: ch = 0xCA; break;
					case 0x47: ch = 0x011C; break;
					case 0x48: ch = 0x124; break;
					case 0x49: ch = 0xCE; break;
					case 0x4A: ch = 0x134; break;
					case 0x4F: ch = 0xD4; break;
					case 0x53: ch = 0x015C; break;
					case 0x55: ch = 0xDB; break;
					case 0x57: ch = 0x174; break;
					case 0x59: ch = 0x176; break;
					case 0x61: ch = 0xE2; break;
					case 0x63: ch = 0x109; break;
					case 0x65: ch = 0xEA; break;
					case 0x67: ch = 0x011D; break;
					case 0x68: ch = 0x125; break;
					case 0x69: ch = 0xEE; break;
					case 0x6A: ch = 0x135; break;
					case 0x6F: ch = 0xF4; break;
					case 0x73: ch = 0x015D; break;
					case 0x75: ch = 0xFB; break;
					case 0x77: ch = 0x175; break;
					case 0x79: ch = 0x177; break;
					// unknown character --> fallback
					default: ch = b; break;
				}
				break;
			case 0xC4:
				b = READ_BYTE();
				switch (b)
				{
					case 0x41: ch = 0xC3; break;
					case 0x49: ch = 0x128; break;
					case 0x4E: ch = 0xD1; break;
					case 0x4F: ch = 0xD5; break;
					case 0x55: ch = 0x168; break;
					case 0x61: ch = 0xE3; break;
					case 0x69: ch = 0x129; break;
					case 0x6E: ch = 0xF1; break;
					case 0x6F: ch = 0xF5; break;
					case 0x75: ch = 0x169; break;
					// unknown character --> fallback
					default: ch = b; break;
				}
				break;
			case 0xC5:
				b = READ_BYTE();
				switch (b)
				{
					case 0x20: ch = 0xAF; break;
					case 0x41: ch = 0x100; break;
					case 0x45: ch = 0x112; break;
					case 0x49: ch = 0x012A; break;
					case 0x4F: ch = 0x014C; break;
					case 0x55: ch = 0x016A; break;
					case 0x61: ch = 0x101; break;
					case 0x65: ch = 0x113; break;
					case 0x69: ch = 0x012B; break;
					case 0x6F: ch = 0x014D; break;
					case 0x75: ch = 0x016B; break;
					// unknown character --> fallback
					default: ch = b; break;
				}
				break;
			case 0xC6:
				b = READ_BYTE();
				switch (b)
				{
					case 0x20: ch = 0x02D8; break;
					case 0x41: ch = 0x102; break;
					case 0x47: ch = 0x011E; break;
					case 0x55: ch = 0x016C; break;
					case 0x61: ch = 0x103; break;
					case 0x67: ch = 0x011F; break;
					case 0x75: ch = 0x016D; break;
					// unknown character --> fallback
					default: ch = b; break;
				}
				break;
			case 0xC7:
				b = READ_BYTE();
				switch (b)
				{
					case 0x20: ch = 0x02D9; break;
					case 0x43: ch = 0x010A; break;
					case 0x45: ch = 0x116; break;
					case 0x47: ch = 0x120; break;
					case 0x49: ch = 0x130; break;
					case 0x5A: ch = 0x017B; break;
					case 0x63: ch = 0x010B; break;
					case 0x65: ch = 0x117; break;
					case 0x67: ch = 0x121; break;
					case 0x7A: ch = 0x017C; break;
					// unknown character --> fallback
					default: ch = b; break;
				}
				break;
			case 0xC8:
				b = READ_BYTE();
				switch (b)
				{
					case 0x20: ch = 0xA8; break;
					case 0x41: ch = 0xC4; break;
					case 0x45: ch = 0xCB; break;
					case 0x49: ch = 0xCF; break;
					case 0x4F: ch = 0xD6; break;
					case 0x55: ch = 0xDC; break;
					case 0x59: ch = 0x178; break;
					case 0x61: ch = 0xE4; break;
					case 0x65: ch = 0xEB; break;
					case 0x69: ch = 0xEF; break;
					case 0x6F: ch = 0xF6; break;
					case 0x75: ch = 0xFC; break;
					case 0x79: ch = 0xFF; break;
					// unknown character --> fallback
					default: ch = b; break;
				}
				break;
			case 0xCA:
				b = READ_BYTE();
				switch (b)
				{
					case 0x20: ch = 0x02DA; break;
					case 0x41: ch = 0xC5; break;
					case 0x55: ch = 0x016E; break;
					case 0x61: ch = 0xE5; break;
					case 0x75: ch = 0x016F; break;
					// unknown character --> fallback
					default: ch = b; break;
				}
				break;
			case 0xCB:
				b = READ_BYTE();
				switch (b)
				{
					case 0x20: ch = 0xB8; break;
					case 0x43: ch = 0xC7; break;
					case 0x47: ch = 0x122; break;
					case 0x4B: ch = 0x136; break;
					case 0x4C: ch = 0x013B; break;
					case 0x4E: ch = 0x145; break;
					case 0x52: ch = 0x156; break;
					case 0x53: ch = 0x015E; break;
					case 0x54: ch = 0x162; break;
					case 0x63: ch = 0xE7; break;
					case 0x67: ch = 0x123; break;
					case 0x6B: ch = 0x137; break;
					case 0x6C: ch = 0x013C; break;
					case 0x6E: ch = 0x146; break;
					case 0x72: ch = 0x157; break;
					case 0x73: ch = 0x015F; break;
					case 0x74: ch = 0x163; break;
					// unknown character --> fallback
					default: ch = b; break;
				}
				break;
			case 0xCD:
				b = READ_BYTE();
				switch (b)
				{
					case 0x20: ch = 0x02DD; break;
					case 0x4F: ch = 0x150; break;
					case 0x55: ch = 0x170; break;
					case 0x6F: ch = 0x151; break;
					case 0x75: ch = 0x171; break;
					// unknown character --> fallback
					default: ch = b; break;
				}
				break;
			case 0xCE:
				b = READ_BYTE();
				switch (b)
				{
					case 0x20: ch = 0x02DB; break;
					case 0x41: ch = 0x104; break;
					case 0x45: ch = 0x118; break;
					case 0x49: ch = 0x012E; break;
					case 0x55: ch = 0x172; break;
					case 0x61: ch = 0x105; break;
					case 0x65: ch = 0x119; break;
					case 0x69: ch = 0x012F; break;
					case 0x75: ch = 0x173; break;
					// unknown character --> fallback
					default: ch = b; break;
				}
				break;
			case 0xCF:
				b = READ_BYTE();
				switch (b)
				{
					case 0x20: ch = 0x02C7; break;
					case 0x43: ch = 0x010C; break;
					case 0x44: ch = 0x010E; break;
					case 0x45: ch = 0x011A; break;
					case 0x4C: ch = 0x013D; break;
					case 0x4E: ch = 0x147; break;
					case 0x52: ch = 0x158; break;
					case 0x53: ch = 0x160; break;
					case 0x54: ch = 0x164; break;
					case 0x5A: ch = 0x017D; break;
					case 0x63: ch = 0x010D; break;
					case 0x64: ch = 0x010F; break;
					case 0x65: ch = 0x011B; break;
					case 0x6C: ch = 0x013E; break;
					case 0x6E: ch = 0x148; break;
					case 0x72: ch = 0x159; break;
					case 0x73: ch = 0x161; break;
					case 0x74: ch = 0x165; break;
					case 0x7A: ch = 0x017E; break;
					// unknown character --> fallback
					default: ch = b; break;
				}
				break;
			/* rest is the same */
			default: ch = b; break;
		}
		if (b != 0)
		{
			b = READ_BYTE();
		}
		if (ch != 0)
		{
			/* dest buffer not enough, and this will not happen */
			if ((dlen+1) >= (src_len*2))
				goto iso6937_end;
			ucs2[dlen++] = (ch&0xff00) >> 8;
			ucs2[dlen++] = ch;
		}
	}

iso6937_end:
	if (dlen > 0)
	{
		iconv_t handle;
		char *org_ucs2 = ucs2;
		char **pin=&ucs2;
		char **pout=&dest;
		char *o_dest = dest;
		size_t l_dest = *dest_len;
		size_t l_dlen = dlen;

		handle=iconv_open("utf-8","utf-16");
		if (handle == (iconv_t)-1)
		{
			AM_DEBUG(1, "iconv_open err: %d", errno);
			return AM_FAILURE;
		}

		if ((int)iconv(handle,pin,&l_dlen,pout, &l_dest) < 0)
		{
		    int i;
		    AM_DEBUG(1, "iconv err:%d", errno);
		    iconv_close(handle);
		    return AM_FAILURE;
		}

		*dest_len = l_dest;
		AM_Check_UTF8(o_dest, *dest_len, o_dest, dest_len);

		free(org_ucs2);

		return iconv_close(handle);
    }

    free(ucs2);

    return -1;
}

static void si_add_audio(AM_SI_AudioInfo_t *ai, int aud_pid, int aud_fmt, char lang[3],int audio_type,int audio_exten)
{
	int i, j;
	int i_exten = 0, sub_id;
	int id = 0;
	/*i_exten is 32 bit,31:24 bit: exten type,*/
	/*23:16:mainid or asvc id,8 bit*/
	/*15:0 bit:no use*/
	for (i=0; i<ai->audio_count; i++)
	{
		if (ai->audios[i].pid == aud_pid &&
			ai->audios[i].fmt == aud_fmt &&
			! memcmp(ai->audios[i].lang, lang, 3))
		{
			if ((strncmp(ai->audios[i].lang, "Audio", 5) == 0) && lang[0] != 0) {
				memset(ai->audios[i].lang, 0, sizeof(ai->audios[i].lang));
				memcpy(ai->audios[i].lang, lang, 3);
			}

			AM_DEBUG(1, "Skipping a exist audio: pid %d, fmt %d, lang %c%c%c",
				aud_pid, aud_fmt, lang[0], lang[1], lang[2]);
			return;
		}
	}
	if (ai->audio_count >= AM_SI_MAX_AUD_CNT)
	{
		AM_DEBUG(1, "Too many audios, Max count %d", AM_SI_MAX_AUD_CNT);
		return;
	}
	if (ai->audio_count < 0)
		ai->audio_count = 0;

	ai->audios[ai->audio_count].pid = aud_pid;
	ai->audios[ai->audio_count].fmt = aud_fmt;
	ai->audios[ai->audio_count].audio_type = audio_type;
	/*set audio exten flag*/
	if (aud_fmt == AFORMAT_AC3 || aud_fmt == AFORMAT_EAC3) {
		ai->audios[ai->audio_count].audio_exten = audio_exten;
	} else {
		if (audio_type == 0) {
			id = ai->audio_mainid;
			i_exten = AM_Audio_AC3MAIN<<24 | id<<16;
			ai->audio_mainid++;
			if (ai->audio_mainid >= AM_SI_MAX_MAIN_AUD_CNT) {
				/*max main audio sum is AM_SI_MAX_MAIN_AUD_CNT*/
				ai->audio_mainid = AM_SI_MAX_MAIN_AUD_CNT-1;
			}
			AM_DEBUG(1, "add audio main exten 0x%x mainid index:%d", i_exten, ai->audio_mainid);
		}
		ai->audios[ai->audio_count].audio_exten = i_exten;
	}

	memset(ai->audios[ai->audio_count].lang, 0, sizeof(ai->audios[ai->audio_count].lang));
	if (lang[0] != 0)
	{
		memcpy(ai->audios[ai->audio_count].lang, lang, 3);
	}
	else
	{
		snprintf(ai->audios[ai->audio_count].lang, sizeof(ai->audios[ai->audio_count].lang), "Audio%d", ai->audio_count+1);
	}

	AM_DEBUG(1, "---Add a audio: pid %d, fmt %d, language: %s ,audio_type:%d, audio_exten:0x%x", aud_pid, aud_fmt, ai->audios[ai->audio_count].lang,audio_type,audio_exten);
	ai->audio_count++;
	/*ad sub audio exten when fmt is not ac3*/
	for (i=0; i<ai->audio_count; i++)
	{
		if (ai->audios[i].fmt != AFORMAT_AC3 && ai->audios[i].fmt != AFORMAT_EAC3 && ai->audios[i].audio_type != 0) {
			AM_DEBUG(1,"sub audio lang :%s ", ai->audios[i].lang);
			for (j=0; j<ai->audio_count; j++)
			{
				if (ai->audios[j].fmt != AFORMAT_AC3 && ai->audios[j].audio_type == 0) {
					AM_DEBUG(1,"main audio lang :%s ", ai->audios[j].lang);
					/*get same audio lang*/
					if (!memcmp(ai->audios[i].lang, ai->audios[j].lang, 3)) {
						/*get main audio mainid and set sub audio exten*/
						sub_id = (ai->audios[j].audio_exten&0x00ff0000)>>16;
						AM_DEBUG(1,"sub audio id index :%d ", sub_id);
						sub_id = 1 << sub_id;
						i_exten = AM_Audio_AC3ASVC<<24 | ((sub_id<<16)&0x00ff0000);
						AM_DEBUG(1, "Add a sub audio: audio_exten:0x%x main:0x%x",  i_exten, ai->audios[j].audio_exten);
						break;
					}
				}
			}
			/*set sub audio exten*/
			ai->audios[i].audio_exten = i_exten;
		}
	}
}

/****************************************************************************
 * API functions
 ***************************************************************************/
/**\brief 创建一个SI解析器
 * \param [out] handle 返回SI解析句柄
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_si.h)
 */
AM_ErrorCode_t AM_SI_Create(AM_SI_Handle_t *handle)
{
	SI_Decoder_t *dec;

	assert(handle);

	dec = (SI_Decoder_t *)malloc(sizeof(SI_Decoder_t));
	if (dec == NULL)
	{
		AM_DEBUG(1, "Cannot create SI Decoder, no enough memory");
		return AM_SI_ERR_NO_MEM;
	}
	/*add set decode des callback*/
	AM_DEBUG(1, "dvbpsi_Set_DecodeDescriptor_Callback at si creat");
	dvbpsi_Set_DecodeDescriptor_Callback(si_decode_descriptor);
	//SET_DECODE_DESCRIPTOR_CALLBACK(pat, si_decode_descriptor, 0);
	SET_DECODE_DESCRIPTOR_CALLBACK(pmt, si_decode_descriptor, 0);
	SET_DECODE_DESCRIPTOR_CALLBACK(sdt, si_decode_descriptor, 0);
	SET_DECODE_DESCRIPTOR_CALLBACK(cat, si_decode_descriptor, 0);
	SET_DECODE_DESCRIPTOR_CALLBACK(nit, si_decode_descriptor, SI_DESCR_87_LCN);
	SET_DECODE_DESCRIPTOR_CALLBACK(eit, si_decode_descriptor, 0);
	SET_DECODE_DESCRIPTOR_CALLBACK(tot, si_decode_descriptor, 0);
	SET_DECODE_DESCRIPTOR_CALLBACK(bat, si_decode_descriptor, 0);
	SET_DECODE_DESCRIPTOR_CALLBACK(atsc_mgt, si_decode_descriptor, 0);
	SET_DECODE_DESCRIPTOR_CALLBACK(atsc_vct, si_decode_descriptor, 0);
	SET_DECODE_DESCRIPTOR_CALLBACK(atsc_stt, si_decode_descriptor, 0);
	SET_DECODE_DESCRIPTOR_CALLBACK(atsc_eit, si_decode_descriptor, 0);
	SET_DECODE_DESCRIPTOR_CALLBACK(atsc_ett, si_decode_descriptor, 0);

	dec->prv_data = (void*)si_prv_data;
	dec->allocated = AM_TRUE;

	*handle = dec;

	return AM_SUCCESS;
}

/**\brief 销毀一个SI解析器
 * \param handle SI解析句柄
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_si.h)
 */
AM_ErrorCode_t AM_SI_Destroy(AM_SI_Handle_t handle)
{
	SI_Decoder_t *dec = (SI_Decoder_t*)handle;

	AM_TRY(si_check_handle(handle));

	dec->allocated = AM_FALSE;
	dec->prv_data = NULL;
	free(dec);

	return AM_SUCCESS;
}

/**\brief 解析一个section,并返回解析数据
 * 支持的表(相应返回结构):CAT(dvbpsi_cat_t) PAT(dvbpsi_pat_t) PMT(dvbpsi_pmt_t)
 * SDT(dvbpsi_sdt_t) EIT(dvbpsi_eit_t) TOT(dvbpsi_tot_t) NIT(dvbpsi_nit_t).
 * VCT(vct_section_info_t) MGT(mgt_section_info_t)
 * RRT(rrt_section_info_t) STT(stt_section_info_t)
 * e.g.解析一个PAT section:
 * 	dvbpsi_pat_t *pat_sec;
 * 	AM_SI_DecodeSection(hSI, AM_SI_PID_PAT, pat_buf, len, &pat_sec);
 *
 * \param handle SI解析句柄
 * \param pid section pid
 * \param [in] buf section原始数据
 * \param len section原始数据长度
 * \param [out] sec 返回section解析后的数据
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_si.h)
 */
AM_ErrorCode_t AM_SI_DecodeSection(AM_SI_Handle_t handle, uint16_t pid, uint8_t *buf, uint16_t len, void **sec)
{
	dvbpsi_psi_section_t *psi_sec = NULL;
	AM_ErrorCode_t ret = AM_SUCCESS;
	uint8_t table_id;

	assert(buf && sec);
	AM_TRY(si_check_handle(handle));

	table_id = buf[0];

	if (table_id <= AM_SI_TID_PSIP_CEA)
	{
		/*生成dvbpsi section*/
		AM_TRY(si_gen_dvbpsi_section(buf, len, &psi_sec));
	}

	*sec = NULL;
	/*Decode*/
	switch (table_id)
	{
		case AM_SI_TID_PAT:
			if (pid != AM_SI_PID_PAT)
				ret = AM_SI_ERR_NOT_SUPPORTED;
			else
				ret = si_decode_pat(sec, psi_sec);
			break;
		case AM_SI_TID_PMT:
			ret = si_decode_pmt(sec, psi_sec);
			break;
		case AM_SI_TID_CAT:
			if (pid != AM_SI_PID_CAT)
				ret = AM_SI_ERR_NOT_SUPPORTED;
			else
				ret = si_decode_cat(sec, psi_sec);
			break;
		case AM_SI_TID_NIT_ACT:
		case AM_SI_TID_NIT_OTH:
			if (pid != AM_SI_PID_NIT)
				ret = AM_SI_ERR_NOT_SUPPORTED;
			else
				ret = si_decode_nit(sec, psi_sec);
			break;
		case AM_SI_TID_BAT:
			if (pid != AM_SI_PID_BAT)
				ret = AM_SI_ERR_NOT_SUPPORTED;
			else
				ret = si_decode_bat(sec, psi_sec);
			break;
		case AM_SI_TID_SDT_ACT:
		case AM_SI_TID_SDT_OTH:
			if (pid != AM_SI_PID_SDT)
				ret = AM_SI_ERR_NOT_SUPPORTED;
			else
				ret = si_decode_sdt(sec, psi_sec);
			break;
		case AM_SI_TID_EIT_PF_ACT:
		case AM_SI_TID_EIT_PF_OTH:
		case AM_SI_TID_EIT_SCHE_ACT:
		case AM_SI_TID_EIT_SCHE_OTH:
		case (AM_SI_TID_EIT_SCHE_ACT + 1):
		case (AM_SI_TID_EIT_SCHE_OTH + 1):
			if (pid != AM_SI_PID_EIT)
				ret = AM_SI_ERR_NOT_SUPPORTED;
			else
				ret = si_decode_eit(sec, psi_sec);
			break;
		case AM_SI_TID_TOT:
		case AM_SI_TID_TDT:
			if (pid != AM_SI_PID_TOT)
				ret = AM_SI_ERR_NOT_SUPPORTED;
			else
				ret = si_decode_tot(sec, psi_sec);
			break;
		case AM_SI_TID_PSIP_MGT:
			if (pid != AM_SI_ATSC_BASE_PID)
				ret = AM_SI_ERR_NOT_SUPPORTED;
			else
				ret = si_decode_atsc_mgt(sec, psi_sec);
			break;
		case AM_SI_TID_PSIP_TVCT:
		case AM_SI_TID_PSIP_CVCT:
			if (pid != AM_SI_ATSC_BASE_PID)
				ret = AM_SI_ERR_NOT_SUPPORTED;
			else
				ret = si_decode_atsc_vct(sec, psi_sec);
			break;
		case AM_SI_TID_PSIP_RRT:
			if (pid != AM_SI_ATSC_BASE_PID)
				ret = AM_SI_ERR_NOT_SUPPORTED;
			else
				si_decode_psip_table(*sec, rrt, rrt_section_info_t, buf, len);
			break;
		case AM_SI_TID_PSIP_STT:
			if (pid != AM_SI_ATSC_BASE_PID)
				ret = AM_SI_ERR_NOT_SUPPORTED;
			else
				ret = si_decode_atsc_stt(sec, psi_sec);
			break;
		case AM_SI_TID_PSIP_EIT:
			ret = si_decode_atsc_eit(sec, psi_sec);
			break;
		case AM_SI_TID_PSIP_ETT:
			ret = si_decode_atsc_ett(sec, psi_sec);
			break;
		case AM_SI_TID_PSIP_CEA:
		    AM_DEBUG(1, "CEA PSI DECODE");
			ret = si_decode_atsc_cea(sec, psi_sec);
			break;
		default:
			ret = AM_SI_ERR_NOT_SUPPORTED;
			break;
	}

	/*release the psi_sec*/
	if (psi_sec)
		free(psi_sec);

	return ret;
}

/**\brief 释放一个从 AM_SI_DecodeSection()返回的section
 * \param handle SI解析句柄
 * \param table_id 用于标示section类型
 * \param [in] sec 需要释放的section
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_si.h)
 */
AM_ErrorCode_t AM_SI_ReleaseSection(AM_SI_Handle_t handle, uint8_t table_id, void *sec)
{
	AM_ErrorCode_t ret = AM_SUCCESS;

	assert(sec);
	AM_TRY(si_check_handle(handle));

	switch (table_id)
	{
		case AM_SI_TID_PAT:
			dvbpsi_DeletePAT((dvbpsi_pat_t*)sec);
			break;
		case AM_SI_TID_PMT:
			dvbpsi_DeletePMT((dvbpsi_pmt_t*)sec);
			break;
		case AM_SI_TID_CAT:
			dvbpsi_DeleteCAT((dvbpsi_cat_t*)sec);
			break;
		case AM_SI_TID_NIT_ACT:
		case AM_SI_TID_NIT_OTH:
			dvbpsi_DeleteNIT((dvbpsi_nit_t*)sec);
			break;
		case AM_SI_TID_BAT:
			dvbpsi_DeleteBAT((dvbpsi_bat_t*)sec);
			break;
		case AM_SI_TID_SDT_ACT:
		case AM_SI_TID_SDT_OTH:
			dvbpsi_DeleteSDT((dvbpsi_sdt_t*)sec);
			break;
		case AM_SI_TID_EIT_PF_ACT:
		case AM_SI_TID_EIT_PF_OTH:
		case AM_SI_TID_EIT_SCHE_ACT:
		case AM_SI_TID_EIT_SCHE_OTH:
		case (AM_SI_TID_EIT_SCHE_ACT + 1):
		case (AM_SI_TID_EIT_SCHE_OTH + 1):
			dvbpsi_DeleteEIT((dvbpsi_eit_t*)sec);
			break;
		case AM_SI_TID_TOT:
		case AM_SI_TID_TDT:
			dvbpsi_DeleteTOT((dvbpsi_tot_t*)sec);
			break;
		case AM_SI_TID_PSIP_MGT:
			dvbpsi_atsc_DeleteMGT((dvbpsi_atsc_mgt_t*)sec);
			break;
		case AM_SI_TID_PSIP_TVCT:
		case AM_SI_TID_PSIP_CVCT:
			dvbpsi_atsc_DeleteVCT((dvbpsi_atsc_vct_t*)sec);
			break;
		case AM_SI_TID_PSIP_RRT:
			atsc_psip_free_rrt_info((rrt_section_info_t*)sec);
			break;
		case AM_SI_TID_PSIP_STT:
			dvbpsi_atsc_DeleteSTT((dvbpsi_atsc_stt_t*)sec);
			break;
		case AM_SI_TID_PSIP_EIT:
			dvbpsi_atsc_DeleteEIT((dvbpsi_atsc_eit_t*)sec);
			break;
		case AM_SI_TID_PSIP_ETT:
			dvbpsi_atsc_DeleteETT((dvbpsi_atsc_ett_t*)sec);
			break;
		case AM_SI_TID_PSIP_CEA:
			dvbpsi_atsc_DeleteCEA((dvbpsi_atsc_cea_t*)sec);
			break;
		default:
			ret = AM_SI_ERR_INVALID_SECTION_DATA;
	}

	return ret;
}

/**\brief 获得一个section头信息
 * \param handle SI解析句柄
 * \param [in] buf section原始数据
 * \param len section原始数据长度
 * \param [out] sec_header section header信息
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_si.h)
 */
AM_ErrorCode_t AM_SI_GetSectionHeader(AM_SI_Handle_t handle, uint8_t *buf, uint16_t len, AM_SI_SectionHeader_t *sec_header)
{
	assert(buf && sec_header);
	AM_TRY(si_check_handle(handle));

	if (len < 8)
		return AM_SI_ERR_INVALID_SECTION_DATA;

	AM_TRY(si_get_section_header(buf, sec_header));

	return AM_SUCCESS;
}

/**\brief 设置默认的DVB编码方式，当前端流未按照DVB标准，即第一个
 * 字符没有指定编码方式时，可以调用该函数来指定一个强制转换的编码。
 * \param [in] code 默认进行强制转换的字符编码方式,如GB2312，BIG5等.
 * \return
 */
void AM_SI_SetDefaultDVBTextCoding(const char *coding)
{
	AM_DEBUG(1, "AM_SI_SetDefaultDVBTextCoding coding == [%s] \n",coding);
	snprintf(forced_dvb_text_coding, sizeof(forced_dvb_text_coding), "%s", coding);
}

AM_ErrorCode_t AM_SI_GetDVBTextCodingAndData(char *in, int in_len, char *coding, int coding_len, int *offset)
{
	char fbyte;

	if (!in || in_len <= 0
			|| !coding || !coding_len || !offset) {
		AM_DEBUG(1,"%s : bad param\n", __FUNCTION__);
		return AM_FAILURE;
	}

	/*查找输入编码方式*/
	AM_DEBUG(2,"%s : in_len=%d\n", __FUNCTION__, in_len);

	#define SET_CODING(_c_) strncpy(coding, (_c_), coding_len)
	#define SET_PRINT_CODING(_fmt_, _args_...) snprintf(coding, coding_len, _fmt_, _args_)

	*offset = 0;

	if (in_len <= 1) {
		SET_CODING("ISO-8859-1");
	} else {
		fbyte = in[0];
		AM_DEBUG(2, "%s fbyte == 0x%x \n", __FUNCTION__, fbyte);
		if (fbyte >= 0x01 && fbyte <= 0x0B) {
			SET_PRINT_CODING("ISO-8859-%d", fbyte + 4);
		} else if (fbyte >= 0x0C && fbyte <= 0x0F) {
			/*Reserved for future use, we set to ISO8859-1*/
			SET_CODING("ISO-8859-1");
		} else if (fbyte == 0x10 && in_len >= 3) {
			uint16_t val = (uint16_t)(((uint16_t)in[1]<<8) | (uint16_t)in[2]);
			if (val >= 0x0001 && val <= 0x000F) 	{
				SET_PRINT_CODING("ISO-8859-%d", val);
			} else {
				/*Reserved for future use, we set to ISO8859-1*/
				SET_CODING("ISO-8859-1");
			}
			*offset = 2;
		} else if (fbyte == 0x11) {
			SET_CODING("UTF-16");
		} else if (fbyte == 0x13) {
			SET_CODING("GB2312");
		} else if (fbyte == 0x14) {
                        if (!strcmp(forced_dvb_text_coding, "unicode")) {/*unicode or big5hk*/
                            SET_CODING(forced_dvb_text_coding);
                        } else {
                            SET_CODING("big5hk");
                        }
		} else if (fbyte == 0x15) {
			SET_CODING("utf-8");
		} else if (fbyte >= 0x20) {
			AM_DEBUG(2, "%s fbyte >= 0x20 \n", __FUNCTION__);
			if (strcmp(forced_dvb_text_coding, "")) {
				/*强制将输入按默认编码处理*/
				AM_DEBUG(2,"-fbyte >= 0x20-forced_dvb_text_coding-[%s]-\n",forced_dvb_text_coding);
				SET_CODING(forced_dvb_text_coding);
			} else {
				AM_DEBUG(2,"-fbyte >= 0x20---ISO6937--\n");
				SET_CODING("ISO6937");
			}
		} else if (fbyte == 0x1f) {
			SET_CODING("FreesatHuffuman");
		} else
			return AM_FAILURE;

		/*调整输入*/
		if (fbyte < 0x1f)
			*offset = 1;

		AM_DEBUG(2,"%s in[0]=0x%x-\n",__FUNCTION__, in[0]);
	}
	return AM_SUCCESS;
}

//Covert @in to UTF8 @out
AM_ErrorCode_t AM_SI_ConvertToUTF8(char *in, int in_len, char *out, int out_len, char *coding)
{
	if (!in || !out || in_len <= 0 || out_len <= 0 || !coding) {
		AM_DEBUG(1,"%s : bad param\n", __FUNCTION__);
		return AM_FAILURE;
	}

	memset(out,0,out_len);

	if (! strcmp(coding, "ISO6937"))
		return si_convert_iso6937_to_utf8(in, in_len, out, &out_len);
	else if(! strcmp(coding, "utf-8"))
		return AM_Check_UTF8(in, in_len, out, &out_len);
	else if(! strcmp(coding, "FreesatHuffuman")){
		char *temp = freesat_huffman_decode((unsigned char*)in, in_len);
		if (temp ) {
			int len = strlen(temp);
			len = len < out_len ? len : out_len;
			strncpy(out, temp, len);
			free(temp);
			temp = NULL;
		}
		AM_DEBUG(2,"--pp-out_code=%s--\n",out);
		return 0;
	} else if (!strcmp(coding, "utf-16")) {
		uint8_t *src, *dst;
		uint16_t uc;
		int sleft, dleft;

		src = (uint8_t*)in;
		dst = (uint8_t*)out;
		sleft = in_len;
		dleft = out_len;

		while (sleft > 0) {
			uc = (src[0] << 8) | src[1];

			if (uc <= 0x7f) {
				if (dleft < 1)
					break;
				dleft --;
				*dst++ = uc;
			} else if (uc <= 0x7ff) {
				if (dleft < 2)
					break;
				dleft -= 2;
				*dst++ = (uc >> 6) | 0xb0;
				*dst++ = (uc & 0x3f) | 0x80;
			} else {
				if (dleft < 3)
					break;
				dleft -= 3;
				*dst++ = (uc >> 12) | 0xe0;
				*dst++ = ((uc >> 6) & 0x3f) | 0x80;
				*dst++ = (uc & 0x3f) | 0x80;
			}

			src += 2;
			sleft -= 2;
		}
	} else {
		char **pin = &in;
		char **pout = &out;
		char *o_out = out;
		size_t inLength = in_len;
		size_t outLength = out_len;
		iconv_t handle;

		handle = iconv_open("utf-8", coding);
		if (handle == (iconv_t)-1) {
			AM_DEBUG(1, "Covert DVB text code failed, iconv_open err: %d", errno);
			return AM_FAILURE;
		}
		if ((int)iconv(handle, pin, &inLength, pout, &outLength) == -1) {
		    AM_DEBUG(1, "Covert DVB text code failed, iconv err: %d, in_len %d, out_len %d",
				errno, in_len, out_len);
		    iconv_close(handle);
		    return AM_FAILURE;
		}
		AM_Check_UTF8(o_out, out_len, o_out, &out_len);
		return iconv_close(handle);
	}
	return AM_SUCCESS;
}

AM_ErrorCode_t AM_SI_ConvertDVBTextCodeEx(char *in, int in_len, char *out, int out_len, char *coding)
{
	char cod[64] = {0};
	int offset = 0;

	if (!coding) {
		AM_TRY(AM_SI_GetDVBTextCodingAndData(in, in_len, cod, 64, &offset));
	} else {
		strncpy(cod, coding, 64);
	}

	return AM_SI_ConvertToUTF8(in+offset, in_len-offset, out, out_len, cod);
}

/**\brief 按DVB标准将输入字符转成UTF-8编码
 * \param [in] in_code 需要转换的字符数据
 * \param in_len 需要转换的字符数据长度
 * \param [out] out_code 转换后的字符数据
 * \param out_len 输出字符缓冲区大小
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_si.h)
 */
AM_ErrorCode_t AM_SI_ConvertDVBTextCode(char *in_code,int in_len,char *out_code,int out_len)
{
	char cod[64] = {0};
	int offset = 0;

	AM_TRY(AM_SI_GetDVBTextCodingAndData(in_code, in_len, cod, 64, &offset));

	AM_DEBUG(2, "coding:%s, offset:%d", cod, offset);

	return AM_SI_ConvertToUTF8(in_code+offset, in_len-offset, out_code, out_len, cod);
}

/**\brief 根据ac3 des 获取audio 的 exten
 * \param [in] p_decoded dvbpsi_AC3_dr_t
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_si.h)
 */
AM_ErrorCode_t AM_SI_GetAudioExten_from_AC3des(dvbpsi_AC3_dr_t *p_decoded,int *p_exten)
{
	int i_exten = 0;
	/*i_exten is 32 bit,31:24 bit: exten type,*/
	/*23:16:mainid or asvc id,8 bit*/
	/*15:0 bit:no use*/
	if (p_decoded->i_mainid_flag == 1)
	{
		i_exten = AM_Audio_AC3MAIN<<24 | p_decoded->i_mainid<<16;
	}
	if (p_decoded->i_asvc_flag == 1)
	{
		i_exten = AM_Audio_AC3ASVC<<24 | p_decoded->i_asvc<<16;
	}
	*p_exten = i_exten;
	AM_DEBUG(1, "audio->audio_exten : 0x%04x", i_exten);
	return  AM_SUCCESS;
}

/**\brief 根据enac3 des 获取audio 的 exten
 * \param [in] p_decoded dvbpsi_ENAC3_dr_t
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_si.h)
 */
AM_ErrorCode_t AM_SI_GetAudioExten_from_ENAC3des(dvbpsi_ENAC3_dr_t *p_decoded,int *p_exten)
{
	int i_exten = 0;
	/*i_exten is 32 bit,31:24 bit: exten type,*/
	/*23:16:mainid or asvc id,8 bit*/
	/*15:0 bit:no use*/
	if (p_decoded->i_mainid_flag == 1)
	{
		i_exten = AM_Audio_AC3MAIN<<24 | p_decoded->i_mainid<<16;
	}
	if (p_decoded->i_asvc_flag == 1)
	{
		i_exten = AM_Audio_AC3ASVC<<24 | p_decoded->i_asvc<<16;
	}
	*p_exten = i_exten;
	AM_DEBUG(1, "audio->audio_exten : 0x%04x", i_exten);
	return  AM_SUCCESS;
}
/**\brief 根据 PSIP enac3 des 获取audio 的 exten
 * \param [in] p_decoded dvbpsi_ENAC3_dr_t
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_si.h)
 */
AM_ErrorCode_t AM_SI_GetAudioExten_from_PSIPENAC3des(dvbpsi_PSIPENAC3_dr_t *p_decoded,int *p_exten)
{
	int i_exten = 0;
	/*i_exten is 32 bit,31:24 bit: exten type,*/
	/*23:16:mainid or asvc id,8 bit*/
	/*15:0 bit:no use*/
	if (p_decoded->i_mainid_flag == 1)
	{
		i_exten = AM_Audio_AC3MAIN<<24 | p_decoded->i_mainid<<16;
	}
	if (p_decoded->i_asvc_flag == 1)
	{
		i_exten = AM_Audio_AC3ASVC<<24 | p_decoded->i_asvc<<16;
	}
	*p_exten = i_exten;
	AM_DEBUG(1, "audio->audio_exten : 0x%04x", i_exten);
	return  AM_SUCCESS;
}
/**\brief 根据 PSIP ac3 audio stream des 获取audio 的 exten
 * \param [in] p_decoded dvbpsi_ENAC3_dr_t
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_si.h)
 */
AM_ErrorCode_t AM_SI_GetAudioExten_from_PSIPAC3AStreamdes(dvbpsi_atsc_ac3_audio_dr_t *p_decoded,int *p_exten)
{
	int i_exten = 0;
	/*i_exten is 32 bit,31:24 bit: exten type,*/
	/*23:16:mainid or asvc id,8 bit*/
	/*15:0 bit:no use*/
	if (p_decoded->i_bsmod < 1)
	{
		i_exten = AM_Audio_AC3MAIN<<24 | p_decoded->i_mainid<<16;
	}
	else
	{
		i_exten = AM_Audio_AC3ASVC<<24 | p_decoded->i_asvcflags<<16;
	}
	*p_exten = i_exten;
	AM_DEBUG(1, "audio->audio_exten : 0x%04x", i_exten);
	return  AM_SUCCESS;
}
/**\brief 从一个ES流中提取音视频
 * \param [in] es ES流
 * \param [out] vid 提取出的视频PID
 * \param [out] vfmt 提取出的视频压缩格式
 * \param [out] aud_info 提取出的音频数据
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_si.h)
 */
AM_ErrorCode_t AM_SI_ExtractAVFromES(dvbpsi_pmt_es_t *es, int *vid, int *vfmt, AM_SI_AudioInfo_t *aud_info)
{
	char lang_tmp[3];
	int	audio_type=0;
	int	audio_exten=0;
	int afmt_tmp, vfmt_tmp;
	dvbpsi_descriptor_t *descr;

	afmt_tmp = -1;
	vfmt_tmp = -1;
	memset(lang_tmp, 0, sizeof(lang_tmp));
	switch (es->i_type)
	{
		AM_DEBUG(1, "es->i_type : 0x%02x", es->i_type);
		/*override by parse descriptor*/
		case 0x6:
			AM_SI_LIST_BEGIN(es->p_first_descriptor, descr)
				switch (descr->i_tag)
				{
					case AM_SI_DESCR_AC3:
						AM_DEBUG(1, "!!Found AC3 Descriptor!!!");
						afmt_tmp = AFORMAT_AC3;
						if (descr->p_decoded != NULL)
						{
							AM_SI_GetAudioExten_from_AC3des((dvbpsi_AC3_dr_t*)descr->p_decoded,&audio_exten);
						}
						break;
					case AM_SI_DESCR_ENHANCED_AC3:
						AM_DEBUG(1, "!!Found Enhanced AC3 Descriptor!!!");
						afmt_tmp = AFORMAT_EAC3;
						if (descr->p_decoded != NULL)
						{
							AM_SI_GetAudioExten_from_ENAC3des((dvbpsi_ENAC3_dr_t*)descr->p_decoded,&audio_exten);
						}
						break;
					case AM_SI_DESCR_PSIPENHANCED_AC3:
						AM_DEBUG(1, "!!Found PSIP Enhanced AC3 Descriptor!!!");
						afmt_tmp = AFORMAT_EAC3;
						if (descr->p_decoded != NULL)
						{
							AM_SI_GetAudioExten_from_PSIPENAC3des((dvbpsi_PSIPENAC3_dr_t*)descr->p_decoded,&audio_exten);
						}
						break;
					case AM_SI_DESCR_PSIP_AUDIOSTREAM_AC3:
						AM_DEBUG(1, "!!Found PSIP AC3 audio stream Descriptor!!!");
						afmt_tmp = AFORMAT_AC3;
						if (descr->p_decoded != NULL)
						{
							AM_SI_GetAudioExten_from_PSIPAC3AStreamdes((dvbpsi_atsc_ac3_audio_dr_t*)descr->p_decoded,&audio_exten);
						}
						break;
					case AM_SI_DESCR_AAC:
						AM_DEBUG(1, "!!Found AAC Descriptor!!!");
						afmt_tmp = AFORMAT_AAC;
						break;
					case AM_SI_DESCR_DTS:
						AM_DEBUG(1, "!!Found DTS Descriptor!!!");
						afmt_tmp = AFORMAT_DTS;
						break;
					case AM_SI_DESCR_DRA:
						AM_DEBUG(1, "!!Found DRA Descriptor!!!");
						afmt_tmp = AFORMAT_DRA;
						break;
					case AM_SI_DESCR_REGISTRATION:
						AM_DEBUG(1, "!!Found Registeration Descriptor!!!");
						if (descr->p_decoded != NULL)
						{
							dvbpsi_registration_dr_t *pregd = (dvbpsi_registration_dr_t*)descr->p_decoded;
							switch (pregd->i_format_identifier)
							{
								case 0x44545331:
									AM_DEBUG(1, "found  format identifier for [DTS1]");
									afmt_tmp = AFORMAT_DTS;
									break;
								case 0x44545332:
									AM_DEBUG(1, "found  format identifier for [DTS2]");
									afmt_tmp = AFORMAT_DTS;
									break;
								case 0x44545333:
									AM_DEBUG(1, "found  format identifier for [DTS3]");
									afmt_tmp = AFORMAT_DTS;
									break;
								case 0x44545348:
									AM_DEBUG(1, "found  format identifier for [DTSH]");
									afmt_tmp = AFORMAT_DTS;
									break;
								case 0x44524131:
									AM_DEBUG(1, "found  format identifier for [DRA]");
									afmt_tmp = AFORMAT_DRA;;
									break;
								default:
									break;
							}
						}
						break;
					default:
						break;
				}
			AM_SI_LIST_END()
			break;
		/*video pid and video format*/
		case 0x1:
		case 0x2:
		//case 0x80:/*do not support*/
		case 0x80:
			vfmt_tmp = VFORMAT_MPEG12;
			break;
		case 0x10:
			vfmt_tmp = VFORMAT_MPEG4;
			break;
		case 0x1b:
			vfmt_tmp = VFORMAT_H264;
			break;
		case 0x24:
			vfmt_tmp = VFORMAT_HEVC;
			break;
		case 0xea:
			vfmt_tmp = VFORMAT_VC1;
			break;
		case 0x42:
			vfmt_tmp = VFORMAT_AVS;
			break;
		case 0xd2:
			vfmt_tmp = VFORMAT_AVS2;
			break;
		/*audio pid and audio format*/
		case 0x3:
		case 0x4:
			afmt_tmp = AFORMAT_MPEG;
			break;
		case 0x0f:
			afmt_tmp = AFORMAT_AAC;
			break;
		case 0x11:
			afmt_tmp = AFORMAT_AAC_LATM;
			break;
		case 0x81:
			afmt_tmp = AFORMAT_AC3;
			AM_SI_LIST_BEGIN(es->p_first_descriptor, descr)
			//AM_DEBUG(1, "ac3 descr->i_tag : 0x%02x", descr->i_tag);
				switch (descr->i_tag)
				{
					case AM_SI_DESCR_AC3:
						AM_DEBUG(1, "!!Found AC3 Descriptor!!!");
						afmt_tmp = AFORMAT_AC3;
						if (descr->p_decoded != NULL)
						{
							AM_SI_GetAudioExten_from_AC3des((dvbpsi_AC3_dr_t*)descr->p_decoded,&audio_exten);
						}
						break;
					case AM_SI_DESCR_ENHANCED_AC3:
						AM_DEBUG(1, "!!Found Enhanced AC3 Descriptor!!!");
						afmt_tmp = AFORMAT_EAC3;
						if (descr->p_decoded != NULL)
						{
							AM_SI_GetAudioExten_from_ENAC3des((dvbpsi_ENAC3_dr_t*)descr->p_decoded,&audio_exten);
						}
						break;
					case AM_SI_DESCR_PSIPENHANCED_AC3:
						AM_DEBUG(1, "!!Found PSIP Enhanced AC3 Descriptor!!!");
						afmt_tmp = AFORMAT_EAC3;
						if (descr->p_decoded != NULL)
						{
							AM_SI_GetAudioExten_from_PSIPENAC3des((dvbpsi_PSIPENAC3_dr_t*)descr->p_decoded,&audio_exten);
						}
						break;
					case AM_SI_DESCR_PSIP_AUDIOSTREAM_AC3:
						AM_DEBUG(1, "!!Found PSIP AC3 audio stream Descriptor!!!");
						afmt_tmp = AFORMAT_AC3;
						if (descr->p_decoded != NULL)
						{
							AM_SI_GetAudioExten_from_PSIPAC3AStreamdes((dvbpsi_atsc_ac3_audio_dr_t*)descr->p_decoded,&audio_exten);
						}
						break;
					default:
						break;
				}
			AM_SI_LIST_END()
			break;
		case 0x8A:
		//case 0x82:
		case 0x85:
		case 0x86:
			afmt_tmp = AFORMAT_DTS;
			break;
		case 0x87:
			afmt_tmp = AFORMAT_EAC3;
			AM_SI_LIST_BEGIN(es->p_first_descriptor, descr)
			//AM_DEBUG(1, "eac3 descr->i_tag : 0x%02x", descr->i_tag);
				switch (descr->i_tag)
				{
					case AM_SI_DESCR_AC3:
						AM_DEBUG(1, "!!Found AC3 Descriptor!!!");
						afmt_tmp = AFORMAT_AC3;
						if (descr->p_decoded != NULL)
						{
							AM_SI_GetAudioExten_from_AC3des((dvbpsi_AC3_dr_t*)descr->p_decoded,&audio_exten);
						}
						break;
					case AM_SI_DESCR_ENHANCED_AC3:
						AM_DEBUG(1, "!!Found Enhanced AC3 Descriptor!!!");
						afmt_tmp = AFORMAT_EAC3;
						if (descr->p_decoded != NULL)
						{
							AM_SI_GetAudioExten_from_ENAC3des((dvbpsi_ENAC3_dr_t*)descr->p_decoded,&audio_exten);
						}
						break;
					case AM_SI_DESCR_PSIPENHANCED_AC3:
						AM_DEBUG(1, "!!Found PSIP Enhanced AC3 Descriptor!!!");
						afmt_tmp = AFORMAT_EAC3;
						if (descr->p_decoded != NULL)
						{
							AM_SI_GetAudioExten_from_PSIPENAC3des((dvbpsi_PSIPENAC3_dr_t*)descr->p_decoded,&audio_exten);
						}
						break;
					case AM_SI_DESCR_PSIP_AUDIOSTREAM_AC3:
						AM_DEBUG(1, "!!Found PSIP AC3 audio stream Descriptor!!!");
						afmt_tmp = AFORMAT_AC3;
						if (descr->p_decoded != NULL)
						{
							AM_SI_GetAudioExten_from_PSIPAC3AStreamdes((dvbpsi_atsc_ac3_audio_dr_t*)descr->p_decoded,&audio_exten);
						}
						break;
					default:
						break;
				}
			AM_SI_LIST_END()
			break;
		default:
			break;
	}

	/*添加音视频流*/
	if (vfmt_tmp != -1)
	{
		*vid = (es->i_pid >= 0x1fff) ? 0x1fff : es->i_pid;
		AM_DEBUG(1, "Set video format to %d", vfmt_tmp);
		*vfmt = vfmt_tmp;
	}
	if (afmt_tmp != -1)
	{
		AM_SI_LIST_BEGIN(es->p_first_descriptor, descr)
			if (descr->i_tag == AM_SI_DESCR_ISO639 && descr->p_decoded != NULL)
			{
				dvbpsi_iso639_dr_t *pisod = (dvbpsi_iso639_dr_t*)descr->p_decoded;
				if (pisod->i_code_count > 0)
				{
					memcpy(lang_tmp, pisod->code[0].iso_639_code, sizeof(lang_tmp));
					audio_type = pisod->code[0].i_audio_type;
					break;
				}
			}
		AM_SI_LIST_END()
		/*if exist exten des, used exten des info to check audio main or sub*/
		AM_SI_LIST_BEGIN(es->p_first_descriptor, descr)
			if (descr->i_tag == AM_SI_DESCR_EXTENSION && descr->p_decoded != NULL)
			{
				dvbpsi_EXTENTION_dr_t *pisod = (dvbpsi_EXTENTION_dr_t*)descr->p_decoded;
				if (pisod->i_extern_des_tag == AM_SI_EXTEN_DESCR_SUP_AUDIO)
				{
					if (pisod->exten_t.sup_audio.lang_code == 1) {
						memcpy(lang_tmp, pisod->exten_t.sup_audio.iso_639_lang_code, sizeof(lang_tmp));
					}
					audio_type = pisod->exten_t.sup_audio.editorial_classification;
					AM_DEBUG(1, "audio type : %d lang:%s", audio_type, lang_tmp);
					break;
				}
			}
		AM_SI_LIST_END()
		/* Add a audio */
		si_add_audio(aud_info, es->i_pid, afmt_tmp, lang_tmp,audio_type,audio_exten);
	}

	return AM_SUCCESS;
}

/**\brief 按ATSC标准从一个ATSC visual channel中提取音视频
 * \param [in] es ES流
 * \param [out] vid 提取出的视频PID
 * \param [out] vfmt 提取出的视频压缩格式
 * \param [out] aud_info 提取出的音频数据
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_si.h)
 */
AM_ErrorCode_t AM_SI_ExtractAVFromATSCVC(vct_channel_info_t *vcinfo, int *vid, int *vfmt, AM_SI_AudioInfo_t *aud_info)
{
	char lang_tmp[3];
	int	audio_type=0;
	int audio_exten = 0;
	int afmt_tmp, vfmt_tmp, i;
	atsc_descriptor_t *descr;

	afmt_tmp = -1;
	vfmt_tmp = -1;
	memset(lang_tmp, 0, sizeof(lang_tmp));

	AM_SI_LIST_BEGIN(vcinfo->desc, descr)
		if (descr->p_decoded && descr->i_tag == AM_SI_DESCR_SERVICE_LOCATION)
		{
			atsc_service_location_dr_t *asld = (atsc_service_location_dr_t*)descr->p_decoded;
			for (i=0; i<asld->i_elem_count; i++)
			{
				afmt_tmp = -1;
				vfmt_tmp = -1;
				memset(lang_tmp, 0, sizeof(lang_tmp));
				switch (asld->elem[i].i_stream_type)
				{
					/*video pid and video format*/
					case 0x02:
						vfmt_tmp = VFORMAT_MPEG12;
						break;
					/*audio pid and audio format*/
					case 0x81:
						afmt_tmp = AFORMAT_AC3;
						break;
					default:
						break;
				}
				if (vfmt_tmp != -1)
				{
					*vid = (asld->elem[i].i_pid >= 0x1fff) ? 0x1fff : asld->elem[i].i_pid;
					*vfmt = vfmt_tmp;
				}
				if (afmt_tmp != -1)
				{
					memcpy(lang_tmp, asld->elem[i].iso_639_code, sizeof(lang_tmp));
					audio_type = asld->elem[i].i_audio_type;
					si_add_audio(aud_info, asld->elem[i].i_pid, afmt_tmp, lang_tmp,audio_type,audio_exten);
				}
			}
		}
	AM_SI_LIST_END()

	return AM_SUCCESS;
}

AM_ErrorCode_t AM_SI_ExtractAVFromVC(dvbpsi_atsc_vct_channel_t *vcinfo, int *vid, int *vfmt, AM_SI_AudioInfo_t *aud_info)
{
	char lang_tmp[3];
	int audio_type=0;
	int audio_exten = 0;
	int afmt_tmp, vfmt_tmp, i;
	dvbpsi_descriptor_t *descr;

	afmt_tmp = -1;
	vfmt_tmp = -1;
	memset(lang_tmp, 0, sizeof(lang_tmp));

	AM_SI_LIST_BEGIN(vcinfo->p_first_descriptor, descr)
		if (descr->p_decoded && descr->i_tag == AM_SI_DESCR_SERVICE_LOCATION)
		{
			dvbpsi_atsc_service_location_dr_t *asld = (dvbpsi_atsc_service_location_dr_t*)descr->p_decoded;
			for (i=0; i<asld->i_number_elements; i++)
			{
				afmt_tmp = -1;
				vfmt_tmp = -1;
				memset(lang_tmp, 0, sizeof(lang_tmp));
				switch (asld->elements[i].i_stream_type)
				{
					/*video pid and video format*/
					case 0x02:
						vfmt_tmp = VFORMAT_MPEG12;
						break;
					/*audio pid and audio format*/
					case 0x81:
						afmt_tmp = AFORMAT_AC3;
						break;
					default:
						break;
				}
				if (vfmt_tmp != -1)
				{
					*vid = (asld->elements[i].i_elementary_pid >= 0x1fff) ? 0x1fff : asld->elements[i].i_elementary_pid;
					*vfmt = vfmt_tmp;
				}
				if (afmt_tmp != -1)
				{
					memcpy(lang_tmp, asld->elements[i].i_iso_639_code, sizeof(lang_tmp));
					si_add_audio(aud_info, asld->elements[i].i_elementary_pid, afmt_tmp, lang_tmp,audio_type,audio_exten);
				}
			}
		}
	AM_SI_LIST_END()

	return AM_SUCCESS;
}

AM_ErrorCode_t AM_SI_ExtractScte27SubtitleFromES(dvbpsi_pmt_es_t *es, AM_SI_Scte27SubtitleInfo_t *sub_info)
{
	dvbpsi_descriptor_t *descr;
	int i, found = 0;

	if (es->i_type == 0x82)
	{
		for (i=0; i<sub_info->subtitle_count; i++)
		{
			if (es->i_pid == sub_info->subtitles[i].pid)
			{
				found = 1;
				break;
			}
		}

		if (found != 1)
		{
			sub_info->subtitles[sub_info->subtitle_count].pid = es->i_pid;
			sub_info->subtitle_count++;
		}

		AM_DEBUG(0, "Scte27 stream found pid 0x%x count %d", es->i_pid, sub_info->subtitle_count);
		AM_SI_LIST_BEGIN(es->p_first_descriptor, descr)
		{
			if (descr->p_decoded)
			{
				AM_DEBUG(0, "scte27 i_tag table_id 0x%x", descr->i_tag);
			}
		}
		AM_SI_LIST_END()
	}
	return AM_SUCCESS;
}

AM_ErrorCode_t AM_SI_ExtractDVBSubtitleFromES(dvbpsi_pmt_es_t *es, AM_SI_SubtitleInfo_t *sub_info)
{
	dvbpsi_descriptor_t *descr;

	AM_SI_LIST_BEGIN(es->p_first_descriptor, descr)
		if (descr->p_decoded && descr->i_tag == AM_SI_DESCR_SUBTITLING)
		{
			int isub, i;
			dvbpsi_subtitle_t *tmp_sub;
			dvbpsi_subtitling_dr_t *psd = (dvbpsi_subtitling_dr_t*)descr->p_decoded;

			for (isub=0; isub<psd->i_subtitles_number; isub++)
			{
				tmp_sub = &psd->p_subtitle[isub];

				/* already added ? */
				for (i=0; i<sub_info->subtitle_count; i++)
				{
					if (es->i_pid                      == sub_info->subtitles[i].pid &&
						tmp_sub->i_subtitling_type     == sub_info->subtitles[i].type &&
						tmp_sub->i_ancillary_page_id   == sub_info->subtitles[i].anci_page_id &&
						tmp_sub->i_composition_page_id == sub_info->subtitles[i].comp_page_id &&
						! memcmp(tmp_sub->i_iso6392_language_code, sub_info->subtitles[i].lang, 3))
					{
						AM_DEBUG(1, "Skipping a exist subtitle: pid %d, lang %c%c%c",
							es->i_pid, tmp_sub->i_iso6392_language_code[0],
							tmp_sub->i_iso6392_language_code[1],
							tmp_sub->i_iso6392_language_code[2]);
						break;
					}
				}

				if (i < sub_info->subtitle_count)
					continue;

				if (sub_info->subtitle_count >= AM_SI_MAX_SUB_CNT)
				{
					AM_DEBUG(0, "Too many subtitles, Max count %d", AM_SI_MAX_SUB_CNT);
					return AM_SUCCESS;
				}

				if (sub_info->subtitle_count < 0)
					sub_info->subtitle_count = 0;

				sub_info->subtitles[sub_info->subtitle_count].pid          = es->i_pid;
				sub_info->subtitles[sub_info->subtitle_count].type         = tmp_sub->i_subtitling_type;
				sub_info->subtitles[sub_info->subtitle_count].comp_page_id = tmp_sub->i_composition_page_id;
				sub_info->subtitles[sub_info->subtitle_count].anci_page_id = tmp_sub->i_ancillary_page_id;
				if (tmp_sub->i_iso6392_language_code[0] == 0)
				{
					snprintf(sub_info->subtitles[sub_info->subtitle_count].lang,
						sizeof(sub_info->subtitles[sub_info->subtitle_count].lang),
						"Subtitle%d", sub_info->subtitle_count+1);
				}
				else
				{
					memcpy(sub_info->subtitles[sub_info->subtitle_count].lang, tmp_sub->i_iso6392_language_code, 3);
					sub_info->subtitles[sub_info->subtitle_count].lang[3] = 0;
				}

				AM_DEBUG(1, "Add a subtitle: pid %d, language: %s", es->i_pid, sub_info->subtitles[sub_info->subtitle_count].lang);

				sub_info->subtitle_count++;
			}
		}
	AM_SI_LIST_END()

	return AM_SUCCESS;
}


AM_ErrorCode_t AM_SI_ExtractDVBTeletextFromES(dvbpsi_pmt_es_t *es, AM_SI_TeletextInfo_t *ttx_info)
{
	dvbpsi_descriptor_t *descr;

	AM_SI_LIST_BEGIN(es->p_first_descriptor, descr)
		if (descr->p_decoded==NULL && descr->i_tag == AM_SI_DESCR_TELETEXT){
			if(descr->i_length == 0){
				if (ttx_info->teletext_count < 0)
					ttx_info->teletext_count = 0;
				ttx_info->teletexts[ttx_info->teletext_count].pid          = es->i_pid;
				ttx_info->teletexts[ttx_info->teletext_count].type         = 0x0;
				ttx_info->teletexts[ttx_info->teletext_count].magazine_no  = 1;
				ttx_info->teletexts[ttx_info->teletext_count].page_no      = 100;
				snprintf(ttx_info->teletexts[ttx_info->teletext_count].lang,
					sizeof(ttx_info->teletexts[ttx_info->teletext_count].lang),
					"Teletext%d", ttx_info->teletext_count+1);
				ttx_info->teletext_count++;
				return AM_SUCCESS;
			}
		}

		if (descr->p_decoded && descr->i_tag == AM_SI_DESCR_TELETEXT)
		{
			int itel, i;
			dvbpsi_teletextpage_t *tmp_ttx;
			dvbpsi_teletextpage_t def_ttx;
			dvbpsi_teletext_dr_t *ptd = (dvbpsi_teletext_dr_t*)descr->p_decoded;

			memset(&def_ttx, 0, sizeof(def_ttx));
			def_ttx.i_teletext_magazine_number = 1;

			for (itel=0; itel<ptd->i_pages_number; itel++)
			{
				if (ptd != NULL)
					tmp_ttx = &ptd->p_pages[itel];
				else
					tmp_ttx = &def_ttx;

				/* already added ? */
				for (i=0; i<ttx_info->teletext_count; i++)
				{
					if (es->i_pid                           == ttx_info->teletexts[i].pid &&
						tmp_ttx->i_teletext_type            == ttx_info->teletexts[i].type &&
						tmp_ttx->i_teletext_magazine_number == ttx_info->teletexts[i].magazine_no &&
						tmp_ttx->i_teletext_page_number     == ttx_info->teletexts[i].page_no &&
						! memcmp(tmp_ttx->i_iso6392_language_code, ttx_info->teletexts[i].lang, 3))
					{
						AM_DEBUG(1, "Skipping a exist teletext: pid %d, lang %c%c%c",
							es->i_pid, tmp_ttx->i_iso6392_language_code[0],
							tmp_ttx->i_iso6392_language_code[1],
							tmp_ttx->i_iso6392_language_code[2]);
						break;
					}
				}

				if (i < ttx_info->teletext_count)
					continue;

				if (ttx_info->teletext_count >= AM_SI_MAX_TTX_CNT)
				{
					AM_DEBUG(0, "Too many teletexts, Max count %d", AM_SI_MAX_TTX_CNT);
					return AM_SUCCESS;
				}

				if (ttx_info->teletext_count < 0)
					ttx_info->teletext_count = 0;

				ttx_info->teletexts[ttx_info->teletext_count].pid          = es->i_pid;
				ttx_info->teletexts[ttx_info->teletext_count].type         = tmp_ttx->i_teletext_type;
				ttx_info->teletexts[ttx_info->teletext_count].magazine_no  = tmp_ttx->i_teletext_magazine_number;
				ttx_info->teletexts[ttx_info->teletext_count].page_no      = tmp_ttx->i_teletext_page_number;
				if (tmp_ttx->i_iso6392_language_code[0] == 0)
				{
					snprintf(ttx_info->teletexts[ttx_info->teletext_count].lang,
						sizeof(ttx_info->teletexts[ttx_info->teletext_count].lang),
						"Teletext%d", ttx_info->teletext_count+1);
				}
				else
				{
					memcpy(ttx_info->teletexts[ttx_info->teletext_count].lang, tmp_ttx->i_iso6392_language_code, 3);
					ttx_info->teletexts[ttx_info->teletext_count].lang[3] = 0;
				}

				AM_DEBUG(1, "Add a teletext: pid %d, language: %s", es->i_pid, ttx_info->teletexts[ttx_info->teletext_count].lang);

				ttx_info->teletext_count++;
			}
		}
	AM_SI_LIST_END()

	return AM_SUCCESS;
}

AM_ErrorCode_t AM_SI_ExtractDVBIsdbsubtitleFromES(dvbpsi_pmt_es_t *es, AM_SI_IsdbsubtitleInfo_t *sub_info)
{
	dvbpsi_descriptor_t *descr;

	AM_SI_LIST_BEGIN(es->p_first_descriptor, descr)
	if (descr->i_tag == AM_SI_DESCR_ISDBSUBTITLING)
	{
		if (descr->p_data[0] == 0 && descr->p_data[1] == 8)
		{
			sub_info->isdb_count = 1;
			sub_info->isdbs[0].pid = es->i_pid;
			sub_info->isdbs[0].type = 7;
			memcpy(sub_info->isdbs[0].lang, "isdb", 5);
			AM_DEBUG(0, "Isdb found and added");
		}
	}
	AM_SI_LIST_END()

	return AM_SUCCESS;
}


AM_ErrorCode_t AM_SI_ExtractATSCCaptionFromES(dvbpsi_pmt_es_t *es, AM_SI_CaptionInfo_t *cap_info)
{
	dvbpsi_descriptor_t *descr;

	AM_SI_LIST_BEGIN(es->p_first_descriptor, descr)
		if (descr->p_decoded && descr->i_tag == AM_SI_DESCR_CAPTION_SERVICE)
		{
			int icap, i;
			dvbpsi_caption_service_t *tmp_cap;
			dvbpsi_atsc_caption_service_dr_t *psd = (dvbpsi_atsc_caption_service_dr_t*)descr->p_decoded;

			for (icap=0; icap<psd->i_number_of_services; icap++)
			{
				tmp_cap = &psd->services[icap];

				if (cap_info->caption_count >= AM_SI_MAX_CAP_CNT)
				{
					AM_DEBUG(0, "Too many captions, Max count %d", AM_SI_MAX_CAP_CNT);
					return AM_SUCCESS;
				}

				if (cap_info->caption_count < 0)
					cap_info->caption_count = 0;

				//if (!tmp_cap->b_digtal_cc)
				//	continue;//ignore the analog cc declaration

				cap_info->captions[cap_info->caption_count].service_number = tmp_cap->i_caption_service_number;
				cap_info->captions[cap_info->caption_count].type = tmp_cap->b_digital_cc;
				cap_info->captions[cap_info->caption_count].pid_or_line21 = tmp_cap->b_digital_cc ? es->i_pid : tmp_cap->b_line21_field;
				cap_info->captions[cap_info->caption_count].flags =
					(tmp_cap->b_easy_reader ? 0x80 : 0) |
					(tmp_cap->b_wide_aspect_ratio ? 0x40 : 0);
				cap_info->captions[cap_info->caption_count].private_data = tmp_cap->private_data;
				if (tmp_cap->i_iso_639_code[0] == 0)
				{
					snprintf(cap_info->captions[cap_info->caption_count].lang,
						sizeof(cap_info->captions[cap_info->caption_count].lang),
						"Caption%d", cap_info->caption_count+1);
				}
				else
				{
					memcpy(cap_info->captions[cap_info->caption_count].lang, tmp_cap->i_iso_639_code, 3);
					cap_info->captions[cap_info->caption_count].lang[3] = 0;
				}

				AM_DEBUG(1, "Add a caption: pid %d, language: %s, digital:%d service:%d type %d flags:%x",
					es->i_pid,
					cap_info->captions[cap_info->caption_count].lang,
					cap_info->captions[cap_info->caption_count].type,
					cap_info->captions[cap_info->caption_count].service_number,
					cap_info->captions[cap_info->caption_count].type,
					cap_info->captions[cap_info->caption_count].flags);

				cap_info->caption_count++;
			}
		}
	AM_SI_LIST_END()

	return AM_SUCCESS;
}

AM_ErrorCode_t AM_SI_GetRatingString(dvbpsi_atsc_content_advisory_dr_t *pcad, char *buf, int buf_size)
{
	int i, j;
	/*
	[
	    //g=region, rx=ratings, d=dimension, r=rating value, rs:rating string
	    {g:0,rx:[{d:0,r:3},{d:2,r:1},{d:4,r:1}],rs:[{lng:"eng",txt:"TV-PG-L-V"}]},
		{g:1,rx:[{d:7,r:3}],rs:[{lng:"eng",txt:"MPAA-PG"}]}
		...
	]
	*/
	sprintf(buf, "[");
	for (i=0; i<pcad->i_rating_region_count; i++)
	{
		if (i != 0)
			snprintf(buf, buf_size, "%s,", buf);

		snprintf(buf, buf_size, "%s{g:%d,rx:[", buf, pcad->rating_regions[i].i_rating_region);
		for (j=0; j<pcad->rating_regions[i].i_rated_dimensions; j++)
		{
			if (j != 0)
				snprintf(buf, buf_size, "%s,", buf);

			snprintf(buf, buf_size, "%s{d:%d,r:%d}", buf,
				pcad->rating_regions[i].dimensions[j].i_rating_dimension_j,
				pcad->rating_regions[i].dimensions[j].i_rating_value);
		}
		snprintf(buf, buf_size, "%s]", buf);

		{
			int k;
			atsc_multiple_string_t ms;

			snprintf(buf, buf_size, "%s,rs:[", buf);

			memset(&ms, 0, sizeof(ms));
			atsc_decode_multiple_string_structure(pcad->rating_regions[i].i_rating_description, &ms);
			for (k=0; k<ms.i_string_count; k++)
			{
				if (k != 0)
					snprintf(buf, buf_size, "%s,", buf);

				snprintf(buf, buf_size, "%s{lng:\"%c%c%c\",txt:\"%s\"}", buf,
					ms.string[k].iso_639_code[0],
					ms.string[k].iso_639_code[1],
					ms.string[k].iso_639_code[2],
					ms.string[k].string);
			}
			snprintf(buf, buf_size, "%s]", buf);
		}
		snprintf(buf, buf_size, "%s}", buf);
	}
	snprintf(buf, buf_size, "%s]", buf);

	return AM_SUCCESS;
}

AM_ErrorCode_t AM_SI_GetATSCCaptionString(dvbpsi_atsc_caption_service_dr_t *psd, char *buf, int buf_size)
{
		/*
		[
			//bdig=b_digtal_cc, sn=caption_service_number, lng=language,
			//beasy=b_easy_reader, bwar=b_wide_aspect_ratio
			{bdig:1,sn:0x2,lng:"eng",beasy:0,bwar:1},
			{bdig:0,l21:1},
			...
		]
		*/
		int i;
		sprintf(buf, "[");
		for (i=0; i<psd->i_number_of_services; i++) {
				dvbpsi_caption_service_t *tmp_cap = &psd->services[i];
				if (i != 0)
						snprintf(buf, buf_size, "%s,", buf);
				if (tmp_cap->b_digital_cc) {
						snprintf(buf, buf_size, "%s{bdig:1,sn:%#x", buf, tmp_cap->i_caption_service_number);
						char lang[8] = {0};
						if (tmp_cap->i_iso_639_code[0] == 0)
								snprintf(lang, sizeof(lang), "cap");
						else
								memcpy(lang, tmp_cap->i_iso_639_code, 3);
						lang[3] = 0;
						snprintf(buf, buf_size, "%s,lng:\"%s\",beasy:%d,bwar:%d,private_data:%d}",
								buf, lang, tmp_cap->b_easy_reader, tmp_cap->b_wide_aspect_ratio,
								tmp_cap->private_data);
				} else {
						snprintf(buf, buf_size, "%s{bdig:0,l21:%d}", buf, tmp_cap->b_line21_field);
				}
		}
		snprintf(buf, buf_size, "%s]", buf);
		return AM_SUCCESS;
}

AM_ErrorCode_t AM_SI_GetRatingStringFromDescriptors(dvbpsi_descriptor_t *p_descriptor, char *buf, int buf_size)
{
	dvbpsi_descriptor_t *descr;

	if (!buf || !buf_size)
		return AM_SUCCESS;

	AM_SI_LIST_BEGIN(p_descriptor, descr)
		if (descr->p_decoded && descr->i_tag == AM_SI_DESCR_CONTENT_ADVISORY)
		{
			dvbpsi_atsc_content_advisory_dr_t *pcad = (dvbpsi_atsc_content_advisory_dr_t*)descr->p_decoded;
			AM_SI_GetRatingString(pcad, buf, buf_size);
			return AM_SUCCESS;
		}
	AM_SI_LIST_END()

	return AM_SUCCESS;
}

AM_ErrorCode_t AM_SI_GetATSCCaptionStringFromDescriptors(dvbpsi_descriptor_t *p_descriptor, char *buf, int buf_size)
{
	dvbpsi_descriptor_t *descr;

	if (!buf || !buf_size)
		return AM_SUCCESS;
	AM_SI_LIST_BEGIN(p_descriptor, descr)
		if (descr->p_decoded && descr->i_tag == AM_SI_DESCR_CAPTION_SERVICE)
		{
			dvbpsi_atsc_caption_service_dr_t *pcsd = (dvbpsi_atsc_caption_service_dr_t*)descr->p_decoded;
			AM_SI_GetATSCCaptionString(pcsd, buf, buf_size);
			return AM_SUCCESS;
		}
	AM_SI_LIST_END()

	return AM_SUCCESS;
}

