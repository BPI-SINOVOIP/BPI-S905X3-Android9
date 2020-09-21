/**
* @file aformat.h
* @brief  Porting from decoder driver for audio format
* 
* @version 1.0.0
* @date 2011-02-24
*/
/* Copyright (C) 2007-2011, Amlogic Inc.
* All right reserved
*
*/

/*
 * AMLOGIC Audio/Video streaming port driver.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Author:  Tim Yao <timyao@amlogic.com>
 *
 */

#ifndef AFORMAT_H
#define AFORMAT_H

typedef enum {
    AFORMAT_UNKNOWN = -1,
    AFORMAT_MPEG   = 0,
    AFORMAT_PCM_S16LE = 1,
    AFORMAT_AAC   = 2,
    AFORMAT_AC3   = 3,
    AFORMAT_ALAW = 4,
    AFORMAT_MULAW = 5,
    AFORMAT_DTS = 6,
    AFORMAT_PCM_S16BE = 7,
    AFORMAT_FLAC = 8,
    AFORMAT_COOK = 9,
    AFORMAT_PCM_U8 = 10,
    AFORMAT_ADPCM = 11,
    AFORMAT_AMR  = 12,
    AFORMAT_RAAC  = 13,
    AFORMAT_WMA  = 14,
    AFORMAT_WMAPRO   = 15,
    AFORMAT_PCM_BLURAY  = 16,
    AFORMAT_ALAC  = 17,
    AFORMAT_VORBIS    = 18,
    AFORMAT_AAC_LATM   = 19,
    AFORMAT_APE   = 20,
    AFORMAT_EAC3   = 21,
    AFORMAT_PCM_WIFIDISPLAY = 22,
    AFORMAT_DRA    = 23,
    AFORMAT_SIPR   = 24,
    AFORMAT_TRUEHD = 25,
    AFORMAT_MPEG1  = 26, //AFORMAT_MPEG-->mp3,AFORMAT_MPEG1-->mp1,AFROMAT_MPEG2-->mp2
    AFORMAT_MPEG2  = 27,
    AFORMAT_WMAVOI = 28,
    AFORMAT_WMALOSSLESS =29,
    AFORMAT_PCM_S24LE = 30,
    AFORMAT_UNSUPPORT ,
    AFORMAT_MAX

} aformat_t;

#define AUDIO_EXTRA_DATA_SIZE   (4096)
#define IS_AFMT_VALID(afmt) ((afmt > AFORMAT_UNKNOWN) && (afmt < AFORMAT_MAX))

#define IS_AUIDO_NEED_EXT_INFO(afmt) ((afmt == AFORMAT_ADPCM) \
                                 ||(afmt == AFORMAT_WMA) \
                                 ||(afmt == AFORMAT_WMAPRO) \
                                 ||(afmt == AFORMAT_PCM_S16BE) \
                                 ||(afmt == AFORMAT_PCM_S16LE) \
                                 ||(afmt == AFORMAT_PCM_U8) \
                                 ||(afmt == AFORMAT_PCM_BLURAY) \
                                 ||(afmt == AFORMAT_AMR)\
                                 ||(afmt == AFORMAT_ALAC)\
                                 ||(afmt == AFORMAT_AC3) \
                                 ||(afmt == AFORMAT_EAC3) \
                                 ||(afmt == AFORMAT_APE) \
                                 ||(afmt == AFORMAT_FLAC)\
                                 ||(afmt == AFORMAT_PCM_WIFIDISPLAY) \
                                 ||(afmt == AFORMAT_COOK) \
                                 ||(afmt == AFORMAT_RAAC)) \
                                 ||(afmt == AFORMAT_TRUEHD) \
                                 ||(afmt == AFORMAT_WMAVOI) \
                                 ||(afmt == AFORMAT_WMALOSSLESS)

#define IS_AUDIO_NOT_SUPPORT_EXCEED_2CH(afmt) ((afmt == AFORMAT_RAAC) \
                                        ||(afmt == AFORMAT_COOK) \
                                        /*||(afmt == AFORMAT_FLAC)*/)

#define IS_AUDIO_NOT_SUPPORT_EXCEED_6CH(afmt) ((afmt == AFORMAT_WMAPRO))
#define IS_AUDIO_NOT_SUPPORT_EXCEED_FS48k(afmt) ((afmt == AFORMAT_WMAPRO))


#define IS_AUIDO_NEED_PREFEED_HEADER(afmt) ((afmt == AFORMAT_VORBIS) )
#define IS_AUDIO_NOT_SUPPORTED_BY_AUDIODSP(afmt,codec)  \
                            ((afmt == AFORMAT_AAC_LATM || afmt == AFORMAT_AAC) \
                             &&codec->profile == 0/* FF_PROFILE_AAC_MAIN*/)

#define IS_SUB_NEED_PREFEED_HEADER(sfmt) ((sfmt == CODEC_ID_DVD_SUBTITLE) )

#endif /* AFORMAT_H */

