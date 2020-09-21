/*
** FAAD2 - Freeware Advanced Audio (AAC) Decoder including SBR decoding
** Copyright (C) 2003-2005 M. Bakker, Nero AG, http://www.nero.com
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
**
** Any non-GPL usage of this software or parts of this software is strictly
** forbidden.
**
** The "appropriate copyright message" mentioned in section 2c of the GPLv2
** must read: "Code from FAAD2 is copyright (c) Nero AG, www.nero.com"
**
** Commercial non-GPL licensing of this software is possible.
** For more info contact Nero AG through Mpeg4AAClicense@nero.com.
**
** $Id: decoder.c,v 1.117 2009/02/05 00:51:03 menno Exp $
**/

#include "common.h"
#include "structs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <android/log.h>

#include "mp4.h"
#include "syntax.h"
#include "error.h"
#include "output.h"
#include "filtbank.h"
#include "drc.h"
#ifdef SBR_DEC
#include "sbr_dec.h"
#include "sbr_syntax.h"
#endif
#ifdef SSR_DEC
#include "ssr.h"
#include "ssr_fb.h"
#endif

#ifdef USE_HELIX_AAC_DECODER
#include "aaccommon.h"
#include "aacdec.h"
int AACDataSource = 1;
static HAACDecoder hAACDecoder;
static int adts_sample_rates[] = {96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 7350, 0, 0, 0};

static int FindAdtsSRIndex(int sr)
{
    int i;

    for (i = 0; i < 16; i++) {
        if (sr == adts_sample_rates[i]) {
            return i;
        }
    }
    return 16 - 1;
}
static void MakeAdtsHeader(NeAACDecFrameInfo *hInfo, unsigned char *adts_header, int channel)
{
    unsigned char *data;
    int old_format = 0;
    int profile = hInfo->object_type;
    if (profile == 5) { //sbr
        profile = 2;
    }
    profile = (profile - 1) & 0x3;
    int sr_index = ((hInfo->sbr == SBR_UPSAMPLED) || (hInfo->sbr == NO_SBR_UPSAMPLED)) ?
                   FindAdtsSRIndex(hInfo->samplerate / 2) : FindAdtsSRIndex(hInfo->samplerate);
    int skip = (old_format) ? 8 : 7;
    int framesize = skip + hInfo->bytesconsumed;

    if (hInfo->header_type == ADTS) {
        framesize -= skip;
    }

    // audio_codec_print("MakeAdtsHeader  profile %d ,ch %d,sr_index %d\n",profile,channel,sr_index);

    data = adts_header;
    memset(data, 0, 7 * sizeof(unsigned char));

    data[0] += 0xFF; /* 8b: syncword */

    data[1] += 0xF0; /* 4b: syncword */
    /* 1b: mpeg id = 0 */
    /* 2b: layer = 0 */
    data[1] += 1; /* 1b: protection absent */

    data[2] += ((profile << 6) & 0xC0); /* 2b: profile */
    data[2] += ((sr_index << 2) & 0x3C); /* 4b: sampling_frequency_index */
    /* 1b: private = 0 */
    data[2] += ((channel >> 2) & 0x1); /* 1b: channel_configuration */

    data[3] += ((channel << 6) & 0xC0); /* 2b: channel_configuration */
    /* 1b: original */
    /* 1b: home */
    /* 1b: copyright_id */
    /* 1b: copyright_id_start */
    data[3] += ((framesize >> 11) & 0x3); /* 2b: aac_frame_length */

    data[4] += ((framesize >> 3) & 0xFF); /* 8b: aac_frame_length */

    data[5] += ((framesize << 5) & 0xE0); /* 3b: aac_frame_length */
    data[5] += ((0x7FF >> 6) & 0x1F); /* 5b: adts_buffer_fullness */

    data[6] += ((0x7FF << 2) & 0x3F); /* 6b: adts_buffer_fullness */
    /* 2b: num_raw_data_blocks */

}
#endif

#ifdef ANALYSIS
uint16_t dbg_count;
#endif

#define faad_log_info audio_codec_print


#ifdef NEW_CODE_CHECK_LATM
#define LATM_LOG  audio_codec_print
static const int pi_sample_rates[16] = {
    96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050,
    16000, 12000, 11025, 8000,  7350,  0,     0,     0
};
#if 0
static int LOASSyncInfo(uint8_t p_header[LOAS_HEADER_SIZE], unsigned int *pi_header_size)
{
    *pi_header_size = 3;
    return ((p_header[1] & 0x1f) << 8) + p_header[2];
}
#endif
static int Mpeg4GAProgramConfigElement(bitfile *ld,mpeg4_cfg_t *p_cfg)
{
    /* TODO compute channels count ? */
    //int i_tag = faad_getbits(ld, 4);
    //if (i_tag != 0x05) {
    //    return -1;
    //}
    faad_getbits(ld, 2 + 4); // object type + sampling index
    int i_num_front = faad_getbits(ld, 4);
    int i_num_side = faad_getbits(ld, 4);
    int i_num_back = faad_getbits(ld, 4);
    int i_num_lfe = faad_getbits(ld, 2);
    int i_num_assoc_data = faad_getbits(ld, 3);
    int i_num_valid_cc = faad_getbits(ld, 4);
    int i,tmp;
    if (faad_getbits(ld, 1)) {
        faad_getbits(ld, 4);    // mono downmix
    }
    if (faad_getbits(ld, 1)) {
        faad_getbits(ld, 4);    // stereo downmix
    }
    if (faad_getbits(ld, 1)) {
        faad_getbits(ld, 2 + 1);    // matrix downmix + pseudo_surround
    }

    for (i = 0; i < i_num_front; i++) {
        tmp = faad_get1bit(ld DEBUGVAR(1, 26, "program_config_element(): front_element_is_cpe"));
        faad_getbits(ld, 4 DEBUGVAR(1, 27, "program_config_element(): front_element_tag_select"));

        if (tmp & 1) {
            p_cfg->i_channel += 2;
        } else {
            p_cfg->i_channel++;
        }
    }

    for (i = 0; i < i_num_side; i++) {
        tmp = faad_get1bit(ld DEBUGVAR(1, 28, "program_config_element(): side_element_is_cpe"));
        faad_getbits(ld, 4 DEBUGVAR(1, 29, "program_config_element(): side_element_tag_select"));

        if (tmp & 1) {
            p_cfg->i_channel += 2;
        } else {
            p_cfg->i_channel++;
        }
    }

    for (i = 0; i < i_num_back; i++) {
        tmp = faad_get1bit(ld DEBUGVAR(1, 30, "program_config_element(): back_element_is_cpe"));
        faad_getbits(ld, 4 DEBUGVAR(1, 31, "program_config_element(): back_element_tag_select"));

        if (tmp & 1) {
            p_cfg->i_channel += 2;
        } else {
            p_cfg->i_channel++;
        }
    }

    for (i = 0; i < i_num_lfe; i++) {
        tmp = (uint8_t)faad_getbits(ld, 4 DEBUGVAR(1, 32, "program_config_element(): lfe_element_tag_select"));
        p_cfg->i_channel++;
    }

    for (i = 0; i < i_num_assoc_data; i++)
        faad_getbits(ld, 4 DEBUGVAR(1, 33, "program_config_element(): assoc_data_element_tag_select"));

    for (i = 0; i < i_num_valid_cc; i++) {
        faad_get1bit(ld DEBUGVAR(1, 34, "program_config_element(): cc_element_is_ind_sw"));
        faad_getbits(ld, 4 DEBUGVAR(1, 35, "program_config_element(): valid_cc_element_tag_select"));
    }
    faad_byte_align(ld);
    int i_comment = faad_getbits(ld, 8);
    faad_getbits(ld, i_comment * 8);
    return 0;
}

static int Mpeg4GASpecificConfig(mpeg4_cfg_t *p_cfg, bitfile *ld)
{
    p_cfg->i_frame_length = faad_getbits(ld, 1) ? 960 : 1024;

    if (faad_getbits(ld, 1)) {  // depend on core coder
        faad_getbits(ld, 14);    // core coder delay
    }

    int i_extension_flag = faad_getbits(ld, 1);
    if (p_cfg->i_channel == 0) {
        Mpeg4GAProgramConfigElement(ld,p_cfg);
    }
    if (p_cfg->i_object_type == 6 || p_cfg->i_object_type == 20) {
        faad_getbits(ld, 3);    // layer
    }

    if (i_extension_flag) {
        if (p_cfg->i_object_type == 22) {
            faad_getbits(ld, 5 + 11);    // numOfSubFrame + layer length
        }
        if (p_cfg->i_object_type == 17 || p_cfg->i_object_type == 19 ||
            p_cfg->i_object_type == 20 || p_cfg->i_object_type == 23) {
            faad_getbits(ld, 1 + 1 + 1);    // ER data : section scale spectral */
        }
        if (faad_getbits(ld, 1)) {  // extension 3
            LATM_LOG("Mpeg4GASpecificConfig: error 1\n");
        }
    }
    return 0;
}

static int Mpeg4ReadAudioObjectType(bitfile *ld)
{
    int i_type = faad_getbits(ld, 5);
    if (i_type == 31) {
        i_type = 32 + faad_getbits(ld, 6);
    }
    return i_type;
}

static int Mpeg4ReadAudioSamplerate(bitfile *ld)
{
    int i_index = faad_getbits(ld, 4);
    if (i_index != 0x0f) {
        return pi_sample_rates[i_index];
    }
    return faad_getbits(ld, 24);
}

