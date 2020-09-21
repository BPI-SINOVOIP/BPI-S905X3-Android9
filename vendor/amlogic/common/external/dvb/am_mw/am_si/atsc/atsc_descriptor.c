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
#include "atsc/atsc_types.h"
#include "atsc/atsc_descriptor.h"
#include "atsc/huffman_decode.h"
#include <errno.h>
#include <am_iconv.h>

#define SHORT_NAME_LEN (14)

static unsigned char BMPtoChar(unsigned char codeBMP1, unsigned char codeBMP0)
{
	unsigned char codeChar;
	unsigned short int temp;
	if ((codeBMP0) <= 0x7f)
		codeChar = codeBMP0;
	else{
		temp = (codeBMP1&0x3)<<6;
		temp += codeBMP0&0x3f;
		codeChar = temp;
	}
	return codeChar;
}

INT32S short_channel_name_parse(INT8U* pstr, INT8U *out_str)
{
	INT8U i;
	for (i=0; i<(SHORT_NAME_LEN/2); i++)
	{
		out_str[i] = BMPtoChar(pstr[i*2], pstr[i*2+1]);
	}
	return 0;
}


int atsc_convert_code_from_utf16_to_utf8(char *in_code,int in_len,char *out_code,int *out_len)
{
    iconv_t handle;
    char **pin = &in_code;
    char **pout = &out_code;
    size_t inLength = in_len;
    size_t outLength = *out_len;
	if (!in_code || !out_code || !out_len || in_len <= 0 || *out_len <= 0)
		return AM_FAILURE;

	memset(out_code, 0, *out_len);

    handle=iconv_open("utf-8","utf-16");

    if (handle == (iconv_t)-1)
    {
    	AM_TRACE("convert_code_to_utf8 iconv_open err");
    	return -1;
    }

    if (iconv(handle, pin, &inLength, pout, &outLength) == (size_t)-1)
    {
        AM_TRACE("convert_code_to_utf8 iconv err: %s, in_len %d, out_len %d", strerror(errno), in_len, *out_len);
        iconv_close(handle);
        return -1;
    }
    *out_len = (int)(*out_len - outLength);
    return iconv_close(handle);
}

void atsc_decode_multiple_string_structure(INT8U *buffer_data, atsc_multiple_string_t *out)
{
	unsigned char *p;
	unsigned char number_strings;
	unsigned char i,j,k;
	unsigned long ISO_639_language_code;
	unsigned char number_segments;
	unsigned char compression_type;
	unsigned char mode;
	unsigned char number_bytes;
	unsigned char *tmp_buff;
	int str_bytes, left_bytes;
	unsigned char utf16_buf[2*2048];

	if (!buffer_data || !out)
		return;

	p = buffer_data;

	number_strings = p[0];
	if(0 == number_strings)
	{
		return;
	}
	p++;
	memset(out, 0, sizeof(atsc_multiple_string_t));

	if (number_strings > (int)AM_ARRAY_SIZE(out->string))
		number_strings = AM_ARRAY_SIZE(out->string);
	out->i_string_count = number_strings;	
	for (i=0; i < number_strings; i++)
	{
		out->string[i].iso_639_code[0] = p[0];
		out->string[i].iso_639_code[1] = p[1];
		out->string[i].iso_639_code[2] = p[2];

		/*AM_TRACE("multiple_string_structure-->lang '%c%c%c'", p[0], p[1], p[2]);*/

		number_segments = p[3];

		if(0 == number_segments)
		{
			p += 4;
			continue;
		}

		p +=4;
		tmp_buff = out->string[i].string;
		for (j = 0; j< number_segments; j++)
		{
			compression_type = p[0];	
			mode = p[1];

			number_bytes = p[2];

			if(0 == number_bytes)
			{
				p += 3;
				continue;
			}
			left_bytes = out->string[i].string + sizeof(out->string[i].string) - tmp_buff; /* left bytes in out->string[i].string */
			if (left_bytes <= 0)
			{
				AM_TRACE("no more space for new segment of this string");
				break;
			}
			p+=3;
			/*AM_TRACE("mode %d, ct %d", mode, compression_type);*/
			if ((mode == 0) && ((compression_type == 1) | (compression_type == 2)))
			{
				/* tmp_buff may overflow here, as we cannot get the out length,
				 * we make the out->string[i].string large enough to avoid this bug 
				 */
				str_bytes = atsc_huffman_to_string(tmp_buff, p, number_bytes, compression_type-1);
				tmp_buff += str_bytes;
			}
			else if ((mode >= 0x0 && mode <= 0x7) || (mode >= 0x9 && mode <= 0x10) ||
				(mode >= 0x20 && mode <= 0x27) || (mode >= 0x30 && mode <= 0x33) || mode == 0x3F)
			{
				if (mode != 0x3F)
				{
					for (k = 0; k < number_bytes; k++)
					{
						utf16_buf[2*k] = mode;
						utf16_buf[2*k+1] = p[k];
					}
				}
				else
				{
					/* UTF16 form now*/
					memcpy(utf16_buf, p, number_bytes);
				}
				/* convert utf16 to utf8 */
				str_bytes = left_bytes;
				atsc_convert_code_from_utf16_to_utf8((char*)utf16_buf, (mode==0x3F)?number_bytes:2*number_bytes, (char*)tmp_buff, &str_bytes);
				tmp_buff += str_bytes;
			}
			else if (mode == 0x3E)
			{
				/* SCSU */
				memcpy(tmp_buff, p, number_bytes);
				tmp_buff += number_bytes;
			}
			else
			{
				AM_TRACE("multiple_string_structure: unsupported mode 0x%x, compression_type %d",
					mode, compression_type);
				tmp_buff += number_bytes;
			}
			p += number_bytes;
		}
	}	
}

