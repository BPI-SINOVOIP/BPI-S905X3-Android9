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
#include "atsc/atsc_rrt.h"

#define MAKE_SHORT_HL(exp)		            ((INT16U)((exp##_hi<<8)|exp##_lo))
#define MAKE_WORD_HML(exp)			((INT32U)(((exp##_hi)<<24)|((exp##_mh)<<16)|((exp##_ml)<<8)|exp##_lo))

static rrt_section_info_t *new_rrt_section_info(void)
{
    rrt_section_info_t *tmp = NULL;

    tmp = AMMem_malloc(sizeof(rrt_section_info_t));
    if(tmp)
    {
        tmp->rating_region = 0;
        tmp->version_number = DVB_INVALID_VERSION;
        tmp->dimensions_defined = 0;
        tmp->dimensions_info = NULL;
	 	memset(&tmp->rating_region_name, 0, sizeof(tmp->rating_region_name));
	 	tmp->desc = NULL;
        tmp->p_next = NULL;
    }

    return tmp;
}

static void add_rrt_section_info(rrt_section_info_t **head, rrt_section_info_t *info)
{
    rrt_section_info_t **tmp = head;
    rrt_section_info_t **temp = head;

    if(info && head)
    {
        if(*tmp == NULL)
        {
            info->p_next = NULL;
            *tmp = info;
        }
        else
        {
            while(*tmp)
            {
                if(*tmp == info)
                {
                    return ;
                }

                temp = tmp;
                tmp = &(*tmp)->p_next;
            }

            info->p_next = NULL;
            (*temp)->p_next = info;

            return ;
        }
    }

    return ;
}

void add_rrt_dimensions_info(rrt_dimensions_info_t **head, rrt_dimensions_info_t *info)
{
	rrt_dimensions_info_t **tmp = head;
	rrt_dimensions_info_t **node = head;

	if (info && head)
	{
		if (*tmp == NULL)
		{
			info->p_next = NULL;
			*tmp = info;
		}
		else
		{
			while (*tmp)
			{
				if (*tmp == info)
				{
					return ;
				}
				node = tmp;
				tmp = &(*tmp)->p_next;
			}
			info->p_next =NULL;
			(*node)->p_next = info;
			return ;
		}
	}
	return ;
}

