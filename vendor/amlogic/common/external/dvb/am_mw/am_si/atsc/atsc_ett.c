/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: 
 */
#include "atsc/atsc_types.h"
#include "atsc/atsc_descriptor.h"
#include "atsc/atsc_ett.h"

#define AM_DEBUG_LEVEL 5

#define MAKE_SHORT_HL(exp)		            ((INT16U)((exp##_hi<<8)|exp##_lo))
#define MAKE_EVENT_ID(exp)                  ((INT16U)((exp##_hi<<6)|exp##_lo))

static ett_section_info_t *new_ett_section_info(void)
{
    ett_section_info_t *tmp = NULL;

    tmp = AMMem_malloc(sizeof(ett_section_info_t));
    if(tmp)
    {
        tmp->source_id = 0;
        tmp->version_number = DVB_INVALID_VERSION;
        tmp->table_id_extension = 0;
        tmp->ett_event_info = NULL;
        tmp->p_next = NULL;
    }

    return tmp;
}

static void add_ett_section_info(ett_section_info_t **head, ett_section_info_t *info)
{
    ett_section_info_t **tmp = head;
    ett_section_info_t **temp = head;

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

INT32S atsc_psip_parse_ett(INT8U* data, INT32U length, ett_section_info_t *info)
{
	INT8U *sect_data = data;
	INT32S sect_len = length;
	ett_section_info_t *sect_info = info;
	ett_section_header_t *sect_head = NULL;
	ett_event_sect_info_t *ett_sect_evt = NULL;
	ett_event_info_t *tmp_evt_info = NULL;	
	INT8U *ptr = NULL;
	INT16S desc_len, desc_len_back;
	INT8U *desc_ptr;

	if(data && length && info)
	{
		while(sect_len > 0)
		{
			sect_head = (ett_section_header_t*)sect_data;

			if(sect_head->table_id != ATSC_PSIP_ETT_TID)
			{
				sect_len -= 3;
				sect_len -= MAKE_SHORT_HL(sect_head->section_length);

				sect_data += 3;
				sect_data += MAKE_SHORT_HL(sect_head->section_length);

				continue;
			}

			if(NULL == sect_info)
			{
				sect_info = new_ett_section_info();
				if(sect_info == NULL)
				{
					AM_TRACE("[%s] error out of memory !!\n", __FUNC__);
					break;
				}
				else
				{
					add_ett_section_info(&info, sect_info);
				}
			}
            sect_info->i_table_id = sect_head->table_id;
			sect_info->source_id = MAKE_SHORT_HL(sect_head->source_id);
			sect_info->version_number = sect_head->version_number;
			sect_info->table_id_extension = MAKE_SHORT_HL(sect_head->table_id_ext);			
			
			ptr = sect_data+ETT_SECTION_HEADER_LEN;
			ett_sect_evt = (ett_event_sect_info_t *)ptr;
			tmp_evt_info= (ett_event_info_t *)AMMem_malloc(sizeof(ett_event_info_t));
			if (tmp_evt_info)
			{
				tmp_evt_info->event_id = MAKE_EVENT_ID(ett_sect_evt->event_id);
				tmp_evt_info->event_id_flag = ett_sect_evt->event_id_flag;
				atsc_decode_multiple_string_structure(ptr+2,&tmp_evt_info->text);
                sect_info->ett_event_info = tmp_evt_info;
			}
			else
			{
				AM_TRACE("[%s] error out of memory !!\n", __FUNC__);
				return -1;
			}

            sect_len -= 3;
			sect_len -= MAKE_SHORT_HL(sect_head->section_length);

			sect_data += 3;
			sect_data += MAKE_SHORT_HL(sect_head->section_length);

			sect_info = sect_info->p_next;
		}
        
        AM_DEBUG(1,"%s,%d",__FUNCTION__,__LINE__);
		return 0;
	}
	return -1;
}


void atsc_psip_clear_ett_info(ett_section_info_t *info)
{
	ett_section_info_t *ett_sect_info = NULL;
	ett_section_info_t *tmp_ett_sect_info = NULL;

	if(info)
	{
		tmp_ett_sect_info = info->p_next;

		while(tmp_ett_sect_info)
		{			
			ett_event_info_t *tmp_evt_info = NULL;
			tmp_evt_info = tmp_ett_sect_info->ett_event_info;
			if(tmp_evt_info)
			{
				AMMem_free(tmp_evt_info);
			}
			ett_sect_info = tmp_ett_sect_info->p_next;
			AMMem_free(tmp_ett_sect_info);
			tmp_ett_sect_info = ett_sect_info;
		}
		if(info->ett_event_info)
		{
			AMMem_free(info->ett_event_info);
		}
		
		info->source_id = DVB_INVALID_ID;
		info->version_number = DVB_INVALID_VERSION;
		info->table_id_extension = DVB_INVALID_ID;
		info->ett_event_info = NULL;
		info->p_next = NULL;
	}
	return ;
}


ett_section_info_t *atsc_psip_new_ett_info(void)
{
    return new_ett_section_info();
}


void atsc_psip_free_ett_info(ett_section_info_t *info)
{
	ett_event_info_t *tmp_ett_event_info =NULL;
	tmp_ett_event_info = info->ett_event_info;
	if(tmp_ett_event_info)
	{		
		AMMem_free(tmp_ett_event_info);		
	}
	AMMem_free(info);
}