INT8U *audio_stream_desc_parse(INT8U *ptrData)
{
	UNUSED(ptrData);
	return NULL;
}

INT8U *caption_service_desc_parse(INT8U *ptrData)
{
	UNUSED(ptrData);
	return NULL;
}

INT8U *content_advisory_desc_parse(INT8U *ptrData)
{
#if 0
	INT32U i;
	INT8U rating_region_count;
	INT8U rating_desc_length;
	INT8U rating_dimentions;
//	INT8U rating_region; 
	INT8U *ptr = ptrData;
	INT8U *str;
	
	rating_region_count  = RatingRegionCount(ptr);
	ptr += 3;
	for(i=0; i<rating_region_count; i++)
	{
		ptr++;
		rating_dimentions = *ptr;
		ptr += rating_dimentions * 2;

		rating_desc_length = *ptr;
		// test 
		ptr++;
		str = MemMalloc(rating_desc_length);
		parse_multiple_string(ptr, str);
		ptr += rating_desc_length;
		AM_TRACE("%s\n", str);
	}
#else
	UNUSED(ptrData);
#endif
	return NULL;
}


static atsc_service_location_dr_t* atsc_DecodeServiceLocationDr(atsc_descriptor_t * p_descriptor)
{
	atsc_service_location_dr_t * p_decoded;
	int i;

	/* Check the tag */
	if(p_descriptor->i_tag != 0xa1)
	{
		AM_TRACE("dr_service_location decoder,bad tag (0x%x)", p_descriptor->i_tag);
		return NULL;
	}

	/* Don't decode twice */
	if(p_descriptor->p_decoded)
		return p_descriptor->p_decoded;

	/* Allocate memory */
	p_decoded = (atsc_service_location_dr_t*)malloc(sizeof(atsc_service_location_dr_t));
	if(!p_decoded)
	{
		AM_TRACE("dr_service_location decoder,out of memory");
		return NULL;
	}

	/* Decode data and check the length */
	if((p_descriptor->i_length < 3))
	{
		AM_TRACE("dr_service_location decoder,bad length (%d)",
				         p_descriptor->i_length);
		free(p_decoded);
		return NULL;
	}
	
	p_decoded->i_elem_count = p_descriptor->p_data[2];
	if (p_decoded->i_elem_count > (int)AM_ARRAY_SIZE(p_decoded->elem))
		p_decoded->i_elem_count = AM_ARRAY_SIZE(p_decoded->elem);
	p_decoded->i_pcr_pid = ((uint16_t)(p_descriptor->p_data[0] & 0x1f) << 8) | p_descriptor->p_data[1];
	i = 0;
	while( i < p_decoded->i_elem_count) 
	{
		
		p_decoded->elem[i].i_stream_type = p_descriptor->p_data[i*6+3];
		p_decoded->elem[i].i_pid = ((uint16_t)(p_descriptor->p_data[i*6+4] & 0x1f) << 8) | p_descriptor->p_data[i*6+5];
		p_decoded->elem[i].iso_639_code[0] = p_descriptor->p_data[i*6+6];
		p_decoded->elem[i].iso_639_code[1] = p_descriptor->p_data[i*6+7];
		p_decoded->elem[i].iso_639_code[2] = p_descriptor->p_data[i*6+8];
		AM_TRACE("ServiceLocationDesc: stream_type %d, pid %d", p_decoded->elem[i].i_stream_type, p_decoded->elem[i].i_pid);
		i++;
	}
	p_descriptor->p_decoded = (void*)p_decoded;

	return p_decoded;
}

