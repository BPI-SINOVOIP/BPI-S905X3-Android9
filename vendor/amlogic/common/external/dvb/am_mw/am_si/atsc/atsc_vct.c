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
#include "atsc/atsc_vct.h"

#define MAKE_SHORT_HL(exp)		            ((INT16U)((exp##_hi<<8)|exp##_lo))
#define MAKE_WORD_HML(exp)			((INT32U)(((exp##_hi)<<24)|((exp##_mh)<<16)|((exp##_ml)<<8)|exp##_lo))

#define MAJOR_CHANNEL_NUM(exp) ((INT16U)((exp##_hi<<6)|exp##_lo))
#define MINOR_CHANNEL_NUM(exp) ((INT16U)((exp##_hi<<8)|exp##_lo))

static vct_section_info_t *new_vct_section_info(void)
{
    vct_section_info_t *tmp = NULL;

    tmp = AMMem_malloc(sizeof(vct_section_info_t));
    if(tmp)
    {
        tmp->transport_stream_id = DVB_INVALID_ID;
        tmp->version_number = DVB_INVALID_VERSION;
        tmp->num_channels_in_section = 0;
        tmp->vct_chan_info = NULL;
        tmp->p_next = NULL;
    }

    return tmp;
}

static void add_vct_section_info(vct_section_info_t **head, vct_section_info_t *info)
{
    vct_section_info_t **tmp = head;
    vct_section_info_t **temp = head;

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

void add_vct_channel_info(vct_channel_info_t **head, vct_channel_info_t *info)
{
	vct_channel_info_t **tmp = head;
	vct_channel_info_t **node = head;

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

INT32S atsc_psip_parse_vct(INT8U* data, INT32U length, vct_section_info_t *info)
{
	INT8U *sect_data = data;
	INT32S sect_len = length;
	vct_section_info_t *sect_info = info;
	vct_section_header_t *sect_head = NULL;
	vct_sect_chan_info_t *vct_sect_chan = NULL;
	vct_channel_info_t *tmp_chan_info = NULL;
	INT8U num_channels_in_section = 0;
	INT8U *ptr = NULL;
	INT16S desc_len;
	INT8U *desc_ptr;
	int chan_name_len;

	if(data && length && info)
	{
		while(sect_len > 0)
		{
			sect_head = (vct_section_header_t*)sect_data;

			if(sect_head->table_id != ATSC_PSIP_TVCT_TID &&
			   sect_head->table_id != ATSC_PSIP_CVCT_TID)
			{
				sect_len -= 3;
				sect_len -= MAKE_SHORT_HL(sect_head->section_length);

				sect_data += 3;
				sect_data += MAKE_SHORT_HL(sect_head->section_length);

				continue;
			}

			if(NULL == sect_info)
			{
				sect_info = new_vct_section_info();
				if(sect_info == NULL)
				{
					AM_TRACE("[%s] error out of memory !!\n", __FUNC__);
					break;
				}
				else
				{
					add_vct_section_info(&info, sect_info);
				}
			}

			sect_info->transport_stream_id = MAKE_SHORT_HL(sect_head->transport_stream_id);
			sect_info->version_number = sect_head->version_number;

			num_channels_in_section = sect_head->num_channels_in_section;
			sect_info->num_channels_in_section= num_channels_in_section;
			ptr = sect_data+VCT_SECTION_HEADER_LEN;
			AM_TRACE("channels in vct %d", num_channels_in_section);
			while (num_channels_in_section)
			{
				vct_sect_chan = (vct_sect_chan_info_t *)ptr;
				tmp_chan_info= (vct_channel_info_t *)AMMem_malloc(sizeof(vct_channel_info_t));
				if (tmp_chan_info)
				{
					memset(tmp_chan_info, 0, sizeof(vct_channel_info_t));
					chan_name_len = sizeof(tmp_chan_info->short_name);
					if(atsc_convert_code_from_utf16_to_utf8((char*)ptr, 14, (char*)tmp_chan_info->short_name, 
                                          &chan_name_len) < 0 ) {
                                               memset(tmp_chan_info->short_name,0 ,chan_name_len);//reset shortname due to hit name  paser error
                                               strcpy((char*)tmp_chan_info->short_name,"No Name");
                                          }

					//short_channel_name_parse(ptr, tmp_chan_info->short_name);
					tmp_chan_info->major_channel_number = MAJOR_CHANNEL_NUM(vct_sect_chan->major_channel_number);
					tmp_chan_info->minor_channel_number = MINOR_CHANNEL_NUM(vct_sect_chan->minor_channel_number);
					AM_DEBUG(1, "The major is:%d, the minor is:%d.", tmp_chan_info->major_channel_number, tmp_chan_info->minor_channel_number);

					tmp_chan_info->program_number = MAKE_SHORT_HL(vct_sect_chan->program_number);
					tmp_chan_info->service_type = vct_sect_chan->service_type;
					tmp_chan_info->source_id = MAKE_SHORT_HL(vct_sect_chan->source_id);
					tmp_chan_info->carrier_frequency = MAKE_WORD_HML(vct_sect_chan->carrier_frequency);
					tmp_chan_info->modulation_mode = vct_sect_chan->modulation_mode;
					tmp_chan_info->access_controlled = vct_sect_chan->access_controlled;
					tmp_chan_info->hidden = vct_sect_chan->hidden;
					tmp_chan_info->hide_guide = vct_sect_chan->hide_guide;
					tmp_chan_info->path_select = vct_sect_chan->path_select;
					tmp_chan_info->out_of_band = vct_sect_chan->out_of_band;
					tmp_chan_info->channel_TSID = MAKE_SHORT_HL(vct_sect_chan->channel_TSID);
					tmp_chan_info->desc = NULL;
					tmp_chan_info->p_next = NULL;

					add_vct_channel_info(&sect_info->vct_chan_info, tmp_chan_info);
					
					desc_len = MAKE_SHORT_HL(vct_sect_chan->descriptors_length);
					desc_ptr = ptr + 32;
					while (desc_len > 0)
					{
						atsc_descriptor_t* p_descriptor = atsc_NewDescriptor(desc_ptr[0], desc_ptr[1], desc_ptr + 2);

						if(p_descriptor)
						{
							if(tmp_chan_info->desc == NULL)
							{
								tmp_chan_info->desc = p_descriptor;
							}
							else
							{
								atsc_descriptor_t* p_last_descriptor = tmp_chan_info->desc;
								while(p_last_descriptor->p_next != NULL)
									p_last_descriptor = p_last_descriptor->p_next;
								p_last_descriptor->p_next = p_descriptor;
							}
						}
						desc_len -= desc_ptr[1] + 2;
						desc_ptr += desc_ptr[1] + 2;
					}
				}
				else
				{
					AM_TRACE("[%s] error out of memory !!\n", __FUNC__);
					break;
				}         
				ptr += 32 + MAKE_SHORT_HL(vct_sect_chan->descriptors_length);
				num_channels_in_section--;
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

void atsc_psip_clear_vct_info(vct_section_info_t *info)
{
	vct_section_info_t *vct_sect_info = NULL;
	vct_section_info_t *tmp_vct_sect_info = NULL;

	if(info)
	{
		tmp_vct_sect_info = info->p_next;

		while(tmp_vct_sect_info)
		{
			vct_channel_info_t *chan_info = NULL;
			vct_channel_info_t *tmp_chan_info = NULL;
			tmp_chan_info = tmp_vct_sect_info->vct_chan_info;
			while(tmp_chan_info)
			{
				if(tmp_chan_info->desc)
				{
					atsc_DeleteDescriptors(tmp_chan_info->desc);
					tmp_chan_info->desc = NULL;
				}
				chan_info = tmp_chan_info->p_next;
				AMMem_free(tmp_chan_info);
				tmp_chan_info = chan_info;
			}
			vct_sect_info = tmp_vct_sect_info->p_next;
			AMMem_free(tmp_vct_sect_info);
			tmp_vct_sect_info = vct_sect_info;
		}
		if(info->vct_chan_info)
		{
			AMMem_free(info->vct_chan_info);
		}
		
		info->transport_stream_id = DVB_INVALID_ID;
		info->version_number = DVB_INVALID_VERSION;
		info->num_channels_in_section = 0;
		info->vct_chan_info = NULL;
		info->p_next = NULL;
	}
	return ;
}

vct_section_info_t *atsc_psip_new_vct_info(void)
{
    return new_vct_section_info();
}

void atsc_psip_free_vct_info(vct_section_info_t *info)
{
	vct_channel_info_t *vct_chan_info = NULL;
	vct_channel_info_t *tmp_vct_chan_info =NULL;
	tmp_vct_chan_info = info->vct_chan_info;
	while(tmp_vct_chan_info)
	{
		if (tmp_vct_chan_info->desc)
		{
			atsc_DeleteDescriptors(tmp_vct_chan_info->desc);
			tmp_vct_chan_info->desc = NULL;
		}
		vct_chan_info = tmp_vct_chan_info->p_next;
		AMMem_free(tmp_vct_chan_info);
		tmp_vct_chan_info = vct_chan_info;
	}
	AMMem_free(info);
}

void atsc_psip_dump_vct_info(vct_section_info_t *info)
{
#ifndef __ROM_
	vct_section_info_t *tmp = info;

	INT8U i = 0;

	if(info)
	{
		AM_TRACE("\r\n============= VCT INFO =============\r\n");

		while(tmp)
		{ 
			vct_channel_info_t *chan_info = tmp->vct_chan_info;
			AM_TRACE("transport_stream_id: 0x%04x\r\n", tmp->transport_stream_id);
			AM_TRACE("version_number: 0x%02x\r\n", tmp->version_number);
			AM_TRACE("num_channels_in_section: %d\r\n", tmp->num_channels_in_section); 

			while(chan_info)
			{
				
				AM_TRACE("    %d-%d %s program number:%d source id %d\n", 
					chan_info->major_channel_number,
					chan_info->minor_channel_number,
					chan_info->short_name,
					chan_info->program_number,
					chan_info->source_id);
				
				chan_info = chan_info->p_next;
			}

			tmp = tmp->p_next;
		}

		AM_TRACE("\r\n=============== END ================\r\n");
	}
#endif
    return ;
}