static int Mpeg4ReadAudioSpecificInfo(mpeg4_cfg_t *p_cfg, int *pi_extra, uint8_t *p_extra, bitfile *ld, int i_max_size)
{
#if 0
    static const char *ppsz_otype[] = {
        "NULL",
        "AAC Main", "AAC LC", "AAC SSR", "AAC LTP", "SBR", "AAC Scalable",
        "TwinVQ",
        "CELP", "HVXC",
        "Reserved", "Reserved",
        "TTSI",
        "Main Synthetic", "Wavetables Synthesis", "General MIDI",
        "Algorithmic Synthesis and Audio FX",
        "ER AAC LC",
        "Reserved",
        "ER AAC LTP", "ER AAC Scalable", "ER TwinVQ", "ER BSAC", "ER AAC LD",
        "ER CELP", "ER HVXC", "ER HILN", "ER Parametric",
        "SSC",
        "PS", "Reserved", "Escape",
        "Layer 1", "Layer 2", "Layer 3",
        "DST",
    };
#endif
    const int i_pos_start = faad_get_processed_bits(ld);
    bitfile s_sav = *ld;
    int i_bits;
    int i;

    memset(p_cfg, 0, sizeof(*p_cfg));
    *pi_extra = 0;

    p_cfg->i_object_type = Mpeg4ReadAudioObjectType(ld);
    p_cfg->i_samplerate = Mpeg4ReadAudioSamplerate(ld);

    p_cfg->i_channel = faad_getbits(ld, 4);
    if (p_cfg->i_channel == 7) {
        p_cfg->i_channel = 8;    // 7.1
    } else if (p_cfg->i_channel >= 8) {
        p_cfg->i_channel = -1;
    }

    p_cfg->i_sbr = -1;
    p_cfg->i_ps  = -1;
    p_cfg->extension.i_object_type = 0;
    p_cfg->extension.i_samplerate = 0;
    if (p_cfg->i_object_type == 5 || (p_cfg->i_object_type == 29/*&&(faad_showbits(ld, 3) & 0x03 && !(faad_showbits(ld, 9) & 0x3F))*/)) {
        p_cfg->i_sbr = 1;
        if (p_cfg->i_object_type == 29) {
            p_cfg->i_ps = 1;
        }
        p_cfg->extension.i_object_type = 5;
        p_cfg->extension.i_samplerate = Mpeg4ReadAudioSamplerate(ld);

        p_cfg->i_object_type = Mpeg4ReadAudioObjectType(ld);
    }

    switch (p_cfg->i_object_type) {
    case 1:
    case 2:
    case 3:
    case 4:
    case 6:
    case 7:
    case 17:
    case 19:
    case 20:
    case 21:
    case 22:
    case 23:
        Mpeg4GASpecificConfig(p_cfg, ld);
        break;
    case 8:
        // CelpSpecificConfig();
        break;
    case 9:
        // HvxcSpecificConfig();
        break;
    case 12:
        // TTSSSpecificConfig();
        break;
    case 13:
    case 14:
    case 15:
    case 16:
        // StructuredAudioSpecificConfig();
        break;
    case 24:
        // ERCelpSpecificConfig();
        break;
    case 25:
        // ERHvxcSpecificConfig();
        break;
    case 26:
    case 27:
        // ParametricSpecificConfig();
        break;
    case 28:
        // SSCSpecificConfig();
        break;
    case 32:
    case 33:
    case 34:
        // MPEG_1_2_SpecificConfig();
        break;
    case 35:
        // DSTSpecificConfig();
        break;
    case 36:
        // ALSSpecificConfig();
        break;
    default:
        // error
        break;
    }
    switch (p_cfg->i_object_type) {
    case 17:
    case 19:
    case 20:
    case 21:
    case 22:
    case 23:
    case 24:
    case 25:
    case 26:
    case 27: {
        int epConfig = faad_getbits(ld, 2);
        if (epConfig == 2 || epConfig == 3)
            //ErrorProtectionSpecificConfig();
            if (epConfig == 3)
                if (faad_getbits(ld, 1)) {
                    // TODO : directMapping
                }
        break;
    }
    default:
        break;
    }

    if (p_cfg->extension.i_object_type != 5 && i_max_size > 0 && i_max_size - (faad_get_processed_bits(ld) - i_pos_start) >= 16 &&
        faad_getbits(ld, 11) == 0x2b7) {
        p_cfg->extension.i_object_type = Mpeg4ReadAudioObjectType(ld);
        if (p_cfg->extension.i_object_type == 5) {
            p_cfg->i_sbr  = faad_getbits(ld, 1);
            if (p_cfg->i_sbr == 1) {
                p_cfg->extension.i_samplerate = Mpeg4ReadAudioSamplerate(ld);
                if (i_max_size > 0 && i_max_size - (faad_get_processed_bits(ld) - i_pos_start) >= 12 && faad_getbits(ld, 11) == 0x548) {
                    p_cfg->i_ps = faad_getbits(ld, 1);
                }
            }
        }
    }

    //fprintf(stderr, "Mpeg4ReadAudioSpecificInfo: t=%s(%d)f=%d c=%d sbr=%d\n",
    //         ppsz_otype[p_cfg->i_object_type], p_cfg->i_object_type, p_cfg->i_samplerate, p_cfg->i_channel, p_cfg->i_sbr);

    i_bits = faad_get_processed_bits(ld) - i_pos_start;

    *pi_extra = min((i_bits + 7) / 8, LATM_MAX_EXTRA_SIZE);
    for (i = 0; i < *pi_extra; i++) {
        const int i_read = min(8, i_bits - 8 * i);
        p_extra[i] = faad_getbits(&s_sav, i_read) << (8 - i_read);
    }
    return i_bits;
}

static int LatmGetValue(bitfile *ld)
{
    int i_bytes = faad_getbits(ld, 2);
    int v = 0;
    int i;
    for (i = 0; i < i_bytes; i++) {
        v = (v << 8) + faad_getbits(ld, 8);
    }

    return v;
}

static int LatmReadStreamMuxConfiguration(latm_mux_t *m, bitfile *ld)
{
    int i_mux_version;
    int i_mux_versionA;

    i_mux_version = faad_getbits(ld, 1);
    i_mux_versionA = 0;
    if (i_mux_version) {
        i_mux_versionA = faad_getbits(ld, 1);
    }

    if (i_mux_versionA != 0) { /* support only A=0 */
        return -1;
    }

    memset(m, 0, sizeof(*m));

    if (i_mux_versionA == 0)
        if (i_mux_version == 1) {
            LatmGetValue(ld);    /* taraBufferFullness */
        }

    m->b_same_time_framing = faad_getbits(ld, 1);
    m->i_sub_frames = 1 + faad_getbits(ld, 6);
    m->i_programs = 1 + faad_getbits(ld, 4);
    if (m->i_programs > 1) {
        return -1;
    }
    int i_program;
    for (i_program = 0; i_program < m->i_programs; i_program++) {
        m->pi_layers[i_program] = 1 + faad_getbits(ld, 3);
        if (m->pi_layers[0] > 1) {
            return -1;
        }
        int i_layer;
        for (i_layer = 0; i_layer < m->pi_layers[i_program]; i_layer++) {
            latm_stream_t *st = &m->stream[m->i_streams];
            unsigned char b_previous_cfg;

            m->pi_stream[i_program][i_layer] = m->i_streams;
            st->i_program = i_program;
            st->i_layer = i_layer;

            b_previous_cfg = 0;
            if (i_program != 0 || i_layer != 0) {
                b_previous_cfg = faad_getbits(ld, 1);
            }

            if (b_previous_cfg) {
                if (m->i_streams <= 0) {
                    LATM_LOG("assert failed \n");
                    while (1) {
                        ;
                    }
                }
                st->cfg = m->stream[m->i_streams - 1].cfg;
            } else {
                int i_cfg_size = 0;
                if (i_mux_version == 1) {
                    i_cfg_size = LatmGetValue(ld);
                }
                i_cfg_size -= Mpeg4ReadAudioSpecificInfo(&st->cfg, &st->i_extra, st->extra, ld, i_cfg_size);
                if (i_cfg_size > 0) {
                    faad_flushbits(ld, i_cfg_size);
                }
            }

            st->i_frame_length_type = faad_getbits(ld, 3);
            switch (st->i_frame_length_type) {
            case 0: {
                faad_flushbits(ld, 8); /* latmBufferFullnes */
                if (!m->b_same_time_framing)
                    if (st->cfg.i_object_type == 6 || st->cfg.i_object_type == 20 ||
                        st->cfg.i_object_type == 8 || st->cfg.i_object_type == 24) {
                        faad_flushbits(ld, 6);    /* eFrameOffset */
                    }
                break;
            }
            case 1:
                st->i_frame_length = faad_getbits(ld, 9);
                break;
            case 3:
            case 4:
            case 5:
                st->i_frame_length_index = faad_getbits(ld, 6); // celp
                break;
            case 6:
            case 7:
                st->i_frame_length_index = faad_getbits(ld, 1); // hvxc
            default:
                break;
            }
            /* Next stream */
            m->i_streams++;
        }
    }

    /* other data */
    if (faad_getbits(ld, 1)) {
        if (i_mux_version == 1) {
            m->i_other_data = LatmGetValue(ld);
        } else {
            int b_continue;
            do {
                b_continue = faad_getbits(ld, 1);
                m->i_other_data = (m->i_other_data << 8) + faad_getbits(ld, 8);
            } while (b_continue);
        }
    }

    /* crc */
    m->i_crc = -1;
    if (faad_getbits(ld, 1)) {
        m->i_crc = faad_getbits(ld, 8);
    }

    return 0;
}
static int LOASParse(uint8_t *p_buffer, int i_buffer, decoder_sys_t *p_sys)
{
    bitfile ld;// = {0};
    int i_accumulated = 0;
    const latm_stream_t *st;
    faad_initbits(&ld, p_buffer, i_buffer);
    int ret = 0;
    //bs_init(&s, p_buffer, i_buffer);

    /* Read the stream mux configuration if present */
    if (!faad_getbits(&ld, 1) && !(ret = LatmReadStreamMuxConfiguration(&p_sys->latm, &ld)) &&
        p_sys->latm.i_streams > 0) {
        st = &p_sys->latm.stream[0];

        p_sys->i_channels = st->cfg.i_channel;
        p_sys->i_rate = st->cfg.i_samplerate;
        p_sys->i_frame_length = st->cfg.i_frame_length;
        //  LATM_LOG("ch %d, rate %d,frame len %d \n",p_sys->i_channels,p_sys->i_rate,p_sys->i_frame_length);
        /* FIXME And if it changes ? */
        if (p_sys->i_channels && p_sys->i_rate && p_sys->i_frame_length > 0) {
#if 0
            if (!p_dec->fmt_out.i_extra && st->i_extra > 0) {
                p_dec->fmt_out.i_extra = st->i_extra;
                p_dec->fmt_out.p_extra = malloc(st->i_extra);
                if (!p_dec->fmt_out.p_extra) {
                    p_dec->fmt_out.i_extra = 0;
                    return 0;
                }
                memcpy(p_dec->fmt_out.p_extra, st->extra, st->i_extra);
            }
#endif
            p_sys->b_latm_cfg = 1;
        }
    }

    /* Wait for the configuration */
    if (!p_sys->b_latm_cfg || ret < 0) {
        return 0;
    }

    /* FIXME do we need to split the subframe into independent packet ? */
    if (p_sys->latm.i_sub_frames > 1) {
        printf("latm sub frames not yet supported, please send a sample");
    }
    int i_sub;
    for (i_sub = 0; i_sub < p_sys->latm.i_sub_frames; i_sub++) {
        int pi_payload[LATM_MAX_PROGRAM][LATM_MAX_LAYER];
        if (p_sys->latm.b_same_time_framing) {
            /* Payload length */
            int i_program, i_layer;
            for (i_program = 0; i_program < p_sys->latm.i_programs; i_program++) {
                for (i_layer = 0; i_layer < p_sys->latm.pi_layers[i_program]; i_layer++) {
                    latm_stream_t *st = &p_sys->latm.stream[p_sys->latm.pi_stream[i_program][i_layer]];
                    if (st->i_frame_length_type == 0) {
                        int i_payload = 0;
                        for (;;) {
                            int i_tmp = faad_getbits(&ld, 8);
                            i_payload += i_tmp;
                            if (i_tmp != 255) {
                                break;
                            }
                        }
                        pi_payload[i_program][i_layer] = i_payload;
                    } else if (st->i_frame_length_type == 1) {
                        pi_payload[i_program][i_layer] = st->i_frame_length / 8; /* XXX not correct */
                    } else if ((st->i_frame_length_type == 3) ||
                               (st->i_frame_length_type == 5) ||
                               (st->i_frame_length_type == 7)) {
                        faad_getbits(&ld, 2); // muxSlotLengthCoded
                        pi_payload[i_program][i_layer] = 0; /* TODO */
                    } else {
                        pi_payload[i_program][i_layer] = 0; /* TODO */
                    }
                }
            }

            /* Payload Data */
            //      int i_program,i_layer;
            for (i_program = 0; i_program < p_sys->latm.i_programs; i_program++) {
                for (i_layer = 0; i_layer < p_sys->latm.pi_layers[i_program]; i_layer++) {
                    /* XXX we only extract 1 stream */
                    if (i_program != 0 || i_layer != 0) {
                        break;
                    }

                    if (pi_payload[i_program][i_layer] <= 0) {
                        continue;
                    }

                    /* FIXME that's slow (and a bit ugly to write in place) */
                    int i;
                    for (i = 0; i < pi_payload[i_program][i_layer]; i++) {
                        if (i_accumulated >= i_buffer) {
                            return 0;
                        }
                        p_buffer[i_accumulated++] = faad_getbits(&ld, 8);
                    }
                }
            }
        } else {
            const int i_chunks = faad_getbits(&ld, 4);
            int pi_program[16];
            int pi_layer[16];

            //     printf( "latm without same time frameing not yet supported, please send a sample");
            int i_chunk;
            for (i_chunk = 0; i_chunk < i_chunks; i_chunk++) {
                const int streamIndex = faad_getbits(&ld, 4);
                latm_stream_t *st = &p_sys->latm.stream[streamIndex];
                const int i_program = st->i_program;
                const int i_layer = st->i_layer;

                pi_program[i_chunk] = i_program;
                pi_layer[i_chunk] = i_layer;

                if (st->i_frame_length_type == 0) {
                    int i_payload = 0;
                    for (;;) {
                        int i_tmp = faad_getbits(&ld, 8);
                        i_payload += i_tmp;
                        if (i_tmp != 255) {
                            break;
                        }
                    }
                    pi_payload[i_program][i_layer] = i_payload;
                    faad_getbits(&ld, 1); // auEndFlag
                } else if (st->i_frame_length_type == 1) {
                    pi_payload[i_program][i_layer] = st->i_frame_length / 8; /* XXX not correct */
                } else if ((st->i_frame_length_type == 3) ||
                           (st->i_frame_length_type == 5) ||
                           (st->i_frame_length_type == 7)) {
                    faad_getbits(&ld, 2); // muxSlotLengthCoded
                }
            }
            //  int i_chunk;
            for (i_chunk = 0; i_chunk < i_chunks; i_chunk++) {
                //const int i_program = pi_program[i_chunk];
                //const int i_layer = pi_layer[i_chunk];

                /* TODO ? Payload */
            }
        }
    }

#if 0
    if (p_sys->latm.i_other_data > 0) {
        ;    // TODO
    }
#endif
    faad_byte_align(&ld);

    return i_accumulated;
}


