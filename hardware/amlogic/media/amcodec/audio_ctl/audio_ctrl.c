/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */


/**
* @file audio_ctrl.c
* @brief  codec control lib functions for audio
* 
* @version 1.0.0
* @date 2011-02-24
*/
/* Copyright (C) 2007-2011, Amlogic Inc.
* All right reserved
*
*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <codec_error.h>
#include <codec.h>
#include "codec_h_ctrl.h"
#include "adec-external-ctrl.h"

void audio_basic_init(void)
{
    audio_decode_basic_init();
}

/* --------------------------------------------------------------------------*/
/**
* @brief  audio_start  Start audio decoder
*/
/* --------------------------------------------------------------------------*/
void audio_start(void **priv, arm_audio_info *a_ainfo)
{

    audio_decode_init(priv, a_ainfo);
}

/* --------------------------------------------------------------------------*/
/**
* @brief  audio_stop  Stop audio decoder
*/
/* --------------------------------------------------------------------------*/
void audio_stop(void **priv)
{
    audio_decode_stop(*priv);
    audio_decode_release(priv);
}

/* --------------------------------------------------------------------------*/
/**
* @brief  audio_stop_async  Stop audio decoder async without wait.
*/
/* --------------------------------------------------------------------------*/
void audio_stop_async(void **priv)
{
    audio_decode_stop(*priv);
}

/* --------------------------------------------------------------------------*/
/**
* @brief  audio_pause  Pause audio decoder
*/
/* --------------------------------------------------------------------------*/
void audio_pause(void *priv)
{
    audio_decode_pause(priv);
}

