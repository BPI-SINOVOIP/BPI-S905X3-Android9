/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
#ifdef _FORTIFY_SOURCE
#undef _FORTIFY_SOURCE
#endif

#define AM_DEBUG_LEVEL 5

#define PESPACKET_STREAMID_MASK 0x100
#define PROGRAM_STREAM_MAP 0x1bc
#define PRIVATE_STREAM_2   0x1bf
#define ECM_STREAM		   0x1f0
#define EMM_STREAM 		   0x1f1
#define PROGRAM_STREAM_DIRECTORY 0x1ff
#define DSMCC_STREAM       0x1f2
#define H222_TYPE_E_STREAM 0x1f8
#define PADDING_STREAM 	   0x1be

#define PES_PAYLOADINFO_LEN			3
#define PES_PAECKET_HEAD_LEN		6
#define PES_START_CODE_LEN			3
#define AC3_PAYLOAD_START_CODE_1	0x0b77
#define AC3_PAYLOAD_START_CODE_2	0x770b

#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <am_debug.h>
#include <am_pes.h>
#include <am_util.h>
#include <amports/aformat.h>

typedef struct
{
	uint8_t *buf;
	int      size;
	int      len;
	AM_PES_Para_t para;
}AM_PES_Parser_t;


static int calc_invalid_pes_offset(uint8_t  *data, int data_len, int *plen, int *dpos, int *dlen)
{
	int posn = 6;
	int i = 0;
	int pes_head_len = 0;
	int found = 0;
	pes_head_len = data[8];
	/*find next 00 00 01 head*/
	for (i=posn; i< data_len - 4; i++)
	{
		if ((data[i] == 0) && (data[i+1] == 0) && (data[i+2] == 1))
		{
			posn = i;
			found = 1;
			break;
		}
	}
	if (found ==1) {
		if (posn < pes_head_len + 9) {
	      /*del data*/
		  *dpos = *dpos + posn;
		  *dlen = 0;
		  *plen = posn - 6;
		} else {
		  *dpos = *dpos + pes_head_len + 9;
		  *dlen = posn -(pes_head_len + 9);
		  *plen = posn - 6;
		}
	} else {
		return -1;
	}
	return 0;
}

static int calc_mpeg1_pes_offset(uint8_t  *data, int data_len)
{
	int posn = 6;

	while (posn < data_len && data[posn] == 0xFF)
		posn++;

	if (posn < data_len)
	{
		if ((data[posn] & 0xC0) == 0x40)
		posn += 2;

		if ((data[posn] & 0xF0) == 0x20)
			posn += 5;
		else if ((data[posn] & 0xF0) == 0x30)
			posn += 10;
		else if (data[posn] == 0x0F)
			posn ++;
		else
			posn ++;
	}
	return posn;
}


/**\brief 创建一个PES分析器
 * \param[out] 返回创建的句柄
 * \param[in] para PES分析器参数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_sub.h)
 */
AM_ErrorCode_t AM_PES_Create(AM_PES_Handle_t *handle, AM_PES_Para_t *para)
{
	AM_PES_Parser_t *parser;

	if (!handle || !para)
	{
		return AM_PES_ERR_INVALID_PARAM;
	}

	parser = (AM_PES_Parser_t *)malloc(sizeof(AM_PES_Parser_t));
	if (!parser)
	{
		return AM_PES_ERR_NO_MEM;
	}

	memset(parser, 0, sizeof(AM_PES_Parser_t));

	parser->para = *para;

	*handle = parser;

	return AM_SUCCESS;
}

/**\brief 释放一个PES分析器
 * \param 句柄
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_sub.h)
 */
AM_ErrorCode_t AM_PES_Destroy(AM_PES_Handle_t handle)
{
	AM_PES_Parser_t *parser = (AM_PES_Parser_t *)handle;

	if (!parser)
	{
		return AM_PES_ERR_INVALID_HANDLE;
	}

	if (parser->buf)
	{
		free(parser->buf);
	}

	free(parser);

	return AM_SUCCESS;
}

