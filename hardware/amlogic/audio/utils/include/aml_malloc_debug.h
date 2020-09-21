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


#ifndef _AML_MALLOC_DEBUG_H
#define _AML_MALLOC_DEBUG_H
#include <stdlib.h>


//#define AML_MALLOC_DEBUG
#ifdef AML_MALLOC_DEBUG

#define aml_audio_malloc(x) aml_audio_debug_malloc(x, __FILE__, __LINE__)
#define aml_audio_free(x)   aml_audio_debug_free(x)
#define aml_audio_realloc(x, y) aml_audio_debug_realloc(x, y, __FILE__, __LINE__)
#define aml_audio_calloc(x, y) aml_audio_debug_calloc(x, y, __FILE__, __LINE__)
#else
#define aml_audio_malloc(x) malloc(x)
#define aml_audio_free(x)   free(x)
#define aml_audio_realloc(x, y) realloc(x, y)
#define aml_audio_calloc(x, y) calloc(x, y)
#endif

void aml_audio_debug_malloc_open(void);
void aml_audio_debug_malloc_close(void);
void* aml_audio_debug_malloc(size_t size, char * file_name, uint32_t line);
void* aml_audio_debug_realloc(void* pointer, size_t bytes, char * file_name, uint32_t line);
void* aml_audio_debug_calloc(size_t nmemb, size_t bytes, char * file_name, uint32_t line);
void aml_audio_debug_free(void* pointer);
void aml_audio_debug_malloc_showinfo(uint32_t level);

#endif
