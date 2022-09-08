/*
 * Copyright (C) 2019 Amlogic Corporation.
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

#ifndef _AML_MMAP_AUDIO_H_
#define _AML_MMAP_AUDIO_H_

#include <ion/ion.h>

typedef struct AML_MMAP_THREAD_PARAM {
    pthread_t               threadId;
    bool                    bExitThread;
    bool                    bStopPlay;
    int                     status;
    pthread_condattr_t      condAttr;
    pthread_mutex_t         mutex;
    pthread_cond_t          cond;
} aml_mmap_thread_param_st;

typedef struct AML_MMAP_AUDIO_PARAM {
    unsigned char               *pu8MmapAddr;
    ion_user_handle_t           hIonHanndle;
    int                         s32IonFd;
    int                         s32IonShareFd;
    unsigned int                u32FramePosition;
    int64_t                     time_nanoseconds;
    aml_mmap_thread_param_st    stThreadParam;
} aml_mmap_audio_param_st;



int outMmapInit(struct aml_stream_out *out);
int outMmapDeInit(struct aml_stream_out *out);

#endif
