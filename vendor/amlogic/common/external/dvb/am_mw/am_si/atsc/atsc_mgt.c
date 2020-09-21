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
#include "atsc/atsc_mgt.h"

#define MAKE_SHORT_HL(exp)		            ((INT16U)((exp##_hi<<8)|exp##_lo))

static mgt_section_info_t *new_mgt_section_info(void)
{
    mgt_section_info_t *tmp = NULL;

    tmp = AMMem_malloc(sizeof(mgt_section_info_t));
    if(tmp)
    {
        tmp->version_number = DVB_INVALID_VERSION;
	 tmp->tables_defined = 0;
	 tmp->is_cable = 0;
        tmp->com_table_info = NULL;
	 tmp->desc = NULL;
        tmp->p_next = NULL;
    }

    return tmp;
}

static void add_mgt_section_info(mgt_section_info_t **head, mgt_section_info_t *info)
{
    mgt_section_info_t **tmp = head;
    mgt_section_info_t **temp = head;

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

void add_mgt_table_info(com_table_info_t **head, com_table_info_t *info)
{
	com_table_info_t **tmp = head;
	com_table_info_t **node = head;

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

INT32S atsc_psip_parse_mgt(INT8U* data, INT32U length, mgt_section_info_t *info)
{
    INT8U *sect_data = data;
    INT32S sect_len = length;
    mgt_section_info_t *sect_info = info;
    mgt_section_header_t *sect_head = NULL;
    mgt_table_info_t *table_info = NULL;
    com_table_info_t *tmp_table_info = NULL;
    INT16S tables_num = 0;
    INT8U *ptr = NULL;

    if(data && length && info)
    {
        while(sect_len > 0)
        {
            sect_head = (mgt_section_header_t*)sect_data;
            
            if(sect_head->table_id != ATSC_PSIP_MGT_TID)
            {
                sect_len -= 3;
                sect_len -= MAKE_SHORT_HL(sect_head->section_length);

                sect_data += 3;
                sect_data += MAKE_SHORT_HL(sect_head->section_length);

                continue;
            }

            if(NULL == sect_info)
            {
                sect_info = new_mgt_section_info();
                if(sect_info == NULL)
                {
                    AM_TRACE("[%s] error out of memory !!\n", __FUNC__);
                    break;
                }
                else
                {
                    add_mgt_section_info(&info, sect_info);
                }
            }
            
            sect_info->version_number = sect_head->version_number;
	     sect_info->tables_defined = MAKE_SHORT_HL(sect_head->tables_defined);

            ptr = &sect_data[MGT_SECTION_HEADER_LEN];
            tables_num = sect_info->tables_defined;
            while(tables_num > 0)
            {
				table_info = (mgt_table_info_t *)ptr;
				tmp_table_info = (com_table_info_t*)AMMem_malloc(sizeof(com_table_info_t));
				if(tmp_table_info)
				{
					tmp_table_info->table_type = MAKE_SHORT_HL(table_info->table_type);
					tmp_table_info->table_type_pid = MAKE_SHORT_HL(table_info->table_type_pid);
					tmp_table_info->table_type_version = table_info->table_type_version;
					tmp_table_info->desc = NULL;
					tmp_table_info->p_next = NULL;
						   

					if ((tmp_table_info->table_type == 0) | (tmp_table_info->table_type == 1))
					{
					sect_info->is_cable = 0;
					}
					else if ((tmp_table_info->table_type == 2) | (tmp_table_info->table_type == 3))
					{
					sect_info->is_cable = 1;
					}
					else 
					{
					// do nothing
					}
					add_mgt_table_info(&sect_info->com_table_info, tmp_table_info);
				}
				else
				{
					AM_TRACE("[%s] error out of memory !!\n", __FUNC__);
					break;
				} 

				ptr += 11 + MAKE_SHORT_HL(table_info->table_type_desc_len);
				tables_num--;
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

void mgt_psip_clear_mgt_info(mgt_section_info_t *info)
{
	mgt_section_info_t *mgt_sect_info = NULL;
	mgt_section_info_t *tmp_mgt_sect_info = NULL;

	if(info)
	{
		tmp_mgt_sect_info = info->p_next;

		while(tmp_mgt_sect_info)
		{
			com_table_info_t *table_info = NULL;
			com_table_info_t *tmp_table_info = NULL;
			tmp_table_info = tmp_mgt_sect_info->com_table_info;
			while(tmp_table_info)
			{
				if(tmp_table_info->desc)
				{
					atsc_DeleteDescriptors(tmp_table_info->desc);
					tmp_table_info->desc = NULL;
				}
				table_info = tmp_table_info->p_next;
				AMMem_free(tmp_table_info);
				tmp_table_info = table_info;
			}
			mgt_sect_info = tmp_mgt_sect_info->p_next;
			AMMem_free(tmp_mgt_sect_info);
			tmp_mgt_sect_info = mgt_sect_info;
		}
		if(info->com_table_info)
		{
			AMMem_free(info->com_table_info);
		}
		if(info->desc)
		{
			atsc_DeleteDescriptors(info->desc);
		}
		
		info->tables_defined = DVB_INVALID_ID;
		info->version_number = DVB_INVALID_VERSION;
		info->is_cable = 0;
		info->com_table_info = NULL;
		info->desc= NULL;
		info->p_next = NULL;
	}
	return ;
}

mgt_section_info_t *atsc_psip_new_mgt_info(void)
{
    return new_mgt_section_info();
}

void atsc_psip_free_mgt_info(mgt_section_info_t *info)
{
	com_table_info_t *table_info = NULL;
	com_table_info_t *tmp_table_info = NULL;
	tmp_table_info = info->com_table_info;
	
	while(tmp_table_info)
	{
		tmp_table_info->table_type = 0;
		tmp_table_info->table_type_pid = 0;
		if (tmp_table_info->desc)
		{
			atsc_DeleteDescriptors(tmp_table_info->desc);
			tmp_table_info->desc = NULL;
		}
		table_info = tmp_table_info->p_next;
		AMMem_free(tmp_table_info);
		tmp_table_info = table_info;
	}

	info->com_table_info = NULL;

	if(info->desc) //
	{
		atsc_DeleteDescriptors(info->desc);
		info->desc= NULL;
	}

	info->version_number = DVB_INVALID_VERSION;
	info->com_table_info = NULL;
	info->tables_defined = 0;
	info->is_cable = 0;
	AMMem_free(info);
}
void atsc_psip_dump_mgt_info(mgt_section_info_t *info)
{
#ifndef __ROM_
    mgt_section_info_t *tmp = info;
    com_table_info_t *table_info = NULL;
    INT8U i = 0;
    
    if(info)
    {
        AM_TRACE("\r\n============= MGT INFO =============\r\n");

        while(tmp)
        {
            AM_TRACE("version_number: 0x%02x\r\n", tmp->version_number);
            AM_TRACE("tables_defined: %d\r\n", tmp->tables_defined); 
	     AM_TRACE("is_cable: %d\r\n", tmp->is_cable); 	
	     table_info = tmp->com_table_info;
            while(table_info)
            {
                AM_TRACE("table type: 0x%04x table type pid: 0x%04x\r\n", table_info->table_type, table_info->table_type_pid);
				
		  table_info = table_info->p_next;
            }
            
            tmp = tmp->p_next;
        }

        AM_TRACE("\r\n=============== END ================\r\n");
    }
#endif
    return ;
}