#endif

/* static function declarations */
static void* aac_frame_decode(NeAACDecStruct *hDecoder,
                              NeAACDecFrameInfo *hInfo,
                              unsigned char *buffer,
                              unsigned long buffer_size,
                              void **sample_buffer2,
                              unsigned long sample_buffer_size);
static void create_channel_config(NeAACDecStruct *hDecoder,
                                  NeAACDecFrameInfo *hInfo);


char* NEAACDECAPI NeAACDecGetErrorMessage(unsigned char errcode)
{
    if (errcode >= NUM_ERROR_MESSAGES) {
        return NULL;
    }
    return err_msg[errcode];
}

unsigned long NEAACDECAPI NeAACDecGetCapabilities(void)
{
    uint32_t cap = 0;

    /* can't do without it */
    cap += LC_DEC_CAP;

#ifdef MAIN_DEC
    cap += MAIN_DEC_CAP;
#endif
#ifdef LTP_DEC
    cap += LTP_DEC_CAP;
#endif
#ifdef LD_DEC
    cap += LD_DEC_CAP;
#endif
#ifdef ERROR_RESILIENCE
    cap += ERROR_RESILIENCE_CAP;
#endif
#ifdef FIXED_POINT
    cap += FIXED_POINT_CAP;
#endif

    return cap;
}

const unsigned char mes[] = { 0x67, 0x20, 0x61, 0x20, 0x20, 0x20, 0x6f, 0x20, 0x72, 0x20, 0x65, 0x20, 0x6e, 0x20, 0x20, 0x20, 0x74, 0x20, 0x68, 0x20, 0x67, 0x20, 0x69, 0x20, 0x72, 0x20, 0x79, 0x20, 0x70, 0x20, 0x6f, 0x20, 0x63 };
NeAACDecHandle NEAACDECAPI NeAACDecOpen(void)
{
    uint8_t i;
    NeAACDecStruct *hDecoder = NULL;

    if ((hDecoder = (NeAACDecStruct*)faad_malloc(sizeof(NeAACDecStruct))) == NULL) {
        return NULL;
    }

    memset(hDecoder, 0, sizeof(NeAACDecStruct));

    hDecoder->cmes = mes;
    hDecoder->config.outputFormat  = FAAD_FMT_16BIT;
    hDecoder->config.defObjectType = MAIN;
    hDecoder->config.defSampleRate = 44100; /* Default: 44.1kHz */
    hDecoder->config.downMatrix = 0x01;
    hDecoder->adts_header_present = 0;
    hDecoder->adif_header_present = 0;
    hDecoder->latm_header_present = 0;
#ifdef ERROR_RESILIENCE
    hDecoder->aacSectionDataResilienceFlag = 0;
    hDecoder->aacScalefactorDataResilienceFlag = 0;
    hDecoder->aacSpectralDataResilienceFlag = 0;
#endif
    hDecoder->frameLength = 1024;

    hDecoder->frame = 0;
    hDecoder->sample_buffer = NULL;

    hDecoder->__r1 = 1;
    hDecoder->__r2 = 1;

    for (i = 0; i < MAX_CHANNELS; i++) {
        hDecoder->window_shape_prev[i] = 0;
        hDecoder->time_out[i] = NULL;
        hDecoder->fb_intermed[i] = NULL;
#ifdef SSR_DEC
        hDecoder->ssr_overlap[i] = NULL;
        hDecoder->prev_fmd[i] = NULL;
#endif
#ifdef MAIN_DEC
        hDecoder->pred_stat[i] = NULL;
#endif
#ifdef LTP_DEC
        hDecoder->ltp_lag[i] = 0;
        hDecoder->lt_pred_stat[i] = NULL;
#endif
    }

#ifdef SBR_DEC
    for (i = 0; i < MAX_SYNTAX_ELEMENTS; i++) {
        hDecoder->sbr[i] = NULL;
    }
#endif

    hDecoder->drc = drc_init(REAL_CONST(1.0), REAL_CONST(1.0));
    hDecoder->last_ch_configure = -1;
    hDecoder->last_sf_index = -1;
    return hDecoder;
}

NeAACDecConfigurationPtr NEAACDECAPI NeAACDecGetCurrentConfiguration(NeAACDecHandle hpDecoder)
{
    NeAACDecStruct* hDecoder = (NeAACDecStruct*)hpDecoder;
    if (hDecoder) {
        NeAACDecConfigurationPtr config = &(hDecoder->config);

        return config;
    }

    return NULL;
}

unsigned char NEAACDECAPI NeAACDecSetConfiguration(NeAACDecHandle hpDecoder,
        NeAACDecConfigurationPtr config)
{
    NeAACDecStruct* hDecoder = (NeAACDecStruct*)hpDecoder;
    if (hDecoder && config) {
        /* check if we can decode this object type */
        if (can_decode_ot(config->defObjectType) < 0) {
            return 0;
        }
        hDecoder->config.defObjectType = config->defObjectType;

        /* samplerate: anything but 0 should be possible */
        if (config->defSampleRate == 0) {
            return 0;
        }
        hDecoder->config.defSampleRate = config->defSampleRate;

        /* check output format */
#ifdef FIXED_POINT
        if ((config->outputFormat < 1) || (config->outputFormat > 4)) {
            return 0;
        }
#else
        if ((config->outputFormat < 1) || (config->outputFormat > 5)) {
            return 0;
        }
#endif
        hDecoder->config.outputFormat = config->outputFormat;

        if (config->downMatrix > 1) {
            return 0;
        }
        hDecoder->config.downMatrix = config->downMatrix;

        /* OK */
        return 1;
    }

    return 0;
}

