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
 */

#ifndef ADEC_ASSOC_AUDIO_H_
#define ADEC_ASSOC_AUDIO_H_

#define VALID_PID(_pid_) ((_pid_)>0 && (_pid_)<0x1fff)
/* currently we only support EAC3/AC3 */
#define VALID_AD_FMT(fmt)  ((fmt == ACODEC_FMT_EAC3) || (fmt == ACODEC_FMT_AC3) || (fmt == ACODEC_FMT_MPEG) || (fmt == ACODEC_FMT_AAC))

void dtv_assoc_set_main_frame_size(int main_frame_size);
void dtv_assoc_get_main_frame_size(int* main_frame_size);
void dtv_assoc_set_ad_frame_size(int ad_frame_size);
void dtv_assoc_get_ad_frame_size(int* ad_frame_size);
void dtv_assoc_audio_cache(int value);
int dtv_assoc_audio_start(unsigned int handle,int pid,int fmt);
void dtv_assoc_audio_stop(unsigned int handle);
//void dtv_assoc_audio_pause(unsigned int handle);
//void dtv_assoc_audio_resume(unsigned int handle,int pid);
int dtv_assoc_get_avail(void);
int dtv_assoc_read(unsigned char *data, int size);
int dtv_assoc_init(void);
int dtv_assoc_deinit(void);

#endif

