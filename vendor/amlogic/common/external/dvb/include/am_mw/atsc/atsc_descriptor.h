/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package. *
* Description:
*/

#ifndef _ATSC_DESCRIPTOR_H
#define _ATSC_DESCRIPTOR_H

#include "atsc_types.h"

typedef struct 
{
	uint8_t		i_string_count;
	struct
	{
		uint8_t		iso_639_code[3];
		uint8_t		string[2048];
	}string[10]; /* Up to 10 langs */
}atsc_multiple_string_t;

typedef struct atsc_descriptor_s
{
	uint8_t                       i_tag;          /*!< descriptor_tag */
	uint8_t                       i_length;       /*!< descriptor_length */

	uint8_t *                     p_data;         /*!< content */

	struct atsc_descriptor_s *  p_next;         /*!< next element of
		                                             the list */

	void *                        p_decoded;      /*!< decoded descriptor */

} atsc_descriptor_t;


/*ATSC Descriptors definition*/

/*Service Location*/
typedef struct
{
	uint8_t		i_elem_count;
	uint16_t	i_pcr_pid;
	struct 
	{
		uint8_t		i_stream_type;
		uint16_t	i_pid;
		uint8_t		iso_639_code[3];    /*!< ISO_639_language_code */
		uint8_t	i_audio_type;
	} elem[44];

} atsc_service_location_dr_t;

/* Content Advisory*/
typedef struct
{
	uint8_t	 i_region_count;
	struct
	{
		uint8_t i_rating_region;
		uint8_t	 i_dimension_count;
		struct
		{
			uint8_t	 i_dimension_j;
			uint8_t	 i_rating_value; 
		}dimension[126];
		atsc_multiple_string_t rating_description;
	}region[64];
}atsc_content_advisory_dr_t;



atsc_descriptor_t* atsc_NewDescriptor(uint8_t i_tag, uint8_t i_length,
                                          uint8_t* p_data);
void atsc_DeleteDescriptors(atsc_descriptor_t* p_descriptor);

void atsc_decode_multiple_string_structure(INT8U *buffer_data, atsc_multiple_string_t *out);
int atsc_convert_code_from_utf16_to_utf8(char *in_code,int in_len,char *out_code,int *out_len);

#endif /* end */