#if 0
static int latmCheck(latm_header *latm, bitfile *ld)
{
    uint32_t good = 0, bad = 0, bits, m;

    while (ld->bytes_left) {
        bits = faad_latm_frame(latm, ld);
        if (bits == -1U) {
            bad++;
        } else {
            good++;
            while (bits > 0) {
                m = min(bits, 8);
                faad_getbits(ld, m DEBUGVAR(print, var, dbg));
                bits -= m;
            }
        }
    }

    return (good > 0);
}
#endif
#define SKIP_LATM_BYTE  16*4*2
#if 0
static int latm_check_internal(unsigned char *buffer, unsigned buffer_size, unsigned *byte_cost)
{
    latm_header l;// = {0};
    int is_latm = 0;
    bitfile ld;
    int byte_consumed = 0;
    int byte_left = buffer_size;
retry:
    memset(&l, 0, sizeof(latm_header));
    faad_initbits(&ld, buffer + byte_consumed, buffer_size - byte_consumed);
    is_latm = latmCheck(&l, &ld);
    if (is_latm && l.ASCbits > 0) {
        is_latm = 1;
    } else {
        is_latm = 0;
        byte_consumed += SKIP_LATM_BYTE;
        byte_left -= SKIP_LATM_BYTE;

    }
    if (is_latm == 0 && byte_left > 400) {
        goto retry;
    }
//exit:
    *byte_cost =    byte_consumed;
    return  is_latm;
}
#endif
long NEAACDECAPI NeAACDecInit(NeAACDecHandle hpDecoder,
                              unsigned char *buffer,
                              unsigned long buffer_size,
                              unsigned long *samplerate,
                              unsigned char *channels,
                              int is_latm_external,
                              int *skipbytes)
{
    uint32_t bits = 0;
    bitfile ld;
    adif_header adif;
    adts_header adts;
    NeAACDecStruct* hDecoder = (NeAACDecStruct*)hpDecoder;
    unsigned char* temp_bufer = hDecoder->temp_bufer;
    int temp_size = 0;
    faad_log_info("enter NeAACDecInit \r\n");
#ifdef NEW_CODE_CHECK_LATM
    int i_frame_size;
    if (buffer_size > TMP_BUF_SIZE) {
        LATM_LOG("init input buffer size too big %lu, buffer size %d \n", buffer_size,TMP_BUF_SIZE);
        //buffer_size = sizeof(temp_bufer);
        buffer_size =  TMP_BUF_SIZE ;
    }
    if (buffer_size > 0) {
        memcpy(temp_bufer, buffer, buffer_size);
        temp_size = buffer_size;
        buffer  = temp_bufer;
    }
    unsigned char *pbuffer = buffer;
    int  pbuffer_size = buffer_size;
    decoder_sys_t *p_sys =  &hDecoder->dec_sys;
    latm_mux_t *m = &p_sys->latm;
    latm_stream_t *st = NULL;
#endif

    if ((hDecoder == NULL) || (samplerate == NULL) || (channels == NULL)) {
        return -1;
    }
    //memset(latm_payload,0,sizeof(latm_payload));
    hDecoder->sf_index = get_sr_index(hDecoder->config.defSampleRate);
    hDecoder->object_type = hDecoder->config.defObjectType;
    *samplerate = get_sample_rate(hDecoder->sf_index);
    *channels = 1;
    *samplerate = 0;
    *channels = 0;
    //int latm_audio = 0;
    //unsigned byte_cost = 0;
    if (buffer != NULL) {
        latm_header *l = &hDecoder->latm_config;
        faad_initbits(&ld, buffer, buffer_size);
#ifdef NEW_CODE_CHECK_LATM
        memset(&hDecoder->dec_sys, 0, sizeof(decoder_sys_t));
NEXT_CHECK:
        while (pbuffer_size >= 2) {
            if (pbuffer[0] == 0x56 && (pbuffer[1] & 0xe0) == 0xe0) { //LOAS sync word detected
                //  LATM_LOG("find LOAS sync word pos %d\n",buffer_size-pbuffer_size);
                break;
            }
            pbuffer++;
            pbuffer_size--;
        }
        if (pbuffer_size < LOAS_HEADER_SIZE) {
            LATM_LOG("check the loas frame failed\n");
            *skipbytes = buffer_size-pbuffer_size;
            goto exit_check;
        }
        /* Check if frame is valid and get frame info */
        i_frame_size = ((pbuffer[1] & 0x1f) << 8) + pbuffer[2];
        if (i_frame_size <= 0 || i_frame_size > 6 * 768) {
            LATM_LOG("i_frame_size/%d  error\n",i_frame_size);
            pbuffer++;
            pbuffer_size--;
            goto NEXT_CHECK;
        }
        if (pbuffer_size < (LOAS_HEADER_SIZE + i_frame_size)) {
            LATM_LOG("[%s %d]buffer size  %d small then frame size %d,\n", __FUNCTION__,__LINE__,pbuffer_size, i_frame_size+LOAS_HEADER_SIZE);
            *skipbytes = buffer_size-pbuffer_size;
            return -1;
        }
#if 1
        if (pbuffer[LOAS_HEADER_SIZE + i_frame_size] != 0x56 || (pbuffer[LOAS_HEADER_SIZE + i_frame_size + 1] & 0xe0) != 0xe0) { // next frame LOAS sync header detected
            LATM_LOG("emulated sync word no (sync on following frame) \n");
            pbuffer++;
            pbuffer_size--;
            goto NEXT_CHECK;
        }
#endif
        pbuffer += LOAS_HEADER_SIZE; //skip header
        pbuffer_size = pbuffer_size - LOAS_HEADER_SIZE;
        //parse the playload of one real LOAS aac frame
        i_frame_size = LOASParse(pbuffer, i_frame_size, p_sys);
        if (i_frame_size <= 0) {
            LATM_LOG("[%s %d]invalid i_frame_size/%d ,go on next check!...\n", __FUNCTION__, __LINE__, i_frame_size);
            goto NEXT_CHECK;
        } else {
            LATM_LOG("latm detected\n");
            hDecoder->latm_header_present = 1;
        }
        //assue latm detected. start init code
exit_check:
        if (m->i_streams > 0) {
            st = &m->stream[m->i_streams - 1];
        }
        memset(l, 0, sizeof(latm_header));
        if ((st && st->i_extra) || is_latm_external) {
            int32_t x;

            hDecoder->latm_header_present = 1;
            if (st && st->i_extra) {
                x = NeAACDecInit2(hDecoder, st->extra, st->i_extra, samplerate, channels);
            } else {
                x = -1;
            }
            if (x != 0) {
                hDecoder->latm_header_present = 0;
            }
#ifdef USE_HELIX_AAC_DECODER
            else {
                hAACDecoder = AACInitDecoder();
                if (!hAACDecoder) {
                    faad_log_info("fatal error,helix aac decoder init failed\n");
                    return -1;
                } else {
                    AACDecInfo *aacDecInfo = (AACDecInfo *)hAACDecoder;
                    if (aacDecInfo) {
                        aacDecInfo->format = AAC_FF_ADTS;
                        aacDecInfo->nChans = *channels;
                    } else {
                        LATM_LOG("aacDecInfo NULL\n");
                        return NULL;
                    }
                }
            }
#endif
            LATM_LOG("latm init ret %d \n", x);
            return x;
        } else
#else
        int is_latm;
        memset(l, 0, sizeof(latm_header));
        is_latm = latmCheck(l, &ld);
        l->inited = 0;
        l->frameLength = 0;
        faad_rewindbits(&ld);
        if (is_latm && l->ASCbits > 0) {
            int32_t x;
            hDecoder->latm_header_present = 1;
            x = NeAACDecInit2(hDecoder, l->ASC, (l->ASCbits + 7) / 8, samplerate, channels);
            if (x != 0) {
                hDecoder->latm_header_present = 0;
            }
            return x;
        } else
#endif
            /* Check if an ADIF header is present */
            if ((buffer[0] == 'A') && (buffer[1] == 'D') &&
                (buffer[2] == 'I') && (buffer[3] == 'F')) {
                hDecoder->adif_header_present = 1;
                faad_log_info("[%s %d]ADIF aac file detected\n", __FUNCTION__, __LINE__);
                get_adif_header(&adif, &ld);
                faad_byte_align(&ld);

                hDecoder->sf_index = adif.pce[0].sf_index;
                hDecoder->object_type = adif.pce[0].object_type + 1;

                *samplerate = get_sample_rate(hDecoder->sf_index);
                *channels = adif.pce[0].channels;

                memcpy(&(hDecoder->pce), &(adif.pce[0]), sizeof(program_config));
                hDecoder->pce_set = 1;

                bits = bit2byte(faad_get_processed_bits(&ld));

                /* Check if an ADTS header is present */
            } else if (faad_showbits(&ld, 12) == 0xfff) {
                hDecoder->adts_header_present = 1;

                adts.old_format = hDecoder->config.useOldADTSFormat;
                adts_frame(&adts, &ld);

                hDecoder->sf_index = adts.sf_index;
                hDecoder->object_type = adts.profile + 1;
                if (adts.sf_index >= 0 && adts.sf_index < 12 && adts.channel_configuration > 0 && adts.channel_configuration <= 8) {
                    hDecoder->last_sf_index = hDecoder->sf_index;
                    hDecoder->last_ch_configure = adts.channel_configuration;
                }
                *samplerate = get_sample_rate(hDecoder->sf_index);
                *channels = (adts.channel_configuration > 6) ?
                            2 : adts.channel_configuration;
            } else {
                /*we guess it is a ADTS aac files and try to resync from the error*/
                int ii;
                int adts_err = 0;
                faad_log_info("[%s %d]guess it is a ADTS aac files and try to resync\n", __FUNCTION__, __LINE__);
                faad_initbits(&ld, buffer, buffer_size);
                for (ii = 0; ii < (int)buffer_size; ii++) {
                    if ((faad_showbits(&ld, 16) & 0xfff6) != 0xFFF0) {
                        faad_getbits(&ld, 8
                                     DEBUGVAR(0, 0, ""));
                    } else {
                        bits = bit2byte(faad_get_processed_bits(&ld));
                        hDecoder->adts_header_present = 1;
                        faad_log_info("[%s %d]resync and got ADTS header\n", __FUNCTION__, __LINE__);
                        adts.old_format = hDecoder->config.useOldADTSFormat;
                        adts_err = adts_frame(&adts, &ld);
                        if (adts_err == 5) {
                            return -1;
                        }
                        hDecoder->sf_index = adts.sf_index;
                        hDecoder->object_type = adts.profile + 1;
                        faad_log_info("sf index %d,object type %d \n", hDecoder->sf_index, hDecoder->object_type);
                        if (adts.sf_index >= 0 && adts.sf_index < 12 && adts.channel_configuration > 0 && adts.channel_configuration <= 8) {
                            hDecoder->last_sf_index = hDecoder->sf_index;
                            hDecoder->last_ch_configure = adts.channel_configuration;
                        }
                        *samplerate = get_sample_rate(hDecoder->sf_index);
                        if (*samplerate > 96000 || adts.channel_configuration > 6 || hDecoder->sf_index >= 12) {
                            return -1;
                        }
                        *channels = (adts.channel_configuration > 6) ? 2 : adts.channel_configuration;
                        faad_log_info("[%s %d]resync adts info:FS/%lu object_type/%d chnum/%d\n", __FUNCTION__, __LINE__, *samplerate, hDecoder->object_type, (int)channels);
                        break;
                    }
                }
                if (ii == (int)buffer_size) {
                    faad_log_info("[%s %d]sync for adts frame failed\n", __FUNCTION__, __LINE__);
                    return -1;
                }
            }
    }
    if (ld.error) {
        faad_endbits(&ld);
        return -1;
    }
    faad_endbits(&ld);

#if (defined(PS_DEC) || defined(DRM_PS))
    /* check if we have a mono file */
    if (*channels == 1) {
        /* upMatrix to 2 channels for implicit signalling of PS */
        *channels = 2;
    }
#endif

    hDecoder->channelConfiguration = *channels;

#ifdef SBR_DEC
    /* implicit signalling */
    if (*samplerate <= 24000 && (hDecoder->config.dontUpSampleImplicitSBR == 0)) {
        *samplerate *= 2;
        hDecoder->forceUpSampling = 1;
    } else if (*samplerate > 24000 && (hDecoder->config.dontUpSampleImplicitSBR == 0)) {
        hDecoder->downSampledSBR = 1;
    }
#endif

    /* must be done before frameLength is divided by 2 for LD */
#ifdef SSR_DEC
    if (hDecoder->object_type == SSR) {
        hDecoder->fb = ssr_filter_bank_init(hDecoder->frameLength / SSR_BANDS);
    } else
#endif
        hDecoder->fb = filter_bank_init(hDecoder->frameLength);

#ifdef LD_DEC
    if (hDecoder->object_type == LD) {
        hDecoder->frameLength >>= 1;
    }
#endif

    if (can_decode_ot(hDecoder->object_type) < 0) {
        faad_log_info("[%s %d]object_type/%d can not support\n", __FUNCTION__, __LINE__, hDecoder->object_type);
        return -1;
    }
    faad_log_info("[%s %d]aac init finished. cost bits%d\n", __FUNCTION__, __LINE__, bits);
    return bits;
}

