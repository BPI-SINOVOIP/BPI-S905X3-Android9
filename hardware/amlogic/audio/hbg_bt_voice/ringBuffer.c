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

//#define LOG_TAG "audio_hw_blehid"
#include "ringBuffer.h"

#include <android/log.h>
#include <stdlib.h>
#include <string.h>
#define LOGD(fmt, args...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, fmt, ##args)
#define LOGE(fmt, args...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, fmt, ##args)

struct RingBuffer* InitRingBuffer(int len)
{
    struct RingBuffer * ptr = malloc(sizeof(struct RingBuffer));
    if(ptr!=NULL)
    {
        memset(ptr,0, sizeof(struct RingBuffer));
        ptr->buf = malloc(len);
        if(ptr->buf!=NULL)
        {
            ptr->len = len;
            pthread_mutex_init(&(ptr->mutex),NULL);
        }else
        {
            free(ptr);
            ptr = NULL;
        }
    }

    return ptr;
}

void DeInitRingBuffer(struct RingBuffer* ptr)
{
    pthread_mutex_destroy(&(ptr->mutex));
    free(ptr->buf);
    free(ptr);
}

int ReadRingBuffer(struct RingBuffer* ptr, unsigned char *data,int len)
{
    pthread_mutex_lock(&(ptr->mutex));
    if(ptr->validDataLen < len)
    {
        //underflow
        //LOGE("ReadRingBuffer underflow");
        len = ptr->validDataLen;
    }

    if(ptr->end_pos >= ptr->start_pos)
        memcpy(data, ptr->buf+ptr->start_pos,len);
    else
    {
        int tmp = ptr->len - ptr->start_pos;
        if(tmp < len)
        {
            memcpy(data, ptr->buf + ptr->start_pos, tmp);
            memcpy(data+tmp, ptr->buf, len - tmp);
        }
        else
            memcpy(data, ptr->buf + ptr->start_pos, len);
	}

    ptr->start_pos += len;
    if(ptr->start_pos >= ptr->len)
        ptr->start_pos = ptr->start_pos - ptr->len;

    ptr->validDataLen -= len;
    pthread_mutex_unlock(&(ptr->mutex));

    return len;
}


int ReadRingBuffer2(struct RingBuffer* ptr, unsigned char *data,int len)
{
    pthread_mutex_lock(&(ptr->mutex));
    if(ptr->validDataLen < len)
    {
    //underflow
    //LOGE("ReadRingBuffer underflow");
    //len = ptr->validDataLen;
    pthread_mutex_unlock(&(ptr->mutex));
    return 0;
    }

    if(ptr->end_pos >= ptr->start_pos)
        memcpy(data, ptr->buf+ptr->start_pos,len);
    else
    {
        int tmp = ptr->len - ptr->start_pos;
        if(tmp < len)
        {
            memcpy(data, ptr->buf + ptr->start_pos, tmp);
            memcpy(data+tmp, ptr->buf, len - tmp);
        }
        else
            memcpy(data, ptr->buf + ptr->start_pos, len);
	}

    ptr->start_pos += len;
    if(ptr->start_pos >= ptr->len)
        ptr->start_pos = ptr->start_pos - ptr->len;

    ptr->validDataLen -= len;
    pthread_mutex_unlock(&(ptr->mutex));

    return len;
}


int WriteRingBuffer(struct RingBuffer* ptr, unsigned char *data,int len)
{
    int cnt = 0;

    if(data == NULL)
        return -1;

    pthread_mutex_lock(&(ptr->mutex));
    if(ptr->validDataLen + len > ptr->len)
    {
        //overflow
        //LOGE("WriteRingBuffer overflow");
        pthread_mutex_unlock(&(ptr->mutex));
        return 0;
    }

    if(ptr->end_pos + len >= ptr->len)
    {
        cnt = ptr->len - ptr->end_pos;
        memcpy(ptr->buf + ptr->end_pos, data, cnt);
        memcpy(ptr->buf, data + cnt, len - cnt);
        ptr->end_pos = len - cnt;
    }
    else
    {
        memcpy(ptr->buf + ptr->end_pos, data, len);
        ptr->end_pos += len;
    }
    ptr->validDataLen += len;

    pthread_mutex_unlock(&(ptr->mutex));
    return len;
}

void ResetRingBuffer(struct RingBuffer* ptr)
{
    ptr->start_pos = 0;
    ptr->end_pos = 0;
    ptr->validDataLen = 0;
}
