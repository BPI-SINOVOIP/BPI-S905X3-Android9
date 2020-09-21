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
 *  DESCRIPTION
 *      brief  Definitiond Of Audio Out Structures.
 *
 */

#ifndef AUDIO_OUT_H
#define AUDIO_OUT_H

#include <adec-macros.h>

ADEC_BEGIN_DECLS

struct aml_audio_dec;

typedef struct {
    void *private_data;
    void *private_data_raw;
    int (*init)(struct aml_audio_dec *);
    int (*start)(struct aml_audio_dec *);
    int (*pause)(struct aml_audio_dec *);
    int (*resume)(struct aml_audio_dec *);
    int (*stop)(struct aml_audio_dec *);
    unsigned long(*latency)(struct aml_audio_dec *);                    /* get latency in ms */
    int (*mute)(struct aml_audio_dec *, adec_bool_t);           /* 1: enable mute ; 0: disable mute */
    int (*set_volume)(struct aml_audio_dec *, float);
    int (*set_lrvolume)(struct aml_audio_dec *, float, float);
    int (*set_track_rate)(struct aml_audio_dec *, void *rate);
    int (*get_out_position)(struct aml_audio_dec *, int64_t *position,
         int64_t * time_us);
    int audio_out_raw_enable;
    float track_rate;
} audio_out_operations_t;

ADEC_END_DECLS

#endif
