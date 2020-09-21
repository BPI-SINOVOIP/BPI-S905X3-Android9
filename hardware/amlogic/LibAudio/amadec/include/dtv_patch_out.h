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
 */

#ifndef DTV_PATCH_OUT_H
#define DTV_PATCH_OUT_H

#define OUTPUT_BUFFER_SIZE (6 * 1024)

typedef enum {
    BUFFER_LEVEL = 0,
    BUFFER_SPACE,
    AD_MIXING_LEVLE
}INFO_TYPE_E;


typedef int (*out_pcm_write)(unsigned char *pcm_data, int size, int symbolrate, int channel, void *args);
typedef int (*out_raw_wirte)(unsigned char *raw_data, int size,
                             void *args);

/*[SE][BUG][SWPL-14813][chengshun.wang] modify api to get output level,
 * and decide whether read raw data or pcm data to ring buffer*/
typedef int (*out_get_wirte_buffer_info)(void *args, INFO_TYPE_E info_flag);
typedef int (*out_audio_info)(void *args,unsigned char ori_channum,unsigned char lfepresent);

int dtv_patch_input_open(unsigned int *handle, out_pcm_write pcmcb,
                         out_get_wirte_buffer_info buffercb,
                         out_audio_info info_cb,void *args);
int dtv_patch_input_start(unsigned int handle, int aformat, int has_video,bool associate_dec_supported,bool associate_audio_mixing_enable,int dual_decoder_mixing_level);
int dtv_patch_input_stop(unsigned int handle);
int dtv_patch_input_pause(unsigned int handle);
int dtv_patch_input_resume(unsigned int handle);
unsigned long dtv_patch_get_pts(void);

#endif
