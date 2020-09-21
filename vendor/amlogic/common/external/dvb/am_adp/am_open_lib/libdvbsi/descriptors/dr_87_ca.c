/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package. *
* Description:
*/
#ifdef _FORTIFY_SOURCE
#undef _FORTIFY_SOURCE
#endif
/*
dr_87_ca.c

Decode Content Advisory Descriptor.

*/
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#if defined(HAVE_INTTYPES_H)
#include <inttypes.h>
#elif defined(HAVE_STDINT_H)
#include <stdint.h>
#endif

#include "../dvbpsi.h"
#include "../dvbpsi_private.h"
#include "../descriptor.h"

#include "dr_87_ca.h"

/*****************************************************************************
 * dvbpsi_decode_atsc_content_advisory_dr
 *****************************************************************************/
dvbpsi_atsc_content_advisory_dr_t *dvbpsi_decode_atsc_content_advisory_dr(dvbpsi_descriptor_t *p_descriptor)
{
    dvbpsi_atsc_content_advisory_dr_t *p_decoded;
    uint8_t * buf = p_descriptor->p_data;
    uint8_t * end;
    int data_len = p_descriptor->i_length;
    int region = 0;

    /* Check the tag */
    if (!dvbpsi_CanDecodeAsDescriptor(p_descriptor, 0x87))
        return NULL;

    /* Don't decode twice */
    if (dvbpsi_IsDescriptorDecoded(p_descriptor))
        return p_descriptor->p_decoded;

    p_decoded = (dvbpsi_atsc_content_advisory_dr_t*)malloc(sizeof(dvbpsi_atsc_content_advisory_dr_t));
    if (!p_decoded)
        return NULL;

    end = buf + p_descriptor->i_length;

    p_descriptor->p_decoded = (void*)p_decoded;

    p_decoded->i_rating_region_count = 0x2f & buf[0];
    buf++;
    data_len--;

    while (region < p_decoded->i_rating_region_count) {
        dvbpsi_content_advisory_region_t * p_region = &p_decoded->rating_regions[region];

        //if ((buf+1) > end ||
        //    || ((buf+2+buf[1]*2) > end)
        //    || ((buf + 3 + buf[1]*2 + buf[2+buf[1]*2]) > end))
        //    break;
        if (data_len < 2)
            goto ERROR;
        p_region->i_rating_region = buf[0];
        p_region->i_rated_dimensions = buf[1];
        buf += 2;
        data_len -= 2;

        for (int i = 0; i< p_region->i_rated_dimensions; i++) {
            if (data_len < 2)
                goto ERROR;

            p_region->dimensions[i].i_rating_dimension_j = buf[0];
            p_region->dimensions[i].i_rating_value = buf[1] & 0xf;
            buf += 2;
            data_len -= 2;
        }
        if (data_len < 1)
            goto ERROR;
        p_region->i_rating_description_length = buf[0];
        buf++;

        if (data_len < p_region->i_rating_description_length)
            goto ERROR;

        memset(p_region->i_rating_description, 0, sizeof(p_region->i_rating_description));
        memcpy(p_region->i_rating_description, &buf[0], p_region->i_rating_description_length);
        buf += p_region->i_rating_description_length;

        region++;
    }

    p_decoded->i_rating_region_count = region;
    return p_decoded;
ERROR:
    free(p_decoded);
    p_descriptor->p_decoded = NULL;
    return NULL;
}
