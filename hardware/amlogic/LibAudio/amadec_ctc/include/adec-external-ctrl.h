/**
 * \file adec-external-ctrl.h
 * \brief  Function prototypes of Audio Dec
 * \version 1.0.0
 * \date 2011-03-08
 */
/* Copyright (C) 2007-2011, Amlogic Inc.
 * All right reserved
 *
 */
#ifndef ADEC_EXTERNAL_H
#define ADEC_EXTERNAL_H

#ifdef  __cplusplus
extern "C"
{
#endif

    int audio_decode_init(void **handle, arm_audio_info *pcodec);
    int audio_decode_start(void *handle);
    int audio_decode_pause(void *handle);
    int audio_decode_resume(void *handle);
    int audio_decode_stop(void *handle);
    int audio_decode_release(void **handle);
    int audio_decode_automute(void *, int);
    int audio_decode_set_mute(void *handle, int);
    int audio_decode_set_volume(void *, float);
    int audio_decode_get_volume(void *, float *);
    int audio_decode_set_pre_gain(void *, float);
    int audio_decode_get_pre_gain(void *, float *);
    int audio_decode_set_pre_mute(void *, uint);
    int audio_decode_get_pre_mute(void *, uint *);
    int audio_channels_swap(void *);
    int audio_channel_left_mono(void *);
    int audio_channel_right_mono(void *);
    int audio_channel_stereo(void *);
    int audio_output_muted(void *handle);
    int audio_dec_ready(void *handle);
    int audio_get_decoded_nb_frames(void *handle);

    int audio_decode_set_lrvolume(void *, float lvol, float rvol);
    int audio_decode_get_lrvolume(void *, float* lvol, float* rvol);
    int audio_set_av_sync_threshold(void *, int);
    int audio_get_soundtrack(void *, int*);
    int get_audio_decoder(void);
    int get_decoder_status(void *p, struct adec_status *adec);
    int get_decoder_info(void *p);
    int audio_channel_lrmix_flag_set(void *, int enable);
    int audio_decpara_get(void *handle, int *pfs, int *pch,int *lfepresent);
    int audio_get_format_supported(int format);
    int audio_get_pts(void* handle);
    int audio_set_skip_bytes(void* handle, unsigned int bytes);
    int audio_get_pcm_level(void* handle);
    int audio_get_decoded_pcm_delay(void *handle);
    int audio_decode_basic_init(void);
    int audio_decoder_set_trackrate(void* handle, void *rate);
    int audio_decoder_get_enable_status(void* handle);
    int audio_set_associate_enable(void *handle, unsigned int enable);
    int audio_send_associate_data(void *handle, uint8_t *buf, size_t size);
    int audio_get_basic_info(void *handle, arm_audio_info *a_ainfo);
#ifdef  __cplusplus
}
#endif

#endif