/* --------------------------------------------------------------------------*/
/**
* @brief  audio_resume  Resume audio decoder
*/
/* --------------------------------------------------------------------------*/
void audio_resume(void *priv)
{
    audio_decode_resume(priv);
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_get_mutesta  Get codec mute status
*
* @param[in]  p  Pointer of codec parameter structure
*
* @return     audio command result
*/
/* --------------------------------------------------------------------------*/
int codec_get_mutesta(codec_para_t *p)
{
    int ret;
    ret = audio_output_muted(p->adec_priv);
    return ret;
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_set_mute  Set audio mute
*
* @param[in]  p     Pointer of codec parameter structure
* @param[in]  mute  mute command, 1 for mute, 0 for unmute
*
* @return     audio command result
*/
/* --------------------------------------------------------------------------*/
int codec_set_mute(codec_para_t *p, int mute)
{
    int ret;

    /* 1: mut output. 0: unmute output */
    ret = audio_decode_set_mute(p->adec_priv, mute);

    return ret;
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_get_volume_range  Get audio volume range
*
* @param[in]   p    Pointer of codec parameter structure
* @param[out]  min  Data to save the min volume
* @param[out]  max  Data to save the max volume
*
* @return      not used, read failed
*/
/* --------------------------------------------------------------------------*/
int codec_get_volume_range(codec_para_t *p, int *min, int *max)
{
    if (p != NULL && min != NULL && max != NULL)
        return -CODEC_ERROR_IO;

    return -CODEC_ERROR_PARAMETER;
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_set_volume  Set audio volume
*
* @param[in]  p    Pointer of codec parameter structure
* @param[in]  val  Volume to be set
*
* @return     command result
*/
/* --------------------------------------------------------------------------*/
int codec_set_volume(codec_para_t *p, float val)
{
    int ret;

    ret = audio_decode_set_volume(p->adec_priv, val);
    return ret;
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_get_volume  Get audio volume
*
* @param[in]  p  Pointer of codec parameter structure
*
* @return     command result
*/
/* --------------------------------------------------------------------------*/
int codec_get_volume(codec_para_t *p, float *val)
{
    int ret;
    ret = audio_decode_get_volume(p->adec_priv, val);
    return ret;
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_set_pre_gain  Set audio decoder pre-gain
*
* @param[in]  p    Pointer of codec parameter structure
* @param[in]  gain  gain to be set
*
* @return     command result
*/
/* --------------------------------------------------------------------------*/
int codec_set_pre_gain(codec_para_t *p, float gain)
{
    int ret;

    ret = audio_decode_set_pre_gain(p->adec_priv, gain);
    return ret;
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_get_pre_gain  Get audio decoder pre-gain
*
* @param[in]  p  Pointer of codec parameter structure
*
* @return     command result
*/
/* --------------------------------------------------------------------------*/
int codec_get_pre_gain(codec_para_t *p, float *gain)
{
    int ret;
    ret = audio_decode_get_pre_gain(p->adec_priv, gain);
    return ret;
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_set_pre_mute  Set audio decoder pre-mute
*
* @param[in]  p    Pointer of codec parameter structure
* @param[in]  gain  gain to be set
*
* @return     command result
*/
/* --------------------------------------------------------------------------*/
int codec_set_pre_mute(codec_para_t *p, uint mute)
{
    int ret;

    ret = audio_decode_set_pre_mute(p->adec_priv, mute);
    return ret;
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_get_pre_mute  Get audio decoder pre-mute
*
* @param[in]  p  Pointer of codec parameter structure
*
* @return     command result
*/
/* --------------------------------------------------------------------------*/
int codec_get_pre_mute(codec_para_t *p, uint *mute)
{
    int ret;
    ret = audio_decode_get_pre_mute(p->adec_priv, mute);
    return ret;
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_set_volume  Set audio volume seperately
*
* @param[in]  p    Pointer of codec parameter structure
* @param[in]  lvol  left Volume to be set
* @param[in]  rvol  right Volume to be set
*
* @return     command result
*/
/* --------------------------------------------------------------------------*/
int codec_set_lrvolume(codec_para_t *p, float lvol, float rvol)
{
    int ret;

    ret = audio_decode_set_lrvolume(p->adec_priv, lvol, rvol);
    return ret;
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_get_volume  Get audio left and right volume seperately
*
* @param[in]  p  Pointer of codec parameter structure
*
* @return     command result
*/
/* --------------------------------------------------------------------------*/
int codec_get_lrvolume(codec_para_t *p, float *lvol, float* rvol)
{
    int ret;
    ret = audio_decode_get_lrvolume(p->adec_priv, lvol, rvol);
    return ret;
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_set_volume_balance  Set volume balance
*
* @param[in]  p        Pointer of codec parameter structure
* @param[in]  balance  Balance to be set
*
* @return     not used, return failed
*/
/* --------------------------------------------------------------------------*/
int codec_set_volume_balance(codec_para_t *p, int balance)
{
    int bl;
    if (p != NULL) {
       bl = balance;
       return -CODEC_ERROR_IO;
    }
    return -CODEC_ERROR_PARAMETER;
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_swap_left_right  Swap audio left and right channel
*
* @param[in]  p  Pointer of codec parameter structure
*
* @return     Command result
*/
/* --------------------------------------------------------------------------*/
int codec_swap_left_right(codec_para_t *p)
{
    int ret;
    ret = audio_channels_swap(p->adec_priv);
    return ret;
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_left_mono  Set mono with left channel
*
* @param[in]  p  Pointer of codec parameter structure
*
* @return     Command result
*/
/* --------------------------------------------------------------------------*/
int codec_left_mono(codec_para_t *p)
{
    int ret;
    ret = audio_channel_left_mono(p->adec_priv);
    return ret;
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_right_mono  Set mono with right channel
*
* @param[in]  p  Pointer of codec parameter structure
*
* @return     Command result
*/
/* --------------------------------------------------------------------------*/
int codec_right_mono(codec_para_t *p)
{
    int ret;
    ret = audio_channel_right_mono(p->adec_priv);
    return ret;
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_stereo  Set stereo
*
* @param[in]  p  Pointer of codec parameter structure
*
* @return     Command result
*/
/* --------------------------------------------------------------------------*/
int codec_stereo(codec_para_t *p)
{
    int ret;
    ret = audio_channel_stereo(p->adec_priv);
    return ret;
}

/* @brief  codec_lr_mix
* @param[in]  p  Pointer of codec parameter structure
*
* @return     Command result
*/
/* --------------------------------------------------------------------------*/
int codec_lr_mix_set(codec_para_t *p, int enable)
{
    int ret;
    ret = audio_channel_lrmix_flag_set(p->adec_priv, enable);
    return ret;
}

int codec_pcmpara_Applied_get(codec_para_t *p, int *pfs, int *pch,int *lfepresent)
{
    int ret;
    ret = audio_decpara_get(p->adec_priv, pfs, pch,lfepresent);
    return ret;
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_audio_automute  Set decoder to automute mode
*
* @param[in]  auto_mute  automute mode
*
* @return     Command result
*/
/* --------------------------------------------------------------------------*/
int codec_audio_automute(void *priv, int auto_mute)
{
    int ret;
    //char buf[16];
    //sprintf(buf,"automute:%d",auto_mute);
    //ret=amadec_cmd(buf);
    ret = audio_decode_automute(priv, auto_mute);
    return ret;
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_audio_spectrum_switch   Switch audio spectrum
*
* @param[in]  p         Pointer of codec parameter structure
* @param[in]  isStart   Start(1) or stop(0) spectrum
* @param[in]  interval  Spectrum interval
*
* @return     Command result
*/
/* --------------------------------------------------------------------------*/
int codec_audio_spectrum_switch(codec_para_t *p, int isStart, int interval)
{
    int  ret = -1;
    char cmd[32];
    if (p == NULL)
        return -CODEC_ERROR_PARAMETER;
    if (isStart == 1) {
        snprintf(cmd, 32, "spectrumon:%d", interval);
        //ret=amadec_cmd(cmd);
    } else if (isStart == 0) {
        //ret=amadec_cmd("spectrumoff");
    }

    return ret;
}
int codec_get_soundtrack(codec_para_t *p, int* strack)
{
    return audio_get_soundtrack(p->adec_priv, strack);

}

int audio_set_avsync_threshold(void *priv, int threshold)
{
    return audio_set_av_sync_threshold(priv, threshold);
}
int codec_get_decoder_enable(codec_para_t *p)
{
    return audio_decoder_get_enable_status(p->adec_priv);
}
int codec_set_track_rate(codec_para_t *p,void *rate)
{
    return audio_decoder_set_trackrate(p->adec_priv,rate);
}

