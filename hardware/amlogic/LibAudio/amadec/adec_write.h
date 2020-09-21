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

#ifndef __ADEC_WRITE_H__
#define __ADEC_WRITE_H__

#include <pthread.h>
#include <stdlib.h>
#include<stdio.h>
#include <string.h>

#define DEFAULT_BUFFER_SIZE 1024*1024
//#define MIN(a,b) (a>b)?b:a

typedef struct buffer_stream_st {
    int buf_length;
    int buf_level;
    unsigned char * data;
    unsigned char * rd_ptr;
    unsigned char * wr_ptr;
    int bInited;
    int nMutex;
    int data_width;
    int channels;
    int samplerate;
    int format;
    pthread_mutex_t  nMutex1;

} buffer_stream_t;

int init_buff(buffer_stream_t *bs, int length);
int reset_buffer(buffer_stream_t *bs);
int release_buffer(buffer_stream_t *bs);

int is_buffer_empty(buffer_stream_t *bs);

int is_buffer_full(buffer_stream_t *bs);
int get_buffer_length(buffer_stream_t *bs);
int read_pcm_buffer(char * out, buffer_stream_t *bs, int size);
int write_pcm_buffer(char * in, buffer_stream_t *bs, int size);
int get_pcmbuf_level();


#endif
