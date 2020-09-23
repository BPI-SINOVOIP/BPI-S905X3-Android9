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

#ifndef _AML_Treble_Bass_H_
#define _AML_Treble_Bass_H_
#include <stdint.h>

#ifdef __cplusplus
extern "C"  {
#endif

int audio_Treble_Bass_process(int16_t *input, int16_t *output, int frame_length);
void audio_Treble_Bass_init(float bass_gain, float treble_gain);

#ifdef __cplusplus
}
#endif

#endif
