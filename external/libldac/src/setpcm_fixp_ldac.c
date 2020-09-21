/*
 * Copyright (C) 2003 - 2016 Sony Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ldac.h"

/***************************************************************************************************
    Subfunction: Convert from 16bit Signed Integer PCM
***************************************************************************************************/
__inline static void byte_data_to_int_s16_ldac(
char *p_in,
INT32 *p_out,
int nsmpl)
{
    int i;
    short *p_s;

    p_s = (short *)p_in;
    for (i = 0; i < nsmpl; i++) {
        *p_out++ = lsft_ldac((INT32)*p_s++, LDAC_Q_SETPCM);
    }

    return;
}

/***************************************************************************************************
    Subfunction: Convert from 24bit Signed Integer PCM
***************************************************************************************************/
__inline static void byte_data_to_int_s24_ldac(
char *p_in,
INT32 *p_out,
int nsmpl)
{
    int i, val;
    char *p_c;

    p_c = (char *)p_in;
    for (i = 0; i < nsmpl; i++) {
#ifdef LDAC_HOST_ENDIAN_LITTLE
        val  = 0x000000ff & (*p_c++);
        val |= 0x0000ff00 & (*p_c++ << 8);
        val |= 0xffff0000 & (*p_c++ << 16);
#else /* LDAC_HOST_ENDIAN_LITTLE */
        val  = 0xffff0000 & (*p_c++ << 16);
        val |= 0x0000ff00 & (*p_c++ << 8);
        val |= 0x000000ff & (*p_c++);
#endif /* LDAC_HOST_ENDIAN_LITTLE */
        *p_out++ = (INT32)((val << 8) >> 1); /* Sign Extension */
    }

    return;
}

/***************************************************************************************************
    Subfunction: Convert from 32bit Signed Integer PCM
***************************************************************************************************/
__inline static void byte_data_to_int_s32_ldac(
char *p_in,
INT32 *p_out,
int nsmpl)
{
    int i;
    int *p_l;

    p_l = (int *)p_in;
    for (i = 0; i < nsmpl; i++) {
        *p_out++ = rsft_ldac((INT32)*p_l++, 16-LDAC_Q_SETPCM);
    }

    return;
}

/***************************************************************************************************
    Set Input PCM
***************************************************************************************************/
DECLFUNC void set_input_pcm_ldac(
SFINFO *p_sfinfo,
char *pp_pcm[],
LDAC_SMPL_FMT_T format,
int nlnn)
{
    int ich, isp;
    int nchs = p_sfinfo->cfg.ch;
    int nsmpl = npow2_ldac(nlnn);
    INT32 *p_time;

    if (format == LDAC_SMPL_FMT_S16) {
        for (ich = 0; ich < nchs; ich++) {
            p_time = p_sfinfo->ap_ac[ich]->p_acsub->a_time;
            for (isp = 0; isp < nsmpl; isp++) {
                p_time[isp] = p_time[nsmpl+isp];
            }
            byte_data_to_int_s16_ldac(pp_pcm[ich], p_time+nsmpl, nsmpl);
        }
    }
    else if (format == LDAC_SMPL_FMT_S24) {
        for (ich = 0; ich < nchs; ich++) {
            p_time = p_sfinfo->ap_ac[ich]->p_acsub->a_time;
            for (isp = 0; isp < nsmpl; isp++) {
                p_time[isp] = p_time[nsmpl+isp];
            }
            byte_data_to_int_s24_ldac(pp_pcm[ich], p_time+nsmpl, nsmpl);
        }
    }
    else if (format == LDAC_SMPL_FMT_S32) {
        for (ich = 0; ich < nchs; ich++) {
            p_time = p_sfinfo->ap_ac[ich]->p_acsub->a_time;
            for (isp = 0; isp < nsmpl; isp++) {
                p_time[isp] = p_time[nsmpl+isp];
            }
            byte_data_to_int_s32_ldac(pp_pcm[ich], p_time+nsmpl, nsmpl);
        }
    }

    return;
}


