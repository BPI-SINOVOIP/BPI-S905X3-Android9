/*
 * * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 * * *
 * This source code is subject to the terms and conditions defined in the
 * * file 'LICENSE' which is part of this source code package.
 * * *
 * Description:
 * */


#ifndef __AUDIOFADE_H__
#define __AUDIOFADE_H__

#define AUD_FADE_ERR -1
#define AUD_FADE_OK 0

#define DEFAULT_FADE_IN_MS  50 //(ms)
#define DEFAULT_FADE_OUT_MS  50 //(ms)

typedef enum {
    fadeLinear,
    fadeInQuad,
    fadeOutQuad,
    fadeInOutQuad,
    fadeInCubic,
    fadeOutCubic,
    fadeInOutCubic,
    fadeInQuart,
    fadeOutQuart,
    fadeInOutQuart,
    fadeInQuint,
    fadeOutQuint,
    fadeInOutQuint,
    fadeMax
} fadeMethod;


// only used for android audio effect fade out/fade in.
typedef enum {
    AUD_FADE_IDLE      = 0,
    AUD_FADE_OUT_START = 1,
    AUD_FADE_OUT       = 2,
    AUD_FADE_MUTE      = 3,
    AUD_FADE_IN_START  = 4,
    AUD_FADE_IN        = 5,
} fade_state_e;


typedef enum {
    FADE_AUD_IN = 0,
    FADE_AUD_OUT = 1,
} fade_direction_e;

typedef struct {
    int mFadeState;
    fadeMethod mfadeMethod;
    fade_direction_e mfadeDirction;
    unsigned int mfadeTimeUsed;
    unsigned int mfadeTimeTotal;
    unsigned int mfadeFramesUsed;
    unsigned int mfadeFramesTotal;
    int mStartVolume;
    int mCurrentVolume;
    int mTargetVolume;
    int muteCounts;
    unsigned int    samplingRate;   // sampling rate
    unsigned int    channels;       // number of channels
    unsigned int    format;         // Audio format (see audio_format_t in audio.h)

} AudioFade_t;

// fadeMs: time interval need to do fade in/fade out (in million seconds)
// muteFrms: after fade out, how many audio frames need to mute
int AudioFadeInit(AudioFade_t *pAudFade, fadeMethod fade_method, int fadeMs, int muteFrms);

int AudioFadeSetState(AudioFade_t *pAudFade, fade_state_e fadeState);

int AudioFadeBuf(AudioFade_t *pAudFade, void *rawBuf, unsigned int nSamples);

int AudioFadeSetFormat(AudioFade_t *pAudFade,
                       uint32_t        samplingRate,    // sampling rate
                       uint32_t        channels,        // number of channels
                       unsigned int    format      // Audio format (see audio_format_t in audio.h)
                      );

void mutePCMBuf(AudioFade_t *pAudFade, void *rawBuf, unsigned int nSamples);

#endif //__AUDIOFADE_H__