/* Init the library using a DecoderSpecificInfo */
int NEAACDECAPI NeAACDecInit2(NeAACDecHandle hpDecoder,
                              unsigned char *pBuffer,
                              unsigned long SizeOfDecoderSpecificInfo,
                              unsigned long *samplerate,
                              unsigned char *channels)
{
    NeAACDecStruct* hDecoder = (NeAACDecStruct*)hpDecoder;
    int8_t rc;
    mp4AudioSpecificConfig mp4ASC;
    faad_log_info("enter NeAACDecInit2 \r\n");
    faad_log_info("extra data size  %lu\r\n", SizeOfDecoderSpecificInfo);
    if ((hDecoder == NULL)
        || (pBuffer == NULL)
        || (SizeOfDecoderSpecificInfo < 2)
        || (samplerate == NULL)
        || (channels == NULL)) {
        return -1;
    }

    hDecoder->adif_header_present = 0;
    hDecoder->adts_header_present = 0;

    /* decode the audio specific config */
    rc = AudioSpecificConfig2(pBuffer, SizeOfDecoderSpecificInfo, &mp4ASC,
                              &(hDecoder->pce), hDecoder->latm_header_present);

    /* copy the relevant info to the decoder handle */
    *samplerate = mp4ASC.samplingFrequency;
    if (mp4ASC.channelsConfiguration) {
        *channels = mp4ASC.channelsConfiguration;
    } else {
        *channels = hDecoder->pce.channels;
        hDecoder->pce_set = 1;
    }
#if (defined(PS_DEC) || defined(DRM_PS))
    /* check if we have a mono file */
    if (*channels == 1) {
        /* upMatrix to 2 channels for implicit signalling of PS */
        *channels = 2;
    }
#endif
    hDecoder->sf_index = mp4ASC.samplingFrequencyIndex;
    hDecoder->object_type = mp4ASC.objectTypeIndex;
#ifdef ERROR_RESILIENCE
    hDecoder->aacSectionDataResilienceFlag = mp4ASC.aacSectionDataResilienceFlag;
    hDecoder->aacScalefactorDataResilienceFlag = mp4ASC.aacScalefactorDataResilienceFlag;
    hDecoder->aacSpectralDataResilienceFlag = mp4ASC.aacSpectralDataResilienceFlag;
#endif
#ifdef SBR_DEC
    hDecoder->sbr_present_flag = mp4ASC.sbr_present_flag;
    hDecoder->downSampledSBR = mp4ASC.downSampledSBR;
    if (hDecoder->config.dontUpSampleImplicitSBR == 0) {
        hDecoder->forceUpSampling = mp4ASC.forceUpSampling;
    } else {
        hDecoder->forceUpSampling = 0;
    }

    /* AAC core decoder samplerate is 2 times as low */
    if (((hDecoder->sbr_present_flag == 1) && (!hDecoder->downSampledSBR)) || hDecoder->forceUpSampling == 1) {
        hDecoder->sf_index = get_sr_index(mp4ASC.samplingFrequency / 2);
    }
#endif

    if (rc != 0) {
        return rc;
    }
    hDecoder->channelConfiguration = mp4ASC.channelsConfiguration;
    if (mp4ASC.frameLengthFlag)
#ifdef ALLOW_SMALL_FRAMELENGTH
        hDecoder->frameLength = 960;
#else
        return -1;
#endif

    /* must be done before frameLength is divided by 2 for LD */
#ifdef SSR_DEC
    if (hDecoder->object_type == SSR) {
        hDecoder->fb = ssr_filter_bank_init(hDecoder->frameLength / SSR_BANDS);
    } else
#endif
        hDecoder->fb = filter_bank_init(hDecoder->frameLength);

#ifdef LD_DEC
    if (hDecoder->object_type == LD) {
        hDecoder->frameLength >>= 1;
    }
#endif
    faad_log_info("aac init2 finished\r\n");
    return 0;
}

#ifdef DRM
char NEAACDECAPI NeAACDecInitDRM(NeAACDecHandle *hpDecoder,
                                 unsigned long samplerate,
                                 unsigned char channels)
{
    NeAACDecStruct** hDecoder = (NeAACDecStruct**)hpDecoder;
    if (hDecoder == NULL) {
        return 1;    /* error */
    }

    NeAACDecClose(*hDecoder);

    *hDecoder = NeAACDecOpen();

    /* Special object type defined for DRM */
    (*hDecoder)->config.defObjectType = DRM_ER_LC;

    (*hDecoder)->config.defSampleRate = samplerate;
#ifdef ERROR_RESILIENCE // This shoudl always be defined for DRM
    (*hDecoder)->aacSectionDataResilienceFlag = 1; /* VCB11 */
    (*hDecoder)->aacScalefactorDataResilienceFlag = 0; /* no RVLC */
    (*hDecoder)->aacSpectralDataResilienceFlag = 1; /* HCR */
#endif
    (*hDecoder)->frameLength = 960;
    (*hDecoder)->sf_index = get_sr_index((*hDecoder)->config.defSampleRate);
    (*hDecoder)->object_type = (*hDecoder)->config.defObjectType;

    if ((channels == DRMCH_STEREO) || (channels == DRMCH_SBR_STEREO)) {
        (*hDecoder)->channelConfiguration = 2;
    } else {
        (*hDecoder)->channelConfiguration = 1;
    }

#ifdef SBR_DEC
    if ((channels == DRMCH_MONO) || (channels == DRMCH_STEREO)) {
        (*hDecoder)->sbr_present_flag = 0;
    } else {
        (*hDecoder)->sbr_present_flag = 1;
    }
#endif

    (*hDecoder)->fb = filter_bank_init((*hDecoder)->frameLength);

    return 0;
}
#endif

void NEAACDECAPI NeAACDecClose(NeAACDecHandle hpDecoder)
{
    uint8_t i;
    NeAACDecStruct* hDecoder = (NeAACDecStruct*)hpDecoder;
#ifdef USE_HELIX_AAC_DECODER
    if (hAACDecoder) {
        AACFreeDecoder(hAACDecoder);
        hAACDecoder = NULL;
    }
#endif
    if (hDecoder == NULL) {
        return;
    }

#ifdef PROFILE
    //printk("AAC decoder total:  %I64d cycles\n", hDecoder->cycles);
    //printk("requant:            %I64d cycles\n", hDecoder->requant_cycles);
    //printk("spectral_data:      %I64d cycles\n", hDecoder->spectral_cycles);
    //printk("scalefactors:       %I64d cycles\n", hDecoder->scalefac_cycles);
    //printk("output:             %I64d cycles\n", hDecoder->output_cycles);
#endif

    for (i = 0; i < MAX_CHANNELS; i++) {
        if (hDecoder->time_out[i]) {
            faad_free(hDecoder->time_out[i]);
        }
        if (hDecoder->fb_intermed[i]) {
            faad_free(hDecoder->fb_intermed[i]);
        }
#ifdef SSR_DEC
        if (hDecoder->ssr_overlap[i]) {
            faad_free(hDecoder->ssr_overlap[i]);
        }
        if (hDecoder->prev_fmd[i]) {
            faad_free(hDecoder->prev_fmd[i]);
        }
#endif
#ifdef MAIN_DEC
        if (hDecoder->pred_stat[i]) {
            faad_free(hDecoder->pred_stat[i]);
        }
#endif
#ifdef LTP_DEC
        if (hDecoder->lt_pred_stat[i]) {
            faad_free(hDecoder->lt_pred_stat[i]);
        }
#endif
    }

#ifdef SSR_DEC
    if (hDecoder->object_type == SSR) {
        ssr_filter_bank_end(hDecoder->fb);
    } else
#endif
        filter_bank_end(hDecoder->fb);

    drc_end(hDecoder->drc);

    if (hDecoder->sample_buffer) {
        faad_free(hDecoder->sample_buffer);
    }


    if (hDecoder->sample_buffer_all) {
        faad_free(hDecoder->sample_buffer_all);
    }

#ifdef SBR_DEC
    for (i = 0; i < MAX_SYNTAX_ELEMENTS; i++) {
        if (hDecoder->sbr[i]) {
            sbrDecodeEnd(hDecoder->sbr[i]);
        }
    }
#endif
    //why not free before?
    if (hDecoder) {
        faad_free(hDecoder);
    }
}

void NEAACDECAPI NeAACDecPostSeekReset(NeAACDecHandle hpDecoder, long frame)
{
    NeAACDecStruct* hDecoder = (NeAACDecStruct*)hpDecoder;
    if (hDecoder) {
        hDecoder->postSeekResetFlag = 1;

        if (frame != -1) {
            hDecoder->frame = frame;
        }
    }
}

