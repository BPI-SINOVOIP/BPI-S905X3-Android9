#ifdef _FORTIFY_SOURCE
#undef _FORTIFY_SOURCE
#endif
/*****************************************************************************
 * descriptor.c: descriptors functions
 *----------------------------------------------------------------------------
 * (c)2001-2002 VideoLAN
 * $Id: descriptor.c 88 2004-02-24 14:31:18Z sam $
 *
 * Authors: Arnaud de Bossoreille de Ribou <bozo@via.ecp.fr>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 *----------------------------------------------------------------------------
 *
 *****************************************************************************/


#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(HAVE_INTTYPES_H)
#include <inttypes.h>
#elif defined(HAVE_STDINT_H)
#include <stdint.h>
#endif
#include <am_debug.h>
#include "dvbpsi.h"
#include "descriptor.h"

DVBpsi_Decode_Descriptor psi_decode_descriptor = NULL;

/*****************************************************************************
 * dvbpsi_IsDescriptor
 *****************************************************************************
 * Is this descriptor of type i_tag?
 *****************************************************************************/
static inline bool dvbpsi_IsDescriptor(dvbpsi_descriptor_t *p_descriptor, const uint8_t i_tag)
{
    return (p_descriptor->i_tag == i_tag);
}

/*****************************************************************************
 * IsDescriptorDecoded
 *****************************************************************************
 * Is this descriptor already decoded?
 *****************************************************************************/
bool dvbpsi_IsDescriptorDecoded(dvbpsi_descriptor_t *p_descriptor)
{
    return (p_descriptor->p_decoded != NULL);
}

/*****************************************************************************
 * dvbpsi_CanDescodeAsDescriptor
 *****************************************************************************
 * Can Decode this descriptor as?
 *****************************************************************************/
bool dvbpsi_CanDecodeAsDescriptor(dvbpsi_descriptor_t *p_descriptor, const uint8_t i_tag)
{
    if (!p_descriptor)
        return false;

    if (!dvbpsi_IsDescriptor(p_descriptor, i_tag))
        return false;

    return true;
}

/*****************************************************************************
 * dvbpsi_Set_DecodeDescriptor_Callback
 *****************************************************************************
 * set decode des callback
 *****************************************************************************/
int dvbpsi_Set_DecodeDescriptor_Callback(DVBpsi_Decode_Descriptor cb)
{
    int ret = 0;
    if (cb) {
        psi_decode_descriptor = cb;
        //AM_DEBUG(1,"set decode descriptor cb success\n");
    } else {
        AM_DEBUG(1,"set decode descriptor cb null error\n");
        ret = -1;
    }
    return ret;
}
/*****************************************************************************
 * dvbpsi_NewDescriptor
 *****************************************************************************
 * Creation of a new dvbpsi_descriptor_t structure.
 *****************************************************************************/
dvbpsi_descriptor_t* dvbpsi_NewDescriptor(uint8_t i_tag, uint8_t i_length,
        uint8_t* p_data)
{
    //AM_DEBUG(1, "malloc descriptor\n");
    dvbpsi_descriptor_t* p_descriptor
        = (dvbpsi_descriptor_t*)malloc(sizeof(dvbpsi_descriptor_t));
    if (!p_descriptor)
    {
        AM_DEBUG(1,"descriptor malloc failed\n");
        p_descriptor = NULL;
    }
    else
    {
        if (i_length == 0)
        {
            /* In some descriptors, this case is legacy */
            p_descriptor->p_data = NULL;
            p_descriptor->i_tag = i_tag;
            p_descriptor->i_length = i_length;
            p_descriptor->p_decoded = NULL;
            p_descriptor->p_next = NULL;
            return p_descriptor;
        }
        p_descriptor->p_data = (uint8_t*)malloc(i_length * sizeof(uint8_t));

        if (p_descriptor->p_data)
        {
            p_descriptor->i_tag = i_tag;
            p_descriptor->i_length = i_length;
            if (p_data)
                memcpy(p_descriptor->p_data, p_data, i_length);
            p_descriptor->p_decoded = NULL;
            p_descriptor->p_next = NULL;
        }
        else
        {
            AM_DEBUG(1, "descriptor pdata malloc failed\n");
            if (p_descriptor)
                free(p_descriptor);
            p_descriptor = NULL;
        }
    }
    return p_descriptor;
}

dvbpsi_descriptor_t* dvbpsi_NewDescriptorEx(uint8_t i_tag, uint8_t i_length,
        uint8_t* p_data, void *user)
{
    dvbpsi_descriptor_t* p_descriptor
        = dvbpsi_NewDescriptor(i_tag, i_length, p_data);

    if (p_descriptor && p_descriptor->p_data) {
        /*Decode it*/
        //AM_DEBUG(1,"About to decode descriptor\n");
        if (psi_decode_descriptor) {
            psi_decode_descriptor(p_descriptor, user);
        }
    }

    return p_descriptor;
}

/*****************************************************************************
 * dvbpsi_DeleteDescriptors
 *****************************************************************************
 * Destruction of a dvbpsi_descriptor_t structure.
 *****************************************************************************/
void dvbpsi_DeleteDescriptors(dvbpsi_descriptor_t* p_descriptor)
{
    while (p_descriptor != NULL)
    {
        dvbpsi_descriptor_t* p_next = p_descriptor->p_next;

        if (p_descriptor->p_data != NULL)
            free(p_descriptor->p_data);

        if (p_descriptor->p_decoded != NULL)
            free(p_descriptor->p_decoded);

        free(p_descriptor);
        p_descriptor = p_next;
    }
}

/*****************************************************************************
 * dvbpsi_AddDescriptor
 *****************************************************************************
 * Add 'p_descriptor' to the list of descriptors 'p_list'
 *****************************************************************************/
dvbpsi_descriptor_t *dvbpsi_AddDescriptor(dvbpsi_descriptor_t *p_list,
                                          dvbpsi_descriptor_t *p_descriptor)
{
    if (!p_descriptor)
        return p_list;

    if (p_list == NULL)
        p_list = p_descriptor;
    else
    {
        dvbpsi_descriptor_t *p_last = p_list;
        while (p_last->p_next != NULL)
                p_last = p_last->p_next;
        p_last->p_next = p_descriptor;
    }
    return p_list;
}

/*****************************************************************************
 * dvbpsi_DuplicateDecodedDescriptor
 *****************************************************************************
 * Generic function for duplicating a decoded descriptor structure.
 *****************************************************************************/
void *dvbpsi_DuplicateDecodedDescriptor(void *p_decoded, ssize_t i_size)
{
    if (!p_decoded)
        return NULL;

    void *p_duplicate = calloc(1, i_size);
    if (p_duplicate)
        memcpy(p_duplicate, p_decoded, i_size);
    return p_duplicate;
}
