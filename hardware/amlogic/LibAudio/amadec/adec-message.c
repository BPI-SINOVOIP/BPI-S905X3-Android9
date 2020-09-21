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
 *      brief  Audio Dec Message Handlers.
 *
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <audio-dec.h>
#include <amthreadpool.h>

#define LOG_TAG "adec-message"
/**
 * \brief message is empty or not
 * \param pool pointer to message pool
 * \return TREU when empty, otherwise FALSE
 */
adec_bool_t message_pool_empty(aml_audio_dec_t *audec)
{
    message_pool_t *pool = &audec->message_pool;

    if (pool->message_num == 0) {
        return TRUE;
    } else {
        return FALSE;
    }
}

/**
 * \brief init the adec message pool
 * \param audec pointer to audec
 * \return 0 on success
 */
int adec_message_pool_init(aml_audio_dec_t *audec)
{
    message_pool_t *pool;

    pool = &audec->message_pool;
    pool->message_in_index = 0;
    pool->message_out_index  = 0;
    pool->message_num = 0;
    memset(pool->message_lise, 0, sizeof(pool->message_lise));
    pthread_mutex_init(&pool->msg_mutex, NULL);

    return 0;
}

/**
 * \brief alloc new message
 * \return pointer to the new message
 */
adec_cmd_t *adec_message_alloc(void)
{
    adec_cmd_t *cmd;

    cmd = (adec_cmd_t *)malloc(sizeof(adec_cmd_t));
    if (cmd) {
        memset(cmd, 0, sizeof(adec_cmd_t));
    }

    return cmd;
}

/**
 * \brief free the message
 * \param cmd pointer to message
 * \return 0 on success
 */
int adec_message_free(adec_cmd_t *cmd)
{
    if (cmd) {
        free(cmd);
    }

    cmd = NULL;

    return 0;
}

/**
 * \brief send a message
 * \param audec pointer to audec
 * \param cmd pointer to message
 * \return 0 on success otherwise -1
 */
int adec_send_message(aml_audio_dec_t *audec, adec_cmd_t *cmd)
{

    int ret = -1;
    message_pool_t *pool;

    pool = &audec->message_pool;

    int retry_count = 0;
    adec_thread_wakeup(audec);
    while (pool->message_num > MESSAGE_NUM_MAX / 2) {
        usleep(1000 * 10);
        if (retry_count++ > (pool->message_num - MESSAGE_NUM_MAX / 2) * 10) {
            break;
        }
    }

    pthread_mutex_lock(&pool->msg_mutex);
    if (pool->message_num < MESSAGE_NUM_MAX) {
        pool->message_lise[pool->message_in_index] = cmd;
        pool->message_in_index = (pool->message_in_index + 1) % MESSAGE_NUM_MAX;
        pool->message_num += 1;
        ret = 0;
    } else {
        /* message pool is full */
        adec_print("message pool is full! delete the oldest message!");
        adec_cmd_t *oldestcmd;
        oldestcmd = pool->message_lise[pool->message_in_index];
        free(oldestcmd);
        pool->message_out_index = (pool->message_out_index + 1) % MESSAGE_NUM_MAX;
        pool->message_lise[pool->message_in_index] = cmd;
        pool->message_in_index = (pool->message_in_index + 1) % MESSAGE_NUM_MAX;
        ret = 0;
    }
    amthreadpool_thread_wake(audec->thread_pid);
    pthread_mutex_unlock(&pool->msg_mutex);

    return ret;
}

/**
 * \brief get a message
 * \param audec pointer to audec
 * \return pointer to message otherwise NULL if an error occurred
 */
adec_cmd_t *adec_get_message(aml_audio_dec_t *audec)
{

    message_pool_t *pool;
    adec_cmd_t *cmd = NULL;

    pool = &audec->message_pool;
    if (!pool) {
        adec_print("message pool is null! get message failed!");
        return NULL;
    }

    pthread_mutex_lock(&pool->msg_mutex);
    if (pool->message_num > 0) {
        cmd = pool->message_lise[pool->message_out_index];
        pool->message_out_index = (pool->message_out_index + 1) % MESSAGE_NUM_MAX;
        pool->message_num -= 1;
    }
    pthread_mutex_unlock(&pool->msg_mutex);

    return cmd;
}
