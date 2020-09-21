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

#define LOG_TAG "aml_audio_malloc"

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <cutils/list.h>
#include <cutils/log.h>
#include "aml_malloc_debug.h"

#define MEMINFO_SHOW_FILENAME  "/data/audio_meminfo"

enum {
    MEMINFO_SHOW_PRINT,
    MEMINFO_SHOW_FILE
};


struct aml_malloc_node {
    struct listnode list;
    char file_name[128];
    uint32_t line;
    void * pointer;
    size_t size;
};

struct aml_malloc_debug {
    struct listnode malloc_list;
    pthread_mutex_t malloc_lock;
};

static struct aml_malloc_debug *gaudio_malloc_handle = NULL;
static char aml_malloc_temp_buf[256];

static void add_malloc_node(struct aml_malloc_node * malloc_node)
{
    struct aml_malloc_debug *pmalloc_handle = NULL;
    pmalloc_handle = gaudio_malloc_handle;

    pthread_mutex_lock(&pmalloc_handle->malloc_lock);
    list_add_tail(&pmalloc_handle->malloc_list, &malloc_node->list);
    pthread_mutex_unlock(&pmalloc_handle->malloc_lock);
    return;
}

static void remove_malloc_item(void* pointer)
{
    struct listnode *node = NULL;
    struct aml_malloc_node * malloc_node = NULL;
    struct aml_malloc_debug *pmalloc_handle = NULL;
    pmalloc_handle = gaudio_malloc_handle;

    pthread_mutex_lock(&pmalloc_handle->malloc_lock);
    list_for_each(node, &pmalloc_handle->malloc_list) {
        malloc_node = node_to_item(node, struct aml_malloc_node, list);
        if (malloc_node->pointer == pointer) {
            list_remove(&malloc_node->list);
            free(malloc_node);
            break;
        }
    }

    pthread_mutex_unlock(&pmalloc_handle->malloc_lock);
    return;
}


void aml_audio_debug_malloc_open(void)
{
    struct aml_malloc_debug *pmalloc_handle = NULL;
    pmalloc_handle = (struct aml_malloc_debug *)malloc(sizeof(struct aml_malloc_debug));
    if (pmalloc_handle == NULL) {
        ALOGE("%s failed", __FUNCTION__);
        return;
    }
    list_init(&pmalloc_handle->malloc_list);
    pthread_mutex_init(&pmalloc_handle->malloc_lock, NULL);
    gaudio_malloc_handle = pmalloc_handle;
    return;
}

void aml_audio_debug_malloc_close(void)
{
    struct aml_malloc_debug *pmalloc_handle = NULL;
    struct aml_malloc_node *malloc_node = NULL;
    struct listnode *node = NULL, *n = NULL;
    pmalloc_handle = gaudio_malloc_handle;
    if (pmalloc_handle == NULL) {
        return;
    }
    pthread_mutex_lock(&pmalloc_handle->malloc_lock);
    list_for_each_safe(node, n, &pmalloc_handle->malloc_list) {
        malloc_node = node_to_item(node, struct aml_malloc_node, list);
        list_remove(&malloc_node->list);
        free(malloc_node);
    }
    pthread_mutex_unlock(&pmalloc_handle->malloc_lock);
    free(pmalloc_handle);
    gaudio_malloc_handle = NULL;
    return;
}
void* aml_audio_debug_malloc(size_t size, char * file_name, uint32_t line)
{
    void * pointer = NULL;
    struct aml_malloc_node * malloc_node = NULL;

    pointer = malloc(size);
    if (pointer == NULL) {
        return NULL;
    }
    malloc_node = (struct aml_malloc_node *)malloc(sizeof(struct aml_malloc_node));
    if (malloc_node == NULL) {
        free(pointer);
        return NULL;
    }

    snprintf(malloc_node->file_name, 128, "malloc=%s", file_name);
    malloc_node->line    = line;
    malloc_node->pointer = pointer;
    malloc_node->size    = size;

    add_malloc_node(malloc_node);
    return pointer;
}

