/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC AUDIODSP_CONTROL
 */


#ifndef AUDIODSP_CONTROL_H
#define AUDIODSP_CONTROL_H

struct audiodsp_cmd {
    int cmd;
    int fmt;
    int data_len;
    char *data;
};

#ifdef ANDROID
#define AUDIODSP_SET_FMT                        _IOW('a',1,long)
#define AUDIODSP_START                          _IOW('a',2,long)
#define AUDIODSP_STOP                           _IOW('a',3,long)
#define AUDIODSP_DECODE_START                   _IOW('a',4,long)
#define AUDIODSP_DECODE_STOP                    _IOW('a',5,long)
#define AUDIODSP_REGISTER_FIRMWARE              _IOW('a',6,long)
#define AUDIODSP_UNREGISTER_ALLFIRMWARE         _IOW('a',7,long)

#define AUDIODSP_GET_CHANNELS_NUM               _IOR('r',1,long)
#define AUDIODSP_GET_SAMPLERATE                 _IOR('r',2,long)
#define AUDIODSP_GET_BITS_PER_SAMPLE            _IOR('r',3,long)
#define AUDIODSP_GET_PTS                        _IOR('r',4,long)
#define AUDIODSP_WAIT_FORMAT			_IOW('a',11,long)
#else

#define AUDIODSP_SET_FMT                        _IOW('a',1,sizeof(long))
#define AUDIODSP_START                          _IOW('a',2,sizeof(long))
#define AUDIODSP_STOP                           _IOW('a',3,sizeof(long))
#define AUDIODSP_DECODE_START                   _IOW('a',4,sizeof(long))
#define AUDIODSP_DECODE_STOP                    _IOW('a',5,sizeof(long))
#define AUDIODSP_REGISTER_FIRMWARE              _IOW('a',6,sizeof(long))
#define AUDIODSP_UNREGISTER_ALLFIRMWARE         _IOW('a',7,sizeof(long))

#define AUDIODSP_GET_CHANNELS_NUM               _IOR('r',1,sizeof(long))
#define AUDIODSP_GET_SAMPLERATE                 _IOR('r',2,sizeof(long))
#define AUDIODSP_GET_BITS_PER_SAMPLE            _IOR('r',3,sizeof(long))
#define AUDIODSP_GET_PTS                        _IOR('r',4,sizeof(long))



#endif
#define MCODEC_FMT_MPEG123                      (1<<0)
#define MCODEC_FMT_AAC                          (1<<1)
#define MCODEC_FMT_AC3                          (1<<2)
#define MCODEC_FMT_DTS                          (1<<3)
#define MCODEC_FMT_FLAC                         (1<<4)
#define MCODEC_FMT_COOK                         (1<<5)
#define MCODEC_FMT_AMR                          (1<<6)
#define MCODEC_FMT_RAAC                         (1<<7)
#define MCODEC_FMT_ADPCM                        (1<<8)
#define MCODEC_FMT_WMA                          (1<<9)
#define MCODEC_FMT_PCM                          (1<<10)
#define MCODEC_FMT_WMAPRO                       (1<<11)
#define MCODEC_FMT_NULL                         (1<<17)

#ifdef __cplusplus
extern "C" {
#endif

int audiodsp_start(void);
int audiodsp_stop(void);

#ifdef __cplusplus
}
#endif

#endif