static void create_channel_config(NeAACDecStruct *hDecoder, NeAACDecFrameInfo *hInfo)
{
    hInfo->num_front_channels = 0;
    hInfo->num_side_channels = 0;
    hInfo->num_back_channels = 0;
    hInfo->num_lfe_channels = 0;
    memset(hInfo->channel_position, 0, MAX_CHANNELS * sizeof(uint8_t));

    if (hDecoder->downMatrix) {
        hInfo->num_front_channels = 2;
        hInfo->channel_position[0] = FRONT_CHANNEL_LEFT;
        hInfo->channel_position[1] = FRONT_CHANNEL_RIGHT;
        return;
    }

    /* check if there is a PCE */
    if (hDecoder->pce_set) {
        uint8_t i, chpos = 0;
        uint8_t chdir, back_center = 0;

        hInfo->num_front_channels = hDecoder->pce.num_front_channels;
        hInfo->num_side_channels = hDecoder->pce.num_side_channels;
        hInfo->num_back_channels = hDecoder->pce.num_back_channels;
        hInfo->num_lfe_channels = hDecoder->pce.num_lfe_channels;

        chdir = hInfo->num_front_channels;
        if (chdir & 1) {
#if (defined(PS_DEC) || defined(DRM_PS))
            /* When PS is enabled output is always stereo */
            hInfo->channel_position[chpos++] = FRONT_CHANNEL_LEFT;
            hInfo->channel_position[chpos++] = FRONT_CHANNEL_RIGHT;
#else
            hInfo->channel_position[chpos++] = FRONT_CHANNEL_CENTER;
            chdir--;
#endif
        }
        for (i = 0; i < chdir; i += 2) {
            hInfo->channel_position[chpos++] = FRONT_CHANNEL_LEFT;
            hInfo->channel_position[chpos++] = FRONT_CHANNEL_RIGHT;
        }

        for (i = 0; i < hInfo->num_side_channels; i += 2) {
            hInfo->channel_position[chpos++] = SIDE_CHANNEL_LEFT;
            hInfo->channel_position[chpos++] = SIDE_CHANNEL_RIGHT;
        }

        chdir = hInfo->num_back_channels;
        if (chdir & 1) {
            back_center = 1;
            chdir--;
        }
        for (i = 0; i < chdir; i += 2) {
            hInfo->channel_position[chpos++] = BACK_CHANNEL_LEFT;
            hInfo->channel_position[chpos++] = BACK_CHANNEL_RIGHT;
        }
        if (back_center) {
            hInfo->channel_position[chpos++] = BACK_CHANNEL_CENTER;
        }

        for (i = 0; i < hInfo->num_lfe_channels; i++) {
            hInfo->channel_position[chpos++] = LFE_CHANNEL;
        }

    } else {
        switch (hDecoder->channelConfiguration) {
        case 1:
#if (defined(PS_DEC) || defined(DRM_PS))
            /* When PS is enabled output is always stereo */
            hInfo->num_front_channels = 2;
            hInfo->channel_position[0] = FRONT_CHANNEL_LEFT;
            hInfo->channel_position[1] = FRONT_CHANNEL_RIGHT;
#else
            hInfo->num_front_channels = 1;
            hInfo->channel_position[0] = FRONT_CHANNEL_CENTER;
#endif
            break;
        case 2:
            hInfo->num_front_channels = 2;
            hInfo->channel_position[0] = FRONT_CHANNEL_LEFT;
            hInfo->channel_position[1] = FRONT_CHANNEL_RIGHT;
            break;
        case 3:
            hInfo->num_front_channels = 3;
            hInfo->channel_position[0] = FRONT_CHANNEL_CENTER;
            hInfo->channel_position[1] = FRONT_CHANNEL_LEFT;
            hInfo->channel_position[2] = FRONT_CHANNEL_RIGHT;
            break;
        case 4:
            hInfo->num_front_channels = 3;
            hInfo->num_back_channels = 1;
            hInfo->channel_position[0] = FRONT_CHANNEL_CENTER;
            hInfo->channel_position[1] = FRONT_CHANNEL_LEFT;
            hInfo->channel_position[2] = FRONT_CHANNEL_RIGHT;
            hInfo->channel_position[3] = BACK_CHANNEL_CENTER;
            break;
        case 5:
            hInfo->num_front_channels = 3;
            hInfo->num_back_channels = 2;
            hInfo->channel_position[0] = FRONT_CHANNEL_CENTER;
            hInfo->channel_position[1] = FRONT_CHANNEL_LEFT;
            hInfo->channel_position[2] = FRONT_CHANNEL_RIGHT;
            hInfo->channel_position[3] = BACK_CHANNEL_LEFT;
            hInfo->channel_position[4] = BACK_CHANNEL_RIGHT;
            break;
        case 6:
            hInfo->num_front_channels = 3;
            hInfo->num_back_channels = 2;
            hInfo->num_lfe_channels = 1;
            hInfo->channel_position[0] = FRONT_CHANNEL_CENTER;
            hInfo->channel_position[1] = FRONT_CHANNEL_LEFT;
            hInfo->channel_position[2] = FRONT_CHANNEL_RIGHT;
            hInfo->channel_position[3] = BACK_CHANNEL_LEFT;
            hInfo->channel_position[4] = BACK_CHANNEL_RIGHT;
            hInfo->channel_position[5] = LFE_CHANNEL;
            break;
        case 7:
            hInfo->num_front_channels = 3;
            hInfo->num_side_channels = 2;
            hInfo->num_back_channels = 2;
            hInfo->num_lfe_channels = 1;
            hInfo->channel_position[0] = FRONT_CHANNEL_CENTER;
            hInfo->channel_position[1] = FRONT_CHANNEL_LEFT;
            hInfo->channel_position[2] = FRONT_CHANNEL_RIGHT;
            hInfo->channel_position[3] = SIDE_CHANNEL_LEFT;
            hInfo->channel_position[4] = SIDE_CHANNEL_RIGHT;
            hInfo->channel_position[5] = BACK_CHANNEL_LEFT;
            hInfo->channel_position[6] = BACK_CHANNEL_RIGHT;
            hInfo->channel_position[7] = LFE_CHANNEL;
            break;
        default: { /* channelConfiguration == 0 || channelConfiguration > 7 */
            uint8_t i;
            uint8_t ch = hDecoder->fr_channels - hDecoder->has_lfe;
            if (ch & 1) { /* there's either a center front or a center back channel */
                uint8_t ch1 = (ch - 1) / 2;
                if (hDecoder->first_syn_ele == ID_SCE) {
                    hInfo->num_front_channels = ch1 + 1;
                    hInfo->num_back_channels = ch1;
                    hInfo->channel_position[0] = FRONT_CHANNEL_CENTER;
                    for (i = 1; i <= ch1; i += 2) {
                        hInfo->channel_position[i] = FRONT_CHANNEL_LEFT;
                        hInfo->channel_position[i + 1] = FRONT_CHANNEL_RIGHT;
                    }
                    for (i = ch1 + 1; i < ch; i += 2) {
                        hInfo->channel_position[i] = BACK_CHANNEL_LEFT;
                        hInfo->channel_position[i + 1] = BACK_CHANNEL_RIGHT;
                    }
                } else {
                    hInfo->num_front_channels = ch1;
                    hInfo->num_back_channels = ch1 + 1;
                    for (i = 0; i < ch1; i += 2) {
                        hInfo->channel_position[i] = FRONT_CHANNEL_LEFT;
                        hInfo->channel_position[i + 1] = FRONT_CHANNEL_RIGHT;
                    }
                    for (i = ch1; i < ch - 1; i += 2) {
                        hInfo->channel_position[i] = BACK_CHANNEL_LEFT;
                        hInfo->channel_position[i + 1] = BACK_CHANNEL_RIGHT;
                    }
                    hInfo->channel_position[ch - 1] = BACK_CHANNEL_CENTER;
                }
            } else {
                uint8_t ch1 = (ch) / 2;
                hInfo->num_front_channels = ch1;
                hInfo->num_back_channels = ch1;
                if (ch1 & 1) {
                    hInfo->channel_position[0] = FRONT_CHANNEL_CENTER;
                    for (i = 1; i <= ch1; i += 2) {
                        hInfo->channel_position[i] = FRONT_CHANNEL_LEFT;
                        hInfo->channel_position[i + 1] = FRONT_CHANNEL_RIGHT;
                    }
                    for (i = ch1 + 1; i < ch - 1; i += 2) {
                        hInfo->channel_position[i] = BACK_CHANNEL_LEFT;
                        hInfo->channel_position[i + 1] = BACK_CHANNEL_RIGHT;
                    }
                    hInfo->channel_position[ch - 1] = BACK_CHANNEL_CENTER;
                } else {
                    for (i = 0; i < ch1; i += 2) {
                        hInfo->channel_position[i] = FRONT_CHANNEL_LEFT;
                        hInfo->channel_position[i + 1] = FRONT_CHANNEL_RIGHT;
                    }
                    for (i = ch1; i < ch; i += 2) {
                        hInfo->channel_position[i] = BACK_CHANNEL_LEFT;
                        hInfo->channel_position[i + 1] = BACK_CHANNEL_RIGHT;
                    }
                }
            }
            hInfo->num_lfe_channels = hDecoder->has_lfe;
            for (i = ch; i < hDecoder->fr_channels; i++) {
                hInfo->channel_position[i] = LFE_CHANNEL;
            }
        }
        break;
        }
    }
}

void* NEAACDECAPI NeAACDecDecode(NeAACDecHandle hpDecoder,
                                 NeAACDecFrameInfo *hInfo,
                                 unsigned char *buffer,
                                 unsigned long buffer_size)
{
    NeAACDecStruct* hDecoder = (NeAACDecStruct*)hpDecoder;
    return aac_frame_decode(hDecoder, hInfo, buffer, buffer_size, NULL, 0);
}

void* NEAACDECAPI NeAACDecDecode2(NeAACDecHandle hpDecoder,
                                  NeAACDecFrameInfo *hInfo,
                                  unsigned char *buffer,
                                  unsigned long buffer_size,
                                  void **sample_buffer,
                                  unsigned long sample_buffer_size)
{
    NeAACDecStruct* hDecoder = (NeAACDecStruct*)hpDecoder;
    if ((sample_buffer == NULL) || (sample_buffer_size == 0)) {
        hInfo->error = 27;
        return NULL;
    }

    return aac_frame_decode(hDecoder, hInfo, buffer, buffer_size,
                            sample_buffer, sample_buffer_size);
}

#ifdef DRM

#define ERROR_STATE_INIT 6

static void conceal_output(NeAACDecStruct *hDecoder, uint16_t frame_len,
                           uint8_t out_ch, void *sample_buffer)
{
    return;
}
#endif

static int multi_sub_frame(NeAACDecStruct *hDecoder)
{
#ifdef NEW_CODE_CHECK_LATM
    //int i_frame_size;
    decoder_sys_t *p_sys =  &hDecoder->dec_sys;

    if (hDecoder->latm_header_present && p_sys->latm.i_sub_frames > 1)
        return 1;
#endif

    return 0;
}