void* aml_audio_debug_realloc(void* pointer, size_t bytes, char * file_name, uint32_t line)
{
    void * new_pointer = NULL;
    struct aml_malloc_node * malloc_node = NULL;

    if (pointer) {
        remove_malloc_item(pointer);
    }

    new_pointer = realloc(pointer, bytes);
    if (new_pointer == NULL) {
        return NULL;
    }
    malloc_node = (struct aml_malloc_node *)malloc(sizeof(struct aml_malloc_node));
    if (malloc_node == NULL) {
        free(new_pointer);
        return NULL;
    }

    snprintf(malloc_node->file_name, 128, "realloc=%s", file_name);
    malloc_node->line    = line;
    malloc_node->pointer = new_pointer;
    malloc_node->size    = bytes;

    add_malloc_node(malloc_node);
    return new_pointer;

}

void* aml_audio_debug_calloc(size_t nmemb, size_t bytes, char * file_name, uint32_t line)
{
    void * pointer = NULL;
    struct aml_malloc_node * malloc_node = NULL;

    pointer = calloc(nmemb, bytes);
    if (pointer == NULL) {
        return NULL;
    }
    malloc_node = (struct aml_malloc_node *)malloc(sizeof(struct aml_malloc_node));
    if (malloc_node == NULL) {
        free(pointer);
        return NULL;
    }

    snprintf(malloc_node->file_name, 128, "calloc:%s", file_name);
    malloc_node->line    = line;
    malloc_node->pointer = pointer;
    malloc_node->size    = nmemb * bytes;

    add_malloc_node(malloc_node);
    return pointer;
}

void aml_audio_debug_free(void* pointer)
{
    if (pointer) {
        remove_malloc_item(pointer);
    }
    free(pointer);
    return;
}

void aml_audio_debug_malloc_showinfo(uint32_t level)
{
    struct listnode *node = NULL;
    struct aml_malloc_node * malloc_node = NULL;
    struct aml_malloc_debug *pmalloc_handle = NULL;
    FILE *fp1 = NULL;
    uint32_t total_mem = 0;
    pmalloc_handle = gaudio_malloc_handle;

    if (level == MEMINFO_SHOW_FILE) {
        fp1 = fopen(MEMINFO_SHOW_FILENAME, "w+");
        if (fp1 == NULL) {
            return;
        }
    }

    pthread_mutex_lock(&pmalloc_handle->malloc_lock);
    list_for_each(node, &pmalloc_handle->malloc_list) {
        malloc_node = node_to_item(node, struct aml_malloc_node, list);
        if (malloc_node) {
            total_mem += malloc_node->size;
            if (level == MEMINFO_SHOW_PRINT) {
                ALOGI("mem info:%s line=%d pointer =%p size=0x%x", malloc_node->file_name, malloc_node->line, malloc_node->pointer, malloc_node->size);
            } else if (level == MEMINFO_SHOW_FILE) {
                if (fp1) {
                    memset(aml_malloc_temp_buf, 0, sizeof(aml_malloc_temp_buf));
                    sprintf(aml_malloc_temp_buf, "mem info:%s line=%d pointer =%p size=0x%x\n", malloc_node->file_name, malloc_node->line, malloc_node->pointer, malloc_node->size);
                    fwrite((char *)aml_malloc_temp_buf, 1, sizeof(aml_malloc_temp_buf), fp1);
                }
            }
        }
    }
    if (level == MEMINFO_SHOW_PRINT) {
        ALOGI("HAL Audio total use mem =0x%x\n", total_mem);
    } else if (level == MEMINFO_SHOW_FILE) {
        memset(aml_malloc_temp_buf, 0, sizeof(aml_malloc_temp_buf));
        sprintf(aml_malloc_temp_buf, "HAL Audio total use mem =0x%x\n", total_mem);
        fwrite((char *)aml_malloc_temp_buf, 1, sizeof(aml_malloc_temp_buf), fp1);
        fclose(fp1);
    }


    pthread_mutex_unlock(&pmalloc_handle->malloc_lock);

    return;
}
