/*
 * Copyright (C) 2019-2020 HAOBO Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __RIGN_BUFFER_H
#define __RIGN_BUFFER_H

#include <pthread.h>
#include <stdio.h>


#ifdef __cplusplus
extern "C" {
#endif

struct RingBuffer{
    unsigned char *buf;
    int len;
    int start_pos, end_pos;
    int validDataLen;
    pthread_mutex_t mutex;
};

struct RingBuffer* InitRingBuffer(int len);
void DeInitRingBuffer(struct RingBuffer* ptr);
int ReadRingBuffer(struct RingBuffer* ptr, unsigned char *data,int len);
int WriteRingBuffer(struct RingBuffer* ptr, unsigned char *data,int len);
void ResetRingBuffer(struct RingBuffer* ptr);
int ReadRingBuffer2(struct RingBuffer* ptr, unsigned char *data,int len);
#ifdef __cplusplus
}
#endif

#endif /* __RIGN_BUFFER_H */
