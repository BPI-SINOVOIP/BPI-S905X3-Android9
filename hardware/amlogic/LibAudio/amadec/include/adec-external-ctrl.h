/*
 * Copyright (C) 2018 Amlogic Corporation.
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
 *
 *  DESCRIPTION:
 *     brief  Audio Dec Lib Functions.
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