static atsc_content_advisory_dr_t* atsc_DecodeContentAdvisoryDr(atsc_descriptor_t * p_descriptor)
{
	atsc_content_advisory_dr_t * p_decoded;
	int i, j;
	uint8_t *ptr;

	/* Check the tag */
	if(p_descriptor->i_tag != 0x87)
	{
		AM_TRACE("dr_content_advisory decoder,bad tag (0x%x)", p_descriptor->i_tag);

		return NULL;
	}

	/* Don't decode twice */
	if(p_descriptor->p_decoded)
		return p_descriptor->p_decoded;

	/* Allocate memory */
	p_decoded = (atsc_content_advisory_dr_t*)malloc(sizeof(atsc_content_advisory_dr_t));
	if(!p_decoded)
	{
		AM_TRACE("dr_content_advisory decoder,out of memory");
		return NULL;
	}

	/* Decode data and check the length */
	if((p_descriptor->i_length < 1))
	{
		AM_TRACE("dr_content_advisory decoder,bad length (%d)",
				         p_descriptor->i_length);
		free(p_decoded);
		return NULL;
	}
	
	p_decoded->i_region_count = p_descriptor->p_data[0] & 0x3f;
	i = 0;
	ptr = p_descriptor->p_data + 1;
	/*AM_TRACE("region count %d", p_decoded->i_region_count);*/
	while( i < p_decoded->i_region_count) 
	{
		/* to this region */
		p_decoded->region[i].i_rating_region = ptr[0];
		p_decoded->region[i].i_dimension_count = ptr[1];
		/*AM_TRACE("dimension count %d", p_decoded->region[i].i_dimension_count);*/
		ptr += 2;
		
		if (p_decoded->region[i].i_dimension_count > (int)AM_ARRAY_SIZE(p_decoded->region[i].dimension))
			p_decoded->region[i].i_dimension_count = AM_ARRAY_SIZE(p_decoded->region[i].dimension);
		/* dimensions */	
		j = 0;
		while (j < p_decoded->region[i].i_dimension_count)
		{
			p_decoded->region[i].dimension[j].i_dimension_j = ptr[0];
			p_decoded->region[i].dimension[j].i_rating_value = ptr[1]&0x0f;
			
			/*AM_TRACE("dimension_j/value: %d/%d",
				p_decoded->region[i].dimension[j].i_dimension_j,
				p_decoded->region[i].dimension[j].i_rating_value);*/
			ptr += 2;
			j++;
		}
		
		/* decode rating description */
		if (ptr[0] > 0)
			atsc_decode_multiple_string_structure(ptr+1, &p_decoded->region[i].rating_description);
		else
			memset(&p_decoded->region[i].rating_description, 0, sizeof(p_decoded->region[i].rating_description));
			
		/*AM_TRACE("ContentAdvisoryDesc: region %d, description(lang0 '%c%c%c':%s)", 
			p_decoded->region[i].i_rating_region,
			p_decoded->region[i].rating_description.string[0].iso_639_code[0],
			p_decoded->region[i].rating_description.string[0].iso_639_code[1],
			p_decoded->region[i].rating_description.string[0].iso_639_code[2],
			p_decoded->region[i].rating_description.string[0].string);*/
		ptr += ptr[0] + 1;
		i++;
	}
	p_descriptor->p_decoded = (void*)p_decoded;

	return p_decoded;
}


static void atsc_decode_descriptor(atsc_descriptor_t *p_descriptor)
{
	switch (p_descriptor->i_tag)
	{
		case 0xA1:
			/*service location descriptor*/
			atsc_DecodeServiceLocationDr(p_descriptor);
			break;
		case 0x87:
			/* content advisory descriptor */
			atsc_DecodeContentAdvisoryDr(p_descriptor);
			break;
		default:
			break;
	}
}


atsc_descriptor_t* atsc_NewDescriptor(uint8_t i_tag, uint8_t i_length, uint8_t* p_data)
{
	atsc_descriptor_t* p_descriptor
                = (atsc_descriptor_t*)malloc(sizeof(atsc_descriptor_t));

	if(p_descriptor)
	{
		p_descriptor->p_data = (uint8_t*)malloc(i_length * sizeof(uint8_t));

		if(p_descriptor->p_data)
		{
			p_descriptor->i_tag = i_tag;
			p_descriptor->i_length = i_length;
			if(p_data)
				memcpy(p_descriptor->p_data, p_data, i_length);
			p_descriptor->p_decoded = NULL;
			p_descriptor->p_next = NULL;

			/*Decode it*/
			atsc_decode_descriptor(p_descriptor);
		}
		else
		{
			free(p_descriptor);
			p_descriptor = NULL;
		}
	}

	return p_descriptor;
}

void atsc_DeleteDescriptors(atsc_descriptor_t* p_descriptor)
{
	while(p_descriptor != NULL)
	{ 
		atsc_descriptor_t* p_next = p_descriptor->p_next;

		if(p_descriptor->p_data != NULL)
			free(p_descriptor->p_data);

		if(p_descriptor->p_decoded != NULL)
			free(p_descriptor->p_decoded);

		free(p_descriptor);
		p_descriptor = p_next;
	}
}

