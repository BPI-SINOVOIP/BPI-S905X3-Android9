/*
 * Copyright (C) 2017 Amlogic Corporation.
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

#ifndef _BUFFER_PROVIDER_H
#define _BUFFER_PROVIDER_H
#endif

#ifdef __cplusplus
extern "C" {
#endif

int init_buffer_provider();
int release_buffer_provider();
struct ring_buffer *get_general_buffer();
struct ring_buffer *get_DDP_buffer();
struct ring_buffer *get_DD_61937_buffer();

#ifdef __cplusplus
}
#endif