/**\brief 分析PES数据
 * \param 句柄
 * \param[in] buf PES数据缓冲区
 * \param size 缓冲区中数据大小
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_sub.h)
 */
AM_ErrorCode_t AM_PES_Decode(AM_PES_Handle_t handle, uint8_t *buf, int size)
{
	AM_PES_Parser_t *parser = (AM_PES_Parser_t *)handle;
	int pos, total, left;

	if (!parser)
	{
		return AM_PES_ERR_INVALID_HANDLE;
	}

	if (!buf || !size)
	{
		return AM_PES_ERR_INVALID_PARAM;
	}

	total = AM_MAX(size + parser->len, parser->size);
	if (total > parser->size)
	{
		uint8_t *buf;

		buf = realloc(parser->buf, total);
		if (!buf)
		{
			return AM_PES_ERR_NO_MEM;
		}

		parser->buf  = buf;
		parser->size = total;
	}


	memcpy(parser->buf + parser->len, buf, size);
	parser->len += size;
	pos = 0;

	do {
		AM_Bool_t found = AM_FALSE;
		int i, plen = 0;
		int dpos = 0, dlen = 0;
		/*pes header len*/
		int pes_header_len = 0;
		/*need skip data len*/
		int hlen = 0;
		int invalid = 0;

		for (i=pos; i< parser->len - 4; i++)
		{
			if ((parser->buf[i] == 0) && (parser->buf[i+1] == 0) && (parser->buf[i+2] == 1))
			{
				found = AM_TRUE;
				pos = i;
				break;
			}
		}

		if (!found || (parser->len - pos < 6))
		{
			goto end;
		}

		plen = (parser->buf[pos+4]<<8) | (parser->buf[pos+5]);
		if (plen == 0xffff || plen == 0x0) {
			invalid = 1;
			AM_DEBUG(1, "pes packet plen error plen[%d]parser->len[%d]", plen, parser->len);
		}
		if ((parser->len - pos) < (plen + PES_PAECKET_HEAD_LEN) && invalid == 0)
		{
			// check is valid pes packet
			// check ac3 payload is flag is ac3, is not ac3 payload
			// we need skip
			int stream_id = parser->buf[pos + PES_START_CODE_LEN] | PESPACKET_STREAMID_MASK;
			int start_code = 0;
			/* program_stream_map, private_stream_2 */
			/* ECM, EMM */
			/* program_stream_directory, DSMCC_stream */
			/* ITU-T Rec. H.222.1 type E stream */
			if (stream_id != PROGRAM_STREAM_MAP && stream_id != PRIVATE_STREAM_2 &&
				stream_id != ECM_STREAM && stream_id != EMM_STREAM &&
				stream_id != PROGRAM_STREAM_DIRECTORY && stream_id != DSMCC_STREAM &&
				stream_id != H222_TYPE_E_STREAM) {
				/*get pes header len, case have pes header*/
				pes_header_len = parser->buf[pos + PES_PAECKET_HEAD_LEN + 2];
				/*pes table skip len*/
				hlen = PES_PAECKET_HEAD_LEN + PES_PAYLOADINFO_LEN + pes_header_len;
				if (parser->para.afmt == AFORMAT_AC3 || parser->para.afmt == AFORMAT_EAC3) {
					start_code = ((uint16_t)parser->buf[pos + hlen] << 8)
					| parser->buf[pos + hlen + 1];
					if (start_code != AC3_PAYLOAD_START_CODE_1 && start_code != AC3_PAYLOAD_START_CODE_2) {
						// skip this pes data
						AM_DEBUG(1, "pes packet skip not ac3 data [0x%x]", start_code);
						plen = 0;
						goto skip;
					}
				}
			} else {
				/*payload case*/
				hlen = PES_PAECKET_HEAD_LEN;
				if (parser->para.afmt == AFORMAT_AC3 || parser->para.afmt == AFORMAT_EAC3) {
					start_code = ((uint16_t)parser->buf[pos + hlen] << 8)
								| parser->buf[pos + hlen + 1];
					if (start_code != AC3_PAYLOAD_START_CODE_1 && start_code != AC3_PAYLOAD_START_CODE_2) {
						// skip this pes data
						AM_DEBUG(1, "pes packet skip not ac3 data payload [0x%x] ", start_code);
						plen = 0;
						goto skip;
					}
				}
			}
			goto end;
		}

		if (parser->para.payload_only) {
			if (invalid == 1) {
				dpos = pos;
				if (calc_invalid_pes_offset(parser->buf +pos, parser->len - pos, &plen, &dpos, &dlen) != 0) {
					AM_DEBUG(1, "pes packet calc error");
					goto end;
				} else {
					AM_DEBUG(1, "pes packet calc plen=[%d]dpos[%d]dlen[%d]", plen, dpos, dlen);
				}
			} else {
				int stream_id = parser->buf[pos+3] | PESPACKET_STREAMID_MASK;
				if (stream_id == PADDING_STREAM) {
					/* padding_stream,need skip padding stream */
					AM_DEBUG(1, "pes packet skip padding");
					goto skip;
				}
				/* program_stream_map, private_stream_2 */
				/* ECM, EMM */
				/* program_stream_directory, DSMCC_stream */
				/* ITU-T Rec. H.222.1 type E stream */
				if (stream_id != PROGRAM_STREAM_MAP && stream_id != PRIVATE_STREAM_2 &&
					stream_id != ECM_STREAM && stream_id != EMM_STREAM &&
					stream_id != PROGRAM_STREAM_DIRECTORY && stream_id != DSMCC_STREAM &&
					stream_id != H222_TYPE_E_STREAM) {
					/*get pes header len, case have pes header*/
					pes_header_len = parser->buf[pos+8];
					/*pes table skip len*/
					hlen = 6 + 3 + pes_header_len;
					/*payload pos*/
					dpos = pos + hlen;
					/* payload len*/
					dlen = plen + 6 - hlen;
					/* skip stuf data*/
					AM_DEBUG(2, "pes packet has head");
				} else {
					/*payload case*/
					hlen = 6;
					dpos = pos + hlen;
					dlen = plen + 6 - hlen;
					AM_DEBUG(1, "pes packet payload");
				}
				if (0) {
					if ((parser->buf[pos+6] & 0xC0) == 0x80) {
						hlen = 6 + 3 + parser->buf[pos+8];
					} else {
						AM_DEBUG(1, "pes packet assume MPEG-1 [%#x][%#x]\n", parser->buf[pos+3], parser->buf[pos+6]);
						hlen = calc_mpeg1_pes_offset(parser->buf +pos, plen);
					}
					dpos = pos + hlen;
					dlen = plen + 6 - hlen;
				}
			}
		} else {
			dpos = pos;
			dlen = plen + 6;
		}
		if (parser->para.packet) {
			if (parser->len < dpos + dlen) {
				AM_DEBUG(1, "pes packet parse error plen=[%d]dpos[%d]dlen[%d]", parser->len, dpos, dlen);
			} else {
				parser->para.packet(handle, parser->buf + dpos, dlen);
			}
		}
skip:
		pos += plen + 6;
	} while(pos < parser->len);

end:
	left = parser->len - pos;

	if (left > 0) {
		memmove(parser->buf, parser->buf + pos, left);
	} else {
		left = 0;
	}
	parser->len = left;
	return AM_SUCCESS;
}

/**\brief 取得分析器中用户定义数据
 * \param 句柄
 * \return 用户定义数据
 */
void*          AM_PES_GetUserData(AM_PES_Handle_t handle)
{
	AM_PES_Parser_t *parser = (AM_PES_Parser_t*)handle;

	if (!parser)
	{
		return NULL;
	}

	return parser->para.user_data;
}