INT32S atsc_psip_parse_rrt(INT8U* data, INT32U length, rrt_section_info_t *info)
{
	INT8U *sect_data = data;
	INT32S sect_len = length;
	rrt_section_info_t *sect_info = info;
	rrt_section_header_t *sect_head = NULL;
	rrt_dimensions_info_t *tmp_dimensions_info = NULL;
	INT32S dimensions_defined = 0;
	INT32S rating_region_name_len;
	INT32S dimensions_name_len;
	INT32S values_defined, text_len;
	INT32S i;
	INT8U *ptr = NULL;

	if(data && length && info)
	{
		while(sect_len > 0)
		{
			sect_head = (rrt_section_header_t*)sect_data;

			if(sect_head->table_id != ATSC_PSIP_RRT_TID)
			{
				sect_len -= 3;
				sect_len -= MAKE_SHORT_HL(sect_head->section_length);

				sect_data += 3;
				sect_data += MAKE_SHORT_HL(sect_head->section_length);

				continue;
			}

			if(NULL == sect_info)
			{
				sect_info = new_rrt_section_info();
				if(sect_info == NULL)
				{
					AM_TRACE("[%s] error out of memory !!\n", __FUNC__);
					break;
				}
				else
				{
					add_rrt_section_info(&info, sect_info);
				}
			}

			sect_info->rating_region = sect_head->rating_region;
			sect_info->version_number = sect_head->version_number;

			ptr = sect_data + RRT_SECTION_HEADER_LEN;
			rating_region_name_len = *ptr++;
			if (rating_region_name_len > 0)
				atsc_decode_multiple_string_structure(ptr, &sect_info->rating_region_name);

			ptr += rating_region_name_len;
			dimensions_defined = *ptr++;
			sect_info->dimensions_defined = dimensions_defined;
			
			while (dimensions_defined)
			{
				dimensions_name_len = *ptr++;
				tmp_dimensions_info = (rrt_dimensions_info_t *)AMMem_malloc(sizeof(rrt_dimensions_info_t));
				if (tmp_dimensions_info)
				{
					memset(tmp_dimensions_info, 0, sizeof(rrt_dimensions_info_t));
					if (dimensions_name_len > 0)
						atsc_decode_multiple_string_structure(ptr, &tmp_dimensions_info->dimensions_name); //
					ptr += dimensions_name_len;
					tmp_dimensions_info->graduated_scale = ((*ptr)&0x10) >> 4;
					values_defined = (*ptr++) & 0x0F;
					tmp_dimensions_info->values_defined = values_defined;
					for (i = 0; i < values_defined; i++)
					{
						text_len = *ptr++;
						if (text_len > 0)
							atsc_decode_multiple_string_structure(ptr, &tmp_dimensions_info->rating_value[i].abbrev_rating_value_text);
						ptr += text_len;
						
						text_len = *ptr++;
						if (text_len > 0)
							atsc_decode_multiple_string_structure(ptr, &tmp_dimensions_info->rating_value[i].rating_value_text);
						ptr += text_len;
					}
					tmp_dimensions_info->p_next = NULL;

					add_rrt_dimensions_info(&sect_info->dimensions_info, tmp_dimensions_info);
				}
				else
				{
					AM_TRACE("[%s] error out of memory !!\n", __FUNC__);
					break;
				}         
				dimensions_defined--;
			}

			sect_len -= 3;
			sect_len -= MAKE_SHORT_HL(sect_head->section_length);

			sect_data += 3;
			sect_data += MAKE_SHORT_HL(sect_head->section_length);

			sect_info = sect_info->p_next;
		}

		return 0;
	}

	return -1;
}

void atsc_psip_clear_rrt_info(rrt_section_info_t *info)
{
	rrt_section_info_t *sect_info = NULL;
	rrt_section_info_t *tmp_sect_info = NULL;
	INT8U i;

	if(info)
	{
		tmp_sect_info = info->p_next;

		while(tmp_sect_info)
		{
			rrt_dimensions_info_t *dimensions_info = NULL;
			rrt_dimensions_info_t *tmp_dimensions_info = NULL;
			tmp_dimensions_info = tmp_sect_info->dimensions_info;
			while(tmp_dimensions_info)
			{
				dimensions_info = tmp_dimensions_info->p_next;
				AMMem_free(tmp_dimensions_info);
				tmp_dimensions_info = dimensions_info;
			}
			sect_info = tmp_sect_info->p_next;
			AMMem_free(tmp_sect_info);
			tmp_sect_info = sect_info;
		}
		if(info->dimensions_info)
		{
			AMMem_free(info->dimensions_info);
		}
		if(info->desc)
		{
			atsc_DeleteDescriptors(info->desc);
		}
		info->rating_region = 0;
		info->version_number = DVB_INVALID_VERSION;
		info->dimensions_defined = 0;
		memset(&info->rating_region_name, 0, sizeof(info->rating_region_name));
		info->dimensions_info = NULL;
		info->desc = NULL;
		info->p_next = NULL;
	}
	return ;
}

rrt_section_info_t *atsc_psip_new_rrt_info(void)
{
    return new_rrt_section_info();
}

void atsc_psip_free_rrt_info(rrt_section_info_t *info)
{
	rrt_dimensions_info_t *dimensions_info = NULL;
	rrt_dimensions_info_t *tmp_dimensions_info =NULL;
	tmp_dimensions_info = info->dimensions_info;
	while(tmp_dimensions_info)
	{
		dimensions_info = tmp_dimensions_info->p_next;
		AMMem_free(tmp_dimensions_info);
		tmp_dimensions_info = dimensions_info;
	}
	if (info->desc)
	{
		atsc_DeleteDescriptors(info->desc);	 // --
	}
	AMMem_free(info);
}


