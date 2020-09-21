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
#include "atsc/atsc_eit.h"

#define MAKE_SHORT_HL(exp)		            ((INT16U)((exp##_hi<<8)|exp##_lo))
#define MAKE_WORD_HML(exp)			((INT32U)(((exp##_hi)<<24)|((exp##_mh)<<16)|((exp##_ml)<<8)|exp##_lo))
#define MAKE_WORD_HML24(exp)			((INT32U)(((exp##_mh)<<16)|((exp##_ml)<<8)|exp##_lo))

#define MAJOR_CHANNEL_NUM(exp) ((INT16U)((exp##_hi<<6)|exp##_lo))
#define MINOR_CHANNEL_NUM(exp) ((INT16U)((exp##_hi<<8)|exp##_lo))

static eit_section_info_t *new_eit_section_info(void)
{
    eit_section_info_t *tmp = NULL;

    tmp = AMMem_malloc(sizeof(eit_section_info_t));
    if(tmp)
    {
        tmp->source_id = 0;
        tmp->version_number = DVB_INVALID_VERSION;
        tmp->num_events_in_section = 0;
        tmp->eit_event_info = NULL;
        tmp->p_next = NULL;
    }

    return tmp;
}

static void add_eit_section_info(eit_section_info_t **head, eit_section_info_t *info)
{
    eit_section_info_t **tmp = head;
    eit_section_info_t **temp = head;

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

void add_eit_event_info(eit_event_info_t **head, eit_event_info_t *info)
{
	eit_event_info_t **tmp = head;
	eit_event_info_t **node = head;

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

INT32S atsc_psip_parse_eit(INT8U* data, INT32U length, eit_section_info_t *info)
{
	INT8U *sect_data = data;
	INT32S sect_len = length;
	eit_section_info_t *sect_info = info;
	eit_section_header_t *sect_head = NULL;
	event_sect_info_t *eit_sect_evt = NULL;
	eit_event_info_t *tmp_evt_info = NULL;
	INT8U num_events_in_section = 0;
	INT8U *ptr = NULL;
	INT16S desc_len, desc_len_back;
	INT8U *desc_ptr;

	if(data && length && info)
	{
		while(sect_len > 0)
		{
			sect_head = (eit_section_header_t*)sect_data;

			if(sect_head->table_id != ATSC_PSIP_EIT_TID)
			{
				sect_len -= 3;
				sect_len -= MAKE_SHORT_HL(sect_head->section_length);

				sect_data += 3;
				sect_data += MAKE_SHORT_HL(sect_head->section_length);

				continue;
			}

			if(NULL == sect_info)
			{
				sect_info = new_eit_section_info();
				if(sect_info == NULL)
				{
					AM_TRACE("[%s] error out of memory !!\n", __FUNC__);
					break;
				}
				else
				{
					add_eit_section_info(&info, sect_info);
				}
			}

			sect_info->source_id = MAKE_SHORT_HL(sect_head->source_id);
			sect_info->version_number = sect_head->version_number;

			num_events_in_section = sect_head->num_events_in_section;
			sect_info->num_events_in_section= num_events_in_section;
			ptr = sect_data+EIT_SECTION_HEADER_LEN;
			while (num_events_in_section)
			{
				eit_sect_evt = (event_sect_info_t *)ptr;
				tmp_evt_info= (eit_event_info_t *)AMMem_malloc(sizeof(eit_event_info_t));
				if (tmp_evt_info)
				{
					memset(tmp_evt_info, 0, sizeof(eit_event_info_t));

					tmp_evt_info->event_id = MAKE_SHORT_HL(eit_sect_evt->event_id);
					tmp_evt_info->start_time = MAKE_WORD_HML(eit_sect_evt->start_time);
					tmp_evt_info->start_time += 315964800; /* secs_Between_1Jan1970_6Jan1980*/
					
					tmp_evt_info->ETM_location = eit_sect_evt->ETM_location;
					tmp_evt_info->length_in_seconds = MAKE_WORD_HML24(eit_sect_evt->length_in_seconds);
					
					if (eit_sect_evt->title_length > 0)
						atsc_decode_multiple_string_structure(ptr+10, &tmp_evt_info->title);
					tmp_evt_info->desc = NULL;
					tmp_evt_info->p_next = NULL;

					add_eit_event_info(&sect_info->eit_event_info, tmp_evt_info);
					
					desc_ptr = ptr + 10 + eit_sect_evt->title_length;
					desc_len = ((INT16U)(desc_ptr[0]&0x0f)<<8) | (INT16U)desc_ptr[1];
					desc_len_back = desc_len;
					desc_ptr += 2;
					while (desc_len > 0)
					{
						atsc_descriptor_t* p_descriptor = atsc_NewDescriptor(desc_ptr[0], desc_ptr[1], desc_ptr + 2);

						if(p_descriptor)
						{
							if(tmp_evt_info->desc == NULL)
							{
								tmp_evt_info->desc = p_descriptor;
							}
							else
							{
								atsc_descriptor_t* p_last_descriptor = tmp_evt_info->desc;
								while(p_last_descriptor->p_next != NULL)
									p_last_descriptor = p_last_descriptor->p_next;
								p_last_descriptor->p_next = p_descriptor;
							}
						}
						desc_len -= desc_ptr[1] + 2;
						desc_ptr += desc_ptr[1] + 2;
					}
					ptr += 12 + eit_sect_evt->title_length + desc_len_back;
					num_events_in_section--;
				}
				else
				{
					AM_TRACE("[%s] error out of memory !!\n", __FUNC__);
					break;
				}         
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

void atsc_psip_clear_eit_info(eit_section_info_t *info)
{
	eit_section_info_t *eit_sect_info = NULL;
	eit_section_info_t *tmp_eit_sect_info = NULL;

	if(info)
	{
		tmp_eit_sect_info = info->p_next;

		while(tmp_eit_sect_info)
		{
			eit_event_info_t *evt_info = NULL;
			eit_event_info_t *tmp_evt_info = NULL;
			tmp_evt_info = tmp_eit_sect_info->eit_event_info;
			while(tmp_evt_info)
			{
				if(tmp_evt_info->desc)
				{
					atsc_DeleteDescriptors(tmp_evt_info->desc);
					tmp_evt_info->desc = NULL;
				}
				evt_info = tmp_evt_info->p_next;
				AMMem_free(tmp_evt_info);
				tmp_evt_info = evt_info;
			}
			eit_sect_info = tmp_eit_sect_info->p_next;
			AMMem_free(tmp_eit_sect_info);
			tmp_eit_sect_info = eit_sect_info;
		}
		if(info->eit_event_info)
		{
			AMMem_free(info->eit_event_info);
		}
		
		info->source_id = DVB_INVALID_ID;
		info->version_number = DVB_INVALID_VERSION;
		info->num_events_in_section = 0;
		info->eit_event_info = NULL;
		info->p_next = NULL;
	}
	return ;
}

eit_section_info_t *atsc_psip_new_eit_info(void)
{
    return new_eit_section_info();
}

void atsc_psip_free_eit_info(eit_section_info_t *info)
{
	eit_event_info_t *eit_event_info = NULL;
	eit_event_info_t *tmp_eit_event_info =NULL;
	tmp_eit_event_info = info->eit_event_info;
	while(tmp_eit_event_info)
	{
		if (tmp_eit_event_info->desc)
		{
			atsc_DeleteDescriptors(tmp_eit_event_info->desc);
			tmp_eit_event_info->desc = NULL;
		}
		eit_event_info = tmp_eit_event_info->p_next;
		AMMem_free(tmp_eit_event_info);
		tmp_eit_event_info = eit_event_info;
	}
	AMMem_free(info);
}