static void* aac_frame_decode(NeAACDecStruct *hDecoder,
                              NeAACDecFrameInfo *hInfo,
                              unsigned char *buffer,
                              unsigned long buffer_size,
                              void **sample_buffer2,
                              unsigned long sample_buffer_size)
{
    uint16_t i;
    uint8_t channels = 0;
    uint8_t output_channels = 0;
    bitfile ld;// = {0};
    uint32_t bitsconsumed;
    uint16_t frame_len;
    void *sample_buffer;
    uint32_t startbit = 0, endbit = 0, payload_bits = 0;
    int b_multi_sub_frame;
    int mux_length = 0;
    short* dec_buffer = hDecoder->dec_buffer;
    short* output_buffer = hDecoder->output_buffer;
    unsigned char* temp_bufer = hDecoder->temp_bufer;
    int temp_size = 0;
#ifdef NEW_CODE_CHECK_LATM
    int i_frame_size;
    decoder_sys_t *p_sys =  &hDecoder->dec_sys;
#endif
#ifdef PROFILE
    int64_t count = faad_get_ts();
#endif

    /* safety checks */
    if ((hDecoder == NULL) || (hInfo == NULL) || (buffer == NULL)) {
        return NULL;
    }

    frame_len = hDecoder->frameLength;
    memset(hInfo, 0, sizeof(NeAACDecFrameInfo));
    memset(hDecoder->internal_channel, 0, MAX_CHANNELS * sizeof(hDecoder->internal_channel[0]));

#ifdef USE_TIME_LIMIT
    if ((TIME_LIMIT * get_sample_rate(hDecoder->sf_index)) > hDecoder->TL_count) {
        hDecoder->TL_count += 1024;
    } else {
        hInfo->error = (NUM_ERROR_MESSAGES - 1);
        goto error;
    }
#endif


    /* check for some common metadata tag types in the bitstream
     * No need to return an error
     */
    /* ID3 */
    if (buffer_size >= 128) {
        if (memcmp(buffer, "TAG", 3) == 0) {
            /* found it */
            hInfo->bytesconsumed = 128; /* 128 bytes fixed size */
            /* no error, but no output either */
            return NULL;
        }
    }
#ifdef NEW_CODE_CHECK_LATM
    if (buffer_size > TMP_BUF_SIZE) {
        LATM_LOG("input buffer size tooo big %lu, buffer size %d \n", buffer_size,TMP_BUF_SIZE);
        //buffer_size = sizeof(temp_bufer);
        buffer_size =  TMP_BUF_SIZE;
    }
    if (buffer_size > 0) {
        memcpy(temp_bufer, buffer, buffer_size);
        temp_size = buffer_size;
        buffer  = temp_bufer;
    }
NEXT_CHECK:
    if (hDecoder->latm_header_present) {
        while (buffer_size >= 7) {
            if (buffer[0] == 0x56 && (buffer[1] & 0xe0) == 0xe0) {

                break;
            }
            buffer++;
            buffer_size--;
        }
        if (buffer_size <= 2) {
            LATM_LOG("check the loas frame failed\n");
            return  NULL;
        }
        /* Check if frame is valid and get frame info */
        i_frame_size = ((buffer[1] & 0x1f) << 8) + buffer[2];
        //LATM_LOG("i_frame_size  %d \n",i_frame_size);
        mux_length = i_frame_size + 3;
        if (i_frame_size <= 0) {
            LATM_LOG("i_frame_size  error\n");
            return NULL;
        }
        if ((int)buffer_size < (LOAS_HEADER_SIZE + i_frame_size)) {
            hInfo->error =  35;
            LATM_LOG("buffer size small then frame size,need more data\n");
            return NULL;
        }
#if 1
        if (buffer[3 + i_frame_size] != 0x56 || (buffer[3 + i_frame_size + 1] & 0xe0) != 0xe0) {

            LATM_LOG("emulated sync word (no sync on following frame) \n");
            buffer++;
            buffer_size--;
            goto NEXT_CHECK;
        }
#endif
        buffer += LOAS_HEADER_SIZE; //skip header
        buffer_size = buffer_size - LOAS_HEADER_SIZE;
        i_frame_size = LOASParse(buffer, i_frame_size, p_sys);
        if (i_frame_size <= 0) {
            goto NEXT_CHECK;
        } else {
            //  LATM_LOG("latm detected\n");
        }

    }

    b_multi_sub_frame = multi_sub_frame(hDecoder);

    /* check if we want to use internal sample_buffer */
    if (sample_buffer_size == 0 && b_multi_sub_frame) {
        if (hDecoder->sample_buffer_all) {
            faad_free(hDecoder->sample_buffer_all);
        }
        hDecoder->sample_buffer_all = NULL;
    }
#endif

start_decode:

    /* initialize the bitstream */
    faad_initbits(&ld, buffer, buffer_size);

#ifndef NEW_CODE_CHECK_LATM
    if (hDecoder->latm_header_present) {
        payload_bits = faad_latm_frame(&hDecoder->latm_config, &ld);
        startbit = faad_get_processed_bits(&ld);
        if (payload_bits == -1U) {
            hInfo->error = 1;
            goto error;
        }
    }
#endif
#ifdef DRM
    if (hDecoder->object_type == DRM_ER_LC) {
        /* We do not support stereo right now */
        if (0) { //(hDecoder->channelConfiguration == 2)
            hInfo->error = 28; // Throw CRC error
            goto error;
        }

        faad_getbits(&ld, 8
                     DEBUGVAR(1, 1, "NeAACDecDecode(): skip CRC"));
    }
#endif

    if (hDecoder->adts_header_present) {
        adts_header adts;

        adts.old_format = hDecoder->config.useOldADTSFormat;
        if ((hInfo->error = adts_frame(&adts, &ld)) > 0) {
            goto error;
        }
        if (adts.aac_frame_length > buffer_size) {
            hInfo->error = 35; //more data needed
            audio_codec_print("decoder need more data for adts frame,frame len %d,have %lu \n", adts.aac_frame_length, buffer_size);
            if (adts.aac_frame_length > 6 * 768) {
                audio_codec_print("adts frame len exceed aac spec \n");
                hInfo->error = 36;//
                goto error;

            }
            //here need return to get more input data for decoder
            faad_endbits(&ld);
            return NULL;
        }
        if (adts.sf_index >= 12 || adts.channel_configuration > 6) {
            audio_codec_print("adts sf/ch error,sf %d,ch config %d \n", adts.sf_index, adts.channel_configuration);
            hDecoder->sf_index = 3;
            hInfo->error = 12;
            goto error;
        }
        hDecoder->sf_index = adts.sf_index;
        if (adts.sf_index != hDecoder->last_sf_index && adts.channel_configuration != hDecoder->last_ch_configure) {
            if (adts.sf_index >= 0 && adts.sf_index < 12 && adts.channel_configuration > 0 && adts.channel_configuration <= 8) {
                hInfo->error = 34;
                audio_codec_print("[%s %d]last_sf_index/%d,Ch/%d,Now %d/%d\n", __FUNCTION__, __LINE__, hDecoder->last_sf_index, hDecoder->last_ch_configure, adts.sf_index, adts.channel_configuration);
                hDecoder->last_sf_index = hDecoder->sf_index;
                hDecoder->last_ch_configure = adts.channel_configuration;
                goto error;
            }
        }
    }

#ifdef ANALYSIS
    dbg_count = 0;
#endif

    /* decode the complete bitstream */
#ifdef DRM
    if (/*(hDecoder->object_type == 6) ||*/ (hDecoder->object_type == DRM_ER_LC)) {
        DRM_aac_scalable_main_element(hDecoder, hInfo, &ld, &hDecoder->pce, hDecoder->drc);
    } else {
#endif
        raw_data_block(hDecoder, hInfo, &ld, &hDecoder->pce, hDecoder->drc);
#ifdef DRM
    }
#endif
#ifndef  NEW_CODE_CHECK_LATM
    if (hDecoder->latm_header_present) {
        endbit = faad_get_processed_bits(&ld);
        if (endbit - startbit > payload_bits)
            DEBUG("[%s %d]ERROR, too many payload bits read: %u > %d. Please. report with a link to a sample\n",
                  __FUNCTION__, __LINE__, endbit - startbit, payload_bits);
        if (hDecoder->latm_config.otherDataLenBits > 0) {
            faad_getbits(&ld, hDecoder->latm_config.otherDataLenBits);
        }
        faad_byte_align(&ld);
    }
#endif

    channels = hDecoder->fr_channels;

    if (hInfo->error > 0) {
        goto error;
    }

    /* safety check */
    if (channels == 0 || channels > MAX_CHANNELS) {
        DEBUG("[%s %d]invalid Channels/%d\n", __FUNCTION__, __LINE__, channels);
        hInfo->error = 12;
        goto error;
    }

    /* no more bit reading after this */
    bitsconsumed = faad_get_processed_bits(&ld);
    hInfo->bytesconsumed = bit2byte(bitsconsumed);
    if (mux_length && hDecoder->latm_header_present && !ld.error) {
        if (p_sys->latm.i_sub_frames <= 1)
            hInfo->bytesconsumed = mux_length;
    }
    if (ld.error) {
        hInfo->error = 14;
        goto error;
    }
    faad_endbits(&ld);


    if (!hDecoder->adts_header_present && !hDecoder->adif_header_present
#if 1
        && !hDecoder->latm_header_present
#endif
       ) {
        if (hDecoder->channelConfiguration == 0) {
            hDecoder->channelConfiguration = channels;
        }

        if (channels == 8) { /* 7.1 */
            hDecoder->channelConfiguration = 7;
        }
        if (channels == 7) { /* not a standard channelConfiguration */
            hDecoder->channelConfiguration = 0;
        }
    }

    if ((channels == 5 || channels == 6) && hDecoder->config.downMatrix) {
        hDecoder->downMatrix = 1;
        output_channels = 2;
    } else {
        //      output_channels = channels;
        if (channels == 6 || channels == 4) {
            output_channels = 2;
        } else if (channels == 3 && hDecoder->config.downMatrix) {
            output_channels = 2;
        } else {
            output_channels = channels;
        }
    }

#if (defined(PS_DEC) || defined(DRM_PS))
    hDecoder->upMatrix = 0;
    /* check if we have a mono file */
    if (output_channels == 1) {
        /* upMatrix to 2 channels for implicit signalling of PS */
        hDecoder->upMatrix = 1;
        output_channels = 2;
    }
#endif

    /* Make a channel configuration based on either a PCE or a channelConfiguration */
    create_channel_config(hDecoder, hInfo);

    /* number of samples in this frame */
    hInfo->samples = frame_len * output_channels;
    /* number of channels in this frame */
    hInfo->channels = output_channels;
    /* samplerate */
    hInfo->samplerate = get_sample_rate(hDecoder->sf_index);
    /* object type */
    hInfo->object_type = hDecoder->object_type;
    /* sbr */
    hInfo->sbr = NO_SBR;
    /* header type */
    hInfo->header_type = RAW;
    if (hDecoder->adif_header_present) {
        hInfo->header_type = ADIF;
    }
    if (hDecoder->adts_header_present) {
        hInfo->header_type = ADTS;
    }
#if 1
    if (hDecoder->latm_header_present) {
        hInfo->header_type = LATM;
    }
#endif
#if (defined(PS_DEC) || defined(DRM_PS))
    hInfo->ps = hDecoder->ps_used_global;
#endif

    /* check if frame has channel elements */
    if (channels == 0) {
        hDecoder->frame++;
        return NULL;
    }

    /* allocate the buffer for the final samples */
    if ((hDecoder->sample_buffer == NULL) ||
        (hDecoder->alloced_channels != output_channels)) {
        static const uint8_t str[] = { sizeof(int16_t), sizeof(int32_t), sizeof(int32_t),
                                       sizeof(float32_t), sizeof(double), sizeof(int16_t), sizeof(int16_t),
                                       sizeof(int16_t), sizeof(int16_t), 0, 0, 0
                                     };
        uint8_t stride = str[hDecoder->config.outputFormat - 1];
#ifdef SBR_DEC
        if (((hDecoder->sbr_present_flag == 1) && (!hDecoder->downSampledSBR)) || (hDecoder->forceUpSampling == 1)) {
            stride = 2 * stride;
        }
#endif
        /* check if we want to use internal sample_buffer */
        if (sample_buffer_size == 0) {
            if (hDecoder->sample_buffer) {
                faad_free(hDecoder->sample_buffer);
            }
            hDecoder->sample_buffer = NULL;
            hDecoder->sample_buffer = faad_malloc(frame_len * output_channels * stride);
        } else if (sample_buffer_size < frame_len * output_channels * stride) {
            /* provided sample buffer is not big enough */
            hInfo->error = 27;
            return NULL;
        }
        hDecoder->alloced_channels = output_channels;
    }

    if (sample_buffer_size == 0) {
        sample_buffer = hDecoder->sample_buffer;
    } else {
        sample_buffer = *sample_buffer2;
    }

#ifdef SBR_DEC
    if ((hDecoder->sbr_present_flag == 1) || (hDecoder->forceUpSampling == 1)) {
        uint8_t ele;

        /* this data is different when SBR is used or when the data is upsampled */
        if (!hDecoder->downSampledSBR) {
            frame_len *= 2;
            hInfo->samples *= 2;
            hInfo->samplerate *= 2;
        }

        /* check if every element was provided with SBR data */
        for (ele = 0; ele < hDecoder->fr_ch_ele; ele++) {
            if (hDecoder->sbr[ele] == NULL) {
                hInfo->error = 25;
                goto error;
            }
        }

        /* sbr */
        if (hDecoder->sbr_present_flag == 1) {
            hInfo->object_type = HE_AAC;
            hInfo->sbr = SBR_UPSAMPLED;
        } else {
            hInfo->sbr = NO_SBR_UPSAMPLED;
        }
        if (hDecoder->downSampledSBR) {
            hInfo->sbr = SBR_DOWNSAMPLED;
        }
    }
#endif

    if (b_multi_sub_frame && sample_buffer_size == 0 &&
        hDecoder->sample_buffer_all == NULL) {

        hDecoder->sample_buffer_all = faad_malloc(p_sys->latm.i_sub_frames * hInfo->samples * 2);
    }

    sample_buffer = output_to_PCM(hDecoder, hDecoder->time_out, sample_buffer,
                                  output_channels, frame_len, hDecoder->config.outputFormat);

    if (b_multi_sub_frame && i_frame_size > 0 && sample_buffer_size == 0) {

        memcpy(hDecoder->sample_buffer_all,sample_buffer,hInfo->samples * 2);
        char * tmp = (char *)(hDecoder->sample_buffer_all);
        tmp += hInfo->samples * 2;
        i_frame_size -= hInfo->bytesconsumed;
        buffer += hInfo->bytesconsumed;
        buffer_size -= hInfo->bytesconsumed;
        if (i_frame_size > 0)
            goto start_decode;
    }

    if (b_multi_sub_frame && sample_buffer_size == 0) {
        // calculate all sub_frames as one samples
        hInfo->samples = hInfo->samples * p_sys->latm.i_sub_frames;
        char * tmp = (char *)(hDecoder->sample_buffer_all);
        tmp -= hInfo->samples * 2;
        hInfo->bytesconsumed = mux_length;
        return hDecoder->sample_buffer_all;
    }


#ifdef DRM
    //conceal_output(hDecoder, frame_len, output_channels, sample_buffer);
#endif


    hDecoder->postSeekResetFlag = 0;

    hDecoder->frame++;
#ifdef LD_DEC
    if (hDecoder->object_type != LD) {
#endif
        if (hDecoder->frame <= 1) {
            hInfo->samples = 0;
        }
#ifdef LD_DEC
    } else {
        /* LD encoders will give lower delay */
        if (hDecoder->frame <= 0) {
            hInfo->samples = 0;
        }
    }
#endif

    /* cleanup */
#ifdef ANALYSIS
    fflush(stdout);
#endif

#ifdef PROFILE
    count = faad_get_ts() - count;
    hDecoder->cycles += count;
#endif

#ifdef USE_HELIX_AAC_DECODER
    /* Channel definitions */
#define FRONT_CENTER (0)
#define FRONT_LEFT   (1)
#define FRONT_RIGHT  (2)
#define SIDE_LEFT    (3)
#define SIDE_RIGHT   (4)
#define BACK_LEFT    (5)
#define LFE_CHANNEL  (6)

    if (hDecoder->latm_header_present && !hInfo->error) {
        unsigned char *dec_buf = buffer;
        int dec_size = hInfo->bytesconsumed ;
        int err;
        int ch_num;
        int sample_out;
        int sum;
        unsigned ch_map_scale[6] = {2, 4, 4, 2, 2, 0}; //full scale == 8
        short *ouput = dec_buffer;
        unsigned char adts_header[7];
        unsigned char *pbuf = NULL;
        unsigned char *inbuf = NULL;
#ifdef PS_DEC
        if (hDecoder->ps_used_global) {
            //  LATM_LOG("decoder ps channel %d \n",channels);
            if (channels == 2) {
                channels = 1;
            }
        }
#endif
        MakeAdtsHeader(hInfo, adts_header, channels);
        pbuf = malloc(7 + dec_size);
        if (!pbuf) {
            LATM_LOG("malloc decoder buffer failed %d \n", dec_size);
            return NULL;
        }
        dec_size  += 7;
        memcpy(pbuf, adts_header, 7);
        memcpy(pbuf + 7, buffer, hInfo->bytesconsumed);
        inbuf = pbuf;
        err = AACDecode(hAACDecoder, &inbuf, &dec_size, dec_buffer);
        if (pbuf) {
            free(pbuf);
            pbuf = NULL;
        }
        if (err == 0) {
            AACFrameInfo aacFrameInfo = {0};
            AACGetLastFrameInfo(hAACDecoder, &aacFrameInfo);
            hInfo->error = 0;
            hInfo->bytesconsumed = mux_length;
            hInfo->channels = aacFrameInfo.nChans > 2 ? 2 : aacFrameInfo.nChans;
            hInfo->samplerate =  aacFrameInfo.sampRateOut;;
            if (aacFrameInfo.nChans > 2) { //should do downmix to 2ch output.
                ch_num =    aacFrameInfo.nChans;
                sample_out = aacFrameInfo.outputSamps / ch_num * 2 * 2; //ch_num*sample_num*16bit
                if (ch_num == 3 || ch_num == 4) {
                    ch_map_scale[0] = 4; //50%
                    ch_map_scale[1] = 4;//50%
                    ch_map_scale[2] = 4;//50%
                    ch_map_scale[3] = 0;
                    ch_map_scale[4] = 0;
                    ch_map_scale[5] = 0;
                }
                for (i = 0; i < aacFrameInfo.outputSamps / ch_num; i++) {
                    if (ch_num == 5 || ch_num == 6) {
                        output_buffer[i * 2] = ((int)(ouput[ch_num * i + FRONT_LEFT]) +
                                                ((int)((((int)ouput[ch_num * i + FRONT_CENTER]) -
                                                        ((int)ouput[ch_num * i + SIDE_LEFT]) -
                                                        ((int)ouput[ch_num * i + SIDE_RIGHT])) * 707 / 1000)));
                        output_buffer[2 * i + 1] = ((int)(ouput[ch_num * i + FRONT_RIGHT]) +
                                                    ((int)((((int)ouput[ch_num * i + FRONT_CENTER]) +
                                                            ((int)ouput[ch_num * i + SIDE_LEFT]) +
                                                            ((int)ouput[ch_num * i + SIDE_RIGHT])) * 707 / 1000)));
                    } else {
                        sum = ((int)ouput[ch_num * i + FRONT_LEFT] * ch_map_scale[FRONT_LEFT] + (int)ouput[ch_num * i + FRONT_CENTER] * ch_map_scale[FRONT_CENTER] + (int)ouput[ch_num * i + BACK_LEFT] * ch_map_scale[BACK_LEFT]);
                        output_buffer[i * 2] = sum >> 3;
                        sum = ((int)ouput[ch_num * i + FRONT_RIGHT] * ch_map_scale[FRONT_RIGHT] + (int)ouput[ch_num * i + FRONT_CENTER] * ch_map_scale[FRONT_CENTER] + (int)ouput[ch_num * i + BACK_LEFT] * ch_map_scale[BACK_LEFT]);
                        output_buffer[2 * i + 1] = sum >> 3;
                    }
                }
            } else {
                sample_out = aacFrameInfo.outputSamps * 2; //ch_num*sample_num*16bit
                memcpy(output_buffer, dec_buffer, sample_out);

            }
            hInfo->samples = sample_out / 2;
            return  output_buffer;

        } else {
            LATM_LOG("decoder error id %d \n", err);
            hInfo->error  = err > 0 ? err : -err;
            return NULL;
        }
    }
#endif
    return sample_buffer;

error:


#ifdef DRM
    hDecoder->error_state = ERROR_STATE_INIT;
#endif

    /* reset filterbank state */
    for (i = 0; i < MAX_CHANNELS; i++) {
        if (hDecoder->fb_intermed[i] != NULL) {
            memset(hDecoder->fb_intermed[i], 0, hDecoder->frameLength * sizeof(real_t));
        }
    }
#ifdef SBR_DEC
    for (i = 0; i < MAX_SYNTAX_ELEMENTS; i++) {
        if (hDecoder->sbr[i] != NULL) {
            sbrReset(hDecoder->sbr[i]);
        }
    }
#endif
    faad_endbits(&ld);
    /* cleanup */
#ifdef ANALYSIS
    fflush(stdout);
#endif

    return NULL;
}

int is_latm_aac(NeAACDecHandle hpDecoder)
{
    NeAACDecStruct* hDecoder = (NeAACDecStruct*)hpDecoder;
    return  hDecoder->latm_header_present;

}

